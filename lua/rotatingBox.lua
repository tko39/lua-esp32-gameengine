-- 
--     3D Rotating Cube in Lua with LGE API
--     - Software-only rendering
--     - Perspective Projection
--     - Back-face Culling
--     - Depth Sorting (Painter's Algorithm)
--     - Colors in #RRGGBB format
--     - OPTIMIZED: Pre-calculated trig and inlined rotations
---------------------------------
--  🌎 Global Variables
---------------------------------
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

---------------------------------
--  Main Loop Functions
---------------------------------

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

    -- **OPTIMIZATION 1: Pre-calculate trig values**
    -- These are constant for all vertices in this frame.
    local cos_x = math.cos(angle_x)
    local sin_x = math.sin(angle_x)
    local cos_y = math.cos(angle_y)
    local sin_y = math.sin(angle_y)
    local cos_z = math.cos(angle_z)
    local sin_z = math.sin(angle_z)

    -- **Step 1 & 2: Transform and Project all vertices**
    for i = 1, #vertices do
        local v = vertices[i]

        -- **OPTIMIZATION 2: Inlined rotations**
        -- We apply the rotations directly here to avoid
        -- function call overhead and, more importantly,
        -- to prevent creating 3 new tables per vertex.

        -- Rotate X
        local y1 = v.y * cos_x - v.z * sin_x
        local z1 = v.y * sin_x + v.z * cos_x

        -- Rotate Y (using results from Rotate X)
        local x2 = v.x * cos_y + z1 * sin_y
        local z2 = -v.x * sin_y + z1 * cos_y

        -- Rotate Z (using results from Rotate Y)
        -- 'px, py, pz' are the final transformed point coordinates
        local px = x2 * cos_z - y1 * sin_z
        local py = x2 * sin_z + y1 * cos_z
        local pz = z2 -- z-coordinate is unchanged by z-rotation

        -- Translate cube away from camera
        pz = pz + distance

        -- **Step 3: Perspective Projection**
        -- This is the core 3D -> 2D conversion
        local z_factor = fov / pz

        local screen_x = px * z_factor + center_x
        local screen_y = py * z_factor + center_y

        -- Store the projected 2D point AND its original z-depth
        -- We only create ONE new table per vertex here.
        transformed_vertices[i] = {
            x = screen_x,
            y = screen_y,
            z = pz -- Store the *projected* z for depth sorting
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

        -- **Optimization: Back-Face Culling**
        local normal_z = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)

        if normal_z < 0 then
            -- **Optimization: Depth Sorting (Painter's Algorithm)**
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
        lge.draw_triangle(f.v1.x, f.v1.y, f.v2.x, f.v2.y, f.v3.x, f.v3.y, f.color)
    end

    -- Add some help text
    local fps = math.floor(lge.fps() * 100 + 0.5) / 100
    lge.draw_text(10, 10, "FPS: " .. fps, "#C8C8C8")
end

---------------------------------
--  Main Program Loop
---------------------------------

function main_loop()
    lge.draw_text(center_x - 50, center_y, "Loading...", "#FFFFFF")
    lge.present()
    lge.delay(500)

    while true do
        update()
        draw()

        -- Present the frame and delay
        lge.present()
        lge.delay(1) -- Yield CPU to the OS
    end
end

-- Start the application
main_loop()
-- End of rotatingBox.lua
