-- =====================================================
-- GEMFALL INFINITE - Phase I Implementation
-- A Match-3 Puzzle Game for LuaArduinoEngine
-- =====================================================
-- Seed random number generator
math.randomseed(os.time())

-- =====================================================
-- CONSTANTS & CONFIGURATION
-- =====================================================

local BOARD_SIZE = 8
local SCREEN_W, SCREEN_H = lge.get_canvas_size()

-- Gem 3D Model (Icosahedron)
local GEM_VERTICES = {0.000000, -0.525731, 0.850651, 0.850651, 0.000000, 0.525731, 0.850651, 0.000000, -0.525731,
                      -0.850651, 0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.525731, 0.850651, 0.000000,
                      0.525731, 0.850651, 0.000000, 0.525731, -0.850651, 0.000000, -0.525731, -0.850651, 0.000000,
                      0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.850651, 0.000000, 0.525731, 0.850651}

local GEM_FACES = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12,
                   1, 2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}

-- Gem Colors (7 total for Phase I)
local GEM_COLORS = {"#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF", "#FF8800"}

-- Gem Types
local GEM_NORMAL = 0
local GEM_LINE_CLEAR = 1
local GEM_BOMB = 2
local GEM_HYPER_CUBE = 3

-- Scoring Constants
local SCORE_MATCH_3 = 10
local SCORE_MATCH_4 = 20
local SCORE_MATCH_5 = 50
local MAX_CASCADE_MULTIPLIER = 10

-- Matching
local GEM_MATCH_START_SIZE = 0.5
local GEM_MATCH_SHRINK = 0.1

-- Difficulty Thresholds
local DIFFICULTY_THRESHOLDS = {{
    score = 0,
    colors = 5
}, {
    score = 10000,
    colors = 6
}, {
    score = 30000,
    colors = 7
}}

-- Visual Constants
local GEM_SIZE = math.min(SCREEN_W, SCREEN_H) / (BOARD_SIZE + 2)
local BOARD_OFFSET_X = (SCREEN_W - GEM_SIZE * BOARD_SIZE) / 2
local BOARD_OFFSET_Y = 40 -- Leave space for score
local GEM_3D_Z = 220 -- Increased Z distance
local GEM_3D_RADIUS = GEM_SIZE * 0.45 -- Increased radius

-- Camera & Lighting
local FOV = 220
local CAM_DISTANCE = 220
local LIGHT_AMBIENT = 0.3
local LIGHT_DIFFUSE = 0.7

-- Animation
local ANIM_SWAP_TIME = 0.2
local ANIM_MATCH_TIME = 0.15

-- Game States
local STATE_IDLE = 0
local STATE_SELECTED = 1
local STATE_SWAPPING = 2
local STATE_MATCHING = 3
local STATE_FALLING = 4
local STATE_GAME_OVER = 5

-- =====================================================
-- GAME STATE VARIABLES
-- =====================================================

local board = {}
local gem_types = {}
local gem_y_offsets = {} -- For falling animation
local gem_scales = {} -- For match animation
local score = 0
local best_score = 0
local current_colors = 5
local game_state = STATE_IDLE
local selected_x = nil
local selected_y = nil
local model_id = nil
local gem_instances = {}
local rotation_angles = {}

local swap_anim = {
    active = false,
    x1 = nil,
    y1 = nil,
    x2 = nil,
    y2 = nil,
    t = 0, -- progress 0..1
    valid = false
}
local SWAP_ANIM_DURATION = 0.18 -- seconds
local swap_anim_dx, swap_anim_dy = 0, 0

-- =====================================================
-- MEMORY OPTIMIZATION: PRE-ALLOCATED OBJECTS & POOLS
-- =====================================================

-- Flat array match detection system
-- Instead of nested tables, use flat arrays with index arithmetic
local MAX_MATCHES = 5 -- Maximum simultaneous matches we can detect
local MAX_GEMS_PER_MATCH = BOARD_SIZE -- Max gems in a single match

