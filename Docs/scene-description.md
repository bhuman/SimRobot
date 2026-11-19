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
    backdropClass              = Backdrop;
    bandClass                  = Band;
    bodyClass                  = Body;
    compoundClass              = Compound;
    deflectionClass            = Deflection;
    environmentClass           = Environment;
    groundClass                = Ground;
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
                               | SpotLight
                               | Sky;
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
    
    Sky                        = "<Sky>"
                                 ?( [shadowMapClass] )?
                                 "</Sky>"
                               | "<Sky/>";

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
    
    Backdrop                   = "<Backdrop>"
                                 ?( {bandClass} )?
                                 "</Backdrop>"
                               | "<Backdrop/>";

    Band                       = "<Band>" "</Band>"
                               | "<Band/>";

    Ground                     = "<Ground>" "</Ground>"
                               | "<Ground/>";

    Environment                = "<Environment>"
                                 ?( [backdropClass] [groundClass] {lightClass} )?
                                 "</Environment>"
                               | "<Environment/>";

    Scene                      = "<Scene>"
                                 ?( [environmentClass]
                                    {setClass | bodyClass | compoundClass
                                    | userInputClass} )?
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
      - `format`: The format of the images: three bytes per pixel (red, green, blue) or four bytes per two pixels (Y of the first pixel, U, Y of the second pixel, V). `yuyv` requires an even `imageWidth`.
          - **Default**: rgb
          - **Use**: optional
          - **Range**: rgb, yuyv
      - `multisample`: Whether the image is rendered with multisampling (four samples per pixel), which smooths the edges like the optics of a real camera does. It costs rendering time, most with OpenGL.
          - **Default**: false
          - **Use**: optional
          - **Range**: true, false
      - `blur`: The standard deviation of a Gaussian blur in pixels, which models the limited sharpness of the lens.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, 1.5]
      - `gain`: A factor for all color channels, which models the exposure of the camera. The color values are those of the tone mapped image, which the camera does not adapt to the brightness of the scene.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `redGain`: An additional factor for the red channel (white balance).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `blueGain`: An additional factor for the blue channel (white balance).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `saturation`: The saturation of the colors (0 is gray, 1 is unchanged).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `vignette`: How much darker the corners of the image are than the center (the loss grows with the squared distance from the center).
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, 1]
      - `noise`: The standard deviation of the noise of medium bright pixels as a fraction of the full range. The noise is different in each image, independent per pixel, stronger in bright pixels, and partly the same in all channels.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]

    The attributes from `blur` on are applied by the GPU when the image is converted into the format of the sensor, in this order: blur, gain, saturation, vignette, noise. The defaults are those of an ideal camera, which does not change the rendered image.
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


