# Mesh Primitive

## Description

A `mesh` is a triangulated surface loaded from a Wavefront `.obj` file. It is a
single scene primitive that internally owns its triangles and a **local BVH**, so
a mesh of hundreds of thousands of triangles is intersected in logarithmic time
and behaves like any other primitive (sphere, cube, …) to the rest of the
engine.

The `.obj` file is loaded once, at construction, through the existing
[`ObjParser`](../../../srcs/core/obj/ObjParser.hpp). The geometry is recentered on
its own bounding-box center, so `position` places the **center** of the mesh
(the same convention as the other primitives), then scaled, rotated and
translated into world space.

## Features

- **Local BVH**: an internal [`BVHNode`](../../../srcs/core/scene/bvh/BVHNode.hpp)
  is built over the mesh triangles. Rays never test the triangles linearly.
- **Global AABB**: the mesh exposes one bounding box to the scene BVH
  (`scene.buildBvh()`), so it participates in scene-level acceleration.
- **Smooth normals**: when the `.obj` provides vertex normals (`vn`), they are
  interpolated with barycentric coordinates at the hit point. Otherwise a
  per-face (flat) normal is computed as a fallback.
- **Möller–Trumbore** ray/triangle intersection.
- **Transforms**: `position`, `rotation` (Euler degrees) and non-uniform `scale`
  are baked into the world-space vertices; normals use the inverse-transpose
  (`R · S⁻¹`) so non-uniform scale stays correct.
- **Material**: the full PBR / Phong material model is supported, exactly like
  the other primitives. The shared material is applied to every hit.

## Normal Computation

For a hit with barycentric coordinates `(u, v)` and `w = 1 - u - v`:

```text
Smooth (vertex normals present):  N = normalize(w·N0 + u·N1 + v·N2)
Flat  (fallback):                 N = normalize((V1 - V0) x (V2 - V0))
```

The normal is then oriented against the ray (front/back face) like every other
primitive.

## Parameters

| Parameter  | Type        | Default       | Description |
|------------|-------------|---------------|-------------|
| `type`     | string      | —             | Must be `"mesh"`. |
| `file`     | string      | `""`          | Path to the `.obj` file, resolved from the working directory. |
| `position` | vec3        | `[0, 0, 0]`   | World position of the mesh center. |
| `rotation` | vec3        | `[0, 0, 0]`   | Euler rotation in **degrees** (applied X, then Y, then Z). |
| `scale`    | vec3        | `[1, 1, 1]`   | Per-axis scale (non-uniform allowed). |
| `material` | material ref| default       | Name of a material declared in the `materials` list. |

## Example

```json
{
    "name": "Bust",
    "type": "mesh",
    "file": "tests/obj/slt.obj",
    "position": [0.0, 20.0, 6.0],
    "rotation": [0.0, 0.0, 25.0],
    "scale": [110.0, 110.0, 110.0],
    "material": "GoldMetal"
}
```

See [`tests/configs/mesh_showcase.json`](../../../tests/configs/mesh_showcase.json)
for a full scene using three instances of the same mesh with contrasting PBR
materials (polished metal, glass, clearcoat).

## Behavior & Limitations

- Only **triangular** faces are supported (as with the `ObjParser`); the loader
  raises an error on non-triangular faces.
- A missing or invalid `.obj` path raises a clear parsing error; the object is
  skipped rather than crashing the renderer.
- Vertex texture coordinates (`vt`) and material libraries (`mtllib` / `usemtl`)
  are ignored; shading uses the material assigned in the scene file.
- An empty `file` produces an empty mesh (no geometry) — used as the editor's
  default placeholder.
- Editing the transform rebuilds the local BVH from the cached object-space
  geometry (the `.obj` file is not re-read).

## Related

- [Triangle primitive](./triangle.md) — the single-triangle primitive and the
  Möller–Trumbore reference.
- [AABB](./aabb.md) and the scene BVH — acceleration structures reused here.