-- Flat arrays for match data
local match_sizes = {} -- Size of each match
local match_gem_x = {} -- X coordinates (flat: match_gem_x[(match_idx-1)*MAX_GEMS_PER_MATCH + gem_idx])
local match_gem_y = {} -- Y coordinates (flat: match_gem_y[(match_idx-1)*MAX_GEMS_PER_MATCH + gem_idx])

-- Pre-allocate flat arrays
for i = 1, MAX_MATCHES do
    match_sizes[i] = 0
end

for i = 1, MAX_MATCHES * MAX_GEMS_PER_MATCH do
    match_gem_x[i] = 0
    match_gem_y[i] = 0
end

-- Reusable checked array for match detection
local checked_array = {}
for y = 1, BOARD_SIZE do
    checked_array[y] = {}
    for x = 1, BOARD_SIZE do
        checked_array[y][x] = false
    end
end

-- Reusable arrays for horizontal/vertical gem collection
local h_gems_temp = {}
local v_gems_temp = {}
for i = 1, BOARD_SIZE do
    h_gems_temp[i] = 0
    v_gems_temp[i] = 0
end

-- Reusable match sizes array
local match_sizes_temp = {}
for i = 1, MAX_MATCHES do
    match_sizes_temp[i] = 0
end

-- Reusable matches result array
local matches_result = {}
for i = 1, MAX_MATCHES do
    matches_result[i] = nil
end

-- =====================================================
-- UTILITY FUNCTIONS
-- =====================================================

local function get_board_pos(x, y)
    local screen_x = BOARD_OFFSET_X + (x - 1) * GEM_SIZE + GEM_SIZE / 2
    local screen_y = BOARD_OFFSET_Y + (y - 1) * GEM_SIZE + GEM_SIZE / 2
    return screen_x, screen_y
end

local function screen_to_board(sx, sy)
    local x = math.floor((sx - BOARD_OFFSET_X) / GEM_SIZE) + 1
    local y = math.floor((sy - BOARD_OFFSET_Y) / GEM_SIZE) + 1
    if x >= 1 and x <= BOARD_SIZE and y >= 1 and y <= BOARD_SIZE then
        return x, y
    end
    return nil, nil
end

local function is_adjacent(x1, y1, x2, y2)
    local dx = math.abs(x1 - x2)
    local dy = math.abs(y1 - y2)
    return (dx == 1 and dy == 0) or (dx == 0 and dy == 1)
end

local function update_color_count()
    for i = #DIFFICULTY_THRESHOLDS, 1, -1 do
        if score >= DIFFICULTY_THRESHOLDS[i].score then
            current_colors = DIFFICULTY_THRESHOLDS[i].colors
            return
        end
    end
end

-- =====================================================
-- 3D MODEL SETUP
-- =====================================================

local function setup_3d_models()
    -- Setup camera and lighting
    lge.set_3d_camera(FOV, CAM_DISTANCE)
    lge.set_3d_light(0, 1, -0.5, LIGHT_AMBIENT, LIGHT_DIFFUSE)

    -- Create gem model
    model_id = lge.create_3d_model(GEM_VERTICES, GEM_FACES)

    -- Create instances for each color
    local num_faces = #GEM_FACES / 3
    for color_idx = 1, #GEM_COLORS do
        local tri_colors = {}
        for i = 1, num_faces do
            tri_colors[i] = GEM_COLORS[color_idx]
        end
        gem_instances[color_idx] = lge.create_3d_instance(model_id, tri_colors)
    end

    -- Initialize rotation angles for each board position
    for y = 1, BOARD_SIZE do
        rotation_angles[y] = {}
        for x = 1, BOARD_SIZE do
            rotation_angles[y][x] = {
                x = math.random() * math.pi * 2,
                y = math.random() * math.pi * 2,
                z = math.random() * math.pi * 2,
                dx = (math.random() - 0.5) * 0.02,
                dy = (math.random() - 0.5) * 0.02,
                dz = (math.random() - 0.5) * 0.02
            }
        end
    end
end

-- =====================================================
-- BOARD INITIALIZATION
-- =====================================================

