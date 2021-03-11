/**
 * @file Simulation/Axis.h
 * Declaration of class Axis
 * @author Colin Graf
 */

#pragma once

#include "Parser/Element.h"

class Joint;
class Motor;

/**
 * @class Axis
 * An axis of a joint
 */
class Axis : public Element
{
public:
  class Deflection
  {
  public:
    float min = 0.f;
    float max = 0.f;
    float stopCFM = -1.f;
    float stopERP = -1.f;
    float offset = 0.f;
  };

  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  float cfm = -1.f;
  Deflection* deflection = nullptr;
  Motor* motor = nullptr;
  Joint* joint = nullptr; /**< The joint that own this axis. */

  /** Destructor */
  ~Axis();

  /** Normalizes the axis */
  void create();

private:
  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;
};
