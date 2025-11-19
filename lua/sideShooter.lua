------------------------------------------------------------
-- Starline (Lua + lge)
-- 320x240, black background, shapes only
-- Player on the left, moves up/down with mouse Y
-- Auto-fire, 3 lives, endless levels, powerups: Speed, Power
------------------------------------------------------------
-- ============ Config ============
local W, H = 320, 240

-- Player
local PLAYER_X = 14
local PLAYER_SPEED = 220 -- (not used if we follow touch/mouse)
local PLAYER_WIDTH = 10
local PLAYER_HEIGHT = 8
local PLAYER_IFRAME = 1.0 -- seconds of invulnerability after hit

-- Shots
local FIRE_RATE_BASE = 3.5 -- shots/sec
local FIRE_RATE_MAX = 10.0
local SHOT_SPEED = 280 -- px/s
local SHOT_W, SHOT_H = 5, 2
local DMG_BASE = 1
local DMG_MAX = 6

-- Enemy
local ENEMY_BASE_HP = 3 -- scales up with level
local ENEMY_SPEED_MIN = 55
local ENEMY_SPEED_MAX = 85

-- Enemy bullets
local ESHOT_SPEED = 140
local ESHOT_W, ESHOT_H = 3, 2

-- Level / Spawning
local LEVEL_TIME = 34.0 -- seconds per level
local SPAWN_INTERVAL_BASE = 0.90 -- seconds (decreases with levels)
local MAX_ENEMIES_ON_SCREEN = 26

-- Powerups
local DROP_CHANCE = 0.06
local DROP_PITY_N = 12 -- guarantee a drop if no drops in N kills
local PU_SIZE = 7
local PU_SPEED = 50
local MAGNET_BASE = 16 -- pixels
local MAGNET_GROWTH = 1.2 -- +per level
local PU_KIND = {
    SPEED = 1,
    POWER = 2
}

-- Misc
local FIXED_DT = 1 / 60
local FRAME_DELAY_MS = 16

-- Closest 8-bit approximations
local C = {
    WHITE = "#FFFFFF",
    GREY = "#B5B6AD",
    LIGHT = "#94DBFF",
    CYAN = "#00DBFF",
    BLUE = "#4A6DFF",
    GREEN = "#00FF5A",
    YELLOW = "#FFDB5A",
    ORANGE = "#FFB65A",
    RED = "#FF495A",
    DARKRED = "#B52400",
    PURPLE = "#B56DAD"
}

-- EASY MODE for testing
DMG_BASE = 4
ENEMY_BASE_HP = 1
SPAWN_INTERVAL_BASE = 1.5
MAX_ENEMIES_ON_SCREEN = 10
DROP_CHANCE = 0.2
--

-- ============ State ============
local player = {
    x = PLAYER_X,
    y = H / 2,
    lives = 3,
    fr = FIRE_RATE_BASE, -- shots/sec
    fire_cd = 0.0,
    dmg = DMG_BASE,
    iframe = 0.0,
    sideguns = 0 -- unlocks when fr crosses milestones (visual spice)
}

local bullets = {} -- player bullets
local ebullets = {} -- enemy bullets
local enemies = {}
local powerups = {}
local particles = {}

local level = 1
local level_time = 0
local spawn_cd = 0
local spawn_interval = SPAWN_INTERVAL_BASE
local score = 0
local chain_timer = 0.0
local chain_mult = 1.0

local kills_since_drop = 0
local game_over = false

-- RNG helper
local function rand(a, b)
    return a + math.random() * (b - a)
end

-- Utility
local function clamp(v, a, b)
    return (v < a) and a or ((v > b) and b or v)
end
local function aabb(ax, ay, aw, ah, bx, by, bw, bh)
    return (ax < bx + bw) and (bx < ax + aw) and (ay < by + bh) and (by < ay + ah)
end

-- Enemy kinds
local EKIND = {
    FODDER = 1,
    ZIGZAG = 2,
    SNIPER = 3,
    BULKER = 4
}

local function enemy_hp_for_level(L, base)
    return math.floor(base * (1 + 0.22 * L + 0.015 * L * L))
end

local function enemy_speed_for_level(L)
    local s = ENEMY_SPEED_MIN + (ENEMY_SPEED_MAX - ENEMY_SPEED_MIN) * math.min(1.0, L / 12)
    return s + 4 * math.sin(L * 0.7)
end

local function ebullet_speed_for_level(L)
    return math.min(220, ESHOT_SPEED * (1 + 0.02 * L))
