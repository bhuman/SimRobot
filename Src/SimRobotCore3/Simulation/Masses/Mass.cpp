/**
 * @file Simulation/Masses/Mass.cpp
 * Implementation of class Mass
 * @author Colin Graf
 */

#include "Mass.h"
#include "Platform/Assert.h"

void Mass::Properties::add(const Properties& other)
{
  const float totalMass = mass + other.mass;
  if(totalMass <= 0.f)
    return;
  const Vector3f newCom = (mass * com + other.mass * other.com) / totalMass;
  // Parallel axis theorem for both inertia matrices, which are given with respect to their own center of mass.
  const auto shifted = [&newCom](const Properties& properties)
  {
    const Vector3f offset = properties.com - newCom;
    return Matrix3f(properties.inertia + properties.mass * (offset.squaredNorm() * Matrix3f::Identity() - offset * offset.transpose()));
  };
  inertia = shifted(*this) + shifted(other);
  com = newCom;
  mass = totalMass;
}

Mass::Properties Mass::Properties::transformed(const RotationMatrix* rotation, const Vector3f* translation) const
{
  Properties result = *this;
  if(rotation)
  {
    const Matrix3f& matrix = *rotation;
    result.com = matrix * com;
    result.inertia = matrix * inertia * matrix.transpose();
  }
  if(translation)
    result.com += *translation;
  return result;
}

void Mass::Properties::getFullInertia(float* fullInertia) const
{
  fullInertia[0] = inertia(0, 0);
  fullInertia[1] = inertia(1, 1);
  fullInertia[2] = inertia(2, 2);
  fullInertia[3] = inertia(0, 1);
  fullInertia[4] = inertia(0, 2);
  fullInertia[5] = inertia(1, 2);
}

Mass::Properties Mass::createMass() const
{
  Properties properties = assembleMass();
  for(const SimObject* iter : children)
  {
    const auto* childMass = dynamic_cast<const Mass*>(iter);
    ASSERT(childMass);
    properties.add(childMass->createMassInParent());
  }
  return properties;
}

Mass::Properties Mass::assembleMass() const
{
  return Properties();
}
