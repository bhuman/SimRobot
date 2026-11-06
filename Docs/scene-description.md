# Scene Description

## Structure of a Scene Description File

### The Beginning of a Scene File

Every scene file consists of a `<Simulation>` element. In addition to other elements, it can contain `<Include href="...">` tags to insert the contents of other files. These also consist of `<Simulation>` elements, but after the inclusion, their `Simulation` element is removed and only its contents are inserted into the including file. After all includes are resolved, the main `<Simulation>` element must at least contain a `<Scene>` element. The `Scene` specifies which controller is loaded for this scene via the `controller` attribute.


### The ref Attribute

An element with a `name` attribute can be referenced by the `ref`-attribute using its name, i.e. elements that are needed repeatedly in a scene need to be defined only once. For example there is only one description of a NAO in its definition file (`NaoV6H25.rsi2`), but NAOs with different jersey colors are needed on a field. For each NAO on the field, there is a reference to the original model. The positioning of the NAOs is done by `Translation` and `Rotation` elements. The color is set by a `Set` element, which is described below.

       ⫶
    <Body name="Nao">
      <Set name="NaoColor" value="blue"/>
        ⫶
    </Body>
       ⫶
    <Body ref="Nao" name="BlueNao">
      <Translation x="-2" y="0.4" z="320mm"/>
    </Body>
    <Body ref="Nao" name="RedNao">
      <Translation x="-1.5" y="-0.9" z="320mm"/>
      <Rotation z="180degree"/>
      <Set name="NaoColor" value="red"/>
    </Body>
       ⫶


### Placeholders and Set Element

A placeholder has to start with a `$` followed by an arbitrary string. A placeholder is replaced by the definition specified within the corresponding `Set` element. The attribute `name` of a `Set` elements specifies the placeholder, which is replaced by the value specified by the attribute `value` of the `Set` element.

In the following code example, the color of NAO's jersey is set by a `Set` element. Within the definition of the body *Nao* named *RedNao*, the *Set* element sets the placeholder color to the value *red*. The placeholder named *NaoColor* of *Nao*, which is defined in the general definition of a NAO, is replaced by *red* in all elements of the model, also in the ones that are just referenced, such as the appearances of individual body parts. So the `Surface` elements reference a surface named *nao-red*.

       ⫶
    <ComplexAppearance name="naoTorsoV6_jersey">
      <Surface ref="nao-$NaoColor"/>
         ⫶
    </ComplexAppearance>
       ⫶
    <Surface name="nao-red" albedo="rgb(100%, 0%, 0%)"/>
       ⫶


## Grammar

