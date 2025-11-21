-- Tic-Tac-Toe WebSocket Game
-- Connects to a Tic-Tac-Toe server and plays the game
local width, height = lge.get_canvas_size()

-- Configuration
local WS_URL = "wss://www2.kraspel.com:8080/ws"

-- Game state
local gameState = "connecting" -- connecting, waiting, playing, game_over
local board = {nil, nil, nil, nil, nil, nil, nil, nil, nil} -- 0-8 positions
local mySymbol = nil -- "X" or "O"
local currentTurn = "X" -- X always goes first
local roomId = nil
local gameResult = nil -- "win", "loss", "draw"
local winner = nil

-- UI Constants
local BOARD_SIZE = math.min(width, height) - 40
local BOARD_X = (width - BOARD_SIZE) / 2
local BOARD_Y = 30
local CELL_SIZE = BOARD_SIZE / 3
local LINE_WIDTH = 3

-- Colors
local COLOR_BG = "#001122"
local COLOR_GRID = "#00AAFF"
local COLOR_X = "#FF3333"
local COLOR_O = "#33FF33"
local COLOR_TEXT = "#FFFFFF"
local COLOR_HIGHLIGHT = "#FFFF00"

-- Helper Functions
local function drawBoard()
    -- Draw grid lines
    -- Vertical lines
    lge.draw_rectangle(BOARD_X + CELL_SIZE - LINE_WIDTH / 2, BOARD_Y, LINE_WIDTH, BOARD_SIZE, COLOR_GRID)
    lge.draw_rectangle(BOARD_X + CELL_SIZE * 2 - LINE_WIDTH / 2, BOARD_Y, LINE_WIDTH, BOARD_SIZE, COLOR_GRID)

    -- Horizontal lines
    lge.draw_rectangle(BOARD_X, BOARD_Y + CELL_SIZE - LINE_WIDTH / 2, BOARD_SIZE, LINE_WIDTH, COLOR_GRID)
    lge.draw_rectangle(BOARD_X, BOARD_Y + CELL_SIZE * 2 - LINE_WIDTH / 2, BOARD_SIZE, LINE_WIDTH, COLOR_GRID)
end

local function drawX(cellX, cellY)
    local centerX = BOARD_X + cellX * CELL_SIZE + CELL_SIZE / 2
    local centerY = BOARD_Y + cellY * CELL_SIZE + CELL_SIZE / 2
    local size = CELL_SIZE / 3
    local offset = size * 0.7
    local thickness = 8

    -- Draw X as four triangles forming two diagonal lines
    -- First diagonal (top-left to bottom-right)
    lge.draw_triangle(centerX - offset, centerY - offset, centerX + offset, centerY + offset,
        centerX - offset + thickness, centerY - offset, COLOR_X)
    lge.draw_triangle(centerX + offset, centerY + offset, centerX + offset - thickness, centerY + offset,
        centerX - offset, centerY - offset, COLOR_X)

    -- Second diagonal (top-right to bottom-left)
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
    -- Draw inner circle to make it a ring
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

-- WebSocket message handler
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
            currentTurn = "X" -- X always goes first
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

        -- Update turn
        currentTurn = (player == "X") and "O" or "X"

    elseif message.type == "game_over" then
        gameState = "game_over"
        gameResult = message.result
        winner = message.winner

        print("Game over! Result: " .. gameResult)

        -- Reset for new game after 3 seconds
        local resetTime = millis() + 3000

        -- Store new room info
        local newRoomId = message.new_room_id
        local newStatus = message.new_status

        -- Wait a bit then reset
        while millis() < resetTime do
            lge.ws_loop()

            -- Draw game over screen
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

        -- Reset board
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

-- Connect to server
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

-- Wait for connection
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

-- Main game loop
while true do
    lge.ws_loop()

    -- Handle input
    if gameState == "playing" and isMyTurn() then
        local button, x, y = lge.get_mouse_click()
        if button ~= nil then
            local position = positionFromTouch(x, y)
            if position ~= nil and board[position + 1] == nil then
                -- Valid move
                board[position + 1] = mySymbol
                currentTurn = (mySymbol == "X") and "O" or "X"
                sendMove(position)
            end
        end
    end

    -- Draw
    lge.clear_canvas(COLOR_BG)

    -- Status text
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

    -- Draw room info
    if roomId then
        lge.draw_text(width - 100, 10, "Room: " .. roomId:sub(1, 8), "#888888")
    end

    -- Draw board
    drawBoard()

    -- Draw symbols
    for i = 0, 8 do
        drawSymbol(i)
    end

    -- Draw instructions at bottom
    if gameState == "playing" then
        lge.draw_text(10, height - 20, "Tap a cell to play", "#888888")
    end

    lge.present()
    lge.delay(16) -- ~60 FPS
end
