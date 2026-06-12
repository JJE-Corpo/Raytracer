# Camera Model

## Description

The raytracer uses a perspective camera model. Rays originate from camera position and pass through image-plane sample points, producing perspective projection.

## Perspective Ray Generation

Given:

- Camera position `C`.
- Look-at target `T`.
- Up vector `Up`.
- Vertical field of view `fovY`.
- Resolution `(W, H)`.

Build camera basis:

```text
F = normalize(T - C)         # forward
R = normalize(F x Up)        # right
U = R x F                    # corrected up
```

Viewport dimensions at unit focal distance:

```text
aspect = W / H
viewportHeight = 2 * tan(fovY / 2)
viewportWidth  = aspect * viewportHeight
```

For pixel `(i, j)` (zero-based), normalized sample center:

```text
u = (i + 0.5) / W
v = (j + 0.5) / H
```

Map to camera plane coordinates in `[-0.5, 0.5]` then scale:

```text
xCam = (2u - 1) * viewportWidth / 2
yCam = (1 - 2v) * viewportHeight / 2
```

Ray direction:

```text
D = normalize(F + xCam * R + yCam * U)
```

Final ray:

```text
origin = C
direction = D
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `position` | vec3 | Camera world position `C` |
| `look_at` | vec3 | Target point `T` |
| `up` | vec3 | Up reference vector |
| `fov` | float | Vertical field of view |
| `width` | int | Output width in pixels |
| `height` | int | Output height in pixels |
| `near` | float (optional) | Near clipping distance |
| `far` | float (optional) | Far clipping distance |

## Notes

- `Up` must not be colinear with `F`.
- Anti-aliasing can jitter `(u, v)` inside each pixel footprint.
- Depth of field extensions replace single origin with lens sampling.
