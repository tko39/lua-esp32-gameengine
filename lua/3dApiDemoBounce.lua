-----------------------------------------
-- Bouncing 3D Icosahedra in full 3D space
-----------------------------------------
math.randomseed(millis() % 1000000)

-----------------------------------------
-- 3D setup: camera + light
-----------------------------------------
local FOV = 220
local CAM_DISTANCE = 180

-- These are *world-space* radii now (3D sphere radius)
local RADIUS_MIN = 12
local RADIUS_MAX = 22

local LIGHT_AMBIENT = 0.2
local LIGHT_DIFFUSE = 0.8
local light_dx = 0
local light_dy = 1
local light_dz = -1

lge.set_3d_camera(FOV, CAM_DISTANCE)
-- Initial light from above and slightly towards the camera
lge.set_3d_light(light_dx, light_dy, light_dz, LIGHT_AMBIENT, LIGHT_DIFFUSE)

local SCREEN_W, SCREEN_H = lge.get_canvas_size()

-----------------------------------------
-- World bounds in 3D (camera at (0,0,0), looking along +Z)
-----------------------------------------
local WORLD_X_MIN, WORLD_X_MAX = -80, 80
local WORLD_Y_MIN, WORLD_Y_MAX = -60, 60
local WORLD_Z_MIN, WORLD_Z_MAX = 80, 260 -- must stay >0 (in front of camera)

-----------------------------------------
-- Icosahedron model (normalized)
-----------------------------------------
local vertices = {0.000000, -0.525731, 0.850651, 0.850651, 0.000000, 0.525731, 0.850651, 0.000000, -0.525731, -0.850651,
                  0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.525731, 0.850651, 0.000000, 0.525731, 0.850651,
                  0.000000, 0.525731, -0.850651, 0.000000, -0.525731, -0.850651, 0.000000, 0.000000, -0.525731,
                  -0.850651, 0.000000, 0.525731, -0.850651, 0.000000, 0.525731, 0.850651}

-- Faces as triangles, 1-based
local faces = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12, 1,
               2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}

-- Create model in C++ renderer
local MODEL_ID = lge.create_3d_model(vertices, faces)

-- Simple repeating palette (avoid blues for your 3-3-2 setup)
local palette = {"#00ff00", -- green
"#ff0000", -- red
"#ffff00" -- yellow
}

