#pragma once

#include "SimRobot.h"
#include <QElapsedTimer>
#include <QStatusBar>

class QAction;

class StatusBar : public QStatusBar
{
  Q_OBJECT

public:
  StatusBar(QWidget* parent);

  void addLabel(const SimRobot::Module* module, SimRobot::StatusLabel* statusLabel);
  void removeAllLabels();
  void removeLabelsFromModule(const SimRobot::Module* module);

  void setUserMessage(const QString& userMessage);

  /**
   * Updates the labels and the message.
   * The labels are only updated every \c labelInterval ms (their texts, e.g. the number of steps, change all the
   * time, and every change is painted into the backing store of the window and uploaded to the GPU, which Qt
   * skips if only widgets that render to a texture, like the 3D views, have changed).
   * @param force Whether the labels are updated even if they were updated recently.
   */
  void update(bool force = false);

  QAction* toggleViewAction();

private:
  class RegisteredLabel
  {
  public:
    const SimRobot::Module* module;
    SimRobot::StatusLabel* label;

    RegisteredLabel(const SimRobot::Module* module, SimRobot::StatusLabel* label) :
      module(module), label(label) {}
  };

  static constexpr int labelInterval = 100; /**< The minimum time between updates of the labels (in ms). */

  QAction* toggleViewAct;
  QElapsedTimer labelTimer; /**< Measures the time since the labels were updated. */
  QList<RegisteredLabel> registeredLabels;
  QString userMessage;
  QString latestMessage;

  void hideEvent(QHideEvent* event) override;
  void showEvent(QShowEvent* event) override;

private slots:
  void messageChanged(const QString& message);
};