### environmentClass

  - `Environment`: Sets the background color, the ambient light and the tone mapping of the scene, and contains lights. It is a child of the `Scene`. Since it can have a `name` and be used with `ref` like other elements, an environment with its lights can be defined once and shared by scenes. A `Sky` replaces the ambient light and the exposure by ones that depend on the time of day.
      - `color`: The RGBA background color of the scene, see [this section](#color-specification).
          - **Default**: #000000ff
          - **Use**: optional
      - `ambient`: The RGB color of the ambient light that comes from above, see [this section](#color-specification). The light that every surface receives from all directions, in addition to the lights of the scene.
          - **Default**: rgb(40%, 40%, 40%)
          - **Use**: optional
      - `ambientGround`: The RGB color of the ambient light that comes from below (e.g. light that is reflected by the ground). The ambient light of a surface is interpolated between this and `ambient` by the direction of its normal.
          - **Default**: the value of `ambient`
          - **Use**: optional
      - `ambientIntensity`: A factor for `ambient` and `ambientGround` .
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `exposure`: A factor that the brightness of lit surfaces is multiplied with before tone mapping.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `toneMapping`: How the brightness of lit surfaces is mapped to displayable colors: `reinhard` (the default) or `filmic` (an S-shaped curve with more contrast).
          - **Default**: reinhard
          - **Use**: optional
          - **Range**: reinhard, filmic

  - `Backdrop`: A room around the scene, which is drawn in the background (in the views and in the images of cameras, only if lighting is enabled) and which lights the scene (image-based lighting): surfaces receive diffuse light from it, and glossy surfaces reflect it, so metals and plastics do not look black. It consists of a floor (at the height 0), walls, a ceiling with a grid of lamps, and `Band` children that change the color of a part of the walls (for example crowds, stands or banners). The room is centered at the origin of the x and y axes. The radiance is computed when the scene is loaded; reflections are as if the room was infinitely far away. A backdrop replaces the ambient light of the environment (`ambient` and `ambientGround`) and cannot be combined with a `Sky`. It is a child of the `Environment`; the lights of the environment still cast their light (and shadows), the backdrop adds the light of the room without shadows.
      - `sizeX`: The size of the room in x direction.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 40m
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `sizeY`: The size of the room in y direction.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 30m
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `sizeZ`: The height of the room.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 10m
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `floorColor`: The color of the floor, see [this section](#color-specification). The floor lights the scene from below (for example with the green of the turf).
          - **Default**: rgb(50%, 50%, 50%)
          - **Use**: optional
      - `wallColor`: The color of the walls.
          - **Default**: rgb(70%, 70%, 70%)
          - **Use**: optional
      - `ceilingColor`: The color of the ceiling.
          - **Default**: rgb(60%, 60%, 60%)
          - **Use**: optional
      - `intensity`: A factor for the brightness of the floor, the walls, the ceiling and the bands.
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `lampColor`: The color of the lamps in the ceiling.
          - **Default**: #ffffff
          - **Use**: optional
      - `lampIntensity`: The brightness of the lamps (they are much brighter than the other parts of the room). 0 means that there are no lamps.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `lampRows`: The number of rows of lamps (arranged in the direction of the y axis).
          - **Default**: 3
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `lampColumns`: The number of columns of lamps (arranged in the direction of the x axis).
          - **Default**: 5
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `lampSize`: The side length of the (square) lamps.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 1m
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `resolution`: The size of the sides of the cube map of the room (a power of two from 32 to 2048). Larger values make the background sharper and take longer to compute when the scene is loaded.
          - **Default**: 256
          - **Use**: optional
          - **Range**: 32, 64, 128, 256, 512, 1024, 2048
  - `Ground`: An infinite horizontal plane with a tile pattern, which is drawn in the background below the horizon (the ground that is seen beyond the geometry of the scene), optionally with a line of trees at the horizon. It is lit like a horizontal surface (by the sky or by the backdrop and the directional lights) but without shadows, and it fades into the haze at the horizon. Together with a `Backdrop`, it replaces the floor of the room wherever the floor is seen (the floor of the room is still what lights the scene), and the walls and the ceiling of the room are seen from the position of the viewer instead of from the center of the room. A ground needs a `Sky` or a `Backdrop` (it is part of the background), and can be a child of the `Environment` once.
      - `color`: The color of the tiles.
          - **Default**: rgb(50%, 50%, 50%)
          - **Use**: optional
      - `height`: The height of the plane.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
      - `tileSize`: The side length of the square tiles. The joints between them are darker. 0 means that there are no tiles.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 1m
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `jointWidth`: The width of the joints between the tiles.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 1cm
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `treeDistance`: The distance of a line of trees from the origin (the camera must be inside of the circle). It is a wall of foliage with crowns of different height around the scene, which is only used with a `Sky` (a backdrop has walls). With a `Sky`, the trees follow the season of the date and the latitude: green in the summer, yellow to red in the autumn, bare in the winter (not in the tropics, where they stay green). 0 means that there are no trees.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
      - `treeHeight`: The mean height of the trees.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 8m
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]

  - `Band`: A horizontal band on the walls of a `Backdrop`. Bands that come later cover earlier ones.
      - `minHeight`: The height of the lower edge of the band.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0m
          - **Use**: optional
      - `maxHeight`: The height of the upper edge of the band.
          - **Units**: mm, cm, dm, m, km
          - **Use**: required
      - `color`: The color of the band.
          - **Use**: required
      - `variation`: How much the brightness changes over the band (noise, for seats, crowds or banners): 0 means not at all, 1 means from black to twice as bright.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, 1]
      - `scale`: The size of the changes of the brightness.
          - **Units**: mm, cm, dm, m, km
          - **Default**: 0.5m
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]


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
  - `Sky`: Specifies a sky with the sun, the moon and the stars, whose position depends on the time of day (and the place and date). It is drawn behind everything else, in the views and in the images of cameras (only if lighting is enabled), and it is also a directional light, namely the sunlight by day and the moonlight at night. This light, the ambient light (blue by day, red at sunrise and sunset, dark at night) and the exposure (which is longer at night, like an eye or camera adapts) follow the sky, so the attributes `ambient`, `ambientGround` and `exposure` of the `Environment` are replaced (the `exposure` becomes the one of the day). An environment can have one sky. A `ShadowMap` child makes the light of the sky cast shadows (the direction of the shadow map follows the light). The sky has polar lights at night if the latitude is high. Stars, moon phase and polar lights are drawn procedurally.
      - `clock`: Where the time comes from. `simulation`: The time is the one that is given by `date` and `time`, plus the time of the simulation multiplied with `timeScale`. `wall`: The time is the one of the clock of the computer (`date`, `time` and `timeScale` are ignored).
          - **Default**: simulation
          - **Use**: optional
          - **Range**: simulation, wall
      - `date`: The date at the start (as `year-month-day`).
          - **Default**: 2026-06-21
          - **Use**: optional
      - `time`: The local mean solar time at the start (as `hours:minutes` or `hours:minutes:seconds`, where 12:00 is when the sun is highest). This is not the time of the time zone, but the one of the longitude.
          - **Default**: 17:00
          - **Use**: optional
      - `timeScale`: How many seconds pass in the sky for each second of the simulation. 0 freezes the time, 1 is real time, and 2160 lets a day pass in 40 seconds.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `latitude`: The latitude of the place in degrees (north is positive). It determines how high the sun climbs, how long the days are and if there are polar lights.
          - **Default**: 53.08 (Bremen)
          - **Use**: optional
          - **Units**: degree, radian
          - **Range**: [-90, 90]
      - `longitude`: The longitude of the place in degrees (east is positive).
          - **Default**: 8.8 (Bremen)
          - **Use**: optional
          - **Units**: degree, radian
          - **Range**: [-180, 180]
      - `north`: The direction of the geographic north in the scene as an angle in degrees around the z axis (counter-clockwise, starting at the x axis). The default means that the north is in the direction of the y axis (and the east in the direction of the x axis).
          - **Default**: 90
          - **Use**: optional
          - **Units**: degree, radian
          - **Range**: [-360, 360]
      - `intensity`: The intensity of the sunlight when the sun is high (in the same unit as the intensity of the other lights). The moonlight is much weaker.
          - **Default**: 5.5
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `nightExposure`: How much longer the scene is exposed at night than by day.
          - **Default**: 6
          - **Use**: optional
          - **Range**: [0, MAXFLOAT]
      - `moonRadius`: The angular radius of the moon in degrees. The real one is 0.26 degrees, but it looks larger than it is in photos, and a small one would hardly be seen.
          - **Default**: 1.5
          - **Use**: optional
          - **Units**: degree, radian
          - **Range**: [0, 180]
      - `aurora`: How visible the polar lights are at night, from 0 to 1. By default, they are visible for a latitude of more than 55 degrees (north or south) and as bright as possible above 67 degrees.
          - **Use**: optional
          - **Range**: [0, 1]
      - `clouds`: How much of the sky is covered by clouds, from 0 (none) to 1. The clouds drift slowly, they are lit by the sun (red at sunrise and sunset) and hide the sun, the moon and the stars. From about 0.5 they close up, and at 1 the sky is a gray, dark layer of clouds without a visible sun, which also lets only a small part of the sunlight through (the light of the scene is weaker, softer and the ambient light grayer).
          - **Default**: 0.35
          - **Use**: optional
          - **Range**: [0, 1]


