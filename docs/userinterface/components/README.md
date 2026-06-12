# UI Components

## Overview

Reusable widgets are defined in `srcs/plugins/user_interface/components/`.

## Component Pages

- [Button](./button.md)
- [Checkbox](./checkbox.md)
- [ColorPicker](./color-picker.md)
- [Dropdown](./dropdown.md)
- [Slider](./slider.md)
- [SegmentedControl](./segmented-control.md)
- [TextField](./text-field.md)
- [VectorField](./vector-field.md)
- [Separator](./separator.md)
- [Item](./item.md)
- [Menu System](./menu-system.md)

## Shared Pattern

Most widgets:

1. store geometry in SFML shapes/text,
2. compute `hovered` in `update(...)`,
3. mutate state in `handleEvent(...)`,
4. expose callback hooks (`onClick`, `onChange`, etc.),
5. return cursor hints through `getCursor()`.
