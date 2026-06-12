//
// Created by jazema on 5/16/26.
//

#ifndef RENDERKERNEL_HPP
#define RENDERKERNEL_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "../Color.hpp"
#include "../Intersection.hpp"
#include "../VectorUtils.hpp"
#include "../scene/ICamera.hpp"
#include "../scene/ILight.hpp"
#include "../scene/IScene.hpp"

namespace rc::render_kernel
{
    constexpr float EPS = 0.001f;
    constexpr float INF = std::numeric_limits<float>::infinity();

    inline float clamp01(float v)
    {
        return std::max(0.0f, std::min(1.0f, v));
    }

    inline ColorF color_sub(const ColorF &a, const ColorF &b)
    {
        return {a.r - b.r, a.g - b.g, a.b - b.b};
    }

    inline ColorF color_lerp(const ColorF &a, const ColorF &b, float t)
    {
        return a * (1.0f - t) + b * t;
    }

    inline float max_component(const ColorF &c)
    {
        return std::max(c.r, std::max(c.g, c.b));
    }

    inline float dielectric_f0(float ior)
    {
        float r0 = (ior - 1.0f) / (ior + 1.0f);
        return r0 * r0;
    }

    inline float effective_transparency(const Material &material)
    {
        float t = clamp01(material.transparency);
        if (material.model == MaterialModel::PBR)
        {
            t = std::max(t, clamp01(material.transmission));
            t = std::max(t, 1.0f - clamp01(material.alpha));
        }
        return t;
    }

    inline Vector3f reflect_dir(const Vector3f &v, const Vector3f &n)
    {
        return v - 2.0f * dot(v, n) * n;
    }

    inline Vector3f cross_dir(const Vector3f &a, const Vector3f &b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline uint32_t mix_uint32(uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;
        return value;
    }

    inline uint32_t hash_combine(uint32_t seed, uint32_t value)
    {
        return mix_uint32(seed ^ (value + 0x9e3779b9u + (seed << 6) + (seed >> 2)));
    }

    inline uint32_t quantize_float(float value)
    {
        return static_cast<uint32_t>(static_cast<int32_t>(std::floor(value * 2048.0f)));
    }

    inline float next_random(uint32_t &state)
    {
        state = mix_uint32(state);
        return static_cast<float>(state & 0x00ffffffu) / 16777216.0f;
    }

    inline void build_tangent_space(const Vector3f &normal, Vector3f &tangent, Vector3f &bitangent)
    {
        if (std::fabs(normal.z) < 0.999f)
            tangent = Vector3f(-normal.y, normal.x, 0.0f).unit_vector();
        else
            tangent = Vector3f(0.0f, -normal.z, normal.y).unit_vector();
        bitangent = cross_dir(normal, tangent).unit_vector();
    }

    inline uint32_t make_seed(const Vector3f &point, const Vector3f &normal, const Vector3f &dir, int depth)
    {
        uint32_t seed = 2166136261u;
        seed = hash_combine(seed, quantize_float(point.x));
        seed = hash_combine(seed, quantize_float(point.y));
        seed = hash_combine(seed, quantize_float(point.z));
        seed = hash_combine(seed, quantize_float(normal.x));
        seed = hash_combine(seed, quantize_float(normal.y));
        seed = hash_combine(seed, quantize_float(normal.z));
        seed = hash_combine(seed, quantize_float(dir.x));
        seed = hash_combine(seed, quantize_float(dir.y));
        seed = hash_combine(seed, quantize_float(dir.z));
        seed = hash_combine(seed, static_cast<uint32_t>(depth));
        return seed;
    }

    inline Vector3f sample_ggx_half_vector(const Vector3f &normal, float roughness, uint32_t &seed)
    {
        float alpha = std::max(1e-4f, roughness);
        float u1 = std::min(0.999999f, next_random(seed));
        float u2 = next_random(seed);
        float phi = 2.0f * static_cast<float>(M_PI) * u1;
        float tan2_theta = (alpha * alpha) * u2 / std::max(1e-7f, 1.0f - u2);
        float cos_theta = 1.0f / std::sqrt(1.0f + tan2_theta);
        float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));

        Vector3f tangent;
        Vector3f bitangent;
        build_tangent_space(normal, tangent, bitangent);

