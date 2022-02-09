#ifndef _CBSP_UTILS_H_
#define _CBSP_UTILS_H_

#include <fstream>
#include <memory>
#include <cstring>
#include <cstdio>

#ifdef _WIN32_WINNT
#include <io.h>
#else
#include <unistd.h>
#endif

#include "cbsp_structor.hpp"

#ifdef DEBUG
#include <cassert>
#define cbsp_assert(expression) assert(expression)
#define cbsp_assert_msg(expression, ...) \
    if (!(expression))                   \
    {                                    \
        fprintf(stderr, __VA_ARGS__);    \
        assert(expression);              \
    }
#else
#define cbsp_assert(expression) void(0)
#define cbsp_assert_msg(expression, ...) void(0)
#endif

namespace cbsp
{
    const uint64_t CBSP_MAGIC = 0x4BF2D1;

    template <typename T>
    inline T read(std::FILE *&fp, uint64_t offset, uint32_t size)
    {
        T out;
        memset(&out, 0, sizeof(T));

        uint64_t _offset = std::ftell(fp);
        std::fseek(fp, offset, SEEK_SET);
        std::fread(reinterpret_cast<char *>(&out), sizeof(char), size, fp);
        std::fseek(fp, _offset, SEEK_SET);

        return out;
    }

    inline void *read(std::FILE *&fp, void *out, uint64_t offset, uint32_t size)
    {
        uint64_t _offset = std::ftell(fp);
        std::fseek(fp, offset, SEEK_SET);
        std::fread(reinterpret_cast<char *>(out), sizeof(char), size, fp);
        std::fseek(fp, _offset, SEEK_SET);

        return out;
    }

    inline int write(const void *data, size_t size, size_t n, FILE *fp)
    {
        auto ok = std::fwrite(data, size, n, fp);
        // #ifdef _WIN32_WINNT
        //         int fd = _fileno(fp);
        //         _commit(fd);
        // #else
        //         int fd = ::fileno(fp);
        //         ::fsync(fd);
        // #endif

        return ok;
    }

    template <typename T>
    inline int write(std::FILE *&fp, T &data, uint64_t offset, uint32_t size)
    {
        uint64_t _offset = std::ftell(fp);
        std::fseek(fp, offset, SEEK_SET);
        auto ok = write(reinterpret_cast<char *>(&data), sizeof(char), size, fp);
        std::fseek(fp, _offset, SEEK_SET);

        return ok;
    }

    template <typename T>
    inline int write(std::FILE *&fp, T *data, uint64_t offset, uint32_t size)
    {
        uint64_t _offset = std::ftell(fp);
        std::fseek(fp, offset, SEEK_SET);
        auto ok = write(reinterpret_cast<char *>(data), sizeof(char), size, fp);
        std::fseek(fp, _offset, SEEK_SET);

        return ok;
    }

    template <typename T>
    inline int write(std::FILE *&fp, T &data, uint32_t size)
    {
        auto ok = write(reinterpret_cast<char *>(data), sizeof(char), size, fp);

        return ok;
    }

    template <typename T>
    inline int write(std::FILE *&fp, T *data, uint32_t size)
    {
        auto ok = write(reinterpret_cast<char *>(data), sizeof(char), size, fp);

        return ok;
    }

    inline bool isCBSP(const CBSP_HEADER &header)
    {
        return header.magic == CBSP_MAGIC;
    }

    inline bool isCBSP(const CBSP_BLOCKER &blocker)
    {
        return blocker.magic == CBSP_MAGIC;
    }

    inline bool isCBSP(std::FILE *&fp)
    {
        uint32_t magic = read<uint32_t>(fp, 0, sizeof(uint32_t));
        return magic == CBSP_MAGIC;
    }

    inline bool isCBSP(std::FILE *&fp, uint64_t offset)
    {
        uint32_t magic = read<uint32_t>(fp, offset, sizeof(uint32_t));
        return magic == CBSP_MAGIC;
    }

    inline uint32_t getCBSPSize(std::FILE *&fp, uint64_t offset)
    {
        // uint32_t magic
        // uint32_t size
        uint32_t size = 0;
        // check the header or blocker is cbsp
        if (isCBSP(fp, offset))
        {
            size = read<uint32_t>(fp, offset + sizeof(uint32_t), sizeof(uint32_t));
        }
        return size;
    }

