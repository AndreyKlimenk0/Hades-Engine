import bpy
import math
import os
import struct
import zlib
from mathutils import Vector


# ============================================================
# CONFIGURATION
# ============================================================

OUTPUT_DIR = os.path.join(
    os.path.expanduser("~"),
    "point_light_test"
)

os.makedirs(
    OUTPUT_DIR,
    exist_ok=True
)

GLB_PATH = os.path.join(
    OUTPUT_DIR,
    "point_light_normal_test.glb"
)

BLEND_PATH = os.path.join(
    OUTPUT_DIR,
    "point_light_normal_test.blend"
)

NORMAL_TEXTURE_PATH = os.path.join(
    OUTPUT_DIR,
    "Test_Normal.png"
)

NORMAL_TEXTURE_SIZE = 512


# ============================================================
# CLEAR SCENE
# ============================================================

def clear_scene():

    bpy.ops.object.select_all(
        action="SELECT"
    )

    bpy.ops.object.delete(
        use_global=False
    )

    # Remove unused resources.
    collections = [
        bpy.data.meshes,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,
    ]

    for collection in collections:

        for block in list(collection):

            if block.users == 0:
                collection.remove(block)


# ============================================================
# SIMPLE PNG WRITER
#
# This avoids problems with Blender-generated images.
# We create an actual PNG on disk.
# ============================================================

def make_png_chunk(
    chunk_type,
    data
):

    chunk = chunk_type + data

    return (
        struct.pack(
            ">I",
            len(data)
        )
        +
        chunk
        +
        struct.pack(
            ">I",
            zlib.crc32(chunk) & 0xFFFFFFFF
        )
    )


def write_rgb_png(
    filepath,
    width,
    height,
    rgb_data
):

    png = bytearray(
        b"\x89PNG\r\n\x1a\n"
    )

    # --------------------------------------------------------
    # PNG header
    #
    # Bit depth: 8
    # Color type: RGB
    # --------------------------------------------------------

    header = struct.pack(
        ">IIBBBBB",
        width,
        height,
        8,      # bits per channel
        2,      # RGB
        0,      # compression
        0,      # filtering
        0       # no interlace
    )

    png.extend(
        make_png_chunk(
            b"IHDR",
            header
        )
    )

    # --------------------------------------------------------
    # Raw image rows
    # --------------------------------------------------------

    raw = bytearray()

    row_size = width * 3

    for y in range(height):

        # PNG filter = none
        raw.append(0)

        start = y * row_size
        end = start + row_size

        raw.extend(
            rgb_data[start:end]
        )

    compressed = zlib.compress(
        bytes(raw),
        9
    )

    png.extend(
        make_png_chunk(
            b"IDAT",
            compressed
        )
    )

    png.extend(
        make_png_chunk(
            b"IEND",
            b""
        )
    )

    with open(
        filepath,
        "wb"
    ) as file:

        file.write(png)


# ============================================================
# CREATE FLAT TANGENT-SPACE NORMAL MAP
#
# Tangent-space normal:
#
#       X = 0
#       Y = 0
#       Z = 1
#
# Encoded:
#
#       R = 128
#       G = 128
#       B = 255
#
#
# After decoding in HLSL:
#
#       normal = rgb * 2 - 1
#
# approximately:
#
#       (0, 0, 1)
#
#
# Then:
#
#       mul(normalTS, TBN)
#
# gives the interpolated vertex normal.
# ============================================================

def create_flat_normal_map(
    name,
    size,
    filepath
):

    print("")
    print("Creating flat normal texture...")

    # --------------------------------------------------------
    # RGB 128,128,255
    # --------------------------------------------------------

    pixel = bytes([
        128,
        128,
        255
    ])

    rgb_data = pixel * (
        size * size
    )

    write_rgb_png(
        filepath,
        size,
        size,
        rgb_data
    )

    print(
        "Normal texture written:",
        filepath
    )

    # --------------------------------------------------------
    # Remove old image if script was already executed.
    # --------------------------------------------------------

    old_image = bpy.data.images.get(
        name
    )

    if old_image is not None:

        bpy.data.images.remove(
            old_image
        )

    # --------------------------------------------------------
    # Load actual PNG file.
    # --------------------------------------------------------

    image = bpy.data.images.load(
        filepath,
        check_existing=False
    )

    image.name = name

    # IMPORTANT:
    #
    # Normal textures must NOT be sampled as sRGB.
    # --------------------------------------------------------

    image.colorspace_settings.name = (
        "Non-Color"
    )

    print(
        "Normal texture RGB = 128, 128, 255"
    )

    print(
        "Normal texture loaded into Blender."
    )

    return image


# ============================================================
# MATERIAL CREATION
# ============================================================

