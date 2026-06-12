//
// Created by jazema on 4/24/26.
//

#include "FileObserver.hpp"

namespace rc
{
    FileObserver::FileObserver(const std::string &filePath)
    {
        setFilePath(filePath);
    }

    void FileObserver::setFilePath(const std::string &filePath)
    {
        this->_filePath = filePath;
        if (!std::filesystem::exists(this->_filePath))
        {
            this->_lastWriteTime = std::filesystem::file_time_type::min();
            return;
        }
        this->_lastWriteTime = std::filesystem::last_write_time(this->_filePath);
    }

    std::string FileObserver::getFilePath() const
    {
        return (this->_filePath);
    }

    bool FileObserver::pollChanges()
    {
        if (this->_filePath.empty())
            return (false);
        std::filesystem::file_time_type current;
        std::error_code ec;

        current = std::filesystem::last_write_time(this->_filePath, ec);
        if (ec || current == this->_lastWriteTime)
            return (false);
        this->_lastWriteTime = current;
        return (true);
    }

    void FileObserver::reset()
    {
        if (this->_filePath.empty() || !std::filesystem::exists(this->_filePath))
            return;
        this->_lastWriteTime = std::filesystem::last_write_time(this->_filePath);
    }

    //todo revoir en temps voulu pour faire un vrai observer
    // -> callback

}
