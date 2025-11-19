-- bubblePop.lua
-- Casual bubble popping game using the lge.* API described in api2d.md
-- Controls: touches (mouse clicks) to pop bubbles. Single-file game.
-- Contract
-- Inputs: lge.get_mouse_click(), lge.get_mouse_position()
-- Outputs: drawing via lge.draw_* functions, lge.present(), lge.delay()
-- Success: game runs, spawns bubbles, popping increases score, combos, timer, restart.
---------------------------------
-- Game settings
local SPAWN_INTERVAL = 700 -- ms between spawns at base difficulty
local MIN_RADIUS = 10
local MAX_RADIUS = 40
local GRAVITY = 20 -- pixels per second^2 (bubbles gently float upward negative gravity)
local FLOAT_SPEED_MIN = -30 -- upward speed (px/s)
local FLOAT_SPEED_MAX = -80
local POP_PARTICLES = 12
local GAME_TIME = 45 * 1000 -- 45 seconds
local COMBO_WINDOW = 800 -- ms to continue a combo

-- Colors
local BG_COLOR = "#061226"
local BUBBLE_COLORS = {"#67e8f9", "#99f6e4", "#a78bfa", "#f472b6", "#fca5a5", "#fde68a"}
local TEXT_COLOR = "#ffffff"
local HUD_COLOR = "#ffffff"

-- State
local bubbles = {}
local particles = {}
local last_spawn = 0
local score = 0
local start_time = nil
local game_over = false
local combo = 0
local last_pop_time = 0
local best_combo = 0
local canvas_w, canvas_h = 240, 135 -- fallback, will query
local mouse_click = nil
local min_rise_y = 0

-- Game logic
-- difficulty / time toggles
local time_options = {30, 45, 60} -- seconds
local time_index = 2
local difficulty_options = {"Easy", "Normal", "Hard"}
local difficulty_index = 2

local base_spawn_interval = SPAWN_INTERVAL
local spawn_interval_local = base_spawn_interval
local float_speed_min_local = FLOAT_SPEED_MIN
local float_speed_max_local = FLOAT_SPEED_MAX

-- Utilities
local function rand(a, b)
    return math.random() * (b - a) + a
end

