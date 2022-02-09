#ifndef _CBSP_BUFFER_H_
#define _CBSP_BUFFER_H_

#include <list>
#include <cstdio>
#include <memory>
#include <cstring>

namespace cbsp
{
    class Buffer final
    {
    public:
    private:
        Buffer() = delete;
        ~Buffer() = default;
        static const int m_size = 10;
        std::list<std::shared_ptr<uint8_t[]>> m_free;
        std::list<std::shared_ptr<uint8_t[]>> m_busy;
    };
}

#endif