// Auto-generated header file from lua/bouncingBalls.lua lua/touchSequence.lua lua/sideShooter.lua lua/rotatingBox.lua lua/bouncingRotatingCubes.lua
#ifndef LUASCRIPT_H
#define LUASCRIPT_H

const char* lua_scripts[] = {
  R"(
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
  lge.delay(1) 
end
)"
  ,
  R"(
local circles = {}
local sequence = {}
local current_step = 1
local round = 1
local showing_sequence = true
local show_timer = 0
local show_delay = 30 
local canvas_w, canvas_h = lge.get_canvas_size()
local score = 0
local game_over = false
local colors = {'#ff0000', '#00ff00', '#4A49FF', '#ffff00', '#ff00ff', '#00ffff'}
function circles_overlap(x1, y1, r1, x2, y2, r2)
  local dx = x1 - x2
  local dy = y1 - y2
  local dist = math.sqrt(dx * dx + dy * dy)
  return dist < (r1 + r2 + 8) 
end
function random_circle(existing)
  local margin = 30
  local tries = 0
  while true do
    local x = math.random(margin, canvas_w - margin)
    local y = math.random(margin, canvas_h - margin)
    local r = 24
    local color = colors[math.random(1, #colors)]
    local overlap = false
    if existing then
      for _, c in ipairs(existing) do
        if circles_overlap(x, y, r, c.x, c.y, c.r) then
          overlap = true
          break
        end
      end
    end
    if not overlap or tries > 30 then
      return {
        x = x,
        y = y,
        r = r,
        color = color
      }
    end
    tries = tries + 1
  end
end
function start_round()
  circles = {}
  sequence = {}
  current_step = 1
  showing_sequence = true
  show_timer = 0
  for i = 1, round do
    local c = random_circle(circles)
    table.insert(circles, c)
    table.insert(sequence, c)
  end
end
function reset_game()
  round = 1
  score = 0
  game_over = false
  start_round()
end
function draw_circles(highlight_idx)
  for i, c in ipairs(circles) do
    local color = c.color
    if highlight_idx == i then
      color = '#ffffff'
    end
    lge.draw_circle(c.x, c.y, c.r, color)
  end
end
function loop()
  lge.clear_canvas()
  if game_over then
    lge.draw_text(canvas_w // 2 - 60, canvas_h // 2 - 20, 'Game Over!', '#ff0000')
    lge.draw_text(canvas_w // 2 - 60, canvas_h // 2 + 10, 'Score: ' .. score, '#ffffff')
    lge.draw_text(canvas_w // 2 - 60, canvas_h // 2 + 40, 'Touch to restart', '#00ff00')
    local _, x, y = lge.get_mouse_click()
    if x then
      reset_game()
    end
  elseif showing_sequence then
    local idx = math.floor(show_timer / show_delay) + 1
    if idx <= #sequence then
      draw_circles(idx)
      show_timer = show_timer + 1
    else
      showing_sequence = false
    end
  else
    draw_circles()
    local _, x, y = lge.get_mouse_click()
    if x then
      local touched_idx = nil
      for i, c in ipairs(circles) do
        if math.sqrt((x - c.x) ^ 2 + (y - c.y) ^ 2) <= c.r then
          touched_idx = i
          break
        end
      end
      if touched_idx then
        local c = circles[touched_idx]
        local expected = sequence[current_step]
        if c == expected then
          table.remove(circles, touched_idx)
          current_step = current_step + 1
          if current_step > #sequence then
            score = round
            round = round + 1
            start_round()
          end
        else
          game_over = true
        end
      end
    end
  end
end
math.randomseed(os.time())
reset_game()
while true do
  loop()
  lge.present()
  lge.delay(33)
end
)"
  ,
  R"(
local W, H = 320, 240
local PLAYER_X = 14
local PLAYER_SPEED = 220 
local PLAYER_WIDTH = 10
local PLAYER_HEIGHT = 8
local PLAYER_IFRAME = 1.0 
local FIRE_RATE_BASE = 3.5 
local FIRE_RATE_MAX = 10.0
local SHOT_SPEED = 280 
local SHOT_W, SHOT_H = 5, 2
local DMG_BASE = 1
local DMG_MAX = 6
local ENEMY_BASE_HP = 3 
local ENEMY_SPEED_MIN = 55
local ENEMY_SPEED_MAX = 85
local ESHOT_SPEED = 140
local ESHOT_W, ESHOT_H = 3, 2
local LEVEL_TIME = 34.0 
local SPAWN_INTERVAL_BASE = 0.90 
local MAX_ENEMIES_ON_SCREEN = 26
local DROP_CHANCE = 0.06
local DROP_PITY_N = 12 
local PU_SIZE = 7
local PU_SPEED = 50
local MAGNET_BASE = 16 
local MAGNET_GROWTH = 1.2 
local PU_KIND = {
  SPEED = 1,
  POWER = 2
}
local FIXED_DT = 1 / 60
local FRAME_DELAY_MS = 16
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
DMG_BASE = 4
ENEMY_BASE_HP = 1
SPAWN_INTERVAL_BASE = 1.5
MAX_ENEMIES_ON_SCREEN = 10
DROP_CHANCE = 0.2
local player = {
  x = PLAYER_X,
  y = H / 2,
  lives = 3,
  fr = FIRE_RATE_BASE, 
  fire_cd = 0.0,
  dmg = DMG_BASE,
  iframe = 0.0,
  sideguns = 0 
}
local bullets = {} 
local ebullets = {} 
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
local function rand(a, b)
  return a + math.random() * (b - a)
end
local function clamp(v, a, b)
  return (v < a) and a or ((v > b) and b or v)
end
local function aabb(ax, ay, aw, ah, bx, by, bw, bh)
  return (ax < bx + bw) and (bx < ax + aw) and (ay < by + bh) and (by < ay + ah)
end
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
  return clamp(SPAWN_INTERVAL_BASE * (1.0 - 0.035 * L), 0.28, SPAWN_INTERVAL_BASE)
end
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
  local d_fr = (FIRE_RATE_MAX - player.fr) / (FIRE_RATE_MAX - FIRE_RATE_BASE)
  local d_dmg = (DMG_MAX - player.dmg) / (DMG_MAX - DMG_BASE)
  local kind = (d_fr > d_dmg) and PU_KIND.SPEED or PU_KIND.POWER
  spawn_powerup(x, y, kind)
end
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
  if #enemies >= MAX_ENEMIES_ON_SCREEN then
    return
  end
  local r = math.random()
  if r < 0.5 then
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
local function fire_player()
  bullets[#bullets + 1] = {
    x = player.x + 10,
    y = player.y,
    vx = SHOT_SPEED,
    w = SHOT_W,
    h = SHOT_H,
    dmg = player.dmg
  }
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
local function update_player(dt)
  local _, mx, my = lge.get_mouse_position()
  if my then
    player.y = clamp(my, 10, H - 10)
  else
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
    p.x = p.x - PU_SPEED * dt
    local dx = (player.x - p.x)
    local dy = (player.y - p.y)
    local dist2 = dx * dx + dy * dy
    if dist2 < magnet * magnet then
      local m = math.sqrt(dist2) + 1e-5
      p.x = p.x + dx / m * 120 * dt
      p.y = p.y + dy / m * 120 * dt
    end
  end
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
  if player.iframe <= 0 then
    for i = #ebullets, 1, -1 do
      local b = ebullets[i]
      if aabb(player.x - 4, player.y - 4, 8, 8, b.x, b.y, b.w, b.h) then
        table.remove(ebullets, i)
        player_hit()
      end
    end
  end
  if player.iframe <= 0 then
    for _, e in ipairs(enemies) do
      if not e.dead and aabb(player.x - 4, player.y - 4, 8, 8, e.x - e.w / 2, e.y - e.h / 2, e.w, e.h) then
        player_hit()
        e.dead = true
        spawn_pop(e.x, e.y, C.RED)
      end
    end
  end
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
    level = level + 1
    level_time = 0
    spawn_cd = 0.1
    player.iframe = math.max(player.iframe, 0.3)
    spawn_powerup(rand(W * 0.55, W * 0.85), rand(20, H - 20),
      (math.random() < 0.5) and PU_KIND.SPEED or PU_KIND.POWER)
  end
  if chain_timer > 0 then
    chain_timer = chain_timer - dt
    if chain_timer <= 0 then
      chain_mult = math.max(1.0, chain_mult - 0.1)
    end
  end
end
local function draw_player()
  local x = player.x
  local y = player.y
  local blink = (player.iframe > 0) and ((math.floor(player.iframe * 20) % 2) == 0)
  if not blink then
    lge.draw_triangle(x - 6, y - 6, x - 6, y + 6, x + 6, y, C.CYAN)
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
  lge.draw_text(6, 4, string.format("SCORE %06d", score), C.WHITE)
  lge.draw_text(W - 86, 4, string.format("LV %d", level), C.WHITE)
  local lx = 6;
  local ly = H - 12
  for i = 1, 3 do
    local col = (i <= player.lives) and C.CYAN or C.GREY
    lge.draw_triangle(lx + (i - 1) * 14, ly - 3, lx + (i - 1) * 14, ly + 3, lx + (i - 1) * 14 + 8, ly, col)
  end
  lge.draw_text(W - 110, H - 20, "POW", C.WHITE)
  local pw = math.floor(48 * (player.dmg - DMG_BASE) / (DMG_MAX - DMG_BASE))
  lge.draw_rectangle(W - 78, H - 21, 50, 6, C.GREY)
  lge.draw_rectangle(W - 78, H - 21, pw, 6, C.ORANGE)
  lge.draw_text(W - 110, H - 10, "SPD", C.WHITE)
  local spd_norm = (player.fr - FIRE_RATE_BASE) / (FIRE_RATE_MAX - FIRE_RATE_BASE)
  local sw = math.floor(48 * math.max(0, math.min(1, spd_norm)))
  lge.draw_rectangle(W - 78, H - 11, 50, 6, C.GREY)
  lge.draw_rectangle(W - 78, H - 11, sw, 6, C.GREEN)
  if game_over then
    lge.draw_text(W / 2 - 48, H / 2 - 10, "GAME OVER", C.WHITE)
    lge.draw_text(W / 2 - 78, H / 2 + 6, "Click to restart", C.WHITE)
  end
end
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
math.randomseed(os.time())
reset_game()
local last = collectgarbage("generational", 10, 50)
local framesBeforeTest = 300
while true do
  lge.clear_canvas() 
  if game_over then
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
  draw_player()
  draw_enemies()
  draw_bullets()
  draw_powerups()
  draw_particles()
  draw_ui()
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
)"
  ,
  R"(
local canvas_w, canvas_h = lge.get_canvas_size()
local center_x = canvas_w / 2
local center_y = canvas_h / 2
local vertices = {{
  x = -1,
  y = -1,
  z = -1
}, 
{
  x = 1,
  y = -1,
  z = -1
}, 
{
  x = 1,
  y = 1,
  z = -1
}, 
{
  x = -1,
  y = 1,
  z = -1
}, 
{
  x = -1,
  y = -1,
  z = 1
}, 
{
  x = 1,
  y = -1,
  z = 1
}, 
{
  x = 1,
  y = 1,
  z = 1
}, 
{
  x = -1,
  y = 1,
  z = 1
} 
}
local faces = { 
{1, 4, 3, "#FF0000"}, {1, 3, 2, "#FF0000"}, 
{5, 6, 7, "#00FF00"}, {5, 7, 8, "#00FF00"}, 
{1, 5, 8, "#0000FF"}, {1, 8, 4, "#0000FF"}, 
{2, 3, 7, "#FFFF00"}, {2, 7, 6, "#FFFF00"}, 
{4, 8, 7, "#FF00FF"}, {4, 7, 3, "#FF00FF"}, 
{1, 2, 6, "#00FFFF"}, {1, 6, 5, "#00FFFF"}}
local angle_x = 0
local angle_y = 0
local angle_z = 0
local fov = 256 
local distance = 5 
function update()
  angle_x = angle_x + 0.01
  angle_y = angle_y + 0.015
  angle_z = angle_z + 0.005
end
function draw()
  lge.clear_canvas()
  local transformed_vertices = {}
  local faces_to_draw = {}
  local cos_x = math.cos(angle_x)
  local sin_x = math.sin(angle_x)
  local cos_y = math.cos(angle_y)
  local sin_y = math.sin(angle_y)
  local cos_z = math.cos(angle_z)
  local sin_z = math.sin(angle_z)
  for i = 1, #vertices do
    local v = vertices[i]
    local y1 = v.y * cos_x - v.z * sin_x
    local z1 = v.y * sin_x + v.z * cos_x
    local x2 = v.x * cos_y + z1 * sin_y
    local z2 = -v.x * sin_y + z1 * cos_y
    local px = x2 * cos_z - y1 * sin_z
    local py = x2 * sin_z + y1 * cos_z
    local pz = z2 
    pz = pz + distance
    local z_factor = fov / pz
    local screen_x = px * z_factor + center_x
    local screen_y = py * z_factor + center_y
    transformed_vertices[i] = {
      x = screen_x,
      y = screen_y,
      z = pz 
    }
  end
  for i = 1, #faces do
    local face = faces[i]
    local i1, i2, i3 = face[1], face[2], face[3]
    local color = face[4] 
    local v1 = transformed_vertices[i1]
    local v2 = transformed_vertices[i2]
    local v3 = transformed_vertices[i3]
    local normal_z = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)
    if normal_z < 0 then
      local avg_z = (v1.z + v2.z + v3.z) / 3
      table.insert(faces_to_draw, {
        z = avg_z,
        v1 = v1,
        v2 = v2,
        v3 = v3,
        color = color
      })
    end
  end
  table.sort(faces_to_draw, function(a, b)
    return a.z > b.z
  end)
  for i = 1, #faces_to_draw do
    local f = faces_to_draw[i]
    lge.draw_triangle(f.v1.x, f.v1.y, f.v2.x, f.v2.y, f.v3.x, f.v3.y, f.color)
  end
  local fps = math.floor(lge.fps() * 100 + 0.5) / 100
  lge.draw_text(10, 10, "FPS: " .. fps, "#C8C8C8")
end
function main_loop()
  lge.draw_text(center_x - 50, center_y, "Loading...", "#FFFFFF")
  lge.present()
  lge.delay(500)
  while true do
    update()
    draw()
    lge.present()
    lge.delay(1) 
  end
end
main_loop()
)"
  ,
  R"(
local ENABLE_PROFILING = false
if ENABLE_PROFILING then
  local profile_data = {}
  local last_profiling_print = 0
  function profiling_start(name)
    if not profile_data[name] then
      profile_data[name] = {
        sum = 0,
        count = 0
      }
    end
    profile_data[name].start = millis()
  end
  function profiling_end(name)
    local now = millis()
    local entry = profile_data[name]
    if entry and entry.start then
      local elapsed = now - entry.start
      entry.sum = entry.sum + elapsed
      entry.count = entry.count + 1
      entry.start = nil
    end
  end
  function print_profiling_results()
    local now = millis()
    if now - last_profiling_print >= 5000 then
      last_profiling_print = now
      for name, data in pairs(profile_data) do
        local avg = data.sum / data.count
        print(string.format("%-30s total: %8.2f ms, avg:%8.2f ms, calls: %10d", name, data.sum, avg, data.count))
      end
      profile_data = {}
    end
  end
else
  function profiling_start(name)
  end
  function profiling_end(name)
  end
  function print_profiling_results()
  end
end
local Cube = {}
Cube.__index = Cube
local lge_draw_triangle = lge.draw_triangle
Cube.model_vertices = {{
  x = -1,
  y = -1,
  z = -1
}, 
{
  x = 1,
  y = -1,
  z = -1
}, 
{
  x = 1,
  y = 1,
  z = -1
}, 
{
  x = -1,
  y = 1,
  z = -1
}, 
{
  x = -1,
  y = -1,
  z = 1
}, 
{
  x = 1,
  y = -1,
  z = 1
}, 
{
  x = 1,
  y = 1,
  z = 1
}, 
{
  x = -1,
  y = 1,
  z = 1
} 
}
local function random_color()
  local r = math.random(50, 255) 
  local g = math.random(50, 255)
  local b = math.random(50, 255)
  return string.format("#%02x%02x%02x", r, g, b)
end
function Cube:prepare_fast_representation()
  local mv = self.model_vertices
  local n = #mv
  local flat = {}
  for i = 1, n do
    local v = mv[i]
    flat[(i - 1) * 3 + 1] = v.x
    flat[(i - 1) * 3 + 2] = v.y
    flat[(i - 1) * 3 + 3] = v.z
  end
  self.model_vertices_flat = flat
  self.model_vertices_count = n
  local faces = self.faces
  local fc = #faces
  local face_flat = {}
  for f = 1, fc do
    local face = faces[f]
    face_flat[(f - 1) * 4 + 1] = face[1]
    face_flat[(f - 1) * 4 + 2] = face[2]
    face_flat[(f - 1) * 4 + 3] = face[3]
    face_flat[(f - 1) * 4 + 4] = face[4] 
  end
  self.faces_flat = face_flat
  self.faces_count = fc
  local tv = {}
  for i = 1, n * 3 do
    tv[i] = 0
  end
  self.transformed_vertices_flat = tv
  self.visible_z = {}
  self.visible_b1 = {}
  self.visible_b2 = {}
  self.visible_b3 = {}
  self.visible_color = {}
end
function Cube:new(x, y, dx, dy, radius, max_vel, restore_rate, fov, cam_dist)
  local self = setmetatable({}, Cube)
  self.x = x
  self.y = y
  self.dx = dx
  self.dy = dy
  self.r = radius 
  self.orig_dx = dx
  self.orig_dy = dy
  self.MAX_VEL = max_vel
  self.RESTORE_RATE = restore_rate
  self.angle_x = math.random() * 2 * math.pi
  self.angle_y = math.random() * 2 * math.pi
  self.angle_z = math.random() * 2 * math.pi
  self.d_angle_x = (math.random() - 0.5) * 0.05
  self.d_angle_y = (math.random() - 0.5) * 0.05
  self.d_angle_z = (math.random() - 0.5) * 0.05
  local face_colors = {random_color(), 
  random_color(), 
  random_color(), 
  random_color(), 
  random_color(), 
  random_color() 
  }
  self.faces = {{1, 4, 3, face_colors[1]}, {1, 3, 2, face_colors[1]}, {5, 6, 7, face_colors[2]},
          {5, 7, 8, face_colors[2]}, {1, 5, 8, face_colors[3]}, {1, 8, 4, face_colors[3]},
          {2, 3, 7, face_colors[4]}, {2, 7, 6, face_colors[4]}, {4, 8, 7, face_colors[5]},
          {4, 7, 3, face_colors[5]}, {1, 2, 6, face_colors[6]}, {1, 6, 5, face_colors[6]}}
  self.fov = fov
  self.z_distance = cam_dist 
  self.size = self.r * (self.z_distance / self.fov)
  self.transformed_vertices = {}
  for i = 1, #Cube.model_vertices do
    self.transformed_vertices[i] = {
      x = 0,
      y = 0,
      z = 0
    }
  end
  self:prepare_fast_representation()
  self.orig_mag_dx = math.abs(dx)
  self.orig_mag_dy = math.abs(dy)
  return self
end
function Cube:update()
  profiling_start("Cube:update")
  local m_abs = math.abs
  self.x = self.x + self.dx
  self.y = self.y + self.dy
  self.angle_x = self.angle_x + self.d_angle_x
  self.angle_y = self.angle_y + self.d_angle_y
  self.angle_z = self.angle_z + self.d_angle_z
  if self.dx > self.MAX_VEL then
    self.dx = self.MAX_VEL
  end
  if self.dx < -self.MAX_VEL then
    self.dx = -self.MAX_VEL
  end
  if self.dy > self.MAX_VEL then
    self.dy = self.MAX_VEL
  end
  if self.dy < -self.MAX_VEL then
    self.dy = -self.MAX_VEL
  end
  local sign_dx = (self.dx >= 0) and 1 or -1
  local sign_dy = (self.dy >= 0) and 1 or -1
  local mag_dx = m_abs(self.dx)
  local mag_dy = m_abs(self.dy)
  local mag_dx = mag_dx + (self.orig_mag_dx - mag_dx) * self.RESTORE_RATE
  local mag_dy = mag_dy + (self.orig_mag_dy - mag_dy) * self.RESTORE_RATE
  self.dx = sign_dx * mag_dx
  self.dy = sign_dy * mag_dy
  profiling_end("Cube:update")
end
function Cube:draw_fast()
  profiling_start("Cube:draw_fast")
  local m_cos = math.cos
  local m_sin = math.sin
  local mv_flat = self.model_vertices_flat
  local mv_count = self.model_vertices_count
  local faces_flat = self.faces_flat
  local faces_count = self.faces_count
  local tv = self.transformed_vertices_flat 
  local vis_z = self.visible_z
  local vis_b1 = self.visible_b1
  local vis_b2 = self.visible_b2
  local vis_b3 = self.visible_b3
  local vis_col = self.visible_color
  local size = self.size
  local x0 = self.x
  local y0 = self.y
  local z_dist = self.z_distance
  local fov = self.fov
  local ax, ay, az = self.angle_x, self.angle_y, self.angle_z
  local cos_x, sin_x = m_cos(ax), m_sin(ax)
  local cos_y, sin_y = m_cos(ay), m_sin(ay)
  local cos_z, sin_z = m_cos(az), m_sin(az)
  for i = 1, mv_count do
    local base = (i - 1) * 3
    local vx = mv_flat[base + 1] * size
    local vy = mv_flat[base + 2] * size
    local vz = mv_flat[base + 3] * size
    local y1 = vy * cos_x - vz * sin_x
    local z1 = vy * sin_x + vz * cos_x
    local x2 = vx * cos_y + z1 * sin_y
    local z2 = -vx * sin_y + z1 * cos_y
    local px = x2 * cos_z - y1 * sin_z
    local py = x2 * sin_z + y1 * cos_z
    local pz = z2
    pz = pz + z_dist
    local z_factor = fov / pz
    tv[base + 1] = px * z_factor + x0
    tv[base + 2] = py * z_factor + y0
    tv[base + 3] = pz
  end
  local vis_count = 0
  for f = 1, faces_count do
    local fbase = (f - 1) * 4
    local i1 = faces_flat[fbase + 1]
    local i2 = faces_flat[fbase + 2]
    local i3 = faces_flat[fbase + 3]
    local color = faces_flat[fbase + 4]
    local b1 = (i1 - 1) * 3
    local b2 = (i2 - 1) * 3
    local b3 = (i3 - 1) * 3
    local v1x = tv[b1 + 1];
    local v1y = tv[b1 + 2];
    local v1z = tv[b1 + 3]
    local v2x = tv[b2 + 1];
    local v2y = tv[b2 + 2];
    local v2z = tv[b2 + 3]
    local v3x = tv[b3 + 1];
    local v3y = tv[b3 + 2];
    local v3z = tv[b3 + 3]
    local normal_z = (v2x - v1x) * (v3y - v1y) - (v2y - v1y) * (v3x - v1x)
    if normal_z < 0 then
      vis_count = vis_count + 1
      vis_z[vis_count] = (v1z + v2z + v3z) * 0.3333333333
      vis_b1[vis_count] = b1
      vis_b2[vis_count] = b2
      vis_b3[vis_count] = b3
      vis_col[vis_count] = color
    end
  end
  if vis_count > 1 then
    for i = 2, vis_count do
      local zkey = vis_z[i]
      local k_b1 = vis_b1[i]
      local k_b2 = vis_b2[i]
      local k_b3 = vis_b3[i]
      local kcol = vis_col[i]
      local j = i - 1
      while j >= 1 and vis_z[j] < zkey do
        vis_z[j + 1] = vis_z[j]
        vis_b1[j + 1] = vis_b1[j]
        vis_b2[j + 1] = vis_b2[j]
        vis_b3[j + 1] = vis_b3[j]
        vis_col[j + 1] = vis_col[j]
        j = j - 1
      end
      vis_z[j + 1] = zkey
      vis_b1[j + 1] = k_b1
      vis_b2[j + 1] = k_b2
      vis_b3[j + 1] = k_b3
      vis_col[j + 1] = kcol
    end
  end
  for i = 1, vis_count do
    local b1 = vis_b1[i]
    local b2 = vis_b2[i]
    local b3 = vis_b3[i]
    lge_draw_triangle(tv[b1 + 1], tv[b1 + 2], tv[b2 + 1], tv[b2 + 2], tv[b3 + 1], tv[b3 + 2], vis_col[i])
  end
  profiling_end("Cube:draw_fast")
end
function Cube:draw()
  profiling_start("Cube:draw")
  local transformed_vertices = {}
  local faces_to_draw = {}
  local cos_x = math.cos(self.angle_x)
  local sin_x = math.sin(self.angle_x)
  local cos_y = math.cos(self.angle_y)
  local sin_y = math.sin(self.angle_y)
  local cos_z = math.cos(self.angle_z)
  local sin_z = math.sin(self.angle_z)
  for i = 1, #Cube.model_vertices do
    local v_model = Cube.model_vertices[i]
    local v = {
      x = v_model.x * self.size,
      y = v_model.y * self.size,
      z = v_model.z * self.size
    }
    local y1 = v.y * cos_x - v.z * sin_x
    local z1 = v.y * sin_x + v.z * cos_x
    local x2 = v.x * cos_y + z1 * sin_y
    local z2 = -v.x * sin_y + z1 * cos_y
    local px = x2 * cos_z - y1 * sin_z
    local py = x2 * sin_z + y1 * cos_z
    local pz = z2
    pz = pz + self.z_distance
    local z_factor = self.fov / pz
    local screen_x = px * z_factor + self.x
    local screen_y = py * z_factor + self.y
    transformed_vertices[i] = {
      x = screen_x,
      y = screen_y,
      z = pz
    }
  end
  for i = 1, #self.faces do
    local face = self.faces[i]
    local i1, i2, i3 = face[1], face[2], face[3]
    local color = face[4]
    local v1 = transformed_vertices[i1]
    local v2 = transformed_vertices[i2]
    local v3 = transformed_vertices[i3]
    local normal_z = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)
    if normal_z < 0 then
      local avg_z = (v1.z + v2.z + v3.z) / 3
      table.insert(faces_to_draw, {
        z = avg_z,
        v1 = v1,
        v2 = v2,
        v3 = v3,
        color = color
      })
    end
  end
  table.sort(faces_to_draw, function(a, b)
    return a.z > b.z
  end)
  for i = 1, #faces_to_draw do
    local f = faces_to_draw[i]
    lge.draw_triangle(f.v1.x, f.v1.y, f.v2.x, f.v2.y, f.v3.x, f.v3.y, f.color)
  end
  profiling_end("Cube:draw")
