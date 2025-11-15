math.randomseed(millis() % 1000000)

local FOV = 220
local CAM_DISTANCE = 180
local RADIUS = 70 -- 2D radius on screen

lge.set_3d_camera(FOV, CAM_DISTANCE)
lge.set_3d_light(0, 1, -1, 0.2, 0.8)

local SCREEN_W, SCREEN_H = lge.get_canvas_size()
local CX = SCREEN_W / 2
local CY = SCREEN_H / 2

local vertices = {0.000000, -0.525731, 0.850651, 0.850651, 0.000000, 0.525731, 0.850651, 0.000000, -0.525731, -0.850651,
                  0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.525731, 0.850651, 0.000000, 0.525731, 0.850651,
                  0.000000, 0.525731, -0.850651, 0.000000, -0.525731, -0.850651, 0.000000, 0.000000, -0.525731,
                  -0.850651, 0.000000, 0.525731, -0.850651, 0.000000, 0.525731, 0.850651}

local faces = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12, 1,
               2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}

-- Create model in C++ renderer
local MODEL_ID = lge.create_3d_model(vertices, faces)

-- Build a simple repeating palette. Blue's are bad for 3-3-2 color representation (8-bit)
local palette = {"#00ff00", "#ff0000", "#ffff00"}

-- Per-triangle colors
local tri_colors = {}
local face_count = #faces / 3
for i = 1, face_count do
    tri_colors[i] = palette[(i - 1) % #palette + 1]
end

-- Create one instance of the model
local INSTANCE_ID = lge.create_3d_instance(MODEL_ID, tri_colors)

-- Animation state
local angle_x = 0
local angle_y = 0
local angle_z = 0

local d_ax = 0.015
local d_ay = 0.02
local d_az = 0.007

local function main_loop()
    while true do
        lge.clear_canvas("#000000")

        -- Spin
        angle_x = angle_x + d_ax
        angle_y = angle_y + d_ay
        angle_z = angle_z + d_az

        -- Draw imported model
        lge.draw_3d_instance(INSTANCE_ID, CX, CY, RADIUS, angle_x, angle_y, angle_z)

        -- Overlay FPS
        local fps = lge.fps()
        fps = math.floor(fps * 100 + 0.5) / 100
        lge.draw_text(5, 5, "OBJ model FPS: " .. fps, "#FFFFFF")

        lge.present()
        lge.delay(1)
    end
end

main_loop()
