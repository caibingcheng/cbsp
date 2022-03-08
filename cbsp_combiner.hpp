#ifndef _CBSP_COMBINER_H_
#define _CBSP_COMBINER_H_

#include <fstream>
#include <memory>
#include <string>

#include <thread>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/param.h>

#include "cbsp_structor.hpp"
#include "cbsp_thread.hpp"
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
            uint32_t pathDigest = crc32(filepath, strlen(filepath));

            auto header = getHeader(fp);
            auto offset = header.first;
            auto count = header.count;
            while (count-- > 0)
            {
                auto blocker = getCBSPBlocker(fp, offset);
                if (pathDigest == blocker.pathDigest &&
                    filename == getFileName(fp, blocker) &&
                    filedir == getFileDir(fp, blocker))
                    return true;
                offset = blocker.next;
            }
            return false;
        }

        static std::mutex g_wrtex;
        inline void asyncAdd(std::FILE *fp, std::string path)
        {
            const char *filepath = path.c_str();
            // check if the source file already exists
            std::FILE *file = std::fopen(filepath, "rb");
            if (!file)
            {
                throw std::runtime_error(std::string("no such file") + filepath);
            }

            // check if the file already exists in cbsp
            std::unique_lock<std::mutex> lock(g_wrtex);
            lock.unlock();

            lock.lock();
            bool exists = alreadyExist(fp, filepath);
            lock.unlock();

            if (exists)
            {
                std::fclose(file);
                ErrorMessage::setMessage("%s already exists", filepath);
            }

            std::string filename = fileName(filepath);
            std::string filedir = fileDir(filepath);

            lock.lock();
            bool is_match = crcMatch(fp);
            lock.unlock();

            if (!is_match)
            {
                std::fclose(file);
                throw std::runtime_error(std::string("crc not matched"));
            }

            // get the source file length
            uint64_t length = fileLenght(file);
            static const uint64_t large_length = batch_size;
            static const uint64_t info_length = 1024 * 1024 * 1;

            if (length < large_length)
            {
                Buffer buffer(length + info_length);

                // cp source to target
                uint32_t crc = 0x0;
                {
                    ChunkFile chunkfile(file, batch_size);
                    while(!chunkfile.eof())
                    {
                        ChunkFile &chunk = chunkfile.get();
                        cbsp_assert(!chunk.empty());
                        if (chunk.empty())
                        {
                            std::fclose(file);
                            throw std::runtime_error(std::string("bad chunk"));
                        }
                        cbsp_assert(buffer.write(chunk.data(), chunk.size()) == chunk.size());
                        crc = crc32(chunk.data(), chunk.size(), crc);
                        chunkfile.next();
                    }
                }

                uint64_t fnameLength = strlen(filename.c_str());
                uint64_t fdirLength = strlen(filedir.c_str());
                buffer.write(const_cast<char *>(filename.c_str()), fnameLength);
                buffer.write(const_cast<char *>(filedir.c_str()), fdirLength);

                CBSP_BLOCKER blocker;
                blocker.magic = CBSP_MAGIC;
                blocker.size = sizeof(CBSP_BLOCKER);
                blocker.crc = crc;
                blocker.length = length;
                blocker.fnameLength = fnameLength;
                blocker.fdirLength = fdirLength;
                blocker.pathDigest = crc32(filepath, strlen(filepath));

                lock.lock();
                // seek to the end of the cbsp file
                // trying to write
                std::fseek(fp, 0, SEEK_END);

                // get the FILE offset
                // content offset
                uint64_t offset = std::ftell(fp);

                // fname offset
                uint64_t fnameOffset = offset + length;

                // fdirname offset
                uint64_t fdirOffset = fnameOffset + fnameLength;

                // struct offset
                uint64_t stOffset = fdirOffset + fdirLength;

                blocker.offset = offset;
                blocker.fnameOffset = fnameOffset;
                blocker.fdirOffset = fdirOffset;
                buffer.write(reinterpret_cast<char *>(&blocker), blocker.size);

                cbsp_assert(write(fp, const_cast<char *>(buffer.data()), buffer.length()) == buffer.length());

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
                    throw std::runtime_error(std::string("bad header"));
                }

                lock.unlock();
            }
            std::fclose(file);
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

            ThreadPool::create(32);
            ThreadPool::enqueue(asyncAdd, fp, filepath);
            // asyncAdd(fp, filepath);

            return CBSP_ERR_SUCCESS;
        }
    }
}

#endif