# Runs cmake against this repo's Windows/vcpkg dependency graph, handling
# everything vcpkg itself needs first. Every argument is passed straight
# through to cmake -- e.g.:
#
#   tool\windows\vcpkg_cmake.ps1 -S . -B out_windows -G Ninja `
#     -DCMAKE_BUILD_TYPE=Release -DSTARFISH_WINDOWS_BUILD_SHELL=ON `
#     -DVCPKG_TARGET_TRIPLET=x64-windows
#
# Do not pass -DCMAKE_TOOLCHAIN_FILE yourself -- this script points cmake at
# vcpkg-root's toolchain file itself, since it's also the one bootstrapping
# vcpkg-root in the first place.
#
# 1. vcpkg-root setup: clones (if missing) and checks out the commit pinned
#    in vcpkg-configuration.json's default-registry.baseline -- the single
#    source of truth for the pin, so this can never drift from what
#    vcpkg.json's overlay ports were written against. Retried: this
#    network's github.com access is intermittently flaky.
# 2. vcpkg's own downloader (libcurl linked straight into vcpkg.exe, not a
#    curl.exe subprocess it shells out to) can fail source-archive
#    downloads with schannel CRYPT_E_NO_REVOCATION_CHECK that no
#    curl-CLI-only flag (--ssl-no-revoke, .curlrc, CURL_SSLBACKEND) can
#    reach, since none of those are read by vcpkg's own linked-in call.
#    X_VCPKG_ASSET_SOURCES=x-script hands every such download to
#    fetch_vcpkg_asset.ps1 instead, which shells out to a real curl.exe
#    that --ssl-no-revoke actually works on.
param(
  [switch]$Dali,
  [Parameter(ValueFromRemainingArguments = $true)]
  [string[]]$CMakeArgs
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

. (Join-Path $PSScriptRoot 'lib_retry.ps1')

if ($Dali) {
  $DaliVcpkgDir = Join-Path $RepoRoot 'tool\windows\dali-vcpkg'
  $configPath = Join-Path $DaliVcpkgDir 'vcpkg-configuration.json'
} else {
  $configPath = Join-Path $RepoRoot 'vcpkg-configuration.json'
}
$vcpkgCommit = (Get-Content $configPath -Raw | ConvertFrom-Json).'default-registry'.baseline
if (-not $vcpkgCommit) { throw "couldn't read default-registry.baseline from $configPath" }

$vcpkgRoot = Join-Path $RepoRoot 'vcpkg-root'
if (-not (Test-Path (Join-Path $vcpkgRoot '.git'))) {
  Invoke-GitRetry -GitArgs @('clone', 'https://github.com/microsoft/vcpkg.git', $vcpkgRoot)
}
Invoke-GitRetry -GitArgs @('-C', $vcpkgRoot, 'fetch', '--depth=1', 'origin', $vcpkgCommit)
git -C $vcpkgRoot checkout --force FETCH_HEAD
if ($LASTEXITCODE -ne 0) { throw "git checkout FETCH_HEAD failed" }

if (-not (Test-Path (Join-Path $vcpkgRoot 'vcpkg.exe'))) {
  & (Join-Path $vcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics
  if ($LASTEXITCODE -ne 0) { throw "bootstrap-vcpkg.bat failed" }
}

$env:VCPKG_ROOT = $vcpkgRoot

# Not a secret -- just the fixed BART endpoint every other script here
# already hardcodes (e.g. windows.yml, tool/ci/lib_submodule_cache.sh).
# Set $env:BART_API_KEY yourself before calling this script to also get
# fetch_vcpkg_asset.ps1's BART asset-mirror check/publish and the bulk
# prefetch below locally; leave it unset to skip BART entirely and always
# fetch from the original URL.
if (-not $env:BART_CACHE_URL) {
  $env:BART_CACHE_URL = 'https://bart.sec.samsung.net/artifactory/starfish-git-cache-service-generic-local/starfish-ci-cache'
}

# Pre-populate vcpkg's downloads/ folder from BART in one bulk pass
# instead of leaving every asset to race BART individually at install
# time -- see the script's own comment.
& (Join-Path $PSScriptRoot 'prefetch_vcpkg_assets.ps1') -VcpkgRoot $vcpkgRoot

$fetchScript = Join-Path $PSScriptRoot 'fetch_vcpkg_asset.ps1'
# vcpkg calls this command line via CreateProcessW directly, not through a
# shell -- a bare "pwsh" name it can't find on PATH fails with "calling
# CreateProcessW failed with 2 (The system cannot find the file
# specified.)" and vcpkg silently falls back to its own broken downloader,
# with no sign fetch_vcpkg_asset.ps1 was ever supposed to run at all
# (confirmed on a real dev machine that only has Windows PowerShell 5.1,
# not PowerShell 7). Resolve whichever PowerShell this script is itself
# already running under just fine, and hand vcpkg that exact full path.
$psCmd = Get-Command pwsh -ErrorAction SilentlyContinue
if (-not $psCmd) { $psCmd = Get-Command powershell -ErrorAction SilentlyContinue }
if (-not $psCmd) { throw "neither pwsh.exe nor powershell.exe found on PATH" }
$env:X_VCPKG_ASSET_SOURCES = "x-script,`"$($psCmd.Source)`" -NoProfile -ExecutionPolicy Bypass -File `"$fetchScript`" {url} {sha512} {dst}"

$toolchain = Join-Path $vcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
$cmakeInvocation = @("-DCMAKE_TOOLCHAIN_FILE=$toolchain")
if ($Dali) {
  $cmakeInvocation += "-DVCPKG_MANIFEST_DIR=$DaliVcpkgDir"
}
& cmake @cmakeInvocation @CMakeArgs
exit $LASTEXITCODE
