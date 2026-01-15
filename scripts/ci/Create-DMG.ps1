param (
    [Parameter(Mandatory)]
    [string]$AppPath,

    [Parameter(Mandatory)]
    [string]$Semver,

    [Parameter(Mandatory)]
    [string]$ApplicationDisplayName,

    [Parameter(Mandatory)]
    [string]$InstallerFileBase
)

$BackgroundSrcDir = "src/app/share/dmg"

$Bg1x = Join-Path $BackgroundSrcDir "dmg_background.png"
$Bg2x = Join-Path $BackgroundSrcDir "dmg_background@2x.png"

if (!(Test-Path $Bg1x) -or !(Test-Path $Bg2x)) {
    throw "dmg_background.png and dmg_background@2x.png do not exist in $BackgroundSrcDir"
}

if (!(Test-Path $AppPath)) {
    throw "App bundle not exist: $AppPath"
}

# Temporary directory
$TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("dmg-build-" + [System.Guid]::NewGuid())
New-Item -ItemType Directory -Path $TempDir | Out-Null

$Bg1xOut = Join-Path $TempDir "dmg_background.png"
$Bg2xOut = Join-Path $TempDir "dmg_background@2x.png"
$BgTiff  = Join-Path $TempDir "dmg_background.tiff"
$AppBundleName = "$ApplicationDisplayName.app"
$AppBundlePath = Join-Path $TempDir $AppBundleName

$VersionText = "Version $Semver"

try {
    # -----------------------------
    # Step 1: Preprocess background
    # -----------------------------

    # 1x image
    & magick `
        "$Bg1x" `
        -gravity south `
        -pointsize 12 `
        -fill "rgba(37,37,37,0.25)" `
        -annotate +0+8 "$VersionText" `
        "$Bg1xOut" | Write-Host

    if ($LASTEXITCODE -ne 0) {
        throw "ImageMagick failed to process dmg_background.png"
    }

    # 2x image (scaled)
    & magick `
        "$Bg2x" `
        -gravity south `
        -pointsize 24 `
        -fill "rgba(37,37,37,0.25)" `
        -annotate +0+16 "$VersionText" `
        "$Bg2xOut" | Write-Host

    if ($LASTEXITCODE -ne 0) {
        throw "ImageMagick failed to process dmg_background@2x.png"
    }

    # Combine into TIFF
    & tiffutil `
        -cathidpicheck `
        "$Bg1xOut" `
        "$Bg2xOut" `
        -out "$BgTiff" | Write-Host

    if ($LASTEXITCODE -ne 0) {
        throw "tiffutil failed to create TIFF"
    }

    # -----------------------------
    # Step 2: Build DMG
    # -----------------------------
    $DmgName = "$InstallerFileBase.dmg"
    $DmgPath = Join-Path (Get-Location) $DmgName

    if (Test-Path $DmgPath) {
        Remove-Item $DmgPath -Force
    }

    if (Test-Path $AppBundlePath) {
        Remove-Item $AppBundlePath -Recurse -Force
    }

    Move-Item -Path $AppPath -Destination $AppBundlePath

    & codesign --deep --force --sign - $AppBundlePath | Write-Host

    <# TODO: create-dmg currently places hidden .background file to the right of the visible area, so we have to leave some space for the horizontal scroll bar #>
    & create-dmg `
        --volname "$ApplicationDisplayName" `
        --background "$BgTiff" `
        --window-size 600 448 `
        --icon-size 128 `
        --icon "$(Split-Path $AppBundlePath -Leaf)" 132 280 `
        --app-drop-link 468 280 `
        "$DmgPath" `
        "$AppBundlePath" | Write-Host

    if ($LASTEXITCODE -ne 0) {
        throw "create-dmg failed"
    }

    Write-Output $DmgPath
}
finally {
    # Cleanup temp files
    if (Test-Path $TempDir) {
        Remove-Item $TempDir -Recurse -Force
    }
}
