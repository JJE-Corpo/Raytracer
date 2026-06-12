# Cone Primitive

## Description

A right circular cone is defined by an apex `C`, an axis `A`, and an opening angle `theta`. It can be infinite or clamped to finite height.

## Mathematical Definition

Let `A` be unit length and `k = tan(theta)`. A point `P` lies on the cone surface if:

```text
|| (P - C) - ((P - C).A)A ||^2 = (k^2) * ((P - C).A)^2
```

Equivalent implicit form:

```text
f(P) = ||v_perp||^2 - k^2 * v_parallel^2 = 0
```

where `v = P - C`, `v_parallel = v.A`, `v_perp = v - v_parallel A`.

## Ray-Intersection Calculation

Ray:

```text
P(t) = O + tD
```

Define:

```text
oc = O - C
dv = D.A
ov = oc.A

Dperp = D - dv A
ocPerp = oc - ov A
```

Substitute into implicit equation:

```text
||ocPerp + tDperp||^2 - k^2(ov + tdv)^2 = 0
```

Expand into quadratic:

```text
a = Dperp.Dperp - k^2 * dv^2
b = 2(ocPerp.Dperp - k^2 * ov * dv)
c = ocPerp.ocPerp - k^2 * ov^2
```

Solve:

```text
Delta = b^2 - 4ac
t = (-b +- sqrt(Delta)) / (2a)
```

Finite cone constraint (if height `h` from apex along axis):

```text
0 <= (H - C).A <= h
```

with `H = O + tD`.

## Normal Computation

Using gradient of implicit function `f(P)`, one common normal form is:

```text
v = H - C
v_parallel = (v.A)A
v_perp = v - v_parallel
N_unnormalized = v_perp - k^2 (v.A) A
N = normalize(N_unnormalized)
```

If cone orientation appears inverted, negate `N` and enforce front-face convention.

## Edge Cases

- `a` near zero: equation becomes linear or numerically unstable.
- `Delta < 0`: no real hit.
- Rays near apex can be precision-sensitive.
- Finite cone clipping removes valid roots from infinite solution.
- If capped cone, cap intersection must be tested separately.

## Pseudocode

```pseudocode
function intersectCone(ray, apex, axis, angle, height, tMin, tMax):
    A = normalize(axis)
    k = tan(angle)
    oc = ray.origin - apex

    dv = dot(ray.direction, A)
    ov = dot(oc, A)
    Dperp = ray.direction - dv * A
    ocPerp = oc - ov * A

    a = dot(Dperp, Dperp) - (k*k) * (dv*dv)
    b = 2.0 * (dot(ocPerp, Dperp) - (k*k) * ov * dv)
    c = dot(ocPerp, ocPerp) - (k*k) * (ov*ov)

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
        axial = dot(H - apex, A)
        if axial < 0 or axial > height:
            continue

        v = H - apex
        vParallel = dot(v, A) * A
        vPerp = v - vParallel
        N = normalize(vPerp - (k*k) * dot(v, A) * A)
        return hit_record(t, H, N)

    return no_hit
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `apex` | vec3 | Cone apex point `C` |
| `axis` | vec3 | Cone axis direction `A` |
| `angle` | float | Opening half-angle `theta` |
| `height` | float | Finite cone height `h` |
| `capped` | bool | Whether base cap is enabled |
| `material` | material ref | Material used for shading |
