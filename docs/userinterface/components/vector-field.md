# VectorField

## Source

- `srcs/plugins/user_interface/components/VectorField.hpp`

## Purpose

Composite vector editor built from 3 `TextField` instances (`x`, `y`, `z`).

## Key API

- `setLabel(text)`
- `setValue(Vector3f)`
- `onValidate: std::function<bool(Axis, float)>`

## Behavior

- Per-axis validation callbacks update target object fields.
- Uses numeric typing checks (`Utils::isFloat`) before accepting user changes.
