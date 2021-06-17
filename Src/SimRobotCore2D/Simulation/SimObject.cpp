/**
 * @file SimObject.cpp
 *
 * This file implements a base class for objects with a name and a transformation.
 *
 * @author Arne Hasselbring
 */

#include "SimObject.h"
#include "CoreModule.h"
#include "SimObjectPainter.h"
#include "SimObjectWidget.h"
#include <typeinfo>
#include <box2d/b2_math.h>

SimObject::~SimObject()
{
  delete translation;
  delete rotation;
}

void SimObject::registerObjects()
{
  for(SimObject* simObject : children)
  {
    if(simObject->name.empty())
    {
      const char* typeName = typeid(*simObject).name();
#ifdef WINDOWS
      const char* str = std::strchr(typeName, ' ');
      if(str)
        typeName = str + 1;
#else
      while(*typeName >= '0' && *typeName <= '9')
        ++typeName;
#endif
      simObject->fullName = fullName + "." + typeName;
    }
    else
      simObject->fullName = fullName + "." + QString::fromStdString(simObject->name);
    CoreModule::application->registerObject(*CoreModule::module, dynamic_cast<SimRobot::Object&>(*simObject), dynamic_cast<SimRobot::Object*>(this));
    simObject->registerObjects();
  }
}

void SimObject::addParent(Element& element)
{
  dynamic_cast<SimObject*>(&element)->children.push_back(this);
}

const QString& SimObject::getFullName() const
{
  return fullName;
}

const QIcon* SimObject::getIcon() const
{
  return &CoreModule::module->objectIcon;
}

SimRobot::Widget* SimObject::createWidget()
{
  return new SimObjectWidget(*this);
}

SimRobotCore2D::Painter* SimObject::createPainter()
{
  return new SimObjectPainter(*this);
}
