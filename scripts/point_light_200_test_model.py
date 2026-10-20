import bpy
import math
import os
import json
import struct
import zlib
import random

from mathutils import Vector


# ============================================================
# OUTPUT
#
# IMPORTANT:
# Do not put a trailing "\" at the end of a raw Python string.
# ============================================================

OUTPUT_DIR = r"D:\dev\Hades-Engine\data\models"

os.makedirs(
    OUTPUT_DIR,
    exist_ok=True
)


GLB_PATH = os.path.join(
    OUTPUT_DIR,
    "point_light_200_models_test.glb"
)

BLEND_PATH = os.path.join(
    OUTPUT_DIR,
    "point_light_200_models_test.blend"
)

LIGHT_JSON_PATH = os.path.join(
    OUTPUT_DIR,
    "point_light_200_models_test_lights.json"
)

NORMAL_TEXTURE_PATH = os.path.join(
    OUTPUT_DIR,
    "Test_Normal.png"
)


# ============================================================
# SOURCE MODELS
# ============================================================

DRAGON_PATH = (
    r"C:\Users\10\Downloads\dragon\dragon.obj"
)

BOWSER_PATH = (
    r"C:\Users\10\Downloads\bowser+001+glb\bowser 001.glb"
)


# ============================================================
# MODEL SETTINGS
# ============================================================

# Dragon will be normalized to this height after orientation.
DRAGON_TARGET_HEIGHT = 8.0

# Bowser previous size was around 4.5.
# 45.0 makes it 10x larger.
BOWSER_TARGET_HEIGHT = 20.0


# ------------------------------------------------------------
# Dragon orientation
#
# Your dragon is horizontally aligned along its source Y axis.
#
# +90 degrees around X:
#
# source Y -> Blender Z
#
# so the dragon becomes vertical.
# ------------------------------------------------------------

DRAGON_ROTATE_X_DEGREES = 90.0


# ------------------------------------------------------------
# Bowser vertical placement
#
# Instance is raised by half its final height.
#
# At scale 1:
#
# 45 * 0.5 = 22.5
# ------------------------------------------------------------

BOWSER_HEIGHT_OFFSET_FACTOR = 0.5


# ============================================================
# NUMBER OF MODELS
# ============================================================

DRAGON_INSTANCE_COUNT = 80

BOWSER_INSTANCE_COUNT = 40


# ============================================================
# POINT LIGHT SETTINGS
#
# 20 x 10 = 200 lights
# ============================================================

LIGHT_COLUMNS = 20
LIGHT_ROWS = 10

# Large amount of space between lights.
LIGHT_SPACING_X = 40.0
LIGHT_SPACING_Y = 40.0

LIGHT_HEIGHT = 14.0

LIGHT_RADIUS = 45.0

LIGHT_INTENSITY = 1200.0


# ============================================================
# NORMAL TEXTURE
# ============================================================

NORMAL_TEXTURE_SIZE = 256


# ============================================================
# RANDOM PLACEMENT
# ============================================================

random.seed(1337)


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


    collections = [

        bpy.data.meshes,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,

    ]


    for collection in collections:

        for block in list(collection):

            if block.users == 0:

                collection.remove(
                    block
                )


# ============================================================
# PNG SUPPORT
# ============================================================

def png_chunk(
    chunk_type,
    data
):

    chunk = (
        chunk_type
        + data
    )

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
            zlib.crc32(chunk)
            & 0xFFFFFFFF
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


    header = struct.pack(

        ">IIBBBBB",

        width,
        height,

        8,      # 8 bits
        2,      # RGB

        0,
        0,
        0
    )


    png.extend(

        png_chunk(
            b"IHDR",
            header
        )

    )


    raw = bytearray()

    row_size = (
        width * 3
    )


    for y in range(height):

        # PNG filter type 0
        raw.append(0)

        start = (
            y * row_size
        )

        end = (
            start + row_size
        )

        raw.extend(
            rgb_data[
                start:end
            ]
        )


    compressed = zlib.compress(
        bytes(raw),
        9
    )


    png.extend(

        png_chunk(
            b"IDAT",
            compressed
        )

    )


    png.extend(

        png_chunk(
            b"IEND",
            b""
        )

    )


    with open(
        filepath,
        "wb"
    ) as file:

        file.write(
            png
        )