local function create_empty_board()
    for y = 1, BOARD_SIZE do
        board[y] = {}
        gem_types[y] = {}
        gem_y_offsets[y] = {}
        gem_scales[y] = {}
        for x = 1, BOARD_SIZE do
            board[y][x] = 0
            gem_types[y][x] = GEM_NORMAL
            gem_y_offsets[y][x] = 0
            gem_scales[y][x] = 1.0
        end
    end
end

local function get_random_gem()
    return math.random(1, current_colors)
end

local function check_match_at(x, y, ignore_x, ignore_y)
    local color = board[y][x]
    if color == 0 then
        return false
    end

    -- Check horizontal
    local h_count = 1
    local tx = x - 1
    while tx >= 1 and board[y][tx] == color and not (tx == ignore_x and y == ignore_y) do
        h_count = h_count + 1
        tx = tx - 1
    end
    tx = x + 1
    while tx <= BOARD_SIZE and board[y][tx] == color and not (tx == ignore_x and y == ignore_y) do
        h_count = h_count + 1
        tx = tx + 1
    end

    -- Check vertical
    local v_count = 1
    local ty = y - 1
    while ty >= 1 and board[ty][x] == color and not (x == ignore_x and ty == ignore_y) do
        v_count = v_count + 1
        ty = ty - 1
    end
    ty = y + 1
    while ty <= BOARD_SIZE and board[ty][x] == color and not (x == ignore_x and ty == ignore_y) do
        v_count = v_count + 1
        ty = ty + 1
    end

    return h_count >= 3 or v_count >= 3
end

local function fill_board_no_matches()
    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            repeat
                board[y][x] = get_random_gem()
            until not check_match_at(x, y, -1, -1)
        end
    end
end

-- =====================================================
-- MATCH DETECTION (OPTIMIZED)
-- =====================================================

local function reset_checked_array()
    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            checked_array[y][x] = false
        end
    end
end

-- Helper function to get gem coordinates from flat array
local function get_match_gem(match_idx, gem_idx)
    local base = (match_idx - 1) * MAX_GEMS_PER_MATCH
    return match_gem_x[base + gem_idx], match_gem_y[base + gem_idx]
end

-- Helper function to set gem coordinates in flat array
local function set_match_gem(match_idx, gem_idx, x, y)
    local base = (match_idx - 1) * MAX_GEMS_PER_MATCH
    match_gem_x[base + gem_idx] = x
    match_gem_y[base + gem_idx] = y
end

local function find_matches()
    reset_checked_array()

    local match_count = 0

    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            if board[y][x] > 0 then
                local color = board[y][x]

                -- Check horizontal
                local h_count = 1
                h_gems_temp[1] = x
                local tx = x + 1
                while tx <= BOARD_SIZE and board[y][tx] == color do
                    h_count = h_count + 1
                    h_gems_temp[h_count] = tx
                    tx = tx + 1
                end

                if h_count >= 3 and match_count < MAX_MATCHES then
                    match_count = match_count + 1
                    match_sizes[match_count] = h_count

                    for i = 1, h_count do
                        set_match_gem(match_count, i, h_gems_temp[i], y)
                    end
                end

                -- Check vertical
                local v_count = 1
                v_gems_temp[1] = y
                local ty = y + 1
                while ty <= BOARD_SIZE and board[ty][x] == color do
                    v_count = v_count + 1
                    v_gems_temp[v_count] = ty
                    ty = ty + 1
                end

                if v_count >= 3 and match_count < MAX_MATCHES then
                    match_count = match_count + 1
                    match_sizes[match_count] = v_count

                    for i = 1, v_count do
                        set_match_gem(match_count, i, x, v_gems_temp[i])
                    end
                end
            end
        end
    end

    return match_count
end

