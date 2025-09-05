/**
 * @file Simulation/Masses/Mass.cpp
 * Implementation of class Mass
 * @author Colin Graf
 */

#include "Mass.h"
#include "Platform/Assert.h"
#include <cstring>

float Mass::createMass(Vector3f& com, float* inertia)
{
  if(!created)
  {
    assembleMass();
    ASSERT(children.empty()); // TODO
    /*
    for(auto* iter : children)
    {
      auto* childMassDesc = dynamic_cast<Mass*>(iter);
      ASSERT(childMassDesc);
      const dMass& childMass = childMassDesc->createMass();
      if(childMassDesc->translation || childMassDesc->rotation)
      {
        dMass shiftedChildMass = childMass;
        if(childMassDesc->rotation)
        {
          dMatrix3 matrix;
          ODETools::convertMatrix(*childMassDesc->rotation, matrix);
          dMassRotate(&shiftedChildMass, matrix);
        }
        if(childMassDesc->translation)
          dMassTranslate(&shiftedChildMass, static_cast<dReal>(childMassDesc->translation->x()), static_cast<dReal>(childMassDesc->translation->y()), static_cast<dReal>(childMassDesc->translation->z()));
        dMassAdd(&mass, &shiftedChildMass);
      }
      else
        dMassAdd(&mass, &childMass);
    }
    */
    created = true;
  }
  com = this->com;
  std::memcpy(inertia, this->inertia, sizeof(this->inertia));
  return mass;
}

void Mass::assembleMass()
{
  mass = 0.f;
  std::memset(inertia, 0, sizeof(inertia));
}
