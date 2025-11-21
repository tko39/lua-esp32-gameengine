// Auto-generated header file from lua/tictactoe.lua lua/touchSequence.lua lua/sideShooter.lua lua/flappyBird.lua lua/orbit.lua lua/gemfall.lua
#ifndef LUASCRIPT_H
#define LUASCRIPT_H

const char* lua_scripts[] = {
  R"(
local width, height = lge.get_canvas_size()
local WS_URL = "wss://www2.kraspel.com:8080/ws" 
local gameState = "connecting" 
local board = {nil, nil, nil, nil, nil, nil, nil, nil, nil} 
local mySymbol = nil 
local currentTurn = "X" 
local roomId = nil
local gameResult = nil 
local winner = nil
local BOARD_SIZE = math.min(width, height) - 40
local BOARD_X = (width - BOARD_SIZE) / 2
local BOARD_Y = 30
local CELL_SIZE = BOARD_SIZE / 3
local LINE_WIDTH = 3
local COLOR_BG = "#001122"
local COLOR_GRID = "#00AAFF"
local COLOR_X = "#FF3333"
local COLOR_O = "#33FF33"
local COLOR_TEXT = "#FFFFFF"
local COLOR_HIGHLIGHT = "#FFFF00"
local function drawBoard()
  lge.draw_rectangle(BOARD_X + CELL_SIZE - LINE_WIDTH / 2, BOARD_Y, LINE_WIDTH, BOARD_SIZE, COLOR_GRID)
  lge.draw_rectangle(BOARD_X + CELL_SIZE * 2 - LINE_WIDTH / 2, BOARD_Y, LINE_WIDTH, BOARD_SIZE, COLOR_GRID)
  lge.draw_rectangle(BOARD_X, BOARD_Y + CELL_SIZE - LINE_WIDTH / 2, BOARD_SIZE, LINE_WIDTH, COLOR_GRID)
  lge.draw_rectangle(BOARD_X, BOARD_Y + CELL_SIZE * 2 - LINE_WIDTH / 2, BOARD_SIZE, LINE_WIDTH, COLOR_GRID)
end
local function drawX(cellX, cellY)
  local centerX = BOARD_X + cellX * CELL_SIZE + CELL_SIZE / 2
  local centerY = BOARD_Y + cellY * CELL_SIZE + CELL_SIZE / 2
  local size = CELL_SIZE / 3
  local offset = size * 0.7
  local thickness = 8
  lge.draw_triangle(centerX - offset, centerY - offset, centerX + offset, centerY + offset,
    centerX - offset + thickness, centerY - offset, COLOR_X)
  lge.draw_triangle(centerX + offset, centerY + offset, centerX + offset - thickness, centerY + offset,
    centerX - offset, centerY - offset, COLOR_X)
  lge.draw_triangle(centerX + offset, centerY - offset, centerX - offset, centerY + offset,
    centerX + offset - thickness, centerY - offset, COLOR_X)
  lge.draw_triangle(centerX - offset, centerY + offset, centerX - offset + thickness, centerY + offset,
    centerX + offset, centerY - offset, COLOR_X)
end
local function drawO(cellX, cellY)
  local centerX = BOARD_X + cellX * CELL_SIZE + CELL_SIZE / 2
  local centerY = BOARD_Y + cellY * CELL_SIZE + CELL_SIZE / 2
  local radius = CELL_SIZE / 3
  lge.draw_circle(centerX, centerY, radius, COLOR_O)
  lge.draw_circle(centerX, centerY, radius - 8, COLOR_BG)
end
local function drawSymbol(position)
  local symbol = board[position + 1]
  if symbol == nil then
    return
  end
  local cellX = position % 3
  local cellY = math.floor(position / 3)
  if symbol == "X" then
    drawX(cellX, cellY)
  elseif symbol == "O" then
    drawO(cellX, cellY)
  end
end
local function positionFromTouch(touchX, touchY)
  if touchX < BOARD_X or touchX > BOARD_X + BOARD_SIZE or touchY < BOARD_Y or touchY > BOARD_Y + BOARD_SIZE then
    return nil
  end
  local cellX = math.floor((touchX - BOARD_X) / CELL_SIZE)
  local cellY = math.floor((touchY - BOARD_Y) / CELL_SIZE)
  return cellY * 3 + cellX
end
local function isMyTurn()
  return gameState == "playing" and currentTurn == mySymbol
end
local function sendHandshake()
  local msg = '{"type":"handshake","version":"1.0"}'
  print("Sending handshake...")
  lge.ws_send(msg)
end
local function sendMove(position)
  local msg = string.format('{"type":"move","position":%d}', position)
  print("Sending move: position " .. position)
  lge.ws_send(msg)
end
lge.ws_on_message(function(message)
  if type(message) ~= "table" then
    print("Received non-JSON message: " .. tostring(message))
    return
  end
  print("Received: " .. (message.type or "unknown"))
  if message.type == "room_assigned" then
    roomId = message.room_id
    mySymbol = message.symbol
    if message.status == "waiting" then
      gameState = "waiting"
      print("Waiting for opponent... [Room: " .. roomId .. ", Symbol: " .. mySymbol .. "]")
    elseif message.status == "paired" then
      gameState = "playing"
      currentTurn = "X" 
      print("Game started! You are " .. mySymbol)
    end
  elseif message.type == "opponent_joined" then
    gameState = "playing"
    currentTurn = "X"
    print("Opponent joined! Game starting...")
  elseif message.type == "move" then
    local pos = message.position
    local player = message.player
    print("Move received: " .. player .. " at position " .. pos)
    board[pos + 1] = player
    currentTurn = (player == "X") and "O" or "X"
  elseif message.type == "game_over" then
    gameState = "game_over"
    gameResult = message.result
    winner = message.winner
    print("Game over! Result: " .. gameResult)
    local resetTime = millis() + 3000
    local newRoomId = message.new_room_id
    local newStatus = message.new_status
    while millis() < resetTime do
      lge.ws_loop()
      lge.clear_canvas(COLOR_BG)
      drawBoard()
      for i = 0, 8 do
        drawSymbol(i)
      end
      local resultText = gameResult == "win" and "YOU WIN!" or gameResult == "loss" and "YOU LOSE!" or "DRAW!"
      local resultColor = gameResult == "win" and "#00FF00" or gameResult == "loss" and "#FF0000" or "#FFFF00"
      lge.draw_text(10, 10, resultText, resultColor)
      lge.draw_text(10, height - 20, "New game starting...", COLOR_TEXT)
      lge.present()
      lge.delay(16)
    end
    board = {nil, nil, nil, nil, nil, nil, nil, nil, nil}
    roomId = newRoomId
    currentTurn = "X"
    gameState = newStatus == "paired" and "playing" or "waiting"
    gameResult = nil
    winner = nil
  elseif message.type == "error" then
    print("Error: " .. (message.message or "Unknown error"))
  end
end)
print("Connecting to " .. WS_URL)
lge.clear_canvas(COLOR_BG)
lge.draw_text(10, 10, "Connecting to WiFi...", COLOR_TEXT)
lge.present()
if lge.ws_connect(WS_URL) then
  gameState = "connecting"
else
  print("Failed to connect!")
  gameState = "error"
end
local connectTimeout = millis() + 5000
if (gameState ~= "error") then
  while not lge.ws_is_connected() and millis() < connectTimeout do
    lge.ws_loop()
    lge.clear_canvas(COLOR_BG)
    lge.draw_text(10, 10, "Connecting to Server...", COLOR_TEXT)
    lge.present()
    lge.delay(100)
  end
  if lge.ws_is_connected() then
    sendHandshake()
  else
    print("Connection timeout!")
    gameState = "error"
  end
end
while true do
  lge.ws_loop()
  if gameState == "playing" and isMyTurn() then
    local button, x, y = lge.get_mouse_click()
    if button ~= nil then
      local position = positionFromTouch(x, y)
      if position ~= nil and board[position + 1] == nil then
        board[position + 1] = mySymbol
        currentTurn = (mySymbol == "X") and "O" or "X"
        sendMove(position)
      end
    end
  end
  lge.clear_canvas(COLOR_BG)
  local statusText = ""
  local statusColor = COLOR_TEXT
  if gameState == "connecting" then
    statusText = "Connecting..."
  elseif gameState == "waiting" then
    statusText = "Waiting for opponent..."
    statusColor = COLOR_HIGHLIGHT
  elseif gameState == "playing" then
    if isMyTurn() then
      statusText = "Your turn [" .. mySymbol .. "]"
      statusColor = COLOR_HIGHLIGHT
    else
      statusText = "Opponent's turn"
    end
  elseif gameState == "error" then
    statusText = "Connection error!"
    statusColor = "#FF0000"
  end
  lge.draw_text(10, 10, statusText, statusColor)
  if roomId then
    lge.draw_text(width - 100, 10, "Room: " .. roomId:sub(1, 8), "#888888")
  end
  drawBoard()
  for i = 0, 8 do
    drawSymbol(i)
  end
  if gameState == "playing" then
    lge.draw_text(10, height - 20, "Tap a cell to play", "#888888")
  end
  lge.present()
  lge.delay(16) 
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
local touch_x, touch_y, touch_state = nil, nil, nil
local key_pressed, key_state = nil, nil
local FRAME_DELAY = 18 
local COLORS = {
  bg = "#4AB6FF",
  pipe = "#00C800",
  bird = "#FFDB00",
  birdEye = "#000000",
  ground = "#B56D00",
  text = "#FFFFFF",
  gameOver = "#FF5050"
}
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
  for i = 1, 3 do
    spawnPipe(SCREEN_W + (i - 1) * PIPE_SPACING)
  end
end
local function updatePipes()
  for i = #pipes, 1, -1 do
    local pipe = pipes[i]
    pipe.x = pipe.x - PIPE_SPEED
    if not pipe.passed and pipe.x + PIPE_W < BIRD_X then
      score = score + 1
      pipe.passed = true
      if score > (highScore or 0) then
        highScore = score
      end
    end
    if pipe.x + PIPE_W < 0 then
      table.remove(pipes, i)
    end
  end
  if #pipes == 0 or (pipes[#pipes].x < SCREEN_W - PIPE_SPACING) then
    spawnPipe(SCREEN_W)
  end
end
local function checkCollision()
  if birdY + BIRD_SIZE / 2 > GROUND_Y or birdY - BIRD_SIZE / 2 < 0 then
    return true
  end
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
    lge.draw_rectangle(pipe.x, 0, PIPE_W, pipe.gapY, COLORS.pipe)
    lge.draw_rectangle(pipe.x, pipe.gapY + PIPE_GAP, PIPE_W, GROUND_Y - (pipe.gapY + PIPE_GAP), COLORS.pipe)
  end
end
local function drawBird()
  lge.draw_circle(BIRD_X, birdY, BIRD_SIZE / 2, COLORS.bird)
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
)"
  ,
  R"(
