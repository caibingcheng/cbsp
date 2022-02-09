#ifndef _CBSP_FILE_H_
#define _CBSP_FILE_H_

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "cbsp_structor.hpp"
#include "cbsp_error.hpp"
#include "cbsp_utils.hpp"
#include "cbsp_crc.hpp"

namespace cbsp
{
    class CBSPFile
    {
    public:
        CBSPFile() = default;
        ~CBSPFile()
        {
            close();
        }
        // just open a cbsp file for read
        int open(const char *filename)
        {
            close();
            m_file = std::fopen(filename, "rb");
            if (!m_file)
            {
                return CBSP_ERR_NO_TARGET;
            }
            if (!isCBSP(m_file))
            {
                close();
                return CBSP_ERR_NO_CBSP;
            }
            return CBSP_ERR_SUCCESS;
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

        std::FILE *&operator&()
        {
            return m_file;
        }

        operator bool() const
        {
            return m_file != nullptr;
        }

    private:
        std::FILE *m_file = nullptr;

        void close()
        {
            if (m_file)
            {
                std::fclose(m_file);
                m_file = nullptr;
            }
        }
    };
}

#endif