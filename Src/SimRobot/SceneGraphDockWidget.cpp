#include <QTreeWidget>
#include <QHeaderView>
#include <QSettings>
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>

#include "SceneGraphDockWidget.h"
#include "MainWindow.h"
#include "Theme.h"

SceneGraphDockWidget::SceneGraphDockWidget(QMenu* contextMenu, QWidget* parent) : QDockWidget(parent), contextMenu(contextMenu)
{
  setFeatures(features() & ~DockWidgetFloatable);
  setAllowedAreas(Qt::TopDockWidgetArea);
  setFocusPolicy(Qt::ClickFocus);
  setObjectName(".SceneGraph");
  setWindowTitle(tr("Scene Graph"));
  treeWidget = new QTreeWidget(this);
  italicFont = treeWidget->font();
  italicFont.setItalic(true);
  boldFont = treeWidget->font();
  boldFont.setBold(true);
  treeWidget->setFrameStyle(QFrame::NoFrame);
  setWidget(treeWidget);
  setFocusProxy(treeWidget);
  treeWidget->setExpandsOnDoubleClick(false);
  treeWidget->setHeaderHidden(true);

  connect(treeWidget, &QTreeWidget::activated, this, &SceneGraphDockWidget::itemActivated);
  connect(treeWidget, &QTreeWidget::collapsed, this, &SceneGraphDockWidget::itemCollapsed);
  connect(treeWidget, &QTreeWidget::expanded, this, &SceneGraphDockWidget::itemExpanded);

  // load layout settings
  QSettings& settings = MainWindow::application->getLayoutSettings();
  settings.beginGroup(".SceneGraph");
  QStringList expandedItemsList = settings.value("ExpandedItems").toStringList();
  expandedItems = QSet<QString>(expandedItemsList.begin(), expandedItemsList.end());
  settings.endGroup();
}

SceneGraphDockWidget::~SceneGraphDockWidget()
{
  // save layout settings
  QSettings& settings = MainWindow::application->getLayoutSettings();
  settings.beginGroup(".SceneGraph");
  settings.setValue("ExpandedItems", QStringList(expandedItems.values()));
  settings.endGroup();

  //
  unregisterAllObjects();
  Q_ASSERT(registeredObjectsByKindAndName.isEmpty());
}

void SceneGraphDockWidget::registerObject(const SimRobot::Module* module, SimRobot::Object* object, const SimRobot::Object* parent, int flags)
{
  QTreeWidgetItem* parentItem = parent ? registeredObjectsByObject.value(parent) : treeWidget->invisibleRootItem();
  RegisteredObject* newItem = new RegisteredObject(module, object, parentItem, flags);
  const auto parentFullNameLength = parent ? static_cast<RegisteredObject*>(parentItem)->fullName.length() : 0;
  newItem->setText(0, parent ? newItem->fullName.mid(parentFullNameLength + 1) : newItem->fullName);
  const QIcon* icon = object->getIcon();
  if(icon)
    newItem->setIcon(0, Theme::updateIcon(this, *icon));
  if(flags & SimRobot::Flag::hidden)
    newItem->setHidden(true);
  if(flags & SimRobot::Flag::windowless)
    newItem->setFont(0, italicFont);
  parentItem->addChild(newItem);
  if(!parent || (flags & SimRobot::Flag::sorted))
    parentItem->sortChildren(0, Qt::AscendingOrder);
  if(expandedItems.contains(newItem->fullName))
    treeWidget->expandItem(newItem);

  registeredObjectsByObject.insert(object, newItem);

  int kind = object->getKind();
  QHash<QString, RegisteredObject*>* registeredObjectsByName = registeredObjectsByKindAndName.value(kind);
  if(!registeredObjectsByName)
  {
    registeredObjectsByName = new QHash<QString, RegisteredObject*>();
    registeredObjectsByKindAndName.insert(kind, registeredObjectsByName);
  }

  registeredObjectsByName->insert(newItem->fullName, newItem);

  if(flags & SimRobot::Flag::showParent)
    while(parentItem)
    {
      parentItem->setHidden(false);
      parentItem = parentItem->parent();
    }
}

void SceneGraphDockWidget::unregisterAllObjects()
{
  registeredObjectsByObject.clear();
  qDeleteAll(registeredObjectsByKindAndName);
  registeredObjectsByKindAndName.clear();
  treeWidget->clear();
}

void SceneGraphDockWidget::unregisterObjectsFromModule(const SimRobot::Module* module)
{
  for(int i = treeWidget->topLevelItemCount() - 1; i >= 0; --i)
    deleteRegisteredObjectsFromModule(static_cast<RegisteredObject*>(treeWidget->topLevelItem(i)), module);
}

