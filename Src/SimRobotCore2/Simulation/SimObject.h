/**
 * @file Simulation/SimObject.h
 * Declaration of class SimObject
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Parser/ElementCore2.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include "Tools/Math/RotationMatrix.h"
#include <QString>
#include <list>
#include <string>

/**
 * @class SimObject
 * Abstract class for scene graph objects with a name and a transformation
 */
class SimObject : public ElementCore2
{
public:
  QString fullName; /**< The path name to the object in the scene graph */
  std::string name; /**< The name of the scene graph object (without path) */
  std::list<SimObject*> children; /**< List of subordinate scene graph objects */
  Vector3f* translation = nullptr; /**< The initial translational offset relative to the origin of the parent object */
  RotationMatrix* rotation = nullptr; /**< The initial rotational offset relative to the origin of the parent object */
  Pose3f poseInParent; /**< The (updated) offset relative to the origin of the parent object */

  /** Destructor */
  ~SimObject();

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  virtual void registerObjects();

protected:
  /**
   * Registers an element as parent
   * @param element The element to register
   */
  virtual void addParent(Element& element);

protected:
  // API
  virtual const QString& getFullName() const {return fullName;}
  virtual SimRobot::Widget* createWidget();
  virtual const QIcon* getIcon() const;
  virtual SimRobotCore2::Renderer* createRenderer();
};