# ============================================================
# FLAT NORMAL MAP
#
# Tangent-space:
#
#     (0, 0, 1)
#
# Encoded:
#
#     RGB = 128, 128, 255
#
# After TBN transformation this represents the interpolated
# vertex normal.
# ============================================================

def create_flat_normal_map():

    pixel = bytes([

        128,
        128,
        255

    ])


    data = (

        pixel
        *
        (
            NORMAL_TEXTURE_SIZE
            *
            NORMAL_TEXTURE_SIZE
        )

    )


    write_rgb_png(

        NORMAL_TEXTURE_PATH,

        NORMAL_TEXTURE_SIZE,
        NORMAL_TEXTURE_SIZE,

        data

    )


    old_image = bpy.data.images.get(
        "Test_Normal"
    )


    if old_image is not None:

        bpy.data.images.remove(
            old_image
        )


    image = bpy.data.images.load(

        NORMAL_TEXTURE_PATH,

        check_existing=False

    )


    image.name = (
        "Test_Normal"
    )


    # Normal maps must be linear.
    image.colorspace_settings.name = (
        "Non-Color"
    )


    return image


# ============================================================
# MATERIAL
# ============================================================

def create_material(
    name,
    color,
    metallic,
    roughness,
    normal_texture
):

    material = bpy.data.materials.new(
        name=name
    )

    material.use_nodes = True


    nodes = (
        material.node_tree.nodes
    )

    links = (
        material.node_tree.links
    )


    bsdf = nodes.get(
        "Principled BSDF"
    )


    if bsdf is None:

        raise RuntimeError(
            "Principled BSDF not found."
        )


    bsdf.inputs[
        "Base Color"
    ].default_value = (

        color[0],
        color[1],
        color[2],

        1.0

    )


    if "Metallic" in bsdf.inputs:

        bsdf.inputs[
            "Metallic"
        ].default_value = (
            metallic
        )


    if "Roughness" in bsdf.inputs:

        bsdf.inputs[
            "Roughness"
        ].default_value = (
            roughness
        )


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
        "Flat Tangent Normal"
    )

    texture_node.image = (
        normal_texture
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

    normal_node.space = (
        "TANGENT"
    )

    normal_node.inputs[
        "Strength"
    ].default_value = (
        1.0
    )

    normal_node.location = (
        -200,
        -300
    )


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
# MATERIAL ASSIGNMENT
# ============================================================

def assign_material(
    obj,
    material,
    replace=True
):

    if obj.data is None:

        return


    if not hasattr(
        obj.data,
        "materials"
    ):

        return


    if replace:

        obj.data.materials.clear()


    obj.data.materials.append(
        material
    )


# ============================================================
# TRANSFORM
# ============================================================

def apply_transform(
    obj,
    location=False,
    rotation=False,
    scale=False
):

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

        location=location,
        rotation=rotation,
        scale=scale

    )


    obj.select_set(
        False
    )


# ============================================================
# SMART UV
# ============================================================

def smart_uv(obj):

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

        island_margin=0.02

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
    position,
    scale,
    material
):

    bpy.ops.mesh.primitive_cube_add(
        location=position
    )


    obj = bpy.context.object

    obj.name = name

    obj.scale = scale


    apply_transform(
        obj,
        scale=True
    )


    assign_material(
        obj,
        material
    )


    smart_uv(
        obj
    )


    return obj


# ============================================================
# SPHERE
# ============================================================

