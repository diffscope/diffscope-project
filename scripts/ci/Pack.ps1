param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$InstallerFileBase,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$InnoSetupCommit
)

if ($IsWindows) {
    $env:INNOSETUP_MESSAGE_FILES_DIR = [System.IO.Path]::GetTempPath() + "Unofficial-InnoSetup-Languages"
    New-Item -ItemType Directory -Force -Path $env:INNOSETUP_MESSAGE_FILES_DIR
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/jrsoftware/issrc/$InnoSetupCommit/Files/Languages/Unofficial/ChineseSimplified.isl" -OutFile "$env:INNOSETUP_MESSAGE_FILES_DIR\ChineseSimplified.isl"
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/jrsoftware/issrc/$InnoSetupCommit/Files/Languages/Unofficial/ChineseTraditional.isl" -OutFile "$env:INNOSETUP_MESSAGE_FILES_DIR\ChineseTraditional.isl"
    ISCC $BuildDir/dist/installer/windows/setup.iss
    if (! (Test-Path "${InstallerFileBase}.exe")) {
        Write-Error "Installer file was not created: ${InstallerFileBase}.exe"
        exit 1
    }
    Write-Output $(Resolve-Path "${InstallerFileBase}.exe")
}
