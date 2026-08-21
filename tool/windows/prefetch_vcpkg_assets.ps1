# Pre-populates vcpkg-root's downloads/ folder from BART's vcpkg-assets/
# mirror before vcpkg install ever runs. vcpkg checks that folder for the
# exact distfile name before invoking any downloader (including
# X_VCPKG_ASSET_SOURCES=x-script) -- a file already sitting there is used
# directly with zero network call during the build, so this replaces N
# separate runtime BART round-trips (each individually exposed to this
# network's flakiness, see fetch_vcpkg_asset.ps1) with one bulk pass up
# front. Best-effort only: anything this can't fetch (BART unreachable, a
# specific asset missing, one download failing) just falls through to
# fetch_vcpkg_asset.ps1's own runtime check/fetch/publish path unchanged --
# this script must never fail the build.
param(
  [Parameter(Mandatory = $true)][string]$VcpkgRoot
)

Write-Host "prefetch_vcpkg_assets.ps1: starting (VcpkgRoot=$VcpkgRoot)"

if (-not ($env:BART_CACHE_URL -and $env:BART_API_KEY)) {
  Write-Host "prefetch_vcpkg_assets.ps1: BART not configured -- skipping"
  return
}

$headers = @{ "X-JFrog-Art-Api" = $env:BART_API_KEY }
$assetsUri = "$($env:BART_CACHE_URL)/vcpkg-assets"
# Artifactory's storage API (distinct from the plain repo path used to
# actually fetch/publish files) is the one that returns folder contents as
# JSON -- see AGENTS.md/commit history for why cache_cleanup.yml's cleanup
# step also had to use this same endpoint to see real folder entries.
$storageListUri = ($env:BART_CACHE_URL -replace '/artifactory/', '/artifactory/api/storage/') + '/vcpkg-assets'

$downloadsDir = Join-Path $VcpkgRoot 'downloads'
New-Item -ItemType Directory -Force -Path $downloadsDir | Out-Null

Write-Host "prefetch_vcpkg_assets.ps1: listing $storageListUri"
try {
  $ProgressPreference = 'SilentlyContinue'
  # Invoke-WebRequest's .Content comes back as a raw byte[] (not a string)
  # for this response -- Artifactory's JSON here has no charset on its
  # Content-Type, and both Windows PowerShell 5.1 and pwsh 7 fall back to
  # binary in that case. Piping a byte[] into ConvertFrom-Json silently
  # converts each individual byte number instead of the actual JSON text,
  # giving an empty result with no error (confirmed on a real run and
  # reproduced locally). Invoke-RestMethod parses JSON directly and
  # doesn't have this ambiguity.
  $parsed = Invoke-RestMethod -Uri $storageListUri -Headers $headers -TimeoutSec 30
  $children = $parsed.children
} catch {
  Write-Host "prefetch_vcpkg_assets.ps1: couldn't list BART vcpkg-assets/ folder ($($_.Exception.Message)) -- skipping"
  if ($_.Exception.Response) {
    Write-Host "prefetch_vcpkg_assets.ps1: HTTP status $([int]$_.Exception.Response.StatusCode)"
  }
  return
}

Write-Host "prefetch_vcpkg_assets.ps1: found $($children.Count) entries in BART vcpkg-assets/"

$prefetched = 0
$failed = 0
foreach ($child in $children) {
  if ($child.folder) { continue }
  $name = $child.uri.TrimStart('/')
  $dst = Join-Path $downloadsDir $name
  if (Test-Path $dst) { continue }

  $ok = $false
  for ($attempt = 1; $attempt -le 3; $attempt++) {
    try {
      $ProgressPreference = 'SilentlyContinue'
      Invoke-WebRequest -Uri "$assetsUri/$name" -Headers $headers -OutFile $dst -TimeoutSec 60
      $ok = $true
      break
    } catch {
      Remove-Item $dst -Force -ErrorAction SilentlyContinue
      if ($attempt -lt 3) { Start-Sleep -Seconds 3 }
    }
  }
  if ($ok) {
    $prefetched++
    Write-Host "prefetch_vcpkg_assets.ps1: prefetched $name from BART"
  } else {
    $failed++
    Write-Host "prefetch_vcpkg_assets.ps1: prefetch of $name failed (non-fatal, will fall back to the per-asset fetch at install time)"
  }
}

Write-Host "prefetch_vcpkg_assets.ps1: done -- $prefetched prefetched, $failed failed"