def add_sphere(
    name,
    position,
    radius,
    material,
    segments=16,
    rings=8
):

    bpy.ops.mesh.primitive_uv_sphere_add(

        segments=segments,
        ring_count=rings,

        radius=radius,

        location=position

    )


    obj = bpy.context.object

    obj.name = name


    assign_material(
        obj,
        material
    )


    for polygon in obj.data.polygons:

        polygon.use_smooth = (
            True
        )


    return obj


# ============================================================
# CYLINDER
# ============================================================

def add_cylinder(
    name,
    position,
    radius,
    height,
    material
):

    bpy.ops.mesh.primitive_cylinder_add(

        vertices=20,

        radius=radius,
        depth=height,

        location=position

    )


    obj = bpy.context.object

    obj.name = name


    assign_material(
        obj,
        material
    )


    smart_uv(
        obj
    )


    return obj


# ============================================================
# IMPORT OBJ
# ============================================================

def import_obj(filepath):

    if not os.path.isfile(
        filepath
    ):

        raise FileNotFoundError(

            "Dragon OBJ not found:\n"
            + filepath

        )


    before = set(

        obj.name

        for obj in bpy.data.objects

    )


    print("")
    print("Importing Dragon:")
    print(filepath)


    try:

        # Blender 4.x
        bpy.ops.wm.obj_import(
            filepath=filepath
        )

    except Exception:

        # Older Blender
        bpy.ops.import_scene.obj(
            filepath=filepath
        )


    imported = []


    for obj in bpy.data.objects:

        if (
            obj.name not in before
            and
            obj.type == "MESH"
        ):

            imported.append(
                obj
            )


    if not imported:

        raise RuntimeError(
            "Dragon OBJ produced no mesh objects."
        )


    return imported


# ============================================================
# IMPORT GLB
# ============================================================

def import_glb(filepath):

    if not os.path.isfile(
        filepath
    ):

        raise FileNotFoundError(

            "Bowser GLB not found:\n"
            + filepath

        )


    before = set(

        obj.name

        for obj in bpy.data.objects

    )


    print("")
    print("Importing Bowser:")
    print(filepath)


    bpy.ops.import_scene.gltf(
        filepath=filepath
    )


    imported_objects = []

    imported_meshes = []


    for obj in bpy.data.objects:

        if obj.name not in before:

            imported_objects.append(
                obj
            )

            if obj.type == "MESH":

                imported_meshes.append(
                    obj
                )


    if not imported_meshes:

        raise RuntimeError(
            "Bowser GLB produced no mesh objects."
        )


    return (
        imported_meshes,
        imported_objects
    )


# ============================================================
# DETACH OBJECT FROM PARENT
#
# Preserve world transform before joining imported GLB meshes.
# ============================================================

def detach_keep_world(obj):

    world_matrix = (
        obj.matrix_world.copy()
    )


    obj.parent = None


    obj.matrix_world = (
        world_matrix
    )


# ============================================================
# JOIN IMPORTED MESHES
# ============================================================

def join_meshes(
    meshes,
    name
):

    bpy.ops.object.select_all(
        action="DESELECT"
    )


    for obj in meshes:

        detach_keep_world(
            obj
        )

        obj.hide_viewport = False
        obj.hide_render = False
        obj.hide_set(False)

        obj.select_set(
            True
        )


    bpy.context.view_layer.objects.active = (
        meshes[0]
    )


    if len(meshes) > 1:

        bpy.ops.object.join()


    result = (
        bpy.context.object
    )


    result.name = (
        name
    )


    result.hide_viewport = False
    result.hide_render = False
    result.hide_set(False)


    return result


# ============================================================
# DELETE UNUSED IMPORT HELPERS
#
# GLB files may contain:
#
# - Empty
# - Armature
# - Camera
# - Light
#
# After converting Bowser to one static mesh, these are not
# required for this stress-test scene.
# ============================================================

def delete_import_helpers(
    imported_objects,
    keep_object
):

    for obj in list(
        imported_objects
    ):

        if obj == keep_object:

            continue


        if obj.type == "MESH":

            # Joined mesh parts were already consumed.
            if obj.name not in bpy.data.objects:

                continue


        try:

            bpy.data.objects.remove(
                obj,
                do_unlink=True
            )

        except Exception:

            pass


