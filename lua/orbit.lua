-----------------------------------------
-- Sun + Two Planets + Moon Orbit (Icosahedra)
-- - Full 3D orbits in X–Z plane
-- - Light follows the sun
-- - Orbit plane tilts with touch
-----------------------------------------
math.randomseed(millis() % 1000000)

-----------------------------------------
-- Camera + light setup
-----------------------------------------
local FOV = 220
local CAM_DISTANCE = 180

lge.set_3d_camera(FOV, CAM_DISTANCE)

local LIGHT_AMBIENT = 0.25
local LIGHT_DIFFUSE = 0.9

-- Will be updated every frame to follow the sun
local light_dx, light_dy, light_dz = 0.0, 1.0, -1.0
lge.set_3d_light(light_dx, light_dy, light_dz, LIGHT_AMBIENT, LIGHT_DIFFUSE)

local SCREEN_W, SCREEN_H = lge.get_canvas_size()

-----------------------------------------
-- Icosahedron model
-----------------------------------------
local vertices = {0.000000, -0.525731, 0.850651, 0.850651, 0.000000, 0.525731, 0.850651, 0.000000, -0.525731, -0.850651,
                  0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.525731, 0.850651, 0.000000, 0.525731, 0.850651,
                  0.000000, 0.525731, -0.850651, 0.000000, -0.525731, -0.850651, 0.000000, 0.000000, -0.525731,
                  -0.850651, 0.000000, 0.525731, -0.850651, 0.000000, 0.525731, 0.850651}

local faces = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12, 1,
               2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}

local MODEL_ID = lge.create_3d_model(vertices, faces)

-----------------------------------------
-- Colors for sun, planets, moon
-----------------------------------------
local function make_colors_sun()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        tri_colors[i] = (i % 2 == 0) and "#ffcc33" or "#ff9933"
    end
    return tri_colors
end

local function make_colors_planet1()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        tri_colors[i] = (i % 2 == 0) and "#00ff80" or "#00ffaa"
    end
    return tri_colors
end

local function make_colors_planet2()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        tri_colors[i] = (i % 2 == 0) and "#ff66ff" or "#ff99ff"
    end
    return tri_colors
end

local function make_colors_moon()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        tri_colors[i] = (i % 2 == 0) and "#dddddd" or "#bbbbbb"
    end
    return tri_colors
end

local SUN_INSTANCE_ID = lge.create_3d_instance(MODEL_ID, make_colors_sun())
local PLANET1_INSTANCE_ID = lge.create_3d_instance(MODEL_ID, make_colors_planet1())
local PLANET2_INSTANCE_ID = lge.create_3d_instance(MODEL_ID, make_colors_planet2())
local MOON_INSTANCE_ID = lge.create_3d_instance(MODEL_ID, make_colors_moon())

-----------------------------------------
-- Projection helper: world (x,y,z,r3d) -> screen (sx,sy,sr)
-----------------------------------------
local function project_point(x, y, z, radius3d)
    local depth = z
    if depth < 1 then
        depth = 1
    end

    local factor = FOV / depth

    local sx = x * factor + SCREEN_W / 2
    local sy = y * factor + SCREEN_H / 2
    local sr = radius3d * factor

    return sx, sy, sr
end

-----------------------------------------
-- Sun + Planets + Moon state
-----------------------------------------
local SUN_Z = 180
local SUN_RADIUS = 28

local ORBIT_RADIUS_1 = 70
local ORBIT_RADIUS_2 = 110

local PLANET1_RADIUS = 12
local PLANET2_RADIUS = 9

local ORBIT_SPEED_1 = 0.02 -- radians per frame
local ORBIT_SPEED_2 = -0.013 -- opposite direction, slower

local MOON_ORBIT_RADIUS = 20
local MOON_RADIUS = 5
local MOON_ORBIT_SPEED = 0.08

local sun = {
    x = 0.0,
    y = 0.0,
    z = SUN_Z,
    r = SUN_RADIUS,
    angle_x = 0,
    angle_y = 0,
    angle_z = 0,
    d_angle_x = 0.01,
    d_angle_y = 0.013,
    d_angle_z = 0.008,
    instance_id = SUN_INSTANCE_ID
}

local planet1 = {
    orbit_angle = 0.0,
    x = 0.0,
    y = 0.0,
    z = SUN_Z + ORBIT_RADIUS_1,
    r = PLANET1_RADIUS,
    angle_x = 0,
    angle_y = 0,
    angle_z = 0,
    d_angle_x = 0.03,
    d_angle_y = 0.017,
    d_angle_z = 0.025,
    instance_id = PLANET1_INSTANCE_ID
}

local planet2 = {
    orbit_angle = math.pi * 0.4, -- phase offset
    x = 0.0,
    y = 0.0,
    z = SUN_Z + ORBIT_RADIUS_2,
    r = PLANET2_RADIUS,
    angle_x = 0,
    angle_y = 0,
    angle_z = 0,
    d_angle_x = 0.02,
    d_angle_y = 0.019,
    d_angle_z = 0.03,
    instance_id = PLANET2_INSTANCE_ID
}

