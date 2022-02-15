#include "example.hpp"

#include <cstring>

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