Scene decription files are encoded in XML. Since document type definitions or XML schemas are hard to read, the format description is split into two parts. In this section, the relations between different tags are described in an EBNF-ish grammar. However, the attributes of the tags are missing, i.e. a terminal symbol such as `"<Deflection>"` or `<Deflection/>` can actually have a number of attributes that are described later in the [next section](#attributes).

In addition to the usual grouping elements `(...)`, `[...]`, and `{...}`, the following EBNF grammar uses the fourth one `?(...)?` that defines that the sequence of the grammar symbols in between is arbitrary. This reflects the fact that XML usually does not enforce a certain order for subtags. The start symbol of the grammar is `Simulation`.

    appearanceClass            = Appearance
                               | BoxAppearance
                               | CapsuleAppearance
                               | ComplexAppearance
                               | CylinderAppearance
                               | SphereAppearance;
    axisClass                  = Axis;
    bodyClass                  = Body;
    compoundClass              = Compound;
    deflectionClass            = Deflection;
    extSensorClass             = Camera
                               | DepthImageSensor
                               | ObjectSegmentedImageSensor
                               | SingleDistanceSensor;
    geometryClass              = BoxGeometry
                               | CapsuleGeometry
                               | CylinderGeometry
                               | Geometry
                               | SphereGeometry;
    infrastructureClass        = Include
                               | Simulation;
    intSensorClass             = Accelerometer
                               | CollisionSensor
                               | Gyroscope;
    jointClass                 = Hinge
                               | Slider;
    lightClass                 = DirLight
                               | PointLight
                               | SpotLight;
    massClass                  = BoxMass
                               | CapsuleMass
                               | CylinderMass
                               | InertiaMatrixMass
                               | Mass
                               | SphereMass;
    materialClass              = Material;
    motorClass                 = ServoMotor
                               | VelocityMotor;
    normalsClass               = Normals;
    primitiveGroupClass        = Quads
                               | Triangles;
    rotationClass              = Rotation;
    sceneClass                 = Scene;
    setClass                   = Set;
    shadowMapClass             = ShadowMap;
    surfaceClass               = Surface;
    texCoordsClass             = TexCoords;
    translationClass           = Translation;
    userInputClass             = UserInput;
    verticesClass              = Vertices;
    
    Appearance                 = "<Appearance>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | appearanceClass} )?
                                 "</Appearance>"
                               | "<Appearance/>";
    BoxAppearance              = "<BoxAppearance>"
                                 ?( surfaceClass
                                    [translationClass] [rotationClass]
                                    {setClass | appearanceClass} )?
                                 "</BoxAppearance>";
    CapsuleAppearance          = "<CapsuleAppearance>"
                                 ?( surfaceClass
                                    [translationClass] [rotationClass]
                                    {setClass | appearanceClass} )?
                                 "</CapsuleAppearance>";
    ComplexAppearance          = "<ComplexAppearance>"
                                 ?( surfaceClass verticesClass primitiveGroupClass
                                    [translationClass] [rotationClass]
                                    [normalsClass] [texCoordsClass]
                                    {setClass | appearanceClass | primitiveGroupClass} )?
                                 "</ComplexAppearance>";
    CylinderAppearance         = "<CylinderAppearance>"
                                 ?( surfaceClass
                                    [translationClass] [rotationClass]
                                    {setClass | appearanceClass} )?
                                 "</CylinderAppearance>";
    SphereAppearance           = "<SphereAppearance>"
                                 ?( surfaceClass
                                    [translationClass] [rotationClass]
                                    {setClass | appearanceClass} )?
                                 "</SphereAppearance>";
    
    Axis                       = "<Axis>"
                                 ?( [motorClass] [deflectionClass]
                                    {setClass} )?
                                 "</Axis>"
                               | "<Axis/>";
    
    Body                       = "<Body>"
                                 ?( massClass
                                    [translationClass] [rotationClass]
                                    {setClass | jointClass | appearanceClass
                                     | geometryClass | massClass | intSensorClass
                                     | extSensorClass | userInputClass} )?
                                 "</Body>";
    
    Compound                   = "<Compound>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | jointClass | compoundClass | bodyClass
                                     | appearanceClass | geometryClass | extSensorClass
                                     | userInputClass} )?
                                 "</Compound>"
                               | "<Compound/>";
    
    Deflection                 = "<Deflection>" "</Deflection>"
                               | "<Deflection/>";
    
    Camera                     = "<Camera>"
                                 ?( [translationClass] [rotationClass] )?
                                 "</Camera>"
                               | "<Camera/>";
    DepthImageSensor           = "<DepthImageSensor>"
                                 ?( [translationClass] [rotationClass] )?
                                 "</DepthImageSensor>"
                               | "<DepthImageSensor/>";
    ObjectSegmentedImageSensor = "<ObjectSegmentedImageSensor>"
                                 ?( [translationClass] [rotationClass] )?
                                 "</ObjectSegmentedImageSensor>"
                               | "<ObjectSegmentedImageSensor/>";
    SingleDistanceSensor       = "<SingleDistanceSensor>"
                                 ?( [translationClass] [rotationClass] )?
                                 "</SingleDistanceSensor>"
                               | "<SingleDistanceSensor/>";
    
    BoxGeometry                = "<BoxGeometry>"
                                 ?( [translationClass] [rotationClass] [materialClass]
                                    {setClass | geometryClass} )?
                                 "</BoxGeometry>"
                               | "<BoxGeometry/>";
    CapsuleGeometry            = "<CapsuleGeometry>"
                                 ?( [translationClass] [rotationClass] [materialClass]
                                    {setClass | geometryClass} )?
                                 "</CapsuleGeometry>"
                               | "<CapsuleGeometry/>";
    CylinderGeometry           = "<CylinderGeometry>"
                                 ?( [translationClass] [rotationClass] [materialClass]
                                    {setClass | geometryClass} )?
                                 "</CylinderGeometry>"
                               | "<CylinderGeometry/>";
    Geometry                   = "<Geometry>"
                                 ?( [translationClass] [rotationClass] [materialClass]
                                    {setClass | geometryClass} )?
                                 "</Geometry>"
                               | "<Geometry/>";
    SphereGeometry             = "<SphereGeometry>"
                                 ?( [translationClass] [rotationClass] [materialClass]
                                    {setClass | geometryClass} )?
                                 "</SphereGeometry>"
                               | "<SphereGeometry/>";
    
    Simulation                 = "<Simulation>"
                                 ?( sceneClass
                                    { Include | appearanceClass | bodyClass
                                    | compoundClass | extSensorClass | geometryClass
                                    | intSensorClass | jointClass | massClass
                                    | materialClass | normalsClass | primitiveGroupClass
                                    | setClass | surfaceClass | texCoordsClass
                                    | userInputClass | verticesClass} )?
                                 "</Simulation>";
    Include                    = "<Include>" "</Include>"
                               | "<Include/>";
    
    Accelerometer              = "<Accelerometer>"
                                 ?( [translationClass] [rotationClass] )?
                                 "</Accelerometer>"
                               | "<Accelerometer/>";
    CollisionSensor            = "<CollisionSensor>"
                                 ?( [translationClass] [rotationClass]
                                    {geometryClass} )?
                                 "</CollisionSensor>"
                               | "<CollisionSensor/>";
    Gyroscope                  = "<Gyroscope>"
                                 ?( [translationClass] [rotationClass] )?
                                 "</Gyroscope>"
                               | "<Gyroscope/>";
    
    Hinge                      = "<Hinge>"
                                 ?( bodyClass axisClass
                                    [translationClass] [rotationClass]
                                    {setClass} )?
                                 "</Hinge>";
    Slider                     = "<Slider>"
                                 ?( bodyClass axisClass
                                    [translationClass] [rotationClass]
                                    {setClass} )?
                                 "</Slider>";
    
    DirLight                   = "<DirLight>"
                                 ?( [shadowMapClass] )?
                                 "</DirLight>"
                               | "<DirLight/>";
    
    PointLight                 = "<PointLight>"
                                 ?( [shadowMapClass] )?
                                 "</PointLight>"
                               | "<PointLight/>";
    
    SpotLight                  = "<SpotLight>"
                                 ?( [shadowMapClass] )?
                                 "</SpotLight>"
                               | "<SpotLight/>";
    
    BoxMass                    = "<BoxMass>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | massClass} )?
                                 "</BoxMass>"
                               | "<BoxMass/>";
    CapsuleMass                = "<CapsuleMass>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | massClass} )?
                                 "</CapsuleMass>"
                               | "<CapsuleMass/>";
    CylinderMass               = "<CylinderMass>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | massClass} )?
                                 "</CylinderMass>"
                               | "<CylinderMass/>";
    InertiaMatrixMass          = "<InertiaMatrixMass>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | massClass} )?
                                 "</InertiaMatrixMass>"
                               | "<InertiaMatrixMass/>";
    Mass                       = "<Mass>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | massClass} )?
                                 "</Mass>"
                               | "<Mass/>";
    SphereMass                 = "<SphereMass>"
                                 ?( [translationClass] [rotationClass]
                                    {setClass | massClass} )?
                                 "</SphereMass>"
                               | "<SphereMass/>";
    
    Material                   = "<Material>" "</Material>"
                               | "<Material/>";
    
    ServoMotor                 = "<ServoMotor>" "</ServoMotor>"
                               | "<ServoMotor/>";
    VelocityMotor              = "<VelocityMotor>" "</VelocityMotor>"
                               | "<VelocityMotor/>";
    
    Normals                    = "<Normals>" Normals Definition "</Normals>"
                               | "<Normals/>";
    
    Quads                      = "<Quads>" Quads Definition "</Quads>"
                               | "<Quads/>";
    Triangles                  = "<Triangles>" Triangles Definition "</Triangles>"
                               | "<Triangles/>";
    
    Rotation                   = "<Rotation>" "</Rotation>"
                               | "<Rotation/>";
    
    Scene                      = "<Scene>"
                                 ?( {setClass | bodyClass | compoundClass
                                    | lightClass | userInputClass} )?
                                 "</Scene>"
                               | "<Scene/>";
    
    Set                        = "<Set>" "</Set>"
                               | "<Set/>";
    
    ShadowMap                  = "<ShadowMap>" "</ShadowMap>"
                               | "<ShadowMap/>";
    
    Surface                    = "<Surface>" "</Surface>"
                               | "<Surface/>";
    
    TexCoords                  = "<TexCoords>" TexCoords Definition "</TexCoords>"
                               | "<TexCoords/>";
    
    Translation                = "<Translation>" "</Translation>"
                               | "<Translation/>";
    
    UserInput                  = "<UserInput>" "</UserInput>"
                               | "<UserInput/>";
    
    Vertices                   = "<Vertices>" Vertices Definition "</Vertices>"
                               | "<Vertices/>";


