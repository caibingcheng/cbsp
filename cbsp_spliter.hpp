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
            uint64_t length = blocker.length;

            uint32_t crc = crcBlocker(fp, blocker);
            if (crc != blocker.crc)
            {
                ErrorMessage::setMessage("Blocker %s broken", filepath);
                ErrorMessage::setMessage("Mismatch crc 0x%x 0x%x", crc, blocker.crc);
                return CBSP_ERR_AL_MODIFY | CBSP_ERR_BAD_CBSP;
            }

            file = std::fopen(filepath, "wb");
            if (!file)
            {
                return CBSP_ERR_NO_TARGET;
            }

            ChunkFile chunkfile(fp, batch_size, blocker.offset, blocker.length);
            for (auto it = chunkfile.begin(); it != chunkfile.end(); it++)
            {
                auto &chunk = *it;
                write(file, chunk.data(), chunk.size());
            }

            std::fclose(file);
            std::fseek(fp, _offset, SEEK_SET);

            // after write done, check the outfile crc again
            file = std::fopen(filepath, "rb");
            if (!file)
            {
                return CBSP_ERR_NO_TARGET;
            }
            crc = 0x0;
            ChunkFile chunkTest(file, batch_size);
            for (auto it = chunkTest.begin(); it != chunkTest.end(); it++)
            {
                auto &chunk = *it;
                crc = crc32(reinterpret_cast<uint8_t *>(chunk.data()), chunk.size(), crc);
            }
            std::fclose(file);

            if (crc != blocker.crc)
            {
                ErrorMessage::setMessage("Extract %s failed", filepath);
                ErrorMessage::setMessage("Mismatch crc 0x%x 0x%x", crc, blocker.crc);
                return CBSP_ERR_AL_MODIFY;
            }

            return CBSP_ERR_SUCCESS;
        }

        inline int extract(std::FILE *&fp, const char *outdir = nullptr)
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

            if (getHeader(fp).count <= 0)
            {
                return CBSP_ERR_NO_CBSP;
            }

            int result = CBSP_ERR_SUCCESS;
            auto hasout = [&outdir]() -> bool
            { return !std::string(outdir).empty(); }();

            auto tr = dirTree(fp);
            tr = cropTree(tr);
            auto old_tr = tr;

            if (hasout)
            {
                auto dirs = listDirs(rpath(outdir));
                while (!dirs.empty())
                {
                    auto dir = dirs.back();
                    dirs.pop_back();
                    CBSP_TREE _tr;
                    _tr.push_back({.isFile = false,
                                   .path = dir,
                                   .children = tr});
                    tr = _tr;
                }
            }
            result = conTree(tr, hasout);
            cbsp_assert(!tr.empty());
            if (result != CBSP_ERR_SUCCESS)
            {
                return result;
            }

            tr = old_tr;
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
                if (hasout)
                {
                    rpath = std::string(outdir) + "/" + rpath;
                }
                cbsp_assert(!rpath.empty());

                result |= genFile(fp, rpath.c_str(), blocker);
                blocker = getCBSPBlocker(fp, blocker.next);
            }

            return result;
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

            if (getHeader(fp).count <= 0)
            {
                return CBSP_ERR_NO_CBSP;
            }

            auto tr = dirTree(fp);
            tr = cropTree(tr);
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