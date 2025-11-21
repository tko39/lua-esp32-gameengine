# WebSocket API Documentation

The LuaArduinoEngine provides WebSocket functionality through the `lge` module, allowing Lua scripts to connect to WebSocket servers (both `ws://` and `wss://`), send messages, and receive messages via callbacks.

## API Functions

### `lge.ws_connect(url)`

Connects to a WebSocket server.

**Parameters:**

- `url` (string): WebSocket URL in format `ws://host:port/path` or `wss://host:port/path`
  - Example: `"ws://echo.websocket.org"`
  - Example: `"wss://myserver.com:443/socket"`
  - If port is omitted, defaults to 80 for `ws://` and 443 for `wss://`

**Returns:**

- `boolean`: `true` if connection initiated successfully, `false` on error

**Example:**

```lua
if lge.ws_connect("ws://echo.websocket.org") then
    print("Connecting...")
else
    print("Failed to initiate connection")
end
```

---

### `lge.ws_send(message)`

Sends a text message to the connected WebSocket server.

**Parameters:**

- `message` (string): The message to send. Can be plain text or JSON string.

**Returns:**

- `boolean`: `true` if message was sent successfully, `false` if not connected or send failed

**Example:**

```lua
-- Send plain text
lge.ws_send("Hello, WebSocket!")

-- Send JSON message
local jsonMsg = '{"type":"chat","user":"player1","message":"Hello!"}'
if lge.ws_send(jsonMsg) then
    print("Message sent")
end
```

---

### `lge.ws_on_message(callback)`

Registers a Lua function to be called when messages are received from the WebSocket server.

**Parameters:**

- `callback` (function): Function to call when a message arrives. The callback receives one parameter:
  - If the message is valid JSON, a Lua table with the parsed data
  - If JSON parsing fails, a string with the raw message

**Returns:**

- Nothing

**Example:**

```lua
lge.ws_on_message(function(message)
    if type(message) == "table" then
        -- JSON message parsed into table
        print("Message type: " .. (message.type or "unknown"))
        print("Data: " .. tostring(message.data))
    else
        -- Raw string message
        print("Raw message: " .. message)
    end
end)
```

---

### `lge.ws_is_connected()`

Checks if the WebSocket connection is currently active.

**Parameters:**

- None

**Returns:**

- `boolean`: `true` if connected, `false` otherwise

**Example:**

```lua
if lge.ws_is_connected() then
    lge.ws_send("ping")
else
    print("Not connected!")
end
```

---

### `lge.ws_disconnect()`

Disconnects from the WebSocket server.

**Parameters:**

- None

**Returns:**

- Nothing

**Example:**

```lua
lge.ws_disconnect()
print("Disconnected from server")
```

---

### `lge.ws_loop()`

Processes WebSocket events. **This function must be called regularly** (typically in your main game loop) to handle incoming messages and maintain the connection.

**Parameters:**

- None

**Returns:**

- Nothing

**Example:**

```lua
while true do
    lge.ws_loop()  -- Process WebSocket events

    -- Your game logic here

    lge.present()
    lge.delay(16)
end
```

---

## Complete Example

```lua
-- Connect to WebSocket server
lge.ws_connect("ws://echo.websocket.org")

-- Register message handler
lge.ws_on_message(function(msg)
    if type(msg) == "table" then
        print("Received JSON:", msg.type, msg.data)
    else
        print("Received:", msg)
    end
end)

local lastSend = millis()

-- Main loop
while true do
    -- IMPORTANT: Process WebSocket events
    lge.ws_loop()

    -- Send periodic messages
    if lge.ws_is_connected() and (millis() - lastSend) > 5000 then
        lge.ws_send('{"type":"heartbeat","time":' .. millis() .. '}')
        lastSend = millis()
    end

    -- Your rendering code
    lge.clear_canvas("#000000")

    local status = lge.ws_is_connected() and "Connected" or "Disconnected"
    lge.draw_text(10, 10, "Status: " .. status, "#FFFFFF")

    lge.present()
    lge.delay(16)
end
```

## JSON Message Format

When the WebSocket server sends JSON messages, they are automatically parsed into Lua tables. For example:

**Server sends:**

```json
{
  "type": "player_update",
  "id": 123,
  "x": 100,
  "y": 200,
  "name": "Player1"
}
```

**Lua callback receives:**

```lua
{
    type = "player_update",
    id = 123,
    x = 100,
    y = 200,
    name = "Player1"
}
```

## Important Notes

1. **Call `lge.ws_loop()` regularly**: This function must be called in your main loop to process incoming messages and maintain the connection.

2. **Connection is asynchronous**: When you call `lge.ws_connect()`, the connection happens in the background. Use `lge.ws_is_connected()` to check when the connection is established.

3. **Auto-reconnect**: The WebSocket client will automatically attempt to reconnect every 5 seconds if the connection is lost.

4. **Memory**: Only one WebSocket connection is supported at a time. Calling `lge.ws_connect()` again will disconnect the previous connection.

5. **SSL/TLS**: Secure WebSocket connections (`wss://`) are supported but may require additional memory and CPU resources on the ESP32.

## Troubleshooting

- **Messages not received**: Make sure you're calling `lge.ws_loop()` in your main loop
- **Connection fails**: Check Serial monitor for error messages and verify the URL format
- **JSON parsing fails**: If the server sends non-JSON text, the callback receives it as a string
- **Memory issues**: WebSocket connections use heap memory. Monitor with `get_system_info()` if experiencing crashes
