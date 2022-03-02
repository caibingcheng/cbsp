#ifndef _CBSP_CRC_H_
#define _CBSP_CRC_H_

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "cbsp_structor.hpp"
#include "cbsp_file.hpp"
#include "cbsp_error.hpp"
#include "cbsp_utils.hpp"

namespace cbsp
{
    /******************************************************************************
     * Name:    CRC-32  x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
     * Poly:    0x4C11DB7
     * Init:    0xFFFFFFF
     * Refin:   True
     * Refout:  True
     * Xorout:  0xFFFFFFF
     * Alias:   CRC_32/ADCCP
     * Use:     WinRAR,ect.
     *****************************************************************************/
    inline uint32_t crc32(uint8_t *data, uint16_t length, uint32_t crc = 0x0)
    {
        uint8_t i;
        crc = ~crc;
        while (length--)
        {
            crc ^= *data++; // crc ^= *data; data++;
            for (i = 0; i < 8; ++i)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320; // 0xEDB88320= reverse 0x04C11DB7
                else
                    crc = (crc >> 1);
            }
        }
        return ~crc;
    }

    inline uint32_t crcBlocker(std::FILE *&fp, const CBSP_BLOCKER &blocker)
    {
        uint32_t crc = 0x0;
        if (!isCBSP(blocker))
        {
            return crc;
        }

        {
            ChunkFile chunkfile(fp, batch_size, blocker.offset, blocker.length);
            for (auto it = chunkfile.begin(); it != chunkfile.end(); it++)
            {
                auto chunk = *it;
                crc = crc32(reinterpret_cast<uint8_t *>(chunk.data()), chunk.size(), crc);
            }
        }

        return crc;
    }

    inline uint32_t crcBlocker(std::FILE *&fp, const CBSP_HEADER &header)
    {
        if (!fp)
            return 0x0;

        // not a cbsp header
        if (!isCBSP(header))
        {
            return 0x0;
        }
        auto blocker = getFirst(fp, header);
        auto count = header.count;

        uint32_t crc = 0x0;
        while (count-- > 0)
        {
            // not a cbsp blocker
            if (!isCBSP(blocker))
            {
                return 0x0;
            }
            crc = crc32(reinterpret_cast<uint8_t *>(&blocker), blocker.size, crc);
            blocker = getCBSPBlocker(fp, blocker.next);
        }

        return crc;
    }

    inline bool crcMatch(std::FILE *&fp)
    {
        if (!fp)
            return false;

        auto header = getHeader(fp);

        // not a cbsp header
        if (!isCBSP(header))
        {
            return false;
        }

        uint32_t crc = crcBlocker(fp, header);
        bool match = (crc == header.crc);

        if (!match)
        {
            ErrorMessage::setMessage("crc mismatch 0x%x -- 0x%x", header.crc, crc);
        }

        return match;
    }
}

#endif