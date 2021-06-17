/**
 * @file Parser.cpp
 *
 * This file implements a class that parses .ros2d scene description files.
 *
 * @author Arne Hasselbring
 * @author Colin Graf (the parts which have been copied from SimRobotCore2)
 */

#include "Parser.h"
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
#include <sstream>

Parser::Parser()
{
  static const ElementInfo elements[] =
  {
    {"Simulation", infrastructureClass, &Parser::simulationElement, nullptr, 0, sceneClass, 0, 0},
    {"Include", infrastructureClass, &Parser::includeElement, nullptr, 0, 0, 0, 0},

    {"Set", setClass, &Parser::setElement, nullptr, 0, 0, 0, 0},

    {"Scene", sceneClass, &Parser::sceneElement, nullptr, 0, 0, 0, setClass | bodyClass | compoundClass},

    {"Body", bodyClass, &Parser::bodyElement, nullptr, 0, massClass, translationClass | rotationClass, setClass | massClass | geometryClass},

    {"Compound", compoundClass, &Parser::compoundElement, nullptr, 0, 0, translationClass | rotationClass, setClass | bodyClass | compoundClass | geometryClass},

    {"Translation", translationClass, &Parser::translationElement, nullptr, 0, 0, 0, 0},

    {"Rotation", rotationClass, &Parser::rotationElement, nullptr, 0, 0, 0, 0},

    {"Mass", massClass, &Parser::massElement, nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass},
    {"DiskMass", massClass, &Parser::diskMassElement, nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass},
    {"PointMass", massClass, &Parser::pointMassElement, nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass},
    {"RectMass", massClass, &Parser::rectMassElement, nullptr, constantFlag, 0, translationClass | rotationClass, setClass | massClass},

    {"Geometry", geometryClass, &Parser::geometryElement, nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass},
    {"ChainGeometry", geometryClass, &Parser::chainGeometryElement, &Parser::verticesText, textFlag, 0, translationClass | rotationClass, setClass | geometryClass},
    {"ConvexGeometry", geometryClass, &Parser::convexGeometryElement, &Parser::verticesText, textFlag, 0, translationClass | rotationClass, setClass | geometryClass},
    {"DiskGeometry", geometryClass, &Parser::diskGeometryElement, nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass},
    {"EdgeGeometry", geometryClass, &Parser::edgeGeometryElement, nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass},
    {"RectGeometry", geometryClass, &Parser::rectGeometryElement, nullptr, 0, 0, translationClass | rotationClass, setClass | geometryClass}
  };

  for(const ElementInfo& element : elements)
    elementInfos[element.name] = &element;
}

Parser::~Parser()
{
  for(const auto& pair : macros)
    delete pair.second;
}

bool Parser::parse(const std::string& fileName, std::list<std::string>& errors)
{
  this->errors = &errors;

  ASSERT(!Simulation::simulation->scene);

  // If the file is specified as a path, save the path to the directory containing it.
  const std::size_t i = fileName.find_last_of("/\\");
  parseRootDir = i != std::string::npos ? fileName.substr(0, i + 1) : std::string();

  const std::size_t preErrorCount = errors.size();
  do
  {
    // Parse the XML file and create macros.
    if(!readFile(fileName) || preErrorCount != errors.size())
      break;
    ASSERT(!Simulation::simulation->scene);

    // Create the scene graph using the macros.
    parseSimulation();
    if(preErrorCount != errors.size())
      break;

    ASSERT(Simulation::simulation->scene);
    return true;
  }
  while(true);

  // Apparently the error is that the file could not be opened at all or is completely invalid XML.
  if(preErrorCount == errors.size())
  {
    // Reset the file name to the root file (might have been overwritten during inclusion of other files).
    this->fileName = fileName;
    handleError("Could not load file", Location());
  }

  // Delete whatever elements have already been created.
  if(Simulation::simulation->scene)
  {
    for(Element* element : Simulation::simulation->elements)
      delete element;
    Simulation::simulation->elements.clear();
    Simulation::simulation->scene = nullptr;
  }
  return false;
}

void Parser::handleError(const std::string& msg, const Location& location)
{
  const std::string fileName = this->fileName.find(parseRootDir) == 0 ? this->fileName.substr(parseRootDir.length()) : this->fileName;

  // <file>:<line>:<column>: error: <msg>
  std::stringstream errorMessage;
  errorMessage << fileName;
  if(location.line)
  {
    errorMessage << ":" << location.line;
    if(location.column)
      errorMessage << ":" << location.column;
  }
  errorMessage << ": error: " << msg;
  errors->push_back(errorMessage.str());
}

