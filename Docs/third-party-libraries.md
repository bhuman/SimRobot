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

For macOS, the Qt version is currently 6.3.1. The steps to populate `Util/qt/macOS` (after removing its previous content) were as follows (of course, future versions can require different steps):
1. Download the macOS online installer from [here](https://www.qt.io/download-qt-installer) (the resulting file should be called something like `qt-unified-macOS-x64-4.4.1-online.dmg`).
2. Execute the installer in that disk image.
3. Proceed through the installer by signing in with a Qt account and agreeing to the open source conditions.
4. Choose a custom installation and some destination folder, e.g. `~/Qt`.
5. In the following screen, everything can be deselected, but `Qt>Qt 6.3.1>macOS` has to be selected (it seems that QtCreator cannot be deselected).
6. Proceed by agreeing to the license terms and confirming the installation.
7. Copy the relevant files/directories from `<Qt directory>/6.3.1/macos` to `Util/qt/macOS`. In 6.3.1, they were the following (of course this also depends on the required Qt components):
- `Licenses/`
- `libexec/{moc,rcc}`
- `lib/Qt{Concurrent,Core,DBus,Gui,OpenGL,OpenGLWidgets,Svg,SvgWidgets,Widgets}.framework/`
- `plugins/{imageformats/libqjpeg,platforms/libqcocoa,styles/libqmacstyle}.dylib`
8. Remove `.prl` files: `find Util/qt/macOS/lib -name '*.prl' | xargs rm`
9. Adjust CMake files, e.g. to update dependency relations, new components etc.
10. Update these instructions.

TODO: More information.
