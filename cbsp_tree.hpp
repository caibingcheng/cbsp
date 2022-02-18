#ifndef _CBSP_TREE_H_
#define _CBSP_TREE_H_

#include <fstream>
#include <memory>
#include <string>
#include <stack>
#include <list>
#include <map>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "cbsp_structor.hpp"
#include "cbsp_utils.hpp"

namespace cbsp
{
    inline bool isFile(const char *path)
    {
        struct stat sts;
        bool is_file = false;
        // if path not exist, it's not a file
        if (0 == stat(path, &sts))
        {
            // if not a directory, it's a file
            is_file = !(sts.st_mode & __S_IFDIR);
        }
        return is_file;
    }

    inline bool isDir(const char *path)
    {
        struct stat sts;
        bool is_dir = false;
        // if not exist, not a directory
        if (0 == stat(path, &sts))
        {
            // if has a directory flag, it's a directory
            is_dir = sts.st_mode & __S_IFDIR;
        }
        return is_dir;
    }

    inline std::list<std::string> getDirFiles(const char *path)
    {
        if (!path)
        {
            return std::list<std::string>{};
        }

        // it is a file, return the only file
        if (isFile(path))
        {
            return std::list<std::string>{path};
        }

        // if not a file and not a directory
        if (!isDir(path))
        {
            return std::list<std::string>{};
        }

        DIR *dir = opendir(path);
        // open directory failed
        if (!dir)
        {
            return std::list<std::string>{};
        }

        std::string spath = [&path]
        {
            std::string tpath = path;
            while (tpath.back() == '/')
            {
                tpath.pop_back();
            }
            return tpath + "/";
        }();
        struct dirent *ptr;
        std::list<std::string> filenames;
        while ((ptr = readdir(dir)) != 0)
        {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0)
            {
                std::string cand = spath + ptr->d_name;
                if (isDir(cand.c_str()))
                {
                    auto subfiles = getDirFiles(cand.c_str());
                    if (!subfiles.empty())
                    {
                        filenames.sort();
                        subfiles.sort();
                        filenames.merge(subfiles);
                    }
                }
                else if (isFile(cand.c_str()))
                {
                    filenames.push_back(cand);
                }
            }
        }
        closedir(dir);

