/**
 * @file SimRobot/MainWindow.h
 * Declaration of the main window of SimRobot
 * @author Colin Graf
 */

#pragma once

#include <QMainWindow>
#include <QActionGroup>
#include <QSettings>
#include <QSet>
#include <QHash>
#include <QLibrary>

#include "SimRobot.h"

class SceneGraphDockWidget;
class RegisteredDockWidget;
class StatusBar;

class MainWindow : public QMainWindow, public SimRobot::Application
{
  Q_OBJECT

public:
  static SimRobot::Application* application;

  MainWindow(int argc, char* argv[]);

  QMenu* createSimMenu();

private:

  static QString getAppPath(const char* argv0);
  static unsigned int getAppLocationSum(const QString& appPath);
  static unsigned int getSystemTime();

  class LoadedModule : public QLibrary
  {
  public:
    SimRobot::Module* module = nullptr;
    bool compiled = false;
    using CreateModuleProc = SimRobot::Module* (*)(SimRobot::Application&);
    CreateModuleProc createModule = nullptr;

    LoadedModule(const QString& name) : QLibrary(name) {}
  };

  int timerId = 0; /**< The id of the timer used to get something like an OnIdle callback function to update the simulation. */

  QAction* fileOpenAct;
  QAction* fileCloseAct;
#ifndef MACOS
  QAction* fileExitAct;
#endif
  QAction* toolbarOpenAct;
  QAction* simResetAct;
  QAction* simStartAct;
  QAction* simStepAct;

  QMenu* fileMenu;
  QMenu* recentFileMenu;
#ifdef FIX_MACOS_EDIT_MENU
  QMenu* editMenu;
  QAction* editMenuEndSeparator;
#endif
  QMenu* viewMenu;
  QMenu* viewUpdateRateMenu = nullptr;
  QActionGroup* viewUpdateRateActionGroup = nullptr;
  QMenu* addonMenu;
  QMenu* helpMenu;

  QToolBar* toolBar;
  StatusBar* statusBar;

  QString appPath;
  QString appString;

  QSettings settings;
  QSettings layoutSettings;
  QStringList recentFiles;

  bool opened = false;
  bool compiled = false;
  bool running = false;
  bool resetting = false;
  bool layoutRestored = true;
  int guiUpdateRate = 100;
  unsigned int lastGuiUpdate = 0;
  QString filePath; /**< the path to the currently opened file */

  class RegisteredModule
  {
  public:
    QString name;
    QString displayName;

    RegisteredModule(const QString& name, const QString& displayName) : name(name), displayName(displayName) {}
  };

  QMap<QString, RegisteredModule> registeredModules; /**< suggested modules (a.k.a. addons) */
  QStringList manuallyLoadedModules; /**< modules (a.k.a. addons) that were loaded manually */
  QList<LoadedModule*> loadedModules;
  QHash<QString, LoadedModule*> loadedModulesByName; /**< all loaded modules associated to the currently opened file */

  QDockWidget* activeDockWidget = nullptr;
  QMenu* dockWidgetFileMenu = nullptr;
  QMenu* dockWidgetEditMenu = nullptr;
  QMenu* dockWidgetUserMenu = nullptr;
  QMenu* moduleUserMenu = nullptr;

  SceneGraphDockWidget* sceneGraphDockWidget = nullptr;
  QStringList openedObjects;
  QMap<QString, RegisteredDockWidget*> openedObjectsByName;

  bool registerObject(const SimRobot::Module& module, SimRobot::Object& object, const SimRobot::Object* parent, int flags) override;
  bool unregisterObject(const SimRobot::Object& object) override;
  SimRobot::Object* resolveObject(const QString& fullName, int kind) override;
  SimRobot::Object* resolveObject(const QVector<QString>& parts, const SimRobot::Object* parent, int kind) override;
  int getObjectChildCount(const SimRobot::Object& object) override;
  SimRobot::Object* getObjectChild(const SimRobot::Object& object, int index) override;
  bool addStatusLabel(const SimRobot::Module& module, SimRobot::StatusLabel* statusLabel) override;
  bool registerModule(const SimRobot::Module& module, const QString& displayName, const QString& name) override;
  bool loadModule(const QString& name) override;
  bool openObject(const SimRobot::Object& object) override;
  bool closeObject(const SimRobot::Object& object) override;
  bool selectObject(const SimRobot::Object& object) override;
  void showWarning(const QString& title, const QString& message) override;
  const QString& getFilePath() const override { return filePath; }
  void setStatusMessage(const QString& message) override;
  const QString& getAppPath() const override {return appPath;}
  QSettings& getSettings() override {return settings;}
  QSettings& getLayoutSettings() override {return layoutSettings;}

  void paintEvent(QPaintEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void timerEvent(QTimerEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void changeEvent(QEvent* event) override;
  QMenu* createPopupMenu() override;

  bool loadModule(const QString& name, bool manually);
  void unloadModule(const QString& name);
  bool compileModules();
  void updateViewMenu(QMenu* menu);
  void addToolBarButtonsFromMenu(QMenu* menu, QToolBar* toolBar, bool addSeparator);

public slots:
  void openFile(const QString& fileName) override;

private slots:
  void unlockLayout();

  void updateFileMenu();
  void updateRecentFileMenu();
  void updateViewMenu();
  void updateAddonMenu();
  void updateMenuAndToolBar();

  void setGuiUpdateRate(int rate);

  void open();
  bool closeFile();
  void about();
  void loadAddon(const QString& name);

  void openObject(const QString& fullName, const SimRobot::Module* module, SimRobot::Object* object, int flags);
  void closeObject(const QString& fullName);
  void closedObject(const QString& fullName);
  void visibilityChanged(bool visible);

  void focusChanged(QWidget* old, QWidget* now);

public:
  // Provides information on whether the simulation is running or not
  bool isSimRunning() override { return running; }

  // Provides information on whether the simulation is currently resetting
  bool isSimResetting() override { return resetting; }

public slots:
  void simReset() override;
  void simStart() override;
  void simStep() override;
  void simStop() override;
  void applicationStateChanged(Qt::ApplicationState);
};
