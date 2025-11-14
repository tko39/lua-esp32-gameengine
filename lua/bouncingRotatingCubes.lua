-- 
--     Bouncing 3D Cubes in Lua with LGE API
--     - Combines 2D physics with 3D software rendering.
-- 
---------------------------------
--  Cube "Class" Definition
---------------------------------
local Cube = {}
Cube.__index = Cube

-- Store the 3D model shape (vertices) once, statically.
-- These are 1x1x1 coordinates, which we will scale.
Cube.model_vertices = {{
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

-- Function to generate a random hex color
local function random_color()
    local r = math.random(50, 255) -- Avoid very dark colors
    local g = math.random(50, 255)
    local b = math.random(50, 255)
    return string.format("#%02x%02x%02x", r, g, b)
end

-- Constructor for a new Cube
-- We now pass in 'radius', which is the desired 2D collision/visual radius
function Cube:new(x, y, dx, dy, radius, max_vel, restore_rate, fov, cam_dist)
    local self = setmetatable({}, Cube)

    -- 2D Physics properties
    self.x = x
    self.y = y
    self.dx = dx
    self.dy = dy
    self.r = radius -- This is the collision radius
    self.orig_dx = dx
    self.orig_dy = dy
    self.MAX_VEL = max_vel
    self.RESTORE_RATE = restore_rate

    -- 3D Rotation properties
    self.angle_x = math.random() * 2 * math.pi
    self.angle_y = math.random() * 2 * math.pi
    self.angle_z = math.random() * 2 * math.pi

    -- Random rotation speeds
    self.d_angle_x = (math.random() - 0.5) * 0.05
    self.d_angle_y = (math.random() - 0.5) * 0.05
    self.d_angle_z = (math.random() - 0.5) * 0.05

    -- 3D Model Faces (instance-specific with random colors)
    local face_colors = {random_color(), -- Front
    random_color(), -- Back
    random_color(), -- Left
    random_color(), -- Right
    random_color(), -- Top
    random_color() -- Bottom
    }
    self.faces = {{1, 4, 3, face_colors[1]}, {1, 3, 2, face_colors[1]}, {5, 6, 7, face_colors[2]},
                  {5, 7, 8, face_colors[2]}, {1, 5, 8, face_colors[3]}, {1, 8, 4, face_colors[3]},
                  {2, 3, 7, face_colors[4]}, {2, 7, 6, face_colors[4]}, {4, 8, 7, face_colors[5]},
                  {4, 7, 3, face_colors[5]}, {1, 2, 6, face_colors[6]}, {1, 6, 5, face_colors[6]}}

    -- Projection settings
    self.fov = fov
    self.z_distance = cam_dist -- Use the constant distance

    -- Calculate the 3D model scale (`self.size`) needed
    -- to produce the desired 2D visual/collision radius (`self.r`).
    -- The formula is: visual_radius = model_scale * (fov / distance)
    -- So, rearranging: model_scale = visual_radius * (distance / fov)
    self.size = self.r * (self.z_distance / self.fov)

    --[[
    -- Trace the math for a small cube:
    -- r = 15, z_distance = 100, fov = 200
    -- self.size = 15 * (100 / 200) = 7.5
    -- 
    -- In draw():
    -- px (scaled vertex) will be ~ 1 * self.size = 7.5
    -- pz (distance) will be ~ self.z_distance = 100
    -- z_factor = fov / pz = 200 / 100 = 2
    -- projected_radius = px * z_factor = 7.5 * 2 = 15
    -- This matches self.r!
    --]]

    return self
end

-- Update the cube's internal state (position, rotation, velocity)
function Cube:update()
    -- 1. Update position
    self.x = self.x + self.dx
    self.y = self.y + self.dy

    -- 2. Update rotation
    self.angle_x = self.angle_x + self.d_angle_x
    self.angle_y = self.angle_y + self.d_angle_y
    self.angle_z = self.angle_z + self.d_angle_z

    -- 3. Velocity management
    if self.dx > self.MAX_VEL then
        self.dx = self.MAX_VEL
    end
    if self.dx < -self.MAX_VEL then
        self.dx = -self.MAX_VEL
    end
    if self.dy > self.MAX_VEL then
        self.dy = self.MAX_VEL
    end
    if self.dy < -self.MAX_VEL then
        self.dy = -self.MAX_VEL
    end

    local sign_dx = (self.dx >= 0) and 1 or -1
    local sign_dy = (self.dy >= 0) and 1 or -1
    local mag_dx = math.abs(self.dx) + (math.abs(self.orig_dx) - math.abs(self.dx)) * self.RESTORE_RATE
    local mag_dy = math.abs(self.dy) + (math.abs(self.orig_dy) - math.abs(self.dy)) * self.RESTORE_RATE
    self.dx = sign_dx * mag_dx
    self.dy = sign_dy * mag_dy
end

-- Draw the cube on screen (the 3D pipeline)
function Cube:draw()
    local transformed_vertices = {}
    local faces_to_draw = {}

    -- 1. Pre-calculate trig values
    local cos_x = math.cos(self.angle_x)
    local sin_x = math.sin(self.angle_x)
    local cos_y = math.cos(self.angle_y)
    local sin_y = math.sin(self.angle_y)
    local cos_z = math.cos(self.angle_z)
    local sin_z = math.sin(self.angle_z)

    -- 2. Transform and Project all vertices
    for i = 1, #Cube.model_vertices do
        local v_model = Cube.model_vertices[i]

        -- Apply calculated scale from self.size
        local v = {
            x = v_model.x * self.size,
            y = v_model.y * self.size,
            z = v_model.z * self.size
        }

        -- Inlined rotations
        local y1 = v.y * cos_x - v.z * sin_x
        local z1 = v.y * sin_x + v.z * cos_x
        local x2 = v.x * cos_y + z1 * sin_y
        local z2 = -v.x * sin_y + z1 * cos_y
        local px = x2 * cos_z - y1 * sin_z
        local py = x2 * sin_z + y1 * cos_z
        local pz = z2

        -- Translate cube away from camera using CONSTANT distance
        pz = pz + self.z_distance

        -- 3. Perspective Projection
        local z_factor = self.fov / pz

        -- Use self.x and self.y as the 2D center
        local screen_x = px * z_factor + self.x
        local screen_y = py * z_factor + self.y

        transformed_vertices[i] = {
            x = screen_x,
            y = screen_y,
            z = pz
        }
    end

    -- 4. Process faces for Culling and Sorting
    for i = 1, #self.faces do
        local face = self.faces[i]
        local i1, i2, i3 = face[1], face[2], face[3]
        local color = face[4]

        local v1 = transformed_vertices[i1]
        local v2 = transformed_vertices[i2]
        local v3 = transformed_vertices[i3]

        -- Back-Face Culling
        local normal_z = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)
        if normal_z < 0 then
            -- Depth Sorting (Painter's Algorithm)
            local avg_z = (v1.z + v2.z + v3.z) / 3
            table.insert(faces_to_draw, {
                z = avg_z,
                v1 = v1,
                v2 = v2,
                v3 = v3,
                color = color
            })
        end
    end

    -- 5. Sort the faces (farthest to nearest)
    table.sort(faces_to_draw, function(a, b)
        return a.z > b.z
    end)

    -- 6. Render the sorted triangles
    for i = 1, #faces_to_draw do
        local f = faces_to_draw[i]
        lge.draw_triangle(f.v1.x, f.v1.y, f.v2.x, f.v2.y, f.v3.x, f.v3.y, f.color)
    end
end

---------------------------------
--  Global Setup
---------------------------------

-- Define the number of cubes to generate
local NUM_CUBES = 4
local CUBE_MIN_R = 15 -- Renamed from _SIZE to _R (Radius)
local CUBE_MAX_R = 30 -- This is the desired 2D collision/visual radius
local CUBE_MAX_V = 6

local FOV = 200 -- Field of View (acts as a zoom)
local CAM_DISTANCE = 100 -- Constant distance from camera to z=0 plane

local MAX_VEL = 10 -- maximum allowed velocity for dx/dy
local RESTORE_RATE = 0.05 -- how quickly velocity returns to original (0.0-1.0)

-- Get canvas size once
local w, h = lge.get_canvas_size()

-- Initialize cubes table
if not cubes then
    cubes = {}
    -- Initialize multiple cubes
    for i = 1, NUM_CUBES do
        -- This is the desired 2D radius
        local radius = math.random(CUBE_MIN_R, CUBE_MAX_R)

        -- Ensure position is well within boundaries
        local x = math.random(radius, w - radius)
        local y = math.random(radius, h - radius)

        -- Generate non-zero random velocities
        local dx = math.random() * CUBE_MAX_V * (math.random() > 0.5 and 1 or -1)
        if math.abs(dx) < 1 then
            dx = dx * 2 * (dx >= 0 and 1 or -1)
        end
        local dy = math.random() * CUBE_MAX_V * (math.random() > 0.5 and 1 or -1)
        if math.abs(dy) < 1 then
            dy = dy * 2 * (dy >= 0 and 1 or -1)
        end

        -- Create the new cube object
        cubes[i] = Cube:new(x, y, dx, dy, radius, MAX_VEL, RESTORE_RATE, FOV, CAM_DISTANCE)
    end
end

---------------------------------
--  Physics Functions
---------------------------------

-- Function to handle wall collisions for a single object
local function check_wall_collision(obj, w, h)
    -- X-axis collision
    if (obj.x - obj.r < 0 and obj.dx < 0) then
        obj.dx = -obj.dx
        obj.x = obj.r
    elseif (obj.x + obj.r > w and obj.dx > 0) then
        obj.dx = -obj.dx
        obj.x = w - obj.r
    end
    -- Y-axis collision
    if (obj.y - obj.r < 0 and obj.dy < 0) then
        obj.dy = -obj.dy
        obj.y = obj.r
    elseif (obj.y + obj.r > h and obj.dy > 0) then
        obj.dy = -obj.dy
        obj.y = h - obj.r
    end
end

-- Function to handle elastic collision between two objects (o1 and o2)
local function check_object_collision(o1, o2)
    local dx = o2.x - o1.x
    local dy = o2.y - o1.y
    local dist_sq = dx * dx + dy * dy
    local min_dist = o1.r + o2.r

    if dist_sq <= min_dist * min_dist and dist_sq > 0.001 then
        local dist = math.sqrt(dist_sq)
        local overlap = min_dist - dist
        local nx = dx / dist
        local ny = dy / dist

        o1.x = o1.x - nx * overlap * 0.5
        o1.y = o1.y - ny * overlap * 0.5
        o2.x = o2.x + nx * overlap * 0.5
        o2.y = o2.y + ny * overlap * 0.5

        local vx1 = o1.dx
        local vy1 = o1.dy
        local vx2 = o2.dx
        local vy2 = o2.dy
        local rv_x = vx2 - vx1
        local rv_y = vy2 - vy1
        local v_dot_n = rv_x * nx + rv_y * ny

        if v_dot_n < 0 then
            local impulse = 2 * v_dot_n
            o1.dx = vx1 + nx * impulse
            o1.dy = vy1 + ny * impulse
            o2.dx = vx2 - nx * impulse
            o2.dy = vy2 - ny * impulse
        end
    end
end

---------------------------------
--  ⚙️ Main Program Loop
---------------------------------

while true do
    lge.clear_canvas()

    -- 1. Update cube internal states
    for _, cube in ipairs(cubes) do
        cube:update()
    end

    -- 2. Check wall collisions
    for _, cube in ipairs(cubes) do
        check_wall_collision(cube, w, h)
    end

    -- 3. Check all unique cube-to-cube collisions
    for i = 1, #cubes do
        for j = i + 1, #cubes do
            check_object_collision(cubes[i], cubes[j])
        end
    end

    -- 4. Draw cubes
    for _, cube in ipairs(cubes) do
        cube:draw()
    end

    -- 5. Draw UI
    local fps = math.floor(lge.fps() * 100 + 0.5) / 100
    lge.draw_text(5, 5, "FPS: " .. fps, "#FFFFFF")

    lge.present()
    lge.delay(1) -- This is used to determine maximal FPS
end
