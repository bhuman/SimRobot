/**
 * @file SimRobot/Main.cpp
 * Implementation of the main function of SimRobot
 * @author Colin Graf
 */

#include <QApplication>
#include <QLocale>
#include <QSurfaceFormat>

extern void qt_registerDefaultPlatformBackingStoreOpenGLSupport();

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
 * Also, on a quit event, floating dock widgets could be closed first (without being deleted),
 * which would unregister them in the main window. If the main window was closed after that,
 * all loaded modules would be unloaded before the widgets are destroyed. Therefore, the
 * quit event is intercepted here and the main window is closed. This means that when the
 * quit event is triggered by the main window being closed, the main window is effectively
 * closed twice, but this is no problem as the main window is not destroyed on being closed.
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
    else if(ev->type() == QEvent::Quit && !mainWindow->close())
    {
      ev->ignore();
      return false;
    }
    return QApplication::event(ev);
  }
};

/** Use the new class rather than the default one. */
#define QApplication SimRobotApp

/**
 * A helper for diplaying QStrings in Xcode that converts UTF16 strings to UTF8
 * character arrays that Xcode can display.
 * @param string The string to convert.
 * @return The address of the converted string. Note that it points to a static
 *         buffer that will be reused in the next call.
 */
const char* _fromQString(const QString& string)
{
  static char buffer[1000];
  strncpy(buffer, string.toUtf8().constData(), sizeof(buffer));
  buffer[sizeof(buffer) - 1] = 0;
  return buffer;
}
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
  QSurfaceFormat format;
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSamples(1);
  format.setStencilBufferSize(0);
  QSurfaceFormat::setDefaultFormat(format);

  // Workaround: For OpenGL to be used in windows, support must be registered before the window is created.
  // The following function is declared as a constructor in QtOpenGL (i.e. executed at library loading time),
  // but since the SimRobot application doesn't reference QtOpenGL it isn't sufficient to link QtOpenGL
  // due to lazy loading. Therefore, we call this function here (probably resulting in the function being
  // called twice, but this is handled by the function).
  qt_registerDefaultPlatformBackingStoreOpenGLSupport();

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

#ifdef MACOS
  mainWindow.show();
#endif

  // open file from commandline
  for(int i = 1; i < argc; i++)
    if(*argv[i] != '-' && strcmp(argv[i], "YES"))
    {
      mainWindow.openFile(argv[i]);
      break;
    }

#ifndef MACOS
  mainWindow.show();
#endif

  return app.exec();
}
