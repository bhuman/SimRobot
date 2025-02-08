/**
 * @file SimRobot/Main.cpp
 * Implementation of the main function of SimRobot
 * @author Colin Graf
 */

#include <QApplication>
#include <QLocale>
#include <QSurfaceFormat>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
extern void qt_registerDefaultPlatformBackingStoreOpenGLSupport();
#endif

#ifdef WINDOWS
#include <crtdbg.h>
#else
#include <clocale>
#endif

#include "MainWindow.h"

#ifdef MACOS
#include <QFileOpenEvent>

/** The address of the main window object used by the following class. */
static MainWindow* mainWindow = nullptr;

/**
 * A helper for opening files when they were launched from the Finder and closing windows
 * in the correct order.
 * macOS triggers an event for them rather than passing them as a command line
 * parameter. This class handles that event.
 */
class SimRobotApp : public QApplication
{
public:
  SimRobotApp(int& argc, char** argv)
    : QApplication(argc, argv) {}

protected:
  bool event(QEvent* ev)
  {
    if(ev->type() == QEvent::FileOpen)
    {
      mainWindow->openFile(static_cast<QFileOpenEvent*>(ev)->file());
      return true;
    }
    return QApplication::event(ev);
  }
};

/** Use the new class rather than the default one. */
#define QApplication SimRobotApp
#endif // MACOS

int main(int argc, char* argv[])
{
#ifdef WINDOWS
  _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
  //_CrtSetBreakAlloc(18969); // Use to track down memory leaks
#endif

  // Handle floating point values as programming languages would.
  QLocale::setDefault(QLocale::C);

  QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#ifdef MACOS
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
  QSurfaceFormat format;
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSamples(1);
  format.setStencilBufferSize(0);
  QSurfaceFormat::setDefaultFormat(format);

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
  // Workaround: For OpenGL to be used in windows, support must be registered before the window is created.
  // The following function is declared as a constructor in QtOpenGL (i.e. executed at library loading time),
  // but since the SimRobot application doesn't reference QtOpenGL it isn't sufficient to link QtOpenGL
  // due to lazy loading. Therefore, we call this function here (probably resulting in the function being
  // called twice, but this is handled by the function).
  qt_registerDefaultPlatformBackingStoreOpenGLSupport();
#endif

  QApplication app(argc, argv);
#ifndef WINDOWS
  setlocale(LC_NUMERIC, "C");
#endif
  MainWindow mainWindow(argc, argv);

#ifdef WINDOWS
  app.setStyle("fusion");
#elif defined MACOS
  ::mainWindow = &mainWindow;
#endif

  app.setApplicationName("SimRobot");

  bool noWindow = false;

  // open file from commandline
  for(int i = 1; i < argc; i++)
  {
    if(*argv[i] != '-' && strcmp(argv[i], "YES"))
    {
#ifdef MACOS
      if(!mainWindow.isVisible() && !noWindow)
        mainWindow.show();
#endif
      mainWindow.openFile(argv[i]);
    }
    if(strcmp(argv[i], "-noWindow") == 0)
      noWindow = true;
  }

  if(
#ifdef MACOS
     !mainWindow.isVisible() &&
#endif
     !noWindow)
    mainWindow.show();

  return app.exec();
}
