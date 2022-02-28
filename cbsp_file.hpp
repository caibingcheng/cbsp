#ifndef _CBSP_FILE_H_
#define _CBSP_FILE_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include "cbsp_structor.hpp"
#include "cbsp_error.hpp"
#include "cbsp_utils.hpp"
#include "cbsp_crc.hpp"

namespace cbsp
{
    class Chunk
    {
    public:
        Chunk() = default;
        Chunk(const uint64_t &size) : m_size(0), m_data(new char[size]), m_rsize(size) {}
        virtual ~Chunk() = default;

        virtual char *data() const noexcept { return m_data.get(); }
        virtual uint64_t size() const noexcept { return m_size; }
        virtual bool empty() const noexcept { return m_data.get() == nullptr || m_size == 0; }

    protected:
        uint64_t m_size;
        uint64_t m_rsize;
        std::shared_ptr<char[]> m_data;
    };

    class ChunkFile : public Chunk
    {
    public:
        ChunkFile() = default;
        ChunkFile(const ChunkFile &other) { *this = other; }
        ChunkFile(std::FILE *&file) : m_file(file),                // file pointer
                                      Chunk(flength(file)),        // memory allocated
                                      m_storage(std::ftell(file)), // current position
                                      m_mlength(flength(file)),    // file length
                                      m_length(m_mlength),         // chunk length <= file length, the end of the chunk
                                      m_offset(0),                 // start position
                                      m_bsize(m_mlength)           // expected chunk size
        {
        }
        ChunkFile(std::FILE *&file, const uint64_t &size) : m_file(file),
                                                            Chunk(size),
                                                            m_storage(std::ftell(file)),
                                                            m_mlength(flength(file)),
                                                            m_length(m_mlength),
                                                            m_offset(0),
                                                            m_bsize(size)
        {
        }
        ChunkFile(std::FILE *&file, const uint64_t &size, const uint64_t &offset) : m_file(file),
                                                                                    Chunk(size),
                                                                                    m_storage(std::ftell(file)),
                                                                                    m_mlength(flength(file)),
                                                                                    m_length(rlength(offset, m_mlength, m_mlength)),
                                                                                    m_offset((offset > m_length) ? m_length : offset),
                                                                                    m_bsize(size)
        {
        }
        ChunkFile(std::FILE *&file, const uint64_t &size, const uint64_t &offset, const uint64_t &length) : m_file(file),
                                                                                                            Chunk(size),
                                                                                                            m_storage(std::ftell(file)),
                                                                                                            m_mlength(flength(file)),
                                                                                                            m_length(rlength(offset, length, m_mlength)),
                                                                                                            m_offset((offset > m_length) ? m_length : offset),
                                                                                                            m_bsize((size > length) ? length : size)
        {
        }
        virtual ~ChunkFile()
        {
            reset();
        };

        ChunkFile &operator=(const ChunkFile &other) = default;
        bool operator==(const ChunkFile &other) const noexcept
        {
            return std::tie(m_file, m_offset, m_length) == std::tie(other.m_file, other.m_offset, other.m_length);
        }
        bool operator!=(const ChunkFile &other) const noexcept
        {
            return m_file != other.m_file ||
                   m_offset != other.m_offset ||
                   m_length != other.m_length;
        }
        ChunkFile &operator++() noexcept
        {
            next();
            return *this;
        }
        // TODO: should no copy
        ChunkFile &operator++(int) noexcept
        {
            next();
            return *this;
        }
        ChunkFile &operator*() noexcept
        {
            uint64_t pos = fpos();

            // reach the end of file
            if (pos >= m_length ||
                pos >= m_mlength ||
                std::feof(m_file))
            {
                m_size = 0;
            }
            else
            {
                std::fseek(m_file, m_offset, SEEK_SET);
                memset(m_data.get(), 0, m_rsize);
                m_size = std::fread(m_data.get(), sizeof(char), m_bsize, m_file);
                if (m_size != m_bsize)
                {
                    m_bsize = m_size;
                }
                std::fseek(m_file, pos, SEEK_SET);
            }

            return *this;
        }

