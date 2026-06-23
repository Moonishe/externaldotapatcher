@echo off
setlocal

set "GCC=%~dp0tools\mingw64\bin\g++.exe"

if not exist "%GCC%" (
    where g++.exe >nul 2>&1
    if %errorlevel% neq 0 (
        echo g++.exe not found.
        echo Download MinGW-w64 or add it to PATH.
        exit /b 1
    )
    set "GCC=g++.exe"
)

echo Compiling executable...
"%GCC%" -std=c++20 -O2 -Wall -m64 -static -I"%~dp0core" "%~dp0core\main.cpp" "%~dp0core\memory.cpp" "%~dp0core\autoaccept.cpp" -o "%~dp0external_patcher.exe" -lpsapi -lkernel32 -luser32 -lgdi32
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

echo Copying manifest for UAC admin request...
copy /Y "%~dp0app.manifest" "%~dp0external_patcher.exe.manifest" >nul

echo Done: external_patcher.exe
endlocal
