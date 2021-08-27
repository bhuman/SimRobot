# SimRobot

A physical robotics simulator, which is able to simulate arbitrary user-defined robots in three-dimensional space. It includes a physical model which is based on rigid body dynamics. To allow an extensive flexibility in building accurate models, a variety of different generic bodies, sensors, and actuators has been implemented. Furthermore, the simulator follows a user-oriented approach by including several mechanisms for visualization, direct actuator manipulation, and interaction with the simulated world.

## Microsoft Windows

### Prerequisites

- Microsoft Windows 10 64-bit Version 20H2
-  Microsoft Visual Studio Community 2019 Version 16.8.2 or newer. Installing the workload *Desktop development with C++* as well as the packages *MSVC v142 - VS 2019 C++ x86/x64 build tools* and *Windows 10 SDK 10.0.18362.0* or newer is sufficient.
-  CMake 3.16 or newer

### Compiling

Run `Make/VS2019/generate.cmd` and open the solution `Make/VS2019/SimRobot.sln` in Visual Studio. Select the desired configuration (*Develop* is a good start) and build the target *SimRobot*. Select *SimRobot* as *StartUp Project* and select *Debug/Start Debugging* to run it.


## Linux

### Prerequisites

- A 64-bit Linux, e.g. Ubuntu 20.04.2 LTS
- The following packages (here for Ubuntu 20.04.2 LTS):
```
sudo apt-get install clang cmake libglew-dev libqt5opengl5-dev libqt5svg5-dev lld llvm ninja-build qtbase5-dev
```
- (optionally) CLion 2019.3 or newer

### Compiling (no IDE)

In the directory `Make/Linux` run `NO_CLION=true ./generate` to generate CMake caches and `./compile [Debug|Develop|Release]` to compile the code. The executable will be located in `Build/Linux/SimRobot/<Debug|Develop|Release>/SimRobot`.

### Compiling (CLion)

Run `Make/Linux/generate`. Open `Make/Linux/CMakeLists.txt` in CLion as project (*not* the the one in `Make/Common`). In CLion, you can select a build type, e.g. *Develop*, and then select *Run/Debug SimRobot*.

## macOS

### Prerequisites

- macOS Mojave or newer
- Xcode 11 oder newer
- CMake 3.16 or newer

### Compiling

Run `Make/macOS/generate` and then open `Make/macOS/SimRobot.xcodeproj`. Select a scheme, e.g. *Develop* and select *Product/Run*.


## Opening a Scene File

After SimRobot is started, an example scene file in `Scenes` can be opened. Then, different parts of the scene graph can be opened by double-clicking them.
