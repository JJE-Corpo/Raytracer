#ifndef ONLINEMARKET_HPP
#define ONLINEMARKET_HPP

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include "../../../common/Color.hpp"

namespace rc
{
    struct OnlineMaterial
    {
        enum class Thumb { None, Queued, Downloading, ImageReady, Ready, Failed };
        // Full-res diffuse texture download pipeline (separate from the thumbnail).
        enum class Tex { None, Queued, Downloading, Ready, Failed };

        std::string id;
        std::string name;
        std::string author;
        std::string categoriesText;
        std::string tagsText;
        std::string thumbnailUrl;
        int resolutionX = 0;
        int resolutionY = 0;

        Thumb thumb = Thumb::None;
        sf::Image image;
        bool hasAvgColor = false;
        ColorF avgColor = {0.6f, 0.6f, 0.6f};

        bool added = false;

        Tex texState = Tex::None;
        std::string texturePath;     // local file once downloaded
        bool queuedToScene = false;  // material handed off to the scene already
    };

    struct OnlineMarket
    {
        enum class Status { Idle, Loading, Ready, Error };

        std::mutex mutex;
        std::vector<OnlineMaterial> materials;
        std::string errorText;
        std::atomic<Status> status{Status::Idle};

        std::atomic<bool> running{false};
        std::atomic<bool> fetchRequested{false};
        std::thread worker;

        ~OnlineMarket()
        {
            this->stop();
        }

        void start()
        {
            if (this->running.exchange(true))
                return;
            this->worker = std::thread(&OnlineMarket::run, this);
        }

        void stop()
        {
            this->running = false;
            if (this->worker.joinable())
                this->worker.join();
        }

        void requestFetch()
        {
            this->start();
            this->fetchRequested = true;
        }

        void run()
        {
            while (this->running)
            {
                if (this->fetchRequested.exchange(false))
                    this->doFetch();

                // Texture downloads take priority so a clicked "Add" resolves fast.
                int texJob = this->nextTextureJob();
                if (texJob >= 0)
                {
                    this->doTexture(static_cast<std::size_t>(texJob));
                    continue;
                }

                int job = this->nextThumbnailJob();
                if (job < 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(40));
                    continue;
                }
                this->doThumbnail(static_cast<std::size_t>(job));
            }
        }