end

local function next_spawn_interval_for_level(L)
    -- Decrease slightly with level, clamp
    return clamp(SPAWN_INTERVAL_BASE * (1.0 - 0.035 * L), 0.28, SPAWN_INTERVAL_BASE)
end

-- Particles (simple circles)
local function spawn_pop(x, y, col)
    for i = 1, 6 do
        particles[#particles + 1] = {
            x = x,
            y = y,
            r = 2 + i % 2,
            vx = rand(-30, 30),
            vy = rand(-30, 30),
            t = 0,
            life = 0.25,
            col = col or C.YELLOW
        }
    end
end

-- Powerups
local function spawn_powerup(x, y, kind)
    powerups[#powerups + 1] = {
        x = x,
        y = y,
        vx = -PU_SPEED,
        kind = kind
    }
end

local function try_drop_powerup(x, y)
    kills_since_drop = kills_since_drop + 1
    local want = math.random() < DROP_CHANCE or (kills_since_drop >= DROP_PITY_N)
    if not want then
        return
    end
    kills_since_drop = 0

    -- Bias toward the stat further from its cap
    local d_fr = (FIRE_RATE_MAX - player.fr) / (FIRE_RATE_MAX - FIRE_RATE_BASE)
    local d_dmg = (DMG_MAX - player.dmg) / (DMG_MAX - DMG_BASE)
    local kind = (d_fr > d_dmg) and PU_KIND.SPEED or PU_KIND.POWER

    spawn_powerup(x, y, kind)
end

-- Enemy spawners
local function spawn_enemy(kind)
    local y = rand(14, H - 14)
    local x = W + 12
    local v = enemy_speed_for_level(level)

    if kind == EKIND.FODDER then
        enemies[#enemies + 1] = {
            kind = kind,
            x = x,
            y = y,
            w = 12,
            h = 8,
            vx = -v,
            vy = 0,
            hp = enemy_hp_for_level(level, ENEMY_BASE_HP),
            t = 0
        }
    elseif kind == EKIND.ZIGZAG then
        enemies[#enemies + 1] = {
            kind = kind,
            x = x,
            y = y,
            w = 12,
            h = 8,
            vx = -v,
            vy = 0,
            amp = rand(12, 28),
            freq = rand(2.0, 3.2),
            hp = enemy_hp_for_level(level, ENEMY_BASE_HP + 1),
            t = 0
        }
    elseif kind == EKIND.SNIPER then
        enemies[#enemies + 1] = {
            kind = kind,
            x = x,
            y = y,
            w = 10,
            h = 8,
            vx = -(v * 0.85),
            vy = 0,
            pause = rand(0.5, 1.1),
            fired = false,
            hp = enemy_hp_for_level(level, ENEMY_BASE_HP),
            t = 0
        }
    elseif kind == EKIND.BULKER then
        enemies[#enemies + 1] = {
            kind = kind,
            x = x,
            y = y,
            w = 16,
            h = 12,
            vx = -(v * 0.6),
            vy = 0,
            hp = enemy_hp_for_level(level, ENEMY_BASE_HP + 6),
            t = 0
        }
    end
end

local function spawn_wave()
    -- Simple weighted random selection that respects on-screen cap
    if #enemies >= MAX_ENEMIES_ON_SCREEN then
        return
    end
    local r = math.random()
    if r < 0.5 then
        -- small Fodder pack
        for i = 1, math.random(2, 3) do
            spawn_enemy(EKIND.FODDER)
        end
    elseif r < 0.78 then
        for i = 1, math.random(2, 3) do
            spawn_enemy(EKIND.ZIGZAG)
        end
    elseif r < 0.93 then
        for i = 1, 2 do
            spawn_enemy(EKIND.SNIPER)
        end
    else
        spawn_enemy(EKIND.BULKER)
        if math.random() < 0.7 then
            spawn_enemy(EKIND.FODDER)
        end
    end
end

-- Shooting
local function fire_player()
    -- main shot
    bullets[#bullets + 1] = {
        x = player.x + 10,
        y = player.y,
        vx = SHOT_SPEED,
        w = SHOT_W,
        h = SHOT_H,
        dmg = player.dmg
    }
    -- side micro-shots unlocked via fr milestones
    if player.fr >= 6.0 then
        bullets[#bullets + 1] = {
            x = player.x + 10,
            y = player.y - 8,
            vx = SHOT_SPEED,
            w = 4,
            h = 1,
            dmg = 1
        }
        bullets[#bullets + 1] = {
            x = player.x + 10,
            y = player.y + 8,
            vx = SHOT_SPEED,
            w = 4,
            h = 1,
            dmg = 1
        }
    end
