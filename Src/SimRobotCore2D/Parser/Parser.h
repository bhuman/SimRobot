/**
 * @file Parser.h
 *
 * This file declares a class that parses .ros2d scene description files.
 *
 * @author Arne Hasselbring
 * @author Colin Graf (the parts which have been copied from SimRobotCore2)
 */

#pragma once

#include "Parser/Reader.h"
#include <list>
#include <string>
#include <unordered_map>

class Element;
class QColor;

class Parser : protected Reader
{
public:
  /** Constructor. */
  Parser();

  /** Destructor. */
  ~Parser();

  /**
   * Parses a .ros2d file into the scene graph.
   * @param fileName The name of the file to parse.
   * @param errors A list which is filled with messages about errors during parsing.
   * @return Whether the file was parsed successfully.
   */
  bool parse(const std::string& fileName, std::list<std::string>& errors);

private:
  using StartElementProc = Element* (Parser::*)();
  using TextProc = void (Parser::*)(std::string& text, Location location);

  enum ElementClass
  {
    infrastructureClass = 0u,
    setClass            = (1u << 0u),
    sceneClass          = (1u << 1u),
    bodyClass           = (1u << 2u),
    compoundClass       = (1u << 3u),
    translationClass    = (1u << 4u),
    rotationClass       = (1u << 5u),
    massClass           = (1u << 6u),
    geometryClass       = (1u << 7u)
  };

  enum ElementFlags
  {
    textFlag = (1u << 0u), /**< The element can have a text / data segment. */
    constantFlag = (1u << 1u), /**< The element is constant in a way that it can be used multiple times in scene graph to avoid multiple element instantiations. */
  };

  struct ElementInfo final
  {
    const char* name; /**< The name of the element type. */
    unsigned int elementClass; /**< The class which the element type belongs to. */
    StartElementProc startElementProc; /**< The function that is called when an element of this type is encountered. */
    TextProc textProc; /**< The function that is called when text / data within an element of this type is encountered. */
    unsigned int flags; /**< Additional flags from \c ElementFlags. */
    unsigned int requiredChildren; /**< Element classes of which one must be a child of this element type. */
    unsigned int optionalChildren; /**< Element classes which may be children of this element type once. */
    unsigned int repeatableChildren; /**< Element classes which may be children of this element type in an arbitrary number. */
  };

  struct ElementData final
  {
    ElementData* parent; /**< The parsing context of the parent element. */
    const ElementInfo* info; /**< The info about the type of this element. */
    unsigned int parsedChildren = 0; /**< Bit mask of the already parsed child element classes. */
    unsigned int parsedAttributes = 0; /**< Bit mask of the already parsed attributes. */
    std::unordered_map<std::string, std::string> vars; /**< User defined variables for placeholders in attributes. */
    bool usedPlaceholdersInAttributes = false; /**< Whether this element used placeholders in its attributes. */
    Location location; /**< The location of the instantiated element. */

    ElementData(ElementData* parent, const Location& location, const ElementInfo* info) :
      parent(parent), info(info), location(location)
    {}
  };

  struct MacroElement
  {
  public:
    MacroElement* parent; /**< The parent macro element. */
    const ElementInfo* elementInfo; /**< The info about the type of this macro element */
    Attributes attributes; /**< The attributes of this macro element. */
    std::string text; /**< The text / data belonging to this macro element (can be empty if there is none). */
    Location textLocation; /**< The location of the text / data (if there is some). */
    std::list<MacroElement*> children; /**< The child macro elements of this macro element. */
    Element* element = nullptr; /**< An actual element that was created from the macro element. */
    Location location; /**< The location of the macro element. */

    MacroElement(MacroElement* parent, const ElementInfo* elementInfo, Attributes& attributes, const Location& location) :
      parent(parent), elementInfo(elementInfo), location(location)
    {
      this->attributes.swap(attributes);
    }

    ~MacroElement()
    {
      for(MacroElement* child : children)
        delete child;
    }

    /**
     * Returns whether the macro element has child elements or text / data inside.
     * @return Whether the macro element ...
     */
    bool hasTextOrChildren() const
    {
      return !children.empty() || !text.empty();
    }
  };

  struct Macro : MacroElement
  {
    std::string fileName; /**< The file in which the macro was declared. */
    bool replaying = false; /**< A flag for detecting macro reference loops. */

