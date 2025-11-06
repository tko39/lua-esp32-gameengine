-- touchExample.lua: Demo for touch click and growing ball
-- Requires lge.get_mouse_click()
local balls = {}
local growth_rate = 4
local max_radius = 60

function loop()
    local button, x, y = lge.get_mouse_click()
    if x then
        table.insert(balls, {
            x = x,
            y = y,
            r = 1
        })
    end

    lge.clear_canvas()
    local i = 1
    while i <= #balls do
        local ball = balls[i]
        ball.r = ball.r + growth_rate
        if ball.r >= max_radius then
            table.remove(balls, i)
        else
            lge.draw_circle(ball.x, ball.y, ball.r, '#00aaff')
            i = i + 1
        end
    end
    lge.present()
end

while true do
    loop()
    lge.delay(33)
end