end
local NUM_CUBES = 4
local CUBE_MIN_R = 15 
local CUBE_MAX_R = 30 
local CUBE_MAX_V = 6
local FOV = 200 
local CAM_DISTANCE = 100 
local MAX_VEL = 10 
local RESTORE_RATE = 0.05 
local w, h = lge.get_canvas_size()
if not cubes then
  cubes = {}
  for i = 1, NUM_CUBES do
    local radius = math.random(CUBE_MIN_R, CUBE_MAX_R)
    local x = math.random(radius, w - radius)
    local y = math.random(radius, h - radius)
    local dx = math.random() * CUBE_MAX_V * (math.random() > 0.5 and 1 or -1)
    if math.abs(dx) < 1 then
      dx = dx * 2 * (dx >= 0 and 1 or -1)
    end
    local dy = math.random() * CUBE_MAX_V * (math.random() > 0.5 and 1 or -1)
    if math.abs(dy) < 1 then
      dy = dy * 2 * (dy >= 0 and 1 or -1)
    end
    cubes[i] = Cube:new(x, y, dx, dy, radius, MAX_VEL, RESTORE_RATE, FOV, CAM_DISTANCE)
  end
end
local function check_wall_collision(obj, w, h)
  profiling_start("check_wall_collision")
  if (obj.x - obj.r < 0 and obj.dx < 0) then
    obj.dx = -obj.dx
    obj.x = obj.r
  elseif (obj.x + obj.r > w and obj.dx > 0) then
    obj.dx = -obj.dx
    obj.x = w - obj.r
  end
  if (obj.y - obj.r < 0 and obj.dy < 0) then
    obj.dy = -obj.dy
    obj.y = obj.r
  elseif (obj.y + obj.r > h and obj.dy > 0) then
    obj.dy = -obj.dy
    obj.y = h - obj.r
  end
  profiling_end("check_wall_collision")