math.randomseed(millis() % 1000000)
local FOV = 220
local CAM_DISTANCE = 180
lge.set_3d_camera(FOV, CAM_DISTANCE)
local LIGHT_AMBIENT = 0.25
local LIGHT_DIFFUSE = 0.9
local light_dx, light_dy, light_dz = 0.0, 1.0, -1.0
lge.set_3d_light(light_dx, light_dy, light_dz, LIGHT_AMBIENT, LIGHT_DIFFUSE)
local SCREEN_W, SCREEN_H = lge.get_canvas_size()
local vertices = {0.000000, -0.525731, 0.850651, 0.850651, 0.000000, 0.525731, 0.850651, 0.000000, -0.525731, -0.850651,
          0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.525731, 0.850651, 0.000000, 0.525731, 0.850651,
          0.000000, 0.525731, -0.850651, 0.000000, -0.525731, -0.850651, 0.000000, 0.000000, -0.525731,
          -0.850651, 0.000000, 0.525731, -0.850651, 0.000000, 0.525731, 0.850651}
local faces = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12, 1,
         2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}
local MODEL_ID = lge.create_3d_model(vertices, faces)
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
local SUN_Z = 180
local SUN_RADIUS = 28
local ORBIT_RADIUS_1 = 70
local ORBIT_RADIUS_2 = 110
local PLANET1_RADIUS = 12
local PLANET2_RADIUS = 9
local ORBIT_SPEED_1 = 0.02 
local ORBIT_SPEED_2 = -0.013 
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
  orbit_angle = math.pi * 0.4, 
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
local MAX_TILT = math.rad(45) 
local tiltX = 0.0 
local tiltY = 0.0 
local function update_tilt_from_mouse()
  local _, mx, my = lge.get_mouse_position()
  if not mx then
    return
  end
  local nx = (mx - SCREEN_W / 2) / (SCREEN_W / 2)
  local ny = (my - SCREEN_H / 2) / (SCREEN_H / 2)
  tiltY = nx * MAX_TILT
  tiltX = ny * MAX_TILT
