#ifndef _CBSP_MIXER_H_
#define _CBSP_MIXER_H_

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "cbsp_structor.hpp"
#include "cbsp_error.hpp"
#include "cbsp_utils.hpp"

namespace cbsp
{
    inline int mixLinear(uint8_t *data, uint64_t length)
    {
        if (data == nullptr)
        {
            return CBSP_ERR_BAD_OFFSET;
        }

        while (length-- > 0)
        {
            *data = ~*data;
            data++;
        }

        return CBSP_ERR_SUCCESS;
    }

    const int CBSP_MIX_LINEAR = 1;
    inline int mixer(uint8_t *data, uint64_t length, int type)
    {
        switch (type)
        {
        case CBSP_MIX_LINEAR:
            return mixLinear(data, length);
        default:
            return CBSP_ERR_SUCCESS;
        }
        return CBSP_ERR_SUCCESS;
    }
}

#endif