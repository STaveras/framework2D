// FileSystem
// Access to storage

#pragma once

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#if __APPLE__
#include <dirent.h>
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
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

    static std::vector<std::string> ListAllSubdirectories(const char *path) 
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
                        std::vector<std::string> subfiles = ListAllSubdirectories((path + file + "/").c_str());
                        for (std::string subfile : subfiles) {
                            files.push_back(file + "/" + subfile);
                        }
                    }
                }
            }
            closedir(dir);
        }
        return files;
    }

    static bool FileExists(const std::string& path)
    {
        std::ifstream file(path);
        return file.good();
    }


    static void ScoutDirectory(const char *path)
	{
        std::cout << "Working directory: " << FileSystem::GetWorkingDirectory() << std::endl;

        std::string dataDirectoryPath = path;
        std::vector<std::string> subdirectories = FileSystem::ListAllSubdirectories(dataDirectoryPath.c_str());

        for (std::string subdirectory : subdirectories)
        {
            std::cout << dataDirectoryPath + subdirectory << std::endl;
            std::vector<std::string> files = FileSystem::ListFiles((dataDirectoryPath + subdirectory).c_str());

            for (std::string file: files) {
                std::cout << "Found file: " << file << std::endl;
            }

            std::cout << std::endl;
        }
    }

    namespace File
    {
        // Reads in 
        static std::vector<char> Read(const std::string &filename)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }

        static const char* GetFileExtension(const char* szFilepath)
        {
            size_t uiFilepathLength = strlen(szFilepath);

            const char* szExtension = szFilepath + uiFilepathLength;

            while (*(--szExtension) != '.' && --uiFilepathLength)
                continue;

            return szExtension;
        }
    }
}
#endif