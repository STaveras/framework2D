// FileSystem
// Access to storage

#pragma once

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#if __APPLE__
#include <dirent.h>
#include <unistd.h>
#endif

#include <string>
#include <vector>

namespace FileSystem
{
    static std::string GetWorkingDirectory(void)
    {
        char buffer[1024];
        getcwd(buffer, 1024);
        return std::string(buffer);
    }

    static std::vector<std::string> GetFiles(const std::string& path, const std::string& extension)
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
    
    static std::vector<std::string> ListFiles(const char *path) 
    {
        std::vector<std::string> files;
        DIR* dir = opendir(path);
        if (dir)
        {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr)
            {
                if (ent->d_type == DT_REG)
                {
                    std::string file = ent->d_name;
                    files.push_back(file);
                }
            }
            closedir(dir);
        }
        return files;        
    }

    static std::vector<std::string> ListSubdirectories(const char *path)
    {
        std::vector<std::string> files;
        DIR* dir = opendir(path);
        if (dir)
        {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr)
            {
                if (ent->d_type == DT_DIR)
                {
                    std::string file = ent->d_name;
                    if (file != "." && file != "..")
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