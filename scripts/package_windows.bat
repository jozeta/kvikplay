@echo off
setlocal enabledelayedexpansion

set "EXE_PATH=%~1"
if "%EXE_PATH%"=="" set "EXE_PATH=build\kvikplay.exe"

set "CUSTOM_WINDEPLOYQT=%~2"
set "MPV_HINT=%~3"

for %%I in ("%EXE_PATH%") do set "TARGET_DIR=%%~dpI"
echo [Standalone Deploy] Packaging standalone Windows app: %EXE_PATH%

if not exist "%EXE_PATH%" (
    echo [Standalone Deploy] Error: Executable not found at %EXE_PATH%
    exit /b 1
)

:: Locate windeployqt
set "WINDEPLOYQT_BIN="
if exist "%CUSTOM_WINDEPLOYQT%" (
    set "WINDEPLOYQT_BIN=%CUSTOM_WINDEPLOYQT%"
) else (
    where windeployqt >nul 2>&1
    if !errorlevel! equ 0 (
        for /f "delims=" %%I in ('where windeployqt') do (
            if not defined WINDEPLOYQT_BIN set "WINDEPLOYQT_BIN=%%I"
        )
    ) else if exist "%QTDIR%\bin\windeployqt.exe" (
        set "WINDEPLOYQT_BIN=%QTDIR%\bin\windeployqt.exe"
    )
)

if "%WINDEPLOYQT_BIN%"=="" (
    echo [Standalone Deploy] Warning: windeployqt not found in PATH or QTDIR. Skipping automatic Qt DLL copy.
) else (
    echo [Standalone Deploy] Running windeployqt: %WINDEPLOYQT_BIN%
    "%WINDEPLOYQT_BIN%" "%EXE_PATH%" --no-translations --compiler-runtime
)

:: Locate and copy mpv-2.dll / libmpv-2.dll if not present
if not exist "%TARGET_DIR%mpv-2.dll" if not exist "%TARGET_DIR%libmpv-2.dll" (
    echo [Standalone Deploy] Checking for mpv dynamic library...
    set "FOUND_MPV="
    if exist "%MPV_HINT%\mpv-2.dll" set "FOUND_MPV=%MPV_HINT%\mpv-2.dll"
    if exist "%MPV_HINT%\libmpv-2.dll" set "FOUND_MPV=%MPV_HINT%\libmpv-2.dll"
    if exist "%MPV_HINT%\..\bin\mpv-2.dll" set "FOUND_MPV=%MPV_HINT%\..\bin\mpv-2.dll"
    if exist "%MPV_HINT%\..\bin\libmpv-2.dll" set "FOUND_MPV=%MPV_HINT%\..\bin\libmpv-2.dll"
    
    if not defined FOUND_MPV (
        where mpv-2.dll >nul 2>&1
        if !errorlevel! equ 0 (
            for /f "delims=" %%I in ('where mpv-2.dll') do (
                if not defined FOUND_MPV set "FOUND_MPV=%%I"
            )
        )
    )
    if not defined FOUND_MPV (
        where libmpv-2.dll >nul 2>&1
        if !errorlevel! equ 0 (
            for /f "delims=" %%I in ('where libmpv-2.dll') do (
                if not defined FOUND_MPV set "FOUND_MPV=%%I"
            )
        )
    )

    if defined FOUND_MPV (
        echo [Standalone Deploy] Copying mpv library from !FOUND_MPV! to %TARGET_DIR%
        copy /y "!FOUND_MPV!" "%TARGET_DIR%" >nul
    ) else (
        echo [Standalone Deploy] Notice: mpv-2.dll was not found in hint paths. Ensure mpv-2.dll is located in %TARGET_DIR% at runtime.
    )
)

echo [Standalone Deploy] Standalone Windows application ready in: %TARGET_DIR%
exit /b 0
