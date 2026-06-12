# ColorPicker

## Source

- `srcs/plugins/user_interface/components/ColorPicker.hpp`

## Purpose

Compact swatch + popup color editor based on 3 channel sliders (R, G, B).

## Key API

- `setColor(ColorF)`
- `getColor()`
- `openAt(x, y)` / `close()` / `isOpen()`
- `processEvent(event, mouse)`
- `onChange: std::function<void(const ColorF &)>`

## Behavior

- Clicking swatch toggles popup.
- Popup contains sliders for `0..255` channels.
- Clicking outside popup closes it.
- ESC closes popup.