bool Parser::handleElement(const std::string& name, Attributes& attributes, const Location& location)
{
  const auto iter = elementInfos.find(name);
  const ElementInfo* const elementInfo = iter != elementInfos.end() ? iter->second : nullptr;
  ASSERT(!elementInfo || elementInfo->startElementProc);

  // The <Simulation> tag must be the outermost and there must be no other ones.
  if(!elementInfo || passedSimulationTag == (elementInfo->startElementProc == &Parser::simulationElement))
  {
    handleError("Unexpected element \"" + name + "\"", location);
    return readElements(false);
  }

  // Due to the 32 bit attribute set.
  if(attributes.size() > 32)
  {
    handleError("Only up to 32 attributes per element are supported", location);
    return readElements(false);
  }

  // <Simulation> and <Include> need special handling.
  if(elementInfo->elementClass == infrastructureClass)
  {
    ElementData elementData(nullptr, location, elementInfo);
    this->elementData = &elementData;
    this->attributes = &attributes;

    (this->*elementInfo->startElementProc)();

    if(elementInfo->startElementProc == &Parser::includeElement)
    {
      // Save information that will be overwritten when reading the included file.
      const Location savedIncludeFileLocation = this->includeFileLocation;
      std::string savedIncludeFile;
      savedIncludeFile.swap(includeFile);
      // Check that there were no unknown attributes.
      checkAttributes();
      // Read subordinate elements (which should be none).
      const bool result = readElements(true);

      if(!savedIncludeFile.empty())
      {
        // Reset the parser state and save some more information.
        passedSimulationTag = false;
        const std::size_t preErrorCount = errors->size();
        const Location savedSimulationTagLocation = simulationTagLocation;
        std::string savedRootDir;
        savedRootDir.swap(parseRootDir);
        // The path to the included file may be relative to its including file.
        std::string fileName;
        if(savedIncludeFile[0] == '/' || savedIncludeFile[0] == '\\' || // absolute path on Unix
           (savedIncludeFile.size() >= 2 && savedIncludeFile[1] == ':')) // or Windows
          fileName = savedIncludeFile;
        else
          fileName = savedRootDir + savedIncludeFile;
        const std::size_t i = fileName.find_last_of("/\\");
        parseRootDir = i != std::string::npos ? fileName.substr(0, i + 1) : std::string();
        // Parse the included file.
        if(!readFile(fileName))
        {
          if(preErrorCount == errors->size())
            handleError("Could not include file \"" + savedIncludeFile + "\"", savedIncludeFileLocation);
        }
        // Restore parser state.
        parseRootDir = savedRootDir;
        passedSimulationTag = true;
        simulationTagLocation = savedSimulationTagLocation;
      }

      return result;
    }
    else
    {
      ASSERT(elementInfo->startElementProc == &Parser::simulationElement);
      // Only check that there are no attributes and parse children.
      checkAttributes();
      return readElements(true);
    }
  }

  // Expand paths in attributes.
  if(elementInfo->elementClass == sceneClass)
  {
    Attributes::iterator iter = attributes.find("background");
    if(iter != attributes.end() && !iter->second.value.empty() &&
       iter->second.value[0] != '/' && iter->second.value[0] != '\\' && // not absolute path on Unix
       (iter->second.value.size() < 2 || iter->second.value[1] != ':')) // or Windows
      iter->second.value = parseRootDir + iter->second.value;
  }

  // All children of the Simulation element are macros.
  if(recordingMacroElement)
  {
    // If there is already a macro being recorded, this element is added as child to it.
    auto* const newMacroElement = new MacroElement(recordingMacroElement, elementInfo, attributes, location);
    recordingMacroElement->children.push_back(newMacroElement);
    recordingMacroElement = newMacroElement;
    // The child elements are added to this macro element.
    const bool result = readElements(true);
    ASSERT(recordingMacroElement == newMacroElement);
    recordingMacroElement = recordingMacroElement->parent;
    return result;
  }
  else
  {
    // There may be only one Scene element in the scene description.
    const bool isScene = elementInfo->startElementProc == &Parser::sceneElement;
    if(isScene && sceneMacro)
    {
      handleError("Unexpected element \"" + name + "\"", location);
      return readElements(false);
    }

    ElementData elementData(nullptr, location, elementInfo);
    this->elementData = &elementData;
    this->attributes = &attributes;
    const std::string& macroName = getString("name", true);

    // The full macro name is combined from its name attribute and its element name. This combination must be unique.
    const std::string combinedMacroName = macroName + " " + name;
    if(macros.find(combinedMacroName) != macros.end())
    {
      handleError("Duplicated name \"" + macroName + "\"", attributes.find("name")->second.valueLocation);
      handleError("Note: Defined here", macros.find(combinedMacroName)->second->location);
      return readElements(false);
    }

    // A new macro is created from this element. Its children will be added in the call to \c readElements.
    auto* const macro = new Macro(elementInfo, fileName, attributes, location);
    macros[combinedMacroName] = macro;
    if(isScene)
      sceneMacro = macro;
    recordingMacroElement = macro;
    const bool result = readElements(true);
    ASSERT(recordingMacroElement == macro);
    recordingMacroElement = nullptr;
    return result;
  }
}

void Parser::handleText(std::string& text, const Location& location)
{
  // Only add text / data to element types that allow for it.
  if(!recordingMacroElement || !(recordingMacroElement->elementInfo->flags & textFlag))
  {
    handleError("Unexpected text", location);
    return;
  }

  // Add the text / data to the macro element being recorded.
  ASSERT(recordingMacroElement->text.empty());
  recordingMacroElement->text.swap(text);
  recordingMacroElement->textLocation = location;
}

