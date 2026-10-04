# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

param(
    [Parameter(Mandatory)]
    [string]$InstallDir,

    [Parameter(Mandatory)]
    [ValidatePattern('^[A-Za-z0-9_]+$')]
    [string]$ApplicationName,

    [Parameter(Mandatory)]
    [string]$ArchivePath
)

$ErrorActionPreference = 'Stop'
$installPath = (Resolve-Path -LiteralPath $InstallDir).Path
$archiveFullPath = [System.IO.Path]::GetFullPath($ArchivePath)
$stagingRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("DiffScope-Portable-" + [guid]::NewGuid())
$applicationDir = Join-Path $stagingRoot $ApplicationName
New-Item -ItemType Directory -Path $applicationDir | Out-Null
Get-ChildItem -LiteralPath $installPath -Force | Copy-Item -Destination $applicationDir -Recurse -Force

Push-Location $stagingRoot
try {
    & 7z a -tzip -mx=9 $archiveFullPath $ApplicationName | Write-Host
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $archiveFullPath -PathType Leaf)) {
        throw 'Portable archive creation failed'
    }
} finally {
    Pop-Location
}

Write-Output $archiveFullPath
