#ifndef _CBSP_BUFFER_H_
#define _CBSP_BUFFER_H_

#include <map>
#include <cstdio>
#include <memory>
#include <cstring>
#include <algorithm>
#include <type_traits>

#include "cbsp_utils.hpp"

namespace cbsp
{
    typedef struct _CBSP_BUFFER_
    {
        std::shared_ptr<char[]> buffer;
        size_t size;
        size_t wpos;
    } CBSP_BUFFER;

    using CBSP_BUFFER_RB = std::multimap<size_t, CBSP_BUFFER>;
    using CBSP_BUFFER_IT = CBSP_BUFFER_RB::iterator;

    namespace
    {
        class Buffer
        {
        public:
            Buffer() : m_buffer(empty()), m_rbuffer(null()) {}
            Buffer(const Buffer &buffer) = delete;
            Buffer(const size_t &size) noexcept
            {
                {
                    std::lock_guard<std::mutex> lk(locker);
                    auto it = getBufferFromFree(size);
                    if (isEmpty(it))
                    {
                        auto buffer = generateBuffer(size);
                        it = setBufferToFree(buffer);
                    }
                    m_rbuffer = extract(it);
                    m_buffer = setBufferToBusy(m_rbuffer);
                    removeBufferFromFree(it);
                    cbsp_assert_msg(m_buffer->second.buffer.get() != nullptr, "null pointer\n");
                }

                cbsp_assert(!isEmpty());
                bufferInit();
            }
            ~Buffer()
            {
                std::lock_guard<std::mutex> lk(locker);
                reset();
            }

            Buffer &operator=(const Buffer &buffer)
            {
                *this = std::forward<Buffer>(const_cast<Buffer &>(buffer));
                return *this;
            }
            Buffer &operator=(Buffer &&buffer)
            {
                {
                    std::lock_guard<std::mutex> lk(locker);
                    reset();
                }
                swap(std::move(buffer));
                return *this;
            }
            char &operator[](const size_t &index) noexcept
            {
                cbsp_assert(index >= 0);
                cbsp_assert(index < size());
                return get()[index];
            }

            template <typename T>
            typename std::enable_if<!std::is_pointer<T>::value, size_t>::type
            write(const T &data, size_t length)
            {
                bool fit = (length + this->length()) <= this->size();
                if (!fit)
                {
                    length = length - (length + this->length() - this->size());
                }

                char *p = get() + this->length();
                std::memset(p, static_cast<uint8_t>(data), length);
                m_rbuffer.wpos = this->length() + length;

                return length;
            }

            size_t write(const void *data, size_t length)
            {
                bool fit = (length + this->length()) <= this->size();
                if (!fit)
                {
                    length = length - (length + this->length() - this->size()) - 10;
                }

                cbsp_assert(this->length() + length < this->size());
                char *p = &(this->operator[](this->length()));
                char *q = const_cast<char *>(reinterpret_cast<const char *>(data));
                m_rbuffer.wpos += length;

                size_t count = length;
                while (count-- > 0)
                {
                    try
                    {
                        //case 0 ... failed
                        *p = *q;

                        //case 1 ... ok
                        // *p = 'x';

                        //case 2 ... ok
                        // *q = 'x';
                        // *p = *q;
                    }
                    catch(std::exception &e)
                    {
                        fprintf(stderr, "CBC TEST %s\n", e.what());
                    }
                    p++;
                    q++;
                }

                return length;
            }

            const size_t length() const noexcept
            {
                return m_rbuffer.wpos;
            }

            const char *data() const noexcept
            {
                return reinterpret_cast<const char *>(get());
            }

            char *get() const noexcept
            {
                return m_rbuffer.buffer.get();
            }

            const size_t size() const noexcept
            {
                return m_rbuffer.size;
            }

            const size_t useCount() const noexcept
            {
                return m_rbuffer.buffer.use_count();
            }

            static const size_t capacity() noexcept
            {
                std::lock_guard<std::mutex> lk(locker);
                return size_full;
            }

            static const size_t capacity(const size_t &size) noexcept
            {
                std::lock_guard<std::mutex> lk(locker);
                size_full = size;
                return size_full;
            }