        return filenames;
    }

    inline std::string getFileName(std::FILE *&fp, const CBSP_BLOCKER &blocker)
    {
        auto filename = std::make_unique<char[]>(blocker.fnameLength + 1);
        memset(filename.get(), 0, blocker.fnameLength + 1);
        read(fp, filename.get(), blocker.fnameOffset, blocker.fnameLength);

        return std::string(filename.get());
    }

    inline std::string getFileDir(std::FILE *&fp, const CBSP_BLOCKER &blocker)
    {
        auto filedir = std::make_unique<char[]>(blocker.fdirLength + 1);
        memset(filedir.get(), 0, blocker.fdirLength + 1);
        read(fp, filedir.get(), blocker.fdirOffset, blocker.fdirLength);

        return std::string(filedir.get());
    }

    inline std::string fileName(const char *filepath)
    {
        std::string path = std::string(filepath);
        std::string::size_type pos = path.find_last_of('/') + 1;
        std::string filename = path.substr(pos, path.length() - pos);

        return filename;
    }

    inline std::string fileDir(const char *filepath)
    {
        std::string path = std::string(filepath);
        std::string::size_type pos = path.find_last_of('/');
        std::string filedir = path.substr(0, pos);

        return filedir;
    }

    inline std::list<std::string> listDirs(const char *dir)
    {
        std::list<std::string> dlist;
        std::string dirpath = std::string(dir);
        std::string::size_type pos = 0;
        do
        {
            auto ppos = pos;
            pos = dirpath.find_first_of('/', ppos);

            if (pos == std::string::npos)
            {
                pos = dirpath.size();
            }

            auto tdir = dirpath.substr(ppos, pos - ppos);
            if (tdir.size() > 0)
            {
                dlist.push_back(tdir);
            }
            pos += 1;
        } while (pos < dirpath.size());

        return dlist;
    }

    inline std::pair<int, std::string> matchTreeRe(const CBSP_TREE &tree, std::stack<std::string> tstack, std::stack<std::string> dstack)
    {
        std::pair<int, std::string> result{0, ""};
        if (tree.empty())
        {
            while (!tstack.empty() && !dstack.empty())
            {
                if (tstack.top() == dstack.top())
                {
                    result.first++;
                    result.second = dstack.top() + "/" + result.second;
                    tstack.pop();
                    dstack.pop();
                }
                else
                {
                    result.first = 0;
                    result.second = "";
                    break;
                }
            }
            return result;
        }

        for (auto &tr : tree)
        {
            tstack.push(tr.path);
            auto _result = matchTreeRe(tr.children, tstack, dstack);
            tstack.pop();
            if (result.first < _result.first)
                result = _result;
        }
        return result;
    }

    inline std::string matchTree(const CBSP_TREE &tree, const char *dir)
    {
        auto dlist = listDirs(dir);
        std::stack<std::string> dstack;
        for (auto &d : dlist)
        {
            dstack.push(d);
        }
        std::string path;
        std::tie(std::ignore, path) = matchTreeRe(tree, std::stack<std::string>(), dstack);
        cbsp_assert(!path.empty());

        return path.substr(0, path.size() - 1);
    }

    inline void printTreeRe(const CBSP_TREE &tree, std::list<std::string> &path)
    {
        if (tree.empty())
        {
            for (auto &p : path)
            {
                printf("/%s", p.c_str());
            }
            printf("\n");
            return;
        }

        for (auto &tr : tree)
        {
            path.push_back(tr.isFile ? tr.path + "*" : tr.path);
            printTreeRe(tr.children, path);
            path.pop_back();
        }
    }

    inline void printTree(const CBSP_TREE &tree)
    {
        cbsp_assert(!tree.empty());
        std::list<std::string> path;
        printTreeRe(tree, path);

        return;
    }

    inline CBSP_TREE cropTree(CBSP_TREE &tree)
    {
        cbsp_assert(!tree.empty());
        // the nearlest common parrent node
        if (tree.size() > 1)
            return tree;

        // the leaf node
        if (tree.size() == 1 &&
            tree.front().children.empty())
        {
            return tree;
        }

        return cropTree(tree.front().children);
    }

    inline int conTreeRe(const CBSP_TREE &tree, std::string path)
    {
        if (!tree.empty())
        {
            // if not exists, or a directory, it's not a file
            if (isFile(path.c_str()))
            {
                ErrorMessage::setMessage("Target %s is not a directory", path.c_str());
                return CBSP_ERR_AL_EXIST;
            }
            system(std::string("mkdir -p " + path).c_str());
        }
        int result = CBSP_ERR_SUCCESS;
        for (auto &tr : tree)
        {
            result = conTreeRe(tr.children, path + "/" + tr.path);
            if (result != CBSP_ERR_SUCCESS)
            {
                return result;
            }
        }
        return CBSP_ERR_SUCCESS;
    }
    inline int conTree(const CBSP_TREE &tree, const bool &root = false)
    {
        int result = CBSP_ERR_SUCCESS;
        std::string path(root ? "/" : ".");
        for (auto &tr : tree)
        {
            if (!tr.isFile)
            {
                result = conTreeRe(tr.children, path + "/" + tr.path);
                if (result != CBSP_ERR_SUCCESS)
                {
                    return result;
                }
            }
        }
        return CBSP_ERR_SUCCESS;
    }

    inline CBSP_TREE dirTree(std::FILE *&fp)
    {
        if (!isCBSP(fp))
            return CBSP_TREE();

        auto header = getHeader(fp);
        auto blocker = getFirst(fp, header);

        CBSP_TREE tree;

        // if call dirTree, it must be a cbsp file
        cbsp_assert(header.count > 0);
        while (header.count > 0)
        {
            header.count--;

            auto filedir = getFileDir(fp, blocker);
            auto dlist = listDirs(filedir.c_str());
            dlist.push_back(getFileName(fp, blocker));
            cbsp_assert(!dlist.empty());

            auto ptree = &tree;
            size_t i = dlist.size();
            for (auto &dl : dlist)
            {
                bool isFile = (--i == 0);
                if (ptree->empty())
                {
                    ptree->push_back({.isFile = isFile,
                                      .path = dl,
                                      .children = {}});
                    ptree = &(ptree->front().children);
                    continue;
                }
                else
                {
                    bool newpath = true;
                    for (auto &tr : *ptree)
                    {
                        if (tr.path == dl)
                        {
                            ptree = &(tr.children);
                            newpath = false;
                            break;
                        }
                    }
                    if (newpath)
                    {
                        ptree->push_back({.isFile = isFile,
                                          .path = dl,
                                          .children = {}});
                        ptree = &(ptree->back().children);
                    }
                }
            }

            blocker = getCBSPBlocker(fp, blocker.next);
        }

        cbsp_assert(!tree.empty());
        return tree;
    }
}
#endif