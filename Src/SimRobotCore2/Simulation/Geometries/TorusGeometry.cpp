/**
 * @file Simulation/Geometries/TorusGeometry.cpp
 * Implementation of class TorusGeometry
 * @author Arne Hasselbring
 */

#include "TorusGeometry.h"
#include "Platform/Assert.h"
#include "Platform/OpenGL.h"
#include <ode/collision.h>
#include <ode/collision_space.h>
#include <ode/odemath.h>

struct TorusData
{
  dReal majorRadius; /**< Same as \c TorusGeometry::majorRadius */
  dReal minorRadius; /**< Same as \c TorusGeometry::minorRadius */
};

static int dTorusClass = dGeomNumClasses;

static inline dContactGeom* contactOffset(dContactGeom* contact, int offset)
{
  return reinterpret_cast<dContactGeom*>(reinterpret_cast<char*>(contact) + offset);
}

template<typename T>
static inline T sqr(T x)
{
  return x * x;
}

static int collideTorusSphere(dGeomID o1, dGeomID o2, int flags, dContactGeom* contact, int skip)
{
  ASSERT(skip >= 0 && static_cast<std::size_t>(skip) >= sizeof(dContactGeom));
  ASSERT(dGeomGetClass(o1) == dTorusClass);
  ASSERT(dGeomGetClass(o2) == dSphereClass);
  const int nContacts = flags & 0xffff;
  ASSERT(nContacts >= 1);

  const TorusData* torus = static_cast<const TorusData*>(dGeomGetClassData(o1));
  const dReal* torusR = dGeomGetRotation(o1);
  const dReal sphereRadius = dGeomSphereGetRadius(o2);
  const dReal* sphereP = dGeomGetPosition(o2);
  dVector3 sphereInTorus;
  dGeomGetPosRelPoint(o1, sphereP[0], sphereP[1], sphereP[2], sphereInTorus);

  int result = 0;
  const auto sphereInTorusPlaneNorm = dSqrt(sqr(sphereInTorus[0]) + sqr(sphereInTorus[1]));
  if(sphereInTorusPlaneNorm < REAL(1e-7))
  {
    // 1. The sphere is on the torus axis.

    // If the sphere is too small (or the torus too large), a collision is not possible.
    // The equals is there because it doesn't make sense to let an exactly fitting sphere get stuck in the center of the torus.
    if(sphereRadius + torus->minorRadius <= torus->majorRadius)
      return 0;

    // minHeight is the vertical offset to the torus plane at which the sphere would touch the torus.
    const dReal minHeight = dSqrt(sqr(torus->minorRadius + sphereRadius) - sqr(torus->majorRadius));
    if(dFabs(sphereInTorus[2]) >= minHeight)
      return 0;

    // Simulate this as a single contact (while in fact it is an entire circle) forcing the sphere out of the torus along its axis.
    dZeroVector3(contact->pos);
    dAssignVector3(contact->normal, REAL(0), REAL(0), -dCopySign(REAL(1), sphereInTorus[2]));
    contact->depth = minHeight - dFabs(sphereInTorus[2]);
    ++result;
  }
  else if(sphereInTorusPlaneNorm <= torus->majorRadius + torus->minorRadius + sphereRadius)
  {
    // 2. The sphere is close enough to the torus axis to potentially intersect.

    // ringPoint is the point on the ring skeleton that is closest to the sphere's center.
    // sphereInRingPoint is the center of the sphere relativ to that point.
    dVector3 ringPoint, sphereInRingPoint;
    ringPoint[0] = sphereInTorus[0] / sphereInTorusPlaneNorm * torus->majorRadius;
    ringPoint[1] = sphereInTorus[1] / sphereInTorusPlaneNorm * torus->majorRadius;
    ringPoint[2] = REAL(0);
    dSubtractVectors3(sphereInRingPoint, sphereInTorus, ringPoint);

    // If the sphere is closer to the ring point than the sum of their radii, there is a collision.
    const dReal distance = dCalcVectorLength3(sphereInRingPoint);
    if(distance > torus->minorRadius + sphereRadius)
      return 0;
    dCopyVector3(contactOffset(contact, result * skip)->pos, ringPoint);
    dCopyScaledVector3(contactOffset(contact, result * skip)->normal, sphereInRingPoint, -REAL(1) / distance);
    contactOffset(contact, result * skip)->depth = torus->minorRadius + sphereRadius - distance;
    ++result;

    // Only check for another contact if the caller accepts it.
    if(nContacts > result)
    {
      // Check for a collision at the mirrored ring point.
      // This means that the sphere is in the inner part of the torus.
      dVector3 otherRingPoint, sphereInOtherRingPoint;
      dCopyNegatedVector3(otherRingPoint, ringPoint);
      dSubtractVectors3(sphereInOtherRingPoint, sphereInTorus, otherRingPoint);
      const dReal distance = dCalcVectorLength3(sphereInOtherRingPoint);
      if(distance <= torus->minorRadius + sphereRadius)
      {
        dCopyVector3(contactOffset(contact, result * skip)->pos, otherRingPoint);
        dCopyScaledVector3(contactOffset(contact, result * skip)->normal, sphereInOtherRingPoint, -REAL(1) / distance);
        contactOffset(contact, result * skip)->depth = torus->minorRadius + sphereRadius - distance;
        ++result;
      }
    }
  }

  for(int i = 0; i < result; ++i)
  {
    auto* c = contactOffset(contact, i * skip);
    // Transform contact point and normal back to global frame.
    dGeomGetRelPointPos(o1, c->pos[0], c->pos[1], c->pos[2], c->pos);
    dMultiply0_331(c->normal, torusR, c->normal);
    c->g1 = o1;
    c->g2 = o2;
    c->side1 = -1;
    c->side2 = -1;
  }

  return result;
}