end
local function apply_orbit_tilt(local_x, local_y, local_z)
  local cosX = math.cos(tiltX)
  local sinX = math.sin(tiltX)
  local y1 = local_y * cosX - local_z * sinX
  local z1 = local_y * sinX + local_z * cosX
  local x1 = local_x
  local cosY = math.cos(tiltY)
  local sinY = math.sin(tiltY)
  local x2 = x1 * cosY + z1 * sinY
  local z2 = -x1 * sinY + z1 * cosY
  local y2 = y1
  return sun.x + x2, sun.y + y2, sun.z + z2
end
local function main_loop()
  local clear_canvas = lge.clear_canvas
  local draw_text = lge.draw_text
  local fps_func = lge.fps
  local present = lge.present
  local delay = lge.delay
  while true do
    clear_canvas("#000000")
    update_tilt_from_mouse()
    sun.angle_x = sun.angle_x + sun.d_angle_x
    sun.angle_y = sun.angle_y + sun.d_angle_y
    sun.angle_z = sun.angle_z + sun.d_angle_z
    planet1.orbit_angle = planet1.orbit_angle + ORBIT_SPEED_1
    local px1 = math.cos(planet1.orbit_angle) * ORBIT_RADIUS_1
    local py1 = 0
    local pz1 = math.sin(planet1.orbit_angle) * ORBIT_RADIUS_1
    planet1.x, planet1.y, planet1.z = apply_orbit_tilt(px1, py1, pz1)
    planet1.angle_x = planet1.angle_x + planet1.d_angle_x
    planet1.angle_y = planet1.angle_y + planet1.d_angle_y
    planet1.angle_z = planet1.angle_z + planet1.d_angle_z
    planet2.orbit_angle = planet2.orbit_angle + ORBIT_SPEED_2
    local px2 = math.cos(planet2.orbit_angle) * ORBIT_RADIUS_2
    local py2 = 0
    local pz2 = math.sin(planet2.orbit_angle) * ORBIT_RADIUS_2
    planet2.x, planet2.y, planet2.z = apply_orbit_tilt(px2, py2, pz2)
    planet2.angle_x = planet2.angle_x + planet2.d_angle_x
    planet2.angle_y = planet2.angle_y + planet2.d_angle_y
    planet2.angle_z = planet2.angle_z + planet2.d_angle_z
    moon.orbit_angle = moon.orbit_angle + MOON_ORBIT_SPEED
    local mpx = px1 + math.cos(moon.orbit_angle) * MOON_ORBIT_RADIUS
    local mpy = 0
    local mpz = pz1 + math.sin(moon.orbit_angle) * MOON_ORBIT_RADIUS
    moon.x, moon.y, moon.z = apply_orbit_tilt(mpx, mpy, mpz)
    moon.angle_x = moon.angle_x + moon.d_angle_x
    moon.angle_y = moon.angle_y + moon.d_angle_y
    moon.angle_z = moon.angle_z + moon.d_angle_z
    local bodies = {sun, planet1, planet2, moon}
    table.sort(bodies, function(a, b)
      return a.z > b.z
    end)
    for i = 1, #bodies do
      local o = bodies[i]
      lge.draw_3d_instance(o.instance_id, o.x, o.y, o.z, o.r, o.angle_x, o.angle_y, o.angle_z)
    end
    local fps = fps_func()
    fps = math.floor(fps * 100 + 0.5) / 100
    draw_text(5, 5, "Orbit demo FPS: " .. fps, "#FFFFFF")
    draw_text(5, 20, "Touch/drag = tilt orbit plane", "#AAAAAA")
    present()
    delay(1)
  end