end
local function check_object_collision(o1, o2)
  profiling_start("check_object_collision")
  local dx = o2.x - o1.x
  local dy = o2.y - o1.y
  local dist_sq = dx * dx + dy * dy
  local min_dist = o1.r + o2.r
  if dist_sq <= min_dist * min_dist and dist_sq > 0.001 then
    local dist = math.sqrt(dist_sq)
    local overlap = min_dist - dist
    local nx = dx / dist
    local ny = dy / dist
    o1.x = o1.x - nx * overlap * 0.5
    o1.y = o1.y - ny * overlap * 0.5
    o2.x = o2.x + nx * overlap * 0.5
    o2.y = o2.y + ny * overlap * 0.5
    local vx1 = o1.dx
    local vy1 = o1.dy
    local vx2 = o2.dx
    local vy2 = o2.dy
    local rv_x = vx2 - vx1
    local rv_y = vy2 - vy1
    local v_dot_n = rv_x * nx + rv_y * ny
    if v_dot_n < 0 then
      local impulse = 2 * v_dot_n
      o1.dx = vx1 + nx * impulse
      o1.dy = vy1 + ny * impulse
      o2.dx = vx2 - nx * impulse
      o2.dy = vy2 - ny * impulse
    end
  end
  profiling_end("check_object_collision")
