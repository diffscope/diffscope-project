param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("dev", "alpha", "beta", "release")]
    [string]$BuildType,
    
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$VcpkgRootDir,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$InstallDir,

    [string]$VersionIdentifier = "0",

    [switch]$CCache = $false
)

if (-not (Test-Path $VcpkgRootDir)) {
    throw "Vcpkg root directory does not exist: $VcpkgRootDir"
}

New-Item $BuildDir -ItemType Directory -Force
New-Item $InstallDir -ItemType Directory -Force

Write-Host "Build type: $BuildType"
Write-Host "Vcpkg root directory: $VcpkgRootDir"
Write-Host "Build directory: $(Resolve-Path $BuildDir)"
Write-Host "Install directory: $(Resolve-Path $InstallDir)"
Write-Host "Version identifier: $VersionIdentifier"
Write-Host "Use CCache: $CCache"

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

$depsDir = (Get-ChildItem -Path $(Join-Path $VcpkgRootDir installed) | Where-Object {$_.Name -ne "vcpkg"})[0].FullName

cmake -S . -B $(Resolve-Path $BuildDir) -G Ninja `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    "-DCMAKE_TOOLCHAIN_FILE=$(Join-Path $VcpkgRootDir scripts/buildsystems/vcpkg.cmake)" `
    "-DCMAKE_C_COMPILER_LAUNCHER=$($CCache ? 'ccache' : '')" `
    "-DCMAKE_CXX_COMPILER_LAUNCHER=$($CCache ? 'ccache' : '')" `
    -DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded `
    -DCK_ENABLE_CONSOLE:BOOL=FALSE `
    -DQT_NO_PRIVATE_MODULE_WARNING:BOOL=ON `
    "-DQMSETUP_APPLOCAL_DEPS_PATHS_RELWITHDEBINFO=$(Join-Path $depsDir lib)" `
    -DAPPLICATION_INSTALL:BOOL=ON `
    -DAPPLICATION_CONFIGURE_INSTALLER:BOOL=ON `
    -DINNOSETUP_USE_UNOFFICIAL_LANGUAGE:BOOL=ON `
    "-DAPPLICATION_INSTALLER_OUTPUT_DIR=$(Resolve-Path .)" `
    "-DAPPLICATION_INSTALLER_FILE_BASE=$installerFileBase" `
    "-DAPPLICATION_NAME=$applicationName" `
    "-DAPPLICATION_DISPLAY_NAME=$applicationDisplayName" `
    "-DAPPLICATION_SEMVER=$semver" `
    "-DCMAKE_INSTALL_PREFIX=$(Resolve-Path $InstallDir)" | Write-Host
if ($LASTEXITCODE -ne 0) {
    throw "Configure failed"
}

if ($CCache) {
    ccache --zero-stats | Write-Host
}

cmake --build $(Resolve-Path $BuildDir) --target all | Write-Host
if ($LASTEXITCODE -ne 0) {
    throw "Build failed"
}

if ($CCache) {
    ccache --show-stats | Write-Host
}

cmake --build $(Resolve-Path $BuildDir) --target install | Write-Host
if ($LASTEXITCODE -ne 0) {
    throw "Install failed"
}

$buildResult = @{
    ProjectName = $projectName
    Version = $version
    ApplicationName = $applicationName
    ApplicationDisplayName = $applicationDisplayName
    Semver = $semver
    InstallerFileBase = $installerFileBase
}

Write-Output $buildResult
