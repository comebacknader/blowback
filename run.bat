@echo off

call build.bat
pushd "build"
call win32_blowback.exe
popd "build"