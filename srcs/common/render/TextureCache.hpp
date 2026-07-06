#ifndef TEXTURECACHE_HPP
#define TEXTURECACHE_HPP

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <stb_image.h>

#include "../Color.hpp"

namespace rc
{
    struct TextureImage
    {
        int width = 0;
        int height = 0;
        std::vector<uint8_t> pixels;
        bool valid = false;

        ColorF texel(int x, int y) const
        {
            const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 3;
            return ColorF{pixels[i] / 255.f, pixels[i + 1] / 255.f, pixels[i + 2] / 255.f};
        }

        ColorF sample(float u, float v) const
        {
            if (!valid || width <= 0 || height <= 0)
                return ColorF{1.f, 1.f, 1.f};

            u = u - std::floor(u);
            v = v - std::floor(v);
            v = 1.f - v;

            const float fx = u * static_cast<float>(width) - 0.5f;
            const float fy = v * static_cast<float>(height) - 0.5f;

            const int x0 = static_cast<int>(std::floor(fx));
            const int y0 = static_cast<int>(std::floor(fy));
            const float dx = fx - static_cast<float>(x0);
            const float dy = fy - static_cast<float>(y0);

            auto wrap = [](int c, int n) {
                c %= n;
                return (c < 0) ? c + n : c;
            };
            const int xa = wrap(x0, width);
            const int xb = wrap(x0 + 1, width);
            const int ya = wrap(y0, height);
            const int yb = wrap(y0 + 1, height);

            const ColorF c00 = texel(xa, ya);
            const ColorF c10 = texel(xb, ya);
            const ColorF c01 = texel(xa, yb);
            const ColorF c11 = texel(xb, yb);

            const ColorF top = c00 * (1.f - dx) + c10 * dx;
            const ColorF bottom = c01 * (1.f - dx) + c11 * dx;
            return top * (1.f - dy) + bottom * dy;
        }
    };

    // Header-only, thread-safe cache of decoded textures keyed by file path.
    // Decoding uses stb_image so it works in the render path (no SFML there).
    // The core binary and each renderer plugin link one stb_image_impl.cpp.
    class TextureCache
    {
        public:
            static const TextureImage &get(const std::string &path)
            {
                std::lock_guard<std::mutex> lock(mutex());

                auto &store = cache();
                auto it = store.find(path);
                if (it != store.end())
                    return (*it->second);

                auto image = std::make_shared<TextureImage>();
                load(*image, path);
                store.emplace(path, image);
                return (*store.at(path));
            }

        private:
            static std::map<std::string, std::shared_ptr<TextureImage>> &cache()
            {
                static std::map<std::string, std::shared_ptr<TextureImage>> store;
                return (store);
            }

            static std::mutex &mutex()
            {
                static std::mutex m;
                return (m);
            }

            static void load(TextureImage &image, const std::string &path)
            {
                if (path.empty())
                    return;

                int w = 0;
                int h = 0;
                int channels = 0;
                unsigned char *data = stbi_load(path.c_str(), &w, &h, &channels, 3);
                if (data == nullptr || w <= 0 || h <= 0)
                {
                    if (data != nullptr)
                        stbi_image_free(data);
                    return;
                }

                image.width = w;
                image.height = h;
                image.pixels.assign(data, data + static_cast<std::size_t>(w) * h * 3);
                image.valid = true;
                stbi_image_free(data);
            }
    };
}

#endif