static void getTorusAABB(dGeomID geom, dReal aabb[6])
{
  const dReal* R = dGeomGetRotation(geom);
  const dReal* p = dGeomGetPosition(geom);
  const TorusData* torus = static_cast<const TorusData*>(dGeomGetClassData(geom));
  const dReal xrange = torus->majorRadius * (dFabs(R[0]) + dFabs(R[1])) + torus->minorRadius * dFabs(R[2]);
  const dReal yrange = torus->majorRadius * (dFabs(R[4]) + dFabs(R[5])) + torus->minorRadius * dFabs(R[6]);
  const dReal zrange = torus->majorRadius * (dFabs(R[8]) + dFabs(R[9])) + torus->minorRadius * dFabs(R[10]);
  aabb[0] = p[0] - xrange;
  aabb[1] = p[0] + xrange;
  aabb[2] = p[1] - yrange;
  aabb[3] = p[1] + yrange;
  aabb[4] = p[2] - zrange;
  aabb[5] = p[2] + zrange;
}

static dColliderFn* getTorusCollider(int num)
{
  switch(num)
  {
    case dSphereClass:
      return collideTorusSphere;
    default:
      return nullptr;
  }
}

dGeomID TorusGeometry::createGeometry(dSpaceID space)
{
  ASSERT(minorRadius < majorRadius);
  if(dTorusClass == dGeomNumClasses)
  {
    static dGeomClass torusClass;
    torusClass.bytes = sizeof(TorusData);
    torusClass.collider = getTorusCollider;
    torusClass.aabb = getTorusAABB;
    torusClass.aabb_test = nullptr;
    torusClass.dtor = nullptr;
    dTorusClass = dCreateGeomClass(&torusClass);
    ASSERT(dTorusClass >= dFirstUserClass && dTorusClass <= dLastUserClass);
  }

  Geometry::createGeometry(space);
  innerRadius = 0.f;
  innerRadiusSqr = 0.f;
  outerRadius = majorRadius + minorRadius;

  dGeomID geom = dCreateGeom(dTorusClass);
  if(space)
    dSpaceAdd(space, geom);
  TorusData* torus = static_cast<TorusData*>(dGeomGetClassData(geom));
  torus->majorRadius = majorRadius;
  torus->minorRadius = minorRadius;
  return geom;
}

void TorusGeometry::drawPhysics(unsigned int flags) const
{
  glPushMatrix();
  glMultMatrixf(transformation);

  if(flags & SimRobotCore2::Renderer::showPhysics)
  {
    glColor4fv(color);
    GLUquadricObj* q = gluNewQuadric();
    gluSphere(q, majorRadius + minorRadius, 16, 16);
    gluDeleteQuadric(q);
  }

  ::PhysicalObject::drawPhysics(flags);
  glPopMatrix();
}
