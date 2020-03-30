/**
 * @file Element.h
 *
 * This file declares a base class for all simulation elements.
 *
 * @author Colin Graf
 */

#pragma once

class Element
{
public:
  /** Constructor. */
  Element();

  /** Virtual destructor for polymorphism. */
  virtual ~Element() = default;

  /**
   * Registers another element as parent of this element.
   * @param element The element to register.
   */
  virtual void addParent(Element& element);
};

inline void Element::addParent(Element&) {}
