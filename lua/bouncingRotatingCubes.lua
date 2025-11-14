-- 
--     Bouncing 3D Cubes in Lua with LGE API
--     - Combines 2D physics with 3D software rendering.
-- 
---------------------------------
--  Cube "Class" Definition
---------------------------------
local ENABLE_PROFILING = false

if ENABLE_PROFILING then
    local profile_data = {}
    local last_profiling_print = 0

    function profiling_start(name)
        if not profile_data[name] then
            profile_data[name] = {
                sum = 0,
                count = 0
            }
        end
        profile_data[name].start = millis()
    end

    function profiling_end(name)
        local now = millis()
        local entry = profile_data[name]
        if entry and entry.start then
            local elapsed = now - entry.start
            entry.sum = entry.sum + elapsed
            entry.count = entry.count + 1
            entry.start = nil
        end
    end

    function print_profiling_results()
        local now = millis()
        if now - last_profiling_print >= 5000 then
            last_profiling_print = now
            for name, data in pairs(profile_data) do
                local avg = data.sum / data.count
                print(string.format("%-30s total: %8.2f ms, avg:%8.2f ms, calls: %10d", name, data.sum, avg, data.count))
            end

            -- Reset profiling data
            profile_data = {}
        end
    end
else
    -- define empty no-op functions
    function profiling_start(name)
    end

    function profiling_end(name)
    end

    function print_profiling_results()
    end
end

local Cube = {}
Cube.__index = Cube
local lge_draw_triangle = lge.draw_triangle

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

function Cube:prepare_fast_representation()
    -- model_vertices -> flat array [x1,y1,z1,x2,y2,z2,...]
    local mv = self.model_vertices
    local n = #mv
    local flat = {}
    for i = 1, n do
        local v = mv[i]
        flat[(i - 1) * 3 + 1] = v.x
        flat[(i - 1) * 3 + 2] = v.y
        flat[(i - 1) * 3 + 3] = v.z
    end
    self.model_vertices_flat = flat
    self.model_vertices_count = n

    -- faces -> flat index list [i1,i2,i3, colorIndex, i1,i2,i3,colorIndex, ...]
    -- assume face is {i1,i2,i3,color}
    local faces = self.faces
    local fc = #faces
    local face_flat = {}
    for f = 1, fc do
        local face = faces[f]
        face_flat[(f - 1) * 4 + 1] = face[1]
        face_flat[(f - 1) * 4 + 2] = face[2]
        face_flat[(f - 1) * 4 + 3] = face[3]
        face_flat[(f - 1) * 4 + 4] = face[4] -- keep color as-is (could be index)
    end
    self.faces_flat = face_flat
    self.faces_count = fc

    -- preallocate transformed flat arrays (x,y,z per vertex)
    local tv = {}
    for i = 1, n * 3 do
        tv[i] = 0
    end
    self.transformed_vertices_flat = tv

    -- preallocate face-visible pools (numbers only)
    -- z_pool, i1_pool, i2_pool, i3_pool, color_pool
    self.visible_z = {}
    self.visible_b1 = {}
    self.visible_b2 = {}
    self.visible_b3 = {}
    self.visible_color = {}
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

    -- Optimizations - lower allocation count:
    self.transformed_vertices = {}
    for i = 1, #Cube.model_vertices do
        self.transformed_vertices[i] = {
            x = 0,
            y = 0,
            z = 0
        }
    end

    self:prepare_fast_representation()

    self.orig_mag_dx = math.abs(dx)
    self.orig_mag_dy = math.abs(dy)

    return self
end

-- Update the cube's internal state (position, rotation, velocity)
function Cube:update()
    profiling_start("Cube:update")
    local m_abs = math.abs
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

    local mag_dx = m_abs(self.dx)
    local mag_dy = m_abs(self.dy)

    local mag_dx = mag_dx + (self.orig_mag_dx - mag_dx) * self.RESTORE_RATE
    local mag_dy = mag_dy + (self.orig_mag_dy - mag_dy) * self.RESTORE_RATE
    self.dx = sign_dx * mag_dx
    self.dy = sign_dy * mag_dy
    profiling_end("Cube:update")
