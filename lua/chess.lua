-- Real-Time Chess Game
-- Connects to a Chess server via WebSocket and plays chess
local width, height = lge.get_canvas_size()

-- Configuration
local WS_URL = "ws://192.168.1.208:8081/ws" -- Update with your server address

-- Game state
local gameState = "disconnected" -- disconnected, connected, queued, playing, game_over
local userId = "esp32-" .. tostring(math.random(1000, 9999))
local gameId = nil
local playerColor = nil -- "white" or "black"
local currentTurn = "white"
local gameResult = nil
local gameReason = nil

-- Board state (8x8, white on bottom)
local board = {}
local selectedSquare = nil -- {row, col}
local validMoves = {}

-- Piece encoding: "wP" = white pawn, "bR" = black rook, etc.
-- Pieces: P=pawn, R=rook, N=knight, B=bishop, Q=queen, K=king

-- UI Constants
local BOARD_SIZE = math.min(width, height) - 60
local BOARD_X = math.floor((width - BOARD_SIZE) / 2)
local BOARD_Y = 40
local CELL_SIZE = math.floor(BOARD_SIZE / 8)

-- Colors
local COLOR_BG = "#1a1a2e"
local COLOR_LIGHT_SQUARE = "#f0d9b5"
local COLOR_DARK_SQUARE = "#b58863"
local COLOR_SELECTED = "#ffff00"
local COLOR_VALID_MOVE = "#90ee90"
local COLOR_TEXT = "#ffffff"
local COLOR_STATUS = "#00ffff"