void Parser::checkAttributes()
{
  // It is not an error if the name attribute has not been parsed. In that case, it is marked as parsed here.
  const auto iter = attributes->find("name");
  if(iter != attributes->end())
  {
    const Attribute& ai = iter->second;
    elementData->parsedAttributes |= 1u << ai.index;
  }

  // Construct the bit mask of the attributes that were not parsed.
  const unsigned int allAttributes = attributes->empty() ? 0u : (0xffffffffu >> static_cast<unsigned int>(32 - attributes->size()));
  const unsigned int unexpectedAttributes = allAttributes & ~elementData->parsedAttributes;
  // If there are any, errors are thrown.
  if(unexpectedAttributes)
  {
    for(const auto& attribute : *attributes)
      if(unexpectedAttributes & (1u << attribute.second.index))
        handleError("Unexpected attribute \"" + attribute.first + "\"", attribute.second.nameLocation);
  }
}

void Parser::checkElements()
{
  // Construct the bit mask of the element classes of which an element should have been a child but was not.
  const unsigned int missingChildren = elementData->info->requiredChildren & ~elementData->parsedChildren;
  if(missingChildren)
  {
    // Generate an error message for each element class.
    for(unsigned int i = 0; i < 32; ++i)
      if(missingChildren & (1u << i))
      {
        // Collect the element types one of which would have been required.
        const unsigned int missingClass = 1u << i;
        std::string elements;
        unsigned int count = 0;
        for(const auto& elementInfo : elementInfos)
          if(elementInfo.second->elementClass == missingClass)
          {
            if(count > 0)
              elements += ", ";
            elements += elementInfo.second->name;
            ++count;
          }
        ASSERT(count);
        if(count == 1)
          handleError("Expected element \"" + elements + "\" as child", elementData->location);
        else
          handleError("Expected one of the elements \"" + elements + "\" as child", elementData->location);
      }
  }
}

const std::string* Parser::resolvePlaceholder(const std::string& name)
{
  ASSERT(element);
  ElementData* elementData = this->elementData;
  elementData->usedPlaceholdersInAttributes = true;
  // Search in the namespace of all ancestor elements until the variable is found.
  do
  {
    const auto iter = elementData->vars.find(name);
    if(iter != elementData->vars.end())
      return &iter->second;
    elementData = elementData->parent;
  }
  while(elementData);
  return nullptr;
}

const std::string& Parser::replacePlaceholders(const std::string& str, const Location& location)
{
  std::size_t varStart = str.find_first_of('$');
  // No placeholders -> return input.
  if(varStart == std::string::npos)
    return str;

  // Add the part up to the first placeholder.
  std::string& result = placeholderBuffer;
  result.resize(0);
  result += str.substr(0, varStart);

  std::size_t varEnd;
  for(;;)
  {
    // Skip $.
    ++varStart;
    if(char c = str[varStart]; c == '(' || c == '{')
    {
      // Skip opening parenthesis.
      ++varStart;
      // Find closing parenthesis.
      const char cEnd = c == '(' ? ')' : '}';
      varEnd = str.find_first_of(cEnd, varStart);
      if(varEnd == std::string::npos)
      {
        handleError("Invalid attribute format", location);
        return str;
      }
      else
      {
        // The name of the placeholder is the string between the parentheses.
        const std::string name = str.substr(varStart, varEnd - varStart);
        const std::string* const value = resolvePlaceholder(name);
        // Either insert the variable (if it was defined) or the reference verbatim because the variable has not been defined.
        if(value)
          result += *value;
        else
          result += std::string("$") + c + name + cEnd;
        // Skip closing parenthesis.
        ++varEnd;
      }
    }
    else
    {
      varEnd = varStart;
      // The placeholder consists of all contiguous alphanumeric characters.
      while(std::isalnum(str[varEnd]))
        ++varEnd;
      const std::string name = str.substr(varStart, varEnd - varStart);
      const std::string* const value = resolvePlaceholder(name);
      // Either insert the variable (if it was defined) or the reference verbatim because the variable has not been defined.
      if(value)
        result += *value;
      else
        result += std::string("$") + name;
    }

    // Proceed up to the next $ or add the remaining string if there is none.
    varStart = str.find_first_of('$', varEnd);
    if(varStart == std::string::npos)
    {
      result += str.substr(varEnd);
      return result;
    }
  }
}

void Parser::parseSimulation()
{
  auto iter = elementInfos.find("Simulation");
  ASSERT(iter != elementInfos.end());
  const ElementInfo* const elementInfo = iter->second;
  ElementData elementData(nullptr, simulationTagLocation, elementInfo);
  this->elementData = &elementData;
  ASSERT(!element);

  // Replay the scene macro if it has been defined.
  if(sceneMacro)
  {
    sceneMacro->replaying = true;

    replayingMacroElement = sceneMacro;
    ElementData childElementData(&elementData, replayingMacroElement->location, replayingMacroElement->elementInfo);
    parseMacroElement(childElementData);
  }

  // Check that there were no required children missing.
  this->elementData = &elementData;
  checkElements();
}

