# Plane Primitive

## Description

A plane is an infinite flat surface defined by one point `P0` on the plane and a normal vector `N`.

## Mathematical Definition

Implicit equation:

```text
(P - P0) . N = 0
```

with `N` normalized for stable distance interpretation.

## Ray-Intersection Calculation

Ray:

```text
P(t) = O + tD
```

Substitute into plane equation:

```text
(O + tD - P0) . N = 0
```

Expand:

```text
(O - P0) . N + t(D . N) = 0
```

Solve for `t`:

```text
t = ((P0 - O) . N) / (D . N)
```

Validity conditions:

- Denominator `D.N` must not be zero (or near zero with epsilon).
- Hit must satisfy `t_min < t < t_max`.

## Normal Computation

The geometric normal is constant everywhere:

```text
N_plane = normalize(N)
```

Optional front-face orientation:

```text
if dot(D, N_plane) > 0 then N_shading = -N_plane else N_shading = N_plane
```

## Edge Cases

- Ray parallel to plane: `|D.N| < epsilon`.
- Ray lies in the plane: numerator and denominator both near zero; treated as no single finite hit.
- Infinite extent: additional clipping may be required for finite plane patches.

## Pseudocode

```pseudocode
function intersectPlane(ray, pointOnPlane, normal, tMin, tMax):
    n = normalize(normal)
    denom = dot(ray.direction, n)

    if abs(denom) < epsilon:
        return no_hit

    t = dot(pointOnPlane - ray.origin, n) / denom
    if t <= tMin or t >= tMax:
        return no_hit

    hitPoint = ray.origin + t * ray.direction
    hitNormal = n
    if dot(ray.direction, hitNormal) > 0:
        hitNormal = -hitNormal

    return hit_record(t, hitPoint, hitNormal)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `point` | vec3 | A point `P0` on the plane |
| `normal` | vec3 | Plane normal `N` |
| `material` | material ref | Material used for shading |
| `u_axis` | vec3 (optional) | Local tangent axis for UV mapping |
| `v_axis` | vec3 (optional) | Local bitangent axis for UV mapping |
