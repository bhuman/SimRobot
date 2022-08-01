/**
 * @file ParserCore2D.cpp
 *
 * This file implements a class that parses .ros2d scene description files.
 *
 * @author Arne Hasselbring
 * @author Colin Graf (the parts which have been copied from SimRobotCore2)
 */

#include "ParserCore2D.h"
#include "Parser/Element.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Simulation/Compound.h"
#include "Simulation/Geometries/ChainGeometry.h"
#include "Simulation/Geometries/ConvexGeometry.h"
#include "Simulation/Geometries/DiskGeometry.h"
#include "Simulation/Geometries/EdgeGeometry.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Geometries/RectGeometry.h"
#include "Simulation/Masses/DiskMass.h"
#include "Simulation/Masses/Mass.h"
#include "Simulation/Masses/PointMass.h"
#include "Simulation/Masses/RectMass.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/Math/Constants.h"
#include <box2d/b2_math.h>
#include <QColor>
#include <cctype>
#include <cstring>

ParserCore2D::ParserCore2D()
{
  using namespace std::placeholders;

  elements =
  {
    {"Simulation", infrastructureClass, std::bind(&ParserCore2D::simulationElement, this), nullptr, 0, sceneClass, 0, 0, {}},
    {"Include", infrastructureClass, std::bind(&ParserCore2D::includeElement, this), nullptr, 0, 0, 0, 0, {}},

    {"Set", setClass, std::bind(&ParserCore2D::setElement, this), nullptr, 0, 0, 0, 0, {}},

    {"Scene", sceneClass, std::bind(&ParserCore2D::sceneElement, this), nullptr, 0, 0, 0, setClass | bodyClass | compoundClass, {"background"}},

    {"Body", bodyClass, std::bind(&ParserCore2D::bodyElement, this), nullptr, 0, massClass, translationClass | rotationClass, setClass | massClass | geometryClass, {}},

    {"Compound", compoundClass, std::bind(&ParserCore2D::compoundElement, this), nullptr, 0, 0, translationClass | rotationClass, setClass | bodyClass | compoundClass | geometryClass, {}},

    {"Translation", translationClass, std::bind(&ParserCore2D::translationElement, this), nullptr, 0, 0, 0, 0, {}},

    {"Rotation", rotationClass, std::bind(&ParserCore2D::rotationElement, this), nullptr, 0, 0, 0, 0, {}},

    {"Mass", massClass, std::bind(&ParserCore2D::massElement, this), nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass, {}},
    {"DiskMass", massClass, std::bind(&ParserCore2D::diskMassElement, this), nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass, {}},
    {"PointMass", massClass, std::bind(&ParserCore2D::pointMassElement, this), nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass, {}},
    {"RectMass", massClass, std::bind(&ParserCore2D::rectMassElement, this), nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass, {}},

    {"Geometry", geometryClass, std::bind(&ParserCore2D::geometryElement, this), nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass, {}},
    {"ChainGeometry", geometryClass, std::bind(&ParserCore2D::chainGeometryElement, this), std::bind(&ParserCore2D::verticesText, this, _1, _2), textFlag, 0, translationClass | rotationClass, setClass | geometryClass, {}},
    {"ConvexGeometry", geometryClass, std::bind(&ParserCore2D::convexGeometryElement, this), std::bind(&ParserCore2D::verticesText, this, _1, _2), textFlag, 0, translationClass | rotationClass, setClass | geometryClass, {}},
    {"DiskGeometry", geometryClass, std::bind(&ParserCore2D::diskGeometryElement, this), nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass, {}},
    {"EdgeGeometry", geometryClass, std::bind(&ParserCore2D::edgeGeometryElement, this), nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass, {}},
    {"RectGeometry", geometryClass, std::bind(&ParserCore2D::rectGeometryElement, this), nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass, {}}
  };

  for(const ElementInfo& element : elements)
    elementInfos[element.name] = &element;
}

bool ParserCore2D::getColor(const char* key, bool required, QColor& color)
{
  unsigned char colorUC[4];
  if(!Parser::getColor(key, required, colorUC))
    return false;
  color.setRed(colorUC[0]);
  color.setGreen(colorUC[1]);
  color.setBlue(colorUC[2]);
  color.setAlpha(colorUC[3]);
  return true;
}

Element* ParserCore2D::setElement()
{
  ASSERT(element);
  const std::string& name = getString("name", true);
  const std::string& value = getString("value", true);
  auto& vars = elementData->parent->vars;
  if(vars.find(name) == vars.end())
    vars[name] = value;
  return nullptr;
}

Element* ParserCore2D::sceneElement()
{
  auto* const scene = new Scene;
  scene->name = getString("name", false);
  scene->controller = getString("controller", false);
  scene->stepLength = getTimeNonZeroPositive("stepLength", false, 0.01f);
  scene->velocityIterations = getInteger("velocityIterations", false, 8, true);
  scene->positionIterations = getInteger("positionIterations", false, 3, true);
  scene->background = getString("background", false);

  ASSERT(!Simulation::simulation->scene);
  Simulation::simulation->scene = scene;
  return scene;
}

Element* ParserCore2D::bodyElement()
{
  auto* const body = new Body;
  body->name = getString("name", false);
  return body;
}