end

local function fire_sniper(e)
    local vy = (player.y - e.y)
    local mag = math.sqrt(vy * vy + 1)
    local speed = ebullet_speed_for_level(level)
    ebullets[#ebullets + 1] = {
        x = e.x - 2,
        y = e.y,
        vx = -speed * 1.0,
        vy = speed * (vy / mag) * 0.35,
        w = ESHOT_W,
        h = ESHOT_H
    }
end

-- Damage / death
local function damage_enemy(e, dmg)
    e.hp = e.hp - dmg
    if e.hp <= 0 then
        e.dead = true
        score = score + 10 * level
        chain_timer = 1.2
        chain_mult = math.min(3.0, chain_mult + 0.1)
        spawn_pop(e.x, e.y, C.ORANGE)
        try_drop_powerup(e.x, e.y)
    end
end

local function player_hit()
    if player.iframe > 0 then
        return
    end
    player.lives = player.lives - 1
    player.iframe = PLAYER_IFRAME
    chain_mult = 1.0
    chain_timer = 0.0
    spawn_pop(player.x + 4, player.y, C.DARKRED)
    if player.lives <= 0 then
        game_over = true
    end
end

-- ============ Update ============
local function update_player(dt)
    local _, mx, my = lge.get_mouse_position()
    if my then
        player.y = clamp(my, 10, H - 10)
    else
        -- BLE Joystick fallback
        if lge.is_key_down("UP") then
            player.y = math.max(10, player.y - PLAYER_SPEED * dt)
        elseif lge.is_key_down("DOWN") then
            player.y = math.min(H - 10, player.y + PLAYER_SPEED * dt)
        end
    end

    player.fire_cd = player.fire_cd - dt
    if player.fire_cd <= 0 then
        fire_player()
        player.fire_cd = 1.0 / player.fr
    end

    if player.iframe > 0 then
        player.iframe = math.max(0, player.iframe - dt)
    end
end

local function update_bullets(dt)
    local i = 1
    while i <= #bullets do
        local b = bullets[i]
        b.x = b.x + b.vx * dt
        if b.x > W + 8 then
            table.remove(bullets, i)
        else
            i = i + 1
        end
    end
end

local function update_ebullets(dt)
    local i = 1
    while i <= #ebullets do
        local b = ebullets[i]
        b.x = b.x + b.vx * dt
        b.y = b.y + (b.vy or 0) * dt
        if b.x < -8 or b.y < -8 or b.y > H + 8 then
            table.remove(ebullets, i)
        else
            i = i + 1
        end
    end
end

local function update_enemies(dt)
    for _, e in ipairs(enemies) do
        e.t = e.t + dt
        if e.kind == EKIND.ZIGZAG then
            e.y = e.y + math.sin(e.t * (e.freq or 2.5)) * (e.amp or 18) * dt * 6.0
        elseif e.kind == EKIND.SNIPER then
            if (not e.fired) and e.t >= (e.pause or 0.8) then
                fire_sniper(e);
                e.fired = true
            end
        end
        e.x = e.x + e.vx * dt
    end
    -- cull offscreen
    local i = 1
    while i <= #enemies do
        local e = enemies[i]
        if e.x < -20 or e.y < -20 or e.y > H + 20 then
            table.remove(enemies, i)
        else
            i = i + 1
        end
    end
end

local function update_powerups(dt)
    local magnet = MAGNET_BASE + MAGNET_GROWTH * (level - 1)
    for _, p in ipairs(powerups) do
        -- drift left
        p.x = p.x - PU_SPEED * dt
        -- magnet
        local dx = (player.x - p.x)
        local dy = (player.y - p.y)
        local dist2 = dx * dx + dy * dy
        if dist2 < magnet * magnet then
            local m = math.sqrt(dist2) + 1e-5
            p.x = p.x + dx / m * 120 * dt
            p.y = p.y + dy / m * 120 * dt
        end
    end
    -- collect / cull
    local i = 1
    while i <= #powerups do
        local p = powerups[i]
        if p.x < -10 then
            table.remove(powerups, i)
        elseif aabb(player.x - 4, player.y - 4, 8, 8, p.x - PU_SIZE / 2, p.y - PU_SIZE / 2, PU_SIZE, PU_SIZE) then
            if p.kind == PU_KIND.SPEED then
                player.fr = clamp(player.fr * 1.12, FIRE_RATE_BASE, FIRE_RATE_MAX)
            else
                player.dmg = math.min(DMG_MAX, player.dmg + 1)
            end
            spawn_pop(p.x, p.y, C.GREEN)
            table.remove(powerups, i)
        else
            i = i + 1
        end
    end