local function remove_matches(match_count)
    local total_removed = 0
    local sizes_count = 0

    -- Mark all matched gems in a temp array
    local to_remove = {}
    for y = 1, BOARD_SIZE do
        to_remove[y] = {}
        for x = 1, BOARD_SIZE do
            to_remove[y][x] = false
        end
    end

    for i = 1, match_count do
        local match_size = match_sizes[i]
        sizes_count = sizes_count + 1
        match_sizes_temp[sizes_count] = match_size

        for j = 1, match_size do
            local gx, gy = get_match_gem(i, j)
            to_remove[gy][gx] = true
        end
    end

    -- Now shrink all matched gems only once
    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            if to_remove[y][x] and board[y][x] > 0 then
                gem_scales[y][x] = GEM_MATCH_START_SIZE
                total_removed = total_removed + 1
            end
        end
    end

    return total_removed, sizes_count
end

local function clear_matched_gems(match_count)
    -- Use the same logic as remove_matches to clear only once
    local to_remove = {}
    for y = 1, BOARD_SIZE do
        to_remove[y] = {}
        for x = 1, BOARD_SIZE do
            to_remove[y][x] = false
        end
    end

    for i = 1, match_count do
        local match_size = match_sizes[i]
        for j = 1, match_size do
            local gx, gy = get_match_gem(i, j)
            to_remove[gy][gx] = true
        end
    end

    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            if to_remove[y][x] then
                board[y][x] = 0
                gem_types[y][x] = GEM_NORMAL
                gem_scales[y][x] = 1.0
            end
        end
    end
end

-- =====================================================
-- GRAVITY & REFILL
-- =====================================================

local function apply_gravity()
    local moved = false

    for x = 1, BOARD_SIZE do
        local write_y = BOARD_SIZE
        for y = BOARD_SIZE, 1, -1 do
            if board[y][x] > 0 then
                if y ~= write_y then
                    board[write_y][x] = board[y][x]
                    gem_types[write_y][x] = gem_types[y][x]
                    board[y][x] = 0
                    gem_types[y][x] = GEM_NORMAL

                    -- Set falling animation offset
                    local distance = write_y - y
                    gem_y_offsets[write_y][x] = -distance * GEM_SIZE

                    moved = true
                end
                write_y = write_y - 1
            end
        end
    end

    return moved
end

local function animate_falling()
    local still_falling = false
    local fall_speed = GEM_SIZE * 0.5 -- pixels per frame

    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            if gem_y_offsets[y][x] < 0 then
                gem_y_offsets[y][x] = gem_y_offsets[y][x] + fall_speed
                if gem_y_offsets[y][x] > 0 then
                    gem_y_offsets[y][x] = 0
                end
                still_falling = true
            elseif gem_y_offsets[y][x] > 0 then
                gem_y_offsets[y][x] = 0
            end
        end
    end

    return still_falling
end

local function refill_board()
    local filled = false

    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            if board[y][x] == 0 then
                board[y][x] = get_random_gem()
                gem_types[y][x] = GEM_NORMAL
                gem_scales[y][x] = 1.0
                -- Animate from above
                gem_y_offsets[y][x] = -(BOARD_SIZE - y) * GEM_SIZE
                filled = true
            end
        end
    end

    return filled
end

-- =====================================================
-- MOVE VALIDATION
-- =====================================================

local function has_valid_moves()
    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            -- Try swap right
            if x < BOARD_SIZE then
                board[y][x], board[y][x + 1] = board[y][x + 1], board[y][x]
                local match1 = check_match_at(x, y, -1, -1)
                local match2 = check_match_at(x + 1, y, -1, -1)
                board[y][x], board[y][x + 1] = board[y][x + 1], board[y][x]

                if match1 or match2 then
                    return true
                end
            end

            -- Try swap down
            if y < BOARD_SIZE then
                board[y][x], board[y + 1][x] = board[y + 1][x], board[y][x]
                local match1 = check_match_at(x, y, -1, -1)
                local match2 = check_match_at(x, y + 1, -1, -1)
                board[y][x], board[y + 1][x] = board[y + 1][x], board[y][x]

                if match1 or match2 then
                    return true
                end
            end
        end
    end

    return false
end

