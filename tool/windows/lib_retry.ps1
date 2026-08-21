# Shared PowerShell network-retry helpers for windows.yml and
# tool/windows/vcpkg_cmake.ps1. Direct github.com access from this pool is
# intermittently flaky at the plain connect level, not just the SSL-
# revocation-check level fetch_vcpkg_asset.ps1 works around -- a plain git
# clone/fetch or curl download can time out even though the same host is
# reachable moments later. Retry instead of failing the whole job on one
# transient blip. Do NOT force a proxy in here to "fix" this -- confirmed on
# a real run, this pool's proxy is itself unreliable and turns a fixable
# SSL error into an unfixable connect timeout instead.

function Invoke-GitRetry {
  param(
    [Parameter(Mandatory = $true)][string[]]$GitArgs,
    [int]$MaxAttempts = 5,
    [int]$DelaySeconds = 5
  )
  for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
    & git @GitArgs
    if ($LASTEXITCODE -eq 0) { return }
    if ($attempt -eq $MaxAttempts) {
      throw "git $($GitArgs -join ' ') failed with exit $LASTEXITCODE after $MaxAttempts attempts"
    }
    Write-Warning "git $($GitArgs -join ' ') failed (exit $LASTEXITCODE), attempt $attempt/$MaxAttempts -- retrying in ${DelaySeconds}s"
    Start-Sleep -Seconds $DelaySeconds
  }
}

function Invoke-CurlRetry {
  param(
    [Parameter(Mandatory = $true)][string]$Url,
    [Parameter(Mandatory = $true)][string]$OutFile,
    [int]$MaxAttempts = 5,
    [int]$DelaySeconds = 5
  )
  for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
    if (Test-Path $OutFile) { Remove-Item $OutFile -Force }
    # --ssl-no-revoke: curl.exe run as a real subprocess reads this CLI
    # flag -- unlike vcpkg's own downloader, which links libcurl directly
    # into vcpkg.exe rather than shelling out to curl.exe, so it can never
    # be handed this flag. Deliberately no proxy env vars here either --
    # confirmed on a real run, forcing this pool's proxy turns this call's
    # fixable SSL error into an unfixable connect timeout instead.
    & curl.exe --fail --location --ssl-no-revoke `
      --connect-timeout 15 --speed-limit 1024 --speed-time 15 `
      --output $OutFile $Url
    if ($LASTEXITCODE -eq 0) { return }
    if ($attempt -eq $MaxAttempts) {
      throw "curl download of $Url failed with exit $LASTEXITCODE after $MaxAttempts attempts"
    }
    Write-Warning "curl download of $Url failed (exit $LASTEXITCODE), attempt $attempt/$MaxAttempts -- retrying in ${DelaySeconds}s"
    Start-Sleep -Seconds $DelaySeconds
  }
}