void Parser::parseMacroElements()
{
  MacroElement* const parentReplayingMacroElement = replayingMacroElement;

  // Handle text / data of the parent.
  if(!replayingMacroElement->text.empty())
    (this->*elementData->info->textProc)(replayingMacroElement->text, replayingMacroElement->textLocation);

  const unsigned int parsedChildren = elementData->parsedChildren;
  elementData->parsedChildren = 0;
  for(MacroElement* child : replayingMacroElement->children)
  {
    replayingMacroElement = child;

    const ElementInfo* const parentInfo = elementData->info;
    const ElementInfo* const info = replayingMacroElement->elementInfo;
    if((parsedChildren & info->elementClass) && !(parentInfo->repeatableChildren & info->elementClass))
    {
      if(!(elementData->parsedChildren & info->elementClass))
      {
        elementData->parsedChildren |= info->elementClass;
        continue;
      }
    }

    ElementData* const parentElementData = elementData;
    Element* const parentElement = element;
    ElementData childElementData(elementData, replayingMacroElement->location, replayingMacroElement->elementInfo);
    parseMacroElement(childElementData);
    ASSERT(elementData->parent == parentElementData);
    parentElementData->usedPlaceholdersInAttributes |= elementData->usedPlaceholdersInAttributes;
    elementData = parentElementData;
    element = parentElement;
  }
  elementData->parsedChildren |= parsedChildren;

  replayingMacroElement = parentReplayingMacroElement;
}

void Parser::parseMacroElement(ElementData& elementData)
{
  this->elementData = &elementData;
  this->attributes = &replayingMacroElement->attributes;

  // Check if this element is allowed to be a child of its parent and throw an error otherwise.
  {
    ElementData* const parentElementData = elementData.parent;
    ASSERT(parentElementData);
    const ElementInfo* parentInfo = parentElementData->info;
    const ElementInfo* info = elementData.info;
    if(!((parentInfo->requiredChildren | parentInfo->optionalChildren | parentInfo->repeatableChildren) & info->elementClass) ||
       (parentElementData->parsedChildren & info->elementClass && !(parentInfo->repeatableChildren & info->elementClass)))
    {
      handleError("Unexpected element \"" + std::string(info->name) + "\"", replayingMacroElement->location);
      return;
    }
    parentElementData->parsedChildren |= info->elementClass;
  }

  // If there is already an instance of this macro element, use that.
  if(replayingMacroElement->element)
  {
    replayingMacroElement->element->addParent(*element);
    return;
  }

  // Check if this element references another macro.
  const std::string* ref = &getString("ref", false);

  // Handle elements that do not reference another macro.
  if(ref->empty())
  {
    // Create the new element and set it as current.
    Element* const parentElement = element;
    Element* const childElement = (this->*elementData.info->startElementProc)();
    element = childElement;
    // Check that all attributes have been used during creation of the element.
    checkAttributes();
    // Parse its children.
    parseMacroElements();
    ASSERT(this->elementData == &elementData);
    ASSERT(element == childElement);
    // Check that there were no required children missing.
    checkElements();
    if(element)
    {
      // Link element to its parent.
      if(parentElement)
        element->addParent(*parentElement);
      // Save the element instance for further usage if this is allowed and possible.
      if((elementData.info->flags & constantFlag) && !elementData.usedPlaceholdersInAttributes)
        replayingMacroElement->element = element;
    }
    return;
  }

  // Resolve the referenced macro.
  auto iter = macros.find(*ref + " " + elementData.info->name);
  Macro* const macro = iter == macros.end() ? nullptr : iter->second;
  if(!macro || macro->replaying)
  {
    if(macro)
      handleError("Looping reference \"" + *ref + "\"", attributes->find("ref")->second.valueLocation);
    else
      handleError("Unresolvable reference \"" + *ref + "\"", attributes->find("ref")->second.valueLocation);
    return;
  }

  // Handle "reference-only" elements (e.g. <Mass ref="anyMass"/>).
  const bool isReferenceOnlyElement = attributes->size() == 1 && !replayingMacroElement->hasTextOrChildren();
  if(isReferenceOnlyElement && macro->element)
  {
    // Use the already created "reference-only" instance.
    macro->element->addParent(*element);
    replayingMacroElement->element = macro->element;
    return;
  }

  // Handle normal macro references.
  {
    std::list<Macro*> referencedMacros;
    Attributes* copiedAttributes = nullptr; // We might need this, since this->attributes is read-only.

    macro->replaying = true;

    // Traverse the inheritance hierarchy to generate the list of referenced macros and the combined attribute set.
    Macro* nextMacro = macro;
    for(;;)
    {
      referencedMacros.push_back(nextMacro);

      // Combine current attributes with the attributes of the referenced macro.
      for(const auto& pair : nextMacro->attributes)
        if(attributes->find(pair.first) == attributes->end())
        {
          if(!copiedAttributes)
            attributes = copiedAttributes = new Attributes(*attributes);
          if(attributes->size() >= 32)
          {
            handleError("Macro attribute combination results in more than 32 attributes", replayingMacroElement->location);
            for(Macro* m : referencedMacros)
              m->replaying = false;
            return;
          }
          copiedAttributes->emplace(pair.first, Attribute(pair.second, static_cast<unsigned int>(copiedAttributes->size())));
        }

      // Check if the macro references another one. If not, we are done.
      const auto refIter = nextMacro->attributes.find("ref");
      if(refIter == nextMacro->attributes.end())
        break;
      ref = &replacePlaceholders(refIter->second.value, refIter->second.valueLocation);

      // Resolve the referenced macro.
      iter = macros.find(*ref + " " + elementData.info->name);
      nextMacro = iter == macros.end() ? nullptr : iter->second;
      if(!nextMacro || nextMacro->replaying)
      {
        if(nextMacro)
          handleError("Looping reference \"" + *ref + "\"", refIter->second.valueLocation);
        else
          handleError("Unresolvable reference \"" + *ref + "\"", refIter->second.valueLocation);
        for(Macro* m : referencedMacros)
          m->replaying = false;
        return;
      }
      nextMacro->replaying = true;
    }

    // Create the new element and set it as current.
    Element* const parentElement = element;
    Element* const childElement = (this->*elementData.info->startElementProc)();
    element = childElement;
    // Check that all attributes have been used during creation of the element.
    checkAttributes();
    delete copiedAttributes;
    // this->attributes is dangling now, but that's okay.

    // Parse direct subordinate elements.
    parseMacroElements();
    ASSERT(this->elementData == &elementData);
    ASSERT(element == childElement);

    // Parse inherited subordinate elements.
    MacroElement* const parentReplayingMacroElement = replayingMacroElement;
    for(Macro* m : referencedMacros)
    {
      ASSERT(m->replaying);
      fileName.swap(m->fileName);
      replayingMacroElement = m;

      parseMacroElements();
      ASSERT(this->elementData == &elementData);
      ASSERT(element == childElement);

      fileName.swap(m->fileName);
      m->replaying = false;
    }
    replayingMacroElement = parentReplayingMacroElement;

    // Check that there were no required children missing.
    checkElements();
    if(element)
    {
      // Link element to its parent.
      if(parentElement)
        element->addParent(*parentElement);
      // Save the element instance for further usage if this is allowed and possible.
      if(elementData.info->flags & constantFlag && !elementData.usedPlaceholdersInAttributes)
      {
        replayingMacroElement->element = element;
        // The element instance can also be saved for the macro that this element references (i.e. it is identical to it).
        if(isReferenceOnlyElement)
          macro->element = element;
      }
    }
    return;
  }
}

