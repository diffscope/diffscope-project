# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

param(
    [Parameter(Mandatory)]
    [string]$InstallDir,

    [Parameter(Mandatory)]
    [ValidatePattern('^[A-Za-z0-9_]+$')]
    [string]$ApplicationName,

    [Parameter(Mandatory)]
    [string]$AppImageToolPath,

    [Parameter(Mandatory)]
    [string]$RuntimePath,

    [Parameter(Mandatory)]
    [string]$AppImagePath
)

if (-not $IsLinux) {
    throw 'AppImage packaging requires Linux'
}

$ErrorActionPreference = 'Stop'
$installPath = (Resolve-Path -LiteralPath $InstallDir).Path
$appImageTool = (Resolve-Path -LiteralPath $AppImageToolPath).Path
$runtime = (Resolve-Path -LiteralPath $RuntimePath).Path
$appImageFullPath = [System.IO.Path]::GetFullPath($AppImagePath)
$stagingRoot = [System.IO.Directory]::CreateTempSubdirectory('DiffScope-AppImage-').FullName

try {
    $appDir = Join-Path $stagingRoot "$ApplicationName.AppDir"
    New-Item -ItemType Directory -Path $appDir | Out-Null
    & /bin/cp -a -- "$installPath/." $appDir
    if ($LASTEXITCODE -ne 0) {
        throw 'AppImage staging failed'
    }

    $desktopId = "org.diffscope.$ApplicationName"
    $desktopPath = "share/applications/$desktopId.desktop"
    $iconPath = "share/icons/hicolor/256x256/apps/$desktopId.png"
    if (-not (Test-Path -LiteralPath (Join-Path $appDir $iconPath) -PathType Leaf)) {
        throw 'The installation does not contain the AppImage icon'
    }

    $desktopEntry = Get-Content -LiteralPath (Join-Path $appDir $desktopPath) -Raw
    $tryExec = [regex]::Match($desktopEntry, '(?m)^TryExec=([A-Za-z0-9_]+)\r?$')
    if (-not $tryExec.Success) {
        throw 'The desktop entry does not specify an executable name in TryExec'
    }
    $executableName = $tryExec.Groups[1].Value
    if (-not (Test-Path -LiteralPath (Join-Path $appDir "bin/$executableName") -PathType Leaf)) {
        throw 'The installation does not contain the AppImage executable'
    }

    [System.IO.File]::CreateSymbolicLink((Join-Path $appDir "$desktopId.desktop"), $desktopPath) | Out-Null
    [System.IO.File]::CreateSymbolicLink((Join-Path $appDir "$desktopId.png"), $iconPath) | Out-Null
    [System.IO.File]::CreateSymbolicLink((Join-Path $appDir '.DirIcon'), "$desktopId.png") | Out-Null

    $appRun = Join-Path $appDir 'AppRun'
    Set-Content -LiteralPath $appRun -Value @(
        '#!/bin/sh'
        ('exec "$APPDIR/bin/{0}" "$@"' -f $executableName)
    )

    & /bin/chmod +x -- $appRun
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to make AppRun executable'
    }
    & $appImageTool --appimage-extract-and-run --no-appstream `
        --comp xz `
        --mksquashfs-opt=-b --mksquashfs-opt=1M `
        --runtime-file $runtime $appDir $appImageFullPath | Write-Host
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $appImageFullPath -PathType Leaf)) {
        throw 'AppImage creation failed'
    }
} finally {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}

Write-Output $appImageFullPath
