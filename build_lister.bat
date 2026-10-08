@echo off
setlocal
cd /d "%~dp0"
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto failed
cmake --build build64 --config Release --target XRayTHMLister XRayTHMEditor thm_info test_document test_decoder test_texture test_safe_file
if errorlevel 1 goto failed
ctest --test-dir build64 -C Release --output-on-failure
if errorlevel 1 goto failed
cmake -S . -B build32 -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 goto failed
cmake --build build32 --config Release --target XRayTHMLister XRayTHMEditor test_document test_decoder test_texture test_safe_file
if errorlevel 1 goto failed
ctest --test-dir build32 -C Release --output-on-failure
if errorlevel 1 goto failed
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0package_lister.ps1"
if errorlevel 1 goto failed
echo Lister plugin ZIP ready in dist. Install it in Total Commander; administrator rights are not required.
pause
exit /b 0
:failed
echo Build failed. See the error above.
pause
exit /b 1