    Macro(const ElementInfo* elementInfo, std::string fileName, Attributes& attributes, const Location& location) :
      MacroElement(nullptr, elementInfo, attributes, location), fileName(std::move(fileName))
    {}
  };

  /**
   * Handler for errors during parsing.
   * @param msg An error message.
   * @param location The location where the error occurred.
   */
  void handleError(const std::string& msg, const Location& location) override;

  /**
   * Handler for XML elements.
   * @param name The name of the element.
   * @param attributes The attributes of the element.
   * @param location The location of the element.
   * @return Whether subordinate elements could be read successfully.
   */
  bool handleElement(const std::string& name, Attributes& attributes, const Location& location) override;

  /**
   * Handler for text / data.
   * @param text The text / data to be handled.
   * @param location The location of the text / data.
   */
  void handleText(std::string& text, const Location& location) override;

  /** Checks if there are any unexpected attributes in the current set of attributes. */
  void checkAttributes();

  /** Checks if some required subordinate elements have not been parsed. */
  void checkElements();

  /**
   * Resolves a placeholder in the context of the current element.
   * @param name The name of the placeholder
   * @return A pointer to the value of the placeholder (or null if it is not defined).
   */
  const std::string* resolvePlaceholder(const std::string& name);

  /**
   * Replaces placeholders ($String) in a string with their values.
   * @param str The string in which to replace placeholders with their values.
   * @param location The location of the string.
   * @return The resulting string (which is overwritten by further calls to this method).
   */
  const std::string& replacePlaceholders(const std::string& str, const Location& location);

  /** Instantiates the elements below <Simulation>. */
  void parseSimulation();

  /** Instantiates all children of the currently replaying macro element. */
  void parseMacroElements();

  /**
   * Instantiates a macro element.
   * @param elementData The parsing context of the macro element.
   */
  void parseMacroElement(ElementData& elementData);

  bool getStringRaw(const char* key, bool required, const std::string*& value);
  bool getFloatRaw(const char* key, bool required, float& value);
  bool getIntegerRaw(const char* key, bool required, int& value);
  const std::string& getString(const char* key, bool required);
  bool getBool(const char* key, bool required, bool defaultValue);
  bool getFloatAndUnit(const char* key, bool required, float& value, char** unit, Location& unitLocation);
  int getIntegerNonZeroPositive(const char* key, bool required, int defaultValue);
  std::uint16_t getUInt16(const char* key, bool required, std::uint16_t defaultValue);
  float getLength(const char* key, bool required, float defaultValue);
  float getAngle(const char* key, bool required, float defaultValue);
  float getMass(const char* key, bool required, float defaultValue);
  float getLengthNonZeroPositive(const char* key, bool required, float defaultValue);
  float getTimeNonZeroPositive(const char* key, bool required, float defaultValue);
  bool getColor(const char* key, bool required, QColor& color);

  Element* simulationElement();
  Element* includeElement();
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

  std::unordered_map<std::string, const ElementInfo*> elementInfos; /**< Mapping element name strings to handler info. */

  std::list<std::string>* errors = nullptr; /**< List of error messages that occurred during parsing. */
  std::string parseRootDir; /**< The directory in which the main .ros2d file is stored. */
  std::string includeFile; /**< A file to be included. */
  Location includeFileLocation; /**< The location of the path to the included file in the including file. */

  bool passedSimulationTag = false; /**< Whether the <Simulation> tag has been passed yet. */
  Location simulationTagLocation; /**< The location of the <Simulation> tag. */

  std::unordered_map<std::string, Macro*> macros; /**< A storage for macros. */
  Macro* sceneMacro = nullptr; /**< The macro created from the <Scene> element. */

  MacroElement* recordingMacroElement = nullptr; /**< A macro element set to record subordinate nodes of a macro. */
  MacroElement* replayingMacroElement = nullptr; /**< A macro element set to insert subordinate nodes of a macro. */
  Element* element = nullptr; /**< The last inserted XML element. */
  ElementData* elementData = nullptr; /** Element context data required for parsing an XML element. */
  const Attributes* attributes = nullptr; /**< The current set of attributes. */

  std::string placeholderBuffer; /**< A buffer which contains the most recently resolved placeholder. */
};
