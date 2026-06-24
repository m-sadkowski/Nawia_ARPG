"""
Naprawa pliku Blender: włącza gizmo transformacji, ustawia pivot i przenosi originy
meshów oraz pustych parentów do środka ich geometrii/pod-obiektów.

Jak użyć:
1. Otwórz problematyczny plik .blend w Blenderze.
2. Przejdź do Scripting > Text > Open i wybierz ten plik .py.
3. Kliknij Run Script.
4. Skrypt zapisze kopię obok oryginału z dopiskiem _fixed.blend.
"""

import os
import bpy
from mathutils import Vector


def safe_set(obj, attr, value):
    if hasattr(obj, attr):
        try:
            setattr(obj, attr, value)
        except Exception:
            pass


def enable_transform_gizmos():
    """Przywraca strzałki/obręcze/kostki move/rotate/scale w widoku 3D."""
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type != "VIEW_3D":
                continue
            for space in area.spaces:
                if space.type != "VIEW_3D":
                    continue
                safe_set(space, "show_gizmo", True)
                safe_set(space, "show_gizmo_tool", True)
                safe_set(space, "show_gizmo_context", True)
                safe_set(space, "show_gizmo_object_translate", True)
                safe_set(space, "show_gizmo_object_rotate", True)
                safe_set(space, "show_gizmo_object_scale", True)
            area.tag_redraw()

    # Standardowe, wygodne ustawienia transformacji.
    for scene in bpy.data.scenes:
        safe_set(scene.tool_settings, "transform_pivot_point", "MEDIAN_POINT")
        safe_set(scene.tool_settings, "use_transform_data_origin", False)
        safe_set(scene.tool_settings, "use_proportional_edit_objects", False)
        scene.cursor.location = (0.0, 0.0, 0.0)

    try:
        bpy.ops.wm.tool_set_by_id(name="builtin.move")
    except Exception:
        pass


def all_mesh_objects():
    return [obj for obj in bpy.data.objects if obj.type == "MESH" and obj.data is not None]


def make_mesh_data_single_user(mesh_objs):
    """Origin-set modyfikuje dane siatki, więc najpierw rozłączamy współdzielone meshe."""
    for obj in mesh_objs:
        if obj.data and obj.data.users > 1:
            obj.data = obj.data.copy()


def set_mesh_origins_to_geometry(mesh_objs):
    """Ustawia origin każdego mesha na środek jego geometrii, bez przesuwania modelu."""
    if not mesh_objs:
        return 0

    bpy.ops.object.mode_set(mode="OBJECT") if bpy.ops.object.mode_set.poll() else None
    bpy.ops.object.select_all(action="DESELECT")

    # Zaznaczenie wszystkich meshów i jedna operacja jest znacznie szybsza niż pętla po obiektach.
    for obj in mesh_objs:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_objs[0]

    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
    bpy.ops.object.select_all(action="DESELECT")
    return len(mesh_objs)


def world_bounds_center(objects):
    coords = []
    for obj in objects:
        if obj.type != "MESH" or obj.data is None:
            continue
        try:
            for corner in obj.bound_box:
                coords.append(obj.matrix_world @ Vector(corner))
        except Exception:
            pass

    if not coords:
        return None

    min_v = Vector((min(v.x for v in coords), min(v.y for v in coords), min(v.z for v in coords)))
    max_v = Vector((max(v.x for v in coords), max(v.y for v in coords), max(v.z for v in coords)))
    return (min_v + max_v) * 0.5


def move_empty_origins_to_children_bounds():
    """
    Przesuwa origin pustych obiektów-parentów do środka ich potomków-meshów,
    zachowując wizualne położenie wszystkich dzieci.
    """
    empties = [obj for obj in bpy.data.objects if obj.type == "EMPTY"]
    moved = 0

    # Najpierw głębsze empties, potem nadrzędne. To daje bardziej intuicyjne centra grup.
    empties.sort(key=lambda o: len(o.children_recursive), reverse=False)

    for empty in empties:
        mesh_desc = [child for child in empty.children_recursive if child.type == "MESH" and child.data is not None]
        center = world_bounds_center(mesh_desc)
        if center is None:
            continue

        descendants = list(empty.children_recursive)
        saved_world = {child: child.matrix_world.copy() for child in descendants}

        new_world = empty.matrix_world.copy()
        new_world.translation = center
        empty.matrix_world = new_world

        # Przywróć dzieciom pozycje, żeby model nie „odjechał” po przeniesieniu originu parenta.
        for child, matrix in saved_world.items():
            child.matrix_world = matrix

        moved += 1

    return moved


def save_fixed_copy():
    if not bpy.data.filepath:
        print("Plik nie ma ścieżki na dysku — użyj File > Save As ręcznie.")
        return None

    root, ext = os.path.splitext(bpy.data.filepath)
    fixed_path = root + "_fixed" + ext
    bpy.ops.wm.save_as_mainfile(filepath=fixed_path)
    return fixed_path


def main():
    enable_transform_gizmos()

    mesh_objs = all_mesh_objects()
    make_mesh_data_single_user(mesh_objs)
    fixed_meshes = set_mesh_origins_to_geometry(mesh_objs)
    fixed_empties = move_empty_origins_to_children_bounds()
    fixed_path = save_fixed_copy()

    print("--- Naprawa zakończona ---")
    print(f"Meshe z ustawionym originem: {fixed_meshes}")
    print(f"Puste parenty z przeniesionym originem: {fixed_empties}")
    if fixed_path:
        print(f"Zapisano kopię: {fixed_path}")


if __name__ == "__main__":
    main()
