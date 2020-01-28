@echo off
pushd "%~dp0"
cmake.exe -G "Visual Studio 16 2019" -A x64 -S ..\Common -B ..\..\Build\Windows\CMake
if not exist SimRobot.sln mklink SimRobot.sln ..\..\Build\Windows\CMake\SimRobot.sln
popd
