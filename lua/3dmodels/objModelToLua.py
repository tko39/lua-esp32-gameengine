#!/usr/bin/env python3
import sys
import math
from pathlib import Path

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} input.obj output.lua")
    sys.exit(1)

in_path = Path(sys.argv[1])
out_path = Path(sys.argv[2])

vertices = []  # list of (x,y,z)
faces = []  # list of [i1, i2, i3] (1-based indices)

with in_path.open("r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue

        if line.startswith("v "):  # vertex
            _, x, y, z = line.split(maxsplit=3)
            vertices.append((float(x), float(y), float(z)))

        elif line.startswith("f "):  # face
            parts = line.split()[1:]
            # tokens like "3", "3/1/2", "3//2", etc. -> take the first number
            idxs = []
            for p in parts:
                first = p.split("/")[0]
                if first:
                    idxs.append(int(first))
            if len(idxs) < 3:
                continue
            # triangulate via fan
            i0 = idxs[0]
            for k in range(1, len(idxs) - 1):
                i1 = idxs[k]
                i2 = idxs[k + 1]
                faces.append([i0, i1, i2])

if not vertices or not faces:
    print("No vertices or faces parsed!")
    sys.exit(1)

# --- recenter + normalize to radius 1 ---
xs = [v[0] for v in vertices]
ys = [v[1] for v in vertices]
zs = [v[2] for v in vertices]

cx = (min(xs) + max(xs)) * 0.5
cy = (min(ys) + max(ys)) * 0.5
cz = (min(zs) + max(zs)) * 0.5

# shift to center
vertices_centered = []
for x, y, z in vertices:
    vertices_centered.append((x - cx, y - cy, z - cz))

# compute max radius
max_r = 0.0
for x, y, z in vertices_centered:
    r = math.sqrt(x * x + y * y + z * z)
    if r > max_r:
        max_r = r

if max_r == 0:
    max_r = 1.0

scale = 1.0 / max_r

vertices_norm = [(x * scale, y * scale, z * scale) for (x, y, z) in vertices_centered]

# --- write Lua file ---
with out_path.open("w", encoding="utf-8") as out:
    out.write("-- Auto-generated from %s\n" % in_path.name)
    out.write("return {\n")

    out.write("  vertices = {\n")
    for x, y, z in vertices_norm:
        out.write("    %.6f, %.6f, %.6f,\n" % (x, y, z))
    out.write("  },\n\n")

    out.write("  faces = {\n")
    for i1, i2, i3 in faces:
        out.write("    %d, %d, %d,\n" % (i1, i2, i3))
    out.write("  },\n")

    out.write("}\n")

print("Wrote Lua model to", out_path)