def create_material(
    name,
    base_color,
    metallic,
    roughness,
    normal_image,
    normal_strength=1.0
):

    material = bpy.data.materials.new(
        name=name
    )

    material.use_nodes = True

    nodes = material.node_tree.nodes
    links = material.node_tree.links

    bsdf = nodes.get(
        "Principled BSDF"
    )

    if bsdf is None:

        raise RuntimeError(
            "Principled BSDF was not found."
        )

    # ========================================================
    # BASE COLOR
    # ========================================================

    bsdf.inputs[
        "Base Color"
    ].default_value = (
        base_color[0],
        base_color[1],
        base_color[2],
        1.0
    )

    # ========================================================
    # METALLIC
    # ========================================================

    if "Metallic" in bsdf.inputs:

        bsdf.inputs[
            "Metallic"
        ].default_value = metallic

    # ========================================================
    # ROUGHNESS
    # ========================================================

    if "Roughness" in bsdf.inputs:

        bsdf.inputs[
            "Roughness"
        ].default_value = roughness

    # ========================================================
    # NORMAL TEXTURE NODE
    # ========================================================

    texture_node = nodes.new(
        "ShaderNodeTexImage"
    )

    texture_node.name = (
        "NormalTexture"
    )

    texture_node.label = (
        "Normal Texture"
    )

    texture_node.image = (
        normal_image
    )

    texture_node.image.colorspace_settings.name = (
        "Non-Color"
    )

    texture_node.interpolation = (
        "Linear"
    )

    texture_node.extension = (
        "REPEAT"
    )

    texture_node.location = (
        -500,
        -300
    )

    # ========================================================
    # NORMAL MAP NODE
    # ========================================================

    normal_node = nodes.new(
        "ShaderNodeNormalMap"
    )

    normal_node.name = (
        "NormalMap"
    )

    normal_node.label = (
        "Tangent Space Normal"
    )

    normal_node.space = (
        "TANGENT"
    )

    normal_node.inputs[
        "Strength"
    ].default_value = normal_strength

    normal_node.location = (
        -200,
        -300
    )

    # ========================================================
    # CONNECT
    #
    # Texture -> Normal Map -> Principled BSDF
    # ========================================================

    links.new(
        texture_node.outputs[
            "Color"
        ],
        normal_node.inputs[
            "Color"
        ]
    )

    links.new(
        normal_node.outputs[
            "Normal"
        ],
        bsdf.inputs[
            "Normal"
        ]
    )

    return material


# ============================================================
# ASSIGN MATERIAL
# ============================================================

def assign_material(
    obj,
    material
):

    if obj.data is None:
        return

    obj.data.materials.append(
        material
    )


# ============================================================
# APPLY SCALE
# ============================================================

def apply_scale(obj):

    bpy.ops.object.select_all(
        action="DESELECT"
    )

    obj.select_set(
        True
    )

    bpy.context.view_layer.objects.active = (
        obj
    )

    bpy.ops.object.transform_apply(
        location=False,
        rotation=False,
        scale=True
    )

    obj.select_set(
        False
    )


# ============================================================
# GENERATE UVs
# ============================================================

def smart_uv_project(obj):

    bpy.ops.object.select_all(
        action="DESELECT"
    )

    obj.select_set(
        True
    )

    bpy.context.view_layer.objects.active = (
        obj
    )

    bpy.ops.object.mode_set(
        mode="EDIT"
    )

    bpy.ops.mesh.select_all(
        action="SELECT"
    )

    bpy.ops.uv.smart_project(
        angle_limit=math.radians(
            66.0
        ),
        island_margin=0.03
    )

    bpy.ops.object.mode_set(
        mode="OBJECT"
    )

    obj.select_set(
        False
    )


# ============================================================
# CUBE
# ============================================================

def add_cube(
    name,
    location,
    scale,
    material
):

    bpy.ops.mesh.primitive_cube_add(
        location=location
    )

    obj = bpy.context.object

    obj.name = name
    obj.scale = scale

    apply_scale(
        obj
    )

    assign_material(
        obj,
        material
    )

    smart_uv_project(
        obj
    )

    return obj


# ============================================================
# SPHERE
# ============================================================

def add_sphere(
    name,
    location,
    radius,
    material,
    segments=48,
    rings=24
):

    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=segments,
        ring_count=rings,
        radius=radius,
        location=location
    )

    obj = bpy.context.object

    obj.name = name

    assign_material(
        obj,
        material
    )

    # --------------------------------------------------------
    # Smooth shading is important here.
    #
    # This gives us interpolated vertex normals.
    # --------------------------------------------------------

    for polygon in obj.data.polygons:

        polygon.use_smooth = True

    return obj


# ============================================================
# CYLINDER
# ============================================================