        // Ask the worker to download the full-res texture for `index`.
        void requestTexture(std::size_t index)
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            if (index < this->materials.size() && this->materials[index].texState == OnlineMaterial::Tex::None)
                this->materials[index].texState = OnlineMaterial::Tex::Queued;
        }

        int nextTextureJob()
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            for (std::size_t i = 0; i < this->materials.size(); ++i)
            {
                if (this->materials[i].texState == OnlineMaterial::Tex::Queued)
                {
                    this->materials[i].texState = OnlineMaterial::Tex::Downloading;
                    return (static_cast<int>(i));
                }
            }
            return (-1);
        }

        void doTexture(std::size_t index)
        {
            std::string id;
            {
                std::lock_guard<std::mutex> lock(this->mutex);
                if (index >= this->materials.size())
                    return;
                id = this->materials[index].id;
            }

            std::string path;
            const bool ok = downloadTexture(id, path);

            std::lock_guard<std::mutex> lock(this->mutex);
            if (index >= this->materials.size() || this->materials[index].id != id)
                return;
            if (ok)
            {
                this->materials[index].texturePath = path;
                this->materials[index].texState = OnlineMaterial::Tex::Ready;
            }
            else
            {
                this->materials[index].texState = OnlineMaterial::Tex::Failed;
            }
        }

        int nextThumbnailJob()
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            for (std::size_t i = 0; i < this->materials.size(); ++i)
            {
                if (this->materials[i].thumb == OnlineMaterial::Thumb::Queued)
                {
                    this->materials[i].thumb = OnlineMaterial::Thumb::Downloading;
                    return (static_cast<int>(i));
                }
            }
            return (-1);
        }

        void doFetch()
        {
            this->status = Status::Loading;

            std::string body;
            if (!curlFetch("https://api.polyhaven.com/assets?type=textures", body, 20))
            {
                std::lock_guard<std::mutex> lock(this->mutex);
                this->errorText = "Could not reach api.polyhaven.com (offline, or curl missing).";
                this->status = Status::Error;
                return;
            }

            std::vector<OnlineMaterial> parsed;
            try
            {
                nlohmann::json root = nlohmann::json::parse(body);
                for (auto it = root.begin(); it != root.end(); ++it)
                {
                    const nlohmann::json &a = it.value();
                    if (!a.is_object())
                        continue;

                    OnlineMaterial m;
                    m.id = it.key();
                    m.name = a.value("name", m.id);
                    m.thumbnailUrl = a.value("thumbnail_url", std::string());
                    if (a.contains("authors") && a["authors"].is_object() && !a["authors"].empty())
                        m.author = a["authors"].begin().key();
                    if (a.contains("categories"))
                        m.categoriesText = joinStrings(a["categories"], ", ", 4);
                    if (a.contains("tags"))
                        m.tagsText = joinStrings(a["tags"], ", ", 8);
                    if (a.contains("max_resolution") && a["max_resolution"].is_array()
                        && a["max_resolution"].size() == 2
                        && a["max_resolution"][0].is_number() && a["max_resolution"][1].is_number())
                    {
                        m.resolutionX = a["max_resolution"][0].get<int>();
                        m.resolutionY = a["max_resolution"][1].get<int>();
                    }
                    parsed.push_back(std::move(m));
                }
            }
            catch (const std::exception &e)
            {
                std::lock_guard<std::mutex> lock(this->mutex);
                this->errorText = std::string("Malformed response: ") + e.what();
                this->status = Status::Error;
                return;
            }

            std::sort(parsed.begin(), parsed.end(), [](const OnlineMaterial &a, const OnlineMaterial &b) {
                return (a.name < b.name);
            });

            {
                std::lock_guard<std::mutex> lock(this->mutex);
                this->materials = std::move(parsed);
            }
            this->status = Status::Ready;
        }

        void doThumbnail(std::size_t index)
        {
            std::string url;
            std::string id;
            {
                std::lock_guard<std::mutex> lock(this->mutex);
                if (index >= this->materials.size())
                    return;
                url = this->materials[index].thumbnailUrl;
                id = this->materials[index].id;
            }

            std::string body;
            sf::Image image;
            const bool ok = !url.empty()
                && curlFetch(url, body, 10)
                && image.loadFromMemory(body.data(), body.size());

            std::lock_guard<std::mutex> lock(this->mutex);
            if (index >= this->materials.size() || this->materials[index].id != id)
                return;
            OnlineMaterial &m = this->materials[index];
            if (!ok)
            {
                m.thumb = OnlineMaterial::Thumb::Failed;
                return;
            }
            m.avgColor = averageColor(image);
            m.hasAvgColor = true;
            m.image = std::move(image);
            m.thumb = OnlineMaterial::Thumb::ImageReady;
        }

        static bool curlFetch(const std::string &url, std::string &out, int timeoutSeconds)
        {
            out.clear();

            std::string cmd = "curl -fsSL --max-time " + std::to_string(timeoutSeconds)
                + " " + shellQuote(url);
            FILE *pipe = popen(cmd.c_str(), "r");
            if (pipe == nullptr)
                return (false);

            char buffer[8192];
            std::size_t n;
            while ((n = std::fread(buffer, 1, sizeof(buffer), pipe)) > 0)
                out.append(buffer, n);

            const int rc = pclose(pipe);
            return (rc == 0 && !out.empty());
        }

        static std::string shellQuote(const std::string &s)
        {
            std::string q = "'";
            for (char c : s)
            {
                if (c == '\'')
                    q += "'\\''";
                else
                    q += c;
            }
            q += "'";
            return (q);
        }

        // ~/.raytracer/textures, created on demand. "" if HOME is unusable.
        static std::string texturesDir()
        {
            const char *home = std::getenv("HOME");
            if (home == nullptr || home[0] == '\0')
                return ("");

            std::error_code ec;
            std::filesystem::path dir = std::filesystem::path(home) / ".raytracer" / "textures";
            std::filesystem::create_directories(dir, ec);
            if (ec)
                return ("");
            return (dir.string());
        }

        // Pick a sane resolution node from a Poly Haven map entry: prefer 1k/2k
        // so downloads stay small, else fall back to whatever exists.
        static const nlohmann::json *pickResolution(const nlohmann::json &mapNode)
        {
            if (!mapNode.is_object())
                return (nullptr);
            for (const char *pref : {"1k", "2k", "4k"})
                if (mapNode.contains(pref) && mapNode[pref].is_object())
                    return (&mapNode[pref]);
            for (auto it = mapNode.begin(); it != mapNode.end(); ++it)
                if (it.value().is_object())
                    return (&it.value());
            return (nullptr);
        }

        // Fetch the asset's file listing, locate the diffuse/albedo map, download
        // it to ~/.raytracer/textures and return the saved path in `outPath`.
        static bool downloadTexture(const std::string &id, std::string &outPath)
        {
            std::string listing;
            if (!curlFetch("https://api.polyhaven.com/files/" + id, listing, 20))
                return (false);

            std::string url;
            std::string ext;
            try
            {
                nlohmann::json root = nlohmann::json::parse(listing);
                const nlohmann::json *diffuse = nullptr;
                for (auto it = root.begin(); it != root.end(); ++it)
                {
                    std::string key = it.key();
                    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (key.find("diff") != std::string::npos || key.find("albedo") != std::string::npos
                        || key == "col")
                    {
                        diffuse = &it.value();
                        break;
                    }
                }
                if (diffuse == nullptr)
                    return (false);

                const nlohmann::json *res = pickResolution(*diffuse);
                if (res == nullptr)
                    return (false);

                for (const char *fmt : {"png", "jpg"})
                {
                    if (res->contains(fmt) && (*res)[fmt].is_object() && (*res)[fmt].contains("url")
                        && (*res)[fmt]["url"].is_string())
                    {
                        url = (*res)[fmt]["url"].get<std::string>();
                        ext = fmt;
                        break;
                    }
                }
                if (url.empty())
                    return (false);
            }
            catch (const std::exception &)
            {
                return (false);
            }

            std::string body;
            if (!curlFetch(url, body, 60))
                return (false);

            const std::string dir = texturesDir();
            if (dir.empty())
                return (false);
            outPath = dir + "/" + id + "." + ext;
            std::ofstream file(outPath, std::ios::binary);
            if (!file.is_open())
                return (false);
            file.write(body.data(), static_cast<std::streamsize>(body.size()));
            return (file.good());
        }

        static std::string joinStrings(const nlohmann::json &arr, const std::string &sep, std::size_t maxCount)
        {
            if (!arr.is_array())
                return ("");

            std::string out;
            std::size_t count = 0;
            for (const auto &v : arr)
            {
                if (!v.is_string())
                    continue;
                if (count >= maxCount)
                {
                    out += sep + "...";
                    break;
                }
                if (count > 0)
                    out += sep;
                out += v.get<std::string>();
                ++count;
            }
            return (out);
        }

        static ColorF averageColor(const sf::Image &img)
        {
            const sf::Vector2u size = img.getSize();
            if (size.x == 0 || size.y == 0)
                return (ColorF{0.6f, 0.6f, 0.6f});

            std::uint64_t r = 0;
            std::uint64_t g = 0;
            std::uint64_t b = 0;
            std::uint64_t n = 0;
            const unsigned int step = std::max(1u, size.x / 32u);
            for (unsigned int y = 0; y < size.y; y += step)
            {
                for (unsigned int x = 0; x < size.x; x += step)
                {
                    const sf::Color c = img.getPixel(x, y);
                    r += c.r;
                    g += c.g;
                    b += c.b;
                    ++n;
                }
            }
            if (n == 0)
                return (ColorF{0.6f, 0.6f, 0.6f});
            Color avg(static_cast<uint8_t>(r / n), static_cast<uint8_t>(g / n), static_cast<uint8_t>(b / n));
            return (ColorF::fromColor(avg));
        }
    };
}

#endif