### shadowMapClass

  - `ShadowMap`: Makes the light it is a child of cast shadows. Without this element, a light does not cast shadows and costs nothing. A shadow map is rendered from the light and only covers an axis-aligned bounding box in world coordinates; only objects inside it cast and receive shadows. The bounding box is given by `aabbExtentX`, `aabbExtentY` and `aabbExtentZ` (and a center). If none of them is given, the box is fitted to the scene when the shadow maps are rendered the first time: it contains everything that is drawn at that time, except for horizontal surfaces like floors (unless there is nothing else), plus a margin of 0.75 m around it and a height of at least 2.5 m. The smaller the box, the better is the resolution of the shadows, so this is better than a fixed box for scenes of different sizes (objects that are later outside of the box do not cast shadows). Shadow maps are only used with lighting enabled. At most 8 lights can have a shadow map.
      - The shadow map is rotated around the direction of the light such that the part of it that is needed is as small as possible.
      - Directional lights use an orthographic projection that fits the bounding box.
      - Point and spot lights use a single perspective projection, so shadows are only cast within its frustum (they are not cast in all directions around the light).
        - A spot light looks along its direction, and its frustum is limited by its cone (nothing outside of it is lit anyway) and the bounding box. The spot light may be inside of the bounding box. A spot light with a `cutoff` of (almost) 0 is treated like a point light.
        - A point light looks in the direction of (approximately) the smallest cone around the bounding box. It must be outside of the bounding box (otherwise it casts no shadows). Parts of the bounding box that are more than 80 degrees away from that direction are not covered.
      - `width`: The width of the shadow map in pixels.
          - **Default**: 2048
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `height`: The height of the shadow map in pixels.
          - **Default**: 2048
          - **Use**: optional
          - **Range**: (0, MAXINTEGER]
      - `aabbCenterX`: The x coordinate of the center of the bounding box (only together with the extents).
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
      - `aabbExtentX`: The size of the bounding box in x direction. Either all three extents are given or none of them (then the box is fitted to the scene).
          - **Units**: mm, cm, dm, m, km
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `aabbExtentY`: The size of the bounding box in y direction. Either all three extents are given or none of them (then the box is fitted to the scene).
          - **Units**: mm, cm, dm, m, km
          - **Use**: optional
          - **Range**: (0, MAXFLOAT]
      - `aabbExtentZ`: The size of the bounding box in z direction. Either all three extents are given or none of them (then the box is fitted to the scene).
          - **Units**: mm, cm, dm, m, km
          - **Use**: optional
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
      - `textureVariation`: How much the repetition of the texture is hidden, from 0 (the texture repeats as it is) to 1. A second layer of the texture with another scale and orientation is blended in, and large patches of the surface become slightly brighter or darker. This is meant for textures of natural materials like turf; it costs one more texture lookup per pixel.
          - **Default**: 0
          - **Use**: optional
          - **Range**: [0, 1]
      - `sheen`: The color of the sheen: fibers (like those of cloth or turf) look brighter at grazing angles. The sheen is added to the light (a lobe of the lights as in the glTF extension `KHR_materials_sheen`, and a term of the ambient light that grows towards the silhouette), so it depends on the lights and their shadows. Black means no sheen.
          - **Default**: black
          - **Use**: optional
          - **Range**: Color
      - `sheenRoughness`: The width of the lobe of the sheen, from 0 (only at the most grazing angles) to 1 (wide).
          - **Default**: 0.5
          - **Use**: optional
          - **Range**: [0, 1]
      - `normalMap`: The path to a normal map (tangent space, the green channel points to the top of the texture). It bends the normal that is used for lighting. The tangent frame is computed from the texture coordinates, so it only works for meshes with texture coordinates (not for meshes without them, like most CAD meshes). With `textureVariation`, a second, rotated layer is added like with the texture. It costs a texture lookup per pixel (two with `textureVariation`).
          - **Use**: optional
          - **Range**: String
      - `normalScale`: How strong the normal map bends the normal (0: not at all, 1: as in the map).
          - **Default**: 1
          - **Use**: optional
          - **Range**: [0, 4]
      - `roughnessMap`: The path to a texture whose red channel is multiplied with `roughness` (also only for meshes with texture coordinates).
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