end
main_loop()
)"
  ,
  R"(
math.randomseed(os.time())
local BOARD_SIZE = 8
local SCREEN_W, SCREEN_H = lge.get_canvas_size()
local GEM_VERTICES = {0.000000, -0.525731, 0.850651, 0.850651, 0.000000, 0.525731, 0.850651, 0.000000, -0.525731,
            -0.850651, 0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.525731, 0.850651, 0.000000,
            0.525731, 0.850651, 0.000000, 0.525731, -0.850651, 0.000000, -0.525731, -0.850651, 0.000000,
            0.000000, -0.525731, -0.850651, 0.000000, 0.525731, -0.850651, 0.000000, 0.525731, 0.850651}
local GEM_FACES = {2, 3, 7, 2, 8, 3, 4, 5, 6, 5, 4, 9, 7, 6, 12, 6, 7, 11, 10, 11, 3, 11, 10, 4, 8, 9, 10, 9, 8, 1, 12,
           1, 2, 1, 12, 5, 7, 3, 11, 2, 7, 12, 4, 6, 11, 6, 5, 12, 3, 8, 10, 8, 2, 1, 4, 10, 9, 5, 9, 1}
local GEM_COLORS = {"#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF", "#FF8800"}
local GEM_NORMAL = 0
local GEM_LINE_CLEAR = 1
local GEM_BOMB = 2
local GEM_HYPER_CUBE = 3
local SCORE_MATCH_3 = 10
local SCORE_MATCH_4 = 20
local SCORE_MATCH_5 = 50
local MAX_CASCADE_MULTIPLIER = 10
local GEM_MATCH_START_SIZE = 0.5
local GEM_MATCH_SHRINK = 0.1
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
local GEM_SIZE = math.min(SCREEN_W, SCREEN_H) / (BOARD_SIZE + 2)
local BOARD_OFFSET_X = (SCREEN_W - GEM_SIZE * BOARD_SIZE) / 2
local BOARD_OFFSET_Y = 40 
local GEM_3D_Z = 220 
local GEM_3D_RADIUS = GEM_SIZE * 0.45 
local FOV = 220
local CAM_DISTANCE = 220
local LIGHT_AMBIENT = 0.3
local LIGHT_DIFFUSE = 0.7
local ANIM_SWAP_TIME = 0.2
local ANIM_MATCH_TIME = 0.15
local STATE_IDLE = 0
local STATE_SELECTED = 1
local STATE_SWAPPING = 2
local STATE_MATCHING = 3
local STATE_FALLING = 4
local STATE_GAME_OVER = 5
local board = {}
local gem_types = {}
local gem_y_offsets = {} 
local gem_scales = {} 
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
  t = 0, 
  valid = false
}
local SWAP_ANIM_DURATION = 0.18 
local swap_anim_dx, swap_anim_dy = 0, 0
local MAX_MATCHES = 5 
local MAX_GEMS_PER_MATCH = BOARD_SIZE 
local match_sizes = {} 
local match_gem_x = {} 
local match_gem_y = {} 
for i = 1, MAX_MATCHES do
  match_sizes[i] = 0
