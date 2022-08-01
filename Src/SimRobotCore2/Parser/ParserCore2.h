/**
 * @file ParserCore2.h
 * Declaration of class ParserCore2
 * @author Colin Graf
 */

#pragma once

#include "Parser/Parser.h"
#include <string>
#include <vector>

class Element;

/**
 * @class ParserCore2
 * A parser for .ros2 files.
 */
class ParserCore2 : public Parser
{
public:
  /** Constructor. */
  ParserCore2();

private:
  enum ElementClass
  {
    sceneClass          = (1 << 0),
    setClass            = (1 << 1),
    compoundClass       = (1 << 2),
    bodyClass           = (1 << 3),
    translationClass    = (1 << 4),
    rotationClass       = (1 << 5),
    massClass           = (1 << 6),
    geometryClass       = (1 << 7),
    appearanceClass     = (1 << 8),
    jointClass          = (1 << 9),
    axisClass           = (1 << 10),
    motorClass          = (1 << 11),
    deflectionClass     = (1 << 12),
    solverClass         = (1 << 13),
    surfaceClass        = (1 << 14),
    primitiveGroupClass = (1 << 15),
    verticesClass       = (1 << 16),
    normalsClass        = (1 << 17),
    texCoordsClass      = (1 << 18),
    intSensorClass      = (1 << 19),
    extSensorClass      = (1 << 20),
    materialClass       = (1 << 21),
    frictionClass       = (1 << 22),
    lightClass          = (1 << 23),
    userInputClass      = (1 << 24),
  };

  bool getColor(const char* key, bool required, float* colors);

  Element* sceneElement();
  Element* setElement();
  Element* compoundElement();
  Element* bodyElement();
  Element* translationElement();
  Element* rotationElement();
  Element* massElement();
  Element* boxMassElement();
  Element* sphereMassElement();
  Element* inertiaMatrixMassElement();
  Element* capsuleMassElement();
  Element* cylinderMassElement();
  Element* geometryElement();
  Element* boxGeometryElement();
  Element* sphereGeometryElement();
  Element* cylinderGeometryElement();
  Element* capsuleGeometryElement();
  Element* torusGeometryElement();
  Element* materialElement();
  Element* frictionElement();
  Element* rollingFrictionElement();
  Element* appearanceElement();
  Element* boxAppearanceElement();
  Element* sphereAppearanceElement();
  Element* cylinderAppearanceElement();
  Element* capsuleAppearanceElement();
  Element* complexAppearanceElement();
  Element* trianglesElement();
  Element* quadsElement();
  void trianglesAndQuadsText(std::string& text, Location location);
  Element* verticesElement();
  void verticesText(std::string& text, Location location);
  Element* normalsElement();
  void normalsText(std::string& text, Location location);
  Element* texCoordsElement();
  void texCoordsText(std::string& text, Location location);
  Element* hingeElement();
  Element* sliderElement();
  Element* axisElement();
  Element* deflectionElement();
  Element* PT2MotorElement();
  Element* servoMotorElement();
  Element* velocityMotorElement();
  Element* quickSolverElement();
  Element* dirLightElement();
  Element* pointLightElement();
  Element* spotLightElement();
  Element* surfaceElement();
  Element* gyroscopeElement();
  Element* accelerometerElement();
  Element* cameraElement();
  Element* collisionSensorElement();
  Element* objectSegmentedImageSensorElement();
  Element* singleDistanceSensorElement();
  Element* approxDistanceSensorElement();
  Element* depthImageSensorElement();
  Element* userInputElement();

  std::vector<ElementInfo> elements;
};
