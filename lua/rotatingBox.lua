-- 
--     3D Rotating Cube in Lua with LGE API
--     - Software-only rendering
--     - Perspective Projection
--     - Back-face Culling
--     - Depth Sorting (Painter's Algorithm)
--     - Colors in #RRGGBB format
-- Helper functions for 3D rotation matrices
-- These take a point (a table {x, y, z}) and an angle (in radians)
-- and return a new, rotated point.
local function rotate_x(p, angle)
    local cos_a = math.cos(angle)
    local sin_a = math.sin(angle)
    return {
        x = p.x,
        y = p.y * cos_a - p.z * sin_a,
        z = p.y * sin_a + p.z * cos_a
    }
end

local function rotate_y(p, angle)
    local cos_a = math.cos(angle)
    local sin_a = math.sin(angle)
    return {
        x = p.x * cos_a + p.z * sin_a,
        y = p.y,
        z = -p.x * sin_a + p.z * cos_a
    }
end

local function rotate_z(p, angle)
    local cos_a = math.cos(angle)
    local sin_a = math.sin(angle)
    return {
        x = p.x * cos_a - p.y * sin_a,
        y = p.x * sin_a + p.y * cos_a,
        z = p.z
    }
end

---
-- ### 🌎 Global Variables
---

-- Canvas dimensions
local canvas_w, canvas_h = lge.get_canvas_size()
local center_x = canvas_w / 2
local center_y = canvas_h / 2

-- 3D Model Vertices (8 corners of a 1x1x1 cube)
-- We use 1-based indexing for Lua tables.
local vertices = {{
    x = -1,
    y = -1,
    z = -1
}, -- 1
{
    x = 1,
    y = -1,
    z = -1
}, -- 2
{
    x = 1,
    y = 1,
    z = -1
}, -- 3
{
    x = -1,
    y = 1,
    z = -1
}, -- 4
{
    x = -1,
    y = -1,
    z = 1
}, -- 5
{
    x = 1,
    y = -1,
    z = 1
}, -- 6
{
    x = 1,
    y = 1,
    z = 1
}, -- 7
{
    x = -1,
    y = 1,
    z = 1
} -- 8
}

-- 3D Model Faces (12 triangles, 2 per face)
-- Each entry is: {vertex1_idx, vertex2_idx, vertex3_idx, color_string}
-- Vertices are defined in counter-clockwise order (when viewed from outside)
-- This is important for back-face culling.
local faces = { -- Front face (Red)
{1, 4, 3, "#FF0000"}, {1, 3, 2, "#FF0000"}, -- Back face (Green)
{5, 6, 7, "#00FF00"}, {5, 7, 8, "#00FF00"}, -- Left face (Blue)
{1, 5, 8, "#0000FF"}, {1, 8, 4, "#0000FF"}, -- Right face (Yellow)
{2, 3, 7, "#FFFF00"}, {2, 7, 6, "#FFFF00"}, -- Top face (Magenta)
{4, 8, 7, "#FF00FF"}, {4, 7, 3, "#FF00FF"}, -- Bottom face (Cyan)
{1, 2, 6, "#00FFFF"}, {1, 6, 5, "#00FFFF"}}

-- Rotation angles
local angle_x = 0
local angle_y = 0
local angle_z = 0

-- Projection settings
local fov = 256 -- Field of View (acts as a zoom)
local distance = 5 -- Distance from camera to cube's center

---
-- ### ⚙️ Main Loop Functions
---

-- Update logic (called once per frame)
function update()
    -- Increment rotation angles
    angle_x = angle_x + 0.01
    angle_y = angle_y + 0.015
    angle_z = angle_z + 0.005
end

-- Draw logic (called once per frame)
function draw()
    lge.clear_canvas()

    local transformed_vertices = {}
    local faces_to_draw = {}

    -- **Step 1 & 2: Transform and Project all vertices**
    for i = 1, #vertices do
        local v = vertices[i]

        -- Apply rotations (in order: X, Y, Z)
        local p = rotate_x(v, angle_x)
        p = rotate_y(p, angle_y)
        p = rotate_z(p, angle_z)

        -- Translate cube away from camera
        p.z = p.z + distance

        -- **Step 3: Perspective Projection**
        -- This is the core 3D -> 2D conversion
        -- A point further away (larger p.z) will be divided by a larger
        -- number, making it smaller and closer to the center.
        local z_factor = fov / p.z

        local screen_x = p.x * z_factor + center_x
        local screen_y = p.y * z_factor + center_y

        -- Store the projected 2D point AND its original z-depth
        transformed_vertices[i] = {
            x = screen_x,
            y = screen_y,
            z = p.z
        }
    end

    -- **Step 4: Process faces for Culling and Sorting**
    for i = 1, #faces do
        local face = faces[i]
        local i1, i2, i3 = face[1], face[2], face[3]
        local color = face[4] -- This is now a string, e.g., "#FF0000"

        -- Get the 3 projected vertices for this face
        local v1 = transformed_vertices[i1]
        local v2 = transformed_vertices[i2]
        local v3 = transformed_vertices[i3]

        -- **Optimization 1: Back-Face Culling**
        -- We calculate the 2D cross-product of two edges of the triangle.
        -- The sign tells us if the triangle is "wound" clockwise or
        -- counter-clockwise on the screen.
        -- If it's (e.g.) clockwise, it's facing away, so we cull (skip) it.
        local normal_z = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)

        if normal_z < 0 then
            -- **Optimization 2: Depth Sorting (Painter's Algorithm)**
            -- Calculate average depth of the triangle
            local avg_z = (v1.z + v2.z + v3.z) / 3

            -- Add this face to a list to be sorted and drawn
            table.insert(faces_to_draw, {
                z = avg_z,
                v1 = v1,
                v2 = v2,
                v3 = v3,
                color = color
            })
        end
    end

    -- **Step 5: Sort the faces**
    -- Sort the list from FARTHEST to NEAREST (descending Z)
    table.sort(faces_to_draw, function(a, b)
        return a.z > b.z
    end)

    -- **Step 6: Render the sorted triangles**
    -- This draws the farthest faces first, so closer faces
    -- correctly draw over them.
    for i = 1, #faces_to_draw do
        local f = faces_to_draw[i]
        lge.draw_triangle(f.v1.x, f.v1.y, f.v2.x, f.v2.y, f.v3.x, f.v3.y, f.color -- Pass the color string directly
        )
    end

    -- Add some help text
    lge.draw_text(10, 10, "3D Rotating Cube - Software Rendered", "#FFFFFF")
    local fps = math.floor(lge.fps() * 100 + 0.5) / 100
    lge.draw_text(10, 30, "FPS: " .. fps, "#C8C8C8")
end

---
-- ### 🚀 Main Program Loop
---

function main_loop()
    lge.draw_text(center_x - 50, center_y, "Loading...", "#FFFFFF")
    lge.present()
    lge.delay(500)

    while true do
        update()
        draw()

        -- Present the frame and delay
        lge.present()
        lge.delay(1) -- Target ~60 FPS
    end
end

-- Start the application
main_loop()
