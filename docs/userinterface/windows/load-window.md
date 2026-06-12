# LoadWindow

## Source

- `srcs/plugins/user_interface/windows/LoadWindow.hpp`

## Role

Merge/import decision window between current scene and loaded scene.

## Behavior

- Presents paired checkbox groups for:
  - camera fields
  - environment light coefficients
- User chooses whether each property should come from current scene or loaded scene.
- On save/apply, selected values are written back to active scene.