    inline CBSP_BLOCKER getCBSPBlocker(std::FILE *&fp, uint64_t offset)
    {
        // header is not a blocker as defined
        if (offset <= 0)
            return CBSP_BLOCKER();

        uint32_t size = getCBSPSize(fp, offset);
        // return empty block if failed
        if (size <= 0)
            return CBSP_BLOCKER();

        CBSP_BLOCKER blocker = read<CBSP_BLOCKER>(fp, offset, size);
        return blocker;
    }

    inline CBSP_HEADER getHeader(std::FILE *&fp)
    {
        uint32_t size = getCBSPSize(fp, 0);
        // return empty header if failed
        if (size <= 0)
            return CBSP_HEADER();

        CBSP_HEADER header = read<CBSP_HEADER>(fp, 0, size);
        return header;
    }

    inline bool hasFirst(const CBSP_HEADER &header)
    {
        return header.first != 0;
    }

    inline bool hasLast(const CBSP_HEADER &header)
    {
        return header.last != 0;
    }

    inline CBSP_BLOCKER getNext(std::FILE *&fp, const CBSP_BLOCKER &blocker)
    {
        CBSP_BLOCKER next = getCBSPBlocker(fp, blocker.next);
        return next;
    }

    inline CBSP_BLOCKER getFirst(std::FILE *&fp, const CBSP_HEADER &header)
    {
        CBSP_BLOCKER blocker = getCBSPBlocker(fp, header.first);
        return blocker;
    }

    inline CBSP_BLOCKER getLast(std::FILE *&fp, const CBSP_HEADER &header)
    {
        CBSP_BLOCKER blocker = getCBSPBlocker(fp, header.last);
        return blocker;
    }

    inline CBSP_BLOCKER getFirst(std::FILE *&fp)
    {
        CBSP_HEADER header = getHeader(fp);
        return getFirst(fp, header);
    }

    inline CBSP_BLOCKER getLast(std::FILE *&fp)
    {
        CBSP_HEADER header = getHeader(fp);
        return getLast(fp, header);
    }

    inline bool isLast(std::FILE *&fp, const CBSP_BLOCKER &current)
    {
        auto last = getLast(fp);

        // if not a cbsp blocker, define it's not last
        if (!isCBSP(last) || !isCBSP(current))
        {
            return false;
        }

        return std::tie(
                   last.fnameOffset,
                   last.fnameLength,
                   last.fdirOffset,
                   last.fdirLength,
                   last.offset,
                   last.length) == std::tie(current.fnameOffset,
                                            current.fnameLength,
                                            current.fdirOffset,
                                            current.fdirLength,
                                            current.offset,
                                            current.length);
    }

    inline int setHeader(std::FILE *&fp, CBSP_HEADER &header)
    {
        header.magic = CBSP_MAGIC;
        return write(fp, header, 0, header.size);
    }

    inline int setFirst(std::FILE *&fp, const CBSP_HEADER &header, CBSP_BLOCKER &blocker)
    {
        if (header.first <= 0)
            return CBSP_ERR_BAD_OFFSET;
        return write(fp, blocker, header.first, blocker.size);
    }

    inline int setLast(std::FILE *&fp, const CBSP_HEADER &header, CBSP_BLOCKER &blocker)
    {
        if (header.last <= 0)
            return CBSP_ERR_BAD_OFFSET;
        return write(fp, blocker, header.last, blocker.size);
    }

    inline int setFirst(std::FILE *&fp, CBSP_BLOCKER &blocker)
    {
        CBSP_HEADER header = getHeader(fp);
        return setFirst(fp, header, blocker);
    }

    inline int setLast(std::FILE *&fp, CBSP_BLOCKER &blocker)
    {
        CBSP_HEADER header = getHeader(fp);
        return setLast(fp, header, blocker);
    }

    inline uint64_t fileLenght(std::FILE *&fp)
    {
        uint64_t _offset = std::ftell(fp);
        std::fseek(fp, 0, SEEK_END);
        uint64_t lenght = std::ftell(fp);
        std::fseek(fp, _offset, SEEK_SET);

        return lenght;
    }
}

#endif