end
local function main_loop()
  print("Starting Bouncing Rotating Cubes... Optimized main_loop")
  local clear_canvas = lge.clear_canvas
  local draw_text = lge.draw_text
  local fps_func = lge.fps
  local present = lge.present
  local delay = lge.delay
  local check_wall_collision = check_wall_collision
  local check_object_collision = check_object_collision
  local profiling_start = profiling_start
  local profiling_end = profiling_end
  local print_profiling_results = print_profiling_results
  local cubes_ref = cubes 
  local n = #cubes_ref 
  local ww = w
  local hh = h
  while true do
    clear_canvas()
    profiling_start("main_loop")
    for i = 1, n do
      cubes_ref[i]:update()
    end
    for i = 1, n do
      check_wall_collision(cubes_ref[i], ww, hh)
    end
    for i = 1, n - 1 do
      local ci = cubes_ref[i]
      for j = i + 1, n do
        check_object_collision(ci, cubes_ref[j])
      end
    end
    for i = 1, n do
      cubes_ref[i]:draw_fast()
    end
    profiling_end("main_loop")
    local fps = fps_func()
    fps = math.floor(fps * 100 + 0.5) * 0.01
    draw_text(5, 5, "FPS: " .. fps, "#FFFFFF")
    print_profiling_results()
    present()
    delay(1)
  end
end
main_loop()
)"
  ,
  nullptr
};

const char* script_names[] = {
  "Bouncing Balls",
  "Touch Sequence",
  "Side Shooter",
  "Rotating Box",
  "Bouncing Rotating Cubes",
  nullptr
};

#endif // LUASCRIPT_H