## Attributes

### appearanceClass

  - `Appearance`: Specifies an appearance (the visual shape of an object).
      - `name`: The name of the appearance.
          - **Use**: optional
          - **Range**: String
  - `BoxAppearance`: Specifies a box-shaped appearance.
      - `name`: The name of the appearance.
          - **Use**: optional
          - **Range**: String
      - `width`: The width of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `depth`: The depth of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `CapsuleAppearance`: Specifies a capsule-shaped appearance.
      - `name`: The name of the appearance.
          - **Use**: optional
          - **Range**: String
      - `height`: The height of the capsule.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `radius`: The radius of the capsule.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `ComplexAppearance`: Specifies an appearance with an arbitrary mesh.
      - `name`: The name of the appearance.
          - **Use**: optional
          - **Range**: String
  - `CylinderAppearance`: Specifies a cylinder-shaped appearance.
      - `name`: The name of the appearance.
          - **Use**: optional
          - **Range**: String
      - `height`: The height of the cylinder.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `radius`: The radius of the cylinder.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `SphereAppearance`: Specifies a sphere-shaped appearance.
      - `name`: The name of the appearance.
          - **Use**: optional
          - **Range**: String
      - `radius`: The radius of the sphere.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]


### axisClass

  - `Axis`: Specifies the axis of a joint.
      - `x`: The x direction of the axis.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `y`: The y direction of the axis.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `z`: The z direction of the axis.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]


