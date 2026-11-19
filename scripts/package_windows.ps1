param(
    [string]$ExePath = "build\kvikplay.exe",
    [string]$CustomWindeployqt = "",
    [string]$MpvHint = ""
)

$ErrorActionPreference = "Stop"
Write-Host "[Standalone Deploy] Packaging standalone Windows app: $ExePath"

if (-not (Test-Path $ExePath)) {
    Write-Error "[Standalone Deploy] Error: Executable not found at $ExePath"
    exit 1
}

$TargetDir = Split-Path -Parent (Resolve-Path $ExePath)

# Locate windeployqt
$Windeployqt = $null
if ($CustomWindeployqt -and (Test-Path $CustomWindeployqt)) {
    $Windeployqt = $CustomWindeployqt
} else {
    $cmd = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        $Windeployqt = $cmd.Source
    } elseif ($env:QTDIR -and (Test-Path "$env:QTDIR\bin\windeployqt.exe")) {
        $Windeployqt = "$env:QTDIR\bin\windeployqt.exe"
    }
}

if ($Windeployqt) {
    Write-Host "[Standalone Deploy] Running windeployqt: $Windeployqt"
    & $Windeployqt "$ExePath" --no-translations --compiler-runtime
} else {
    Write-Warning "[Standalone Deploy] windeployqt not found. Skipping Qt deployment."
}

# Check for mpv library
$TargetMpv = Join-Path $TargetDir "mpv-2.dll"
$TargetLibMpv = Join-Path $TargetDir "libmpv-2.dll"
if (-not (Test-Path $TargetMpv) -and -not (Test-Path $TargetLibMpv)) {
    $MpvCandidates = @(
        (Join-Path $MpvHint "mpv-2.dll"),
        (Join-Path $MpvHint "libmpv-2.dll"),
        (Join-Path $MpvHint "..\bin\mpv-2.dll"),
        (Join-Path $MpvHint "..\bin\libmpv-2.dll")
    )
    $FoundMpv = $null
    foreach ($c in $MpvCandidates) {
        if (Test-Path $c) {
            $FoundMpv = $c
            break
        }
    }
    if (-not $FoundMpv) {
        $cmdMpv = Get-Command mpv-2.dll -ErrorAction SilentlyContinue
        if ($cmdMpv) { $FoundMpv = $cmdMpv.Source }
    }
    if ($FoundMpv) {
        Write-Host "[Standalone Deploy] Copying mpv library from $FoundMpv to $TargetDir"
        Copy-Item -Path $FoundMpv -Destination $TargetDir -Force
    } else {
        Write-Warning "[Standalone Deploy] mpv-2.dll not located. Ensure mpv-2.dll is in $TargetDir."
    }
}

Write-Host "[Standalone Deploy] Standalone Windows application ready in: $TargetDir"
