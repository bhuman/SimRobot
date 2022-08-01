/**
 * @file ParserCore2.cpp
 * Implementation of class ParserCore2
 * @author Colin Graf
 */

#include "ParserCore2.h"
#include "Graphics/Light.h"
#include "Platform/Assert.h"
#include "Simulation/Actuators/Hinge.h"
#include "Simulation/Actuators/Slider.h"
#include "Simulation/Appearances/BoxAppearance.h"
#include "Simulation/Appearances/CapsuleAppearance.h"
#include "Simulation/Appearances/ComplexAppearance.h"
#include "Simulation/Appearances/CylinderAppearance.h"
#include "Simulation/Appearances/SphereAppearance.h"
#include "Simulation/Axis.h"
#include "Simulation/Body.h"
#include "Simulation/Compound.h"
#include "Simulation/Geometries/BoxGeometry.h"
#include "Simulation/Geometries/CapsuleGeometry.h"
#include "Simulation/Geometries/CylinderGeometry.h"
#include "Simulation/Geometries/SphereGeometry.h"
#include "Simulation/Geometries/TorusGeometry.h"
#include "Simulation/Masses/BoxMass.h"
#include "Simulation/Masses/CapsuleMass.h"
#include "Simulation/Masses/CylinderMass.h"
#include "Simulation/Masses/InertiaMatrixMass.h"
#include "Simulation/Masses/SphereMass.h"
#include "Simulation/Motors/PT2Motor.h"
#include "Simulation/Motors/ServoMotor.h"
#include "Simulation/Motors/VelocityMotor.h"
#include "Simulation/Scene.h"
#include "Simulation/Sensors/Accelerometer.h"
#include "Simulation/Sensors/ApproxDistanceSensor.h"
#include "Simulation/Sensors/Camera.h"
#include "Simulation/Sensors/CollisionSensor.h"
#include "Simulation/Sensors/DepthImageSensor.h"
#include "Simulation/Sensors/Gyroscope.h"
#include "Simulation/Sensors/ObjectSegmentedImageSensor.h"
#include "Simulation/Sensors/SingleDistanceSensor.h"
#include "Simulation/Simulation.h"
#include "Simulation/UserInput.h"

