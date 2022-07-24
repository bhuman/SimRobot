@echo off
pushd "%~dp0"
cmake.exe -G "Visual Studio 17 2022" -A x64 -S ..\CMake -B ..\..\Build\Windows\CMake
if exist SimRobot.lnk goto linkExists
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%CD%\SimRobot.lnk');$s.TargetPath='%CD%\..\..\Build\Windows\CMake\SimRobot.sln';$s.Save()"
:linkExists
popd
