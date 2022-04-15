// FileSystem
// Access to storage

#if __APPLE__
#include <dirent.h>
#endif

#include <string>
#include <vector>

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

namespace FileSystem
{
    std::vector<std::string> GetFiles(const std::string& path, const std::string& extension)
    {
        std::vector<std::string> files;
        DIR* dir = opendir(path.c_str());
        if (dir)
        {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr)
            {
                if (ent->d_type == DT_REG)
                {
                    std::string file = ent->d_name;
                    if (file.find(extension) != std::string::npos)
                    {
                        files.push_back(file);
                    }
                }
            }
            closedir(dir);
        }
        return files;
    }
}

#endif