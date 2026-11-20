/**
 * @file SimRobot/SimulationScheduler.h
 * Declaration of class SimulationScheduler
 */

#pragma once

#include <QObject>
#include <QTimer>
#include <functional>

/**
 * Decides when the simulation is stepped and when the GUI is updated.
 *
 * Both happen on the main thread, so a step and a GUI update never overlap. Nothing here blocks the main thread:
 * waiting is done with a timer, so that the event loop can handle input, paints and other timers in the meantime.
 *
 * - The simulation runs as fast as possible or with a fixed duration per step (\c setStepInterval). The deadline of
 *   the next step is accumulated, so that the pace does not drift. If the simulation cannot keep up, it falls behind
 *   by at most \c maxLag and then runs as fast as it can.
 * - The GUI is updated with a fixed interval or "every frame", i.e. once per refresh of the display. A GUI update
 *   makes the main thread wait for the display when the window is presented. Therefore, in the latter case it is
 *   started so shortly before the display refreshes that the wait is short, which leaves almost the whole refresh
 *   period to the simulation. Steps are not started shortly before the GUI update is due, and are paused from the
 *   GUI update until the frame was presented, because the paint would otherwise be delayed by them.
 */
class SimulationScheduler : public QObject
{
  Q_OBJECT

public:
  static constexpr int defaultGuiUpdateInterval = 100; /**< The initial interval of GUI updates (in ms). */

  /**
   * @param step Steps the simulation once.
   * @param updateGui Updates (i.e. schedules the repaint of) the GUI.
   * @param refreshRate Returns the refresh rate (in Hz) of the screen that shows the window.
   * @param parent The parent object.
   */
  SimulationScheduler(std::function<void()> step, std::function<void()> updateGui, std::function<double()> refreshRate, QObject* parent);

  bool isRunning() const {return running;}

  /** Runs the simulation until \c stop is called. */
  void start();

  /** Stops the simulation (it can be called from within a step). The GUI is updated once more. */
  void stop();

  /** Performs a single step and updates the GUI. */
  void stepOnce();

  /** Forgets everything about a running simulation (e.g. when the file is closed). Nothing is updated anymore. */
  void reset();

  /**
   * Sets the duration of a simulation step in real time.
   * @param seconds The duration or 0 for as fast as possible.
   */
  void setStepInterval(double seconds);

  /**
   * Sets how often the GUI is updated.
   * @param milliseconds The interval or 0 for once per refresh of the display.
   */
  void setGuiUpdateInterval(int milliseconds);

  /** @return How often the GUI is updated (in ms, 0 for once per refresh of the display). */
  int getGuiUpdateInterval() const {return guiInterval;}

public slots:
  /** Has to be called when the window was presented. */
  void frameFinished();

private:
  static constexpr double maxLag = 100.; /**< The maximum delay of the simulation relative to real time (in ms). */
  static constexpr double sliceDuration = 2.; /**< How long steps are executed before the event loop is run (in ms). */
  static constexpr double minLeadTime = 2.5; /**< The lead time of a GUI update is not reduced below this (in ms). */
  static constexpr double leadTimeDecay = 0.00007; /**< How much the lead time shrinks per ms (so 0.07 ms per second). */

  /** @return The time in ms. */
  static double now();

  /** @return The duration of a refresh of the display (in ms). */
  double refreshPeriod() const;

  /** @return When the next GUI update is due. */
  double guiDue() const;

  void schedule(double time);
  void slice();
  void doUpdateGui();

  std::function<void()> step;
  std::function<void()> updateGui;
  std::function<double()> refreshRate;
  QTimer timer;

  bool running = false;
  bool singleStep = false; /**< A single step is requested while the simulation is not running. */
  bool finalUpdate = false; /**< The GUI has to be updated after the simulation has stopped. */
  double stepInterval = 0.; /**< ms, 0 for as fast as possible. */
  double nextStep = 0.; /**< When the next step is due (ms). */
  double stepCost = 0.; /**< The average duration of recent steps (ms). */

  int guiInterval = defaultGuiUpdateInterval; /**< ms, 0 for once per refresh of the display. */
  double lastGuiUpdate = 0.; /**< When the GUI was updated the last time (ms). */
  bool framesExpected = false; /**< Whether presenting a frame was reported after the last GUI updates. */
  bool waitingForFrame = false; /**< A GUI update was done and the frame has not been presented yet. */
  double frameTimeout = 0.; /**< When to stop waiting for the frame. */
  double lastFrame = 0.; /**< When the last frame has been presented (ms). */
  double excess = 0.; /**< The average time that frames took longer than the refresh period (ms). */
  double leadTime = 3.; /**< How long before the display refreshes the GUI update has to start (in ms). */
};