end
for i = 1, MAX_MATCHES * MAX_GEMS_PER_MATCH do
  match_gem_x[i] = 0
  match_gem_y[i] = 0
end
local checked_array = {}
for y = 1, BOARD_SIZE do
  checked_array[y] = {}
  for x = 1, BOARD_SIZE do
    checked_array[y][x] = false
  end
end
local h_gems_temp = {}
local v_gems_temp = {}
for i = 1, BOARD_SIZE do
  h_gems_temp[i] = 0
  v_gems_temp[i] = 0
end
local match_sizes_temp = {}
for i = 1, MAX_MATCHES do
  match_sizes_temp[i] = 0
end
local matches_result = {}
for i = 1, MAX_MATCHES do
  matches_result[i] = nil
end
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
local function setup_3d_models()
  lge.set_3d_camera(FOV, CAM_DISTANCE)
  lge.set_3d_light(0, 1, -0.5, LIGHT_AMBIENT, LIGHT_DIFFUSE)
  model_id = lge.create_3d_model(GEM_VERTICES, GEM_FACES)
  local num_faces = #GEM_FACES / 3
  for color_idx = 1, #GEM_COLORS do
    local tri_colors = {}
    for i = 1, num_faces do
      tri_colors[i] = GEM_COLORS[color_idx]
    end
    gem_instances[color_idx] = lge.create_3d_instance(model_id, tri_colors)
  end
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
local function reset_checked_array()
  for y = 1, BOARD_SIZE do
    for x = 1, BOARD_SIZE do
      checked_array[y][x] = false
    end
  end
