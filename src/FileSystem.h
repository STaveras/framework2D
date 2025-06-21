// FileSystem
// Access to storage

#pragma once

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include "Types.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#if defined(_WIN32)
#define DIR_SEP "\\"
#include <direct.h>
#else
#define DIR_SEP "/"
#include <unistd.h>
#include <dirent.h>
#endif

namespace FileSystem
{
	static void SetWorkingDirectory(const std::string& path)
	{
#if defined(__APPLE__)
		chdir(path.c_str());
#else
		_chdir(path.c_str());
#endif
	}

	static std::string GetWorkingDirectory(void)
	{
		char buffer[1024];
		getcwd(buffer, 1024);

		if (getcwd(buffer, sizeof(buffer)) != nullptr)
			return std::string(buffer);
		else
			return "";
	}

	// Finds at files at the specified path with the given extension
	static std::vector<std::string> GetFiles(const std::string& path, const std::string& extension)
	{
		std::vector<std::string> files;
#ifndef _WIN32
		DIR* dir = opendir(path.c_str());
		if (dir)
		{
			struct dirent* ent;
			while ((ent = readdir(dir)) != nullptr)
			{
				if (ent->d_type == DT_REG)
				{
					std::string file = ent->d_name;
					if (file.size() >= extension.size() && file.compare(file.size() - extension.size(), extension.size(), extension) == 0) {
						files.push_back(file);
					}
				}
			}
			closedir(dir);
		}
#else
		std::string search_path = path; search_path += std::string(DIR_SEP) + "*" + extension;
		WIN32_FIND_DATA ffd;
		HANDLE handle = FindFirstFile(search_path.c_str(), &ffd);

		if (handle != INVALID_HANDLE_VALUE) {
			do {
				if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					std::string file = ffd.cFileName;
					files.push_back(file);
				}
			} while (FindNextFile(handle, &ffd) != 0);
			FindClose(handle);
		}
#endif
		return files;
	}

	// Lists all files in the specified directory (i.e., files that are not directories) \
	//	and return their names as a vector of strings.
	static std::vector<std::string> ListFiles(const char* path)
	{
		std::vector<std::string> files;
#ifndef _WIN32
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
#else
		std::string search_path = path; search_path += std::string(DIR_SEP) + "*";
		WIN32_FIND_DATA ffd;
		HANDLE handle = FindFirstFile(search_path.c_str(), &ffd);

		if (handle != INVALID_HANDLE_VALUE) {
			do {
				if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					std::string file = ffd.cFileName;
					files.push_back(file);
				}
			} while (FindNextFile(handle, &ffd) != 0);
			FindClose(handle);
		}
#endif
		return files;
	}

	static std::vector<std::string> ListSubdirectories(const char* path)
	{
		std::vector<std::string> files;
#ifndef _WIN32
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
#else
		std::string search_path = path; search_path += std::string(DIR_SEP) + "*";
		WIN32_FIND_DATA ffd;
		HANDLE handle = FindFirstFile(search_path.c_str(), &ffd);

		if (handle != INVALID_HANDLE_VALUE) {
			do {
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					std::string file = ffd.cFileName;
					if (file != "." && file != "..") {
						files.push_back(file);
					}
				}
			} while (FindNextFile(handle, &ffd) != 0);
			FindClose(handle);
		}
#endif
		return files;
	}

	static std::vector<std::string> ListAllSubdirectories(const char* path)
	{
		std::vector<std::string> files;
#ifndef _WIN32
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
						std::string subPath = std::string(path) + DIR_SEP + file;
						std::vector<std::string> subFiles = = ListAllSubdirectories(subPath.c_str());
						for (std::string subFile : subFiles) {
							files.push_back(file + DIR_SEP + subFile);
						}
					}
				}
			}
			closedir(dir);
		}
#else
		std::string search_path = path; search_path += std::string(DIR_SEP) + "*";
		WIN32_FIND_DATA ffd;
		HANDLE handle = FindFirstFile(search_path.c_str(), &ffd);

		if (handle != INVALID_HANDLE_VALUE) {
			do {
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					std::string file = ffd.cFileName;
					if (file != "." && file != "..") {
						files.push_back(file);
						std::string subPath = std::string(path) + DIR_SEP + file;
						std::vector<std::string> subFiles = ListAllSubdirectories(subPath.c_str());
						for (std::string subFile : subFiles) {
							files.push_back(file + DIR_SEP + subFile);
						}
					}
				}
			} while (FindNextFile(handle, &ffd) != 0);
			FindClose(handle);
		}
#endif
		return files;
	}

	static bool FileExists(const std::string& path)
	{
		std::ifstream file(path);
		return file.good();
	}


	static void ListDirectoryContents(const char* path)
	{
		std::string dataDirectoryPath = path;
		std::vector<std::string> subdirectories = FileSystem::ListAllSubdirectories(dataDirectoryPath.c_str());

		for (std::string subdirectory : subdirectories)
		{
			std::string fullPath = dataDirectoryPath + '/' + subdirectory;
			std::cout << fullPath << std::endl;

			ListDirectoryContents(fullPath.c_str());

			std::vector<std::string> files = FileSystem::ListFiles(fullPath.c_str());

			for (std::string file : files) {
				std::cout << "Found file: " << file << std::endl;
			}

			std::cout << std::endl;
		}
	}

	namespace File
	{
		typedef std::fstream Stream;

		// Opens a file ofstream for a given filepath
		static Stream Open(const std::string& filename, bool append = false)
		{
			std::ios_base::openmode modeFlags = std::ios::in | std::ios::out | ((append) ? std::ios::app : 0); // ios::binary	eventually
			return Stream(filename, modeFlags);
		}

		// Reads in 
		static std::vector<char> Read(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			// We should just return an empty vector if the file doesn't exist
			if (!file.is_open()) {
				return std::vector<char>();
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

		static std::string GetFilePath(const std::string& path)
		{
			// Find the last occurrence of the separator character '/'
			size_t pos = path.find_last_of("/");

			// If the separator character was found, return everything before it
			if (pos != std::string::npos) {
				return path.substr(0, pos);
			}

			// If the separator character was not found, return the original string
			return path;
		}
	}
}
#endif