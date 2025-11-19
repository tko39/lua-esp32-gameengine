API:

## Canvas Functions

- `lge.get_canvas_size()`: Returns the width and height of the canvas as two numbers.

  ```lua
  local w, h = lge.get_canvas_size()
  ```

- `lge.clear_canvas(color default "#000000")`: Clears the canvas before drawing the next frame.
  ```lua
  lge.clear_canvas()
  ```

## Drawing Functions

- `lge.draw_circle(x, y, radius, color)`: Draws a filled circle at the given position with the given color.
  ```lua
  lge.draw_circle(100, 100, 20, "#ff0000")
  ```
- `lge.draw_rectangle(x, y, width, height, color)`: Draws a filled rectangle at the specified position with the given color.

  ```lua
  lge.draw_rectangle(100, 100, 20, 40, "#ff0000")
  ```

- `lge.draw_triangle(x0, y0, x1, y1, x2, y2, color)`: Draws a filled circle at the specified position with the given color.

  ```lua
  lge.draw_triangle(100, 100, 100, 200, 200, 200, "#ff0000")
  ```

- `lge.draw_text(x, y, text, color)`: Draws a string of text at the given position with the specified color.
  ```lua
  lge.draw_text(10, 20, "Score: 42", "#ffffff")
  ```

## Input Functions

- `lge.get_mouse_click()`: Returns `button, x, y` if a mouse click occurred, or `nil` otherwise. The click resets after being read.

  ```lua
  local button, x, y = lge.get_mouse_click()
  if x then
    spawn_bomb(x, y)
  end
  ```

- `lge.get_mouse_position()`: Returns the current mouse x and y position, or `nil` if the mouse position is unavailable.
  ```lua
  local button, x, y = lge.get_mouse_position()
  if x then
    lge.draw_circle(x, y, 10, "#00ffff")
  end
  ```

## Timing and Coroutines

- `lge.delay(ms)`: Delays execution for the given number of milliseconds, yielding the coroutine.

  ```lua
  lge.delay(1000)
  ```

- `lge.present()`: Transfers data to the screen. Usually called before `lge.delay(...)` in the main loop
  ```lua
  lge.present()
  ```