### bodyClass

  - `Body`: Specifies an object that has a mass and can move.
      - `name`: The name of the body.
          - **Use**: optional
          - **Range**: String


### compoundClass

  - `Compound`: Specifies an object that cannot move.
      - `name`: The name of the compound.
          - **Use**: optional
          - **Range**: String


### deflectionClass

  - `Deflection`: Specifies the maximum and minimum deflection of a joint.
      - `min`: The minimal deflection.
          - **Units**: Units matching the type of the joint.
          - **Use**: required
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `max`: The maximal deflection.
          - **Units**: Units matching the type of the joint.
          - **Use**: required
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `init`: The initial deflection.
          - **Units**: Units matching the type of the joint.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]


### extSensorClass

  - `Camera`: Instantiates a color image camera.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String
      - `imageWidth`: The width of the camera image.
          - **Use**: required
          - **Range**: (0, MAXINTEGER]
      - `imageHeight`: The height of the camera image.
          - **Use**: required
          - **Range**: (0, MAXINTEGER]
      - `angleX`: The horizontal opening angle.
          - **Units**: degree, radian
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `angleY`: The vertical opening angle.
          - **Units**: degree, radian
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `DepthImageSensor`: Instantiates a depth image camera.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String
      - `imageWidth`: The width of the image.
          - **Use**: required
          - **Range**: (0, MAXINTEGER]
      - `imageHeight`: The height of the image.
          - **Default**: 1
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `angleX`: The horizontal opening angle.
          - **Units**: degree, radian
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `angleY`: The vertical opening angle.
          - **Units**: degree, radian
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `min`: The minimum distance this sensor can measure.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `max`: The maximum distance this sensor can measure.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 999999
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `projection`: The kind of projection. A spherical projection is only supported for 1-D sensors (`imageHeight` must be 1).
          - **Default**: perspective
          - **Use**: optional
          - **Range**: perspective, spherical
  - `ObjectSegmentedImageSensor`: Instantiates a camera which renders an objected segmented image.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String
      - `imageWidth`: The width of the camera image.
          - **Use**: required
          - **Range**: (0, MAXINTEGER]
      - `imageHeight`: The height of the camera image.
          - **Use**: required
          - **Range**: (0, MAXINTEGER]
      - `angleX`: The horizontal opening angle.
          - **Units**: degree, radian
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `angleY`: The vertical opening angle.
          - **Units**: degree, radian
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `SingleDistanceSensor`: Instantiates a sensor that measures a distance on a single ray.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String
      - `min`: The minimum distance this sensor can measure.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `max`: The maximum distance this sensor can measure.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 999999
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]


