# Cube Primitive

## Description

A cube is a finite convex polyhedron with 6 planar faces, 12 edges, and 8 vertices. In this project it is an oriented cube (position + rotation + scale) built from 6 face parallelograms.

## Mathematical Definition

In local object space (before transform), for half-size `s = size / 2`:

```text
|x| <= s
|y| <= s
|z| <= s
```

Each face is a bounded plane patch (parallelogram/rectangle).

## Ray-Intersection Calculation

Ray equation:

```text
P(t) = O + tD
```

The implementation tests intersection against each face patch and keeps the minimum valid `t`.

For one face plane with point `A` and normal `N`:

```text
t = ((A - O) . N) / (D . N)
```

Then point-in-face test is done by expressing hit point in local face basis `(AB, AC)`:

```text
P = A + u AB + v AC
0 <= u <= 1 and 0 <= v <= 1
```

If valid, candidate `t` is accepted. The global cube hit uses the smallest candidate in `[t_min, t_max]`.

## Normal Computation

1. Determine which face produced the winning hit.
2. Use that local axis-aligned face normal (`+/-X`, `+/-Y`, `+/-Z`).
3. Transform normal with rotation and inverse scale correction.
4. Normalize and orient against ray if needed.

## Edge Cases

- Ray parallel to face plane: denominator near zero.
- Hit outside face bounds after plane hit.
- Non-uniform scale requires inverse-scale correction for normals.
- Origin inside cube: nearest valid exiting face still selected.

## Pseudocode

```pseudocode
function intersectCube(ray, transform, size, tMin, tMax):
    faces = buildSixTransformedFaces(transform, size)

    bestT = +infinity
    bestFace = -1
    for i in 0..5:
        t = intersectFaceParallelogram(ray, faces[i])
        if tMin < t < tMax and t < bestT:
            bestT = t
            bestFace = i

    if bestFace == -1:
        return no_hit

    Nlocal = normalFromFaceIndex(bestFace)
    Nworld = normalize(transformNormal(Nlocal, transform.rotation, transform.scale))

    H = ray.at(bestT)
    return hit_record(bestT, H, Nworld)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `position` | vec3 | Cube center |
| `rotation` | vec3 | Euler rotation |
| `scale` | vec3 | Non-uniform scale |
| `size` | float | Base edge length |
| `material` | material ref | Surface material |
