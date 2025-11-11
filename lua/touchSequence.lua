-- touchSequence.lua: Circle Memory Challenge Game
-- Player must touch circles in the order they appear. Each round adds a new circle.
local circles = {}
local sequence = {}
local current_step = 1
local round = 1
local showing_sequence = true
local show_timer = 0
local show_delay = 30 -- frames per highlight
local canvas_w, canvas_h = lge.get_canvas_size()
local score = 0
local game_over = false
local colors = {'#ff0000', '#00ff00', '#4444ff', '#ffff00', '#ff00ff', '#00ffff'}

function circles_overlap(x1, y1, r1, x2, y2, r2)
    local dx = x1 - x2
    local dy = y1 - y2
    local dist = math.sqrt(dx * dx + dy * dy)
    return dist < (r1 + r2 + 8) -- 8px buffer
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
            -- Check if any circle was touched
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
            -- If no circle was touched, do nothing (no penalty)
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