### geometryClass

  - `BoxGeometry`: Specifies a box-shaped geometry.
      - `color`: An RGBA color definition, see [this section](#color-specification).
          - **Default**: #ccccccff
          - **Use**: optional
      - `name`: The name of the geometry.
          - **Use**: optional
          - **Range**: String
      - `width`: The width of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `depth`: The depth of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `CapsuleGeometry`: Specifies a capsule-shaped geometry.
      - `color`: An RGBA color definition, see [this section](#color-specification).
          - **Default**: #ccccccff
          - **Use**: optional
      - `name`: The name of the geometry.
          - **Use**: optional
          - **Range**: String
      - `radius`: The radius of the capsule.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the capsule.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `CylinderGeometry`: Specifies a cylinder-shaped geometry.
      - `color`: An RGBA color definition, see [this section](#color-specification).
          - **Default**: #ccccccff
          - **Use**: optional
      - `name`: The name of the geometry.
          - **Use**: optional
          - **Range**: String
      - `radius`: The radius of the cylinder.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the cylinder.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `Geometry`: Specifies a geometry (the physical shape of an object).
      - `name`: The name of the geometry.
          - **Use**: optional
          - **Range**: String
  - `SphereGeometry`: Specifies a sphere-shaped geometry.
      - `color`: An RGBA color definition, see [this section](#color-specification).
          - **Default**: #ccccccff
          - **Use**: optional
      - `name`: The name of the geometry.
          - **Use**: optional
          - **Range**: String
      - `radius`: The radius of the sphere.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]


### infrastructureClass

  - `Include`: Includes another scene description file.
      - `href`: The path to the included file.
          - **Use**: required
          - **Range**: String
  - `Simulation`: Has to be the outermost element of every file.


### intSensorClass

  - `Accelerometer`: Instantiates an accelerometer on a body.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String
  - `CollisionSensor`: Instantiates a collision sensor on a body which uses geometries to detect collisions with other objects.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String
  - `Gyroscope`: Instantiates a gyroscope on a body.
      - `name`: The name of the sensor.
          - **Use**: optional
          - **Range**: String


### jointClass

  - `Hinge`: Defines a hinge joint. Requires an axis element to specify the axis and a body element which defines the body this hinge is connected to.
      - `name`: The name of the joint.
          - **Use**: optional
          - **Range**: String
  - `Slider`: Defines a slider joint. Requires an axis element to specify the axis and a body element which defines the body this slider is connected to.
      - `name`: The name of the joint.
          - **Use**: optional
          - **Range**: String


### lightClass

  - `DirLight`: Specifies a directional light source.
      - `color`: RGB color definition, see [this section](#color-specification).
          - **Default**: #ffffff
          - **Use**: optional
      - `intensity`: The intensity of the light source. This correlates with the amount of light that it emits, but has no physical unit.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `x`: The x direction of the light.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-1, 1]
      - `y`: The y direction of the light.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-1, 1]
      - `z`: The z direction of the light.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [-1, 1]
  - `PointLight`: Specifies a point light source.
      - `color`: RGB color definition, see [this section](#color-specification).
          - **Default**: #ffffff
          - **Use**: optional
      - `intensity`: The intensity of the light source. This correlates with the amount of light that it emits, but has no physical unit.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `x`: The x coordinate of the light source.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `y`: The y coordinate of the light source.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `z`: The z coordinate of the light source.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `constantAttenuation`: The constant attenuation of the light.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `linearAttenuation`: The linear attenuation of the light.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `quadraticAttenuation`: The quadratic attenuation of the light.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
  - `SpotLight`: Specifies a spot light source.
      - `color`: RGB color definition, see [this section](#color-specification).
          - **Default**: #ffffff
          - **Use**: optional
      - `intensity`: The intensity of the light source. This correlates with the amount of light that it emits, but has no physical unit.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `x`: The x coordinate of the light source.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `y`: The y coordinate of the light source.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `z`: The z coordinate of the light source.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `constantAttenuation`: The constant attenuation of the light.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `linearAttenuation`: The linear attenuation of the light.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `quadraticAttenuation`: The quadratic attenuation of the light.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `dirX`: The x direction of the light spot.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-1, 1]
      - `dirY`: The y direction of the light spot.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-1, 1]
      - `dirZ`: The z direction of the light spot.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [-1, 1]
      - `cutoff`: The cosine of the opening angle of the light spot cone.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, 1]


### massClass

  - `BoxMass`: Specifies a box-shaped mass.
      - `name`: The name of the mass.
          - **Use**: optional
          - **Range**: String
      - `value`: The mass of the box.
          - **Units**: g, kg
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `width`: The width of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `depth`: The depth of the box.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `CapsuleMass`: Specifies a capsule-shaped mass.
      - `name`: The name of the mass.
          - **Use**: optional
          - **Range**: String
      - `value`: The mass of the capsule.
          - **Units**: g, kg
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `radius`: The radius of the capsule.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the capsule.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `CylinderMass`: Specifies a cylinder-shaped mass.
      - `name`: The name of the mass.
          - **Use**: optional
          - **Range**: String
      - `value`: The mass of the cylinder.
          - **Units**: g, kg
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `radius`: The radius of the cylinder.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `height`: The height of the cylinder.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `InertiaMatrixMass`: Specifies a mass with a given inertia matrix (which is symmetric and positive-definite).
      - `name`: The name of the mass.
          - **Use**: optional
          - **Range**: String
      - `value`: The total mass.
          - **Units**: g, kg
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `x`: The x coordinate of the center of mass.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `y`: The x coordinate of the center of mass.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `z`: The x coordinate of the center of mass.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `ixx`: Moment of inertia around the x-axis when the object is rotated around the x-axis.
          - **Units**: g⋅mm², kg⋅m²
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `ixy`: Moment of inertia around the y-axis when the object is rotated around the x-axis.
          - **Units**: g⋅mm², kg⋅m²
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `ixz`: Moment of inertia around the z-axis when the object is rotated around the x-axis.
          - **Units**: g⋅mm², kg⋅m²
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `iyy`: Moment of inertia around the y-axis when the object is rotated around the y-axis.
          - **Units**: g⋅mm², kg⋅m²
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `iyz`: Moment of inertia around the z-axis when the object is rotated around the y-axis
          - **Units**: g⋅mm², kg⋅m²
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `izz`: Moment of inertia around the z-axis when the object is rotated around the z-axis.
          - **Units**: g⋅mm², kg⋅m²
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
  - `Mass`: Specifies a mass without a mass of its own. It is a container for other masses, which can be moved and rotated as a whole with `Translation` and `Rotation`. The mass of a `Body` is the sum of all masses (including nested ones), the center of mass and inertia are combined accordingly.
      - `name`: The name of the mass.
          - **Use**: optional
          - **Range**: String
  - `SphereMass`: Specifies a sphere-shaped mass.
      - `name`: The name of the mass.
          - **Use**: optional
          - **Range**: String
      - `value`: The mass of the sphere.
          - **Units**: g, kg
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `radius`: The radius of the sphere.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]