local function would_create_match(x1, y1, x2, y2)
    -- Perform swap
    board[y1][x1], board[y2][x2] = board[y2][x2], board[y1][x1]

    -- Check for matches
    local match1 = check_match_at(x1, y1, -1, -1)
    local match2 = check_match_at(x2, y2, -1, -1)

    -- Swap back
    board[y1][x1], board[y2][x2] = board[y2][x2], board[y1][x1]

    return match1 or match2
end

-- =====================================================
-- SCORING (OPTIMIZED)
-- =====================================================

local function calculate_match_score(match_size, cascade_level)
    local base_score = 0

    if match_size == 3 then
        base_score = SCORE_MATCH_3
    elseif match_size == 4 then
        base_score = SCORE_MATCH_4
    elseif match_size >= 5 then
        base_score = SCORE_MATCH_5
    end

    local multiplier = math.min(cascade_level, MAX_CASCADE_MULTIPLIER)
    return base_score * multiplier
end

local function add_score(sizes_count, cascade_level)
    for i = 1, sizes_count do
        score = score + calculate_match_score(match_sizes_temp[i], cascade_level)
    end
    update_color_count()
end

-- =====================================================
-- RENDERING
-- =====================================================

local function update_rotations()
    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            local rot = rotation_angles[y][x]
            rot.x = rot.x + rot.dx
            rot.y = rot.y + rot.dy
            rot.z = rot.z + rot.dz

            -- Scale animation: prefer shrinking for matched gems (<= 0.1), otherwise grow back to 1.0
            if gem_scales[y][x] <= GEM_MATCH_START_SIZE and board[y][x] > 0 then
                -- Shrinking for match animation
                gem_scales[y][x] = gem_scales[y][x] - GEM_MATCH_SHRINK
                if gem_scales[y][x] < 0.01 then
                    gem_scales[y][x] = 0.01
                end
            elseif gem_scales[y][x] < 1.0 and board[y][x] > 0 then
                -- Smoothly grow back to full size
                gem_scales[y][x] = gem_scales[y][x] + 0.05
                if gem_scales[y][x] > 1.0 then
                    gem_scales[y][x] = 1.0
                end
            end
        end
    end
end

local function draw_board()
    for y = 1, BOARD_SIZE do
        for x = 1, BOARD_SIZE do
            local color_idx = board[y][x]
            if color_idx > 0 then
                local sx, sy = get_board_pos(x, y)
                sy = sy + gem_y_offsets[y][x]
                local rot = rotation_angles[y][x]
                local scale = GEM_3D_RADIUS * gem_scales[y][x]
                if selected_x == x and selected_y == y and not swap_anim.active then
                    scale = scale * 1.2
                end

                -- Animate swap if needed
                if swap_anim.active then
                    local t = swap_anim.t
                    -- Animate only the two swapping gems
                    if (x == swap_anim.x1 and y == swap_anim.y1) or (x == swap_anim.x2 and y == swap_anim.y2) then
                        local dirx = swap_anim.x2 - swap_anim.x1
                        local diry = swap_anim.y2 - swap_anim.y1
                        local anim_t = t
                        if not swap_anim.valid then
                            -- If invalid, animate to halfway then back
                            if t < 0.5 then
                                anim_t = t * 2
                            else
                                anim_t = (1 - t) * 2
                            end
                        end
                        if x == swap_anim.x1 and y == swap_anim.y1 then
                            sx = sx + dirx * GEM_SIZE * anim_t
                            sy = sy + diry * GEM_SIZE * anim_t
                        elseif x == swap_anim.x2 and y == swap_anim.y2 then
                            sx = sx - dirx * GEM_SIZE * anim_t
                            sy = sy - diry * GEM_SIZE * anim_t
                        end
                    end
                end

                lge.draw_3d_instance(gem_instances[color_idx], sx - SCREEN_W / 2, sy - SCREEN_H / 2, GEM_3D_Z, scale,
                    rot.x, rot.y, rot.z)
            end
        end
    end
end