end
local function get_match_gem(match_idx, gem_idx)
  local base = (match_idx - 1) * MAX_GEMS_PER_MATCH
  return match_gem_x[base + gem_idx], match_gem_y[base + gem_idx]
end
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
  local fall_speed = GEM_SIZE * 0.5 
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
        gem_y_offsets[y][x] = -(BOARD_SIZE - y) * GEM_SIZE
        filled = true
      end
    end
  end
  return filled
end
local function has_valid_moves()
  for y = 1, BOARD_SIZE do
    for x = 1, BOARD_SIZE do
      if x < BOARD_SIZE then
        board[y][x], board[y][x + 1] = board[y][x + 1], board[y][x]
        local match1 = check_match_at(x, y, -1, -1)
        local match2 = check_match_at(x + 1, y, -1, -1)
        board[y][x], board[y][x + 1] = board[y][x + 1], board[y][x]
        if match1 or match2 then
          return true
        end
      end
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
  board[y1][x1], board[y2][x2] = board[y2][x2], board[y1][x1]
  local match1 = check_match_at(x1, y1, -1, -1)
  local match2 = check_match_at(x2, y2, -1, -1)
  board[y1][x1], board[y2][x2] = board[y2][x2], board[y1][x1]
  return match1 or match2
end
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
local function update_rotations()
  for y = 1, BOARD_SIZE do
    for x = 1, BOARD_SIZE do
      local rot = rotation_angles[y][x]
      rot.x = rot.x + rot.dx
      rot.y = rot.y + rot.dy
      rot.z = rot.z + rot.dz
      if gem_scales[y][x] <= GEM_MATCH_START_SIZE and board[y][x] > 0 then
        gem_scales[y][x] = gem_scales[y][x] - GEM_MATCH_SHRINK
        if gem_scales[y][x] < 0.01 then
          gem_scales[y][x] = 0.01
        end
      elseif gem_scales[y][x] < 1.0 and board[y][x] > 0 then
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
        if swap_anim.active then
          local t = swap_anim.t
          if (x == swap_anim.x1 and y == swap_anim.y1) or (x == swap_anim.x2 and y == swap_anim.y2) then
            local dirx = swap_anim.x2 - swap_anim.x1
            local diry = swap_anim.y2 - swap_anim.y1
            local anim_t = t
            if not swap_anim.valid then
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
  lge.draw_text(10, 10, "Score: " .. score, "#FFFFFF")
  lge.draw_text(10, 25, "Best: " .. best_score, "#FFFF00")
  local fps = lge.fps()
  fps = math.floor(fps * 100 + 0.5) / 100
  lge.draw_text(SCREEN_W - 80, 10, "FPS: " .. fps, "#00FF00")
  lge.draw_text(SCREEN_W - 100, 25, "Colors: " .. current_colors, "#00FFFF")
