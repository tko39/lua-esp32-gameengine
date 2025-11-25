#!/usr/bin/env python3
import sys
import math
from pathlib import Path
import trimesh

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} input_mesh.(gltf|glb|obj|stl...) output.lua")
    sys.exit(1)

in_path = Path(sys.argv[1])
out_path = Path(sys.argv[2])

# --- load mesh via trimesh ---
mesh = trimesh.load_mesh(in_path, force='mesh')

if mesh.is_empty:
    print("Failed to load mesh or mesh is empty")
    sys.exit(1)

# Ensure we have a single mesh (if it's a Scene, merge geometry)
if isinstance(mesh, trimesh.Scene):
    mesh = trimesh.util.concatenate([g for g in mesh.geometry.values()])

vertices = mesh.vertices.tolist()  # list of [x,y,z]
faces = mesh.faces.tolist()  # list of [i1,i2,i3], 0-based

if not vertices or not faces:
    print("No vertices or faces parsed!")
    sys.exit(1)

# --- recenter + normalize to radius 1
xs = [v[0] for v in vertices]
ys = [v[1] for v in vertices]
zs = [v[2] for v in vertices]

cx = (min(xs) + max(xs)) * 0.5
cy = (min(ys) + max(ys)) * 0.5
cz = (min(zs) + max(zs)) * 0.5

vertices_centered = []
for x, y, z in vertices:
    vertices_centered.append((x - cx, y - cy, z - cz))

max_r = 0.0
for x, y, z in vertices_centered:
    r = math.sqrt(x * x + y * y + z * z)
    if r > max_r:
        max_r = r

if max_r == 0:
    max_r = 1.0

scale = 1.0 / max_r
vertices_norm = [(x * scale, y * scale, z * scale) for (x, y, z) in vertices_centered]

# trimesh faces are 0-based; your Lua / OBJ style is 1-based.
faces_1based = [(i1 + 1, i2 + 1, i3 + 1) for (i1, i2, i3) in faces]

# --- write Lua file ---
with out_path.open("w", encoding="utf-8") as out:
    out.write("-- Auto-generated from %s\n" % in_path.name)
    out.write("return {\n")

    out.write("  vertices = {\n")
    for x, y, z in vertices_norm:
        out.write("    %.6f, %.6f, %.6f,\n" % (x, y, z))
    out.write("  },\n\n")

    out.write("  faces = {\n")
    for i1, i2, i3 in faces_1based:
        out.write("    %d, %d, %d,\n" % (i1, i2, i3))
    out.write("  },\n")

    out.write("}\n")

print("Wrote Lua model to", out_path)
