-- flappyBird.lua - Flappy Bird clone for Lua Game Engine (lge)
-- Uses lge.* for drawing, input, and timing. Pure Lua logic.
-- Forward declarations for input
local touch_x, touch_y, touch_state = nil, nil, nil
local key_pressed, key_state = nil, nil

local FRAME_DELAY = 18 -- ms, ~55 FPS

-- Color constants
local COLORS = {
    bg = "#4AB6FF",
    pipe = "#00C800",
    bird = "#FFDB00",
    birdEye = "#000000",
    ground = "#B56D00",
    text = "#FFFFFF",
    gameOver = "#FF5050"
}

-- Game constants
local SCREEN_W, SCREEN_H = lge.get_canvas_size()
local GROUND_Y = SCREEN_H - 24
local BIRD_X = math.floor(SCREEN_W / 4)
local GRAVITY = 0.25
local FLAP_VY = -5.5
local PIPE_W = 32
local PIPE_GAP = 80
local PIPE_SPACING = 120
local PIPE_SPEED = 2
local BIRD_SIZE = 16

-- Game state
local birdY, birdVY, score, highScore, gameOver, started
local pipes = {}

local function spawnPipe(x)
    local gapY = math.random(32, GROUND_Y - PIPE_GAP - 32)
    table.insert(pipes, {
        x = x,
        gapY = gapY,
        passed = false
    })
end

local function resetGame()
    birdY = math.floor(SCREEN_H / 2)
    birdVY = 0
    score = 0
    gameOver = false
    started = false
    pipes = {}
    -- Spawn initial pipes
    for i = 1, 3 do
        spawnPipe(SCREEN_W + (i - 1) * PIPE_SPACING)
    end
end

local function updatePipes()
    for i = #pipes, 1, -1 do
        local pipe = pipes[i]
        pipe.x = pipe.x - PIPE_SPEED
        -- Check for scoring
        if not pipe.passed and pipe.x + PIPE_W < BIRD_X then
            score = score + 1
            pipe.passed = true
            if score > (highScore or 0) then
                highScore = score
            end
        end
        -- Remove offscreen pipes
        if pipe.x + PIPE_W < 0 then
            table.remove(pipes, i)
        end
    end
    -- Spawn new pipe if needed
    if #pipes == 0 or (pipes[#pipes].x < SCREEN_W - PIPE_SPACING) then
        spawnPipe(SCREEN_W)
    end
end

local function checkCollision()
    -- Ground/ceiling
    if birdY + BIRD_SIZE / 2 > GROUND_Y or birdY - BIRD_SIZE / 2 < 0 then
        return true
    end
    -- Pipes
    for _, pipe in ipairs(pipes) do
        if BIRD_X + BIRD_SIZE / 2 > pipe.x and BIRD_X - BIRD_SIZE / 2 < pipe.x + PIPE_W then
            if birdY - BIRD_SIZE / 2 < pipe.gapY or birdY + BIRD_SIZE / 2 > pipe.gapY + PIPE_GAP then
                return true
            end
        end
    end
    return false
end

local function drawPipes()
    for _, pipe in ipairs(pipes) do
        -- Top pipe
        lge.draw_rectangle(pipe.x, 0, PIPE_W, pipe.gapY, COLORS.pipe)
        -- Bottom pipe
        lge.draw_rectangle(pipe.x, pipe.gapY + PIPE_GAP, PIPE_W, GROUND_Y - (pipe.gapY + PIPE_GAP), COLORS.pipe)
    end
end

local function drawBird()
    lge.draw_circle(BIRD_X, birdY, BIRD_SIZE / 2, COLORS.bird)
    -- Eye
    lge.draw_circle(BIRD_X + 4, birdY - 3, 2, COLORS.birdEye)
end

local function drawGround()
    lge.draw_rectangle(0, GROUND_Y, SCREEN_W, SCREEN_H - GROUND_Y, COLORS.ground)
end

local function drawUI()
    lge.draw_text(8, 4, "Score: " .. score, COLORS.text)
    if highScore then
        lge.draw_text(SCREEN_W - 90, 4, "Best: " .. highScore, COLORS.text)
    end
    if not started then
        lge.draw_text(SCREEN_W // 2 - 60, SCREEN_H // 2 - 30, "Tap to start", COLORS.text)
    elseif gameOver then
        lge.draw_text(SCREEN_W // 2 - 60, SCREEN_H // 2 - 30, "Game Over!", COLORS.gameOver)
        lge.draw_text(SCREEN_W // 2 - 70, SCREEN_H // 2, "Tap to restart", COLORS.text)
    end
end

local function flap()
    if not started then
        started = true
        birdVY = FLAP_VY
        return
    end
    if not gameOver then
        birdVY = FLAP_VY
    elseif gameOver then
        resetGame()
    end
end

local function update(dt)
    if not started or gameOver then
        return
    end
    birdVY = birdVY + GRAVITY
    birdY = birdY + birdVY
    updatePipes()
    if checkCollision() then
        gameOver = true
    end
end

local function draw()
    lge.clear_canvas(COLORS.bg)
    drawGround()
    drawPipes()
    drawBird()
    drawUI()
end

local function handleInput()
    local _, x, y = lge.get_mouse_click()
    if x then
        flap()
    end
end

local function setup()
    resetGame()
end

local function loop()
    local updateDt = FRAME_DELAY / 1000
    while true do
        handleInput()
        update(updateDt)
        draw()
        lge.present()
        lge.delay(FRAME_DELAY)
    end
end

setup()
loop()
