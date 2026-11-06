/**
 * @file Simulation/Masses/Mass.h
 * Declaration of class Mass
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore3.h"
#include "Simulation/SimObject.h"
#include "Simulation/PhysicalObject.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/RotationMatrix.h"

/**
 * @class Mass
 * Abstract class for masses of physical objects
 */
class Mass : public SimObject, public SimRobotCore3::Mass
{
public:
  /**
   * The physical properties of a rigid body that are relevant for its dynamics.
   */
  struct Properties
  {
    float mass = 0.f; /**< The total mass. */
    Vector3f com = Vector3f::Zero(); /**< The center of mass. */
    Matrix3f inertia = Matrix3f::Zero(); /**< The inertia matrix with respect to the center of mass (but with the axes of the reference frame). */

    /**
     * Adds another mass (given in the same frame) to this one.
     * @param other The mass to add.
     */
    void add(const Properties& other);

    /**
     * Expresses these properties in a frame that is rotated and translated relative to the current one.
     * @param rotation The rotation of the current frame in the new frame (can be \c nullptr).
     * @param translation The position of the origin of the current frame in the new frame (can be \c nullptr).
     * @return The properties in the new frame.
     */
    Properties transformed(const RotationMatrix* rotation, const Vector3f* translation) const;

    /**
     * Writes the inertia matrix in the order that MuJoCo expects (xx, yy, zz, xy, xz, yz).
     * @param fullInertia The array to write to.
     */
    void getFullInertia(float* fullInertia) const;
  };

  /**
   * Creates the mass of a physical object (including children, but not including its own \c translation and \c rotation).
   * @return The mass properties in the frame of this mass
   */
  Properties createMass() const;

  /**
   * Creates the mass of a physical object (including children and its own \c translation and \c rotation).
   * @return The mass properties in the frame of the parent
   */
  Properties createMassInParent() const {return createMass().transformed(rotation, translation);}

protected:
  /**
   * Computes the mass properties of this element alone.
   * @return The mass properties (not including children, \c translation or \c rotation)
   */
  virtual Properties assembleMass() const;

private:
  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override {return SimObject::getIcon();}
  SimRobotCore3::Renderer* createRenderer() override {return SimObject::createRenderer();}
};
