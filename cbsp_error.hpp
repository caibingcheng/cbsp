#ifndef _CBSP_ERROR_H_
#define _CBSP_ERROR_H_

#include <deque>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>

namespace cbsp
{
    const int32_t CBSP_ERR_BAIS = __LINE__ + 1;
    const int32_t CBSP_ERR_SUCCESS = __LINE__ - CBSP_ERR_BAIS; // 0
    const int32_t CBSP_ERR_CREATE_FAILED = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_NO_CBSP = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_NO_TARGET = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_NO_SOURCE = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_AL_EXIST = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_AL_MODIFY = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_BAD_CBSP = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_BAD_PATH = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_BAD_OFFSET = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);
    const int32_t CBSP_ERR_DEN_ACCESS = 1 << (__LINE__ - CBSP_ERR_BAIS - 1);

    inline std::list<int32_t> extError(int32_t err)
    {
        uint32_t ext = 0x1;
        std::list<int32_t> errs;
        for (int i = 0; i < 31; i++)
        {
            int erro = (err & ext);
            err ^= ext;
            ext <<= 1;

            if (erro != CBSP_ERR_SUCCESS)
            {
                errs.push_back(erro);
            }
        }

        return errs;
    }

    inline const char *strError(int32_t err)
    {
        switch (err)
        {
        case CBSP_ERR_SUCCESS:
            return "Success";
        case CBSP_ERR_NO_CBSP:
            return "Not cbsp";
        case CBSP_ERR_NO_TARGET:
            return "No target";
        case CBSP_ERR_NO_SOURCE:
            return "No source";
        case CBSP_ERR_AL_EXIST:
            return "Already exists";
        case CBSP_ERR_AL_MODIFY:
            return "Already modified";
        case CBSP_ERR_BAD_CBSP:
            return "Broken cbsp";
        case CBSP_ERR_BAD_PATH:
            return "Bad path";
        case CBSP_ERR_BAD_OFFSET:
            return "Bad offset";
        case CBSP_ERR_DEN_ACCESS:
            return "Access denied";
        default:
            return "Unkown";
        }
    }

    class ErrorMessage final
    {
    public:
        static const char *getMessage()
        {
            m_ridx = std::min(++m_ridx, m_wmax);
            return m_msgs[m_ridx - 1].c_str();
        }
        template <typename... ARGS>
        static void setMessage(const char *fmt, ARGS &&...args)
        {
            char msg[1024];
            memset(msg, 0, sizeof(msg));
            sprintf(msg, fmt, args...);
            setMessage(msg);
        }
        static void setMessage(const char *message)
        {
            setMessage(std::string(message));
        }
        static void setMessage(std::string &&msg)
        {
            if (m_widx < m_wmax)
            {
                m_widx++;
            }
            else
            {
                m_msgs.pop_front();
                m_ridx = std::max(--m_ridx, 0);
            }
            m_msgs.push_back(msg);
        }
        static bool hasMessage()
        {
            return m_ridx < m_widx;
        }

    private:
        ErrorMessage() = delete;
        ~ErrorMessage() = default;

    private:
        static std::deque<std::string> m_msgs;
        static int m_ridx;
        static int m_widx;
        static int m_wmax;
    };
    std::deque<std::string> ErrorMessage::m_msgs{};
    int ErrorMessage::m_ridx = 0;
    int ErrorMessage::m_widx = 0;
    int ErrorMessage::m_wmax = 512;

    inline void printError(int32_t err)
    {
        auto errs = extError(err);
        for (auto &e : errs)
        {
            fprintf(stderr, "Error no.%d: %s\n", e, strError(e));
        }
        if (errno != 0)
            fprintf(stderr, "Error no.%d: %s\n", err, strerror(errno));
        while (ErrorMessage::hasMessage())
        {
            fprintf(stderr, "Error no.%d: %s\n", err, ErrorMessage::getMessage());
        }
    }
}
#endif