bool SceneGraphDockWidget::unregisterObject(const SimRobot::Object* object)
{
  RegisteredObject* regObject = registeredObjectsByObject.value(object);
  if(!regObject)
    return false;
  deleteRegisteredObject(regObject);
  return true;
}

SimRobot::Object* SceneGraphDockWidget::resolveObject(const QString& fullName, int kind)
{
  for(QHash<int, QHash<QString, RegisteredObject*>*>::iterator i = kind ? registeredObjectsByKindAndName.find(kind) : registeredObjectsByKindAndName.begin(); i != registeredObjectsByKindAndName.end(); ++i)
  {
    const QHash<QString, RegisteredObject*>* registeredObjectsByName = *i;
    if(!registeredObjectsByName)
      continue;
    RegisteredObject* object = registeredObjectsByName->value(fullName);
    if(object)
      return object->object;

    if(kind)
      break;
  }
  return nullptr;
}

SimRobot::Object* SceneGraphDockWidget::resolveObject(const SimRobot::Object* parent, const QVector<QString>& parts, int kind)
{
  const auto partsCount = parts.count();
  if(partsCount <= 0)
    return nullptr;
  for(QHash<int, QHash<QString, RegisteredObject*>*>::iterator i = kind ? registeredObjectsByKindAndName.find(kind) : registeredObjectsByKindAndName.begin(); i != registeredObjectsByKindAndName.end(); ++i)
  {
    QHash<QString, RegisteredObject*>* registeredObjectsByName = *i;
    if(!registeredObjectsByName)
      continue;
    const QString& lastPart = parts.at(partsCount - 1);
    for(RegisteredObject* object : *registeredObjectsByName)
    {
      if(object->fullName.endsWith(lastPart))
      {
        RegisteredObject* currentObject = object;
        for(auto i = partsCount - 2; i >= 0; --i)
        {
          currentObject = static_cast<RegisteredObject*>(currentObject->parent());
          const QString& currentPart = parts.at(i);
          for(;;)
          {
            if(!currentObject)
              goto continueSearch;
            if(currentObject->fullName.endsWith(currentPart))
              break;
            currentObject = static_cast<RegisteredObject*>(currentObject->parent());
          }
        }
        if(parent)
        {
          currentObject = static_cast<RegisteredObject*>(currentObject->parent());
          for(;;)
          {
            if(!currentObject)
              goto continueSearch;
            if(currentObject->object == parent)
              break;
            currentObject = static_cast<RegisteredObject*>(currentObject->parent());
          }
        }
        return object->object;
      }
    continueSearch:
      ;
    }

    if(kind)
      break;
  }
  return nullptr;
}

int SceneGraphDockWidget::getObjectChildCount(const SimRobot::Object* object)
{
  const RegisteredObject* item = registeredObjectsByObject.value(object);
  return item ? item->childCount() : 0;
}

SimRobot::Object* SceneGraphDockWidget::getObjectChild(const SimRobot::Object* object, int index)
{
  const RegisteredObject* item = registeredObjectsByObject.value(object);
  return item && index >= 0 && index < item->childCount() ? static_cast<RegisteredObject*>(item->child(index))->object : 0;
}

bool SceneGraphDockWidget::activateFirstObject()
{
  RegisteredObject* item = static_cast<RegisteredObject*>(treeWidget->invisibleRootItem()->child(0));
  if(!item)
    return false;
  emit activatedObject(item->fullName, item->module, item->object, item->flags);
  return true;
}

bool SceneGraphDockWidget::activateObject(const SimRobot::Object* object)
{
  RegisteredObject* item = registeredObjectsByObject.value(object);
  if(!item)
    return false;
  emit activatedObject(item->fullName, item->module, item->object, item->flags);
  return true;
}

bool SceneGraphDockWidget::setOpened(const SimRobot::Object* object, bool opened)
{
  RegisteredObject* item = registeredObjectsByObject.value(object);
  if(!item)
    return false;
  item->opened = opened;
  item->setFont(0, opened ? boldFont : QFont());
  if(!opened)
    item->setSelected(false);
  return true;
}

bool SceneGraphDockWidget::setActive(const SimRobot::Object* object, bool active)
{
  treeWidget->selectionModel()->clearSelection();
  RegisteredObject* item = registeredObjectsByObject.value(object);
  if(!item)
    return false;
  item->setSelected(active);
  return true;
}

QAction* SceneGraphDockWidget::toggleViewAction() const
{
  QAction* action = QDockWidget::toggleViewAction();
  QIcon icon(":/Icons/icons8-stacked-organizational-chart-50.png");
  icon.setIsMask(true);
  action->setIcon(icon);
  action->setShortcut(QKeySequence(Qt::Key_F2));
  action->setText(tr("Scene Graph"));
  action->setStatusTip(tr("Show or hide the scene graph"));
  return action;
}