def add_cylinder(
    name,
    location,
    radius,
    depth,
    material,
    vertices=48
):

    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=depth,
        location=location
    )

    obj = bpy.context.object

    obj.name = name

    assign_material(
        obj,
        material
    )

    smart_uv_project(
        obj
    )

    return obj


# ============================================================
# CAMERA LOOK-AT
# ============================================================

def look_at(
    obj,
    target
):

    direction = (
        Vector(target)
        -
        obj.location
    )

    obj.rotation_euler = (
        direction
        .to_track_quat(
            "-Z",
            "Y"
        )
        .to_euler()
    )


# ============================================================
# BEGIN
# ============================================================

clear_scene()


scene = bpy.context.scene


# ============================================================
# UNITS
#
# 1 Blender unit = 1 meter
# ============================================================

scene.unit_settings.system = (
    "METRIC"
)

scene.unit_settings.scale_length = (
    1.0
)


# ============================================================
# CREATE FLAT NORMAL TEXTURE
# ============================================================

normal_texture = create_flat_normal_map(
    name="Test_Normal",
    size=NORMAL_TEXTURE_SIZE,
    filepath=NORMAL_TEXTURE_PATH
)


# ============================================================
# MATERIALS
# ============================================================

mat_floor = create_material(
    name="MAT_Floor",
    base_color=(
        0.18,
        0.18,
        0.18
    ),
    metallic=0.0,
    roughness=0.8,
    normal_image=normal_texture
)


mat_white_rough = create_material(
    name="MAT_White_Rough",
    base_color=(
        0.75,
        0.75,
        0.75
    ),
    metallic=0.0,
    roughness=0.8,
    normal_image=normal_texture
)


mat_white_smooth = create_material(
    name="MAT_White_Smooth",
    base_color=(
        0.78,
        0.78,
        0.78
    ),
    metallic=0.0,
    roughness=0.15,
    normal_image=normal_texture
)


mat_metal = create_material(
    name="MAT_Metal",
    base_color=(
        0.65,
        0.67,
        0.70
    ),
    metallic=1.0,
    roughness=0.2,
    normal_image=normal_texture
)


mat_red = create_material(
    name="MAT_Red",
    base_color=(
        0.65,
        0.05,
        0.03
    ),
    metallic=0.0,
    roughness=0.45,
    normal_image=normal_texture
)


mat_green = create_material(
    name="MAT_Green",
    base_color=(
        0.04,
        0.55,
        0.12
    ),
    metallic=0.0,
    roughness=0.45,
    normal_image=normal_texture
)


mat_blue = create_material(
    name="MAT_Blue",
    base_color=(
        0.03,
        0.12,
        0.70
    ),
    metallic=0.0,
    roughness=0.45,
    normal_image=normal_texture
)


# ============================================================
# FLOOR
# ============================================================

add_cube(
    name="Floor",
    location=(
        0.0,
        0.0,
        -0.25
    ),
    scale=(
        6.0,
        6.0,
        0.25
    ),
    material=mat_floor
)


# ============================================================
# PEDESTAL
# ============================================================

add_cylinder(
    name="Pedestal",
    location=(
        0.0,
        0.0,
        0.25
    ),
    radius=1.4,
    depth=0.5,
    material=mat_white_rough
)


# ============================================================
# MAIN SPHERES
# ============================================================

add_sphere(
    name="Sphere_Rough",
    location=(
        -1.7,
        0.0,
        1.25
    ),
    radius=0.75,
    material=mat_white_rough
)


add_sphere(
    name="Sphere_Glossy",
    location=(
        0.0,
        0.0,
        1.25
    ),
    radius=0.75,
    material=mat_white_smooth
)


add_sphere(
    name="Sphere_Metal",
    location=(
        1.7,
        0.0,
        1.25
    ),
    radius=0.75,
    material=mat_metal
)


# ============================================================
# COLORED CUBES
# ============================================================

add_cube(
    name="Cube_Red",
    location=(
        -2.8,
        2.4,
        0.6
    ),
    scale=(
        0.6,
        0.6,
        0.6
    ),
    material=mat_red
)


add_cube(
    name="Cube_Green",
    location=(
        0.0,
        2.4,
        0.6
    ),
    scale=(
        0.6,
        0.6,
        0.6
    ),
    material=mat_green
)


add_cube(
    name="Cube_Blue",
    location=(
        2.8,
        2.4,
        0.6
    ),
    scale=(
        0.6,
        0.6,
        0.6
    ),
    material=mat_blue
)


# ============================================================
# ATTENUATION CYLINDERS
# ============================================================

marker_positions = [
    -4.0,
    -2.0,
    0.0,
    2.0,
    4.0
]