### materialClass

  - `Material`: Specifies the physical properties of a material.
      - `friction`: Specifies the friction of a material.
          - **Use**: optional
          - **Default**: 0
          - **Range**: [0, MAXFLOAT]
      - `rollingFriction`: Specifies the rolling friction of a material.
          - **Use**: optional
          - **Default**: 0
          - **Range**: [0, MAXFLOAT]


### motorClass

  - `ServoMotor`: Instantiates a position-controlled servo motor.
      - `maxVelocity`: The maximum velocity of this motor.
          - **Units**: radian/s, degree/s (hinge); mm/s, cm/s, dm/s, m/s, km/s, km/h (slider)
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `maxForce`: The maximum torque (hinge) or force (slider) of this motor.
          - **Units**: N⋅m, N⋅mm (hinge); N (slider)
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `armature`: The inertia (hinge) or mass (slider) that is added to the joint to account for the rotor.
          - **Default**: 0.01
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `p`: The p value of the motor's pid interface.
          - **Use**: required
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `i`: The i value of the motor's pid interface.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `d`: The d value of the motor's pid interface.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `delay`: The delay of the commands being executed.
          - **Units**: s
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `velocityLowPassFactor`: The factor of the low pass filter that is applied to the measured velocity (1 means no filtering).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, 1]
  - `VelocityMotor`: Instantiates a velocity-controlled motor.
      - `maxVelocity`: The maximum velocity of this motor.
          - **Units**: radian/s, degree/s (hinge); mm/s, cm/s, dm/s, m/s, km/s, km/h (slider)
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `maxForce`: The maximum torque (hinge) or force (slider) of this motor.
          - **Units**: N⋅m, N⋅mm (hinge); N (slider)
          - **Use**: required
          - **Range**: (0, MAXFLOAT]