Element* ParserCore2D::compoundElement()
{
  auto* const compound = new Compound;
  compound->name = getString("name", false);
  return compound;
}

Element* ParserCore2D::translationElement()
{
  auto* const translation = new b2Vec2(getLength("x", false, 0.f, false), getLength("y", false, 0.f, false));

  auto* const simObject = dynamic_cast<SimObject*>(element);
  ASSERT(simObject);
  ASSERT(!simObject->translation);
  simObject->translation = translation;

  return nullptr;
}

Element* ParserCore2D::rotationElement()
{
  auto* const rotation = new b2Rot(getAngle("angle", false, 0.f, false));

  auto* const simObject = dynamic_cast<SimObject*>(element);
  ASSERT(simObject);
  ASSERT(!simObject->rotation);
  simObject->rotation = rotation;

  return nullptr;
}

Element* ParserCore2D::massElement()
{
  auto* const mass = new Mass;
  mass->name = getString("name", false);
  return mass;
}

Element* ParserCore2D::diskMassElement()
{
  auto* const diskMass = new DiskMass;
  diskMass->name = getString("name", false);
  diskMass->value = getMass("value", true, 0.f);
  diskMass->radius = getLength("radius", true, 0.f, true);
  return diskMass;
}

Element* ParserCore2D::pointMassElement()
{
  auto* const pointMass = new PointMass;
  pointMass->name = getString("name", false);
  pointMass->value = getMass("value", true, 0.f);
  return pointMass;
}

Element* ParserCore2D::rectMassElement()
{
  auto* const rectMass = new RectMass;
  rectMass->name = getString("name", false);
  rectMass->value = getMass("value", true, 0.f);
  rectMass->width = getLength("width", true, 0.f, true);
  rectMass->height = getLength("height", true, 0.f, true);
  return rectMass;
}

Element* ParserCore2D::geometryElement()
{
  auto* const geometry = new Geometry;
  geometry->name = getString("name", false);
  geometry->category = getUInt16("category", false, 0);
  geometry->mask = getUInt16("mask", false, 0xffff);
  return geometry;
}

Element* ParserCore2D::chainGeometryElement()
{
  auto* const chainGeometry = new ChainGeometry;
  chainGeometry->name = getString("name", false);
  chainGeometry->category = getUInt16("category", false, 0);
  chainGeometry->mask = getUInt16("mask", false, 0xffff);
  chainGeometry->loop = getBool("loop", false, false);
  getColor("color", false, chainGeometry->color);
  return chainGeometry;
}

Element* ParserCore2D::convexGeometryElement()
{
  auto* const convexGeometry = new ConvexGeometry;
  convexGeometry->name = getString("name", false);
  convexGeometry->category = getUInt16("category", false, 0);
  convexGeometry->mask = getUInt16("mask", false, 0xffff);
  getColor("color", false, convexGeometry->color);
  return convexGeometry;
}

Element* ParserCore2D::diskGeometryElement()
{
  auto* const diskGeometry = new DiskGeometry;
  diskGeometry->name = getString("name", false);
  diskGeometry->category = getUInt16("category", false, 0);
  diskGeometry->mask = getUInt16("mask", false, 0xffff);
  diskGeometry->radius = getLength("radius", true, 0.f, true);
  getColor("color", false, diskGeometry->color);
  return diskGeometry;
}

Element* ParserCore2D::edgeGeometryElement()
{
  auto* const edgeGeometry = new EdgeGeometry;
  edgeGeometry->name = getString("name", false);
  edgeGeometry->category = getUInt16("category", false, 0);
  edgeGeometry->mask = getUInt16("mask", false, 0xffff);
  edgeGeometry->length = getLength("length", true, 0.f, true);
  getColor("color", false, edgeGeometry->color);
  return edgeGeometry;
}

Element* ParserCore2D::rectGeometryElement()
{
  auto* const rectGeometry = new RectGeometry;
  rectGeometry->name = getString("name", false);
  rectGeometry->category = getUInt16("category", false, 0);
  rectGeometry->mask = getUInt16("mask", false, 0xffff);
  rectGeometry->width = getLength("width", true, 0.f, true);
  rectGeometry->height = getLength("height", true, 0.f, true);
  getColor("color", false, rectGeometry->color);
  return rectGeometry;
}

void ParserCore2D::verticesText(std::string& text, Reader::Location location)
{
  std::vector<b2Vec2>* vertices;
  if(auto* const convexGeometry = dynamic_cast<ConvexGeometry*>(element); convexGeometry)
    vertices = &convexGeometry->vertices;
  else
    vertices = &dynamic_cast<ChainGeometry*>(element)->vertices;
  const char* str = text.c_str();
  char* nextStr;
  float components[2];
  skipWhitespace(str, location);
  while(*str)
  {
    for(float& component : components)
    {
      while(*str == '#') { while(*str && *str != '\n' && *str != '\r') { ++str; ++location.column; }  skipWhitespace(str, location); if(!*str) return; }
      component = std::strtof(str, &nextStr);
      if(str == nextStr)
      {
        handleError("Invalid vertex text (must be a space separated list of floats)", location);
        return;
      }
      location.column += static_cast<int>(nextStr - str);
      str = nextStr;
      skipWhitespace(str, location);
    }
    vertices->emplace_back(components[0], components[1]);
  }
}