-- Helper to build per-triangle colors
local function make_tri_colors()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        tri_colors[i] = palette[(i - 1) % #palette + 1]
    end
    return tri_colors
end

-----------------------------------------
-- Projection: world (x,y,z,r3d) -> screen (sx,sy,sr)
-----------------------------------------
local function project_object(o)
    -- Distance from camera; keep in front of camera
    local depth = o.z
    if depth < 1 then
        depth = 1
    end

    local factor = FOV / depth

    local sx = o.x * factor + SCREEN_W / 2
    local sy = o.y * factor + SCREEN_H / 2
    local sr = o.r * factor -- apparent radius on screen

    return sx, sy, sr
end

-----------------------------------------
-- Bouncing object definition (full 3D)
-----------------------------------------
local NUM_OBJS = 4
local MAX_VEL = 4.0
local RESTORE_RATE = 0.05

local objects = {}

local function rand_range(a, b)
    return a + math.random() * (b - a)
end

local function new_object()
    local radius = rand_range(RADIUS_MIN, RADIUS_MAX)

    -- Random position inside world box (respecting radius)
    local x = rand_range(WORLD_X_MIN + radius, WORLD_X_MAX - radius)
    local y = rand_range(WORLD_Y_MIN + radius, WORLD_Y_MAX - radius)
    local z = rand_range(WORLD_Z_MIN + radius, WORLD_Z_MAX - radius)

    -- Random non-zero velocity in 3D
    local function non_zero_vel()
        local v = (math.random() * 2 - 1) * MAX_VEL
        if math.abs(v) < 0.5 then
            v = (v >= 0 and 1 or -1) * 0.5
        end
        return v
    end

    local dx = non_zero_vel()
    local dy = non_zero_vel()
    local dz = non_zero_vel()

    local tri_colors = make_tri_colors()
    local instance_id = lge.create_3d_instance(MODEL_ID, tri_colors)

    local obj = {
        -- 3D position / velocity
        x = x,
        y = y,
        z = z,
        dx = dx,
        dy = dy,
        dz = dz,
        r = radius, -- 3D radius in world units

        orig_mag_dx = math.abs(dx),
        orig_mag_dy = math.abs(dy),
        orig_mag_dz = math.abs(dz),

        -- 3D rotation state
        angle_x = math.random() * 2 * math.pi,
        angle_y = math.random() * 2 * math.pi,
        angle_z = math.random() * 2 * math.pi,

        d_angle_x = (math.random() - 0.5) * 0.05,
        d_angle_y = (math.random() - 0.5) * 0.05,
        d_angle_z = (math.random() - 0.5) * 0.05,

        instance_id = instance_id
    }

    return obj
end

for i = 1, NUM_OBJS do
    objects[i] = new_object()
end

-----------------------------------------
-- Physics functions (3D)
-----------------------------------------
local function update_object(o)
    -- Position
    o.x = o.x + o.dx
    o.y = o.y + o.dy
    o.z = o.z + o.dz

    -- Rotation
    o.angle_x = o.angle_x + o.d_angle_x
    o.angle_y = o.angle_y + o.d_angle_y
    o.angle_z = o.angle_z + o.d_angle_z

    -- Velocity clamp
    if o.dx > MAX_VEL then
        o.dx = MAX_VEL
    end
    if o.dx < -MAX_VEL then
        o.dx = -MAX_VEL
    end
    if o.dy > MAX_VEL then
        o.dy = MAX_VEL
    end
    if o.dy < -MAX_VEL then
        o.dy = -MAX_VEL
    end
    if o.dz > MAX_VEL then
        o.dz = MAX_VEL
    end
    if o.dz < -MAX_VEL then
        o.dz = -MAX_VEL
    end

    -- Gradual restore towards original magnitudes (per axis)
    local mag_dx = math.abs(o.dx)
    local mag_dy = math.abs(o.dy)
    local mag_dz = math.abs(o.dz)

    mag_dx = mag_dx + (o.orig_mag_dx - mag_dx) * RESTORE_RATE
    mag_dy = mag_dy + (o.orig_mag_dy - mag_dy) * RESTORE_RATE
    mag_dz = mag_dz + (o.orig_mag_dz - mag_dz) * RESTORE_RATE

    local sign_dx = (o.dx >= 0) and 1 or -1
    local sign_dy = (o.dy >= 0) and 1 or -1
    local sign_dz = (o.dz >= 0) and 1 or -1

    o.dx = sign_dx * mag_dx
    o.dy = sign_dy * mag_dy
    o.dz = sign_dz * mag_dz
end

local function check_wall_collision_3d(o)
    -- X
    if (o.x - o.r < WORLD_X_MIN and o.dx < 0) then
        o.dx = -o.dx
        o.x = WORLD_X_MIN + o.r
    elseif (o.x + o.r > WORLD_X_MAX and o.dx > 0) then
        o.dx = -o.dx
        o.x = WORLD_X_MAX - o.r
    end

    -- Y
    if (o.y - o.r < WORLD_Y_MIN and o.dy < 0) then
        o.dy = -o.dy
        o.y = WORLD_Y_MIN + o.r
    elseif (o.y + o.r > WORLD_Y_MAX and o.dy > 0) then
        o.dy = -o.dy
        o.y = WORLD_Y_MAX - o.r
    end

    -- Z
    if (o.z - o.r < WORLD_Z_MIN and o.dz < 0) then
        o.dz = -o.dz
        o.z = WORLD_Z_MIN + o.r
    elseif (o.z + o.r > WORLD_Z_MAX and o.dz > 0) then
        o.dz = -o.dz
        o.z = WORLD_Z_MAX - o.r
    end
end

local function check_object_collision_3d(o1, o2)
    local dx = o2.x - o1.x
    local dy = o2.y - o1.y
    local dz = o2.z - o1.z
    local dist_sq = dx * dx + dy * dy + dz * dz
    local min_dist = o1.r + o2.r

    if dist_sq <= min_dist * min_dist and dist_sq > 0.0001 then
        local dist = math.sqrt(dist_sq)
        local overlap = min_dist - dist

        local nx = dx / dist
        local ny = dy / dist
        local nz = dz / dist

        -- Separate
        o1.x = o1.x - nx * overlap * 0.5
        o1.y = o1.y - ny * overlap * 0.5
        o1.z = o1.z - nz * overlap * 0.5

        o2.x = o2.x + nx * overlap * 0.5
        o2.y = o2.y + ny * overlap * 0.5
        o2.z = o2.z + nz * overlap * 0.5

        -- Relative velocity
        local rvx = o2.dx - o1.dx
        local rvy = o2.dy - o1.dy
        local rvz = o2.dz - o1.dz
        local v_dot_n = rvx * nx + rvy * ny + rvz * nz

        -- Elastic collision response
        if v_dot_n < 0 then
            local impulse = 2 * v_dot_n
            o1.dx = o1.dx + nx * impulse
            o1.dy = o1.dy + ny * impulse
            o1.dz = o1.dz + nz * impulse

            o2.dx = o2.dx - nx * impulse
            o2.dy = o2.dy - ny * impulse
            o2.dz = o2.dz - nz * impulse
        end
    end
end

-----------------------------------------
-- Main loop
-----------------------------------------
local function main_loop()
    local clear_canvas = lge.clear_canvas
    local draw_text = lge.draw_text
    local fps_func = lge.fps
    local present = lge.present
    local delay = lge.delay

    while true do
        clear_canvas("#000000")

        ---------------------------------
        -- 0. Update light from touch
        ---------------------------------
        local _, mx, my = lge.get_mouse_position()
        if mx then
            local dx = (mx - SCREEN_W / 2) / (SCREEN_W / 2)
            local dy = (SCREEN_H / 2 - my) / (SCREEN_H / 2)
            local dz = -0.8

            local len = math.sqrt(dx * dx + dy * dy + dz * dz)
            if len > 0.0001 then
                light_dx = dx / len
                light_dy = dy / len
                light_dz = dz / len
                lge.set_3d_light(light_dx, light_dy, light_dz, LIGHT_AMBIENT, LIGHT_DIFFUSE)
            end
        end

        ---------------------------------
        -- 1. Update physics
        ---------------------------------
        for i = 1, NUM_OBJS do
            update_object(objects[i])
        end

        for i = 1, NUM_OBJS do
            check_wall_collision_3d(objects[i])
        end

        for i = 1, NUM_OBJS - 1 do
            local oi = objects[i]
            for j = i + 1, NUM_OBJS do
                check_object_collision_3d(oi, objects[j])
            end
        end

        ---------------------------------
        -- 2. Sort objects by depth (far → near)
        ---------------------------------
        table.sort(objects, function(a, b)
            return a.z > b.z
        end)

        ---------------------------------
        -- 3. Draw 3D instances via C++ renderer
        ---------------------------------
        for i = 1, NUM_OBJS do
            local o = objects[i]
            -- local sx, sy, sr = project_object(o)
            -- lge.draw_3d_instance(o.instance_id, sx, sy, sr, o.angle_x, o.angle_y, o.angle_z)
            lge.draw_3d_instance(o.instance_id, o.x, o.y, o.z, o.r, o.angle_x, o.angle_y, o.angle_z)
        end

        ---------------------------------
        -- 4. UI
        ---------------------------------
        local fps = fps_func()
        fps = math.floor(fps * 100 + 0.5) / 100
        draw_text(5, 5, "3D Icosa FPS: " .. fps, "#FFFFFF")

        present()
        delay(1)
    end
end

main_loop()