# ============================================================
# DRAGON ORIENTATION
#
# Source Y -> Blender Z
# ============================================================

def orient_dragon_vertical(obj):

    print("")
    print(
        "Rotating Dragon +90 degrees around X."
    )


    obj.rotation_euler = (

        math.radians(
            DRAGON_ROTATE_X_DEGREES
        ),

        0.0,
        0.0

    )


    apply_transform(
        obj,
        rotation=True
    )


# ============================================================
# PREPARE MODEL
#
# 1. Apply imported scale
# 2. Center X/Y
# 3. Put model bottom at Z=0
# 4. Normalize height
# 5. Smooth normals
# 6. Ensure UV
# 7. Assign engine-test material
# ============================================================

def prepare_model(
    obj,
    target_height,
    material
):

    obj.hide_viewport = False
    obj.hide_render = False
    obj.hide_set(False)


    apply_transform(
        obj,
        scale=True
    )


    corners = [

        Vector(corner)

        for corner in obj.bound_box

    ]


    min_x = min(
        point.x
        for point in corners
    )

    max_x = max(
        point.x
        for point in corners
    )


    min_y = min(
        point.y
        for point in corners
    )

    max_y = max(
        point.y
        for point in corners
    )


    min_z = min(
        point.z
        for point in corners
    )

    max_z = max(
        point.z
        for point in corners
    )


    center_x = (
        min_x + max_x
    ) * 0.5

    center_y = (
        min_y + max_y
    ) * 0.5


    height = (
        max_z - min_z
    )


    if height <= 0.000001:

        raise RuntimeError(
            obj.name
            + " has invalid height."
        )


    # ========================================================
    # CENTER MODEL
    # ========================================================

    for vertex in obj.data.vertices:

        vertex.co.x -= (
            center_x
        )

        vertex.co.y -= (
            center_y
        )

        vertex.co.z -= (
            min_z
        )


    # ========================================================
    # SCALE TO TARGET HEIGHT
    # ========================================================

    scale_factor = (
        target_height
        / height
    )


    obj.scale = (

        scale_factor,
        scale_factor,
        scale_factor

    )


    apply_transform(
        obj,
        scale=True
    )


    obj.location = (
        0.0,
        0.0,
        0.0
    )


    # ========================================================
    # SMOOTH VERTEX NORMALS
    # ========================================================

    for polygon in obj.data.polygons:

        polygon.use_smooth = (
            True
        )


    # ========================================================
    # UV
    # ========================================================

    if len(
        obj.data.uv_layers
    ) == 0:

        print(
            obj.name,
            "has no UVs. Generating Smart UV."
        )

        smart_uv(
            obj
        )


    # ========================================================
    # MATERIAL
    # ========================================================

    assign_material(

        obj,
        material,

        replace=True

    )


    print(

        obj.name,
        "final height:",
        target_height

    )


# ============================================================
# CREATE LINKED INSTANCE
# ============================================================

def create_linked_instance(
    source,
    name,
    position,
    rotation_z=0.0,
    scale=1.0
):

    obj = (
        source.copy()
    )


    # All copies share the same underlying mesh.
    obj.data = (
        source.data
    )


    bpy.context.collection.objects.link(
        obj
    )


    obj.name = (
        name
    )


    # Force visibility.
    obj.hide_viewport = False
    obj.hide_render = False
    obj.hide_set(False)


    obj.location = (

        position[0],
        position[1],
        position[2]

    )


    obj.rotation_euler = (

        0.0,
        0.0,
        rotation_z

    )


    obj.scale = (

        scale,
        scale,
        scale

    )


    return obj


# ============================================================
# DRAGON INSTANCE
# ============================================================