local function draw_ui()
    -- Draw score - reuse string concatenation
    lge.draw_text(10, 10, "Score: " .. score, "#FFFFFF")
    lge.draw_text(10, 25, "Best: " .. best_score, "#FFFF00")

    -- Draw FPS
    local fps = lge.fps()
    fps = math.floor(fps * 100 + 0.5) / 100
    lge.draw_text(SCREEN_W - 80, 10, "FPS: " .. fps, "#00FF00")

    -- Draw color count indicator
    lge.draw_text(SCREEN_W - 100, 25, "Colors: " .. current_colors, "#00FFFF")
end

local function draw_game_over()
    local text = "GAME OVER"
    local text_x = SCREEN_W / 2 - 50
    local text_y = SCREEN_H / 2 - 20

    -- Background box
    lge.draw_rectangle(text_x - 20, text_y - 10, 140, 60, "#000000")
    lge.draw_rectangle(text_x - 18, text_y - 8, 136, 56, "#FF0000")
    lge.draw_rectangle(text_x - 16, text_y - 6, 132, 52, "#000000")

    lge.draw_text(text_x, text_y, text, "#FFFFFF")
    lge.draw_text(text_x - 10, text_y + 20, "Click to restart", "#FFFF00")
end

-- =====================================================
-- CASCADE SYSTEM (OPTIMIZED)
-- =====================================================

local function process_cascades()
    local cascade_level = 1

    while true do
        local match_count = find_matches()

        if match_count == 0 then
            break
        end

        -- Animate matches shrinking
        local removed, sizes_count = remove_matches(match_count)
        if removed > 0 then
            -- Show shrinking animation
            for i = 1, 8 do
                update_rotations()
                lge.clear_canvas("#1a1a2e")
                draw_board()
                draw_ui()
                lge.present()
                lge.delay(20)
            end

            -- Actually remove the gems
            clear_matched_gems(match_count)
            add_score(sizes_count, cascade_level)
        end

        -- Apply gravity and animate falling
        apply_gravity()

        -- Animate falling
        while animate_falling() do
            update_rotations()
            lge.clear_canvas("#1a1a2e")
            draw_board()
            draw_ui()
            lge.present()
            lge.delay(16)
        end

        -- Refill and animate new gems
        refill_board()

        -- Animate new gems falling into place
        while animate_falling() do
            update_rotations()
            lge.clear_canvas("#1a1a2e")
            draw_board()
            draw_ui()
            lge.present()
            lge.delay(16)
        end

        cascade_level = cascade_level + 1
        if cascade_level > MAX_CASCADE_MULTIPLIER then
            cascade_level = MAX_CASCADE_MULTIPLIER
        end
    end
end

-- =====================================================
-- ANIMATION HELPERS
-- =====================================================

local function start_swap_animation(x1, y1, x2, y2, valid)
    swap_anim.active = true
    swap_anim.x1, swap_anim.y1 = x1, y1
    swap_anim.x2, swap_anim.y2 = x2, y2
    swap_anim.t = 0
    swap_anim.valid = valid
    swap_anim_dx = x2 - x1
    swap_anim_dy = y2 - y1
end

local function update_swap_animation(dt)
    if not swap_anim.active then
        return false
    end
    swap_anim.t = swap_anim.t + dt / SWAP_ANIM_DURATION
    if swap_anim.t >= 1 then
        swap_anim.t = 1
        return true -- animation finished
    end
    return false
end

local function reset_swap_animation()
    swap_anim.active = false
    swap_anim.x1, swap_anim.y1, swap_anim.x2, swap_anim.y2 = nil, nil, nil, nil
    swap_anim.t = 0
    swap_anim.valid = false
end

-- =====================================================
-- INPUT HANDLING
-- =====================================================

local swap_pending = false
local swap_from_x, swap_from_y, swap_to_x, swap_to_y = nil, nil, nil, nil