void SceneGraphDockWidget::deleteRegisteredObjectsFromModule(RegisteredObject* registeredObject, const SimRobot::Module* module)
{
  if(registeredObject->module == module)
    deleteRegisteredObject(registeredObject);
  else
    for(int i = registeredObject->childCount() - 1; i >= 0; --i)
      deleteRegisteredObjectsFromModule(static_cast<RegisteredObject*>(registeredObject->child(i)), module);
}

void SceneGraphDockWidget::deleteRegisteredObject(RegisteredObject* registeredObject)
{
  for(int i = registeredObject->childCount() - 1; i >= 0; --i)
    deleteRegisteredObject(static_cast<RegisteredObject*>(registeredObject->child(i)));
  registeredObjectsByObject.remove(registeredObject->object);
  int kind = registeredObject->object->getKind();
  QHash<QString, RegisteredObject*>* registeredObjectsByName = registeredObjectsByKindAndName.value(kind);
  if(registeredObjectsByName)
  {
    registeredObjectsByName->remove(registeredObject->fullName);
    if(registeredObjectsByName->count() == 0)
    {
      registeredObjectsByKindAndName.remove(kind);
      delete registeredObjectsByName;
    }
  }
  delete registeredObject;
}

void SceneGraphDockWidget::contextMenuEvent(QContextMenuEvent* event)
{
  const QRect content(treeWidget->geometry());
  if(!content.contains(event->x(), event->y()))
  {
    // click on window frame
    QDockWidget::contextMenuEvent(event);
    return;
  }

  clickedItem = static_cast<RegisteredObject*>(treeWidget->itemAt(treeWidget->mapFromParent(event->pos())));

  QMenu menu;
  if(clickedItem)
  {
    if(!(clickedItem->flags & SimRobot::Flag::windowless))
    {
      QAction* action = menu.addAction(tr(clickedItem->opened ? "&Close" : "&Open"));
      connect(action, &QAction::triggered, this, &SceneGraphDockWidget::openOrCloseObject);
      menu.addSeparator();
    }
    if(clickedItem->childCount() > 0)
    {
      QAction* action = menu.addAction(tr(clickedItem->isExpanded() ? "Collaps&e" : "&Expand"));
      connect(action, &QAction::triggered, this, &SceneGraphDockWidget::expandOrCollapseObject);
      menu.addSeparator();
    }
  }
  menu.addAction(contextMenu->menuAction());
  event->accept();
  menu.exec(mapToGlobal(QPoint(event->x(), event->y())));
  clickedItem = 0;
}

void SceneGraphDockWidget::changeEvent(QEvent* event)
{
  if(event->type() == QEvent::PaletteChange)
  {
    for(QTreeWidgetItem* item : treeWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive))
      if(!item->icon(0).isNull())
        item->setIcon(0, Theme::updateIcon(this, item->icon(0)));
  }
  QDockWidget::changeEvent(event);
}

void SceneGraphDockWidget::itemActivated(const QModelIndex& index)
{
  RegisteredObject* item = static_cast<RegisteredObject*>(index.internalPointer());
  if(item->flags & SimRobot::Flag::windowless)
  {
    if(item->isExpanded())
      treeWidget->collapseItem(item);
    else
      treeWidget->expandItem(item);
    // the object does not have a widget, but it might have a simple
    // widget-less callback - call it (by default an empty callback
    // stub is provided)
    item->object->widgetlessActivationCallback();
  }
  else
    emit activatedObject(item->fullName, item->module, item->object, item->flags);
}

void SceneGraphDockWidget::itemCollapsed(const QModelIndex& index)
{
  RegisteredObject* item = static_cast<RegisteredObject*>(index.internalPointer());
  expandedItems.remove(item->fullName);
}

void SceneGraphDockWidget::itemExpanded(const QModelIndex& index)
{
  RegisteredObject* item = static_cast<RegisteredObject*>(index.internalPointer());
  expandedItems.insert(item->fullName);
}

void SceneGraphDockWidget::openOrCloseObject()
{
  if(clickedItem->opened)
    emit deactivatedObject(clickedItem->fullName);
  else
    emit activatedObject(clickedItem->fullName, clickedItem->module, clickedItem->object, clickedItem->flags);
}

void SceneGraphDockWidget::expandOrCollapseObject()
{
  if(clickedItem->isExpanded())
    treeWidget->collapseItem(clickedItem);
  else
    treeWidget->expandItem(clickedItem);
}
