#ifndef _CBSP_BUFFER_H_
#define _CBSP_BUFFER_H_

#include <map>
#include <cstdio>
#include <memory>
#include <cstring>
#include <algorithm>

#include "cbsp_utils.hpp"

namespace cbsp
{
    typedef struct _CBSP_BUFFER_
    {
        std::shared_ptr<char[]> buffer;
        size_t size;
    } CBSP_BUFFER;

    using CBSP_BUFFER_RB = std::multimap<size_t, CBSP_BUFFER>;
    using CBSP_BUFFER_IT = CBSP_BUFFER_RB::iterator;

    namespace
    {
        class Buffer
        {
        public:
            Buffer() : m_buffer(empty()) {}
            Buffer(Buffer &&buffer) { *this = std::move(buffer); }
            Buffer(const size_t &size) noexcept
            {
                auto it = getBufferFromFree(size);
                if (isEmpty(it))
                {
                    auto buffer = generateBuffer(size);
                    it = setBufferToFree(buffer);
                }
                m_buffer = setBufferToBusy(it);
                cbsp_assert(!isEmpty());
                removeBufferFromFree(it);
                memset(this->get(), 0, this->size());
            }
            ~Buffer()
            {
                reset();
            }

            Buffer &operator=(Buffer &&buffer)
            {
                reset();
                std::swap(m_buffer, buffer.m_buffer);
                return *this;
            }

            char &operator[](const size_t &index) noexcept
            {
                cbsp_assert(index >= 0);
                cbsp_assert(index < size());
                return get()[index];
            }

            char *get() const noexcept
            {
                return prBuffer(m_buffer).buffer.get();
            }

            const size_t size() const noexcept
            {
                return prBuffer(m_buffer).size;
            }

            const size_t useCount() const noexcept
            {
                return prBuffer(m_buffer).buffer.use_count();
            }

            static const size_t capacity() noexcept
            {
                return size_full;
            }

            static const size_t capacity(const size_t &size) noexcept
            {
                size_full = size;
                return size_full;
            }

            static const size_t count() noexcept
            {
                return buffer_free.size() + buffer_busy.size();
            }

            static const size_t validCount() noexcept
            {
                return buffer_free.size();
            }

            static const size_t inValidCount() noexcept
            {
                return buffer_busy.size();
            }

            static const void clear() noexcept
            {
                cbsp_assert_msg(buffer_busy.size() == 0, "Busy buffer exists\n");
                buffer_busy.clear();
                buffer_free.clear();
            }

        private:
            void reset() noexcept
            {
                setBufferToFree(m_buffer);
                removeBufferFromBusy(m_buffer);
                m_buffer = empty();
            }

            CBSP_BUFFER &prBuffer(const CBSP_BUFFER_IT &it) const noexcept
            {
                return it->second;
            }

            CBSP_BUFFER_IT empty() noexcept
            {
                return buffer_free.end();
            }

            const bool isEmpty(const CBSP_BUFFER_IT &it) noexcept
            {
                return it == buffer_free.end() || it == buffer_busy.end();
            }

            const bool isEmpty() noexcept
            {
                return isEmpty(m_buffer);
            }

            const bool isItIn(const CBSP_BUFFER_IT &it, CBSP_BUFFER_RB &ls) noexcept
            {
                for (CBSP_BUFFER_IT _it = ls.begin(); _it != ls.end(); _it++)
                {
                    if (it == _it)
                    {
                        return true;
                    }
                }
                return false;
            }

            CBSP_BUFFER generateBuffer(const size_t &size) noexcept
            {
                for (bool is_full = count() >= size_full;
                     is_full && validCount() > 0;
                     is_full = count() >= size_full)
                {
                    auto it = getBufferFromFree(0);
                    if (!isEmpty(it))
                    {
                        removeBufferFromFree(it);
                    }
                }
                CBSP_BUFFER buffer{.buffer = std::shared_ptr<char[]>(new char[size]),
                                   .size = size};
                return buffer;
            }

            CBSP_BUFFER_IT setBufferToBusy(const CBSP_BUFFER_IT &it) noexcept
            {
                if (!isEmpty(it))
                {
                    return setBufferToBusy(prBuffer(it));
                }
                return empty();
            }

            CBSP_BUFFER_IT setBufferToBusy(const CBSP_BUFFER &buffer) noexcept
            {
                return buffer_busy.emplace(std::make_pair(buffer.size, buffer));
            }

            void removeBufferFromBusy(CBSP_BUFFER_IT &it) noexcept
            {
                if (!isEmpty(it) && isItIn(it, buffer_busy))
                {
                    buffer_busy.erase(it);
                }
            }

            CBSP_BUFFER_IT setBufferToFree(const CBSP_BUFFER_IT &it) noexcept
            {
                if (!isEmpty(it))
                {
                    return setBufferToFree(prBuffer(it));
                }
                return empty();
            }

            CBSP_BUFFER_IT setBufferToFree(const CBSP_BUFFER &buffer) noexcept
            {
                return buffer_free.emplace(std::make_pair(buffer.size, buffer));
            }

            void removeBufferFromFree(const CBSP_BUFFER_IT &it) noexcept
            {
                if (!isEmpty(it) && isItIn(it, buffer_free))
                {
                    buffer_free.erase(it);
                }
            }

            CBSP_BUFFER_IT getBufferFromFree(const size_t &size) noexcept
            {
                CBSP_BUFFER_IT target = buffer_free.lower_bound(size);

                if (target != buffer_free.end())
                {
                    return target;
                }

                return buffer_free.end();
            }

        private:
            CBSP_BUFFER_IT m_buffer;

        private:
            static size_t size_full;
            static CBSP_BUFFER_RB buffer_free;
            static CBSP_BUFFER_RB buffer_busy;
        };

        size_t Buffer::size_full = 10;
        CBSP_BUFFER_RB Buffer::buffer_free;
        CBSP_BUFFER_RB Buffer::buffer_busy;
    }
}

#endif