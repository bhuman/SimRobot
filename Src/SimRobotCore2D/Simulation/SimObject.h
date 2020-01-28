/**
 * @file SimObject.h
 *
 * This file declares a base class for objects with a name and a transformation.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Parser/Element.h"
#include "SimRobotCore2D.h"
#include "SimRobot.h"
#include <QString>
#include <QTransform>
#include <list>
#include <string>

struct b2Rot;
struct b2Vec2;
class QIcon;

class SimObject : public Element
{
public:
  /** Destructor. */
  ~SimObject() override;

  /** Registers child objects in the scene graph. */
  virtual void registerObjects();

  QString fullName; /**< The path name to the object in the scene graph. */
  std::string name; /**< The name of the object in the scene graph (without the path to it). */
  std::list<SimObject*> children; /**< The list of children of this object. */
  b2Vec2* translation = nullptr; /**< The initial translation of this object relative to its parent. */
  b2Rot* rotation = nullptr; /**< The initial rotation of this object relative to its parent */
  QTransform transformation; /**< The transformation of this object relative to its parent (or the world if this is a body). */

protected:
  /**
   * Registers another scene graph element as parent of this element.
   * @param element The element to register.
   */
  void addParent(Element& element) override;

  /**
   * Returns the full path to the object in the scene graph.
   * @return The full path ...
   */
  [[nodiscard]] virtual const QString& getFullName() const;

  /**
   * Returns an icon to visualize the object in the scene graph.
   * @return An icon ...
   */
  [[nodiscard]] virtual const QIcon* getIcon() const;

  /**
   * Creates a widget for this object.
   * @return The new widget instance.
   */
  virtual SimRobot::Widget* createWidget();

  /**
   * Creates a painter for this object.
   * @return The new painter instance.
   */
  virtual SimRobotCore2D::Painter* createPainter();
};