def create_dragon_instance(
    source,
    name,
    x,
    y,
    rotation_z,
    scale
):

    return create_linked_instance(

        source,

        name,

        (
            x,
            y,
            0.0
        ),

        rotation_z,

        scale

    )


# ============================================================
# BOWSER INSTANCE
#
# Bowser is raised by half its FINAL height.
# ============================================================

def create_bowser_instance(
    source,
    name,
    x,
    y,
    rotation_z,
    scale
):

    instance_height = (

        BOWSER_TARGET_HEIGHT
        * scale

    )


    z_offset = (

        instance_height
        *
        BOWSER_HEIGHT_OFFSET_FACTOR

    )


    return create_linked_instance(

        source,

        name,

        (
            x,
            y,
            z_offset
        ),

        rotation_z,

        scale

    )


# ============================================================
# CAMERA
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
# START
# ============================================================

clear_scene()


scene = (
    bpy.context.scene
)


scene.unit_settings.system = (
    "METRIC"
)

scene.unit_settings.scale_length = (
    1.0
)


# ============================================================
# NORMAL TEXTURE
# ============================================================

normal_texture = (
    create_flat_normal_map()
)


# ============================================================
# MATERIALS
# ============================================================

mat_floor = create_material(

    "MAT_Floor",

    (
        0.08,
        0.08,
        0.10
    ),

    0.0,
    0.80,

    normal_texture

)


mat_wall = create_material(

    "MAT_Wall",

    (
        0.25,
        0.27,
        0.30
    ),

    0.0,
    0.65,

    normal_texture

)


mat_pillar = create_material(

    "MAT_Pillar",

    (
        0.48,
        0.50,
        0.53
    ),

    0.10,
    0.40,

    normal_texture

)


mat_dragon = create_material(

    "MAT_Dragon",

    (
        0.28,
        0.38,
        0.46
    ),

    0.45,
    0.28,

    normal_texture

)


mat_bowser = create_material(

    "MAT_Bowser",

    (
        0.42,
        0.52,
        0.25
    ),

    0.10,
    0.32,

    normal_texture

)


# ============================================================
# ROOM DIMENSIONS
# ============================================================

room_width = (

    (LIGHT_COLUMNS - 1)
    * LIGHT_SPACING_X
    + 80.0

)


room_depth = (

    (LIGHT_ROWS - 1)
    * LIGHT_SPACING_Y
    + 80.0

)


print("")
print(
    "Room:",
    room_width,
    "x",
    room_depth
)


# ============================================================
# FLOOR
# ============================================================

add_cube(

    "Floor",

    (
        0.0,
        0.0,
        -0.5
    ),

    (
        room_width * 0.5,
        room_depth * 0.5,
        0.5
    ),

    mat_floor

)


# ============================================================
# WALLS
# ============================================================

wall_height = 70.0

wall_thickness = 0.75


add_cube(

    "Wall_Back",

    (
        0.0,
        room_depth * 0.5,
        wall_height * 0.5
    ),

    (
        room_width * 0.5,
        wall_thickness,
        wall_height * 0.5
    ),

    mat_wall

)


add_cube(

    "Wall_Front",

    (
        0.0,
        -room_depth * 0.5,
        wall_height * 0.5
    ),

    (
        room_width * 0.5,
        wall_thickness,
        wall_height * 0.5
    ),

    mat_wall

)


add_cube(

    "Wall_Left",

    (
        -room_width * 0.5,
        0.0,
        wall_height * 0.5
    ),

    (
        wall_thickness,
        room_depth * 0.5,
        wall_height * 0.5
    ),

    mat_wall

)


add_cube(

    "Wall_Right",

    (
        room_width * 0.5,
        0.0,
        wall_height * 0.5
    ),

    (
        wall_thickness,
        room_depth * 0.5,
        wall_height * 0.5
    ),

    mat_wall

)


# ============================================================
# IMPORT DRAGON
# ============================================================

print("")
print("============================================")
print("DRAGON")
print("============================================")
print(DRAGON_PATH)


dragon_parts = import_obj(
    DRAGON_PATH
)