bool Parser::getStringRaw(const char* key, bool required, const std::string*& value)
{
  const auto iter = attributes->find(key);
  if(iter == attributes->end())
  {
    if(required)
      handleError("Expected attribute \"" + std::string(key) + "\"", elementData->location);
    return false;
  }
  const Attribute& ai = iter->second;
  elementData->parsedAttributes |= 1u << ai.index;
  value = &replacePlaceholders(ai.value, ai.valueLocation);
  return true;
}

bool Parser::getFloatRaw(const char* key, bool required, float& value)
{
  const std::string* strvalue;
  if(!getStringRaw(key, required, strvalue))
    return false;
  char* end;
  value = std::strtof(strvalue->c_str(), &end);
  if(*end)
  {
    handleError("Expected float", attributes->find(key)->second.valueLocation);
    return false;
  }
  return true;
}

bool Parser::getIntegerRaw(const char* key, bool required, int& value)
{
  const std::string* strvalue;
  if(!getStringRaw(key, required, strvalue))
    return false;
  char* end;
  value = static_cast<int>(std::strtol(strvalue->c_str(), &end, 10));
  if(*end)
  {
    handleError("Expected integer", attributes->find(key)->second.valueLocation);
    return false;
  }
  return true;
}

const std::string& Parser::getString(const char* key, bool required)
{
  const std::string* value;
  if(!getStringRaw(key, required, value))
  {
    static const std::string emptyString;
    return emptyString;
  }
  return *value;
}

bool Parser::getBool(const char* key, bool required, bool defaultValue)
{
  const std::string* value;
  if(!getStringRaw(key, required, value))
    return defaultValue;
  if(*value == "true" || *value == "1" || *value == "on")
    return true;
  if(*value == "false" || *value == "0" || *value == "off")
    return false;
  handleError("Expected boolean value (true or false)", attributes->find(key)->second.valueLocation);
  return defaultValue;
}

bool Parser::getFloatAndUnit(const char* key, bool required, float& value, char** unit, Location& unitLocation)
{
  const std::string* strValue;
  if(!getStringRaw(key, required, strValue))
    return false;
  unitLocation = attributes->find(key)->second.valueLocation;
  value = std::strtof(strValue->c_str(), unit);
  if(*unit == strValue->c_str())
  {
    handleError("Expected float", unitLocation);
    return false;
  }
  while(std::isspace(**unit))
    ++(*unit);
  unitLocation.column += static_cast<int>(*unit - strValue->c_str());
  return true;
}

