# Directional Light

## Description

A directional light models a source at infinite distance (for example sunlight). All rays arrive with the same direction, independent of scene position.

## Light Model / Formula

Given normalized incoming light direction `L_in` (from light toward scene), the shading direction from point to light is:

```text
L = -L_in
```

Phong-style contribution:

```text
I_dir = I_light * [Kd * max(N.L, 0) + Ks * max(R.V, 0)^n]
```

No distance attenuation is applied for an ideal directional source.

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `direction` | vec3 | Incoming light direction (normalized) |
| `color` | vec3/rgb | Light color |
| `intensity` | float | Intensity multiplier |

## Shadow Ray

Shadow ray from hit point:

```text
origin = H + epsilon * N
direction = L
t_max = +infinity
```

Any occluder along this ray blocks the light.

## Attenuation

None for ideal directional light:

```text
attenuation = 1
```

## Pseudocode

```pseudocode
function evalDirectionalLight(light, hit, viewDir, scene):
    L = normalize(-light.direction)

    shadowRay.origin = hit.point + epsilon * hit.normal
    shadowRay.direction = L
    if isOccluded(scene, shadowRay, epsilon, infinity):
        return (0, 0, 0)

    NdotL = max(dot(hit.normal, L), 0)
    R = normalize(2 * NdotL * hit.normal - L)
    spec = pow(max(dot(R, viewDir), 0), hit.material.shininess)

    return light.color * light.intensity * (
           hit.material.Kd * NdotL + hit.material.Ks * spec)
```
