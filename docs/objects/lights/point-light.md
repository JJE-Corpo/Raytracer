# Point Light

## Description

A point light emits from a single position in space in all directions (isotropic emitter). It approximates small bulbs or local light sources.

## Light Model / Formula

At shaded point `H`:

```text
Lvec = P_light - H
d = ||Lvec||
L = normalize(Lvec)
```

Phong-style direct contribution:

```text
I_point = attenuation(d) * I_light * [Kd * max(N.L, 0) + Ks * max(R.V, 0)^n]
```

with reflected direction:

```text
R = normalize(2(N.L)N - L)
```

## Parameters

| Parameter | Type | Description |
|---|---|---|
| `position` | vec3 | World-space light position |
| `color` | vec3/rgb | Light color |
| `intensity` | float | Scalar intensity multiplier |
| `constant` | float | Constant attenuation term `k_c` |
| `linear` | float | Linear attenuation term `k_l` |
| `quadratic` | float | Quadratic attenuation term `k_q` |

## Shadow Ray

Shadow test uses a segment ray from:

```text
origin = H + epsilon * N
direction = L
t_max = d - epsilon
```

If any occluder intersects before `t_max`, this light contributes zero direct term at `H`.

## Attenuation

Standard attenuation:

```text
attenuation(d) = 1 / (k_c + k_l d + k_q d^2)
```

Clamp denominator to avoid division by near-zero values.

## Pseudocode

```pseudocode
function evalPointLight(light, hit, viewDir, scene):
    Lvec = light.position - hit.point
    dist = length(Lvec)
    L = Lvec / dist

    shadowRay.origin = hit.point + epsilon * hit.normal
    shadowRay.direction = L
    if isOccluded(scene, shadowRay, epsilon, dist - epsilon):
        return (0, 0, 0)

    NdotL = max(dot(hit.normal, L), 0)
    R = normalize(2 * NdotL * hit.normal - L)
    spec = pow(max(dot(R, viewDir), 0), hit.material.shininess)

    att = 1.0 / (light.kc + light.kl * dist + light.kq * dist * dist)
    return light.color * light.intensity * att * (
           hit.material.Kd * NdotL + hit.material.Ks * spec)
```
