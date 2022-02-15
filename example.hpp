#ifndef _EXAMPLE_H_
#define _EXAMPLE_H_

#include <list>
#include <fstream>
#include <cstdio>

#include "cbsp_combiner.hpp"
#include "cbsp_spliter.hpp"
#include "cbsp_error.hpp"
#include "cbsp_file.hpp"

namespace cbsp
{
    template <typename T>
    inline int combine(const char *target, const T &clist)
    {
        if (clist.empty())
        {
            printError(CBSP_ERR_NO_SOURCE);
            return CBSP_ERR_NO_SOURCE;
        }

        int ret = CBSP_ERR_SUCCESS;
        CBSPFile fp;
        ret = fp.create(target);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
            return ret;
        }

        for (auto &source : clist)
        {
            int _ret = combiner::add(&fp, source);
            if (_ret != CBSP_ERR_SUCCESS)
            {
                printError(_ret);
            }
            ret |= _ret;
        }
        return ret;
    }
    inline int split(const char *target, const char *outdir = nullptr)
    {
        int ret = CBSP_ERR_SUCCESS;
        CBSPFile fp;
        ret = fp.open(target);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
            return ret;
        }

        ret = spliter::extract(&fp, outdir);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
        }

        return ret;
    }
    inline int print(const char *target)
    {
        int ret = CBSP_ERR_SUCCESS;
        CBSPFile fp;
        ret = fp.open(target);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
            return ret;
        }

        ret = spliter::printHeader(&fp);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
        }
        ret = spliter::printBlockers(&fp);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
        }
        ret = spliter::printTree(&fp);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
        }

        return ret;
    }
}

#endif