        Vector3f half_vector = tangent * (std::cos(phi) * sin_theta)
            + bitangent * (std::sin(phi) * sin_theta)
            + normal * cos_theta;
        return half_vector.unit_vector();
    }

    // https://stackoverflow.com/questions/34659359/how-do-the-permutation-and-gradient-tables-of-perlin-and-simplex-noise-work-in-p
    static const int PERMUTATION[] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
        140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,
        247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
        57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,
        74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
        60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
        65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,
        200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,
        52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
        207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
        119,248,152, 2,44,154,163,70,221,153,101,155,167, 43,172,9,
        129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
        218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
        81,51,145,235,249,14,239,107,49,192,214, 31,181,199,106,157,
        184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,
        222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    inline int perm(int i)
    {
        return PERMUTATION[i & 255];
    }

    inline float fade(float t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    inline float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    inline float grad(int hash, float x, float y, float z)
    {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    inline float perlin_noise(const Vector3f &p)
    {
        int X = static_cast<int>(std::floor(p.x)) & 255;
        int Y = static_cast<int>(std::floor(p.y)) & 255;
        int Z = static_cast<int>(std::floor(p.z)) & 255;

        float x = p.x - std::floor(p.x);
        float y = p.y - std::floor(p.y);
        float z = p.z - std::floor(p.z);

        float u = fade(x);
        float v = fade(y);
        float w = fade(z);

        int A  = perm(X) + Y;
        int AA = perm(A) + Z;
        int AB = perm(A + 1) + Z;
        int B  = perm(X + 1) + Y;
        int BA = perm(B) + Z;
        int BB = perm(B + 1) + Z;

        float res = lerp(
            lerp(
                lerp(grad(perm(AA), x, y, z), grad(perm(BA), x-1, y, z), u),
                lerp(grad(perm(AB), x, y - 1, z), grad(perm(BB), x - 1, y - 1, z), u), v),
            lerp(
                lerp(grad(perm(AA + 1), x, y, z - 1), grad(perm(BA+1), x - 1, y, z - 1), u),
                lerp(grad(perm(AB + 1), x, y - 1, z - 1), grad(perm(BB+1), x - 1, y - 1, z - 1), u), v), w);

        return res;
    }


    inline Vector3f perturb_direction_ggx(const Vector3f &reflection_dir, const Vector3f &normal, float roughness,
        const Vector3f &point, int depth)
    {
        if (roughness <= 0.0f)
            return reflection_dir.unit_vector();

        uint32_t seed = make_seed(point, normal, reflection_dir, depth);
        Vector3f half_vector = sample_ggx_half_vector(normal, roughness, seed);
        Vector3f perturbed = reflect_dir(reflection_dir, half_vector).unit_vector();

        if (dot(perturbed, normal) <= 0.0f)
            perturbed = perturbed - 2.0f * dot(perturbed, normal) * normal;
        return perturbed;
    }

    inline bool refract_dir(const Vector3f &v, const Vector3f &n, float eta, Vector3f &refracted)
    {
        float cos_theta = std::min(dot(-v, n), 1.0f);
        Vector3f r_out_perp = eta * (v + cos_theta * n);
        float k = 1.0f - r_out_perp.length_squared();
        if (k < 0.0f)
            return false;
        Vector3f r_out_parallel = -std::sqrt(k) * n;
        refracted = r_out_perp + r_out_parallel;
        return true;
    }

    inline float distribution_ggx(float NdotH, float roughness)
    {
        float a = roughness * roughness;
        float a2 = a * a;
        float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
        return a2 / (static_cast<float>(M_PI) * denom * denom + 1e-7f);
    }

    inline float geometry_schlick_ggx(float NdotV, float roughness)
    {
        float r = roughness + 1.0f;
        float k = (r * r) / 8.0f;
        return NdotV / (NdotV * (1.0f - k) + k + 1e-7f);
    }

    inline float geometry_smith(float NdotV, float NdotL, float roughness)
    {
        return geometry_schlick_ggx(NdotV, roughness) * geometry_schlick_ggx(NdotL, roughness);
    }

    inline float distribution_gtr1(float NdotH, float a)
    {
        if (a >= 1.0f)
            return 1.0f / static_cast<float>(M_PI);
        float a2 = a * a;
        float denom = static_cast<float>(M_PI) * std::log(a2) * (1.0f + (a2 - 1.0f) * NdotH * NdotH);
        return (a2 - 1.0f) / (denom + 1e-7f);
    }

    inline ColorF fresnel_schlick(float cosTheta, const ColorF &F0)
    {
        float t = std::pow(1.0f - cosTheta, 5.0f);
        return F0 + color_sub({1.0f, 1.0f, 1.0f}, F0) * t;
    }

    inline ColorF shadow_transmittance(const IScene &scene, const Vector3f &origin, const Vector3f &dir, float max_dist)
    {
        ColorF trans = {1.0f, 1.0f, 1.0f};
        float t_min = EPS;
        Ray shadow(origin, dir);
        Intersection occluder;

        while (scene.intersect(shadow, t_min, max_dist - EPS, occluder))
        {
            float alpha = effective_transparency(occluder.material);
            if (alpha <= 0.0f)
                return {0.0f, 0.0f, 0.0f};

            float tint_factor = 1.0f - alpha;
            ColorF tint = {
                alpha + tint_factor * occluder.material.baseColor.r,
                alpha + tint_factor * occluder.material.baseColor.g,
                alpha + tint_factor * occluder.material.baseColor.b
            };

            trans = trans * tint;
            if (trans.r <= 0.0f && trans.g <= 0.0f && trans.b <= 0.0f)
                return {0.0f, 0.0f, 0.0f};

            t_min = occluder.t + EPS;
        }

        return trans;
    }

    inline ColorF trace_ray(const Ray &ray, const IScene &scene, int depth)
    {
        if (depth <= 0)
            return {0, 0, 0};

        Intersection hit;
        if (!scene.intersect(ray, EPS, INF, hit))
            return {0, 0, 0};

        const Material &material = hit.material;
        ColorF baseColor = material.baseColor;

        Vector3f shaded_normal = hit.normal;
        if (material.normal_map_enabled && material.normal_map.empty())
        {
            float freq = std::max(1e-6f, material.normal_noise_frequency);
            float eps = 0.001f;
            Vector3f p = hit.point * freq;

            float dx = perlin_noise({p.x + eps, p.y, p.z}) - perlin_noise({p.x - eps, p.y, p.z});
            float dy = perlin_noise({p.x, p.y + eps, p.z}) - perlin_noise({p.x, p.y - eps, p.z});

            Vector3f tangent;
            Vector3f bitangent;
            build_tangent_space(hit.normal, tangent, bitangent);

            Vector3f n_tangent = Vector3f(dx, dy, 1.0f).unit_vector();
            Vector3f world_n = (tangent * n_tangent.x + bitangent * n_tangent.y + hit.normal * n_tangent.z).unit_vector();

            float strength = std::min(1.0f, std::max(0.0f, material.normal_scale));
            shaded_normal = (hit.normal * (1.0f - strength) + world_n * strength).unit_vector();
        }


        float ao = (material.model == MaterialModel::PBR) ? clamp01(material.ao) : 1.0f;
        float pbr_metallic = (material.model == MaterialModel::PBR) ? clamp01(material.metallic) : 0.0f;
        float ambient_weight = (material.model == MaterialModel::PBR) ? (1.0f - pbr_metallic) : 1.0f;
        ColorF local = baseColor * (scene.getAmbientCoefficient() * ao * ambient_weight);
        Vector3f view_dir = (-ray.direction).unit_vector();

        for (auto *light : scene.getLights())
        {
            if (!light)
                continue;
            if (light->isHidden())
                continue;
            Vector3f L;
            float dist;
            float attenuation = 1.0f;

            if (light->getKind() == LightKind::POINT)
            {
                Vector3f delta = light->getPosition() - hit.point;
                dist = delta.length();
                L = delta / dist;

                attenuation = 1.0f / (dist * dist);
            }
            else
            {
                L = light->getRotation().unit_vector();
                dist = 1e9f;
            }

            ColorF trans = shadow_transmittance(scene, hit.point + shaded_normal * EPS, L, dist);
            if (trans.r > 0.0f || trans.g > 0.0f || trans.b > 0.0f)
            {
                float NdotL = std::max(0.0f, dot(hit.normal, L));
                ColorF lightColor = light->getColorF() * trans;
                float intensity = light->getIntensity() * attenuation;

                if (material.model == MaterialModel::PBR)
                {
                    float roughness = std::max(0.02f, clamp01(material.roughness));
                    float metallic = clamp01(material.metallic);
                    float specular_level = clamp01(material.specular_level);
                    float specular_tint = clamp01(material.specular_tint);
                    float clearcoat = clamp01(material.clearcoat);
                    float clearcoat_roughness = std::max(0.02f, clamp01(material.clearcoat_roughness));
                    float sheen = clamp01(material.sheen);
                    float sheen_tint = clamp01(material.sheen_tint);
                    float transmission = clamp01(material.transmission);

                    Vector3f H = (view_dir + L).unit_vector();
                    float NdotV = std::max(0.0f, dot(hit.normal, view_dir));
                    float NdotH = std::max(0.0f, dot(hit.normal, H));
                    float VdotH = std::max(0.0f, dot(view_dir, H));

                    float dielectric = dielectric_f0(std::max(1.0f, material.ior));
                    ColorF tint = {1.0f, 1.0f, 1.0f};
                    float max_c = max_component(baseColor);
                    if (max_c > 0.0f)
                        tint = baseColor * (1.0f / max_c);

                    ColorF specular_color = color_lerp({1.0f, 1.0f, 1.0f}, tint, specular_tint);
                    ColorF F0_dielectric = specular_color * (dielectric * specular_level);
                    ColorF F0 = color_lerp(F0_dielectric, baseColor, metallic);
                    ColorF F = fresnel_schlick(VdotH, F0);
                    float D = distribution_ggx(NdotH, roughness);
                    float G = geometry_smith(NdotV, NdotL, roughness);

                    float denom = 4.0f * std::max(NdotV, 0.0f) * std::max(NdotL, 0.0f) + 1e-7f;
                    ColorF specular = F * (D * G / denom);

                    float Fd90 = 0.5f + 2.0f * roughness * VdotH * VdotH;
                    float light_scatter = 1.0f + (Fd90 - 1.0f) * std::pow(1.0f - NdotL, 5.0f);
                    float view_scatter = 1.0f + (Fd90 - 1.0f) * std::pow(1.0f - NdotV, 5.0f);

                    ColorF kS = F;
                    ColorF kD = color_sub({1.0f, 1.0f, 1.0f}, kS) * (1.0f - metallic) * (1.0f - transmission);
                    ColorF diffuse = baseColor * (light_scatter * view_scatter / static_cast<float>(M_PI));

                    float sheen_factor = std::pow(1.0f - VdotH, 5.0f) * sheen;
                    ColorF sheen_color = color_lerp({1.0f, 1.0f, 1.0f}, tint, sheen_tint) * sheen_factor;

                    float clearcoat_a = color_lerp({0.1f, 0.1f, 0.1f}, {0.001f, 0.001f, 0.001f}, clearcoat_roughness).r;
                    float Dc = distribution_gtr1(NdotH, clearcoat_a);
                    float Gc = geometry_schlick_ggx(NdotV, 0.25f) * geometry_schlick_ggx(NdotL, 0.25f);
                    float Fc = 0.04f + (1.0f - 0.04f) * std::pow(1.0f - VdotH, 5.0f);
                    float clearcoat_spec = clearcoat * 0.25f * (Dc * Gc * Fc / (denom + 1e-7f));

                    ColorF radiance = lightColor * intensity;
                    ColorF layered = (kD * diffuse + specular + sheen_color) * radiance * NdotL;
                    layered = layered + radiance * (clearcoat_spec * NdotL);

                    local = local + layered;
                }
                else
                {
                    ColorF diffuse = baseColor * lightColor
                        * (intensity * NdotL / static_cast<float>(M_PI));

                    Vector3f reflect_l = reflect_dir(-L, hit.normal).unit_vector();
                    float spec_angle = std::max(0.0f, dot(view_dir, reflect_l));
                    float specular_term = std::pow(spec_angle, material.shininess);
                    ColorF specular = material.specular * lightColor
                        * (intensity * specular_term);

                    local = local + diffuse + specular;
                }
            }
        }

        ColorF surface = local;

        if (material.model == MaterialModel::PBR)
        {
            Vector3f unit_dir = ray.direction.unit_vector();
            float metallic = pbr_metallic;
            float transmission = clamp01(material.transmission) * (1.0f - metallic);
            float alpha = clamp01(material.alpha);
            float transparency = transmission;
            float reflection_roughness = clamp01(material.roughness);

            float dielectric = dielectric_f0(std::max(1.0f, material.ior));
            float specular_level = clamp01(material.specular_level);
            float specular_tint = clamp01(material.specular_tint);
            ColorF tint = {1.0f, 1.0f, 1.0f};
            float max_c = max_component(baseColor);
            if (max_c > 0.0f)
                tint = baseColor * (1.0f / max_c);

            ColorF specular_color = color_lerp({1.0f, 1.0f, 1.0f}, tint, specular_tint);
            ColorF F0_dielectric = specular_color * (dielectric * specular_level);
            ColorF F0 = color_lerp(F0_dielectric, baseColor, metallic);

            float NdotV = std::max(0.0f, dot(hit.normal, (-unit_dir)));
            ColorF F = fresnel_schlick(NdotV, F0);
            float reflect_weight = clamp01(max_component(F));

            float weight_sum = reflect_weight + transparency;
            if (weight_sum > 1.0f)
            {
                reflect_weight /= weight_sum;
                transparency /= weight_sum;
            }

            // Distribute energy between local (diffuse + direct specular),
            // indirect specular (reflections) and transmission so they sum to 1.
            float non_trans_weight = 1.0f - transparency;
            float local_scale = non_trans_weight * (1.0f - reflect_weight);
            float refl_scale = non_trans_weight * reflect_weight;

            surface = local * std::max(0.0f, local_scale);

            if (refl_scale > 0.0f)
            {
                Vector3f reflected_dir = reflect_dir(unit_dir, hit.normal).unit_vector();
                reflected_dir = perturb_direction_ggx(reflected_dir, hit.normal, reflection_roughness, hit.point, depth);
                Ray reflected_ray(hit.point + hit.normal * EPS, reflected_dir);
                surface = surface + trace_ray(reflected_ray, scene, depth - 1) * refl_scale;
            }

            if (transparency > 0.0f)
            {
                float eta = hit.front_face ? (1.0f / std::max(1.0f, material.ior)) : std::max(1.0f, material.ior);
                Vector3f refracted_dir;
                if (refract_dir(unit_dir, hit.normal, eta, refracted_dir))
                {
                    Ray refracted_ray(hit.point - hit.normal * EPS, refracted_dir.unit_vector());
                    ColorF through = trace_ray(refracted_ray, scene, depth - 1);
                    through = through * baseColor;
                    surface = surface + through * transparency;
                }
                else
                {
                    Vector3f reflected_dir = reflect_dir(unit_dir, hit.normal).unit_vector();
                    Ray reflected_ray(hit.point + hit.normal * EPS, reflected_dir);
                    surface = surface + trace_ray(reflected_ray, scene, depth - 1) * transparency;
                }
            }

            if (alpha < 1.0f)
            {
                Ray through_ray(hit.point + unit_dir * EPS, unit_dir);
                ColorF through = trace_ray(through_ray, scene, depth - 1);
                surface = surface * alpha + through * (1.0f - alpha);
            }

            return surface;
        }

        float reflectivity = clamp01(material.reflectivity);
        float transparency = effective_transparency(material);

        if (reflectivity > 0.0f)
        {
            Vector3f reflected_dir = reflect_dir(ray.direction.unit_vector(), hit.normal).unit_vector();
            Ray reflected_ray(hit.point + hit.normal * EPS, reflected_dir);
            surface = surface + trace_ray(reflected_ray, scene, depth - 1) * reflectivity;
        }

        if (transparency > 0.0f)
        {
            Vector3f unit_dir = ray.direction.unit_vector();
            Ray through_ray(hit.point + unit_dir * EPS, unit_dir);
            ColorF through = trace_ray(through_ray, scene, depth - 1);
            return surface * (1.0f - transparency) + through * transparency;
        }

        return surface;
    }

    inline void render_tile_sample(const IScene &scene, const ICamera &camera,
        int start_x, int start_y, int end_x, int end_y, std::vector<ColorF> &out_colors)
    {
        const int tile_w = end_x - start_x;
        const int tile_h = end_y - start_y;
        out_colors.assign(static_cast<size_t>(tile_w * tile_h), {0.0f, 0.0f, 0.0f});

        for (int y = start_y; y < end_y; ++y)
        {
            for (int x = start_x; x < end_x; ++x)
            {
                Ray r = camera.generateRay(x, y);
                ColorF c = trace_ray(r, scene, 10);
                size_t idx = static_cast<size_t>(y - start_y) * static_cast<size_t>(tile_w)
                    + static_cast<size_t>(x - start_x);
                out_colors[idx] = c;
            }
        }
    }
}

#endif