### normalsClass

  - `Normals`: Specifies the normals of a `ComplexAppearance`. The content is a space separated list of floats, three per normal (x, y, z). Lines starting with `#` are comments. If no normals are given, they are calculated from the faces.


### primitiveGroupClass

  - `Triangles`: Specifies triangles of a `ComplexAppearance`. The content is a space separated list of non-negative integers (lines starting with `#` are comments). Each triangle consists of three vertex indices into the `Vertices` element, or, if `Normals` are present, of three pairs (vertex index, normal index).
  - `Quads`: Specifies quadrilaterals of a `ComplexAppearance`. It is specified like `Triangles`, but with four vertex indices (or pairs) per quad.


### rotationClass

  - `Rotation`: Specifies the rotation of an object relative to its parent. The resulting rotation matrix is R<sub>z</sub> · R<sub>y</sub> · R<sub>x</sub>.
      - `x`: Rotation around the x-axis.
          - **Units**: degree, radian
          - **Default**: 0degree
          - **Use**: optional
      - `y`: Rotation around the y-axis.
          - **Units**: degree, radian
          - **Default**: 0degree
          - **Use**: optional
      - `z`: Rotation around the z-axis.
          - **Units**: degree, radian
          - **Default**: 0degree
          - **Use**: optional


### sceneClass

  - `Scene`: Describes a scene and specifies the controller of the simulation.
      - `name`: The identifier of the scene object.
          - **Use**: optional
          - **Range**: String
      - `controller`: The name of the controller library (without platform-specific prefixes/suffixes such as *lib*, *.so*, *.dylib*, *.dll*).
          - **Use**: optional
          - **Range**: String
      - `color`: The RGBA background color of the scene, see [this section](#color-specification).
          - **Default**: #000000ff
          - **Use**: optional
      - `stepLength`: The duration of each simulation step.
          - **Units**: s
          - **Default**: 0.01s
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `gravity`: Sets the gravity in this scene.
          - **Units**: $\frac{\textrm{mm}}{\textrm{s}^2}$, $\frac{\textrm{m}}{\textrm{s}^2}$
          - **Default**: -9.80665 $\frac{\textrm{m}}{\textrm{s}^2}$
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `bodyCollisions`: Whether collisions between different bodies should be detected.
          - **Default**: true
          - **Use**: optional
          - **Range**: true, false


### setClass

  - `Set`: Sets a placeholder referenced by the attribute `name` to the value specified by the attribute `value`.
      - `name`: The name of a placeholder.
          - **Use**: required
          - **Range**: String
      - `value`: The value the placeholder is set to.
          - **Use**: required
          - **Range**: String


### shadowMapClass

  - `ShadowMap`: Makes the light it is a child of cast shadows. Without this element, a light does not cast shadows and costs nothing. A shadow map is rendered from the light and only covers an axis-aligned bounding box in world coordinates; only objects inside it cast and receive shadows. Shadow maps are only used with lighting enabled. At most 8 lights can have a shadow map.
      - Directional lights use an orthographic projection that fits the bounding box.
      - Point and spot lights use a single perspective projection that looks from the light at the center of the bounding box. The light must be outside of the bounding box (otherwise it casts no shadows), and the shadows are only cast within the frustum that is spanned by the bounding box (they are not cast in all directions of a point light).
      - `width`: The width of the shadow map in pixels.
          - **Default**: 2048
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `height`: The height of the shadow map in pixels.
          - **Default**: 2048
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `aabbCenterX`: The x coordinate of the center of the bounding box.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `aabbCenterY`: The y coordinate of the center of the bounding box.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `aabbCenterZ`: The z coordinate of the center of the bounding box.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `aabbExtentX`: The size of the bounding box in x direction.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `aabbExtentY`: The size of the bounding box in y direction.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]
      - `aabbExtentZ`: The size of the bounding box in z direction.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
          - **Range**: (0, MAXFLOAT]


