# `lge` API Reference

## Canvas Functions

### `lge.get_canvas_size() -> (width, height)`

Returns the width and height of the canvas in pixels.

```lua
local w, h = lge.get_canvas_size()
```

---

### `lge.clear_canvas(color | default "#000000")`

Clears the canvas to a solid color.

- `color`: String, hex RGB of the form `"#rrggbb"`. Default is black (`"#000000"`).

```lua
-- Clear to black
lge.clear_canvas()

-- Clear to dark gray
lge.clear_canvas("#202020")
```

---

## 2D Drawing Functions

### `lge.draw_circle(x, y, radius, color)`

Draws a **filled circle**.

- `x, y`: Center position in canvas coordinates.
- `radius`: Circle radius in pixels.
- `color`: String, `"#rrggbb"`.

```lua
lge.draw_circle(100, 100, 20, "#ff0000")
```

---

### `lge.draw_rectangle(x, y, width, height, color)`

Draws a **filled rectangle**.

- `x, y`: Top-left corner in canvas coordinates.
- `width, height`: Size in pixels.
- `color`: String, `"#rrggbb"`.

```lua
lge.draw_rectangle(100, 100, 20, 40, "#ff0000")
```

---

### `lge.draw_triangle(x0, y0, x1, y1, x2, y2, color)`

Draws a **filled triangle**.

- `(x0, y0)`, `(x1, y1)`, `(x2, y2)`: Vertex coordinates in canvas space.
- `color`: String, `"#rrggbb"`.

```lua
lge.draw_triangle(100, 100, 100, 200, 200, 200, "#ff0000")
```

---

### `lge.draw_text(x, y, text, color)`

Draws a string of text.

- `x, y`: Text position in canvas coordinates.
- `text`: Lua string.
- `color`: String, `"#rrggbb"`.

```lua
lge.draw_text(10, 20, "Score: 42", "#ffffff")
```

---

## 3D Rendering

### Coordinate System

- Camera is at `(0, 0, 0)`.
- Camera looks along the **+Z** axis.
- `x`: horizontal, `y`: vertical, `z`: depth.
- Objects with `z <= 0` are behind the camera and not visible.
- World units are arbitrary but consistent across position, bounds, and radius/scale.

---

### Camera & Projection

#### `lge.set_3d_camera(fov, cam_distance)`

Configures the 3D camera and projection parameters.

- `fov`: Projection factor / effective field-of-view (positive number).
- `cam_distance`: Distance / scale factor used in projection.

Call this at startup or whenever the camera configuration changes.

```lua
local FOV = 220
local CAM_DISTANCE = 180
lge.set_3d_camera(FOV, CAM_DISTANCE)
```

---

### Lighting

#### `lge.set_3d_light(dx, dy, dz, ambient, diffuse)`

Sets a single directional light.

- `dx, dy, dz`: Direction vector in world space. It is recommended that this be normalized.
- `ambient`: Ambient light strength (typically `0.0`–`1.0`).
- `diffuse`: Diffuse light strength (typically `0.0`–`1.0`).

```lua
-- Light from above and slightly toward the camera
lge.set_3d_light(0, 1, -1, 0.2, 0.8)
```

You can update the light dynamically (for example, based on mouse position) each frame.

---

### Models & Instances

#### `lge.create_3d_model(vertices, faces) -> model_id`

Registers a static 3D triangle mesh.

- `vertices`: Flat Lua array of numbers, grouped as `(x, y, z)` triples.
  Example: `{x1, y1, z1, x2, y2, z2, ...}`

- `faces`: Flat Lua array of integers, grouped as triangles `(i1, i2, i3)`.
  Indices are **1-based**, each index referring to a vertex triple in `vertices`.

Returns:

- `model_id`: An opaque identifier used with `lge.create_3d_instance`.

```lua
local vertices = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
}

local faces = {
    1, 2, 3,
}

local model_id = lge.create_3d_model(vertices, faces)
```

---

#### `lge.create_3d_instance(model_id, tri_colors) -> instance_id`

Creates a renderable instance of a 3D model with per-triangle colors.

- `model_id`: Returned from `lge.create_3d_model`.
- `tri_colors`: Lua array of strings, one color per triangle in the model.
  Length must equal `(#faces / 3)` for that model.

Returns:

- `instance_id`: Opaque identifier used with `lge.draw_3d_instance`.

```lua
local tri_colors = {
    "#ff0000",  -- triangle 1
    "#00ff00",  -- triangle 2
    "#0000ff",  -- triangle 3
}

local instance_id = lge.create_3d_instance(model_id, tri_colors)
```

---

### Drawing 3D Instances

