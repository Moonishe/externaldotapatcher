@echo off
setlocal

where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo cl.exe not found. Run this script from a VS 2022 Developer Command Prompt.
        exit /b 1
    )
)

cl.exe /EHsc /std:c++20 /O2 /W3 /MT /DNDEBUG /D_CONSOLE /I"core" ^
    core\main.cpp core\memory.cpp ^
    /Fe:external_patcher.exe ^
    /link kernel32.lib user32.lib psapi.lib

if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

echo Done: external_patcher.exe
endlocal