for index, x in enumerate(
    marker_positions
):

    height = 1.5

    add_cylinder(
        name=f"DistanceMarker_{index}",
        location=(
            x,
            -3.2,
            height * 0.5
        ),
        radius=0.25,
        depth=height,
        material=mat_white_rough,
        vertices=32
    )


# ============================================================
# BACK WALL
# ============================================================

add_cube(
    name="BackWall",
    location=(
        0.0,
        5.5,
        2.5
    ),
    scale=(
        6.0,
        0.15,
        2.5
    ),
    material=mat_white_rough
)


# ============================================================
# SIDE WALL
# ============================================================

add_cube(
    name="SideWall",
    location=(
        -5.5,
        0.0,
        2.5
    ),
    scale=(
        0.15,
        5.5,
        2.5
    ),
    material=mat_white_rough
)


# ============================================================
# STEPS
# ============================================================

for index in range(4):

    height = (
        0.25
        * (index + 1)
    )

    add_cube(
        name=f"Step_{index}",
        location=(
            4.1,
            0.3 + index * 0.75,
            height * 0.5
        ),
        scale=(
            0.8,
            0.6,
            height * 0.5
        ),
        material=mat_white_rough
    )


# ============================================================
# SMALL ATTENUATION SPHERES
# ============================================================

sphere_positions = [
    (-4.0, -1.7, 0.5),
    (-2.0, -1.7, 0.5),
    (0.0, -1.7, 0.5),
    (2.0, -1.7, 0.5),
    (4.0, -1.7, 0.5),
]


for index, position in enumerate(
    sphere_positions
):

    add_sphere(
        name=f"AttenuationSphere_{index}",
        location=position,
        radius=0.4,
        material=mat_white_rough,
        segments=32,
        rings=16
    )


# ============================================================
# BLENDER PREVIEW POINT LIGHT
#
# Not exported.
# ============================================================

light_data = bpy.data.lights.new(
    name="TestPointLight",
    type="POINT"
)

light_data.energy = 1000.0

light_data.color = (
    1.0,
    0.68,
    0.38
)

light_data.shadow_soft_size = (
    0.15
)


light_object = bpy.data.objects.new(
    name="TestPointLight",
    object_data=light_data
)

bpy.context.collection.objects.link(
    light_object
)

light_object.location = (
    0.0,
    -2.5,
    3.0
)


# ============================================================
# CAMERA
# ============================================================

camera_data = bpy.data.cameras.new(
    "Camera"
)

camera = bpy.data.objects.new(
    "Camera",
    camera_data
)

bpy.context.collection.objects.link(
    camera
)

camera.location = (
    10.0,
    -13.0,
    8.0
)

look_at(
    camera,
    (
        0.0,
        0.5,
        1.0
    )
)

scene.camera = camera


# ============================================================
# WORLD
# ============================================================

if scene.world is None:

    scene.world = bpy.data.worlds.new(
        "World"
    )


world = scene.world

world.use_nodes = True


background = world.node_tree.nodes.get(
    "Background"
)


if background is not None:

    background.inputs[
        "Color"
    ].default_value = (
        0.01,
        0.01,
        0.01,
        1.0
    )

    background.inputs[
        "Strength"
    ].default_value = 0.05


# ============================================================
# SAVE BLEND FILE
# ============================================================

bpy.ops.wm.save_as_mainfile(
    filepath=BLEND_PATH
)


# ============================================================
# SELECT MESHES
# ============================================================

bpy.ops.object.select_all(
    action="DESELECT"
)


for obj in scene.objects:

    if obj.type == "MESH":

        obj.select_set(
            True
        )


# ============================================================
# EXPORT GLB
#
# Exports:
#
# POSITION
# NORMAL
# TEXCOORD_0
# TANGENT
# normalTexture
#
# Light and camera are intentionally excluded.
# ============================================================

bpy.ops.export_scene.gltf(
    filepath=GLB_PATH,

    export_format="GLB",

    export_texcoords=True,
    export_normals=True,
    export_tangents=True,

    export_materials="EXPORT",

    export_lights=False,
    export_cameras=False,

    export_yup=True
)


# ============================================================
# FINISHED
# ============================================================

print("")
print("============================================")
print("POINT LIGHT TEST SCENE CREATED")
print("============================================")
print("")
print("GLB:")
print(GLB_PATH)

print("")
print("BLEND:")
print(BLEND_PATH)

print("")
print("NORMAL TEXTURE:")
print(NORMAL_TEXTURE_PATH)

print("")
print(
    "Normal texture RGB = (128, 128, 255)"
)

print(
    "Decoded tangent normal = approximately (0, 0, 1)"
)

print(
    "After TBN transform this represents the mesh vertex normal."
)

print("")
print("============================================")