dragon_template = join_meshes(

    dragon_parts,

    "Dragon_Template"

)


# Dragon must become vertical BEFORE calculating height.
orient_dragon_vertical(
    dragon_template
)


prepare_model(

    dragon_template,

    DRAGON_TARGET_HEIGHT,

    mat_dragon

)


# ============================================================
# IMPORT BOWSER
# ============================================================

print("")
print("============================================")
print("BOWSER")
print("============================================")
print(BOWSER_PATH)


bowser_parts, bowser_import_objects = import_glb(
    BOWSER_PATH
)


bowser_template = join_meshes(

    bowser_parts,

    "Bowser_Template"

)


prepare_model(

    bowser_template,

    BOWSER_TARGET_HEIGHT,

    mat_bowser

)


# Remove GLB helper objects after Bowser becomes one mesh.
delete_import_helpers(

    bowser_import_objects,

    bowser_template

)


# ============================================================
# HERO MODELS
# ============================================================

dragon_center = create_dragon_instance(

    dragon_template,

    "Dragon_Center",

    -35.0,
    0.0,

    math.radians(
        25.0
    ),

    2.0

)


bowser_center = create_bowser_instance(

    bowser_template,

    "Bowser_Center",

    35.0,
    0.0,

    math.radians(
        -25.0
    ),

    1.0

)


# ============================================================
# RANDOM PLACEMENT AREA
# ============================================================

placement_width = (

    room_width * 0.5
    - 30.0

)


placement_depth = (

    room_depth * 0.5
    - 30.0

)


# ============================================================
# MANY DRAGONS
# ============================================================

for index in range(
    DRAGON_INSTANCE_COUNT
):

    x = random.uniform(

        -placement_width,
        placement_width

    )


    y = random.uniform(

        -placement_depth,
        placement_depth

    )


    rotation = random.uniform(

        0.0,
        math.tau

    )


    scale = random.uniform(

        0.9,
        2.0

    )


    create_dragon_instance(

        dragon_template,

        f"Dragon_{index}",

        x,
        y,

        rotation,

        scale

    )


# ============================================================
# MANY BOWSERS
# ============================================================

for index in range(
    BOWSER_INSTANCE_COUNT
):

    x = random.uniform(

        -placement_width,
        placement_width

    )


    y = random.uniform(

        -placement_depth,
        placement_depth

    )


    rotation = random.uniform(

        0.0,
        math.tau

    )


    scale = random.uniform(

        0.75,
        1.15

    )


    create_bowser_instance(

        bowser_template,

        f"Bowser_{index}",

        x,
        y,

        rotation,

        scale

    )


# ============================================================
# PILLARS
# ============================================================

pillar_index = 0


for row in range(
    1,
    LIGHT_ROWS,
    2
):

    for column in range(
        1,
        LIGHT_COLUMNS,
        3
    ):

        x = (

            column
            -
            (LIGHT_COLUMNS - 1)
            * 0.5

        ) * LIGHT_SPACING_X


        y = (

            row
            -
            (LIGHT_ROWS - 1)
            * 0.5

        ) * LIGHT_SPACING_Y


        add_cylinder(

            f"Pillar_{pillar_index}",

            (
                x,
                y,
                12.0
            ),

            1.8,
            24.0,

            mat_pillar

        )


        pillar_index += 1


# ============================================================
# LIGHT COLORS
# ============================================================

light_colors = [

    (
        1.00,
        0.08,
        0.05
    ),

    (
        0.05,
        0.25,
        1.00
    ),

    (
        0.05,
        1.00,
        0.20
    ),

    (
        1.00,
        0.50,
        0.05
    ),

    (
        0.75,
        0.08,
        1.00
    ),

    (
        0.05,
        0.90,
        1.00
    ),

    (
        1.00,
        0.12,
        0.50
    ),

    (
        0.70,
        0.82,
        1.00
    ),

]


# ============================================================
# CREATE 200 POINT LIGHTS
# ============================================================

