# MaterialPanel

## Source

- `srcs/plugins/user_interface/panels/MaterialPanel.hpp`

## Role

Material-focused inspector for selected primitives.

## Features

- Model selector (`Phong` vs `PBR`) through `SegmentedControl`.
- Base color editing via `ColorPicker`.
- Dynamic float sliders filtered by material model through `ViewportHelper::isMaterialFloatSlider(...)`.

## Key Method

- `rebuild(const ISceneObject *currentObject)`

Builds panel controls from current primitive material state.
