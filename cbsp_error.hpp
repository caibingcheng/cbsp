#ifndef _CBSP_ERROR_H_
#define _CBSP_ERROR_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>

namespace cbsp
{
    const int32_t CBSP_ERR_BAIS = __LINE__ + 1;
    const int32_t CBSP_ERR_SUCCESS = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_CREATE_FAILED = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_NO_CBSP = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_NO_TARGET = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_NO_SOURCE = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_AL_EXIST = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_AL_MODIFY = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_BAD_CBSP = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_BAD_PATH = __LINE__ - CBSP_ERR_BAIS;
    const int32_t CBSP_ERR_BAD_OFFSET = __LINE__ - CBSP_ERR_BAIS;

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
        default:
            return "Unkown";
        }
    }

    class ErrorMessage final
    {
    public:
        static const char *getMessage()
        {
            m_ready = false;
            return m_msg.c_str();
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
            m_ready = true;
            m_msg = msg;
        }
        static bool hasMessage()
        {
            return m_ready;
        }

    private:
        ErrorMessage() = delete;
        ~ErrorMessage() = default;

    private:
        static std::string m_msg;
        static bool m_ready;
    };
    std::string ErrorMessage::m_msg = std::string();
    bool ErrorMessage::m_ready = false;

    inline void printError(int32_t err)
    {
        fprintf(stderr, "Error no.%d: %s\n", err, strError(err));
        if (errno != 0)
            fprintf(stderr, "Error no.%d: %s\n", err, strerror(errno));
        if (ErrorMessage::hasMessage())
        {
            fprintf(stderr, "Error no.%d: %s\n", err, ErrorMessage::getMessage());
        }
    }
}
#endif