lights_json = []

light_index = 0


for row in range(
    LIGHT_ROWS
):

    for column in range(
        LIGHT_COLUMNS
    ):

        x = (

            column
            -
            (LIGHT_COLUMNS - 1)
            * 0.5

        ) * LIGHT_SPACING_X


        y = (

            row
            -
            (LIGHT_ROWS - 1)
            * 0.5

        ) * LIGHT_SPACING_Y


        z = (

            LIGHT_HEIGHT

            +

            math.sin(

                column * 0.55
                +
                row * 0.8

            )

            * 4.0

        )


        color = light_colors[

            light_index
            %
            len(light_colors)

        ]


        # ====================================================
        # BLENDER PREVIEW LIGHT
        # ====================================================

        light_data = bpy.data.lights.new(

            name=f"PointLight_{light_index}",

            type="POINT"

        )


        light_data.energy = (
            LIGHT_INTENSITY
        )


        light_data.color = (
            color
        )


        light_data.shadow_soft_size = (
            0.5
        )


        light_object = bpy.data.objects.new(

            name=f"PointLight_{light_index}",

            object_data=light_data

        )


        bpy.context.collection.objects.link(
            light_object
        )


        light_object.location = (

            x,
            y,
            z

        )


        # ====================================================
        # JSON
        #
        # Blender Z-up:
        #
        # X Y Z
        #
        # glTF Y-up:
        #
        # X Z -Y
        # ====================================================

        lights_json.append({

            "name":
                f"PointLight_{light_index}",


            "position_blender": [

                x,
                y,
                z

            ],


            "position_gltf": [

                x,
                z,
                -y

            ],


            "color": [

                color[0],
                color[1],
                color[2]

            ],


            "intensity":
                LIGHT_INTENSITY,


            "radius":
                LIGHT_RADIUS

        })


        light_index += 1


# ============================================================
# LIGHT MARKER MATERIALS
# ============================================================

marker_materials = []


for index, color in enumerate(
    light_colors
):

    marker_material = create_material(

        f"MAT_LightMarker_{index}",

        color,

        0.0,
        0.25,

        normal_texture

    )


    marker_materials.append(
        marker_material
    )


# ============================================================
# LIGHT MARKER GEOMETRY
#
# These spheres ARE exported into the GLB.
# ============================================================

for index, light in enumerate(
    lights_json
):

    position = (
        light[
            "position_blender"
        ]
    )


    add_sphere(

        f"LightMarker_{index}",

        (
            position[0],
            position[1],
            position[2]
        ),

        0.9,

        marker_materials[

            index
            %
            len(marker_materials)

        ],

        segments=12,
        rings=6

    )


# ============================================================
# LIGHT JSON
# ============================================================

with open(

    LIGHT_JSON_PATH,

    "w",

    encoding="utf-8"

) as file:

    json.dump(

        {

            "light_count":
                len(lights_json),


            "grid": {

                "columns":
                    LIGHT_COLUMNS,

                "rows":
                    LIGHT_ROWS,

                "spacing_x":
                    LIGHT_SPACING_X,

                "spacing_y":
                    LIGHT_SPACING_Y

            },


            "models": {

                "dragon_source":
                    DRAGON_PATH,

                "dragon_rotation_x":
                    DRAGON_ROTATE_X_DEGREES,

                "dragon_height":
                    DRAGON_TARGET_HEIGHT,


                "bowser_source":
                    BOWSER_PATH,

                "bowser_height":
                    BOWSER_TARGET_HEIGHT,

                "bowser_height_offset_factor":
                    BOWSER_HEIGHT_OFFSET_FACTOR,


                "dragon_instances":
                    DRAGON_INSTANCE_COUNT + 1,

                "bowser_instances":
                    BOWSER_INSTANCE_COUNT + 1

            },


            "lights":
                lights_json

        },


        file,

        indent=4

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

    room_width * 0.48,

    -room_depth * 0.75,

    190.0

)