        void reset() noexcept
        {
            std::fseek(m_file, m_storage, SEEK_SET);
        }
        ChunkFile &begin() noexcept
        {
            std::fseek(m_file, m_offset, SEEK_SET);
            return *this;
        }
        ChunkFile &next() noexcept
        {
            m_offset += m_bsize;
            m_offset = (m_offset >= m_length) ? m_length : m_offset;
            std::fseek(m_file, m_offset, SEEK_SET);
            return *this;
        }
        const ChunkFile &end() const noexcept
        {
            static ChunkFile chunkfile;
            chunkfile = *this;
            chunkfile.m_offset = m_length;

            return chunkfile;
        }

        const uint64_t batch() const { return m_bsize; }
        const uint64_t offset() const { return m_offset; }
        const uint64_t rlength() const { return m_length; }
        const uint64_t fpos() const { return std::ftell(m_file); }
        const uint64_t flength() const { return m_mlength; }

    private:
        // keep file first
        std::FILE *m_file;
        uint64_t m_storage;
        uint64_t m_mlength;
        uint64_t m_length;
        // keep last
        uint64_t m_offset;
        uint64_t m_bsize;

        uint64_t flength(std::FILE *&file) noexcept
        {
            uint64_t offset = std::ftell(file);
            std::fseek(file, 0, SEEK_END);
            uint64_t length = std::ftell(file);
            std::fseek(file, offset, SEEK_SET);

            return length;
        }
        uint64_t rlength(uint64_t offset, uint64_t length, uint64_t mlength)
        {
            uint64_t nlength = offset + length;
            if (nlength < mlength)
                return nlength;
            return mlength;
        }
    };

    class CBSPFile
    {
    public:
        CBSPFile() = default;
        CBSPFile(const char *filename) { open(filename); }
        CBSPFile(std::FILE *&file) : m_file(file),
                                     m_status(check()) {}
        virtual ~CBSPFile() { close(); }

        std::FILE *&operator&() { return m_file; }
        operator bool() const
        {
            return m_file != nullptr &&
                   m_status == CBSP_ERR_SUCCESS;
        }

        void close()
        {
            if (m_file)
            {
                std::fclose(m_file);
                m_file = nullptr;
            }
        }

        int open(std::FILE *&file)
        {
            close();
            m_file = file;
            m_status = check();

            return m_status;
        }
        // just open a cbsp file for read
        int open(const char *filename)
        {
            close();
            m_file = std::fopen(filename, "rb");

            m_status = check();
            return m_status;
        }

        // open a cbsp file for read and write
        // create if not exist
        int create(const char *filename)
        {
            close();
            // if exists, move to the end
            // if not exists, create a new one
            m_file = std::fopen(filename, "ab+");
            if (!m_file)
            {
                return CBSP_ERR_NO_TARGET;
            }
            close();
            // reading extension mode
            m_file = std::fopen(filename, "rb+");
            if (!m_file)
            {
                return CBSP_ERR_NO_TARGET;
            }

            uint64_t length = fileLenght(m_file);
            // file is empty
            if (length <= 0)
            {
                CBSP_HEADER header;
                header.size = sizeof(CBSP_HEADER);
                setHeader(m_file, header);
            }

            m_status = check();
            return m_status;
        }

        int check()
        {
            if (!m_file)
            {
                return CBSP_ERR_NO_TARGET;
            }

            uint64_t length = fileLenght(m_file);
            // file not empty, and not cbsp
            if (length > 0 && !isCBSP(m_file))
            {
                close();
                return CBSP_ERR_NO_CBSP;
            }

            if (!crcMatch(m_file))
            {
                close();
                return CBSP_ERR_BAD_CBSP;
            }

            return CBSP_ERR_SUCCESS;
        }

    private:
        // keep m_file first
        std::FILE *m_file = nullptr;
        int m_status = CBSP_ERR_SUCCESS;

        CBSPFile(const CBSPFile &) = delete;
        CBSPFile(CBSPFile &&) = delete;
        void operator=(const CBSPFile &) = delete;
        void operator=(CBSPFile &&) = delete;
    };
}

#endif