-- Helper Functions
local function initBoard()
    -- Initialize standard chess starting position
    board = {{"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"}, {"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
             {nil, nil, nil, nil, nil, nil, nil, nil}, {nil, nil, nil, nil, nil, nil, nil, nil},
             {nil, nil, nil, nil, nil, nil, nil, nil}, {nil, nil, nil, nil, nil, nil, nil, nil},
             {"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"}, {"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"}}
end

local function parseFEN(fen)
    -- Parse FEN string and update board
    -- FEN format: pieces/ranks, turn, castling, en passant, halfmove, fullmove
    local parts = {}
    for part in fen:gmatch("%S+") do
        table.insert(parts, part)
    end

    if not parts[1] then
        return
    end

    local ranks = {}
    for rank in parts[1]:gmatch("[^/]+") do
        table.insert(ranks, rank)
    end

    -- Clear board
    for row = 1, 8 do
        board[row] = {}
        for col = 1, 8 do
            board[row][col] = nil
        end
    end

    -- Parse each rank
    for rankIdx, rankStr in ipairs(ranks) do
        local col = 1
        for i = 1, #rankStr do
            local char = rankStr:sub(i, i)
            if char:match("%d") then
                -- Empty squares
                col = col + tonumber(char)
            else
                -- Piece
                local color = char:upper() == char and "w" or "b"
                local piece = char:upper()
                board[rankIdx][col] = color .. piece
                col = col + 1
            end
        end
    end

    -- Update turn
    if parts[2] then
        currentTurn = parts[2] == "w" and "white" or "black"
    end
end

local function squareToNotation(row, col)
    -- Convert row,col to chess notation (e.g., e2)
    local files = "abcdefgh"
    local rank = 9 - row
    return files:sub(col, col) .. tostring(rank)
end

local function notationToSquare(notation)
    -- Convert chess notation to row,col
    local files = "abcdefgh"
    local file = notation:sub(1, 1)
    local rank = tonumber(notation:sub(2, 2))

    local col = files:find(file)
    local row = 9 - rank

    return row, col
end

local function drawBoard()
    -- Draw checkerboard
    for row = 1, 8 do
        for col = 1, 8 do
            local x = math.floor(BOARD_X + (col - 1) * CELL_SIZE)
            local y = math.floor(BOARD_Y + (row - 1) * CELL_SIZE)

            -- Determine square color
            local isLight = (row + col) % 2 == 0
            local color = isLight and COLOR_LIGHT_SQUARE or COLOR_DARK_SQUARE

            -- Highlight selected square
            if selectedSquare and selectedSquare.row == row and selectedSquare.col == col then
                color = COLOR_SELECTED
            end

            -- Highlight valid moves
            for _, move in ipairs(validMoves) do
                if move.row == row and move.col == col then
                    color = COLOR_VALID_MOVE
                    break
                end
            end

            lge.draw_rectangle(x + 1, y + 1, math.floor(CELL_SIZE) - 2, math.floor(CELL_SIZE) - 2, color)
        end
    end
end

local function drawPiece(piece, x, y, size)
    -- Draw simplified piece representations
    local color = piece:sub(1, 1) == "w" and "#ffffff" or "#000000"
    local pieceType = piece:sub(2, 2)

    local centerX = math.floor(x + size / 2)
    local centerY = math.floor(y + size / 2)
    local radius = math.floor(size / 3)

    -- Draw piece base (circle)
    lge.draw_circle(centerX, centerY, radius, color)

    -- Draw piece symbol
    local symbolColor = piece:sub(1, 1) == "w" and "#000000" or "#ffffff"
    local symbolY = math.floor(centerY - 4)

    -- Simple text representation
    lge.draw_text(math.floor(centerX - 4), symbolY, pieceType, symbolColor)
end

local function drawPieces()
    -- Draw all pieces on the board
    for row = 1, 8 do
        for col = 1, 8 do
            local piece = board[row][col]
            if piece then
                local x = math.floor(BOARD_X + (col - 1) * CELL_SIZE)
                local y = math.floor(BOARD_Y + (row - 1) * CELL_SIZE)
                drawPiece(piece, x, y, CELL_SIZE)
            end
        end
    end
end

local function touchToSquare(touchX, touchY)
    -- Convert touch coordinates to board square
    if touchX < BOARD_X or touchX > BOARD_X + BOARD_SIZE or touchY < BOARD_Y or touchY > BOARD_Y + BOARD_SIZE then
        return nil, nil
    end

    local col = math.floor((touchX - BOARD_X) / CELL_SIZE) + 1
    local row = math.floor((touchY - BOARD_Y) / CELL_SIZE) + 1

    if row < 1 or row > 8 or col < 1 or col > 8 then
        return nil, nil
    end

    return row, col
end

local function isMyTurn()
    return gameState == "playing" and
               ((playerColor == "white" and currentTurn == "white") or
                   (playerColor == "black" and currentTurn == "black"))
end

local function sendAuth()
    local msg = string.format('{"type":"AUTH","userId":"%s"}', userId)
    print("Sending AUTH...")
    lge.ws_send(msg)
end

local function sendJoinQueue()
    local msg = string.format('{"type":"JOIN_QUEUE","userId":"%s"}', userId)
    print("Joining queue...")
    lge.ws_send(msg)
end

local function sendMove(fromNotation, toNotation, promotion)
    -- Send move in UCI format (e.g., "e2e4" or "e7e8q" for promotion)
    local uciMove = fromNotation .. toNotation
    if promotion then
        uciMove = uciMove .. promotion
    end
    local msg = string.format('{"type":"MAKE_MOVE","gameId":"%s","move":"%s"}', gameId, uciMove)
    print("Sending move: " .. uciMove)
    lge.ws_send(msg)
end

local function sendResign()
    local msg = string.format('{"type":"RESIGN","gameId":"%s"}', gameId)
    print("Resigning...")
    lge.ws_send(msg)
end

-- WebSocket message handler
lge.ws_on_message(function(message)
    if type(message) ~= "table" then
        print("Received non-JSON: " .. tostring(message))
        return
    end

    print("Received: " .. (message.type or "unknown"))

    if message.type == "MATCH_FOUND" then
        gameId = message.gameId
        playerColor = message.color
        gameState = "playing"

        if message.fen then
            parseFEN(message.fen)
        else
            initBoard()
        end

        print("Match found! You are " .. playerColor)
        print("Opponent: " .. (message.opponentId or "unknown"))

    elseif message.type == "GAME_UPDATE" then
        if message.fen then
            parseFEN(message.fen)
        end

        if message.currentTurn then
            currentTurn = message.currentTurn
        end

        print("Board updated. Turn: " .. currentTurn)

        -- Clear selection after move
        selectedSquare = nil
        validMoves = {}

    elseif message.type == "MOVE_REJECTED" then
        print("Move rejected: " .. (message.reason or "unknown"))

        -- Restore board state
        if message.fen then
            parseFEN(message.fen)
        end

        -- Clear selection
        selectedSquare = nil
        validMoves = {}

    elseif message.type == "GAME_END" then
        gameState = "game_over"
        gameResult = message.result
        gameReason = message.reason

        if message.fen then
            parseFEN(message.fen)
        end

        print("Game Over! Result: " .. gameResult)
        print("Reason: " .. gameReason)

        -- Reset after delay
        local resetTime = millis() + 5000
        while millis() < resetTime do
            lge.ws_loop()

            -- Draw game over screen
            lge.clear_canvas(COLOR_BG)
            drawBoard()
            drawPieces()

            local resultText = gameResult == "WIN" and "YOU WON!" or gameResult == "DRAW" and "DRAW!" or "YOU LOST!"
            local resultColor = gameResult == "WIN" and "#00ff00" or gameResult == "DRAW" and "#ffff00" or "#ff0000"

            lge.draw_text(10, 10, resultText, resultColor)
            lge.draw_text(10, 25, "Reason: " .. gameReason, COLOR_TEXT)
            lge.draw_text(10, height - 15, "Returning to queue...", COLOR_STATUS)

            lge.present()
            lge.delay(16)
        end

        -- Reset state and rejoin queue
        gameId = nil
        playerColor = nil
        gameResult = nil
        gameReason = nil
        initBoard()
        gameState = "connected"
        sendJoinQueue()
        gameState = "queued"

    elseif message.type == "OPPONENT_DISCONNECTED" then
        print("Opponent disconnected")

    elseif message.type == "PONG" then
        -- Heartbeat response
        print("Pong received")
    end
end)

-- Initialize board
initBoard()

-- Connect to server
print("Connecting to " .. WS_URL)
lge.clear_canvas(COLOR_BG)
lge.draw_text(10, 10, "Connecting...", COLOR_TEXT)
lge.present()

if lge.ws_connect(WS_URL) then
    print("Connection initiated")
else
    print("Failed to connect!")
    gameState = "error"
end

-- Wait for connection
local connectTimeout = millis() + 10000
if gameState ~= "error" then
    while not lge.ws_is_connected() and millis() < connectTimeout do
        lge.ws_loop()

        lge.clear_canvas(COLOR_BG)
        lge.draw_text(10, 10, "Connecting to server...", COLOR_TEXT)
        lge.present()
        lge.delay(100)
    end

    if lge.ws_is_connected() then
        gameState = "connected"
        sendAuth()
        lge.delay(500)
        sendJoinQueue()
        gameState = "queued"
    else
        print("Connection timeout!")
        gameState = "error"
    end
end

collectgarbage("generational", 10, 50)
local framesBeforeTest = 100

-- Main game loop
while true do
    lge.ws_loop()

    -- Handle input
    if isMyTurn() then
        local button, x, y = lge.get_mouse_click()
        if button ~= nil then
            local row, col = touchToSquare(x, y)

            if row and col then
                local piece = board[row][col]
                local myColorPrefix = playerColor == "white" and "w" or "b"

                -- If we have a piece selected, try to move
                if selectedSquare then
                    -- Check if this is a valid move destination
                    local isValidMove = false
                    for _, move in ipairs(validMoves) do
                        if move.row == row and move.col == col then
                            isValidMove = true
                            break
                        end
                    end

                    if isValidMove then
                        -- Make the move
                        local fromNotation = squareToNotation(selectedSquare.row, selectedSquare.col)
                        local toNotation = squareToNotation(row, col)

                        -- Check for pawn promotion
                        local selectedPiece = board[selectedSquare.row][selectedSquare.col]
                        local promotion = nil
                        if selectedPiece and selectedPiece:sub(2, 2) == "P" then
                            -- White pawn reaching rank 8 (row 1) or black pawn reaching rank 1 (row 8)
                            if (playerColor == "white" and row == 1) or (playerColor == "black" and row == 8) then
                                promotion = "q" -- Always promote to queen
                            end
                        end

                        sendMove(fromNotation, toNotation, promotion)

                        -- Clear selection
                        selectedSquare = nil
                        validMoves = {}
                    else
                        -- Deselect or select new piece
                        if piece and piece:sub(1, 1) == myColorPrefix then
                            selectedSquare = {
                                row = row,
                                col = col
                            }
                            -- For simplicity, mark all empty squares as valid moves
                            -- (Server will validate actual moves)
                            validMoves = {}
                            for r = 1, 8 do
                                for c = 1, 8 do
                                    if not board[r][c] or board[r][c]:sub(1, 1) ~= myColorPrefix then
                                        table.insert(validMoves, {
                                            row = r,
                                            col = c
                                        })
                                    end
                                end
                            end
                        else
                            selectedSquare = nil
                            validMoves = {}
                        end
                    end
                    -- Select a piece
                elseif piece and piece:sub(1, 1) == myColorPrefix then
                    selectedSquare = {
                        row = row,
                        col = col
                    }
                    -- Mark potential move squares (simplified - all non-friendly squares)
                    validMoves = {}
                    for r = 1, 8 do
                        for c = 1, 8 do
                            if not board[r][c] or board[r][c]:sub(1, 1) ~= myColorPrefix then
                                table.insert(validMoves, {
                                    row = r,
                                    col = c
                                })
                            end
                        end
                    end
                end
            end
        end
    end

    -- Draw
    lge.clear_canvas(COLOR_BG)

    -- Status text
    local statusText = ""
    local statusColor = COLOR_STATUS

    if gameState == "disconnected" or gameState == "error" then
        statusText = "Disconnected!"
        statusColor = "#ff0000"
    elseif gameState == "connected" then
        statusText = "Connected"
    elseif gameState == "queued" then
        statusText = "Waiting for opponent..."
        statusColor = "#ffff00"
    elseif gameState == "playing" then
        if isMyTurn() then
            statusText = "Your turn [" .. playerColor .. "]"
            statusColor = "#00ff00"
        else
            statusText = "Opponent's turn"
            statusColor = "#ff8800"
        end
    end

    lge.draw_text(10, 10, statusText, statusColor)

    -- Draw user ID
    lge.draw_text(10, 25, "ID: " .. userId, "#888888")

    -- Draw board and pieces
    drawBoard()
    drawPieces()

    -- Draw instructions
    if gameState == "playing" and isMyTurn() then
        lge.draw_text(10, math.floor(height - 15), "Tap piece, then destination", "#888888")
    end

    lge.present()
    lge.delay(40) -- ~60 FPS

    framesBeforeTest = framesBeforeTest - 1
    if framesBeforeTest == 0 then
        print(("Mem: %.1f KB"):format(collectgarbage("count")))
        framesBeforeTest = 100
    end

    if framesBeforeTest % 20 == 0 then
        collectgarbage("step", 10)
    end
end
