#ifndef _CBSP_COMBINER_H_
#define _CBSP_COMBINER_H_

#include <fstream>
#include <memory>
#include <string>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/param.h>

#include "cbsp_structor.hpp"
#include "cbsp_error.hpp"
#include "cbsp_file.hpp"
#include "cbsp_utils.hpp"
#include "cbsp_tree.hpp"
#include "cbsp_crc.hpp"

namespace cbsp
{
    namespace combiner
    {
        inline bool alreadyExist(std::FILE *&fp, const char *filepath)
        {
            if (!filepath)
                return CBSP_ERR_BAD_PATH;

            std::string filename = fileName(filepath);
            std::string filedir = fileDir(filepath);

            auto header = getHeader(fp);
            auto offset = header.first;
            auto count = header.count;
            while (count-- > 0)
            {
                auto blocker = getCBSPBlocker(fp, offset);
                if (filename == getFileName(fp, blocker) &&
                    filedir == getFileDir(fp, blocker))
                    return true;
                offset = blocker.next;
            }
            return false;
        }

        int add(std::FILE *&fp, const char *opath)
        {
            if (!opath)
            {
                return CBSP_ERR_BAD_PATH;
            }

            // check if cbsp exists
            if (!fp)
            {
                return CBSP_ERR_NO_TARGET;
            }

            char filepath[PATH_MAX];
            if (!realpath(opath, filepath))
            {
                return CBSP_ERR_NO_SOURCE;
            }

            if (access(filepath, R_OK) != 0)
            {
                ErrorMessage::setMessage("Access deined %s", filepath);
                return CBSP_ERR_DEN_ACCESS;
            }

            // check if the source file already exists
            std::FILE *file = std::fopen(filepath, "rb");
            if (!file)
            {
                return CBSP_ERR_NO_SOURCE;
            }

            // check if the file already exists in cbsp
            if (alreadyExist(fp, filepath))
            {
                std::fclose(file);
                ErrorMessage::setMessage("%s already exists", opath);
                return CBSP_ERR_AL_EXIST;
            }

            std::string filename = fileName(filepath);
            std::string filedir = fileDir(filepath);

            // seek to the end of the cbsp file
            // trying to write
            std::fseek(fp, 0, SEEK_END);

            // get the FILE offset
            // content offset
            uint64_t offset = std::ftell(fp);

            // if the target offset is zero(empty file)
            // reset it to a cbsp file
            // else check if it's a cbsp file
            if (0 == offset)
            {
                CBSP_HEADER header;
                // set the header size
                // the size is set only at initialization
                header.size = sizeof(CBSP_HEADER);
                // default crc
                // the default crc is only set here
                header.crc = 0x0;
                // set the magic number and write header to target
                // if write header failed, the process failed
                if (setHeader(fp, header) < static_cast<int>(header.size))
                {
                    return CBSP_ERR_CREATE_FAILED;
                }
                // move to the end fp target
                std::fseek(fp, 0, SEEK_END);
                offset = std::ftell(fp);
            }
            // if not a empty file
            // check if it's a cbsp file
            else if (!isCBSP(fp))
            {
                std::fclose(file);
                return CBSP_ERR_NO_CBSP;
            }

            if (!crcMatch(fp))
            {
                std::fclose(file);
                return CBSP_ERR_BAD_CBSP;
            }

            // get the source file length
            uint64_t length = fileLenght(file);

            // struct offset
            uint64_t stOffset = offset + length;

            // fname offset
            uint64_t fnameOffset = stOffset + sizeof(CBSP_BLOCKER);
            uint64_t fnameLength = strlen(filename.c_str());

            // fdirname offset
            uint64_t fdirOffset = fnameOffset + fnameLength;
            uint64_t fdirLength = strlen(filedir.c_str());

            // the total length fp the blocker
            // uint64_t reserve = length + sizeof(CBSP_BLOCKER) + fnameLength + fdirLength;

            CBSP_BLOCKER blocker;
            blocker.magic = CBSP_MAGIC;
            blocker.size = sizeof(CBSP_BLOCKER);
            blocker.offset = offset;
            blocker.length = length;
            blocker.fnameOffset = fnameOffset;
            blocker.fnameLength = fnameLength;
            blocker.fdirOffset = fdirOffset;
            blocker.fdirLength = fdirLength;

            // cp source to target
            uint32_t crc = 0x0;
            std::fseek(fp, offset, SEEK_SET);
            {
                const static uint64_t batch_size = 10485760; // 10 × 1024 × 1024
                ChunkFile chunkfile(file, batch_size);

                for (auto it = chunkfile.begin(); it != chunkfile.end(); it++)
                {
                    auto &chunk = *it;
                    cbsp_assert(!chunk.empty());
                    if (chunk.empty())
                    {
                        std::fclose(file);
                        return CBSP_ERR_NO_SOURCE;
                    }
                    write(fp, chunk.data(), chunk.size());
                    crc = crc32(reinterpret_cast<uint8_t *>(chunk.data()), chunk.size(), crc);
                }
            }
            std::fclose(file);

            // set blocker header
            blocker.crc = crc;
            write(fp, blocker, stOffset, sizeof(CBSP_BLOCKER));
            write(fp, const_cast<char *>(filename.c_str()), fnameOffset, fnameLength);
            write(fp, const_cast<char *>(filedir.c_str()), fdirOffset, fdirLength);

            // after write done
            auto header = getHeader(fp);
            if (hasLast(header))
            {
                auto last = getLast(fp, header);
                last.next = stOffset;
                setLast(fp, header, last);
            }
            else
            {
                header.size = sizeof(CBSP_HEADER);
                header.first = stOffset;
            }

            // header crc = all blocker header
            header.count++;
            header.last = stOffset;
            header.crc = crcBlocker(fp, header);
            // if write header failed, the process failed
            if (setHeader(fp, header) < static_cast<int>(header.size))
            {
                return CBSP_ERR_CREATE_FAILED;
            }

            return CBSP_ERR_SUCCESS;
        }
    }
}

#endif