### surfaceClass

  - `Surface`: Specifies the visual properties of a material.
      - `albedo`: The RGB albedo, see [this section](#color-specification).
          - **Use**: required
      - `alpha`: The alpha value, from 0 (transparent) to 1 (opaque).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, 1]
      - `metallic`: Whether the surface is metallic, from 0 (dielectric, scattering) to 1 (metallic, no scattering).
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, 1]
      - `roughness`: The roughness of the surface, from 0 (perfect mirror) to 1 (maximum scattering).
          - **Default**: 0.5
          - **Use**: optional
          - **Range**: [0, 1]
      - `ambient`: The ambient occlusion factor from 0 (no ambient lighting) to 1 (full ambient lighting).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, 1]
      - `texture`: The path to a texture that modulates the albedo / alpha values.
          - **Use**: optional
          - **Range**: String


### texCoordsClass

  - `TexCoords`: Specifies the texture coordinates of a `ComplexAppearance`. The content is a space separated list of floats, two per vertex (u, v). Lines starting with `#` are comments. If the surface has a texture, there must be exactly one pair per vertex.


### translationClass

  - `Translation`: Specifies a translation of an object relative to its parent.
      - `x`: Translation along the x-axis.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `y`: Translation along the y-axis.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `z`: Translation along the z-axis.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]


### userInputClass

  - `UserInput`: Combines an actuator and a sensor. The values set for the actuator are directly returned by the sensor. Using the actuator view, this allows to feed user input to the controller.
      - `name`: The name of the user input.
          - **Use**: optional
          - **Range**: String
      - `type`: The kind of data for which user input is provided. The units of `min`, `max` and `default` depend on it: degree, radian (angle); degree/s, radian/s (angularVelocity); mm, cm, dm, m, km (length); mm/s, cm/s, dm/s, m/s, km/s, km/h (velocity); mm/s^2, m/s^2 (acceleration).
          - **Default**: length
          - **Use**: optional
          - **Range**: angle, angularVelocity, length, velocity, acceleration
      - `min`: The minimum value that can be set.
          - **Units**: Units matching the type (see `type`)
          - **Use**: required
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `max`: The maximum value that can be set.
          - **Units**: Units matching the type (see `type`)
          - **Use**: required
          - **Range**: [-MAXFLOAT, MAXFLOAT]
      - `default`: The value returned by the sensor if no value is set in the actuator view.
          - **Units**: Units matching the type (see `type`)
          - **Default**: 0
          - **Use**: optional
          - **Range**: [-MAXFLOAT, MAXFLOAT]


### verticesClass

  - `Vertices`: Specifies the vertices of a `ComplexAppearance`. The content is a space separated list of floats, three per vertex (x, y, z). Lines starting with `#` are comments.
      - `unit`: The unit of the vertex coordinates.
          - **Units**: mm, cm, dm, m, km
          - **Default**: m
          - **Use**: optional


## Color Specification

There are two ways of specifying a color for a color-attribute.

  - **HTML-Style**: To specify a color in html-style the first character of the color value has to be a `#` followed by hexadecimal values for red, green, blue (and an optional fourth value for the alpha-channel). These values can be one-digit or two-digits, but not mixed.
      - `#rgb` e.g. `#f00`
      - `#rgba` e.g. `#0f08`
      - `#rrggbb` e.g. `#f80011`
      - `#rrggbbaa` e.g. `#1038bc80`
  - **CSS-Style**: A css color starts with`rgb` (or `rgba`) followed by the values for red, green, and blue put into parentheses and separated by commas. The values for *r*, *g*, and *b* have to be between 0 and 255 or between 0% and 100%, the *a* value has to be between 0 and 1.
      - `rgb(r, g, b)` e.g. `rgb(255, 128, 0)`
      - `rgba(r, g, b, a)` e.g. `rgba(0%, 50%, 75%, 0.75)`

Some attributes (e.g. the color of lights or the `albedo` of a surface) only accept colors without an alpha channel. Specifying an alpha value there is an error.
