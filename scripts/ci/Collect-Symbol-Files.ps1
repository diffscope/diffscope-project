param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$VcpkgRootDir,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$InstalledDir
)

$symbolFilesDirectory = [System.IO.Path]::GetTempPath() + "DiffScope-Symbols"
New-Item -ItemType Directory -Force -Path $symbolFilesDirectory

if ($IsWindows) {
    $PATTERN = "PDB file found at.*'(.*)'"
    $env:_NT_ALT_SYMBOL_PATH = $(Join-Path $VcpkgRootDir "/installed/x64-windows/bin")

    Push-Location $InstalledDir
    $dllFiles = Get-ChildItem -Path . -Recurse | Where-Object { $_.Extension -eq '.exe' -or $_.Extension -eq '.dll' }
    foreach ($dllFile in $dllFiles) {
        dumpbin /PDBPATH:VERBOSE $dllFile.FullName
        $dumpbinOutput = dumpbin /PDBPATH $dllFile.FullName
        $matches = [regex]::Matches($dumpbinOutput, $PATTERN)
        if ($matches.Count -gt 0) {
            $pdbPath = $matches.Groups[1].Value
            Write-Output "$dllFile -> $pdbPath"
            $pdbTargetDirectory = "$symbolFilesDirectory/$(Split-Path $(Resolve-Path $dllFile.FullName -Relative))"
            if (!(Test-Path $pdbTargetDirectory)) {
                New-Item $pdbTargetDirectory -ItemType directory
            }
            Copy-Item $pdbPath $pdbTargetDirectory
        } else {
            Write-Output "No PDB file: $dllFile"
        }
    }
    Pop-Location
} elseif ($IsMacOS) {
    Push-Location $InstalledDir
    $dllFiles = Get-ChildItem -Path . -Recurse | Where-Object { (file $_) -match "Mach-O 64-bit" }
    foreach ($dllFile in $dllFiles) {
        $dsymutilOutput = dsymutil -s $dllFile.FullName
        if ($dsymutilOutput -match "N_OSO") {
            Write-Output "Copy and strip debug_info: $dllFile"
            $pdbTargetDirectory = "$symbolFilesDirectory/$(Split-Path $(Resolve-Path $dllFile.FullName -Relative))"
            if (!(Test-Path $pdbTargetDirectory)) {
                New-Item $pdbTargetDirectory -ItemType directory
            }
            dsymutil $dllFile.FullName -o "$pdbTargetDirectory/$($dllFile.Name).dSYM"
            strip -S $dllFile.FullName
        } else {
            Write-Output "Skip: $dllFile"
        }
    }
    Pop-Location
} else {
    Push-Location $InstalledDir
    $dllFiles = Get-ChildItem -Path . -Recurse | Where-Object { (file $_) -match "ELF 64-bit" }
    foreach ($dllFile in $dllFiles) {
        file $dllFile.FullName
        $fileOutput = file $dllFile.FullName
        if ($fileOutput -match "with debug_info") {
            Write-Output "Copy and strip debug_info: $dllFile"
            $pdbTargetDirectory = "$symbolFilesDirectory/$(Split-Path $(Resolve-Path $dllFile.FullName -Relative))"
            if (!(Test-Path $pdbTargetDirectory)) {
                New-Item $pdbTargetDirectory -ItemType directory
            }
            objcopy --only-keep-debug $dllFile.FullName "$pdbTargetDirectory/$($dllFile.Name).debug"
            strip --strip-debug $dllFile.FullName
        } else {
            Write-Output "Skip: $dllFile"
        }
    }
    Pop-Location
}

7z a -t7z -mx=9 -ms=on symbol_files.7z $symbolFilesDirectory
Remove-Item -Recurse -Force $symbolFilesDirectory
Write-Output $(Resolve-Path symbol_files.7z)