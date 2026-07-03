/*
 *  ╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
 *  ║  ████████╗ █████╗ ████████╗ █████╗ ███╗   ██╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗  ║
 *  ║     ██╔╝  ██╔══██╗╚══██╔══╝██╔══██╗████╗  ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝  ║
 *  ║     ██║   ███████║   ██║   ███████║██╔██╗ ██║█████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗  █████╗    ║
 *  ║     ██║   ██╔══██║   ██║   ██╔══██║██║╚██╗██║██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝  ██╔══╝    ║
 *  ║     ██║   ██║  ██║   ██║   ██║  ██║██║ ╚████║███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗███████╗  ║
 *  ║     ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝╚══════╝  ║
 *  ╚════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝
 *
 *  ┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
 *  │ File   : TimestampUtils.h
 *  │ Author : Tataneeeeeeeeeee
 *  │ Date   : 2026-07-02
 *  │ Brief  : Timestamp utils
 *  └────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
*/

#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>

namespace rc
{
    class TimestampUtils
    {
    public:
        static std::string getCurrent()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::tm *local_time = std::localtime(&now_time);

            char buffer[20];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time);

            return std::string(buffer);
        }

        static std::string toString(const std::chrono::system_clock::time_point &timestamp, const char* format = "%Y-%m-%d %H:%M:%S")
        {
            std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
            std::tm *local_time = std::localtime(&time);

            char buffer[20];
            std::strftime(buffer, sizeof(buffer), format, local_time);

            return std::string(buffer);
        }

        static std::string toString(const std::filesystem::file_time_type &timestamp, const char* format = "%Y-%m-%d %H:%M:%S")
        {
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                timestamp - std::filesystem::file_time_type::clock::now()
                          + std::chrono::system_clock::now());

            return toString(sctp, format);
        }
    };
}