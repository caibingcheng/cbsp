#ifndef _CBSP_STRUCTOR_H_
#define _CBSP_STRUCTOR_H_

#include <list>
#include <string>

#include <cstdint>
#include <cstdio>

// keep this first if a struct will be wrote to cbsp file
#define __F_CBSP__      \
    uint32_t magic = 0; \
    uint32_t size = 0;

// cbsp basic property
// keep this first!!!
#define __CBSP_BASE__   \
    __F_CBSP__          \
    uint32_t type = 0;  \
    uint32_t mixer = 0; \
    uint32_t crc = 0;

namespace cbsp
{
    /*
     * this structure is the header of cbsp file
     * while splite a cbsp file, read the structure first
     */
    typedef struct _CBSP_HEADER
    {
        __CBSP_BASE__

        // cbsp version, not header version
        // A|B|C|D
        // Year|Month|Day|miniVersion
        union _version
        {
            uint32_t version;
            struct
            {
                uint8_t year;
                uint8_t month;
                uint8_t day;
                uint8_t mini;
            };
            _version() : year(22), month(2), day(10), mini(0) {}
        } version;

        // combined content offset
        uint64_t offset = 0;
        // combined content length
        uint64_t length = 0;

        // combined files count
        uint32_t count = 0;

        // first subfile
        uint64_t first = 0;
        // last subfile
        uint64_t last = 0;
    } CBSP_HEADER;

    void print(const _CBSP_HEADER &header)
    {
        printf("*****************HEADER*******************\n");
        printf("size   : %u\n", header.size);
        printf("magic  : 0x%x\n", header.magic);
        printf("type   : %u\n", header.type);
        printf("mixer  : %u\n", header.mixer);
        printf("crc    : 0x%x\n", header.crc);
        printf("version: %u.%u.%u.%u\n", header.version.year, header.version.month, header.version.day, header.version.mini);
        printf("offset : %lu\n", header.offset);
        printf("length : %lu\n", header.length);
        printf("count  : %u\n", header.count);
        printf("first  : %lu\n", header.first);
        printf("last   : %lu\n", header.last);
        printf("******************************************\n");
    }

    /*
     * this structure is the header of every sub-file in cbsp file
     */
    typedef struct _CBSP_BLOCKER
    {
        __CBSP_BASE__

        // file content offset
        uint64_t offset = 0;
        // file content length
        uint64_t length = 0;
        // file name offset
        uint64_t fnameOffset = 0;
        // file name length
        uint64_t fnameLength = 0;
        // file dir offset
        uint64_t fdirOffset = 0;
        // file dir length
        uint64_t fdirLength = 0;

        // next cbsp blocker offset
        uint64_t next = 0;

        // appendency content
        // if update file, the content length may longer than the formar
        uint64_t append = 0;
    } CBSP_BLOCKER;

    void print(const _CBSP_BLOCKER &blocker)
    {
        printf("*****************BLOCKER******************\n");
        printf("size       : %u\n", blocker.size);
        printf("magic      : 0x%x\n", blocker.magic);
        printf("type       : %u\n", blocker.type);
        printf("mixer      : %u\n", blocker.mixer);
        printf("crc        : 0x%x\n", blocker.crc);
        printf("offset     : %lu\n", blocker.offset);
        printf("length     : %lu\n", blocker.length);
        printf("fnameOffset: %lu\n", blocker.fnameOffset);
        printf("fnameLength: %lu\n", blocker.fnameLength);
        printf("fdirOffset : %lu\n", blocker.fdirOffset);
        printf("fdirLength : %lu\n", blocker.fdirLength);
        printf("next       : %lu\n", blocker.next);
        printf("append     : %lu\n", blocker.append);
        printf("******************************************\n");
    }

    typedef struct _CBSP_BLOCKER_APPEND
    {
        __F_CBSP__
        // appendency content offset
        uint64_t offset = 0;
        // appendency content length
        uint64_t length = 0;
        // next appendency
        uint64_t next = 0;
    } CBSP_BLOCKER_APPEND;

    void print(const _CBSP_BLOCKER_APPEND &append)
    {
        printf("*****************APPEND ******************\n");
        printf("size       : %u\n", append.size);
        printf("magic      : 0x%x\n", append.magic);
        printf("offset     : %lu\n", append.offset);
        printf("length     : %lu\n", append.length);
        printf("next       : %lu\n", append.next);
        printf("******************************************\n");
    }

    struct _CBSP_TREE
    {
        bool isFile = false;
        std::string path = "";
        std::list<_CBSP_TREE> children;
    };
    using CBSP_TREE = std::list<_CBSP_TREE>;
}

#endif