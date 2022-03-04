#ifndef _CBSP_BUFFER_H_
#define _CBSP_BUFFER_H_

#include <list>
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

    using CBSP_BUFFER_LS = std::list<CBSP_BUFFER>;
    using CBSP_BUFFER_IT = CBSP_BUFFER_LS::iterator;
    using CBSP_BUFFER_PR = std::pair<CBSP_BUFFER, CBSP_BUFFER_IT>;

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
                removeBufferFromFree(it);
                m_buffer = setBufferToBusy(it);
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
                if (!isEmpty())
                {
                    removeBufferFromBusy(m_buffer);
                    setBufferToFree(m_buffer);
                }
                m_buffer = empty();
            }

            CBSP_BUFFER prBuffer(const CBSP_BUFFER_PR &it) const noexcept
            {
                return it.first;
            }

            CBSP_BUFFER_IT prIterator(const CBSP_BUFFER_PR &it) const noexcept
            {
                return it.second;
            }

            CBSP_BUFFER_PR cvtIt2Pr(const CBSP_BUFFER_IT &it) const noexcept
            {
                return {*it, it};
            }

            CBSP_BUFFER_PR empty() noexcept
            {
                return cvtIt2Pr(buffer_empty.begin());
            }

            const bool isEmpty(const CBSP_BUFFER_PR &it) noexcept
            {
                return it.second == empty().second;
            }

            const bool isEmpty() noexcept
            {
                return isEmpty(m_buffer);
            }

            const bool isItIn(const CBSP_BUFFER_IT &it, CBSP_BUFFER_LS &ls) noexcept
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
                bool is_full = (buffer_busy.size() + buffer_free.size()) >= size_full;
                if (is_full)
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

            CBSP_BUFFER_PR setBufferToBusy(const CBSP_BUFFER_PR &it) noexcept
            {
                return setBufferToBusy(prBuffer(it));
            }

            CBSP_BUFFER_PR setBufferToBusy(const CBSP_BUFFER &buffer) noexcept
            {
                buffer_busy.push_front(buffer);
                return cvtIt2Pr(buffer_busy.begin());
            }

            void removeBufferFromBusy(CBSP_BUFFER_PR &it) noexcept
            {
                if (!isEmpty(it) && isItIn(prIterator(it), buffer_busy))
                {
                    buffer_busy.erase(prIterator(it));
                }
            }

            CBSP_BUFFER_PR setBufferToFree(const CBSP_BUFFER_PR &it) noexcept
            {
                return setBufferToFree(prBuffer(it));
            }

            CBSP_BUFFER_PR setBufferToFree(const CBSP_BUFFER &buffer) noexcept
            {
                buffer_free.push_front(buffer);
                return cvtIt2Pr(buffer_free.begin());
            }

            void removeBufferFromFree(const CBSP_BUFFER_PR &it) noexcept
            {
                if (!isEmpty(it) && isItIn(prIterator(it), buffer_free))
                {
                    buffer_free.erase(prIterator(it));
                }
            }

            CBSP_BUFFER_PR getBufferFromFree(const size_t &size) noexcept
            {
                buffer_free.sort([](auto &a, auto &b)
                                 { return a.size < b.size; });

                CBSP_BUFFER_IT target = buffer_free.end();
                for (CBSP_BUFFER_IT cand = buffer_free.begin(); cand != buffer_free.end(); cand++)
                {
                    if (cand->size >= size)
                    {
                        target = cand;
                        break;
                    }
                }

                if (target != buffer_free.end())
                {
                    return cvtIt2Pr(target);
                }

                return empty();
            }

        private:
            CBSP_BUFFER_PR m_buffer;

        private:
            static const size_t size_full = 10;
            static CBSP_BUFFER_LS buffer_free;
            static CBSP_BUFFER_LS buffer_busy;
            static CBSP_BUFFER_LS buffer_empty;
        };

        CBSP_BUFFER_LS Buffer::buffer_free;
        CBSP_BUFFER_LS Buffer::buffer_busy;
        CBSP_BUFFER_LS Buffer::buffer_empty{{nullptr, 0}};
    }
}

#endif