# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

param(
    [Parameter(Mandatory)]
    [ValidateSet('prepare', 'snapshot')]
    [string]$Operation
)

$ErrorActionPreference = 'Stop'
$workspacePath = [System.IO.Path]::GetFullPath($env:GITHUB_WORKSPACE)
$buildPath = Join-Path $workspacePath 'build'
$cachePath = Join-Path $buildPath 'ccache'
$sourcePath = Join-Path $buildPath 'ccache-source'
if ([System.IO.Path]::GetFullPath($env:CCACHE_DIR) -ne $cachePath) {
    throw 'The ccache directory must be the build/ccache directory of the workspace'
}

if ($Operation -eq 'prepare') {
    $versionOutput = ccache --print-version
    if ($LASTEXITCODE -ne 0) {
        throw 'The ccache version could not be determined'
    }
    $installedVersion = [System.Version]::Parse(($versionOutput -join '').Trim())
    if ($installedVersion -lt [System.Version]'4.14') {
        throw 'CCache 4.14 or later is required'
    }
    Write-Host "CCache version: $installedVersion"

    if (Test-Path -LiteralPath $sourcePath) {
        throw 'The ccache source directory already exists'
    }
    if (Test-Path -LiteralPath $cachePath) {
        Move-Item -LiteralPath $cachePath -Destination $sourcePath
    } else {
        New-Item -ItemType Directory -Path $sourcePath -Force | Out-Null
    }
    New-Item -ItemType Directory -Path $cachePath | Out-Null

    $sourceUri = [System.Uri]::new($sourcePath, [System.UriKind]::Absolute).AbsoluteUri
    "CCACHE_REMOTE_STORAGE=$sourceUri @layout=local" >> $env:GITHUB_ENV
    'CCACHE_COMPILERCHECK=content' >> $env:GITHUB_ENV
    'CCACHE_NOHARDLINK=true' >> $env:GITHUB_ENV
    'CCACHE_NOFILECLONE=true' >> $env:GITHUB_ENV
    exit 0
}

function Get-CacheEntries([string]$Directory) {
    $entries = [System.Collections.Generic.Dictionary[string, System.IO.FileInfo]]::new([System.StringComparer]::Ordinal)
    foreach ($file in (Get-ChildItem -LiteralPath $Directory -Recurse -File)) {
        $key = [System.IO.Path]::GetRelativePath($Directory, $file.FullName).Replace('\', '').Replace('/', '')
        if ($key -cmatch '^[0-9a-f]{40}$') {
            $entries.Add($key, $file)
        }
    }
    return ,$entries
}

$sourceEntries = Get-CacheEntries $sourcePath
$currentEntries = Get-CacheEntries $cachePath
$changed = $currentEntries.Count -ne $sourceEntries.Count
if (-not $changed) {
    foreach ($entry in $currentEntries.GetEnumerator()) {
        if (-not $sourceEntries.ContainsKey($entry.Key) -or $entry.Value.Length -ne $sourceEntries[$entry.Key].Length) {
            $changed = $true
            break
        }
    }
}
if (-not $changed) {
    foreach ($entry in $currentEntries.GetEnumerator()) {
        $currentHash = (Get-FileHash -LiteralPath $entry.Value.FullName -Algorithm SHA256).Hash
        $sourceHash = (Get-FileHash -LiteralPath $sourceEntries[$entry.Key].FullName -Algorithm SHA256).Hash
        if ($currentHash -ne $sourceHash) {
            $changed = $true
            break
        }
    }
}

$save = $changed -and $currentEntries.Count -gt 0
@(
    '### CCache Snapshot'
    ''
    '| Restored Entries | Retained Entries |'
    '| ---: | ---: |'
    "| $($sourceEntries.Count) | $($currentEntries.Count) |"
) >> $env:GITHUB_STEP_SUMMARY
if ($save) {
    $compressionStatistics = ccache --recompress 10
    if ($LASTEXITCODE -ne 0) {
        throw 'The ccache snapshot could not be recompressed'
    }
    @(
        ''
        '```text'
        ($compressionStatistics -join "`n")
        '```'
    ) >> $env:GITHUB_STEP_SUMMARY
}
"save=$($save.ToString().ToLowerInvariant())" >> $env:GITHUB_OUTPUT
