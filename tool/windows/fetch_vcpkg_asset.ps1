# Invoked by vcpkg itself via X_VCPKG_ASSET_SOURCES=x-script (set in
# windows.yml's "Configure and build" step and tool/windows/vcpkg_cmake.ps1)
# for every port source download, replacing vcpkg's own internal libcurl
# call, which can never be configured to skip direct access's schannel
# CRYPT_E_NO_REVOCATION_CHECK the way curl.exe run as a real subprocess can
# (--ssl-no-revoke is a CLI-only flag). vcpkg checks the resulting file's
# sha512 itself after this script returns, so this doesn't need to verify
# $Sha512 on its own.
#
# Some upstream sources are simply unreachable from this network on any
# path (confirmed on a real run: automake-1.17.tar.gz failed from every
# mirror vcpkg itself tries, gnu.org included) -- self-heal by mirroring
# through BART instead of retrying the same dead source every run: check
# BART first, and on a genuine fetch from $Url, push a copy there so the
# next build hits BART instead of repeating this. BART is a cache, not a
# system of record -- an asset that's never been fetched successfully by
# anyone yet still needs a one-time manual seed (see AGENTS.md/commit
# history for how automake-1.17.tar.gz was seeded).
#
# Best-effort only when BART_CACHE_URL/BART_API_KEY are set (both empty for
# tool/windows/vcpkg_cmake.ps1's local-dev-tool callers, who have no reason
# to have BART credentials) -- falls straight through to a direct fetch.
param(
  [Parameter(Mandatory = $true)][string]$Url,
  [Parameter(Mandatory = $true)][string]$Sha512,
  [Parameter(Mandatory = $true)][string]$Dst
)

. (Join-Path $PSScriptRoot 'lib_retry.ps1')

$bartConfigured = [bool]($env:BART_CACHE_URL -and $env:BART_API_KEY)
$assetName = [System.IO.Path]::GetFileName(($Dst -replace '\.\d+\.part$', ''))
$bartUri = if ($bartConfigured) { "$($env:BART_CACHE_URL)/vcpkg-assets/$assetName" } else { $null }

if ($bartConfigured) {
  # A single attempt turned real seeded assets into fallback-to-origin
  # misses on a real run (BART itself blipped, not a genuine 404) -- retry
  # like every other network call in this file/lib_retry.ps1 before
  # accepting a miss.
  $bartHit = $false
  for ($attempt = 1; $attempt -le 3; $attempt++) {
    try {
      $ProgressPreference = 'SilentlyContinue'
      Invoke-WebRequest -Uri $bartUri -Headers @{ "X-JFrog-Art-Api" = $env:BART_API_KEY } -OutFile $Dst -TimeoutSec 30
      Write-Host "vcpkg asset mirror hit: $assetName"
      $bartHit = $true
      break
    } catch {
      if ($attempt -eq 3) {
        Write-Host "vcpkg asset mirror miss for $assetName -- fetching from $Url"
      } else {
        Start-Sleep -Seconds 3
      }
    }
  }
  if ($bartHit) { exit 0 }
}

Invoke-CurlRetry -Url $Url -OutFile $Dst

if ($bartConfigured) {
  $mirrored = $false
  for ($attempt = 1; $attempt -le 3; $attempt++) {
    try {
      $ProgressPreference = 'SilentlyContinue'
      Invoke-WebRequest -Method Put -Uri $bartUri -Headers @{ "X-JFrog-Art-Api" = $env:BART_API_KEY } -InFile $Dst -TimeoutSec 60 | Out-Null
      Write-Host "mirrored $assetName to BART for next time"
      $mirrored = $true
      break
    } catch {
      if ($attempt -eq 3) {
        Write-Host "failed to mirror $assetName to BART (non-fatal): $($_.Exception.Message)"
      } else {
        Start-Sleep -Seconds 3
      }
    }
  }
}
