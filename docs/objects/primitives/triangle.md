# Triangle Primitive

## Description

A triangle is a finite planar primitive defined by three vertices `V0`, `V1`, `V2`. It is widely used for meshes and OBJ geometry.

## Mathematical Definition

Parametric surface using barycentric coordinates:

```text
P(u, v) = V0 + u(V1 - V0) + v(V2 - V0)
```

with constraints:

```text
u >= 0, v >= 0, u + v <= 1
```

## Ray-Intersection Calculation

Ray:

```text
P(t) = O + tD
```

Set equality:

```text
O + tD = V0 + uE1 + vE2
```

where:

```text
E1 = V1 - V0
E2 = V2 - V0
```

Rearrange:

```text
O - V0 = -tD + uE1 + vE2
```

This 3x3 system is efficiently solved by Moller-Trumbore.

Moller-Trumbore steps:

```text
Pvec = D x E2
det = E1 . Pvec
```

- If `|det| < epsilon`: ray parallel to triangle plane (or degenerate).

Then:

```text
invDet = 1 / det
Tvec = O - V0
u = (Tvec . Pvec) * invDet
```

- Reject if `u < 0` or `u > 1`.

Next:

```text
Qvec = Tvec x E1
v = (D . Qvec) * invDet
```

- Reject if `v < 0` or `u + v > 1`.

Finally:

```text
t = (E2 . Qvec) * invDet
```

Accept if `t_min < t < t_max`.

## Normal Computation

Flat triangle normal:

```text
N = normalize(E1 x E2)
```

For smooth shading with vertex normals `N0`, `N1`, `N2`:

```text
w = 1 - u - v
N = normalize(wN0 + uN1 + vN2)
```

## Edge Cases

- Degenerate triangle: `E1 x E2` near zero area.
- Parallel ray: `|det| < epsilon`.
- Edge/vertex hits: barycentric values near 0 or 1 (epsilon-safe comparisons).
- Back-face culling optional: reject if `det < epsilon`.

## Pseudocode

```pseudocode
function intersectTriangle(ray, V0, V1, V2, tMin, tMax, cullBackFace):
    E1 = V1 - V0
    E2 = V2 - V0

    Pvec = cross(ray.direction, E2)
    det = dot(E1, Pvec)

    if cullBackFace:
        if det < epsilon:
            return no_hit
    else:
        if abs(det) < epsilon:
            return no_hit

    invDet = 1.0 / det
    Tvec = ray.origin - V0
    u = dot(Tvec, Pvec) * invDet
    if u < 0 or u > 1:
        return no_hit

    Qvec = cross(Tvec, E1)
    v = dot(ray.direction, Qvec) * invDet
    if v < 0 or (u + v) > 1:
        return no_hit

    t = dot(E2, Qvec) * invDet
    if t <= tMin or t >= tMax:
        return no_hit

    hitPoint = ray.origin + t * ray.direction
    normal = normalize(cross(E1, E2))
    return hit_record(t, hitPoint, normal, u, v)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `v0` | vec3 | First vertex `V0` |
| `v1` | vec3 | Second vertex `V1` |
| `v2` | vec3 | Third vertex `V2` |
| `n0,n1,n2` | vec3 (optional) | Vertex normals for smooth shading |
| `cull_backface` | bool | Enable/disable back-face culling |
| `material` | material ref | Material used for shading |
