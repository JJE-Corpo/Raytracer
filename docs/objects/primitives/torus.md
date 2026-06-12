# Torus Primitive

## Description

A torus is a donut-shaped surface defined by:

- Major radius `R` (distance from torus center to tube centerline)
- Minor radius `r` (tube radius)

In this project, torus supports center and rotation.

## Mathematical Definition

Canonical implicit torus (aligned on `z` axis):

```text
(x^2 + y^2 + z^2 + R^2 - r^2)^2 - 4R^2(x^2 + y^2) = 0
```

Equivalent form used in implementation in rotated local space (using `z`-axis term variant):

```text
(||P||^2 + R^2 - r^2)^2 - 4R^2(r^2 - z^2) = 0
```

## Ray-Intersection Calculation

Ray in local torus frame:

```text
P(t) = P0 + tU
```

Define:

```text
K = P0.P0 - R^2 - r^2
m = P0.U
n = U.U
```

After substitution and expansion, the intersection becomes a quartic:

```text
c4 t^4 + c3 t^3 + c2 t^2 + c1 t + c0 = 0
```

with coefficients:

```text
c4 = n^2
c3 = 4 n m
c2 = 2 n K + 4 m^2 + 4 R^2 Uz^2
c1 = 4 K m + 8 R^2 P0z Uz
c0 = K^2 + 4 R^2 P0z^2 - 4 R^2 r^2
```

Solve quartic, keep smallest real root satisfying:

```text
t_min < t < t_max
```

## Normal Computation

Normal is gradient of implicit function `F` in local space:

```text
param = ||Plocal||^2 - R^2 - r^2
Nlocal = (
    4 x param,
    4 y param,
    4 z (param + 2R^2)
)
```

Then rotate back to world space and normalize.

## Edge Cases

- Quartic can produce 0, 2, or 4 real roots.
- Numerical instability near tangency.
- Very small `r` tends toward thin ring, sensitive to precision.
- Root ordering/selection must respect `t_min`, `t_max`.

## Pseudocode

```pseudocode
function intersectTorus(ray, center, rotation, R, r, tMin, tMax):
    P0 = rotate(ray.origin - center, rotation)
    U  = rotate(ray.direction, rotation)

    build quartic coefficients c0..c4 from P0, U, R, r
    roots = solveQuartic(c0..c4)

    tHit = +infinity
    for each real root t in roots:
        if tMin < t < tHit and t < tMax:
            tHit = t

    if tHit is infinity:
        return no_hit

    localH = P0 + tHit * U
    Nlocal = gradientTorus(localH, R, r)
    Nworld = normalize(rotateBack(Nlocal, rotation))

    H = ray.origin + tHit * ray.direction
    return hit_record(tHit, H, Nworld)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `position` | vec3 | Torus center |
| `rotation` | vec3 | Torus orientation |
| `radius` | float | Major radius `R` |
| `height` | float | Minor radius `r` (tube radius) |
| `material` | material ref | Surface material |