end
local function draw_game_over()
  local text = "GAME OVER"
  local text_x = SCREEN_W / 2 - 50
  local text_y = SCREEN_H / 2 - 20
  lge.draw_rectangle(text_x - 20, text_y - 10, 140, 60, "#000000")
  lge.draw_rectangle(text_x - 18, text_y - 8, 136, 56, "#FF0000")
  lge.draw_rectangle(text_x - 16, text_y - 6, 132, 52, "#000000")
  lge.draw_text(text_x, text_y, text, "#FFFFFF")
  lge.draw_text(text_x - 10, text_y + 20, "Click to restart", "#FFFF00")
end
local function process_cascades()
  local cascade_level = 1
  while true do
    local match_count = find_matches()
    if match_count == 0 then
      break
    end
    local removed, sizes_count = remove_matches(match_count)
    if removed > 0 then
      for i = 1, 8 do
        update_rotations()
        lge.clear_canvas("#1a1a2e")
        draw_board()
        draw_ui()
        lge.present()
        lge.delay(20)
      end
      clear_matched_gems(match_count)
      add_score(sizes_count, cascade_level)
    end
    apply_gravity()
    while animate_falling() do
      update_rotations()
      lge.clear_canvas("#1a1a2e")
      draw_board()
      draw_ui()
      lge.present()
      lge.delay(16)
    end
    refill_board()
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
    return true 
  end
  return false
end
local function reset_swap_animation()
  swap_anim.active = false
  swap_anim.x1, swap_anim.y1, swap_anim.x2, swap_anim.y2 = nil, nil, nil, nil
  swap_anim.t = 0
  swap_anim.valid = false
end
local swap_pending = false
local swap_from_x, swap_from_y, swap_to_x, swap_to_y = nil, nil, nil, nil
local function handle_input()
  if swap_anim.active then
    return
  end 
  local button, mx, my = lge.get_mouse_click()
  if not mx then
    return
  end
  if game_state == STATE_GAME_OVER then
    game_state = STATE_IDLE
    selected_x = nil
    selected_y = nil
    score = 0
    current_colors = 5
    fill_board_no_matches()
    while not has_valid_moves() do
      fill_board_no_matches()
    end
    return
  end
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
        selected_x = nil
        selected_y = nil
        game_state = STATE_IDLE
      elseif is_adjacent(selected_x, selected_y, bx, by) then
        local valid = would_create_match(selected_x, selected_y, bx, by)
        swap_from_x, swap_from_y = selected_x, selected_y
        swap_to_x, swap_to_y = bx, by
        swap_pending = valid
        start_swap_animation(selected_x, selected_y, bx, by, valid)
        selected_x = nil
        selected_y = nil
        game_state = STATE_SWAPPING
      else
        if board[by][bx] > 0 then
          selected_x = bx
          selected_y = by
        end
      end
    end
  end
end
local function game_setup()
  setup_3d_models()
  create_empty_board()
  fill_board_no_matches()
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
  if game_state == STATE_SWAPPING and swap_anim.active then
    local finished = update_swap_animation(dt)
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
        board[swap_from_y][swap_from_x], board[swap_to_y][swap_to_x] = board[swap_to_y][swap_to_x],
          board[swap_from_y][swap_from_x]
        gem_types[swap_from_y][swap_from_x], gem_types[swap_to_y][swap_to_x] = gem_types[swap_to_y][swap_to_x],
          gem_types[swap_from_y][swap_from_x]
        game_state = STATE_MATCHING
      else
        game_state = STATE_IDLE
      end
      swap_pending = false
    end
    return
  end
  if game_state == STATE_MATCHING then
    process_cascades()
    if not has_valid_moves() then
      game_state = STATE_GAME_OVER
      if score > best_score then
        best_score = score
      end
    else
      game_state = STATE_IDLE
    end
  end
  update_rotations()
  handle_input()
  lge.clear_canvas("#1a1a2e")
  draw_board()
  draw_ui()
  if game_state == STATE_GAME_OVER then
    draw_game_over()
  end
  lge.present()
  lge.delay(16)
end
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
)"
  ,
  nullptr
};

const char* script_names[] = {
  "Tictactoe",
  "Touch Sequence",
  "Side Shooter",
  "Flappy Bird",
  "Orbit",
  "Gemfall",
  nullptr
};

#endif // LUASCRIPT_H