look_at(

    camera,

    (
        0.0,
        0.0,
        20.0
    )

)


camera_data.lens = (
    45.0
)


scene.camera = (
    camera
)


# ============================================================
# WORLD
# ============================================================

if scene.world is None:

    scene.world = bpy.data.worlds.new(
        "World"
    )


scene.world.use_nodes = (
    True
)


background = (

    scene.world
    .node_tree
    .nodes
    .get(
        "Background"
    )

)


if background is not None:

    background.inputs[
        "Color"
    ].default_value = (

        0.001,
        0.001,
        0.001,
        1.0

    )


    background.inputs[
        "Strength"
    ].default_value = (
        0.01
    )


# ============================================================
# REMOVE SOURCE TEMPLATE OBJECTS
#
# Instances share their mesh data.
#
# Removing these two OBJECTS does not delete the actual mesh
# because all instances still use it.
#
# This prevents duplicate hidden templates from entering GLB.
# ============================================================

bpy.data.objects.remove(

    dragon_template,

    do_unlink=True

)


bpy.data.objects.remove(

    bowser_template,

    do_unlink=True

)


# ============================================================
# FORCE ALL GEOMETRY VISIBLE
#
# We want the whole generated geometry exported.
# ============================================================

for obj in scene.objects:

    if obj.type == "MESH":

        obj.hide_viewport = False

        obj.hide_render = False

        obj.hide_set(False)


# ============================================================
# SAVE BLENDER SCENE
# ============================================================

bpy.ops.wm.save_as_mainfile(
    filepath=BLEND_PATH
)


# ============================================================
# EXPORT ENTIRE SCENE GEOMETRY
#
# IMPORTANT:
#
# use_selection=False
#
# means we are NOT exporting only selected meshes.
#
# All scene geometry will be exported.
#
# Blender preview point lights are excluded because your
# DirectX renderer uses the JSON light list.
# ============================================================

print("")
print("============================================")
print("EXPORTING COMPLETE SCENE")
print("============================================")

print(
    GLB_PATH
)


bpy.ops.export_scene.gltf(

    filepath=GLB_PATH,

    # One binary glTF file
    export_format="GLB",

    # Export complete scene
    use_selection=False,

    # Vertex data
    export_texcoords=True,

    export_normals=True,

    export_tangents=True,

    # Materials + embedded textures
    export_materials="EXPORT",

    # glTF convention
    export_yup=True,

    # Camera is not needed by engine test
    export_cameras=False,

    # Lights are loaded from JSON
    export_lights=False,

    # Static test scene
    export_animations=False

)


# ============================================================
# DONE
# ============================================================

print("")
print("======================================================")
print("COMPLETE STRESS TEST SCENE EXPORTED")
print("======================================================")


print("")
print("OUTPUT DIRECTORY:")
print(
    OUTPUT_DIR
)


print("")
print("MODEL:")
print(
    GLB_PATH
)


print("")
print("LIGHT DATA:")
print(
    LIGHT_JSON_PATH
)


print("")
print("NORMAL TEXTURE:")
print(
    NORMAL_TEXTURE_PATH
)


print("")
print("BLENDER SCENE:")
print(
    BLEND_PATH
)


print("")
print(
    "Lights:",
    len(lights_json)
)


print(
    "Light spacing:",
    LIGHT_SPACING_X,
    "x",
    LIGHT_SPACING_Y
)


print("")
print(
    "Dragons:",
    DRAGON_INSTANCE_COUNT + 1
)


print(
    "Bowsers:",
    BOWSER_INSTANCE_COUNT + 1
)


print("")
print(
    "Dragon orientation:",
    "source Y -> Blender Z"
)


print(
    "Bowser height:",
    BOWSER_TARGET_HEIGHT
)


print(
    "Bowser default vertical offset:",
    BOWSER_TARGET_HEIGHT
    * BOWSER_HEIGHT_OFFSET_FACTOR
)


print("")
print("======================================================")