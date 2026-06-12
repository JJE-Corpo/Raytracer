# SegmentedControl

## Source

- `srcs/plugins/user_interface/components/SegmentedControl.hpp`

## Purpose

Two-option switch control (left/right segment) used for compact mode selection.

## Key API

- `setOptions(left, right)`
- `setSelectedIndex(index)`
- `onChange: std::function<void(int)>`

## Behavior

- Click left segment => index `0`.
- Click right segment => index `1`.
- Background and segment fill encode active option.
