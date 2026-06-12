# Slider

## Source

- `srcs/plugins/user_interface/components/Slider.hpp`

## Purpose

Numeric continuous input with label, value display, track/fill/thumb, and drag support.

## Key API

- `setRange(min, max)`
- `setValue(v)` / `getValue()`
- `setLabel(text)`
- `onChange: std::function<void(float)>`

## Behavior

- Click on slider hitbox starts dragging.
- Drag updates value continuously.
- Value string is auto-formatted (`%.2f`).
