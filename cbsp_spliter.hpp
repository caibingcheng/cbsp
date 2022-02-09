#ifndef _CBSP_SPLITER_H_
#define _CBSP_SPLITER_H_

#include <fstream>
#include <cstdio>
#include <memory>
#include <cstring>

#include "cbsp_structor.hpp"
#include "cbsp_error.hpp"
#include "cbsp_utils.hpp"
#include "cbsp_tree.hpp"
#include "cbsp_crc.hpp"

namespace cbsp
{
    namespace spliter
    {
        inline int genFile(std::FILE *&fp, const char *filepath, const CBSP_BLOCKER &blocker)
        {
            if (!filepath)
                return CBSP_ERR_BAD_PATH;
            std::FILE *file = std::fopen(filepath, "r");
            if (file)
            {
                std::fclose(file);
                ErrorMessage::setMessage("%s already exists", filepath);
                return CBSP_ERR_AL_EXIST;
            }
            file = nullptr;
            auto _offset = std::ftell(fp);

            std::fseek(fp, blocker.offset, SEEK_SET);
            const static uint64_t batch_size = 10485760;
            auto buffer = std::make_unique<char[]>(batch_size);
            uint64_t length = blocker.length;

            uint32_t crc = crcBlocker(fp, blocker);
            if (crc != blocker.crc)
            {
                return CBSP_ERR_AL_MODIFY;
            }

            file = std::fopen(filepath, "wb");
            if (!file)
            {
                return CBSP_ERR_NO_TARGET;
            }
            length = blocker.length;
            std::fseek(fp, blocker.offset, SEEK_SET);
            while (length > batch_size)
            {
                auto gcount = std::fread(buffer.get(), sizeof(char), batch_size, fp);
                write(file, buffer.get(), gcount);
                length -= batch_size;
            }
            if (length > 0)
            {
                auto gcount = std::fread(buffer.get(), sizeof(char), length, fp);
                write(file, buffer.get(), gcount);
            }

            std::fclose(file);
            std::fseek(fp, _offset, SEEK_SET);

            return CBSP_ERR_SUCCESS;
        }

        inline int extract(std::FILE *&fp)
        {
            if (!fp)
            {
                return CBSP_ERR_NO_TARGET;
            }

            if (!isCBSP(fp))
            {
                return CBSP_ERR_NO_CBSP;
            }

            if (!crcMatch(fp))
            {
                return CBSP_ERR_BAD_CBSP;
            }

            auto tr = dirTree(fp);
            tr = cropTree(tr);
            conTree(tr);
            cbsp_assert(!tr.empty());

            auto header = getHeader(fp);
            auto blocker = getFirst(fp, header);
            auto count = header.count;
            while (count-- > 0)
            {
                if (!isCBSP(blocker))
                {
                    return CBSP_ERR_BAD_CBSP;
                }
                auto filename = getFileName(fp, blocker);
                auto filedir = getFileDir(fp, blocker);
                auto rpath = matchTree(tr, (filedir + "/" + filename).c_str());
                cbsp_assert(!rpath.empty());

                int ret = genFile(fp, rpath.c_str(), blocker);
                if (ret != CBSP_ERR_SUCCESS)
                {
                    printError(ret);
                }
                blocker = getCBSPBlocker(fp, blocker.next);
            }

            return CBSP_ERR_SUCCESS;
        }

        inline int printTree(std::FILE *&fp)
        {
            if (!fp)
            {
                return CBSP_ERR_NO_TARGET;
            }

            if (!isCBSP(fp))
            {
                return CBSP_ERR_NO_CBSP;
            }

            if (!crcMatch(fp))
            {
                return CBSP_ERR_BAD_CBSP;
            }

            auto tr = dirTree(fp);
            tr = cropTree(tr);
            conTree(tr);
            cbsp_assert(!tr.empty());

            auto header = getHeader(fp);
            auto blocker = getFirst(fp, header);
            auto count = header.count;
            while (count-- > 0)
            {
                if (!isCBSP(blocker))
                {
                    return CBSP_ERR_BAD_CBSP;
                }
                auto filename = getFileName(fp, blocker);
                auto filedir = getFileDir(fp, blocker);
                auto rpath = matchTree(tr, (filedir + "/" + filename).c_str());
                cbsp_assert(!rpath.empty());
                fprintf(stdout, "%s\n", rpath.c_str());

                blocker = getCBSPBlocker(fp, blocker.next);
            }

            return CBSP_ERR_SUCCESS;
        }

        inline int printHeader(std::FILE *&fp)
        {
            if (!fp)
            {
                return CBSP_ERR_NO_TARGET;
            }

            if (!isCBSP(fp))
            {
                return CBSP_ERR_NO_CBSP;
            }

            auto header = getHeader(fp);
            print(header);

            return CBSP_ERR_SUCCESS;
        }

        inline int printBlockers(std::FILE *&fp)
        {
            if (!fp)
            {
                return CBSP_ERR_NO_TARGET;
            }

            if (!isCBSP(fp))
            {
                return CBSP_ERR_NO_CBSP;
            }

            // print valid cbsp even it has been modified
            auto header = getHeader(fp);
            auto blocker = getFirst(fp, header);
            auto count = header.count;
            while (count-- > 0)
            {
                if (!isCBSP(blocker))
                {
                    return CBSP_ERR_BAD_CBSP;
                }
                print(blocker);
                blocker = getCBSPBlocker(fp, blocker.next);
            }

            // check if all blockers matched
            if (!crcMatch(fp))
            {
                return CBSP_ERR_BAD_CBSP;
            }

            return CBSP_ERR_SUCCESS;
        }
    }
}

#endif