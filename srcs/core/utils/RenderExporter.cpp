//
// Created by jazema on 4/24/26.
//

#include "RenderExporter.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <vector>

#include "../exceptions/ExportRenderException.hpp"

namespace rc
{
    namespace
    {
        void appendBigEndian32(std::vector<uint8_t> &buffer, uint32_t value)
        {
            buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
            buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
            buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            buffer.push_back(static_cast<uint8_t>(value & 0xFF));
        }

        uint32_t crc32(const uint8_t *data, size_t length)
        {
            static uint32_t table[256];
            static bool tableReady = false;

            if (!tableReady)
            {
                for (uint32_t n = 0; n < 256; ++n)
                {
                    uint32_t c = n;
                    for (int k = 0; k < 8; ++k)
                        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                    table[n] = c;
                }
                tableReady = true;
            }
            uint32_t crc = 0xFFFFFFFFu;
            for (size_t i = 0; i < length; ++i)
                crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
            return (crc ^ 0xFFFFFFFFu);
        }

        uint32_t adler32(const uint8_t *data, size_t length)
        {
            const uint32_t MOD = 65521;
            uint32_t a = 1;
            uint32_t b = 0;

            for (size_t i = 0; i < length; ++i)
            {
                a = (a + data[i]) % MOD;
                b = (b + a) % MOD;
            }
            return ((b << 16) | a);
        }

        void writeChunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data)
        {
            std::vector<uint8_t> typed;

            typed.reserve(4 + data.size());
            for (int i = 0; i < 4; ++i)
                typed.push_back(static_cast<uint8_t>(type[i]));
            typed.insert(typed.end(), data.begin(), data.end());
            appendBigEndian32(out, static_cast<uint32_t>(data.size()));
            out.insert(out.end(), typed.begin(), typed.end());
            appendBigEndian32(out, crc32(typed.data(), typed.size()));
        }

        // Wrap the raw scanlines in a zlib stream made only of uncompressed
        // ("stored") DEFLATE blocks. This keeps the encoder dependency-free
        // while still producing a spec-compliant PNG.
        std::vector<uint8_t> zlibStore(const std::vector<uint8_t> &raw)
        {
            std::vector<uint8_t> out;
            const size_t maxBlock = 65535;
            size_t offset = 0;

            out.push_back(0x78);
            out.push_back(0x01);
            do
            {
                size_t block = std::min(maxBlock, raw.size() - offset);
                bool last = (offset + block >= raw.size());
                uint16_t len16 = static_cast<uint16_t>(block);
                uint16_t nlen = static_cast<uint16_t>(~len16);

                out.push_back(last ? 0x01 : 0x00);
                out.push_back(static_cast<uint8_t>(len16 & 0xFF));
                out.push_back(static_cast<uint8_t>((len16 >> 8) & 0xFF));
                out.push_back(static_cast<uint8_t>(nlen & 0xFF));
                out.push_back(static_cast<uint8_t>((nlen >> 8) & 0xFF));
                out.insert(out.end(), raw.begin() + offset, raw.begin() + offset + block);
                offset += block;
            } while (offset < raw.size());
            appendBigEndian32(out, adler32(raw.data(), raw.size()));
            return (out);
        }
    }

    void RenderExporter::saveToFile(const Render &render, const std::string &export_path)
    {
        const size_t width = static_cast<size_t>(render.size_x);
        const size_t height = static_cast<size_t>(render.size_y);
        std::vector<uint8_t> raw;

        raw.reserve(height * (1 + width * 3));
        for (size_t y = 0; y < height; ++y)
        {
            raw.push_back(0); // filter type: None
            for (size_t x = 0; x < width; ++x)
            {
                const Color &pixel = render.pixels[y * width + x];
                raw.push_back(pixel.r);
                raw.push_back(pixel.g);
                raw.push_back(pixel.b);
            }
        }

        std::vector<uint8_t> png;
        const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
        png.insert(png.end(), signature, signature + 8);

        std::vector<uint8_t> ihdr;
        appendBigEndian32(ihdr, static_cast<uint32_t>(width));
        appendBigEndian32(ihdr, static_cast<uint32_t>(height));
        ihdr.push_back(8); // bit depth
        ihdr.push_back(2); // color type: truecolor (RGB)
        ihdr.push_back(0); // compression method
        ihdr.push_back(0); // filter method
        ihdr.push_back(0); // interlace method
        writeChunk(png, "IHDR", ihdr);
        writeChunk(png, "IDAT", zlibStore(raw));
        writeChunk(png, "IEND", {});

        std::ofstream out(export_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
            throw ExportRenderException(export_path, "Could not open file");
        out.write(reinterpret_cast<const char *>(png.data()), static_cast<std::streamsize>(png.size()));
        out.close();
    }
}
