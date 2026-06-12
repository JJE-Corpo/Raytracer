//
// Created by jazema on 4/24/26.
//

#ifndef FILEOBSERVER_HPP
#define FILEOBSERVER_HPP
#include <filesystem>
#include <string>

namespace rc
{
    class FileObserver
    {
        private:
            std::string _filePath;
            std::filesystem::file_time_type _lastWriteTime;
        public:
            FileObserver() = default;
            FileObserver(const std::string &filePath);
            ~FileObserver() = default;

            void setFilePath(const std::string &filePath);
            std::string getFilePath() const;

            /**
             * Poll for changes in the file
             * @return true if the file has been modified since last poll, false otherwise
             */
            bool pollChanges();

            void reset();
    };
}

#endif
