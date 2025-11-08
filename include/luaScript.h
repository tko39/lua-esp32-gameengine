// Auto-generated header file from bouncingBalls.lua
#ifndef LUASCRIPT_H
#define LUASCRIPT_H

const char lua_script[] = R"(
local NUM_BALLS = 4
local BALL_MIN_R = 15
local BALL_MAX_R = 30
local BALL_MAX_V = 6
local MAX_VEL = 10 
local RESTORE_RATE = 0.05 
local function random_color()
  local r = math.random(255)
  local g = math.random(255)
  local b = math.random(255)
  return string.format("#%02x%02x%02x", r, g, b)
end
local w, h = lge.get_canvas_size()
if not balls then
  balls = {}
  for i = 1, NUM_BALLS do
    local r = math.random(BALL_MIN_R, BALL_MAX_R)
    local x = math.random(r, w - r)
    local y = math.random(r, h - r)
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
local function check_wall_collision(ball, w, h)
  if (ball.x - ball.r < 0 and ball.dx < 0) then
    ball.dx = -ball.dx
    ball.x = ball.r 
  elseif (ball.x + ball.r > w and ball.dx > 0) then
    ball.dx = -ball.dx
    ball.x = w - ball.r 
  end
  if (ball.y - ball.r < 0 and ball.dy < 0) then
    ball.dy = -ball.dy
    ball.y = ball.r 
  elseif (ball.y + ball.r > h and ball.dy > 0) then
    ball.dy = -ball.dy
    ball.y = h - ball.r 
  end
end
local function check_ball_collision(b1, b2)
  local dx = b2.x - b1.x
  local dy = b2.y - b1.y
  local dist_sq = dx * dx + dy * dy
  local min_dist = b1.r + b2.r
  if dist_sq <= min_dist * min_dist then
    local dist = math.sqrt(dist_sq)
    local overlap = min_dist - dist
    if dist == 0 then
      dist = 0.001
    end 
    local nx = dx / dist
    local ny = dy / dist
    b1.x = b1.x - nx * overlap * 0.5
    b1.y = b1.y - ny * overlap * 0.5
    b2.x = b2.x + nx * overlap * 0.5
    b2.y = b2.y + ny * overlap * 0.5
    local vx1 = b1.dx
    local vy1 = b1.dy
    local vx2 = b2.dx
    local vy2 = b2.dy
    local rv_x = vx2 - vx1
    local rv_y = vy2 - vy1
    local v_dot_n = rv_x * nx + rv_y * ny
    if v_dot_n < 0 then
      local impulse = 2 * v_dot_n
      b1.dx = vx1 + nx * impulse
      b1.dy = vy1 + ny * impulse
      b2.dx = vx2 - nx * impulse
      b2.dy = vy2 - ny * impulse
    end
  end
end
while true do
  lge.clear_canvas()
  for _, ball in ipairs(balls) do
    ball.x = ball.x + ball.dx
    ball.y = ball.y + ball.dy
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
    local sign_dx = (ball.dx >= 0) and 1 or -1
    local sign_dy = (ball.dy >= 0) and 1 or -1
    local mag_dx = math.abs(ball.dx) + (math.abs(ball.orig_dx) - math.abs(ball.dx)) * RESTORE_RATE
    local mag_dy = math.abs(ball.dy) + (math.abs(ball.orig_dy) - math.abs(ball.dy)) * RESTORE_RATE
    ball.dx = sign_dx * mag_dx
    ball.dy = sign_dy * mag_dy
  end
  for _, ball in ipairs(balls) do
    check_wall_collision(ball, w, h)
  end
  for i = 1, #balls do
    for j = i + 1, #balls do
      check_ball_collision(balls[i], balls[j])
    end
  end
  for _, ball in ipairs(balls) do
    lge.draw_circle(ball.x, ball.y, ball.r, ball.color)
  end
  local fps = math.floor(lge.fps() * 100 + 0.5) / 100
  lge.draw_text(5, 5, "FPS: " .. fps, "#FFFFFF")
  lge.present()
  lge.delay(33)
end
)";

#endif // LUASCRIPT_H
