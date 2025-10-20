/**
 * @file Simulation/UserInput.h
 * Declaration of class to forward user input. An actuator port can be set
 * by the user and a controller can read that value through a sensor port.
 * @author Thomas Röfer
 */

#pragma once

#include "SimRobotCore3.h"
#include "Simulation/SimObject.h"
#include <QStringList>

class UserInput : public SimObject, public SimRobot::Object
{
public:
  class InputPort : public SimRobotCore3::ActuatorPort
  {
  public:
    QString fullName; /**< The path name to the object in the scene graph */
    QString unit; /**< The unit of the input's value */
    float min; /**< The minimum value */
    float max; /**< The maximum value */
    float defaultValue; /**< The value returned when this user input is turned off. */
    SimRobotCore3::SensorPort::Data data; /**< The current value */

    // API
    bool getMinAndMax(float& min, float& max) const override {min = this->min; max = this->max; return true;}
    const QString& getFullName() const override {return fullName;}
    const QIcon* getIcon() const override;
    SimRobot::Widget* createWidget() override;
    const QString& getUnit() const override {return unit;}
    void setValue(float value) override;
  } inputPort;

private:
  class OutputPort : public SimRobotCore3::SensorPort
  {
  public:
    const InputPort* input; /**< The input actuator port */
    QList<int> dimensions; /**< Dummy dimensions to return. */
    QStringList descriptions; /**< Dummy descriptions to return. */

    // API
    const QString& getFullName() const override {return input->fullName;}
    const QIcon* getIcon() const override;
    SimRobot::Widget* createWidget() override;
    const QList<int>& getDimensions() const override {return dimensions;}
    const QStringList& getDescriptions() const override {return descriptions;}
    const QString& getUnit() const override {return input->unit;}
    SensorType getSensorType() const override {return SensorType::floatSensor;}
    Data getValue() override {return input->data;}
    bool renderCameraImages(SimRobotCore3::SensorPort**, unsigned int) override {return false;}
    bool getMinAndMax(float& min, float& max) const override {return input->getMinAndMax(min, max);}
  } outputPort;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;

  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override;
};
