# pyright: reportMissingImports=false

"""
Headless Blender helper:
Renders one static front-view PNG from assets/npc/model.fbx to assets/npc/model.png

Usage:
  blender -b -P tools/blender_render_npc.py
"""

import bpy
import os
from mathutils import Vector


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FBX_PATH = os.path.join(ROOT, "assets", "npc", "model.fbx")
OUT_PATH = os.path.join(ROOT, "assets", "npc", "model.png")


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def import_fbx(path):
    bpy.ops.import_scene.fbx(filepath=path)


def frame_camera_to_objects():
    objs = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not objs:
        raise RuntimeError("No mesh objects found in FBX.")

    cam_data = bpy.data.cameras.new("NPC_Camera")
    cam_obj = bpy.data.objects.new("NPC_Camera", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    # Front-ish placement; tweak if needed.
    cam_obj.location = Vector((0.0, -2.8, 1.5))
    cam_obj.rotation_euler = (1.45, 0.0, 0.0)


def setup_render():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = True
    scene.render.resolution_x = 512
    scene.render.resolution_y = 512
    scene.render.filepath = OUT_PATH


def main():
    if not os.path.exists(FBX_PATH):
        raise FileNotFoundError(f"FBX not found: {FBX_PATH}")

    clear_scene()
    import_fbx(FBX_PATH)
    frame_camera_to_objects()
    setup_render()
    bpy.ops.render.render(write_still=True)
    print(f"[OK] Rendered {OUT_PATH}")


if __name__ == "__main__":
    main()