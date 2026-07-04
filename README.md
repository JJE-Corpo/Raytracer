# Raytracer

![Language](https://img.shields.io/badge/language-C%2B%2B-blue)

A modular raytracer that renders synthetic 3D scenes by casting rays from a virtual camera, intersecting geometric primitives, and evaluating direct lighting.

## Summary

This project generates images by tracing rays through a scene composed of primitives and lights (plus optional material parameters in scene files). For each pixel, the renderer:

- Generates a primary ray from the camera.
- Finds the closest valid hit across scene geometry.
- Computes shading using light contributions and material response.
- Writes final color output to an image buffer/file.

Typical outputs include classic scenes with spheres, planes, cubes, cones, cylinders, triangles, tori, tanglecubes, and fractal distance-estimated objects.

## Documentation

| Topic | File |
|---|---|
| Architecture overview | [docs/architecture.md](docs/architecture.md) |
| Plugin system (`.so`) | [docs/plugins.md](docs/plugins.md) |
| Builders and factories | [docs/builders-and-factories.md](docs/builders-and-factories.md) |
| User interface plugin | [docs/userinterface/README.md](docs/userinterface/README.md) |
| UI core | [docs/userinterface/core/README.md](docs/userinterface/core/README.md) |
| UI components | [docs/userinterface/components/README.md](docs/userinterface/components/README.md) |
| UI panels | [docs/userinterface/panels/README.md](docs/userinterface/panels/README.md) |
| UI windows | [docs/userinterface/windows/README.md](docs/userinterface/windows/README.md) |
| UI toast system | [docs/userinterface/toast/README.md](docs/userinterface/toast/README.md) |
| Rendering pipeline | [docs/rendering-pipeline.md](docs/rendering-pipeline.md) |
| Object system index | [docs/objects/README.md](docs/objects/README.md) |
| Sphere primitive | [docs/objects/primitives/sphere.md](docs/objects/primitives/sphere.md) |
| Plane primitive | [docs/objects/primitives/plane.md](docs/objects/primitives/plane.md) |
| Cube primitive | [docs/objects/primitives/cube.md](docs/objects/primitives/cube.md) |
| Cylinder primitive | [docs/objects/primitives/cylinder.md](docs/objects/primitives/cylinder.md) |
| Cone primitive | [docs/objects/primitives/cone.md](docs/objects/primitives/cone.md) |
| Triangle primitive | [docs/objects/primitives/triangle.md](docs/objects/primitives/triangle.md) |
| Torus primitive | [docs/objects/primitives/torus.md](docs/objects/primitives/torus.md) |
| Tanglecube primitive | [docs/objects/primitives/tanglecube.md](docs/objects/primitives/tanglecube.md) |
| Fractal primitive | [docs/objects/primitives/fractal.md](docs/objects/primitives/fractal.md) |
| AABB primitive | [docs/objects/primitives/aabb.md](docs/objects/primitives/aabb.md) |
| Point light | [docs/objects/lights/point-light.md](docs/objects/lights/point-light.md) |
| Directional light | [docs/objects/lights/directional-light.md](docs/objects/lights/directional-light.md) |
| Camera model | [docs/camera.md](docs/camera.md) |
| Ray model | [docs/ray.md](docs/ray.md) |

## Features

- Primitives available in builders/factories: sphere, plane, cube, cylinder, cone, triangle, torus, tanglecube, fractal.
- Acceleration/utility geometry: AABB, BVH.
- Lights implemented in parsers/builders: point light, directional light.
- Additional source status: `Mobius_strip.*` exists but is currently not wired in parser/builder/Makefile.
- Core rendering capabilities:
	- Primary ray generation from perspective camera.
	- Closest-hit intersection logic.
	- Shadow rays for direct lighting visibility.
	- BVH build before rendering.
	- Config-driven scene construction with builder/factory pipeline.
	- Runtime plugin loading for renderers and user interface (`.so`).
	- Optional cluster module infrastructure.

## Getting Started

Build and run commands depend on your local setup. Typical workflow:

```bash
make
./raytracer path/to/scene.json
```

Optional test invocation placeholder:

```bash
make tests
```