int Parser::getIntegerNonZeroPositive(const char* key, bool required, int defaultValue)
{
  int value;
  if(!getIntegerRaw(key, required, value))
    return defaultValue;
  if(value <= 0)
  {
    handleError("Expected a positive non-zero value", attributes->find(key)->second.valueLocation);
    return defaultValue;
  }
  return value;
}

std::uint16_t Parser::getUInt16(const char* key, bool required, std::uint16_t defaultValue)
{
  int value;
  if(!getIntegerRaw(key, required, value))
    return defaultValue;
  if(value < 0 || value >= std::numeric_limits<std::uint16_t>::max())
  {
    handleError("Expected an unsigned 16 bit value", attributes->find(key)->second.valueLocation);
    return defaultValue;
  }
  return static_cast<std::uint16_t>(value);
}

float Parser::getLength(const char* key, bool required, float defaultValue)
{
  float result;
  char* endPtr;
  Location unitLocation;
  if(!getFloatAndUnit(key, required, result, &endPtr, unitLocation))
    return defaultValue;
  if(*endPtr)
  {
    if(std::strcmp(endPtr, "mm") == 0)
      result *= 0.001f;
    else if(std::strcmp(endPtr, "cm") == 0)
      result *= 0.01f;
    else if(std::strcmp(endPtr, "dm") == 0)
      result *= 0.1f;
    else if(std::strcmp(endPtr, "km") == 0)
      result *= 1000.f;
    else if(std::strcmp(endPtr, "m") != 0)
    {
      handleError("Unexpected unit \"" + std::string(endPtr) + "\" (expected one of \"mm, cm, dm, m, km\")", unitLocation);
      return defaultValue;
    }
  }
  return result;
}

float Parser::getAngle(const char* key, bool required, float defaultValue)
{
  float result;
  char* endPtr;
  Location unitLocation;
  if(!getFloatAndUnit(key, required, result, &endPtr, unitLocation))
    return defaultValue;
  if(*endPtr)
  {
    if(std::strcmp(endPtr, "degree") == 0)
      result *= pi / 180.f;
    else if(std::strcmp(endPtr, "radian") != 0)
    {
      handleError("Unexpected unit \"" + std::string(endPtr) + "\" (expected one of \"degree, radian\")", unitLocation);
      return defaultValue;
    }
  }
  return result;
}

float Parser::getMass(const char* key, bool required, float defaultValue)
{
  float result;
  char* endPtr;
  Location unitLocation;
  if(!getFloatAndUnit(key, required, result, &endPtr, unitLocation))
    return defaultValue;
  if(*endPtr)
  {
    if(std::strcmp(endPtr, "g") == 0)
      result *= 0.001f;
    else if(std::strcmp(endPtr, "kg") != 0)
    {
      handleError("Unexpected unit \"" + std::string(endPtr) + "\" (expected one of \"g, kg\")", unitLocation);
      return defaultValue;
    }
  }
  if(result <= 0.f)
    handleError("A mass should be greater than zero", attributes->find(key)->second.valueLocation);
  return result;
}

float Parser::getLengthNonZeroPositive(const char* key, bool required, float defaultValue)
{
  float result;
  char* endPtr;
  Location unitLocation;
  if(!getFloatAndUnit(key, required, result, &endPtr, unitLocation))
    return defaultValue;
  if(result <= 0.f)
  {
    handleError("Expected a positive non-zero value", attributes->find(key)->second.valueLocation);
    return defaultValue;
  }
  if(*endPtr)
  {
    if(std::strcmp(endPtr, "mm") == 0)
      result *= 0.001f;
    else if(std::strcmp(endPtr, "cm") == 0)
      result *= 0.01f;
    else if(std::strcmp(endPtr, "dm") == 0)
      result *= 0.1f;
    else if(std::strcmp(endPtr, "km") == 0)
      result *= 1000.f;
    else if(std::strcmp(endPtr, "m") != 0)
    {
      handleError("Unexpected unit \"" + std::string(endPtr) + "\" (expected one of \"mm, cm, dm, m, km\")", unitLocation);
      return defaultValue;
    }
  }
  return result;
}

float Parser::getTimeNonZeroPositive(const char* key, bool required, float defaultValue)
{
  float result;
  char* endPtr;
  Location unitLocation;
  if(!getFloatAndUnit(key, required, result, &endPtr, unitLocation))
    return defaultValue;
  if(result <= 0.f)
  {
    handleError("Expected a positive non-zero value", attributes->find(key)->second.valueLocation);
    return defaultValue;
  }
  if(*endPtr)
  {
    if(std::strcmp(endPtr, "s") != 0)
    {
      handleError("Unexpected unit \"" + std::string(endPtr) + "\" (expected \"s\")", unitLocation);
      return defaultValue;
    }
  }
  return result;
}

