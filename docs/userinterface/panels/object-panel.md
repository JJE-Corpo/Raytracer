# ObjectPanel

## Source

- `srcs/plugins/user_interface/panels/ObjectPanel.hpp`

## Role

Inspector/editor for selected scene object properties.

## Handles

- Common transforms:
  - position
  - rotation
  - scale
- Primitive-specific:
  - material dropdown
  - float properties converted to sliders
- Light-specific:
  - color picker
  - intensity text field

## Key Method

- `rebuild(const ISceneObject *currentObject)`

This method reconfigures the panel dynamically based on object runtime type.