local function choose(tbl)
    return tbl[math.floor(rand(1, #tbl + 1))]
end

local function now_ms()
    -- engine provided millis() function
    return millis()
end

-- Bubble factory
local function spawn_bubble()
    local r = math.floor(rand(MIN_RADIUS, MAX_RADIUS))
    local x = math.floor(rand(r, canvas_w - r))
    local y = canvas_h + r + 2
    -- use difficulty-adjusted float speed bounds
    local a = (type(float_speed_min_local) == "number") and float_speed_min_local or FLOAT_SPEED_MIN
    local b = (type(float_speed_max_local) == "number") and float_speed_max_local or FLOAT_SPEED_MAX
    local minv, maxv = (a <= b) and a or b, (a <= b) and b or a
    local vy = rand(minv, maxv)
    local color = choose(BUBBLE_COLORS)
    local id = now_ms() .. ":" .. math.random()
    table.insert(bubbles, {
        x = x,
        y = y,
        r = r,
        vy = vy,
        color = color,
        id = id
    })
end

-- Particle factory
local function spawn_particles(x, y, color)
    for i = 1, POP_PARTICLES do
        local angle = rand(0, math.pi * 2)
        local speed = rand(30, 120)
        local vx = math.cos(angle) * speed
        local vy = math.sin(angle) * speed
        table.insert(particles, {
            x = x,
            y = y,
            vx = vx,
            vy = vy,
            life = rand(300, 700),
            color = color,
            born = now_ms()
        })
    end
end

local function distance_sq(x1, y1, x2, y2)
    local dx = x1 - x2
    local dy = y1 - y2
    return dx * dx + dy * dy
end

local function apply_difficulty()
    local d = difficulty_options[difficulty_index]
    if d == "Easy" then
        spawn_interval_local = base_spawn_interval * 1.6
        float_speed_min_local = FLOAT_SPEED_MIN * 0.7
        float_speed_max_local = FLOAT_SPEED_MAX * 0.7
    elseif d == "Normal" then
        spawn_interval_local = base_spawn_interval * 1.0
        float_speed_min_local = FLOAT_SPEED_MIN
        float_speed_max_local = FLOAT_SPEED_MAX
    else -- Hard
        spawn_interval_local = math.max(120, base_spawn_interval * 0.6)
        float_speed_min_local = FLOAT_SPEED_MIN * 1.3
        float_speed_max_local = FLOAT_SPEED_MAX * 1.3
    end
end

local function reset_game()
    bubbles = {}
    particles = {}
    last_spawn = now_ms()
    score = 0
    start_time = now_ms()
    game_over = false
    combo = 0
    last_pop_time = 0
    best_combo = 0
    canvas_w, canvas_h = lge.get_canvas_size()
    -- compute a minimum rise height (bubbles will keep rising until they reach this Y)
    min_rise_y = math.floor(canvas_h * 0.75)
    -- ensure local difficulty/time settings are applied
    apply_difficulty()
end

local function step(dt)
    local t = now_ms()
    -- spawn logic (difficulty scales with elapsed time)
    if not game_over then
        local elapsed = t - start_time
        -- dynamic difficulty scaling: slightly increase spawn rate over time
        local difficulty_scale = math.min(1.5, 1 + elapsed / 30000)
        local interval = math.max(120, spawn_interval_local / difficulty_scale)
        if t - last_spawn >= interval then
            spawn_bubble()
            last_spawn = t
        end
    end

    -- update bubbles
    for i = #bubbles, 1, -1 do
        local b = bubbles[i]
        b.vy = b.vy + (GRAVITY * (dt / 1000))
        -- force bubbles to continue rising until they reach min_rise_y (prevent early falling)
        if b.y > min_rise_y then
            -- if vy would be downward or too slow, boost it upward to a gentle rise speed
            local min_up_speed = -15
            if b.vy > min_up_speed then
                b.vy = min_up_speed
            end
        end
        b.y = b.y + b.vy * (dt / 1000)
        -- remove if off top of screen
        if b.y + b.r < 0 then
            table.remove(bubbles, i)
        end
    end

    -- update particles
    for i = #particles, 1, -1 do
        local p = particles[i]
        local age = t - p.born
        if age >= p.life then
            table.remove(particles, i)
        else
            p.vy = p.vy + (GRAVITY * 0.5 * (dt / 1000))
            p.x = p.x + p.vx * (dt / 1000)
            p.y = p.y + p.vy * (dt / 1000)
        end
    end

    -- check mouse click
    local btn, mx, my = lge.get_mouse_click()
    if mx then
        -- check for UI toggles top-right (time and difficulty)
        local btnW, btnH, pad = 68, 18, 6
        local bx2 = canvas_w - pad - btnW -- time button (right)
        local by = pad
        local bx1 = bx2 - pad - btnW -- difficulty button (left)
        -- time toggle
        if mx >= bx2 and mx <= bx2 + btnW and my >= by and my <= by + btnH then
            time_index = (time_index % #time_options) + 1
            -- update game time remaining proportionally
            local elapsed = now_ms() - start_time
            local new_total = time_options[time_index] * 1000
            -- clamp start_time so that remaining time scales to new duration
            start_time = now_ms() - math.min(elapsed, new_total)
        elseif mx >= bx1 and mx <= bx1 + btnW and my >= by and my <= by + btnH then
            difficulty_index = (difficulty_index % #difficulty_options) + 1
            apply_difficulty()
        else
            -- find topmost bubble under click
            local hit_index = nil
            for i = #bubbles, 1, -1 do
                local b = bubbles[i]
                if distance_sq(mx, my, b.x, b.y) <= (b.r * b.r) then
                    hit_index = i
                    break
                end
            end
            if hit_index then
                local b = table.remove(bubbles, hit_index)
                -- scoring: bigger bubble -> more points, combo multiplier
                local base = math.floor((MAX_RADIUS - b.r) * 1.2) + 10
                local tnow = now_ms()
                if tnow - last_pop_time <= COMBO_WINDOW then
                    combo = combo + 1
                else
                    combo = 1
                end
                last_pop_time = tnow
                best_combo = math.max(best_combo, combo)
                local points = base * combo
                score = score + points
                spawn_particles(b.x, b.y, b.color)
            end
        end
    end

    -- check game over (uses selected time option)
    local game_time = time_options[time_index] * 1000
    if not game_over and (now_ms() - start_time >= game_time) then
        game_over = true
    end
end

local function draw()
    -- clear
    lge.clear_canvas(BG_COLOR)

    -- draw bubbles
    for _, b in ipairs(bubbles) do
        -- soft outline: draw a slightly larger darker circle behind
        local outline_color = "#002233"
        lge.draw_circle(b.x, b.y + 2, b.r + 2, outline_color)
        lge.draw_circle(b.x, b.y, b.r, b.color)
        -- highlight
        local hx = b.x - b.r * 0.35
        local hy = b.y - b.r * 0.45
        lge.draw_circle(hx, hy, math.max(2, math.floor(b.r * 0.35)), "#ffffff")
    end

    -- draw particles
    for _, p in ipairs(particles) do
        local age = now_ms() - p.born
        local alpha = 1 - (age / p.life)
        local size = math.max(1, math.floor(2 * alpha))
        lge.draw_circle(p.x, p.y, size, p.color)
    end

    -- HUD: score and timer
    local game_time = time_options[time_index] * 1000
    local time_left = math.max(0, game_time - (now_ms() - start_time))
    local seconds = math.ceil(time_left / 1000)
    lge.draw_text(6, 6, string.format("Score: %d", score), HUD_COLOR)
    lge.draw_text(6, 20, string.format("Time: %ds", seconds), HUD_COLOR)
    lge.draw_text(6, 34, string.format("Combo: %d (best %d) ", combo, best_combo), HUD_COLOR)

    if game_over then
        local midx = canvas_w / 2
        local midy = canvas_h / 2
        lge.draw_rectangle(midx - 80, midy - 30, 160, 60, "#000000")
        lge.draw_text(midx - 60, midy - 10, "Game Over", TEXT_COLOR)
        lge.draw_text(midx - 60, midy + 8, string.format("Final Score: %d", score), TEXT_COLOR)
        lge.draw_text(midx - 60, midy + 26, "Click to Restart", TEXT_COLOR)
    end

    -- Draw toggles (top-right)
    local btnW, btnH, pad = 68, 18, 6
    local bx2 = canvas_w - pad - btnW -- time button (right)
    local by = pad
    local bx1 = bx2 - pad - btnW -- difficulty button (left)
    lge.draw_rectangle(bx1, by, btnW, btnH, "#002233")
    lge.draw_text(bx1 + 6, by + 4, "Diff: " .. difficulty_options[difficulty_index], "#ffffff")
    lge.draw_rectangle(bx2, by, btnW, btnH, "#002233")
    lge.draw_text(bx2 + 6, by + 4, "Time: " .. tostring(time_options[time_index]) .. "s", "#ffffff")

    lge.present()
end

function setup()
    math.randomseed(os.time())
    canvas_w, canvas_h = lge.get_canvas_size()
    reset_game()
end

function update()
    local last = now_ms()
    while true do
        local current = now_ms()
        local dt = current - last
        last = current
        if dt <= 0 then
            dt = 1
        end
        -- call frame update with dt
        do
            -- call the inner step(dt) function that advances one frame
            step(dt)
        end
        draw()
        -- if game over, wait for click to restart
        if game_over then
            local btn, mx, my = lge.get_mouse_click()
            if mx then
                reset_game()
            end
        end

        -- yield/frame pacing
        lge.delay(16) -- ~60fps
    end
end

setup()
update()