            static const size_t count() noexcept
            {
                std::lock_guard<std::mutex> lk(locker);
                return buffer_free.size() + buffer_busy.size();
            }

            static const size_t validCount() noexcept
            {
                std::lock_guard<std::mutex> lk(locker);
                return buffer_free.size();
            }

            static const size_t inValidCount() noexcept
            {
                std::lock_guard<std::mutex> lk(locker);
                return buffer_busy.size();
            }

            static const void clear() noexcept
            {
                std::lock_guard<std::mutex> lk(locker);
                cbsp_assert_msg(buffer_busy.size() == 0, "Busy buffer exists\n");
                buffer_busy.clear();
                buffer_free.clear();
            }

        private:
            void reset() noexcept
            {
                cbsp_assert(isEmpty(m_rbuffer) == isEmpty(m_buffer));
                if (!isEmpty(m_rbuffer) && !isEmpty(m_buffer))
                {
                    setBufferToFree(m_rbuffer);
                    removeBufferFromBusy(m_buffer);
                }
                m_buffer = empty();
                m_rbuffer = null();
            }

            void bufferInit() noexcept
            {
                cbsp_assert(get() != nullptr);
                m_rbuffer.wpos = 0;
                memset(get(), 0, size());
            }

            void swap(Buffer &&buffer) noexcept
            {
                std::swap(buffer.m_buffer, m_buffer);
                std::swap(buffer.m_rbuffer, m_rbuffer);
            }

            CBSP_BUFFER &extract(const CBSP_BUFFER_IT &it) const noexcept
            {
                return it->second;
            }

            CBSP_BUFFER_IT empty() noexcept
            {
                return buffer_free.end();
            }

            CBSP_BUFFER null() noexcept
            {
                static CBSP_BUFFER buffer{
                    .buffer = nullptr,
                    .size = 0,
                    .wpos = 0,
                };
                return buffer;
            }

            const bool isEmpty(const CBSP_BUFFER_IT &it) noexcept
            {
                return it == buffer_free.end() || it == buffer_busy.end();
            }

            const bool isEmpty(const CBSP_BUFFER &buffer) noexcept
            {
                return buffer.buffer.get() == nullptr;
            }

            const bool isEmpty() noexcept
            {
                return isEmpty(m_buffer);
            }

            const bool isItIn(const CBSP_BUFFER_IT &it, CBSP_BUFFER_RB &ls) noexcept
            {
                for (auto _it = ls.begin(); _it != ls.end(); _it++)
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
                for (bool is_full = (buffer_free.size() + buffer_busy.size()) >= size_full;
                     is_full && buffer_free.size() > 0;
                     is_full = (buffer_free.size() + buffer_busy.size()) >= size_full)
                {
                    auto it = getBufferFromFree(0);
                    if (!isEmpty(it))
                    {
                        removeBufferFromFree(it);
                    }
                }
                CBSP_BUFFER buffer{
                    .buffer = std::shared_ptr<char[]>(new char[size]),
                    .size = size,
                    .wpos = 0,
                };

                return buffer;
            }

            CBSP_BUFFER_IT setBufferToBusy(const CBSP_BUFFER_IT &it) noexcept
            {
                if (!isEmpty(it))
                {
                    return setBufferToBusy(extract(it));
                }
                return empty();
            }

            CBSP_BUFFER_IT setBufferToBusy(const CBSP_BUFFER &buffer) noexcept
            {
                cbsp_assert(!isEmpty(buffer));
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
                    return setBufferToFree(extract(it));
                }
                return empty();
            }

            CBSP_BUFFER_IT setBufferToFree(const CBSP_BUFFER &buffer) noexcept
            {
                cbsp_assert(!isEmpty(buffer));
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
            CBSP_BUFFER m_rbuffer;

        private:
            static size_t size_full;
            static CBSP_BUFFER_RB buffer_free;
            static CBSP_BUFFER_RB buffer_busy;
            static std::mutex locker;
        };

        size_t Buffer::size_full = 10;
        CBSP_BUFFER_RB Buffer::buffer_free{};
        CBSP_BUFFER_RB Buffer::buffer_busy{};
        std::mutex Buffer::locker;
    }
}

#endif