ParserCore2::ParserCore2()
{
  using namespace std::placeholders;

  elements =
  {
    // { element, class, handler, text handler, flags
    //   required children, optional children, repeatable children, path attributes }
    {"Include", infrastructureClass, std::bind(&ParserCore2::includeElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"Simulation", infrastructureClass, std::bind(&ParserCore2::simulationElement, this), nullptr, 0,
      sceneClass, 0, 0, {}},

    {"Set", setClass, std::bind(&ParserCore2::setElement, this), nullptr, 0,
      0, 0, 0, {}},

    {"Scene", sceneClass, std::bind(&ParserCore2::sceneElement, this), nullptr, 0,
      0, solverClass, setClass | bodyClass | compoundClass | lightClass | userInputClass, {}},
    {"QuickSolver", solverClass, std::bind(&ParserCore2::quickSolverElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"DirLight", lightClass, std::bind(&ParserCore2::dirLightElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"PointLight", lightClass, std::bind(&ParserCore2::pointLightElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"SpotLight", lightClass, std::bind(&ParserCore2::spotLightElement, this), nullptr, 0,
      0, 0, 0, {}},

    {"Compound", compoundClass, std::bind(&ParserCore2::compoundElement, this), nullptr, 0,
      0, translationClass | rotationClass, setClass | jointClass | compoundClass | bodyClass | appearanceClass | geometryClass | extSensorClass | userInputClass, {}},
    {"Body", bodyClass, std::bind(&ParserCore2::bodyElement, this), nullptr, 0,
      massClass, translationClass | rotationClass, setClass | jointClass | appearanceClass | geometryClass | massClass | intSensorClass | extSensorClass | userInputClass, {}},

    {"Translation", translationClass, std::bind(&ParserCore2::translationElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"Rotation", rotationClass, std::bind(&ParserCore2::rotationElement, this), nullptr, 0,
      0, 0, 0, {}},

    {"Mass", massClass, std::bind(&ParserCore2::massElement, this), nullptr, constantFlag,
      0, translationClass | rotationClass, setClass | massClass, {}},
    {"BoxMass", massClass, std::bind(&ParserCore2::boxMassElement, this), nullptr, constantFlag,
      0, translationClass | rotationClass, setClass | massClass, {}},
    {"SphereMass", massClass, std::bind(&ParserCore2::sphereMassElement, this), nullptr, constantFlag,
      0, translationClass | rotationClass, setClass | massClass, {}},
    {"InertiaMatrixMass", massClass, std::bind(&ParserCore2::inertiaMatrixMassElement, this), nullptr, constantFlag,
      0, translationClass | rotationClass, setClass | massClass, {}},
    {"CapsuleMass", massClass, std::bind(&ParserCore2::capsuleMassElement, this), nullptr, constantFlag,
      0, translationClass | rotationClass, setClass | massClass, {}},
    {"CylinderMass", massClass, std::bind(&ParserCore2::cylinderMassElement, this), nullptr, constantFlag,
      0, translationClass | rotationClass, setClass | massClass, {}},

    {"Geometry", geometryClass, std::bind(&ParserCore2::geometryElement, this), nullptr, 0,
      0, translationClass | rotationClass | materialClass, setClass | geometryClass, {}},
    {"BoxGeometry", geometryClass, std::bind(&ParserCore2::boxGeometryElement, this), nullptr, 0,
      0, translationClass | rotationClass | materialClass, setClass | geometryClass, {}},
    {"SphereGeometry", geometryClass, std::bind(&ParserCore2::sphereGeometryElement, this), nullptr, 0,
      0, translationClass | rotationClass | materialClass, setClass | geometryClass, {}},
    {"CylinderGeometry", geometryClass, std::bind(&ParserCore2::cylinderGeometryElement, this), nullptr, 0,
      0, translationClass | rotationClass | materialClass, setClass | geometryClass, {}},
    {"CapsuleGeometry", geometryClass, std::bind(&ParserCore2::capsuleGeometryElement, this), nullptr, 0,
      0, translationClass | rotationClass | materialClass, setClass | geometryClass, {}},
    {"TorusGeometry", geometryClass, std::bind(&ParserCore2::torusGeometryElement, this), nullptr, 0,
      0, translationClass | rotationClass | materialClass, setClass | geometryClass, {}},

    {"Material", materialClass, std::bind(&ParserCore2::materialElement, this), nullptr, constantFlag,
      0, 0, setClass | frictionClass, {}},
    {"Friction", frictionClass, std::bind(&ParserCore2::frictionElement, this), nullptr, constantFlag,
      0, 0, 0, {}},
    {"RollingFriction", frictionClass, std::bind(&ParserCore2::rollingFrictionElement, this), nullptr, constantFlag,
      0, 0, 0, {}},

    {"Appearance", appearanceClass, std::bind(&ParserCore2::appearanceElement, this), nullptr, 0,
      0, translationClass | rotationClass, setClass | appearanceClass, {}},
    {"BoxAppearance", appearanceClass, std::bind(&ParserCore2::boxAppearanceElement, this), nullptr, 0,
      surfaceClass, translationClass | rotationClass, setClass | appearanceClass, {}},
    {"SphereAppearance", appearanceClass, std::bind(&ParserCore2::sphereAppearanceElement, this), nullptr, 0,
      surfaceClass, translationClass | rotationClass, setClass | appearanceClass, {}},
    {"CylinderAppearance", appearanceClass, std::bind(&ParserCore2::cylinderAppearanceElement, this), nullptr, 0,
      surfaceClass, translationClass | rotationClass, setClass | appearanceClass, {}},
    {"CapsuleAppearance", appearanceClass, std::bind(&ParserCore2::capsuleAppearanceElement, this), nullptr, 0,
      surfaceClass, translationClass | rotationClass, setClass | appearanceClass, {}},
    {"ComplexAppearance", appearanceClass, std::bind(&ParserCore2::complexAppearanceElement, this), nullptr, 0,
      surfaceClass | verticesClass | primitiveGroupClass, translationClass | rotationClass | normalsClass | texCoordsClass, setClass | primitiveGroupClass | appearanceClass, {}},

    {"Vertices", verticesClass, std::bind(&ParserCore2::verticesElement, this), std::bind(&ParserCore2::verticesText, this, _1, _2), textFlag | constantFlag,
      0, 0, 0, {}},
    {"Normals", normalsClass, std::bind(&ParserCore2::normalsElement, this), std::bind(&ParserCore2::normalsText, this, _1, _2), textFlag | constantFlag,
      0, 0, 0, {}},
    {"TexCoords", texCoordsClass, std::bind(&ParserCore2::texCoordsElement, this), std::bind(&ParserCore2::texCoordsText, this, _1, _2), textFlag | constantFlag,
      0, 0, 0, {}},
    {"Triangles", primitiveGroupClass, std::bind(&ParserCore2::trianglesElement, this), std::bind(&ParserCore2::trianglesAndQuadsText, this, _1, _2), textFlag | constantFlag,
      0, 0, 0, {}},
    {"Quads", primitiveGroupClass, std::bind(&ParserCore2::quadsElement, this), std::bind(&ParserCore2::trianglesAndQuadsText, this, _1, _2), textFlag | constantFlag,
      0, 0, 0, {}},

    {"Surface", surfaceClass, std::bind(&ParserCore2::surfaceElement, this), nullptr, constantFlag,
      0, 0, 0, {"diffuseTexture"}},

    {"Hinge", jointClass, std::bind(&ParserCore2::hingeElement, this), nullptr, 0,
      bodyClass | axisClass, translationClass | rotationClass, setClass, {}},
    {"Slider", jointClass, std::bind(&ParserCore2::sliderElement, this), nullptr, 0,
      bodyClass | axisClass, translationClass | rotationClass, setClass, {}},
    {"Axis", axisClass, std::bind(&ParserCore2::axisElement, this), nullptr, 0,
      0, motorClass | deflectionClass, setClass, {}},
    {"Deflection", deflectionClass, std::bind(&ParserCore2::deflectionElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"PT2Motor", motorClass, std::bind(&ParserCore2::PT2MotorElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"ServoMotor", motorClass, std::bind(&ParserCore2::servoMotorElement, this), nullptr, 0,
      0, 0, 0, {}},
    {"VelocityMotor", motorClass, std::bind(&ParserCore2::velocityMotorElement, this), nullptr, 0,
      0, 0, 0, {}},

    {"Gyroscope", intSensorClass, std::bind(&ParserCore2::gyroscopeElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},
    {"Accelerometer", intSensorClass, std::bind(&ParserCore2::accelerometerElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},
    {"Camera", extSensorClass, std::bind(&ParserCore2::cameraElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},
    {"CollisionSensor", intSensorClass, std::bind(&ParserCore2::collisionSensorElement, this), nullptr, 0,
      0, translationClass | rotationClass, geometryClass, {}},
    {"ObjectSegmentedImageSensor", extSensorClass, std::bind(&ParserCore2::objectSegmentedImageSensorElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},
    {"SingleDistanceSensor", extSensorClass, std::bind(&ParserCore2::singleDistanceSensorElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},
    {"ApproxDistanceSensor", extSensorClass, std::bind(&ParserCore2::approxDistanceSensorElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},
    {"DepthImageSensor", extSensorClass, std::bind(&ParserCore2::depthImageSensorElement, this), nullptr, 0,
      0, translationClass | rotationClass, 0, {}},

    {"UserInput", userInputClass, std::bind(&ParserCore2::userInputElement, this), nullptr, 0,
      0, 0, 0, {}},
  };

  for(const ElementInfo& element : elements)
    elementInfos[element.name] = &element;
}

bool ParserCore2::getColor(const char* key, bool required, float* colors)
{
  static const float f1_255 = 1.f / 255.f;
  unsigned char colorsUC[4];
  if(!Parser::getColor(key, required, colorsUC))
    return false;
  for(std::size_t i = 0; i < 4; ++i)
    colors[i] = static_cast<float>(colorsUC[i]) * f1_255;
  return true;
}

Element* ParserCore2::setElement()
{
  ASSERT(element);
  const std::string& name = getString("name", true);
  const std::string& value = getString("value", true);
  auto& vars = elementData->parent->vars;
  if(vars.find(name) == vars.end())
    vars[name] = value;
  return nullptr;
}

Element* ParserCore2::sceneElement()
{
  Scene* scene = new Scene();
  scene->name = getString("name", false);
  scene->controller = getString("controller", false);
  getColor("color", false, scene->color);
  scene->stepLength = getTimeNonZeroPositive("stepLength", false, 0.01f);
  scene->gravity = getAcceleration("gravity", false, -9.80665f);
  scene->cfm = getFloatMinMax("CFM", false, -1.f, 0.f, 1.f);
  scene->erp = getFloatMinMax("ERP", false, -1.f, 0.f, 1.f);
  scene->contactSoftERP = getFloatMinMax("contactSoftERP", false, -1.f, 0.f, 1.f);
  if(scene->contactSoftERP != -1.f)
    scene->contactMode |= dContactSoftERP;
  scene->contactSoftCFM = getFloatMinMax("contactSoftCFM", false, -1.f, 0.f, 1.f);
  if(scene->contactSoftCFM != -1.f)
    scene->contactMode |= dContactSoftCFM;
  scene->detectBodyCollisions = getBool("bodyCollisions", false, true);

  ASSERT(!Simulation::simulation->scene);
  Simulation::simulation->scene = scene;
  return scene;
}

Element* ParserCore2::quickSolverElement()
{
  Scene* scene = dynamic_cast<Scene*>(element);
  ASSERT(scene);
  scene->useQuickSolver = true;
  scene->quickSolverIterations = getInteger("iterations", false, -1, true);
  scene->quickSolverSkip = getInteger("skip", false, 1, true);
  return nullptr;
}

Element* ParserCore2::dirLightElement()
{
  DirLight* light = new DirLight();
  getColor("diffuseColor", false, light->diffuseColor);
  getColor("ambientColor", false, light->ambientColor);
  getColor("specularColor", false, light->specularColor);
  light->direction[0] = getFloatMinMax("x", false, light->direction[0], -1.f, 1.f);
  light->direction[1] = getFloatMinMax("y", false, light->direction[1], -1.f, 1.f);
  light->direction[2] = getFloatMinMax("z", false, light->direction[2], -1.f, 1.f);
  return light;
}

Element* ParserCore2::pointLightElement()
{
  PointLight* light = new PointLight();
  getColor("diffuseColor", false, light->diffuseColor);
  getColor("ambientColor", false, light->ambientColor);
  getColor("specularColor", false, light->specularColor);
  light->position[0] = getLength("x", false, light->position[0], false);
  light->position[1] = getLength("y", false, light->position[1], false);
  light->position[2] = getLength("z", false, light->position[2], false);
  light->constantAttenuation = getFloatPositive("constantAttenuation", false, light->constantAttenuation);
  light->linearAttenuation = getFloatPositive("linearAttenuation", false, light->linearAttenuation);
  light->quadraticAttenuation = getFloatPositive("quadraticAttenuation", false, light->quadraticAttenuation);
  return light;
}

Element* ParserCore2::spotLightElement()
{
  SpotLight* light = new SpotLight();
  getColor("diffuseColor", false, light->diffuseColor);
  getColor("ambientColor", false, light->ambientColor);
  getColor("specularColor", false, light->specularColor);
  light->position[0] = getLength("x", false, light->position[0], false);
  light->position[1] = getLength("y", false, light->position[1], false);
  light->position[2] = getLength("z", false, light->position[2], false);
  light->constantAttenuation = getFloatPositive("constantAttenuation", false, light->constantAttenuation);
  light->linearAttenuation = getFloatPositive("linearAttenuation", false, light->linearAttenuation);
  light->quadraticAttenuation = getFloatPositive("quadraticAttenuation", false, light->quadraticAttenuation);
  light->direction[0] = getFloatMinMax("dirX", false, light->direction[0], -1.f, 1.f);
  light->direction[1] = getFloatMinMax("dirY", false, light->direction[1], -1.f, 1.f);
  light->direction[2] = getFloatMinMax("dirZ", false, light->direction[2], -1.f, 1.f);
  light->cutoff = getFloatMinMax("cutoff", false, light->cutoff, 0.f, 1.f);
  return light;
}

Element* ParserCore2::bodyElement()
{
  Body* body = new Body();
  body->name = getString("name", false);
  return body;
}

Element* ParserCore2::compoundElement()
{
  Compound* compound = new Compound();
  compound->name = getString("name", false);
  return compound;
}

Element* ParserCore2::hingeElement()
{
  Hinge* hinge = new Hinge();
  hinge->name = getString("name", false);
  return hinge;
}

Element* ParserCore2::sliderElement()
{
  Slider* slider = new Slider();
  slider->name = getString("name", false);
  return slider;
}

Element* ParserCore2::massElement()
{
  Mass* mass = new Mass();
  mass->name = getString("name", false);
  return mass;
}

Element* ParserCore2::boxMassElement()
{
  BoxMass* boxMass = new BoxMass();
  boxMass->name = getString("name", false);
  boxMass->value = getMass("value", true, 0.f);
  boxMass->width = getLength("width", true, 0.f, true);
  boxMass->height = getLength("height", true, 0.f, true);
  boxMass->depth = getLength("depth", true, 0.f, true);
  return boxMass;
}

Element* ParserCore2::sphereMassElement()
{
  SphereMass* sphereMass = new SphereMass();
  sphereMass->name = getString("name", false);
  sphereMass->value = getMass("value", true, 0.f);
  sphereMass->radius = getLength("radius", true, 0.f, true);
  return sphereMass;
}

Element* ParserCore2::inertiaMatrixMassElement()
{
  InertiaMatrixMass* inertiaMatrixMass = new InertiaMatrixMass();
  inertiaMatrixMass->name = getString("name", false);
  inertiaMatrixMass->value = getMass("value", true, 0.f);
  inertiaMatrixMass->x = getLength("x", false, 0.f, false);
  inertiaMatrixMass->y = getLength("y", false, 0.f, false);
  inertiaMatrixMass->z = getLength("z", false, 0.f, false);
  inertiaMatrixMass->ixx = getMassLengthLength("ixx", true, 0.f);
  inertiaMatrixMass->ixy = getMassLengthLength("ixy", false, 0.f);
  inertiaMatrixMass->ixz = getMassLengthLength("ixz", false, 0.f);
  inertiaMatrixMass->iyy = getMassLengthLength("iyy", true, 0.f);
  inertiaMatrixMass->iyz = getMassLengthLength("iyz", false, 0.f);
  inertiaMatrixMass->izz = getMassLengthLength("izz", true, 0.f);
  return inertiaMatrixMass;
}

Element* ParserCore2::capsuleMassElement()
{
  CapsuleMass* capsuleMass = new CapsuleMass();
  capsuleMass->name = getString("name", false);
  capsuleMass->value = getMass("value", true, 0.f);
  capsuleMass->radius = getLength("radius", true, 0.f, true);
  capsuleMass->height = getLength("height", true, 0.f, true);
  return capsuleMass;
}

Element* ParserCore2::cylinderMassElement()
{
  CylinderMass* cylinderMass = new CylinderMass();
  cylinderMass->name = getString("name", false);
  cylinderMass->value = getMass("value", true, 0.f);
  cylinderMass->radius = getLength("radius", true, 0.f, true);
  cylinderMass->height = getLength("height", true, 0.f, true);
  return cylinderMass;
}

Element* ParserCore2::geometryElement()
{
  Geometry* geometry = new Geometry();
  geometry->name = getString("name", false);
  return geometry;
}

Element* ParserCore2::boxGeometryElement()
{
  BoxGeometry* boxGeometry = new BoxGeometry();
  getColor("color", false, boxGeometry->color);
  boxGeometry->name = getString("name", false);
  boxGeometry->width = getLength("width", true, 0.f, true);
  boxGeometry->height = getLength("height", true, 0.f, true);
  boxGeometry->depth = getLength("depth", true, 0.f, true);
  return boxGeometry;
}

Element* ParserCore2::sphereGeometryElement()
{
  SphereGeometry* sphereGeometry = new SphereGeometry();
  getColor("color", false, sphereGeometry->color);
  sphereGeometry->name = getString("name", false);
  sphereGeometry->radius = getLength("radius", true, 0.f, true);
  return sphereGeometry;
}

Element* ParserCore2::cylinderGeometryElement()
{
  CylinderGeometry* cylinderGeometry = new CylinderGeometry();
  getColor("color", false, cylinderGeometry->color);
  cylinderGeometry->name = getString("name", false);
  cylinderGeometry->radius = getLength("radius", true, 0.f, true);
  cylinderGeometry->height = getLength("height", true, 0.f, true);
  return cylinderGeometry;
}

Element* ParserCore2::capsuleGeometryElement()
{
  CapsuleGeometry* capsuleGeometry = new CapsuleGeometry();
  getColor("color", false, capsuleGeometry->color);
  capsuleGeometry->name = getString("name", false);
  capsuleGeometry->radius = getLength("radius", true, 0.f, true);
  capsuleGeometry->height = getLength("height", true, 0.f, true);
  return capsuleGeometry;
}

Element* ParserCore2::torusGeometryElement()
{
  TorusGeometry* torusGeometry = new TorusGeometry();
  getColor("color", false, torusGeometry->color);
  torusGeometry->name = getString("name", false);
  torusGeometry->majorRadius = getLength("majorRadius", true, 0.f, true);
  torusGeometry->minorRadius = getLength("minorRadius", true, 0.f, true);
  return torusGeometry;
}

Element* ParserCore2::materialElement()
{
  Geometry::Material* material = new Geometry::Material();
  material->name = getString("name", false);
  return material;
}

Element* ParserCore2::frictionElement()
{
  Geometry::Material* material = dynamic_cast<Geometry::Material*>(element);
  ASSERT(material);
  const std::string& otherMaterial = getString("material", true);
  float friction = getFloatPositive("value", true, 1.f);
  material->frictions[otherMaterial] = friction;
  return nullptr;
}

Element* ParserCore2::rollingFrictionElement()
{
  Geometry::Material* material = dynamic_cast<Geometry::Material*>(element);
  ASSERT(material);
  const std::string& otherMaterial = getString("material", true);
  float rollingFriction = getFloatPositive("value", true, 1.f);
  material->rollingFrictions[otherMaterial] = rollingFriction;
  return nullptr;
}

Element* ParserCore2::appearanceElement()
{
  Appearance* appearance = new Appearance();
  appearance->name = getString("name", false);
  return appearance;
}

Element* ParserCore2::boxAppearanceElement()
{
  BoxAppearance* boxAppearance = new BoxAppearance();
  boxAppearance->name = getString("name", false);
  boxAppearance->width = getLength("width", true, 0.f, true);
  boxAppearance->height = getLength("height", true, 0.f, true);
  boxAppearance->depth = getLength("depth", true, 0.f, true);
  return boxAppearance;
}

Element* ParserCore2::sphereAppearanceElement()
{
  SphereAppearance* sphereAppearance = new SphereAppearance();
  sphereAppearance->name = getString("name", false);
  sphereAppearance->radius = getLength("radius", true, 0.f, true);
  return sphereAppearance;
}

Element* ParserCore2::cylinderAppearanceElement()
{
  CylinderAppearance* cylinderAppearance = new CylinderAppearance();
  cylinderAppearance->name = getString("name", false);
  cylinderAppearance->height = getLength("height", true, 0.f, true);
  cylinderAppearance->radius = getLength("radius", true, 0.f, true);
  return cylinderAppearance;
}

Element* ParserCore2::capsuleAppearanceElement()
{
  CapsuleAppearance* capsuleAppearance = new CapsuleAppearance();
  capsuleAppearance->name = getString("name", false);
  capsuleAppearance->height = getLength("height", true, 0.f, true);
  capsuleAppearance->radius = getLength("radius", true, 0.f, true);
  return capsuleAppearance;
}

Element* ParserCore2::complexAppearanceElement()
{
  ComplexAppearance* complexAppearance = new ComplexAppearance();
  complexAppearance->name = getString("name", false);
  return complexAppearance;
}

Element* ParserCore2::trianglesElement()
{
  return new ComplexAppearance::PrimitiveGroup(ComplexAppearance::triangles);
}

void ParserCore2::trianglesAndQuadsText(std::string& text, Location location)
{
  ComplexAppearance::PrimitiveGroup* primitiveGroup = dynamic_cast<ComplexAppearance::PrimitiveGroup*>(element);
  ASSERT(primitiveGroup);
  std::list<unsigned int>& vs = primitiveGroup->vertices;
  const char* str = text.c_str();
  char* nextStr;
  unsigned int l;
  skipWhitespace(str, location);
  while(*str)
  {
    while(*str == '#') { while(*str && *str != '\n' && *str != '\r') { ++str; ++location.column; } skipWhitespace(str, location); if(!*str) return; }
    l = static_cast<unsigned int>(std::strtol(str, &nextStr, 10));
    if(str == nextStr)
    {
      handleError("Invalid index text (must be a space separated list of integers)", location);
      return;
    }
    location.column += nextStr - str;
    str = nextStr;
    skipWhitespace(str, location);
    vs.push_back(l);
  }
}

Element* ParserCore2::quadsElement()
{
  return new ComplexAppearance::PrimitiveGroup(ComplexAppearance::quads);
}

Element* ParserCore2::verticesElement()
{
  ComplexAppearance::Vertices* vertices = new ComplexAppearance::Vertices();
  vertices->unit = getUnit("unit", false, 1);
  return vertices;
}

void ParserCore2::verticesText(std::string& text, Location location)
{
  ComplexAppearance::Vertices* vertices = dynamic_cast<ComplexAppearance::Vertices*>(element);
  ASSERT(vertices);
  std::vector<Vector3f>& vs = vertices->vertices;
  const char* str = text.c_str();
  char* nextStr;
  float components[3];
  skipWhitespace(str, location);
  while(*str)
  {
    for(int i = 0; i < 3; ++i)
    {
      while(*str == '#') { while(*str && *str != '\n' && *str != '\r') { ++str; ++location.column; } skipWhitespace(str, location); if(!*str) return; }
      components[i] = std::strtof(str, &nextStr);
      if(str == nextStr)
      {
        handleError("Invalid vertex text (must be a space separated list of floats)", location);
        return;
      }
      location.column += nextStr - str;
      str = nextStr;
      skipWhitespace(str, location);
    }
    components[0] *= vertices->unit;
    components[1] *= vertices->unit;
    components[2] *= vertices->unit;
    vs.emplace_back(components[0], components[1], components[2]);
  }
}

Element* ParserCore2::normalsElement()
{
  return new ComplexAppearance::Normals();
}

void ParserCore2::normalsText(std::string& text, Location location)
{
  ComplexAppearance::Normals* normals = dynamic_cast<ComplexAppearance::Normals*>(element);
  ASSERT(normals);
  std::vector<Vector3f>& ns = normals->normals;
  const char* str = text.c_str();
  char* nextStr;
  float components[3];
  skipWhitespace(str, location);
  while(*str)
  {
    for(int i = 0; i < 3; ++i)
    {
      while(*str == '#') { while(*str && *str != '\n' && *str != '\r') { ++str; ++location.column; } skipWhitespace(str, location); if(!*str) return; }
      components[i] = std::strtof(str, &nextStr);
      if(str == nextStr)
      {
        handleError("Invalid normal text (must be a space separated list of floats)", location);
        return;
      }
      location.column += nextStr - str;
      str = nextStr;
      skipWhitespace(str, location);
    }
    ns.emplace_back(components[0], components[1], components[2]);
  }
}

Element* ParserCore2::texCoordsElement()
{
  return new ComplexAppearance::TexCoords();
}

void ParserCore2::texCoordsText(std::string& text, Location location)
{
  ComplexAppearance::TexCoords* texCoords = dynamic_cast<ComplexAppearance::TexCoords*>(element);
  ASSERT(texCoords);
  std::vector<Vector2f>& ts = texCoords->coords;
  const char* str = text.c_str();
  char* nextStr;
  float components[2];
  skipWhitespace(str, location);
  while(*str)
  {
    for(int i = 0; i < 2; ++i)
    {
      while(*str == '#') { while(*str && *str != '\n' && *str != '\r') { ++str; ++location.column; } skipWhitespace(str, location); if(!*str) return; }
      components[i] = std::strtof(str, &nextStr);
      if(str == nextStr)
      {
        handleError("Invalid texture coordinate text (must be a space separated list of floats)", location);
        return;
      }
      location.column += nextStr - str;
      str = nextStr;
      skipWhitespace(str, location);
    }
    ts.emplace_back(components[0], components[1]);
  }
}

Element* ParserCore2::translationElement()
{
  Vector3f* translation = new Vector3f(getLength("x", false, 0.f, false), getLength("y", false, 0.f, false), getLength("z", false, 0.f, false));

  SimObject* simObject = dynamic_cast<SimObject*>(element);
  if(simObject)
  {
    ASSERT(!simObject->translation);
    simObject->translation = translation;
  }
  else
  {
    Mass* mass = dynamic_cast<Mass*>(element);
    ASSERT(mass);
    ASSERT(!mass->translation);
    mass->translation = translation;
  }
  return nullptr;
}

Element* ParserCore2::rotationElement()
{
  RotationMatrix* rotation = new RotationMatrix;
  *rotation *= RotationMatrix::aroundZ(getAngle("z", false, 0.f, false));
  *rotation *= RotationMatrix::aroundY(getAngle("y", false, 0.f, false));
  *rotation *= RotationMatrix::aroundX(getAngle("x", false, 0.f, false));

  SimObject* simObject = dynamic_cast<SimObject*>(element);
  if(simObject)
  {
    ASSERT(!simObject->rotation);
    simObject->rotation = rotation;
  }
  else
  {
    Mass* mass = dynamic_cast<Mass*>(element);
    ASSERT(mass);
    ASSERT(!mass->rotation);
    mass->rotation = rotation;
  }
  return nullptr;
}

Element* ParserCore2::axisElement()
{
  Axis* axis = new Axis();
  axis->x = getFloat("x", false, 0.f);
  axis->y = getFloat("y", false, 0.f);
  axis->z = getFloat("z", false, 0.f);
  axis->cfm = getFloatMinMax("cfm", false, -1.f, 0.f, 1.f);
  Joint* joint = dynamic_cast<Joint*>(element);
  ASSERT(joint);
  axis->joint = joint;
  return axis;
}

Element* ParserCore2::deflectionElement()
{
  Axis::Deflection* deflection = new Axis::Deflection();
  Axis* axis = dynamic_cast<Axis*>(element);
  ASSERT(axis);
  ASSERT(axis->joint);

  if(dynamic_cast<Hinge*>(axis->joint))
  {
    deflection->min = getAngle("min", true, 0.f, false);
    deflection->max = getAngle("max", true, 0.f, false);
    deflection->offset = getAngle("init", false, 0.f, false);
  }
  else if(dynamic_cast<Slider*>(axis->joint))
  {
    deflection->min = getLength("min", true, 0.f, false);
    deflection->max = getLength("max", true, 0.f, false);
  }
  else
    ASSERT(false);

  deflection->stopCFM = getFloatMinMax("stopCFM", false, -1.f, 0.f, 1.f);
  deflection->stopERP = getFloatMinMax("stopERP", false, -1.f, 0.f, 1.f);

  ASSERT(!axis->deflection);
  axis->deflection = deflection;
  return nullptr;
}

Element* ParserCore2::PT2MotorElement()
{
  PT2Motor* pt2motor = new PT2Motor();
  Axis* axis = dynamic_cast<Axis*>(element);
  ASSERT(axis);
  ASSERT(!axis->motor);

  pt2motor->T = getFloat("T", true, 0.f);
  pt2motor->D = getFloat("D", true, 0.f);
  pt2motor->K = getFloat("K", true, 0.f);
  pt2motor->V = getFloat("V", true, 0.f);
  pt2motor->F = getForce("F", true, 0.f);

  axis->motor = pt2motor;
  return nullptr;
}

Element* ParserCore2::servoMotorElement()
{
  ServoMotor* servoMotor = new ServoMotor();
  Axis* axis = dynamic_cast<Axis*>(element);
  ASSERT(axis);
  ASSERT(!axis->motor);

  if(dynamic_cast<Hinge*>(axis->joint))
    servoMotor->maxVelocity = getAngularVelocity("maxVelocity", true, 0.f);
  else if(dynamic_cast<Slider*>(axis->joint))
    servoMotor->maxVelocity = getVelocity("maxVelocity", true, 0.f);
  else
    ASSERT(false);

  servoMotor->maxForce = getForce("maxForce", true, 0.f);
  servoMotor->controller.p = getFloat("p", true, 0.f);
  servoMotor->controller.i = getFloat("i", false, 0.f);
  servoMotor->controller.d = getFloat("d", false, 0.f);

  axis->motor = servoMotor;
  return nullptr;
}

Element* ParserCore2::velocityMotorElement()
{
  VelocityMotor* velocityMotor = new VelocityMotor();
  Axis* axis = dynamic_cast<Axis*>(element);
  ASSERT(axis);
  ASSERT(!axis->motor);

  if(dynamic_cast<Hinge*>(axis->joint))
    velocityMotor->maxVelocity = getAngularVelocity("maxVelocity", true, 0.f);
  else if(dynamic_cast<Slider*>(axis->joint))
    velocityMotor->maxVelocity = getVelocity("maxVelocity", true, 0.f);
  else
    ASSERT(false);

  velocityMotor->maxForce = getForce("maxForce", true, 0.f);

  axis->motor = velocityMotor;
  return nullptr;
}

Element* ParserCore2::surfaceElement()
{
  Appearance::Surface* surface = new Appearance::Surface();
  getColor("diffuseColor", true, surface->diffuseColor);
  surface->hasAmbientColor = getColor("ambientColor", false, surface->ambientColor);
  getColor("specularColor", false, surface->specularColor);
  getColor("emissionColor", false, surface->emissionColor);
  surface->shininess = getFloatMinMax("shininess", false, surface->shininess, 0.f, 128.f);
  surface->diffuseTexture = getString("diffuseTexture", false);
  return surface;
}

Element* ParserCore2::gyroscopeElement()
{
  Gyroscope* gyroscope = new Gyroscope();
  gyroscope->name = getString("name", false);
  return gyroscope;
}

Element* ParserCore2::accelerometerElement()
{
  Accelerometer* accelerometer = new Accelerometer();
  accelerometer->name = getString("name", false);
  return accelerometer;
}

Element* ParserCore2::cameraElement()
{
  Camera* camera = new Camera();
  camera->name = getString("name", false);
  camera->imageWidth = getInteger("imageWidth", true, 0, true);
  camera->imageHeight = getInteger("imageHeight", true, 0, true);
  camera->angleX = getAngle("angleX", true, 0.f, true);
  camera->angleY = getAngle("angleY", true, 0.f, true);
  return camera;
}

Element* ParserCore2::collisionSensorElement()
{
  CollisionSensor* collisionSensor = new CollisionSensor();
  collisionSensor->name = getString("name", false);
  return collisionSensor;
}

Element* ParserCore2::objectSegmentedImageSensorElement()
{
  ObjectSegmentedImageSensor* camera = new ObjectSegmentedImageSensor();
  camera->name = getString("name", false);
  camera->imageWidth = getInteger("imageWidth", true, 0, true);
  camera->imageHeight = getInteger("imageHeight", true, 0, true);
  camera->angleX = getAngle("angleX", true, 0.f, true);
  camera->angleY = getAngle("angleY", true, 0.f, true);
  return camera;
}

Element* ParserCore2::singleDistanceSensorElement()
{
  SingleDistanceSensor* singleDistanceSensor = new SingleDistanceSensor();
  singleDistanceSensor->name = getString("name", false);
  singleDistanceSensor->min = getLength("min", false, 0.f, false);
  singleDistanceSensor->max = getLength("max", false, 999999.f, false);
  return singleDistanceSensor;
}

Element* ParserCore2::approxDistanceSensorElement()
{
  ApproxDistanceSensor* approxDistanceSensor = new ApproxDistanceSensor();
  approxDistanceSensor->name = getString("name", false);
  approxDistanceSensor->min = getLength("min", false, 0.f, false);
  approxDistanceSensor->max = getLength("max", false, 999999.f, false);
  approxDistanceSensor->angleX = getAngle("angleX", true, 0.f, true);
  approxDistanceSensor->angleY = getAngle("angleY", true, 0.f, true);
  return approxDistanceSensor;
}

Element* ParserCore2::depthImageSensorElement()
{
  DepthImageSensor* depthImageSensor = new DepthImageSensor();
  depthImageSensor->name = getString("name", false);
  depthImageSensor->imageWidth = getInteger("imageWidth", true, 0, true);
  depthImageSensor->imageHeight = getInteger("imageHeight", false, 1, true);
  depthImageSensor->angleX = getAngle("angleX", true, 0.f, true);
  depthImageSensor->angleY = getAngle("angleY", true, 0.f, true);
  depthImageSensor->min = getLength("min", false, 0.f, false);
  depthImageSensor->max = getLength("max", false, 999999.f, false);

  const std::string& projection = getString("projection", false);
  if(projection == "" || projection == "perspective")
    depthImageSensor->projection = DepthImageSensor::perspectiveProjection;
  else if(projection == "spherical")
  {
    if(depthImageSensor->imageHeight > 1)
      handleError("Spherical projection is currently only supported for 1-D sensors (i.e. with imageHeight=\"1\")",
                  attributes->find("projection")->second.valueLocation);
    else
      depthImageSensor->projection = DepthImageSensor::sphericalProjection;
  }
  else
    handleError("Unexpected projection type \"" + projection + "\" (expected one of \"perspective, spherical\")",
                attributes->find("projection")->second.valueLocation);

  return depthImageSensor;
}

Element* ParserCore2::userInputElement()
{
  UserInput* userInput = new UserInput();

  userInput->name = getString("name", false);
  std::string type = getString("type", false);
  if(type == "angle")
  {
    userInput->inputPort.unit = QString::fromUtf8("°");
    userInput->inputPort.min = getAngle("min", true, 0.f, false);
    userInput->inputPort.max = getAngle("max", true, 0.f, false);
    userInput->inputPort.defaultValue = getAngle("default", false, 0.f, false);
  }
  else if(type == "angularVelocity")
  {
    userInput->inputPort.unit = QString::fromUtf8("°/s");
    userInput->inputPort.min = getAngularVelocity("min", true, 0.f);
    userInput->inputPort.max = getAngularVelocity("max", true, 0.f);
    userInput->inputPort.defaultValue = getAngularVelocity("default", false, 0.f);
  }
  else if(type == "length" || type == "")
  {
    userInput->inputPort.unit = QString::fromUtf8("m");
    userInput->inputPort.min = getLength("min", true, 0.f, false);
    userInput->inputPort.max = getLength("max", true, 0.f, false);
    userInput->inputPort.defaultValue = getLength("default", false, 0.f, false);
  }
  else if(type == "velocity")
  {
    userInput->inputPort.unit = QString::fromUtf8("m/s");
    userInput->inputPort.min = getVelocity("min", true, 0.f);
    userInput->inputPort.max = getVelocity("max", true, 0.f);
    userInput->inputPort.defaultValue = getVelocity("default", false, 0.f);
  }
  else if(type == "acceleration")
  {
    userInput->inputPort.unit = QString::fromUtf8("m/s^2");
    userInput->inputPort.min = getAcceleration("min", true, 0.f);
    userInput->inputPort.max = getAcceleration("max", true, 0.f);
    userInput->inputPort.defaultValue = getAcceleration("default", false, 0.f);
  }
  else
    handleError("Unexpected user input type \"" + type + "\" (expected one of \"length, velocity, acceleration, angle, angularVelocity\")",
                attributes->find("type")->second.valueLocation);

  return userInput;
}
