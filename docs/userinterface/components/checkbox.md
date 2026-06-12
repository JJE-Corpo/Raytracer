# Checkbox

## Source

- `srcs/plugins/user_interface/components/Checkbox.hpp`

## Purpose

Boolean toggle component with text label.

## Key API

- `setLabel(text)`
- `setChecked(bool)`
- `onToggle: std::function<void(bool)>`

## Behavior

- Click inside combined label/box bounds toggles state.
- Visual color changes according to checked and hovered state.
