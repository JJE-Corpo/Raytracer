//
// Created by jazema on 5/14/26.
//

#ifndef BYTEBUFFER_HPP
#define BYTEBUFFER_HPP
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <netinet/in.h>

namespace rc
{
    class ByteBuffer
    {
        public:
            std::vector<std::uint8_t> data;
            int pos = 0;

            // template <typename T>
            // void write(T value)
            // {
                // uint8_t *ptr = reinterpret_cast<uint8_t *>(&value);
                // this->data.insert(this->data.end(), ptr, ptr + sizeof(T));
            // }

            void write(uint16_t value)
            {
                uint16_t nvalue = htons(value);
                this->data.insert(this->data.end(), reinterpret_cast<uint8_t *>(&nvalue), reinterpret_cast<uint8_t *>(&nvalue) + sizeof(nvalue));
            }

            void write(uint32_t value)
            {
                uint32_t nvalue = htonl(value);
                this->data.insert(this->data.end(), reinterpret_cast<uint8_t *>(&nvalue), reinterpret_cast<uint8_t *>(&nvalue) + sizeof(nvalue));
            }

            void write(float value)
            {
                static_assert(sizeof(float) == sizeof(uint32_t));

                uint32_t bits;
                std::memcpy(&bits, &value, sizeof(bits));

                bits = htonl(bits);

                this->data.insert(
                    this->data.end(),
                    reinterpret_cast<uint8_t*>(&bits),
                    reinterpret_cast<uint8_t*>(&bits) + sizeof(bits)
                );
            }

            void writeString(const std::string &str)
            {
                write(static_cast<uint32_t>(str.size()));
                this->data.insert(this->data.end(), str.begin(), str.end());
            }

            template <typename T>
            T read()
            {
                T value;

                if (this->pos + sizeof(T) > this->data.size())
                    throw std::runtime_error("ByteBuffer read out of bounds");
                std::memcpy(&value, this->data.data() + this->pos, sizeof(T));
                this->pos += sizeof(T);
                return value;
            }

            uint16_t readUInt16()
            {
                uint16_t value = read<uint16_t>();
                return ntohs(value);
            }

            uint32_t readUInt32()
            {
                uint32_t value = read<uint32_t>();
                return ntohl(value);
            }

            float readFloat()
            {
                uint32_t bits = readUInt32();
                float value;

                std::memcpy(&value, &bits, sizeof(value));
                return (value);
            }

            std::string readString()
            {
                uint32_t len = readUInt32();
                if (this->pos + len > this->data.size())
                    throw std::runtime_error("ByteBuffer readString out of bounds");

                std::string str(this->data.begin() + this->pos, this->data.begin() + this->pos + len);
                this->pos += len;
                return (str);
            }
    };

}

#endif
