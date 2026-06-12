# Sphere Primitive

## Description

A sphere is the set of points at constant distance `r` from a center `C`. It is rotationally symmetric and one of the most common test primitives in ray tracing.

## Mathematical Definition

Implicit surface equation:

```text
||P - C||^2 - r^2 = 0
```

Expanded in coordinates:

```text
(x - Cx)^2 + (y - Cy)^2 + (z - Cz)^2 - r^2 = 0
```

## Ray-Intersection Calculation

Ray equation:

```text
P(t) = O + tD
```

Substitute into sphere equation:

```text
||O + tD - C||^2 - r^2 = 0
```

Define `oc = O - C`:

```text
||oc + tD||^2 - r^2 = 0
```

Dot expansion:

```text
(oc + tD) . (oc + tD) - r^2 = 0
oc.oc + 2t(oc.D) + t^2(D.D) - r^2 = 0
```

Quadratic coefficients:

```text
a = D.D
b = 2(oc.D)
c = oc.oc - r^2
```

Solve:

```text
t = (-b +- sqrt(b^2 - 4ac)) / (2a)
```

Discriminant:

```text
Delta = b^2 - 4ac
```

- `Delta < 0`: no real intersection.
- `Delta = 0`: tangent hit.
- `Delta > 0`: two hits, choose smallest valid `t` in `[t_min, t_max]`.

## Normal Computation

Hit point:

```text
H = O + t_hit D
```

Outward normal:

```text
N = normalize(H - C)
```

If renderer needs front-face consistency, flip normal if `D.N > 0`.

## Edge Cases

- Ray starts inside sphere: near root can be negative; far root may be valid.
- Very small radius: susceptible to floating-point precision issues.
- Non-normalized direction: still correct if using full `a`, `b`, `c` formula.
- Tangent intersection (`Delta` near zero): compare against epsilon.

## Pseudocode

```pseudocode
function intersectSphere(ray, center, radius, tMin, tMax):
    oc = ray.origin - center
    a = dot(ray.direction, ray.direction)
    b = 2.0 * dot(oc, ray.direction)
    c = dot(oc, oc) - radius * radius

    delta = b*b - 4*a*c
    if delta < 0:
        return no_hit

    sqrtDelta = sqrt(delta)
    t0 = (-b - sqrtDelta) / (2*a)
    t1 = (-b + sqrtDelta) / (2*a)

    tHit = invalid
    if tMin < t0 and t0 < tMax:
        tHit = t0
    else if tMin < t1 and t1 < tMax:
        tHit = t1
    else:
        return no_hit

    hitPoint = ray.origin + tHit * ray.direction
    normal = normalize(hitPoint - center)
    return hit_record(tHit, hitPoint, normal)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `center` | vec3 | Sphere center `C` |
| `radius` | float | Sphere radius `r` (`r > 0`) |
| `material` | material ref | Material used for shading |
| `transform` | matrix (optional) | Optional object transform |