end

local function update_particles(dt)
    local i = 1
    while i <= #particles do
        local P = particles[i]
        P.t = P.t + dt
        P.x = P.x + P.vx * dt
        P.y = P.y + P.vy * dt
        if P.t >= P.life then
            table.remove(particles, i)
        else
            i = i + 1
        end
    end
end

local function handle_collisions()
    -- player bullets vs enemies
    local bi = 1
    while bi <= #bullets do
        local b = bullets[bi]
        local hit = false
        for _, e in ipairs(enemies) do
            if not e.dead and aabb(b.x, b.y - 1, b.w, b.h, e.x - e.w / 2, e.y - e.h / 2, e.w, e.h) then
                damage_enemy(e, b.dmg)
                hit = true
                break
            end
        end
        if hit then
            table.remove(bullets, bi)
        else
            bi = bi + 1
        end
    end

    -- enemy bullets vs player
    if player.iframe <= 0 then
        for i = #ebullets, 1, -1 do
            local b = ebullets[i]
            if aabb(player.x - 4, player.y - 4, 8, 8, b.x, b.y, b.w, b.h) then
                table.remove(ebullets, i)
                player_hit()
            end
        end
    end

    -- enemies touching player
    if player.iframe <= 0 then
        for _, e in ipairs(enemies) do
            if not e.dead and aabb(player.x - 4, player.y - 4, 8, 8, e.x - e.w / 2, e.y - e.h / 2, e.w, e.h) then
                player_hit()
                e.dead = true
                spawn_pop(e.x, e.y, C.RED)
            end
        end
    end

    -- remove dead enemies
    for i = #enemies, 1, -1 do
        if enemies[i].dead then
            table.remove(enemies, i)
        end
    end
end

local function update_level(dt)
    level_time = level_time + dt
    spawn_cd = spawn_cd - dt
    if spawn_cd <= 0 then
        spawn_wave()
        spawn_cd = next_spawn_interval_for_level(level)
    end

    if level_time >= LEVEL_TIME then
        -- next level
        level = level + 1
        level_time = 0
        spawn_cd = 0.1
        -- small end-level “heal” idea: brief iframe
        player.iframe = math.max(player.iframe, 0.3)
        -- guaranteed drop on transition
        spawn_powerup(rand(W * 0.55, W * 0.85), rand(20, H - 20),
            (math.random() < 0.5) and PU_KIND.SPEED or PU_KIND.POWER)
    end

    -- decay chain
    if chain_timer > 0 then
        chain_timer = chain_timer - dt
        if chain_timer <= 0 then
            chain_mult = math.max(1.0, chain_mult - 0.1)
        end
    end
end

-- ============ Draw ============
local function draw_player()
    -- triangle pointing right (simple ship)
    local x = player.x
    local y = player.y
    local blink = (player.iframe > 0) and ((math.floor(player.iframe * 20) % 2) == 0)
    if not blink then
        lge.draw_triangle(x - 6, y - 6, x - 6, y + 6, x + 6, y, C.CYAN)
        -- small fins
        lge.draw_triangle(x - 4, y - 4, x - 8, y, x - 4, y + 4, C.BLUE)
    end
end

local function draw_enemies()
    for _, e in ipairs(enemies) do
        if e.kind == EKIND.FODDER then
            lge.draw_rectangle(e.x - e.w / 2, e.y - e.h / 2, e.w, e.h, C.PURPLE)
        elseif e.kind == EKIND.ZIGZAG then
            lge.draw_triangle(e.x - 6, e.y - 6, e.x - 6, e.y + 6, e.x + 6, e.y, C.ORANGE)
        elseif e.kind == EKIND.SNIPER then
            lge.draw_rectangle(e.x - 5, e.y - 4, 10, 8, C.YELLOW)
            lge.draw_circle(e.x + 2, e.y, 2, C.RED)
        elseif e.kind == EKIND.BULKER then
            lge.draw_rectangle(e.x - 8, e.y - 6, 16, 12, C.DARKRED)
            lge.draw_triangle(e.x - 10, e.y - 6, e.x - 10, e.y + 6, e.x - 2, e.y, C.RED)
        end
    end
end

