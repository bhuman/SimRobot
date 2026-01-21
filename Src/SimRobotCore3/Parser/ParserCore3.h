/**
 * @file ParserCore3.h
 * Declaration of class ParserCore3
 * @author Colin Graf
 */

#pragma once

#include "Parser/Parser.h"
#include <string>
#include <vector>

class Element;

/**
 * @class ParserCore3
 * A parser for .ros3 files.
 */
class ParserCore3 : public Parser
{
public:
  /** Constructor. */
  ParserCore3();

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
    surfaceClass        = (1 << 13),
    primitiveGroupClass = (1 << 14),
    verticesClass       = (1 << 15),
    normalsClass        = (1 << 16),
    texCoordsClass      = (1 << 17),
    intSensorClass      = (1 << 18),
    extSensorClass      = (1 << 19),
    materialClass       = (1 << 20),
    lightClass          = (1 << 21),
    userInputClass      = (1 << 22),
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
  Element* materialElement();
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
  Element* depthImageSensorElement();
  Element* userInputElement();

  std::vector<ElementInfo> elements;
};
