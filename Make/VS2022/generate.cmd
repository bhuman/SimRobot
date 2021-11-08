@echo off
pushd "%~dp0"
cmake.exe -G "Visual Studio 17 2022" -A x64 -S ..\Common -B ..\..\Build\Windows\CMake
if exist SimRobot.sln goto :linkExists
if exist SimRobot.sln.lnk goto :linkExists
mklink SimRobot.sln ..\..\Build\Windows\CMake\SimRobot.sln
:linkExists
popd
