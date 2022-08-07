# Third-Party Libraries

Besides the C++ standard runtime, SimRobot uses certain third-party libraries. With one exception, they come bundled with this repository to simplify development setup and to ensure that versions are used that are known to work.

## Eigen

[Eigen](https://eigen.tuxfamily.org) is a header-only library for linear algebra (source releases can be downloaded [here](https://gitlab.com/libeigen/eigen/-/releases)). The currently used version in `Util/Eigen` is 3.4.0. In the past, updating Eigen has been straightforward since only the `Eigen` and `debug` directories have to be copied from the release to `Util/Eigen`.

The SimRobot build system defines a macro such that Eigen objects are initialized with NaNs in the Debug configuration. This behavior can be controlled by setting the CMake variable `EIGEN_NAN_INITIALIZATION_CONDITION` (which should be a generator expression evaluating to `0` or `1`) before including `Eigen.cmake`.

## Box2D

TODO: Describe where Box2D comes from, what the current version is and how it was compiled, and what to take into account when updating to a new version.

## ODE

[ODE](https://ode.org/) is the dynamics engine used in `SimRobotCore2` (source releases can be downloaded [here](https://bitbucket.org/odedevs/ode/downloads/)). The binaries and headers in `Util/ode` have different versions on different platforms: On macOS, version 0.16.2 is used, while Linux and Windows use 0.16. On Windows, ODE 0.16.1 and 0.16.2 seemed to cause heap corruption. We are using the `double` version (i.e. `dDOUBLE` defined) on all platforms.

On Windows, especially during the Visual Studio 16 period, even minor Visual Studio updates often caused (presumably) object file format incompatibilities that required recompiling ODE. This hasn't occurred in a while (as of July 2022, the last time this was necessary was for VS 16.8.2 in November 2020), in particular no recompilation for Visual Studio 17 was necessary.

For Linux and macOS, the process of compiling the latest libraries is described in `Util/ode/VERSION`. On Windows, the current binaries were produced by running CMake in the `build` directory with the options `-G "Visual Studio 16 2019" -DBUILD_SHARED_LIBS=OFF -DODE_WITH_DEMOS=OFF ..` and subsequently compiling the project for Debug and Release in Visual Studio.

## Qt

[Qt](https://www.qt.io/) is mainly used as GUI framework and as OpenGL provider in `SimRobotCore2`. For Windows and macOS, the necessary parts of Qt are present in `Util/qt`. On Linux, the system's Qt is used, which must be installed separately (e.g. via a package manager as instructed by the [compilation intstructions](/README.md#linux)).

For macOS and Windows, the Qt version is currently 6.3.1. The steps to populate `Util/qt/{macOS,Windows}` (after removing its previous content) were as follows (of course, future versions can require different steps):
1. Download the macOS/Windows online installer from [here](https://www.qt.io/download-qt-installer) (the resulting file should be called something like `qt-unified-{macOS,Windows}-x64-4.4.1-online.dmg`).
2. Execute the installer, either from the disk image (macOS) or directly (Windows).
3. Proceed through the installer by signing in with a Qt account and agreeing to the open source conditions.
4. Choose a custom installation and some destination, e.g. `~/Qt` on macOS or `C:\Qt` on Windows (this will be referred as `<Qt directory>` below).
5. In the following screen, everything should be deselected, but `Qt>Qt 6.3.1>macOS` (on macOS) or `Qt>Qt 6.3.1>MSVC 2019 64-bit` (on Windows) must be selected (it seems that QtCreator cannot be deselected).
6. Proceed by agreeing to the license terms and confirming the installation.
7. After the installation has finished, copy the relevant files/directories. The `Util/qt/Licenses` directory comes directly from `<Qt directory>/Licenses` (they are the same for macOS and Windows except for minor encoding differences). Both `Util/qt/macOS` and `Util/qt/Windows` are reduced copies of `<Qt directory>/6.3.1/macos` and `<Qt directory>/6.3.1/msvc2019_64`, respectively. In 6.3.1, the specific files/directories were (of course this also depends on the required Qt components):
- macOS:
  - `libexec/{moc,rcc}`
  - `lib/Qt{Concurrent,Core,DBus,Gui,OpenGL,OpenGLWidgets,Svg,SvgWidgets,Widgets}.framework/`
  - `plugins/{imageformats/libqjpeg,platforms/libqcocoa,styles/libqmacstyle}.dylib`
- Windows:
  - `bin/{moc,rcc}.exe`
  - `bin/Qt6{Concurrent,Core,Gui,OpenGL,OpenGLWidgets,Svg,SvgWidgets,Widgets}[d].dll`
  - `include/Qt{Concurrent,Core,Gui,OpenGL,OpenGLWidgets,Svg,SvgWidgets,Widgets}/`
  - `lib/Qt6{Concurrent,Core,EntryPoint,Gui,OpenGL,OpenGLWidgets,Svg,SvgWidgets,Widgets}[d].lib`
  - `plugins/{imageformats/qjpeg,platforms/qwindows}[d].dll`
8. The Qt directory can be removed now. On Windows, the "Uninstall Qt" program from the start menu should be used for that.
9. On macOS, remove the `.prl` files in the `Resources` directories of the frameworks: `find Util/qt/macOS/lib -name '*.prl' | xargs rm`
10. Adjust CMake files, e.g. to update dependency relations, new components etc.
11. Update these instructions.

On Windows, the debug versions of the libraries are actually important because the others are incompatible with the debug C++ runtime.

TODO: More information.