#### `lge.draw_3d_instance(instance_id, x, y, z, radius, angle_x, angle_y, angle_z)`

Draws a 3D instance in the current frame.

- `instance_id`: From `lge.create_3d_instance`.
- `x, y, z`: World-space position of the instance’s center.
- `radius`: Uniform scale factor (world units).
- `angle_x, angle_y, angle_z`: Rotation about local X/Y/Z axes, in radians.

```lua
lge.draw_3d_instance(
    instance_id,
    0.0, 0.0, 150.0,  -- position
    20.0,             -- radius / scale
    0.0, 0.5, 0.0     -- rotation
)
```

Call this for each instance you want to render in a frame, after updating their positions and rotations.

---

## Input Functions

### `lge.get_mouse_click() -> (button, x, y) | nil`

Returns information about the most recent mouse click.

- `button`: Integer or engine-specific constant indicating which button was pressed.
- `x, y`: Click position in canvas coordinates.
- If no click is pending, returns `nil`.
- Once read, the stored click is cleared until a new click occurs.

```lua
local button, x, y = lge.get_mouse_click()
if x then
    -- Handle click at (x, y)
end
```

---

### `lge.get_mouse_position() -> (button, x, y) | nil`

Returns the current mouse position.

- `button`: Button state (if provided).
- `x, y`: Current cursor position in canvas coordinates.
- If the mouse position is unavailable, returns `nil`.

```lua
local button, x, y = lge.get_mouse_position()
if x then
    lge.draw_circle(x, y, 10, "#00ffff")
end
```

---

## Timing & Frame Control

### `lge.delay(ms)`

Delays execution for the given number of milliseconds, yielding the coroutine and allowing the engine to process events / timing.

- `ms`: Delay duration in milliseconds.

```lua
-- Small delay to pace the loop
lge.delay(1)
```

---

### `lge.present()`

Swaps or presents the current canvas buffer to the display.
Typically called once per frame, after all drawing calls.

```lua
while true do
    lge.clear_canvas("#000000")
    -- draw stuff...
    lge.present()
    lge.delay(1)
end
```

---

## Diagnostics / Utilities

### `lge.fps() -> number`

Returns the current frames-per-second as a floating-point value.

```lua
local fps = lge.fps()
fps = math.floor(fps * 100 + 0.5) / 100
lge.draw_text(5, 5, "FPS: " .. fps, "#ffffff")
```

---

# Quickstart: Simple Spinning 3D Triangle

This is a minimal example that:

- Sets up the camera and a light.
- Creates a simple 3D model (one triangle).
- Spins it in 3D and draws FPS text.

```lua
-- Seed any RNG you might have (optional)
-- math.randomseed(...)

-- 1. Setup
local SCREEN_W, SCREEN_H = lge.get_canvas_size()

-- Camera (camera at origin, looking along +Z)
local FOV = 220
local CAM_DISTANCE = 180
lge.set_3d_camera(FOV, CAM_DISTANCE)

-- Light: from above, slightly towards the camera
local LIGHT_AMBIENT = 0.2
local LIGHT_DIFFUSE = 0.8
lge.set_3d_light(0, 1, -1, LIGHT_AMBIENT, LIGHT_DIFFUSE)

-- 2. Create a simple 3D model: one triangle
local vertices = {
    -0.5, -0.5,  0.0,
     0.5, -0.5,  0.0,
     0.0,  0.5,  0.0,
}

local faces = {
    1, 2, 3,
}

local model_id = lge.create_3d_model(vertices, faces)

-- Per-triangle colors (only 1 triangle here)
local tri_colors = { "#ff0000" }
local instance_id = lge.create_3d_instance(model_id, tri_colors)

-- 3. Animation state
local angle_x = 0
local angle_y = 0
local angle_z = 0

-- Position it in front of the camera
local pos_x = 0
local pos_y = 0
local pos_z = 150
local radius = 40

-- 4. Main loop
while true do
    -- Clear background
    lge.clear_canvas("#000000")

    -- Update rotation
    angle_x = angle_x + 0.01
    angle_y = angle_y + 0.02
    angle_z = angle_z + 0.015

    -- Draw 3D instance
    lge.draw_3d_instance(
        instance_id,
        pos_x, pos_y, pos_z,  -- world position
        radius,               -- scale
        angle_x, angle_y, angle_z
    )

    -- Draw FPS
    local fps = lge.fps()
    fps = math.floor(fps * 100 + 0.5) / 100
    lge.draw_text(5, 5, "FPS: " .. fps, "#ffffff")

    -- Present frame
    lge.present()
    lge.delay(1)
end
```