local function handle_input()
    if swap_anim.active then
        return
    end -- ignore input during swap animation

    local button, mx, my = lge.get_mouse_click()
    if not mx then
        return
    end

    -- Game over - restart on click
    if game_state == STATE_GAME_OVER then
        game_state = STATE_IDLE
        selected_x = nil
        selected_y = nil
        score = 0
        current_colors = 5
        fill_board_no_matches()

        -- Ensure board has valid moves
        while not has_valid_moves() do
            fill_board_no_matches()
        end
        return
    end

    -- Normal gameplay
    if game_state == STATE_IDLE then
        local bx, by = screen_to_board(mx, my)
        if bx and board[by][bx] > 0 then
            selected_x = bx
            selected_y = by
            game_state = STATE_SELECTED
        end
    elseif game_state == STATE_SELECTED then
        local bx, by = screen_to_board(mx, my)
        if bx then
            if bx == selected_x and by == selected_y then
                -- Deselect
                selected_x = nil
                selected_y = nil
                game_state = STATE_IDLE
            elseif is_adjacent(selected_x, selected_y, bx, by) then
                -- Try swap: animate first
                local valid = would_create_match(selected_x, selected_y, bx, by)
                swap_from_x, swap_from_y = selected_x, selected_y
                swap_to_x, swap_to_y = bx, by
                swap_pending = valid
                start_swap_animation(selected_x, selected_y, bx, by, valid)
                selected_x = nil
                selected_y = nil
                game_state = STATE_SWAPPING
            else
                -- Select new gem
                if board[by][bx] > 0 then
                    selected_x = bx
                    selected_y = by
                end
            end
        end
    end
end

-- =====================================================
-- GAME LOOP
-- =====================================================

local function game_setup()
    setup_3d_models()
    create_empty_board()
    fill_board_no_matches()

    -- Ensure board has valid moves
    while not has_valid_moves() do
        fill_board_no_matches()
    end

    game_state = STATE_IDLE
    score = 0
    best_score = 0
end

local last_time = os.clock()

local function game_loop()
    local now = os.clock()
    local dt = now - last_time
    last_time = now

    -- Handle swapping animation
    if game_state == STATE_SWAPPING and swap_anim.active then
        local finished = update_swap_animation(dt)
        -- Render
        update_rotations()
        lge.clear_canvas("#1a1a2e")
        draw_board()
        draw_ui()
        if game_state == STATE_GAME_OVER then
            draw_game_over()
        end
        lge.present()
        lge.delay(16)
        if finished then
            reset_swap_animation()
            if swap_pending then
                -- Actually swap in board
                board[swap_from_y][swap_from_x], board[swap_to_y][swap_to_x] = board[swap_to_y][swap_to_x],
                    board[swap_from_y][swap_from_x]
                gem_types[swap_from_y][swap_from_x], gem_types[swap_to_y][swap_to_x] = gem_types[swap_to_y][swap_to_x],
                    gem_types[swap_from_y][swap_from_x]
                game_state = STATE_MATCHING
            else
                -- Invalid swap, return to idle
                game_state = STATE_IDLE
            end
            swap_pending = false
        end
        return
    end

    -- Handle matching state
    if game_state == STATE_MATCHING then
        process_cascades()

        -- Check for game over
        if not has_valid_moves() then
            game_state = STATE_GAME_OVER
            if score > best_score then
                best_score = score
            end
        else
            game_state = STATE_IDLE
        end
    end

    -- Update
    update_rotations()
    handle_input()

    -- Render
    lge.clear_canvas("#1a1a2e")
    draw_board()
    draw_ui()
    if game_state == STATE_GAME_OVER then
        draw_game_over()
    end
    lge.present()
    lge.delay(16)
end

-- =====================================================
-- MAIN ENTRY POINT
-- =====================================================

collectgarbage("generational", 10, 50)
game_setup()

local framesBeforeTest = 300

while true do
    game_loop()
    framesBeforeTest = framesBeforeTest - 1
    if framesBeforeTest == 0 then
        print(("Mem: %.1f KB"):format(collectgarbage("count")))
        framesBeforeTest = 300
    end

    if framesBeforeTest % 60 == 0 then
        collectgarbage("step", 10)
    end
end
