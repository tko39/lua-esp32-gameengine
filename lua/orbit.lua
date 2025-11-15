-----------------------------------------
-- 3D Sun + Planet Orbit (Icosahedra)
-- Uses C++ 3D renderer (draw_3d_instance)
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

-- Light from above and slightly towards the camera
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

-- Faces as triangles, 1-based
local faces = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12, 1,
               2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}

local MODEL_ID = lge.create_3d_model(vertices, faces)

-----------------------------------------
-- Colors for sun and planet
-----------------------------------------
local function make_colors_sun()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        -- warm yellows/oranges
        tri_colors[i] = (i % 2 == 0) and "#ffcc33" or "#ff9933"
    end
    return tri_colors
end

local function make_colors_planet()
    local tri_colors = {}
    local face_count = #faces / 3
    for i = 1, face_count do
        -- greens / cyans (avoid dark blues)
        tri_colors[i] = (i % 2 == 0) and "#00ff80" or "#00ffaa"
    end
    return tri_colors
end

local SUN_INSTANCE_ID = lge.create_3d_instance(MODEL_ID, make_colors_sun())
local PLANET_INSTANCE_ID = lge.create_3d_instance(MODEL_ID, make_colors_planet())

-----------------------------------------
-- Simple world / projection helpers
-----------------------------------------
local function project_point(x, y, z, radius3d)
    -- z is distance from camera; must be > 0
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
-- Sun + Planet state
-----------------------------------------
local SUN_Z = 180 -- where the sun sits in front of the camera
local SUN_RADIUS = 25 -- world radius

local ORBIT_RADIUS = 70 -- distance of planet from sun in x-z plane
local PLANET_RADIUS = 12 -- world radius of planet
local ORBIT_SPEED = 0.02 -- radians per frame

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

local planet = {
    orbit_angle = 0.0,
    x = 0.0,
    y = 0.0,
    z = SUN_Z + ORBIT_RADIUS,
    r = PLANET_RADIUS,
    angle_x = 0,
    angle_y = 0,
    angle_z = 0,
    d_angle_x = 0.03,
    d_angle_y = 0.017,
    d_angle_z = 0.025,
    instance_id = PLANET_INSTANCE_ID
}

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
        -- 1. Update sun rotation
        ---------------------------------
        sun.angle_x = sun.angle_x + sun.d_angle_x
        sun.angle_y = sun.angle_y + sun.d_angle_y
        sun.angle_z = sun.angle_z + sun.d_angle_z

        ---------------------------------
        -- 2. Update planet orbit + rotation
        ---------------------------------
        planet.orbit_angle = planet.orbit_angle + ORBIT_SPEED

        -- Orbit in X–Z plane around the sun
        local a = planet.orbit_angle
        planet.x = math.cos(a) * ORBIT_RADIUS
        planet.z = SUN_Z + math.sin(a) * ORBIT_RADIUS

        -- Small vertical wobble so it's not dead-center on screen
        planet.y = math.sin(a * 2.0) * 10.0

        planet.angle_x = planet.angle_x + planet.d_angle_x
        planet.angle_y = planet.angle_y + planet.d_angle_y
        planet.angle_z = planet.angle_z + planet.d_angle_z

        ---------------------------------
        -- 3. Depth sort: draw farther first
        ---------------------------------
        local draw_order = {sun, planet}
        table.sort(draw_order, function(a, b)
            return a.z > b.z -- farther (bigger z) first
        end)

        ---------------------------------
        -- 4. Draw both bodies
        ---------------------------------
        for i = 1, #draw_order do
            local o = draw_order[i]
            local sx, sy, sr = project_point(o.x, o.y, o.z, o.r)
            lge.draw_3d_instance(o.instance_id, sx, sy, sr, o.angle_x, o.angle_y, o.angle_z)
        end

        ---------------------------------
        -- 5. UI
        ---------------------------------
        local fps = fps_func()
        fps = math.floor(fps * 100 + 0.5) / 100
        draw_text(5, 5, "Orbit demo FPS: " .. fps, "#FFFFFF")

        present()
        delay(1)
    end
end

main_loop()
