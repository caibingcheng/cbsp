#include <list>
#include <fstream>
#include <cstdio>
#include <cstring>

#include "cbsp_combiner.hpp"
#include "cbsp_spliter.hpp"
#include "cbsp_thread.hpp"
#include "cbsp_error.hpp"
#include "cbsp_file.hpp"
#include "cbsp_tree.hpp"

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

        auto add = [&fp](const char *source)
        {
            int ret = combiner::add(&fp, source);
            if (ret != CBSP_ERR_SUCCESS)
            {
                printError(ret);
            }
            return ret;
        };

        for (auto &source : clist)
        {
            if (isDir(source))
            {
                auto files = getDirFiles(source);
                for (auto &file : files)
                {
                    ret |= add(file.c_str());
                }
            }
            else
            {
                ret |= add(source);
            }
        }
        ThreadPool::wait();
        fprintf(stderr, "????\n");
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
        // ret = spliter::printBlockers(&fp);
        // if (ret != CBSP_ERR_SUCCESS)
        // {
        //     printError(ret);
        // }
        ret = spliter::printTree(&fp);
        if (ret != CBSP_ERR_SUCCESS)
        {
            printError(ret);
        }

        return ret;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    auto combine = [&argc, &argv](int start)
    {
        char *target = argv[start];
        std::list<const char *> sources;
        for (int i = start + 1; i < argc; i++)
        {
            sources.push_back(argv[i]);
        }
        cbsp::combine(target, sources);
    };

    auto split = [&argc, &argv](int start)
    {
        char *target = argv[start];
        cbsp::split(target, (argc > 3) ? argv[3] : "");
    };

    auto print = [&argc, &argv](int start)
    {
        char *target = argv[start];
        cbsp::print(target);
    };

    if (strcmp(argv[1], "-c") == 0)
    {
        combine(2);
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        split(2);
    }
    else if (strcmp(argv[1], "-p") == 0)
    {
        print(2);
    }

    return 0;
}