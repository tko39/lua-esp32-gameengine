// Auto-generated header file from ./bouncingBalls.lua
#ifndef LUASCRIPT_H
#define LUASCRIPT_H

const char lua_script[] = R"(
-- Define the number of balls to generate
local NUM_BALLS = 4
local BALL_MIN_R = 15
local BALL_MAX_R = 30
local BALL_MAX_V = 6

local MAX_VEL = 10 -- maximum allowed velocity for dx/dy (from your previous update)
local RESTORE_RATE = 0.05 -- how quickly velocity returns to original (0.0-1.0)

-- Function to generate a random hex color
local function random_color()
    local r = math.random(255)
    local g = math.random(255)
    local b = math.random(255)
    return string.format("#%02x%02x%02x", r, g, b)
end

-- Get canvas size once
local w, h = lge.get_canvas_size()

if not balls then
    balls = {}
    -- Initialize multiple balls
    for i = 1, NUM_BALLS do
        local r = math.random(BALL_MIN_R, BALL_MAX_R)
        -- Ensure position is well within boundaries
        local x = math.random(r, w - r)
        local y = math.random(r, h - r)

        -- Generate non-zero random velocities
        local dx = math.random() * BALL_MAX_V * (math.random() > 0.5 and 1 or -1)
        if math.abs(dx) < 1 then
            dx = dx * 2 * (dx >= 0 and 1 or -1)
        end
        local dy = math.random() * BALL_MAX_V * (math.random() > 0.5 and 1 or -1)
        if math.abs(dy) < 1 then
            dy = dy * 2 * (dy >= 0 and 1 or -1)
        end

        balls[i] = {
            x = x,
            y = y,
            dx = dx,
            dy = dy,
            r = r,
            color = random_color(),
            orig_dx = dx,
            orig_dy = dy
        }
    end
end

-- Function to handle wall collisions for a single ball
local function check_wall_collision(ball, w, h)
    -- X-axis collision
    if (ball.x - ball.r < 0 and ball.dx < 0) then
        ball.dx = -ball.dx
        ball.x = ball.r -- reposition to prevent sticking
    elseif (ball.x + ball.r > w and ball.dx > 0) then
        ball.dx = -ball.dx
        ball.x = w - ball.r -- reposition to prevent sticking
    end

    -- Y-axis collision
    if (ball.y - ball.r < 0 and ball.dy < 0) then
        ball.dy = -ball.dy
        ball.y = ball.r -- reposition to prevent sticking
    elseif (ball.y + ball.r > h and ball.dy > 0) then
        ball.dy = -ball.dy
        ball.y = h - ball.r -- reposition to prevent sticking
    end
end

-- Function to handle elastic collision between two balls (b1 and b2)
local function check_ball_collision(b1, b2)
    local dx = b2.x - b1.x
    local dy = b2.y - b1.y
    local dist_sq = dx * dx + dy * dy
    local min_dist = b1.r + b2.r

    -- Check for collision (distance <= sum of radii)
    if dist_sq <= min_dist * min_dist then
        local dist = math.sqrt(dist_sq)

        -- 1. Resolve overlap (push balls apart to prevent sticking)
        local overlap = min_dist - dist
        if dist == 0 then
            dist = 0.001
        end -- Avoid division by zero

        -- Normal vector (unit)
        local nx = dx / dist
        local ny = dy / dist

        -- Push balls apart along the normal vector
        -- We apply half the overlap distance to each ball for a balanced push
        b1.x = b1.x - nx * overlap * 0.5
        b1.y = b1.y - ny * overlap * 0.5
        b2.x = b2.x + nx * overlap * 0.5
        b2.y = b2.y + ny * overlap * 0.5

        -- 2. Elastic Collision Response (assuming equal mass)
        local vx1 = b1.dx
        local vy1 = b1.dy
        local vx2 = b2.dx
        local vy2 = b2.dy

        -- Relative velocity vector
        local rv_x = vx2 - vx1
        local rv_y = vy2 - vy1

        -- Velocity along the normal (dot product of relative velocity and normal)
        local v_dot_n = rv_x * nx + rv_y * ny

        -- Only proceed if balls are moving towards each other
        if v_dot_n < 0 then
            -- Calculate impulse (2 * V.N since masses are equal and elasticity is 1)
            local impulse = 2 * v_dot_n

            -- Apply the impulse to the velocities
            b1.dx = vx1 + nx * impulse
            b1.dy = vy1 + ny * impulse

            b2.dx = vx2 - nx * impulse
            b2.dy = vy2 - ny * impulse
        end
    end
end

while true do
    lge.clear_canvas()

    -- 1. Update positions & Velocity management
    for _, ball in ipairs(balls) do
        ball.x = ball.x + ball.dx
        ball.y = ball.y + ball.dy

        -- Cap velocity
        if ball.dx > MAX_VEL then
            ball.dx = MAX_VEL
        end
        if ball.dx < -MAX_VEL then
            ball.dx = -MAX_VEL
        end
        if ball.dy > MAX_VEL then
            ball.dy = MAX_VEL
        end
        if ball.dy < -MAX_VEL then
            ball.dy = -MAX_VEL
        end

        -- Gradually restore velocity to original
        local sign_dx = (ball.dx >= 0) and 1 or -1
        local sign_dy = (ball.dy >= 0) and 1 or -1
        local mag_dx = math.abs(ball.dx) + (math.abs(ball.orig_dx) - math.abs(ball.dx)) * RESTORE_RATE
        local mag_dy = math.abs(ball.dy) + (math.abs(ball.orig_dy) - math.abs(ball.dy)) * RESTORE_RATE
        ball.dx = sign_dx * mag_dx
        ball.dy = sign_dy * mag_dy
    end

    -- 2. Check wall collisions for each ball
    for _, ball in ipairs(balls) do
        check_wall_collision(ball, w, h)
    end

    -- 3. Check all unique ball-to-ball collisions (O(N^2) checks)
    -- This nested loop ensures every ball is checked against every other ball exactly once.
    for i = 1, #balls do
        for j = i + 1, #balls do
            check_ball_collision(balls[i], balls[j])
        end
    end

    -- 4. Draw balls
    for _, ball in ipairs(balls) do
        lge.draw_circle(ball.x, ball.y, ball.r, ball.color)
    end

    local fps = math.floor(lge.fps() * 100 + 0.5) / 100
    lge.draw_text(5, 5, "FPS: " .. fps, "#FFFFFF")

    lge.present()
    lge.delay(16)
end
)";

#endif // LUASCRIPT_H
