#include "StatusBar.h"
#include <QAction>
#include <algorithm>

StatusBar::StatusBar(QWidget* parent) : QStatusBar(parent), toggleViewAct(0)
{
  userMessage = latestMessage = tr("Ready");
  showMessage(userMessage);
  connect(this, &QStatusBar::messageChanged, this, &StatusBar::messageChanged);
}

void StatusBar::addLabel(const SimRobot::Module* module, SimRobot::StatusLabel* statusLabel)
{
  insertPermanentWidget(0, statusLabel->getWidget());
  registeredLabels.append(RegisteredLabel(module, statusLabel));
}

void StatusBar::removeAllLabels()
{
  for(QList<RegisteredLabel>::iterator it = registeredLabels.begin(), end = registeredLabels.end(); it != end; ++it)
  {
    removeWidget(it->label->getWidget());
    delete it->label;
  }
  registeredLabels.clear();
}

void StatusBar::removeLabelsFromModule(const SimRobot::Module* module)
{
  for(QList<RegisteredLabel>::iterator it = registeredLabels.begin(); it != registeredLabels.end();)
    if(it->module == module)
    {
      removeWidget(it->label->getWidget());
      delete it->label;
      it = registeredLabels.erase(it);
    }
    else
      ++it;
}

void StatusBar::setUserMessage(const QString& userMessage)
{
  latestMessage = userMessage;
}

void StatusBar::update(bool force)
{
  if(force || !labelTimer.isValid() || labelTimer.hasExpired(labelInterval))
  {
    labelTimer.start();
    for(QList<RegisteredLabel>::iterator it = registeredLabels.begin(), end = registeredLabels.end(); it != end; ++it)
    {
      it->label->update();
      // A new text of a label invalidates the layout of the window, which is then laid out and painted again.
      // That would be one more frame (each presented when the display refreshes) per update. Hence, the size of a
      // label is fixed, and it only grows.
      QWidget* const widget = it->label->getWidget();
      const QSize size = widget->sizeHint();
      if(size.width() > widget->width() || size.height() != widget->height())
        widget->setFixedSize(std::max(size.width(), widget->width()), size.height());
    }
  }

  QString currentMessage = this->currentMessage();
  if(currentMessage.isEmpty() || currentMessage == userMessage)
  {
    userMessage = latestMessage;
    if(!currentMessage.isEmpty() || !userMessage.isEmpty())
      showMessage(userMessage);
  }
}

QAction* StatusBar::toggleViewAction()
{
  if(toggleViewAct)
    return toggleViewAct;
  toggleViewAct = new QAction(tr("&Status Bar"), this);
  toggleViewAct->setCheckable(true);
  toggleViewAct->setChecked(isVisible());
  connect(toggleViewAct, &QAction::triggered, this, &StatusBar::setVisible);
  return toggleViewAct;
}

void StatusBar::hideEvent(QHideEvent* event)
{
  QStatusBar::hideEvent(event);

  if(toggleViewAct)
    toggleViewAct->setChecked(false);
}

void StatusBar::showEvent(QShowEvent* event)
{
  QStatusBar::showEvent(event);

  if(toggleViewAct)
    toggleViewAct->setChecked(true);
}

void StatusBar::messageChanged(const QString& message)
{
  if(message.isEmpty() && (!userMessage.isEmpty() || latestMessage != userMessage))
  {
    userMessage = latestMessage;
    showMessage(userMessage);
  }
}
