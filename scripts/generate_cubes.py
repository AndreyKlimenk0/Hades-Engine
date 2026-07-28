import bpy
import math
import random

BOX_COUNT = 1000

# Horizontal scene dimensions.
AREA_SIZE = 200.0

# Vertical scene dimension.
AREA_HEIGHT = 50.0

SEED = 42

random.seed(SEED)

MIN_SCALE = 0.5
MAX_SCALE = 20.0

# Remove all existing objects.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

# Create a temporary cube and keep its mesh.
bpy.ops.mesh.primitive_cube_add(size=1.0)
temporary_cube = bpy.context.active_object
shared_cube_mesh = temporary_cube.data

# Remove only the temporary object.
# The shared mesh data remains available.
bpy.data.objects.remove(temporary_cube, do_unlink=True)

# Create a collection for generated boxes.
box_collection = bpy.data.collections.new("TestBoxes")
bpy.context.scene.collection.children.link(box_collection)

for index in range(BOX_COUNT):
    box = bpy.data.objects.new(
        f"Box_{index:05d}",
        shared_cube_mesh
    )

    box.location = (
        random.uniform(-AREA_SIZE, AREA_SIZE),
        random.uniform(-AREA_SIZE, AREA_SIZE),
        random.uniform(0.0, AREA_HEIGHT),
    )

    box.rotation_euler = (
        random.uniform(0.0, math.tau),
        random.uniform(0.0, math.tau),
        random.uniform(0.0, math.tau),
    )

    box.scale = (
        random.uniform(MIN_SCALE, MAX_SCALE),
        random.uniform(MIN_SCALE, MAX_SCALE),
        random.uniform(MIN_SCALE, MAX_SCALE),
    )

    box_collection.objects.link(box)

# Add a ground plane.
bpy.ops.mesh.primitive_plane_add(
    size=AREA_SIZE * 2.5,
    location=(0.0, 0.0, 0.0)
)

bpy.context.active_object.name = "Ground"

# Export the scene as GLB.
bpy.ops.export_scene.gltf(
    filepath=r"D:\dev\Hades-Engine\data\models\box_test_scene.glb",
    export_format="GLB"
)
print(
    f"Created {BOX_COUNT} boxes "
    f"in an area of {AREA_SIZE * 2.0} x "
    f"{AREA_SIZE * 2.0} x {AREA_HEIGHT}"
)