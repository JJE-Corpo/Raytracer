# Fractal Primitive

## Description

This primitive renders 3D fractals using signed/distance-estimator style ray marching. Supported fractal families include Mandelbulb, Mandelbox, Julia 3D, Menger sponge, and Sierpinski variants.

## Mathematical Definition

Unlike closed-form quadrics, the surface is defined implicitly by a distance estimator `DE(p)`:

```text
surface approx: DE(p) = 0
```

`DE(p)` is computed by iterative fractal formulas (escape-time style transforms).

## Ray-Intersection Calculation

Ray equation:

```text
P(t) = O + tD
```

Intersection is found by sphere tracing/ray marching:

1. Intersect ray with primitive AABB to get `[tEnter, tExit]`.
2. Start `t = max(tEnter, t_min)`.
3. Evaluate local point:

```text
p_local = (P(t) - center) / size
d_local = DE(p_local)
d_world = d_local * size
```

4. If `d_world < epsilon`, accept hit.
5. Else advance:

```text
t = t + d_world
```

6. Stop when `t > tExit` or step limit reached.

Validity condition:

```text
t_min < t < t_max
```

## Normal Computation

Normal is estimated numerically from DE samples around hit point (tetrahedral finite-difference pattern):

```text
N approx normalize(
    d1 * DE(p + d1*h) +
    d2 * DE(p + d2*h) +
    d3 * DE(p + d3*h) +
    d4 * DE(p + d4*h)
)
```

where `d1..d4` are fixed direction offsets.

## Edge Cases

- `DE` underestimation may cause missed thin features.
- Too-large `epsilon` causes surface inflation.
- Too-small `epsilon` increases steps/noise.
- Maximum ray steps can terminate before convergence.
- AABB clipping is required for performance and robustness.

## Pseudocode

```pseudocode
function intersectFractal(ray, bbox, center, size, epsilon, maxSteps, tMin, tMax):
    [tEnter, tExit] = intersectAABB(ray, bbox, tMin, tMax)
    if no interval: return no_hit

    t = max(tEnter, tMin)
    tLimit = min(tExit, tMax)

    for step in 0..maxSteps-1:
        worldP = ray.origin + t * ray.direction
        localP = (worldP - center) / size
        dWorld = DE(localP) * size

        if dWorld < epsilon:
            N = estimateNormalFromDE(localP)
            return hit_record(t, worldP, N)

        t = t + dWorld
        if t > tLimit:
            return no_hit

    return no_hit
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `position` | vec3 | Fractal center |
| `size` | float | World scaling of fractal domain |
| `power` | float | Fractal exponent (model-dependent) |
| `iterations` | int | Iteration count in DE |
| `type` | enum/string | `mandelbulb`, `mandelbox`, `julia3d`, `menger`, `sierpinski` |
| `epsilon` | float | Hit threshold (internal) |
| `maxRaySteps` | int | Marching iteration cap |
