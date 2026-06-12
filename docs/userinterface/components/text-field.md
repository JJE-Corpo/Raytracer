# TextField

## Source

- `srcs/plugins/user_interface/components/TextField.hpp`

## Purpose

Single-line editable text input with focus, cursor navigation, and validation hooks.

## Key API

- `setValue(string)`
- `onType: std::function<bool(const std::string&)>`
- `onValidate: std::function<bool(const std::string&)>`

## Behavior

- Supports caret blinking and click-based cursor reposition.
- Handles insertion/backspace.
- `Return` triggers validation callback.
- Left/Right arrow moves cursor position.
