# Cylinder Primitive

## Description

A cylinder is defined by an axis, a radius, and optionally finite height. The common form in ray tracing is a right circular cylinder.

## Mathematical Definition

For cylinder axis unit vector `A`, base point `C`, and radius `r`, points on the infinite lateral surface satisfy:

```text
|| (P - C) - ((P - C).A)A ||^2 - r^2 = 0
```

This removes the component along axis `A`, keeping only radial distance to axis.

## Ray-Intersection Calculation

Ray:

```text
P(t) = O + tD
```

Let:

```text
oc = O - C
Dperp = D - (D.A)A
ocPerp = oc - (oc.A)A
```

Substitute into implicit equation:

```text
|| ocPerp + tDperp ||^2 - r^2 = 0
```

Expand:

```text
(Dperp.Dperp)t^2 + 2(ocPerp.Dperp)t + (ocPerp.ocPerp - r^2) = 0
```

Coefficients:

```text
a = Dperp.Dperp
b = 2(ocPerp.Dperp)
c = ocPerp.ocPerp - r^2
```

Solve with quadratic formula and discriminant:

```text
Delta = b^2 - 4ac
t = (-b +- sqrt(Delta)) / (2a)
```

For finite cylinder with height `h`, accept candidate `t` only if projected distance along axis is within bounds:

```text
0 <= (H - C).A <= h
```

where `H = O + tD`.

## Normal Computation

At hit point `H`, project to axis to find closest axis point:

```text
Q = C + ((H - C).A)A
N = normalize(H - Q)
```

For capped cylinders, cap normals are `+A` or `-A`.

## Edge Cases

- `a` near zero: ray parallel to cylinder axis (no lateral hit, maybe cap hit only).
- `Delta < 0`: miss.
- Finite height clipping rejects roots outside axial interval.
- Rays originating inside cylinder may still have one valid exiting root.

## Pseudocode

```pseudocode
function intersectCylinder(ray, center, axis, radius, height, tMin, tMax):
    A = normalize(axis)
    oc = ray.origin - center

    Dperp = ray.direction - dot(ray.direction, A) * A
    ocPerp = oc - dot(oc, A) * A

    a = dot(Dperp, Dperp)
    b = 2.0 * dot(ocPerp, Dperp)
    c = dot(ocPerp, ocPerp) - radius * radius

    if abs(a) < epsilon:
        return no_hit

    delta = b*b - 4*a*c
    if delta < 0:
        return no_hit

    sqrtDelta = sqrt(delta)
    candidates = [(-b - sqrtDelta)/(2*a), (-b + sqrtDelta)/(2*a)]

    for t in candidates sorted ascending:
        if t <= tMin or t >= tMax:
            continue
        H = ray.origin + t * ray.direction
        axial = dot(H - center, A)
        if axial < 0 or axial > height:
            continue

        Q = center + axial * A
        N = normalize(H - Q)
        return hit_record(t, H, N)

    return no_hit
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `center` | vec3 | Cylinder base-center or axis anchor point `C` |
| `axis` | vec3 | Cylinder axis direction `A` |
| `radius` | float | Radius `r` |
| `height` | float | Finite height `h` (optional if infinite cylinder) |
| `capped` | bool | Whether top/bottom caps are enabled |
| `material` | material ref | Material used for shading |
