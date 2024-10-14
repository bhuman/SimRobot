# SimRobot

A physical robotics simulator, which is able to simulate arbitrary user-defined robots in three-dimensional space. It includes a physical model which is based on rigid body dynamics. To allow an extensive flexibility in building accurate models, a variety of different generic bodies, sensors, and actuators has been implemented. Furthermore, the simulator follows a user-oriented approach by including several mechanisms for visualization, direct actuator manipulation, and interaction with the simulated world.

## Microsoft Windows

### Prerequisites

- Microsoft Windows 10 64-bit Version 21H1
-  Microsoft Visual Studio Community 2022 Version 17.0.0 or newer. Installing the workload *Desktop development with C++* (including the packages *MSVC v143 - VS 2022 C++ x64/x86 build tools* and *Windows 10 SDK 10.0.19041.0* or newer) is sufficient.
-  CMake 3.21 or newer

### Compiling

Run `Make/Windows/generate.cmd` and open the solution `Make/Windows/SimRobot.lnk` in Visual Studio. Select the desired configuration (*Develop* is a good start) and build the target *SimRobot*. Select *SimRobot* as *StartUp Project* and select *Debug/Start Debugging* to run it.


## Linux

### Prerequisites

- A 64-bit Linux, e.g. Ubuntu 24.04 LTS
- The following packages (here for Ubuntu 24.04 LTS):
```
sudo apt-get install clang cmake libbox2d-dev libgl-dev llvm mold ninja-build qt6-base-dev qt6-svg-dev
```
- (optionally) CLion 2019.3 or newer

### Compiling (no IDE)

Run `Make/Linux/generate` to generate CMake caches and `Make/Linux/compile [Debug|Develop|Release]` to compile the code. The executable will be located in `Build/Linux/SimRobot/<Debug|Develop|Release>/SimRobot`.

### Compiling (CLion)

Run `Make/Linux/generate -c`. Open `Make/Linux/CMakeLists.txt` in CLion as project (*not* the the one in `Make/CMake`). In CLion, you can select a build type, e.g. *Develop*, and then select *Run/Debug SimRobot*.

## macOS

### Prerequisites

- macOS Mojave or newer
- Xcode 11 oder newer
- CMake 3.16 or newer

### Compiling

Run `Make/macOS/generate` and then open `Make/macOS/SimRobot.xcodeproj`. Select a scheme, e.g. *Develop* and select *Product/Run*.


## Opening a Scene File

After SimRobot is started, an example scene file in `Scenes` can be opened. Then, different parts of the scene graph can be opened by double-clicking them.
