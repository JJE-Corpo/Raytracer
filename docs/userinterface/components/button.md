# Button

## Source

- `srcs/plugins/user_interface/components/Button.hpp`

## Purpose

Clickable action control with label and pressed/hover visual states.

## Key API

- `setLabel(text)`
- `layout(x, y, w, h)`
- `onClick: std::function<void()>`

## Behavior

- Press tracked on mouse down inside bounds.
- Callback fired on mouse release if pointer is still inside.
- Cursor:
  - `HAND` when hovered and enabled.
  - `NOT_ALLOWED` when hovered and disabled.