local moon = {
    orbit_angle = 0.0,
    x = 0.0,
    y = 0.0,
    z = 0.0,
    r = MOON_RADIUS,
    angle_x = 0,
    angle_y = 0,
    angle_z = 0,
    d_angle_x = 0.04,
    d_angle_y = 0.03,
    d_angle_z = 0.05,
    instance_id = MOON_INSTANCE_ID
}

-----------------------------------------
-- Orbit plane tilt (controlled by mouse/touch)
-----------------------------------------
local MAX_TILT = math.rad(45) -- max 45° tilt in each axis
local tiltX = 0.0 -- rotation around X (pitch)
local tiltY = 0.0 -- rotation around Y (yaw)

local function update_tilt_from_mouse()
    local _, mx, my = lge.get_mouse_position()
    if not mx then
        return
    end

    -- Normalize to [-1,1]
    local nx = (mx - SCREEN_W / 2) / (SCREEN_W / 2)
    local ny = (my - SCREEN_H / 2) / (SCREEN_H / 2)

    -- Horizontal: yaw (around Y), Vertical: pitch (around X)
    tiltY = nx * MAX_TILT
    tiltX = ny * MAX_TILT
end

-- Apply tilt to a local orbit-space point (around origin), then translate by sun position
local function apply_orbit_tilt(local_x, local_y, local_z)
    -- Rotate around X (tiltX)
    local cosX = math.cos(tiltX)
    local sinX = math.sin(tiltX)

    local y1 = local_y * cosX - local_z * sinX
    local z1 = local_y * sinX + local_z * cosX
    local x1 = local_x

    -- Rotate around Y (tiltY)
    local cosY = math.cos(tiltY)
    local sinY = math.sin(tiltY)

    local x2 = x1 * cosY + z1 * sinY
    local z2 = -x1 * sinY + z1 * cosY
    local y2 = y1

    -- Translate to sun position
    return sun.x + x2, sun.y + y2, sun.z + z2
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
        -- 0. Update tilt from touch + light from sun
        ---------------------------------
        update_tilt_from_mouse()

        ---------------------------------
        -- 1. Update sun rotation
        ---------------------------------
        sun.angle_x = sun.angle_x + sun.d_angle_x
        sun.angle_y = sun.angle_y + sun.d_angle_y
        sun.angle_z = sun.angle_z + sun.d_angle_z

        ---------------------------------
        -- 2. Update planet orbits + rotations
        ---------------------------------

        -- Planet 1
        planet1.orbit_angle = planet1.orbit_angle + ORBIT_SPEED_1
        local px1 = math.cos(planet1.orbit_angle) * ORBIT_RADIUS_1
        local py1 = 0
        local pz1 = math.sin(planet1.orbit_angle) * ORBIT_RADIUS_1
        planet1.x, planet1.y, planet1.z = apply_orbit_tilt(px1, py1, pz1)

        planet1.angle_x = planet1.angle_x + planet1.d_angle_x
        planet1.angle_y = planet1.angle_y + planet1.d_angle_y
        planet1.angle_z = planet1.angle_z + planet1.d_angle_z

        -- Planet 2
        planet2.orbit_angle = planet2.orbit_angle + ORBIT_SPEED_2
        local px2 = math.cos(planet2.orbit_angle) * ORBIT_RADIUS_2
        local py2 = 0
        local pz2 = math.sin(planet2.orbit_angle) * ORBIT_RADIUS_2
        planet2.x, planet2.y, planet2.z = apply_orbit_tilt(px2, py2, pz2)

        planet2.angle_x = planet2.angle_x + planet2.d_angle_x
        planet2.angle_y = planet2.angle_y + planet2.d_angle_y
        planet2.angle_z = planet2.angle_z + planet2.d_angle_z

        -- Moon orbiting Planet 1 in the same tilted plane
        moon.orbit_angle = moon.orbit_angle + MOON_ORBIT_SPEED
        local mpx = px1 + math.cos(moon.orbit_angle) * MOON_ORBIT_RADIUS
        local mpy = 0
        local mpz = pz1 + math.sin(moon.orbit_angle) * MOON_ORBIT_RADIUS
        moon.x, moon.y, moon.z = apply_orbit_tilt(mpx, mpy, mpz)

        moon.angle_x = moon.angle_x + moon.d_angle_x
        moon.angle_y = moon.angle_y + moon.d_angle_y
        moon.angle_z = moon.angle_z + moon.d_angle_z

        ---------------------------------
        -- 3. Depth sort (far → near)
        ---------------------------------
        local bodies = {sun, planet1, planet2, moon}
        table.sort(bodies, function(a, b)
            return a.z > b.z
        end)

        ---------------------------------
        -- 4. Draw all bodies
        ---------------------------------
        for i = 1, #bodies do
            local o = bodies[i]
            local sx, sy, sr = project_point(o.x, o.y, o.z, o.r)
            lge.draw_3d_instance(o.instance_id, sx, sy, sr, o.angle_x, o.angle_y, o.angle_z)
        end

        ---------------------------------
        -- 5. UI
        ---------------------------------
        local fps = fps_func()
        fps = math.floor(fps * 100 + 0.5) / 100
        draw_text(5, 5, "Orbit demo FPS: " .. fps, "#FFFFFF")
        draw_text(5, 20, "Touch/drag = tilt orbit plane", "#AAAAAA")

        present()
        delay(1)
    end
end

main_loop()
