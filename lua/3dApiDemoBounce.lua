-----------------------------------------
-- Bouncing 3D Icosahedra with C++ 3D renderer
-----------------------------------------
math.randomseed(millis() % 1000000)

-----------------------------------------
-- 3D setup: camera + light
-----------------------------------------
local FOV = 220
local CAM_DISTANCE = 180
local RADIUS_MIN = 25 -- 2D radius on screen
local RADIUS_MAX = 40

local LIGHT_AMBIENT = 0.2
local LIGHT_DIFFUSE = 0.8
local light_dx = 0
local light_dy = 1
local light_dz = -1

lge.set_3d_camera(FOV, CAM_DISTANCE)
-- Light from above and slightly towards the camera
lge.set_3d_light(light_dx, light_dy, light_dz, LIGHT_AMBIENT, LIGHT_DIFFUSE)

local SCREEN_W, SCREEN_H = lge.get_canvas_size()

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
-- Bouncing object definition
-----------------------------------------
local NUM_OBJS = 3
local MAX_VEL = 6
local RESTORE_RATE = 0.05

local objects = {}

local function new_object()
    local radius = math.random(RADIUS_MIN, RADIUS_MAX)

    local x = math.random(radius, SCREEN_W - radius)
    local y = math.random(radius, SCREEN_H - radius)

    -- random non-zero velocity
    local dx = math.random() * MAX_VEL * (math.random() > 0.5 and 1 or -1)
    if math.abs(dx) < 1 then
        dx = dx * 2 * (dx >= 0 and 1 or -1)
    end
    local dy = math.random() * MAX_VEL * (math.random() > 0.5 and 1 or -1)
    if math.abs(dy) < 1 then
        dy = dy * 2 * (dy >= 0 and 1 or -1)
    end

    local tri_colors = make_tri_colors()
    local instance_id = lge.create_3d_instance(MODEL_ID, tri_colors)

    local obj = {
        x = x,
        y = y,
        dx = dx,
        dy = dy,
        r = radius,

        orig_mag_dx = math.abs(dx),
        orig_mag_dy = math.abs(dy),

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
-- Physics functions (same spirit as cubes)
-----------------------------------------
local function update_object(o)
    -- Position
    o.x = o.x + o.dx
    o.y = o.y + o.dy

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

    -- Gradual restore towards original magnitudes
    local mag_dx = math.abs(o.dx)
    local mag_dy = math.abs(o.dy)
    mag_dx = mag_dx + (o.orig_mag_dx - mag_dx) * RESTORE_RATE
    mag_dy = mag_dy + (o.orig_mag_dy - mag_dy) * RESTORE_RATE

    local sign_dx = (o.dx >= 0) and 1 or -1
    local sign_dy = (o.dy >= 0) and 1 or -1
    o.dx = sign_dx * mag_dx
    o.dy = sign_dy * mag_dy
end

local function check_wall_collision(o, w, h)
    -- X-axis
    if (o.x - o.r < 0 and o.dx < 0) then
        o.dx = -o.dx
        o.x = o.r
    elseif (o.x + o.r > w and o.dx > 0) then
        o.dx = -o.dx
        o.x = w - o.r
    end

    -- Y-axis
    if (o.y - o.r < 0 and o.dy < 0) then
        o.dy = -o.dy
        o.y = o.r
    elseif (o.y + o.r > h and o.dy > 0) then
        o.dy = -o.dy
        o.y = h - o.r
    end
end

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

        -- Separate them
        o1.x = o1.x - nx * overlap * 0.5
        o1.y = o1.y - ny * overlap * 0.5
        o2.x = o2.x + nx * overlap * 0.5
        o2.y = o2.y + ny * overlap * 0.5

        -- Relative velocity
        local vx1, vy1 = o1.dx, o1.dy
        local vx2, vy2 = o2.dx, o2.dy
        local rv_x = vx2 - vx1
        local rv_y = vy2 - vy1
        local v_dot_n = rv_x * nx + rv_y * ny

        -- Elastic collision response
        if v_dot_n < 0 then
            local impulse = 2 * v_dot_n
            o1.dx = vx1 + nx * impulse
            o1.dy = vy1 + ny * impulse
            o2.dx = vx2 - nx * impulse
            o2.dy = vy2 - ny * impulse
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

        local _, mx, my = lge.get_mouse_position()
        if mx then
            local dx = (mx - SCREEN_W / 2) / (SCREEN_W / 2)
            local dy = (SCREEN_H / 2 - my) / (SCREEN_H / 2)
            local dz = -0.8

            -- Normalize (dx, dy, dz)
            local len = math.sqrt(dx * dx + dy * dy + dz * dz)
            if len > 0.0001 then
                light_dx = dx / len
                light_dy = dy / len
                light_dz = dz / len

                lge.set_3d_light(light_dx, light_dy, light_dz, LIGHT_AMBIENT, LIGHT_DIFFUSE)
            end

        end

        -- 1. Update
        for i = 1, NUM_OBJS do
            update_object(objects[i])
        end

        -- 2. Wall collisions
        for i = 1, NUM_OBJS do
            check_wall_collision(objects[i], SCREEN_W, SCREEN_H)
        end

        -- 3. Object-object collisions (unique pairs)
        for i = 1, NUM_OBJS - 1 do
            local oi = objects[i]
            for j = i + 1, NUM_OBJS do
                check_object_collision(oi, objects[j])
            end
        end

        -- 4. Draw 3D instances via C++ renderer
        for i = 1, NUM_OBJS do
            local o = objects[i]
            lge.draw_3d_instance(o.instance_id, o.x, o.y, o.r, o.angle_x, o.angle_y, o.angle_z)
        end

        -- 5. UI
        local fps = fps_func()
        fps = math.floor(fps * 100 + 0.5) / 100
        draw_text(5, 5, "Icosa FPS: " .. fps, "#FFFFFF")

        present()
        delay(1)
    end
end

main_loop()
