param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("dev", "alpha", "beta", "release")]
    [string]$BuildType,
    
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$VcpkgRootDir,

    [string]$VersionIdentifier = "0"
)

if (-not (Test-Path $VcpkgRootDir)) {
    throw "Vcpkg root directory does not exist: $VcpkgRootDir"
}

Write-Host "Build type: $BuildType"
Write-Host "Commit: $Commit"
Write-Host "Vcpkg root directory: $VcpkgRootDir"

$cmakeListsPath = "CMakeLists.txt"

$cmakeContent = Get-Content $cmakeListsPath -Raw
$projectMatch = [regex]::Match($cmakeContent, 'project\s*\(\s*(\w+)\s+VERSION\s+([\d.]+)')

if (-not $projectMatch.Success) {
    throw "Could not find project declaration with VERSION in CMakeLists.txt"
}

$projectName = $projectMatch.Groups[1].Value
$version = $projectMatch.Groups[2].Value

Write-Host "Project name: $projectName"
Write-Host "Project version: $version"

$applicationName = $projectName
$applicationDisplayName = $projectName
$semver = $version

switch ($BuildType) {
    "dev" {
        $applicationName += "_dev"
        $applicationDisplayName += " (Dev)"
        $semver += "+$VersionIdentifier"
    }
    "alpha" {
        $applicationName += "_alpha"
        $applicationDisplayName += " (Alpha)"
        $semver += "-alpha.$VersionIdentifier"
    }
    "beta" {
        $semver += "-beta.$VersionIdentifier"
    }
    "release" {
        # No changes needed for release
    }
}

Write-Host "Application name: $applicationName"
Write-Host "Application display name: $applicationDisplayName"
Write-Host "Semver: $semver"

$installerFileBase = "${applicationName}_$($semver -replace '[\.\-\+]', '_')_installer"

cmake -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    "-DCMAKE_TOOLCHAIN_FILE=$(Join-Path $VcpkgRootDir scripts/buildsystems/vcpkg.cmake)" `
    -DCK_ENABLE_CONSOLE:BOOL=FALSE `
    -DAPPLICATION_INSTALL:BOOL=ON `
    -DAPPLICATION_CONFIGURE_INSTALLER:BOOL=ON `
    -DINNOSETUP_USE_UNOFFICIAL_LANGUAGE:BOOL=ON `
    "-DAPPLICATION_INSTALLER_OUTPUT_DIR=$(Resolve-Path .)" `
    "-DAPPLICATION_INSTALLER_FILE_BASE=$installerFileBase" `
    "-DAPPLICATION_NAME=$applicationName" `
    "-DAPPLICATION_DISPLAY_NAME=$applicationDisplayName" `
    "-DAPPLICATION_SEMVER=$semver" `
    -DCMAKE_INSTALL_PREFIX=installed | Write-Host

cmake --build build --target all | Write-Host
cmake --build build --target install | Write-Host

$buildResult = @{
    ProjectName = $projectName
    Version = $version
    ApplicationName = $applicationName
    ApplicationDisplayName = $applicationDisplayName
    Semver = $semver
    BuildDir = $(Resolve-Path "build")
    InstalledDir = $(Resolve-Path "installed")
    InstallerFileBase = $installerFileBase
}

Write-Output $buildResult
