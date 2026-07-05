# Objects System

## Overview

The object system separates scene entities into two major categories:

- Primitives: geometric surfaces that can be intersected by rays.
- Lights: emitters that contribute radiance for shading.

This separation allows independent implementation and extension of geometry and illumination logic.

In a typical interface-based design:

- A primitive exposes at least `intersect(ray)` and `normal(hitPoint)` behavior.
- A light exposes an evaluation routine that returns contribution at a shaded point, optionally with visibility checks.

## Primitive Documentation

- [Sphere](./primitives/sphere.md)
- [Plane](./primitives/plane.md)
- [Cube](./primitives/cube.md)
- [Cylinder](./primitives/cylinder.md)
- [Cone](./primitives/cone.md)
- [Triangle](./primitives/triangle.md)
- [Torus](./primitives/torus.md)
- [Tanglecube](./primitives/tanglecube.md)
- [Fractal](./primitives/fractal.md)
- [Mesh](./primitives/mesh.md)
- [AABB](./primitives/aabb.md)

### Notes

- The repository also contains `Mobius_strip.*` source placeholders, but this primitive is not currently connected to scene parsing/building.

## Light Documentation

- [Point Light](./lights/point-light.md)
- [Directional Light](./lights/directional-light.md)

Current parser/builder support in this repository is limited to point and directional lights.

## Data and Flow

At render time:

1. A camera creates a ray.
2. Primitives are queried for the closest hit.
3. Material parameters are read from the hit and used by the active renderer model.
4. Lights are sampled and visibility-tested with shadow rays.
5. Contributions are accumulated into final pixel color.

## Related Documents

- [Rendering Pipeline](../rendering-pipeline.md)
- [Camera](../camera.md)
- [Ray](../ray.md)

