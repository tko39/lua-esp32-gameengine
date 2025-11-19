-- drawingApp.lua
-- Simple drawing app using the lge.* API described in api2d.md
-- White canvas, draw with mouse position. Click "Pen" to toggle pen, "Clear" to clear canvas.
local BG_COLOR = "#ffffff"
local HUD_COLOR = "#000000"

local canvas_w, canvas_h = 240, 135

-- drawing state
local current_color = "#000000"
local current_size = 3
local pen_down = false

-- UI buttons
local buttons = {}

local function now_ms()
    return millis()
end

local function add_point(x, y)
    if not x then
        return
    end

    lge.draw_circle(x, y, current_size, current_color)
end

local function point_in_rect(px, py, rx, ry, rw, rh)
    return px >= rx and px <= rx + rw and py >= ry and py <= ry + rh
end

local function create_buttons()
    buttons = {}
    local pad = 6
    local btnW, btnH = 56, 18
    local x = pad
    local y = pad
    -- Pen toggle
    table.insert(buttons, {
        label = "Pen",
        x = x,
        y = y,
        w = btnW,
        h = btnH,
        action = function()
            pen_down = not pen_down
        end
    })
    x = x + btnW + pad
    -- Clear
    table.insert(buttons, {
        label = "Clear",
        x = x,
        y = y,
        w = btnW,
        h = btnH,
        action = function()
            lge.clear_canvas(HUD_COLOR)
            lge.clear_canvas(BG_COLOR)
        end
    })
    x = x + btnW + pad
    -- Color buttons
    local colors = {"#000000", "#ff0000", "#00aa00", "#0000ff", "#ff9900"}
    for i, c in ipairs(colors) do
        table.insert(buttons, {
            label = "",
            x = x,
            y = y,
            w = btnH,
            h = btnH,
            color = c,
            action = function()
                current_color = c
            end
        })
        x = x + btnH + 4
    end
    -- Brush size buttons
    x = pad
    y = y + btnH + 6
    local sizes = {2, 4, 8}
    for i, s in ipairs(sizes) do
        table.insert(buttons, {
            label = tostring(s),
            x = x,
            y = y,
            w = 36,
            h = 18,
            action = function()
                current_size = s
            end
        })
        x = x + 36 + 6
    end
end

local function handle_click(mx, my)
    if not mx then
        return
    end
    for _, b in ipairs(buttons) do
        if point_in_rect(mx, my, b.x, b.y, b.w, b.h) then
            if b.action then
                b.action()
            end
            return true
        end
    end
    return false
end

function setup()
    canvas_w, canvas_h = lge.get_canvas_size()
    create_buttons()

    -- draw everything
    lge.clear_canvas(BG_COLOR)
end

function update()
    -- sample mouse position; API: lge.get_mouse_position()
    local btn, mx, my = lge.get_mouse_position()
    -- check clicks via get_mouse_click (to toggle buttons / pen)
    local cbtn, cx, cy = lge.get_mouse_click()
    if cx then
        -- if clicked a UI button, handle it and don't draw
        if handle_click(cx, cy) then
            -- click handled
        else
            -- clicking outside UI toggles pen state
            pen_down = true
            -- also add a point at click
            add_point(cx, cy)
        end
    end

    if mx and my and pen_down then
        add_point(mx, my)
    end

    -- draw strokes as small circles
    -- for _, p in ipairs(strokes) do
    --     lge.draw_circle(p.x, p.y, p.size, p.color)
    -- end

    -- draw UI
    for _, b in ipairs(buttons) do
        local col = "#dddddd"
        lge.draw_rectangle(b.x - 1, b.y - 1, b.w + 2, b.h + 2, "#cccccc")
        if b.color then
            -- color swatch
            lge.draw_rectangle(b.x, b.y, b.w, b.h, b.color)
            lge.draw_rectangle(b.x + 1, b.y + 1, b.w - 2, b.h - 2, b.color)
        else
            lge.draw_rectangle(b.x, b.y, b.w, b.h, "#ffffff")
            lge.draw_text(b.x + 6, b.y + 4, b.label, HUD_COLOR)
        end
    end

    -- status
    lge.draw_text(6, canvas_h - 18, string.format("Pen: %s  Color: %s  Size: %d", pen_down and "ON" or "OFF",
        current_color, current_size), HUD_COLOR)

    lge.present()
    lge.delay(16)
end

setup()
while true do
    update()
end