end

-- Optimizations for draw function
function Cube:draw_fast()
    profiling_start("Cube:draw_fast")

    -- Cache math functions
    local m_cos = math.cos
    local m_sin = math.sin

    -- locals / cache
    local mv_flat = self.model_vertices_flat
    local mv_count = self.model_vertices_count
    local faces_flat = self.faces_flat
    local faces_count = self.faces_count
    local tv = self.transformed_vertices_flat -- flat: [x1,y1,z1, x2,y2,z2, ...]
    local vis_z = self.visible_z
    local vis_b1 = self.visible_b1
    local vis_b2 = self.visible_b2
    local vis_b3 = self.visible_b3
    local vis_col = self.visible_color

    local size = self.size
    local x0 = self.x
    local y0 = self.y
    local z_dist = self.z_distance
    local fov = self.fov

    -- =======================================================
    profiling_start("draw_fast:Transform")
    -- =======================================================

    -- 1. Pre-calculate trig values
    local ax, ay, az = self.angle_x, self.angle_y, self.angle_z
    local cos_x, sin_x = m_cos(ax), m_sin(ax)
    local cos_y, sin_y = m_cos(ay), m_sin(ay)
    local cos_z, sin_z = m_cos(az), m_sin(az)

    -- 1) transform all vertices into tv flat array
    for i = 1, mv_count do
        local base = (i - 1) * 3
        -- Get model vertex and apply scale
        local vx = mv_flat[base + 1] * size
        local vy = mv_flat[base + 2] * size
        local vz = mv_flat[base + 3] * size

        -- Inlined rotations (Rx, then Ry, then Rz)
        local y1 = vy * cos_x - vz * sin_x
        local z1 = vy * sin_x + vz * cos_x
        local x2 = vx * cos_y + z1 * sin_y
        local z2 = -vx * sin_y + z1 * cos_y
        local px = x2 * cos_z - y1 * sin_z
        local py = x2 * sin_z + y1 * cos_z
        local pz = z2

        -- Translate and Project
        pz = pz + z_dist
        local z_factor = fov / pz

        -- Store screen coordinates
        tv[base + 1] = px * z_factor + x0
        tv[base + 2] = py * z_factor + y0
        tv[base + 3] = pz
    end

    -- =======================================================
    profiling_end("draw_fast:Transform")
    profiling_start("draw_fast:Cull")
    -- =======================================================

    -- 2) culling: fill visible_* pools (no allocations)
    local vis_count = 0
    for f = 1, faces_count do
        local fbase = (f - 1) * 4
        local i1 = faces_flat[fbase + 1]
        local i2 = faces_flat[fbase + 2]
        local i3 = faces_flat[fbase + 3]
        local color = faces_flat[fbase + 4]

        local b1 = (i1 - 1) * 3
        local b2 = (i2 - 1) * 3
        local b3 = (i3 - 1) * 3

        local v1x = tv[b1 + 1];
        local v1y = tv[b1 + 2];
        local v1z = tv[b1 + 3]
        local v2x = tv[b2 + 1];
        local v2y = tv[b2 + 2];
        local v2z = tv[b2 + 3]
        local v3x = tv[b3 + 1];
        local v3y = tv[b3 + 2];
        local v3z = tv[b3 + 3]

        -- compute 2D cross product for backface culling (screen-space)
        local normal_z = (v2x - v1x) * (v3y - v1y) - (v2y - v1y) * (v3x - v1x)
        if normal_z < 0 then
            vis_count = vis_count + 1
            vis_z[vis_count] = (v1z + v2z + v3z) * 0.3333333333
            vis_b1[vis_count] = b1
            vis_b2[vis_count] = b2
            vis_b3[vis_count] = b3
            vis_col[vis_count] = color
        end
    end

    -- =======================================================
    profiling_end("draw_fast:Cull")
    profiling_start("draw_fast:Sort")
    -- =======================================================

    -- 3) sort visible faces by z descending using insertion sort (fast for small N)
    if vis_count > 1 then
        for i = 2, vis_count do
            local zkey = vis_z[i]
            local k_b1 = vis_b1[i]
            local k_b2 = vis_b2[i]
            local k_b3 = vis_b3[i]
            local kcol = vis_col[i]

            local j = i - 1
            while j >= 1 and vis_z[j] < zkey do
                vis_z[j + 1] = vis_z[j]
                vis_b1[j + 1] = vis_b1[j]
                vis_b2[j + 1] = vis_b2[j]
                vis_b3[j + 1] = vis_b3[j]
                vis_col[j + 1] = vis_col[j]
                j = j - 1
            end
            vis_z[j + 1] = zkey
            vis_b1[j + 1] = k_b1
            vis_b2[j + 1] = k_b2
            vis_b3[j + 1] = k_b3
            vis_col[j + 1] = kcol
        end
    end

    -- =======================================================
    profiling_end("draw_fast:Sort")
    profiling_start("draw_fast:Render")
    -- =======================================================

    -- 4) draw visible faces
    for i = 1, vis_count do
        local b1 = vis_b1[i]
        local b2 = vis_b2[i]
        local b3 = vis_b3[i]
        lge_draw_triangle(tv[b1 + 1], tv[b1 + 2], tv[b2 + 1], tv[b2 + 2], tv[b3 + 1], tv[b3 + 2], vis_col[i])
    end

    -- =======================================================
    profiling_end("draw_fast:Render")
    profiling_end("Cube:draw_fast")
    -- =======================================================
