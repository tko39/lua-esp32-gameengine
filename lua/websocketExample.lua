-- WebSocket Example
-- Demonstrates connecting to a WebSocket server, sending/receiving messages
local width, height = lge.get_canvas_size()
local status = "Initializing..."
local messageCount = 0
local lastMessage = ""

-- WebSocket configuration
local WS_URL = "wss://echo.websocket.org"

-- Register callback for incoming WebSocket messages
lge.ws_on_message(function(message)
    print("Received WebSocket message:")

    -- If message is a table (parsed JSON)
    if type(message) == "table" then
        if message.type then
            print("  Type: " .. message.type)
            lastMessage = "Type: " .. message.type
        end
        -- You can access other fields: message.data, message.id, etc.
        for k, v in pairs(message) do
            print("  " .. k .. ": " .. tostring(v))
        end
    else
        -- Raw string message
        print("  " .. tostring(message))
        lastMessage = tostring(message)
    end

    messageCount = messageCount + 1
end)

-- Connect to WebSocket server
if lge.ws_connect(WS_URL) then
    status = "Connecting to " .. WS_URL
    print(status)
else
    status = "Failed to connect"
    print(status)
end

local lastSendTime = millis()
local sendInterval = 3000 -- Send a message every 3 seconds

-- Main game loop
while true do
    -- Process WebSocket events (IMPORTANT: must call this regularly!)
    lge.ws_loop()

    -- Update connection status
    if lge.ws_is_connected() then
        status = "Connected"

        -- Send periodic test messages
        local currentTime = millis()
        if currentTime - lastSendTime >= sendInterval then
            local testMsg = string.format('{"type":"test","count":%d,"time":%d}', messageCount, currentTime)
            if lge.ws_send(testMsg) then
                print("Sent: " .. testMsg)
            end
            lastSendTime = currentTime
        end
    else
        status = "Disconnected"
    end

    -- Draw UI
    lge.clear_canvas("#000000")

    -- Title
    lge.draw_text(10, 10, "WebSocket Example", "#00FF00")

    -- Status
    local statusColor = lge.ws_is_connected() and "#00FF00" or "#FF0000"
    lge.draw_text(10, 35, "Status: " .. status, statusColor)

    -- Message counter
    lge.draw_text(10, 60, "Messages: " .. messageCount, "#FFFFFF")

    -- Last message
    if lastMessage ~= "" then
        lge.draw_text(10, 85, "Last: " .. lastMessage:sub(1, 30), "#FFFF00")
    end

    -- Instructions
    lge.draw_text(10, height - 40, "Touch screen to", "#AAAAAA")
    lge.draw_text(10, height - 20, "disconnect/reconnect", "#AAAAAA")

    -- Handle touch input
    local btn, x, y = lge.get_mouse_click()
    if btn ~= nil then
        if lge.ws_is_connected() then
            print("Disconnecting...")
            lge.ws_disconnect()
        else
            print("Reconnecting...")
            lge.ws_connect(WS_URL)
        end
    end

    -- Update display
    lge.present()

    -- Control frame rate
    lge.delay(16) -- ~60 FPS
end
