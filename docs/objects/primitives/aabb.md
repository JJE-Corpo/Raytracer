# AABB Primitive

## Description

An Axis-Aligned Bounding Box (AABB) is a box aligned with coordinate axes, defined by minimum and maximum corners. It is commonly used as a primitive or acceleration volume.

## Mathematical Definition

Box bounds:

```text
Bmin = (xmin, ymin, zmin)
Bmax = (xmax, ymax, zmax)
```

A point `P = (x, y, z)` is inside when:

```text
xmin <= x <= xmax
ymin <= y <= ymax
zmin <= z <= zmax
```

## Ray-Intersection Calculation

Ray:

```text
P(t) = O + tD
```

For each axis `i` in `{x, y, z}`:

```text
t1_i = (Bmin_i - O_i) / D_i
t2_i = (Bmax_i - O_i) / D_i
```

Sort interval per axis:

```text
tNear_i = min(t1_i, t2_i)
tFar_i  = max(t1_i, t2_i)
```

Combine intervals:

```text
tEnter = max(tNear_x, tNear_y, tNear_z)
tExit  = min(tFar_x,  tFar_y,  tFar_z)
```

Hit exists when:

```text
tEnter <= tExit
tExit >= t_min
tEnter <= t_max
```

Intersection `t` is typically:

- `tEnter` if outside box,
- `tExit` if origin starts inside.

## Normal Computation

At hit point `H`, determine which face is reached using largest contributing axis for `tEnter` or by comparing `H` to bounds with epsilon:

```text
if abs(H.x - xmin) < eps -> N = (-1, 0, 0)
if abs(H.x - xmax) < eps -> N = ( 1, 0, 0)
if abs(H.y - ymin) < eps -> N = (0, -1, 0)
if abs(H.y - ymax) < eps -> N = (0,  1, 0)
if abs(H.z - zmin) < eps -> N = (0, 0, -1)
if abs(H.z - zmax) < eps -> N = (0, 0,  1)
```

## Edge Cases

- Any `D_i = 0`: ray parallel to slab; reject if origin component outside slab range.
- Origin inside box: `tEnter` can be negative.
- Numerical ties at edges/corners: may hit multiple faces simultaneously.
- `Bmin_i > Bmax_i` means invalid box definition.

## Pseudocode

```pseudocode
function intersectAABB(ray, bMin, bMax, tMin, tMax):
    tEnter = tMin
    tExit = tMax

    for axis in [x, y, z]:
        Oi = ray.origin[axis]
        Di = ray.direction[axis]
        minI = bMin[axis]
        maxI = bMax[axis]

        if abs(Di) < epsilon:
            if Oi < minI or Oi > maxI:
                return no_hit
            continue

        t1 = (minI - Oi) / Di
        t2 = (maxI - Oi) / Di
        tNear = min(t1, t2)
        tFar = max(t1, t2)

        tEnter = max(tEnter, tNear)
        tExit = min(tExit, tFar)

        if tEnter > tExit:
            return no_hit

    tHit = tEnter
    if tHit < tMin:
        tHit = tExit
    if tHit < tMin or tHit > tMax:
        return no_hit

    H = ray.origin + tHit * ray.direction
    N = computeAABBFaceNormal(H, bMin, bMax)
    return hit_record(tHit, H, N)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `min` | vec3 | Minimum corner `Bmin` |
| `max` | vec3 | Maximum corner `Bmax` |
| `material` | material ref | Material used for shading (if renderable) |
| `is_accel_only` | bool | If true, used for culling only, not direct shading |