bool Parser::getColor(const char* key, bool required, QColor& color)
{
  const std::string* strValue;
  if(!getStringRaw(key, required, strValue))
    return false;
  Location location = attributes->find(key)->second.valueLocation;
  const char* strColor = strValue->c_str();
  if(*strColor == '#')
  {
    // HTML style color (#rrggbb and #rgb)
    // + self invented #rrggbbaa and #rgba
    ++strColor;
    unsigned int lcol = 0;
    const char* endPtr = strColor;
    for(;;)
    {
      const int c = std::tolower(*endPtr);
      if(c >= '0' && c <= '9')
      {
        lcol <<= 4u;
        lcol |= static_cast<unsigned int>(c - '0');
      }
      else if(c >= 'a' && c <= 'f')
      {
        lcol <<= 4u;
        lcol |= static_cast<unsigned int>(c - 'a' + 10);
      }
      else if(!c)
        break;
      else
      {
        location.column += static_cast<int>(strColor - strValue->c_str());
        handleError("Invalid color format", location);
        return false;
      }
      ++endPtr;
    }
    switch(endPtr - strColor)
    {
      case 3:
        color.setRed(static_cast<int>(lcol >> 8u));
        color.setGreen(static_cast<int>((lcol >> 4u) & 0xfu));
        color.setBlue(static_cast<int>(lcol & 0xfu));
        color.setAlpha(255);
        return true;
      case 4:
        color.setRed(static_cast<int>(lcol >> 12u));
        color.setGreen(static_cast<int>((lcol >> 8u) & 0xfu));
        color.setBlue(static_cast<int>((lcol >> 4u) & 0xfu));
        color.setAlpha(static_cast<int>(lcol & 0xfu));
        return true;
      case 6:
        color.setRed(static_cast<int>(lcol >> 16u));
        color.setGreen(static_cast<int>((lcol >> 8u) & 0xffu));
        color.setBlue(static_cast<int>(lcol & 0xffu));
        color.setAlpha(255);
        return true;
      case 8:
        color.setRed(static_cast<int>(lcol >> 24u));
        color.setGreen(static_cast<int>((lcol >> 16u) & 0xffu));
        color.setBlue(static_cast<int>((lcol >> 8u) & 0xffu));
        color.setAlpha(static_cast<int>(lcol & 0xffu));
        return true;
      default:
        handleError("Invalid color format", location);
        return false;
    }
  }
  else if(std::strncmp(strColor, "rgb(", 4) == 0)
  {
    // CSS style RGB color (rgb(r,g,b) with r,g,b\in[0..255]\cup[0%,..,100%])
    strColor += 4;
    int colors[3];
    for(int i = 0;; ++i)
    {
      while(std::isspace(*strColor))
        ++strColor;
      colors[i] = static_cast<int>(std::strtol(strColor, const_cast<char**>(&strColor), 10));
      if(*strColor == '%')
      {
        ++strColor;
        colors[i] = (colors[i] * 255 + 50) / 100;
      }
      while(std::isspace(*strColor))
        ++strColor;
      if(i >= 2 || *strColor != ',')
        break;
      ++strColor;
    }
    if(std::strcmp(strColor, ")") != 0)
    {
      location.column += static_cast<int>(strColor - strValue->c_str());
      handleError("Invalid color format", location);
      return false;
    }
    color.setRed(colors[0]);
    color.setGreen(colors[1]);
    color.setBlue(colors[2]);
    color.setAlpha(255);
    return true;
  }
  else if(std::strncmp(strColor, "rgba(", 5) == 0)
  {
    // CSS3 style RGBA color (rgba(r,g,b,a) with r,g,b\in[0..255]\cup[0%,..,100%] and a\in[0..1])
    // http://www.w3.org/TR/css3-color/
    strColor += 5;
    int colors[4];
    for(int i = 0;; ++i)
    {
      while(std::isspace(*strColor))
        ++strColor;
      colors[i] = static_cast<int>(std::strtol(strColor, const_cast<char**>(&strColor), 10));
      if(i < 3)
      {
        if(*strColor == '%')
        {
          ++strColor;
          colors[i] = (colors[i] * 255 + 50) / 100;
        }
      }
      while(std::isspace(*strColor))
        ++strColor;
      if(i >= 3 || *strColor != ',')
        break;
      ++strColor;
    }
    if(std::strcmp(strColor, ")") != 0)
    {
      location.column += static_cast<int>(strColor - strValue->c_str());
      handleError("Invalid color format", location);
      return false;
    }
    color.setRed(colors[0]);
    color.setGreen(colors[1]);
    color.setBlue(colors[2]);
    color.setAlpha(colors[3]);
    return true;
  }
  else
  {
    handleError("Invalid color format", location);
    return false;
  }
}

Element* Parser::simulationElement()
{
  passedSimulationTag = true;
  simulationTagLocation = elementData->location;
  return nullptr;
}

Element* Parser::includeElement()
{
  includeFile = getString("href", true);
  if(!includeFile.empty())
    includeFileLocation = attributes->find("href")->second.valueLocation;
  return nullptr;
}

Element* Parser::setElement()
{
  ASSERT(element);
  const std::string& name = getString("name", true);
  const std::string& value = getString("value", true);
  std::unordered_map<std::string, std::string>& vars = elementData->parent->vars;
  if(vars.find(name) == vars.end())
    vars[name] = value;
  return nullptr;
}

