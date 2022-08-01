/**
 * @file ParserCore2D.h
 *
 * This file declares a class that parses .ros2d scene description files.
 *
 * @author Arne Hasselbring
 * @author Colin Graf (the parts which have been copied from SimRobotCore2)
 */

#pragma once

#include "Parser/Parser.h"
#include <string>
#include <vector>

class Element;
class QColor;

class ParserCore2D : public Parser
{
public:
  /** Constructor. */
  ParserCore2D();

private:
  enum ElementClass
  {
    setClass         = (1u << 0u),
    sceneClass       = (1u << 1u),
    bodyClass        = (1u << 2u),
    compoundClass    = (1u << 3u),
    translationClass = (1u << 4u),
    rotationClass    = (1u << 5u),
    massClass        = (1u << 6u),
    geometryClass    = (1u << 7u)
  };

  bool getColor(const char* key, bool required, QColor& color);

  Element* setElement();
  Element* sceneElement();
  Element* bodyElement();
  Element* compoundElement();
  Element* translationElement();
  Element* rotationElement();
  Element* massElement();
  Element* diskMassElement();
  Element* pointMassElement();
  Element* rectMassElement();
  Element* geometryElement();
  Element* chainGeometryElement();
  Element* convexGeometryElement();
  Element* diskGeometryElement();
  Element* edgeGeometryElement();
  Element* rectGeometryElement();
  void verticesText(std::string& text, Location location);

  std::vector<ElementInfo> elements;
};