end

-- Draw the cube on screen (the 3D pipeline)
function Cube:draw()
    profiling_start("Cube:draw")
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

    profiling_end("Cube:draw")
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
    profiling_start("check_wall_collision")
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
    profiling_end("check_wall_collision")
end

-- Function to handle elastic collision between two objects (o1 and o2)
local function check_object_collision(o1, o2)
    profiling_start("check_object_collision")
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
    profiling_end("check_object_collision")
end

---------------------------------
--  ⚙️ Main Program Loop
---------------------------------

local function main_loop()
    print("Starting Bouncing Rotating Cubes... Optimized main_loop")
    -- Cache globals and functions locally (massive win in Lua)
    local clear_canvas = lge.clear_canvas
    local draw_text = lge.draw_text
    local fps_func = lge.fps
    local present = lge.present
    local delay = lge.delay

    local check_wall_collision = check_wall_collision
    local check_object_collision = check_object_collision

    local profiling_start = profiling_start
    local profiling_end = profiling_end
    local print_profiling_results = print_profiling_results
    local cubes_ref = cubes -- fast local reference to the table
    local n = #cubes_ref -- number of cubes
    local ww = w
    local hh = h

    -- Optimized main loop
    while true do
        clear_canvas()

        profiling_start("main_loop")

        -- update
        for i = 1, n do
            cubes_ref[i]:update()
        end

        -- wall collision
        for i = 1, n do
            check_wall_collision(cubes_ref[i], ww, hh)
        end

        -------------------------------------------------------------------
        -- 3. Cube-to-cube collision (unique pairs)
        -------------------------------------------------------------------
        for i = 1, n - 1 do
            local ci = cubes_ref[i]
            for j = i + 1, n do
                check_object_collision(ci, cubes_ref[j])
            end
        end

        -------------------------------------------------------------------
        -- 4. Draw cubes
        -------------------------------------------------------------------
        for i = 1, n do
            cubes_ref[i]:draw_fast()
        end

        profiling_end("main_loop")

        -------------------------------------------------------------------
        -- 5. Draw UI
        -------------------------------------------------------------------
        local fps = fps_func()
        fps = math.floor(fps * 100 + 0.5) * 0.01
        draw_text(5, 5, "FPS: " .. fps, "#FFFFFF")

        print_profiling_results()
        present()
        delay(1)
    end
end

main_loop()