Element* Parser::sceneElement()
{
  auto* const scene = new Scene;
  scene->name = getString("name", false);
  scene->controller = getString("controller", false);
  scene->stepLength = getTimeNonZeroPositive("stepLength", false, 0.01f);
  scene->velocityIterations = getIntegerNonZeroPositive("velocityIterations", false, 8);
  scene->positionIterations = getIntegerNonZeroPositive("positionIterations", false, 3);
  scene->background = getString("background", false);

  ASSERT(!Simulation::simulation->scene);
  Simulation::simulation->scene = scene;
  return scene;
}

Element* Parser::bodyElement()
{
  auto* const body = new Body;
  body->name = getString("name", false);
  return body;
}

Element* Parser::compoundElement()
{
  auto* const compound = new Compound;
  compound->name = getString("name", false);
  return compound;
}

Element* Parser::translationElement()
{
  auto* const translation = new b2Vec2(getLength("x", false, 0.f), getLength("y", false, 0.f));

  auto* const simObject = dynamic_cast<SimObject*>(element);
  ASSERT(simObject);
  ASSERT(!simObject->translation);
  simObject->translation = translation;

  return nullptr;
}

Element* Parser::rotationElement()
{
  auto* const rotation = new b2Rot(getAngle("angle", false, 0.f));

  auto* const simObject = dynamic_cast<SimObject*>(element);
  ASSERT(simObject);
  ASSERT(!simObject->rotation);
  simObject->rotation = rotation;

  return nullptr;
}

Element* Parser::massElement()
{
  auto* const mass = new Mass;
  mass->name = getString("name", false);
  return mass;
}

Element* Parser::diskMassElement()
{
  auto* const diskMass = new DiskMass;
  diskMass->name = getString("name", false);
  diskMass->value = getMass("value", true, 0.f);
  diskMass->radius = getLengthNonZeroPositive("radius", true, 0.f);
  return diskMass;
}

Element* Parser::pointMassElement()
{
  auto* const pointMass = new PointMass;
  pointMass->name = getString("name", false);
  pointMass->value = getMass("value", true, 0.f);
  return pointMass;
}

Element* Parser::rectMassElement()
{
  auto* const rectMass = new RectMass;
  rectMass->name = getString("name", false);
  rectMass->value = getMass("value", true, 0.f);
  rectMass->width = getLengthNonZeroPositive("width", true, 0.f);
  rectMass->height = getLengthNonZeroPositive("height", true, 0.f);
  return rectMass;
}

Element* Parser::geometryElement()
{
  auto* const geometry = new Geometry;
  geometry->name = getString("name", false);
  geometry->category = getUInt16("category", false, 0);
  geometry->mask = getUInt16("mask", false, 0xffff);
  return geometry;
}

Element* Parser::chainGeometryElement()
{
  auto* const chainGeometry = new ChainGeometry;
  chainGeometry->name = getString("name", false);
  chainGeometry->category = getUInt16("category", false, 0);
  chainGeometry->mask = getUInt16("mask", false, 0xffff);
  chainGeometry->loop = getBool("loop", false, false);
  getColor("color", false, chainGeometry->color);
  return chainGeometry;
}

Element* Parser::convexGeometryElement()
{
  auto* const convexGeometry = new ConvexGeometry;
  convexGeometry->name = getString("name", false);
  convexGeometry->category = getUInt16("category", false, 0);
  convexGeometry->mask = getUInt16("mask", false, 0xffff);
  getColor("color", false, convexGeometry->color);
  return convexGeometry;
}

Element* Parser::diskGeometryElement()
{
  auto* const diskGeometry = new DiskGeometry;
  diskGeometry->name = getString("name", false);
  diskGeometry->category = getUInt16("category", false, 0);
  diskGeometry->mask = getUInt16("mask", false, 0xffff);
  diskGeometry->radius = getLengthNonZeroPositive("radius", true, 0.f);
  getColor("color", false, diskGeometry->color);
  return diskGeometry;
}

Element* Parser::edgeGeometryElement()
{
  auto* const edgeGeometry = new EdgeGeometry;
  edgeGeometry->name = getString("name", false);
  edgeGeometry->category = getUInt16("category", false, 0);
  edgeGeometry->mask = getUInt16("mask", false, 0xffff);
  edgeGeometry->length = getLengthNonZeroPositive("length", true, 0.f);
  getColor("color", false, edgeGeometry->color);
  return edgeGeometry;
}

Element* Parser::rectGeometryElement()
{
  auto* const rectGeometry = new RectGeometry;
  rectGeometry->name = getString("name", false);
  rectGeometry->category = getUInt16("category", false, 0);
  rectGeometry->mask = getUInt16("mask", false, 0xffff);
  rectGeometry->width = getLengthNonZeroPositive("width", true, 0.f);
  rectGeometry->height = getLengthNonZeroPositive("height", true, 0.f);
  getColor("color", false, rectGeometry->color);
  return rectGeometry;
}

void Parser::verticesText(std::string& text, Reader::Location location)
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
