# Dropdown

## Source

- `srcs/plugins/user_interface/components/Dropdown.hpp`

## Purpose

Header + expandable panel allowing one selection among text options.

## Key API

- `setOptions(labels)`
- `setSelectedIndex(index)` / `getSelectedIndex()`
- `setPlaceholder(text)`
- `onSelect: std::function<void(int)>`

## Behavior

- Header click toggles open/close.
- Option click selects item, triggers callback, closes panel.
- Outside click closes panel.