local function draw_bullets()
    for _, b in ipairs(bullets) do
        lge.draw_rectangle(b.x, b.y - 1, b.w, b.h, C.LIGHT)
    end
    for _, b in ipairs(ebullets) do
        lge.draw_rectangle(b.x, b.y - 1, b.w, b.h, C.WHITE)
    end
end

local function draw_powerups()
    for _, p in ipairs(powerups) do
        local col = (p.kind == PU_KIND.SPEED) and C.GREEN or C.ORANGE
        lge.draw_circle(p.x, p.y, PU_SIZE / 2, col)
    end
end

local function draw_particles()
    for _, P in ipairs(particles) do
        local r = math.max(1, math.floor(P.r * (1 - P.t / P.life)))
        lge.draw_circle(P.x, P.y, r, P.col)
    end
end

local function draw_ui()
    -- Score / Level
    lge.draw_text(6, 4, string.format("SCORE %06d", score), C.WHITE)
    lge.draw_text(W - 86, 4, string.format("LV %d", level), C.WHITE)

    -- Lives
    local lx = 6;
    local ly = H - 12
    for i = 1, 3 do
        local col = (i <= player.lives) and C.CYAN or C.GREY
        -- tiny ship icons
        lge.draw_triangle(lx + (i - 1) * 14, ly - 3, lx + (i - 1) * 14, ly + 3, lx + (i - 1) * 14 + 8, ly, col)
    end

    -- POW / SPD bars
    lge.draw_text(W - 110, H - 20, "POW", C.WHITE)
    local pw = math.floor(48 * (player.dmg - DMG_BASE) / (DMG_MAX - DMG_BASE))
    lge.draw_rectangle(W - 78, H - 21, 50, 6, C.GREY)
    lge.draw_rectangle(W - 78, H - 21, pw, 6, C.ORANGE)

    lge.draw_text(W - 110, H - 10, "SPD", C.WHITE)
    local spd_norm = (player.fr - FIRE_RATE_BASE) / (FIRE_RATE_MAX - FIRE_RATE_BASE)
    local sw = math.floor(48 * math.max(0, math.min(1, spd_norm)))
    lge.draw_rectangle(W - 78, H - 11, 50, 6, C.GREY)
    lge.draw_rectangle(W - 78, H - 11, sw, 6, C.GREEN)

    -- Game over banner
    if game_over then
        lge.draw_text(W / 2 - 48, H / 2 - 10, "GAME OVER", C.WHITE)
        lge.draw_text(W / 2 - 78, H / 2 + 6, "Click to restart", C.WHITE)
    end
end

-- ============ Reset ============
local function reset_game()
    player.x = PLAYER_X
    player.y = H / 2
    player.lives = 3
    player.fr = FIRE_RATE_BASE
    player.fire_cd = 0
    player.dmg = DMG_BASE
    player.iframe = 0
    bullets = {}
    ebullets = {}
    enemies = {}
    powerups = {}
    particles = {}
    level = 1
    level_time = 0
    spawn_cd = 0
    score = 0
    chain_timer = 0
    chain_mult = 1.0
    kills_since_drop = 0
    game_over = false
end

-- ============ Main Loop ============
math.randomseed(os.time())

reset_game()

local last = collectgarbage("generational", 10, 50)
local framesBeforeTest = 300

while true do
    lge.clear_canvas() -- black background

    if game_over then
        -- allow quick restart on any mouse motion/click
        local btn, mx, my = lge.get_mouse_position()
        if btn or mx or my then
            reset_game()
        end
    else
        update_player(FIXED_DT)
        update_bullets(FIXED_DT)
        update_ebullets(FIXED_DT)
        update_enemies(FIXED_DT)
        update_powerups(FIXED_DT)
        handle_collisions()
        update_particles(FIXED_DT)
        update_level(FIXED_DT)
    end

    -- Draw
    draw_player()
    draw_enemies()
    draw_bullets()
    draw_powerups()
    draw_particles()
    draw_ui()

    -- Optional FPS
    -- local fps = math.floor(lge.fps()*100+0.5)/100
    -- lge.draw_text(5, 18, "FPS: "..fps, C.WHITE)

    framesBeforeTest = framesBeforeTest - 1
    if framesBeforeTest == 0 then
        print(("Mem: %.1f KB"):format(collectgarbage("count")))
        framesBeforeTest = 300
    end

    if framesBeforeTest % 60 == 0 then
        collectgarbage("step", 10)
    end

    lge.present()
    lge.delay(FRAME_DELAY_MS)
end
