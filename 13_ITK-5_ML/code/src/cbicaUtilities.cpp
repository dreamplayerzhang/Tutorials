/**
\file  cbicaUtilities.cpp

\brief Some basic utility functions.

Dependecies: OpenMP

https://www.cbica.upenn.edu/sbia/software/ <br>
software@cbica.upenn.edu

\author Sarthak Pati

Copyright (c) 2015 University of Pennsylvania. All rights reserved. <br>
See COPYING file or https://www.cbica.upenn.edu/sbia/software/license.html

*/
#if (_WIN32)
  #define NOMINMAX
	#include <direct.h>
  #include <windows.h>
  #include <conio.h>
  #include <lmcons.h>
  #include <Shlobj.h>
  #include <filesystem>
  #define GetCurrentDir _getcwd
  bool WindowsDetected = true;
  static const char  cSeparator  = '\\';
//  static const char* cSeparators = "\\/";
#else
  #include <dirent.h>
  #include <unistd.h>
  #include <libgen.h>
  #include <limits.h>
  #include <cstring>
  #include <cstdlib>
  #include <sys/types.h>
  #include <errno.h>
  #include <ftw.h>
  #define GetCurrentDir getcwd
  bool WindowsDetected = false;
  static const char  cSeparator  = '/';
//  static const char* cSeparators = "/";
#endif

#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <exception>
#include <typeinfo>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <omp.h>

#include "cbicaUtilities.h"

namespace cbica
{
  //====================================== Folder stuff ====================================//

  bool fileExists( const std::string &fName )
  {
    std::ifstream file_exists(fName.c_str());
    if( file_exists.good() )
      return true;
    else
      return false;
  }
  
  bool directoryExists( const std::string &dName )
  {
    struct stat info;
    std::string dName_Wrap = dName;

    if (dName_Wrap[dName_Wrap.length() - 1] == '/')
    {
      dName_Wrap.erase(dName_Wrap.end() - 1);
    }
    
    if (stat(dName_Wrap.c_str(), &info) != 0)
      return false;
    else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on windows
      return true;
    else
      return false;
  }
  
  bool isFile(const std::string &path)
  {
    return cbica::fileExists(path);
  }

  bool isDir(const std::string &path)
  {
    return cbica::directoryExists(path);
  }

  bool exists(const std::string &path)
  {
    struct stat info;
    
    if( stat( path.c_str(), &info ) != 0 )
        return false;
    else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
        return true;
    else
        return true;
  }

  std::string createTmpDir()
  {
    std::string returnDir = "", tempCheck;
    char *tmp;
    char tempPath[FILENAME_MAX];
#if defined(_WIN32)
    //size_t size;
    tmp = getenv("USERPROFILE");
    //getenv_s(&size, tmp, size, "USERPROFILE"); // does not work, for some reason - needs to be tested
    std::string temp = cbica::replaceString(tmp, "\\", "/");
    sprintf_s(tempPath, static_cast<size_t>(FILENAME_MAX), "%s", temp.c_str());
    strcat_s(tempPath, "/tmp");
#else
    tmp = std::getenv("HOME");
    sprintf(tempPath, "%s", tmp);
    strcat(tempPath, "/tmp");
#endif    

    tempCheck = std::string(tempPath);
    tmp[0] = '\0';
    tempPath[0] = '\0';
    if (isDir(tempCheck))
    {
      for (size_t i = 1; i <= FILENAME_MAX; i++)
      {
        returnDir = tempCheck + std::to_string(i);
        if (!isDir(returnDir))
        {
          break;
        }
      }
    }
    else
    {
      returnDir = tempCheck;
    }

    returnDir += "/";
    if (!createDir(returnDir))
    {
      std::cerr << "Could not create the temporary directory '" << returnDir << "'\n";
      exit(EXIT_FAILURE);
    }

    return returnDir;
  }
  
  std::string createTemporaryDirectory()
  {
    return createTmpDir();
  }

  std::string makeTemporaryDirectory()
  {
    return createTmpDir();
  }

  std::string makeTempDir()
  {
    return createTmpDir();
  }

  bool createDir( const std::string &dir_name )
  {
      //! Pure c++ based directory creation
    #if defined(_WIN32)
      DWORD ftyp = GetFileAttributesA(dir_name.c_str()); // check if directory exists or not
      if (ftyp == INVALID_FILE_ATTRIBUTES)
    	  _mkdir(dir_name.c_str());
      return true;
    #else
      DIR *pDir;
      pDir = opendir(dir_name.c_str()); // check if directory exists or not
      if (pDir == NULL)
    	  mkdir(dir_name.c_str(), 0777);
      return true;
    #endif
    return false;
  }

  bool makeDir(const std::string &dir_name)
  {
    return createDir( dir_name );
  }

  bool createDirectory(const std::string &dir_name)
  {
    return createDir(dir_name);
  }

  bool makeDirectory(const std::string &dir_name)
  {
    return createDir(dir_name);
  }

  bool createFolder(const std::string &dir_name)
  {
    return createDir(dir_name);
  }

  int removeDirectoryRecursively(const std::string &dirname, bool bDeleteSubdirectories = true)
  {
    if (!directoryExists(dirname))
    {
      std::cerr << "Supplied directory name wasn't found.\n";
      exit(EXIT_FAILURE);
    }
    #if defined(_WIN32)
      bool bSubdirectory = false;       // Flag, indicating whether
                                        // subdirectories have been found
      HANDLE hFile;                     // Handle to directory
      std::string strFilePath;          // Filepath
      std::string strPattern;           // Pattern
      WIN32_FIND_DATA FileInformation;  // File information    
    
      strPattern = dirname + "/*.*";
      hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
      if(hFile != INVALID_HANDLE_VALUE)
      {
        do
        {
          if(FileInformation.cFileName[0] != '.')
          {
            strFilePath.erase();
            strFilePath = dirname + "/" + FileInformation.cFileName;
    
            if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
              if(bDeleteSubdirectories)
              {
                // Delete subdirectory
                int iRC = cbica::removeDirectoryRecursively(strFilePath, bDeleteSubdirectories);
                if(iRC)
                  return iRC;
              }
              else
                bSubdirectory = true;
            }
            else
            {
              // Set file attributes
              if(::SetFileAttributes(strFilePath.c_str(),
                                     FILE_ATTRIBUTE_NORMAL) == FALSE)
                return ::GetLastError();
    
              // Delete file
              if(::DeleteFile(strFilePath.c_str()) == FALSE)
                return ::GetLastError();
            }
          }
        } while(::FindNextFile(hFile, &FileInformation) == TRUE);
    
        // Close handle
        ::FindClose(hFile);
    
        DWORD dwError = ::GetLastError();
        if(dwError != ERROR_NO_MORE_FILES)
          return dwError;
        else
        {
          if(!bSubdirectory)
          {
            // Set directory attributes
            if(::SetFileAttributes(dirname.c_str(),
                                   FILE_ATTRIBUTE_NORMAL) == FALSE)
              return ::GetLastError();
    
            // Delete directory
            if(::RemoveDirectory(dirname.c_str()) == FALSE)
              return ::GetLastError();
          }
        }
      }
    
      return 0;
    #else   
    std::string passString = "rm -r " + dirname;
    system(passString.c_str());
    #endif
      return 0;
  }

  bool removeDir(const std::string &path)
  {
    if( removeDirectoryRecursively(path, true) != 0 )
      return false;
    return true;
  }
  
  bool deleteDir(const std::string &path)
  {
    if( removeDirectoryRecursively(path, true) != 0 )
      return false;
    return true;
    //return removeDirectoryRecursively(path);
    /*#if defined(_WIN32)
    if (_rmdir(path.c_str()) == -1)
      return false;
    #else
    std::string passString = "rm -r " + path;
    system(passString.c_str());
    #endif*/

    return true;
  }

  bool copyDir(const std::string &inputFolder, const std::string &destination, bool recursion)
  {
    if (cbica::isDir(inputFolder))
    {
      if (!cbica::isDir(destination))
      {
        cbica::createDir(destination);
      }
#ifdef _WIN32 // do intelligent thing for Windows 
      WIN32_FIND_DATA FindFileData;
      HANDLE hFind;
      const size_t MAX_FILE_SIZE = 1025;

      std::string source_tmp = inputFolder;
      // ensure that all folders end with "/"
      if (source_tmp[source_tmp.length() - 1] != '/')
      {
        source_tmp.append("/");
      }
      
      // check for all files inside 
      source_tmp.append("*");

      hFind = FindFirstFile(source_tmp.c_str(), &FindFileData);
      if (hFind == NULL)
      {
        return false;
      }
      do
      {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          if (strcmp(FindFileData.cFileName, "."))
          {
            if (strcmp(FindFileData.cFileName, ".."))
            {
              std::string newSource = inputFolder + std::string(FindFileData.cFileName) + "/",
                newDest = destination + std::string(FindFileData.cFileName) + "/";
              createDir(newDest);
              //CreateDirectory(newDest.c_str(), NULL);
              copyDir(newSource, newDest);
            }
          }
        }
        else
        {
          std::string sourceFile = inputFolder + std::string(FindFileData.cFileName), destinationFile = destination + std::string(FindFileData.cFileName);
          copyFile(sourceFile, destinationFile);
          //BOOL l_bRet = CopyFile(sourceFile.c_str(), destinationFile.c_str(), TRUE);
        }

      } while (FindNextFile(hFind, &FindFileData));
      FindClose(hFind);

#else // initiate system call for Linux
      //if (recursion)
      //{
      //  recur = "-r ";
      //}
      //system(std::string("cp " + recur + " " + inputFolder + " " + destination).c_str());

      DIR *dir = opendir(inputFolder.c_str());                //Assuming absolute pathname here.
      if (dir) 
      {
        char Path[256], *EndPtr = Path;
        struct dirent *e;
        strcpy(Path, inputFolder.c_str());                  //Copies the current path to the 'Path' variable.
        EndPtr += strlen(inputFolder.c_str());              //Moves the EndPtr to the ending position.
        while ((e = readdir(dir)) != NULL) //Iterates through the entire directory
        {  
          struct stat info;                //Helps us know about stuff
          strcpy(EndPtr, e->d_name);       //Copies the current filename to the end of the path, overwriting it with each loop.
          if (!stat(Path, &info)) //stat returns zero on success.
          {         
            if (S_ISDIR(info.st_mode)) //Are we dealing with a directory?
            {  
              //Make corresponding directory in the target folder here.
              copyDir(Path, EndPtr);   //Calls this function AGAIN, this time with the sub-name.
            }
            else if (S_ISREG(info.st_mode)) //Or did we find a regular file?
            { 
              copyFile(Path, EndPtr);
            }
          }
        }
      }
#endif

      if (cbica::isDir(destination))
      {
        return true;
      }
      else
      {
        std::cerr << "Something went wrong when trying to do the copy. Ensure you have write access to destination.\n";
        return false;
      }
    }
    else
    {
      std::cerr << "The input folder '" << inputFolder << "' cannot be verified as a folder.\n";
      return false;
    }
  }

  bool copyDirectory(const std::string &inputFolder, const std::string &destination, bool recursion)
  {
    return copyDir(inputFolder, destination, recursion);
  }

  bool copyFolder(const std::string &inputFolder, const std::string &destination, bool recursion)
  {
    return copyDir(inputFolder, destination, recursion);
  }

  bool copyFile(const std::string &inputFile, const std::string &destination)
  {
    if (cbica::fileExists(inputFile))
    {
      std::ifstream src(inputFile.c_str(), std::ios::binary);
      std::ofstream dst(destination.c_str(), std::ios::binary);

      dst << src.rdbuf();

      if (cbica::fileExists(destination))
      {
        return true;
      }
      else
      {
        std::cerr << "Something went wrong when trying to do the copy. Ensure you have write access to destination.\n";
        return false;
      }
    }
    else
    {
      std::cerr << "The input file '" << inputFile << "' cannot be verified as a file.\n";
      return false;
    }
  }

  size_t getFileSize(const std::string &inputFile)
  {
    std::streampos begin, end;
    std::ifstream myfile(inputFile.c_str(), std::ios::binary);
    begin = myfile.tellg();
    myfile.seekg(0, std::ios::end);
    end = myfile.tellg();
    myfile.close();
    
    return (end - begin);

    /* // if using filesystem
    std::tr2::sys::path folderPath(inputFile);
    return file_size(filePath);
    */
  }

  size_t getFolderSize(const std::string &rootFolder)
  {
    size_t f_size = 0;
#ifdef _WIN32
    std::tr2::sys::path folderPath(rootFolder);
    if (exists(folderPath))
    {
      std::tr2::sys::directory_iterator end_itr;
      for (std::tr2::sys::directory_iterator dirIte(rootFolder); dirIte != end_itr; ++dirIte)
      {
        std::tr2::sys::path filePath(complete(dirIte->path(), folderPath));
        if (!is_directory(dirIte->status()))
        {
          f_size += file_size(filePath);
        }
        else
        {
          f_size += getFolderSize(filePath);
        }
      }
    }
    else
    {
      std::cerr << "Folder not found.\n";
      exit(EXIT_FAILURE);
    }
#else
    DIR *d;
    struct dirent *de;
    struct stat buf;
    int exists;

    d = opendir(".");
    if (d == NULL) 
    {
      perror("prsize");
      exit(1);
    }

    f_size = 0;

    for (de = readdir(d); de != NULL; de = readdir(d)) 
    {
      exists = stat(de->d_name, &buf);
      if (exists < 0) 
      {
        fprintf(stderr, "Couldn't stat %s\n", de->d_name);
      }
      else 
      {
        f_size += buf.st_size;
      }
    }
    closedir(d);
#endif
    return f_size;
  }

  size_t getDirSize(const std::string &rootFolder)
  {
    return getFolderSize(rootFolder);
  }

  size_t getDirectorySize(const std::string &rootFolder)
  {
    return getFolderSize(rootFolder);
  }

  //======================================== OS stuff ======================================//
  
  std::string getFilenameBase(const std::string &filename, bool checkFile)
  {
    if (checkFile)
    {
      if (!fileExists(filename))
      {
        std::cerr << "Supplied file name wasn't found.\n";
        exit(EXIT_FAILURE);
      }
    }
    std::string path, base, ext;
    splitFileName(filename, path, base, ext);
    
    return base;
	}
	
  std::string getFilenameExtension(const std::string &filename, bool checkFile)
  {
    if (checkFile)
    {
      if (!fileExists(filename))
      {
        std::cerr << "Supplied file name wasn't found.\n";
        exit(EXIT_FAILURE);
      }
    }
    std::string path, base, ext;
    splitFileName(filename, path, base, ext);
    
    return ext;
  }

  std::string getFilenamePath(const std::string &filename, bool checkFile)
  {
    if (checkFile)
    {
      if (!fileExists(filename))
      {
        std::cerr << "Supplied file name wasn't found.\n";
        exit(EXIT_FAILURE);
      }
    }
    std::string path, base, ext;
    splitFileName(filename, path, base, ext);
    
    return path;
  }

  std::string getExecutableName()
  {
    std::string return_string;
    #if defined(_WIN32)
    	//! Initialize pointers to file and user names
    	char filename[FILENAME_MAX];
	    GetModuleFileNameA(NULL, filename, FILENAME_MAX);
      std::string path, ext;
      splitFileName(filename, path, return_string, ext);
      //_splitpath_s(filename, NULL, NULL, NULL, NULL, filename, NULL, NULL, NULL);
    #else
    	//! Initialize pointers to file and user names
    	char *filename, filename_2[FILENAME_MAX];    
    	::readlink("/proc/self/exe", filename_2, sizeof(filename_2)-1);
      filename = basename(filename_2);
      return_string = std::string(filename);
    #endif

    filename[0] = '\0';
    
    return return_string;
  }

  std::string getExecutablePath()
  {
    std::string path, base, ext;
    splitFileName(getFullPath(), path, base, ext);
    return path;
  }
  
  std::string getFullPath()
  {
    #if defined(_WIN32)
    	//! Initialize pointers to file and user names
    	char path[FILENAME_MAX];
	    GetModuleFileNameA(NULL, path, FILENAME_MAX);
      //_splitpath_s(filename, NULL, NULL, NULL, NULL, filename, NULL, NULL, NULL);
    #else
    	//! Initialize pointers to file and user names
    	char path[PATH_MAX];    
    	::readlink("/proc/self/exe", path, sizeof(path)-1);
    #endif

    std::string return_string = std::string(path);
    path[0] = '\0';
    
    return return_string;
  }
  
  std::string getUserName()
  {
    #if defined(_WIN32)
    	//! Initialize pointers to file and user names
    	char username[FILENAME_MAX];
    	DWORD username_len = FILENAME_MAX;
    	GetUserName(username, &username_len);
    #else
    	//! Initialize pointers to file and user names
    	char username[FILENAME_MAX];
    	size_t username_len = FILENAME_MAX;
    	getlogin_r(username, username_len);
    #endif
      
    std::string return_string = std::string(username);
    username[0] = '\0';
    
    return return_string;
  }

  std::string getCWD()
  {
    std::string wd = "";
    
    // use c++ convention
    char* buffer = GetCurrentDir(NULL, 0);
    
    if( buffer ) 
    {
      wd = buffer;
      buffer[0] = '\0';
    }
    wd = replaceString(wd, "\\", "/");
    if (wd[wd.length()-1] != '/')
    {
      wd.append("/");
    }

    return wd;
  }

  //! Check for separators [Internal_Function]
  inline bool issep( char c )
  {
    #if defined(_WIN32)
        return c == '/' || c == '\\';
    #else
        return c == '/';
    #endif
  }

  //! Check for absolute path [Internal_Function]
  bool isabs( const std::string& path )
  {
    size_t i = 0;
    #if defined(_WIN32)
        if (path.size() > 1 && path[1] == ':') i = 2;
    #endif
    return i < path.size() && issep(path[i]);
  }
  
  //! Joins separators based on OS [Internal_Function]
  std::string join( const std::string& base, const std::string& path )
  {
    if( base.empty() || isabs(path) )  
      return path;
    if( issep(base[base.size() - 1]) ) 
      return (base + path);

    #if defined(_WIN32)
        return base + '\\' + path;
    #else
        return base + '/' + path;
    #endif
  }

  std::string normPath( const std::string &path )
  {
    if( path.empty() ) 
      return "";
    char drive[3] = {'\0', ':', '\0'};
    size_t i = 0;
    #if defined(_WIN32)
      if( path.size() > 1 && path[1] == ':' ) 
      {
        drive[0] = path[0];
        i = 2;
      }
    #endif
    std::string norm_path = drive;
    bool abs = issep(path[i]);
    if( abs ) 
    {
      #if defined(_WIN32)
        while( i <= path.size() && issep(path[i]) ) 
        {
          norm_path += cSeparator;
          i++;
        }
      #else
        norm_path += cSeparator;
      #endif
    }
    std::string current;
    std::vector<std::string> parts;
    while( i <= path.size() ) 
    {
      if( issep(path[i]) || path[i] == '\0' )
      {
        if( current == ".." ) 
        {
          if( !abs && (parts.empty() || parts.back() == "..") ) 
          {
            parts.push_back(current);
          } 
          else if( !parts.empty() )
          {
            parts.pop_back();
          }
        } 
        else if( current != "" && current != "." ) 
        {
          parts.push_back(current);
        }
        current.clear();
      } 
      else 
      {
        current += path[i];
      }
      i++;
    }
    for( i = 0; i < parts.size(); i++ ) 
    {
      norm_path = join(norm_path, parts[i]);
    }
    return norm_path.empty() ? "." : norm_path;
  }

  std::string normalizePath( const std::string &path )
  {
    return normPath(path);
  }

  //! Absolute path [Internal_Function]
  std::string absPath(const std::string &path)
  {
    return normPath( join(getCWD(),path) );
  }

  //! Split Drive name from path [Internal_Function]
  inline void splitDrive( const std::string& path, std::string& drive, std::string& tail )
  {
    #if defined(_WIN32)
      if( path.size() > 1 && path[1] == ':' ) 
      {
          tail  = path.substr(2);
          drive = path[0]; drive += ':';
      }
      else
    #endif
      {
        tail  = path;
        drive = "";
      }
  }

  //! Split Drive name from path [Internal_Function]
  inline std::vector<std::string> splitDrive( const std::string& path )
  {
    std::vector<std::string> parts(2, "");
    splitDrive(path, parts[0], parts[1]);
    return parts;
  }

  std::string relPath( const std::string &path, const std::string &base )
  {
    // if relative path is given just return it
    if( !isabs(path) ) 
      return path;
    // normalize paths
    std::string norm_path = normPath(path);
    std::string norm_base = normPath(join(getCWD(), base));
    // check if paths are on same drive
    #if defined(_WIN32)
      std::string drive = splitDrive(norm_path)[0];
      std::string base_drive = splitDrive(norm_base)[0];
      if( drive != base_drive ) 
      {
        std::cerr << "Error: Path is on drive " << drive << ", start is on drive " << base_drive;
      }
    #endif
    // find start of first path component in which paths differ
    std::string::const_iterator b = norm_base.begin();
    std::string::const_iterator p = norm_path.begin();
    size_t pos = 0;
    size_t i   = 0;
    while( b != norm_base.end() && p != norm_path.end() ) 
    {
      if( issep(*p) ) 
      {
        if( !issep(*b) ) 
          break;
        pos = i;
      } 
      else if( *b != *p ) 
      {
        break;
      }
      b++; p++; i++;
    }
    // set pos to i (in this case, the size of one of the paths) if the end
    // of one path was reached, but the other path has a path separator
    // at this position, this is required below
    if( (b != norm_base.end() && issep(*b)) || (p != norm_path.end() && issep(*p)) ) 
        pos = i;
    // skip trailing separator of other path if end of one path reached
    if( b == norm_base.end() && p != norm_path.end() && issep(*p) ) 
      p++;
    if( p == norm_path.end() && b != norm_base.end() && issep(*b) ) 
      b++;
    // if paths are the same, just return a period (.)
    //
    // Thanks to the previous skipping of trailing separators, this condition
    // handles all of the following cases:
    //
    //    base := "/usr/bin"  path := "/usr/bin"
    //    base := "/usr/bin/" path := "/usr/bin/"
    //    base := "/usr/bin"  path := "/usr/bin/"
    //    base := "/usr/bin/" path := "/usr/bin"
    if( b == norm_base.end() && p == norm_path.end() ) 
      return ".";
    // otherwise, pos is the index of the last slash for which both paths
    // were identical; hence, everything that comes after in the original
    // path is preserved and for each following component in the base path
    // a "../" is prepended to the relative path
    std::string rel_path;
    // truncate base path with a separator as for each "*/" path component,
    // a "../" will be prepended to the relative path
    if( b != norm_base.end() && !issep(norm_base[norm_base.size() - 1]) ) 
    {
      // attention: This operation may invalidate the iterator b!
      //            Therefore, remember position of iterator and get a new one.
      size_t pos = b - norm_base.begin();
      norm_base += cSeparator;
      b = norm_base.begin() + pos;
    }
    while( b != norm_base.end() ) 
    {
      if( issep(*b) ) 
      {
        rel_path += "..";
        rel_path += cSeparator;
      }
      b++;
    }
    if( pos + 1 < norm_path.size() ) 
      rel_path += norm_path.substr(pos + 1);
    // remove trailing path separator
    if( issep(rel_path[rel_path.size() - 1]) ) 
    {
        rel_path.erase(rel_path.size() - 1);
    }
    return rel_path;
  }

  std::string relativePath(const std::string &path, const std::string &base)
  {
    return relPath(path, base);
  }

  std::string realPath( const std::string &path )
  {
    std::string curr_path = join(getCWD(), path);
    #if defined(_WIN32)
	  // nothing extra required
    #else
      char *actualPath = realpath(const_cast<char *>(curr_path.c_str()), NULL);
      curr_path = std::string(actualPath);
/*
      // use stringstream and std::getline() to split absolute path at slashes (/)
      std::stringstream ss(curr_path);
      curr_path.clear();
      std::string fname;
      std::string prev_path;
      std::string next_path;
      char slash;
      ss >> slash; // root slash
      while( getline(ss, fname, '/') ) 
      {
        // current absolute path
        curr_path += '/';
        curr_path += fname;
        // if current path is a symbolic link, follow it
        if( isLink(curr_path) ) 
        {
          // for safety reasons, restrict the depth of symbolic links followed
          for( unsigned int i = 0; i < 100; i++ ) 
          {
		    char *buffer=NULL, *newbuf=NULL;
            size_t buflen = 256;
            for(;;)
            {
              newbuf = reinterpret_cast<char*>(realloc(buffer, buflen * sizeof(char)) );
              if( !newbuf )
                break;
              buffer = newbuf;
              int n = ::newlink(path.c_str(), buffer, buflen);
              if( n<0 )
                break;
              if( static_cast<size_t>(n)<buflen )
              {
                buffer[n] = '\0';
                next_path = buffer;
                break;
              }
              buflen+=256;
            }
            free(buffer);
            //next_path = os::readlink(curr_path);
            if( next_path.empty() ) 
            {
              // if real path could not be determined because of permissions
              // or invalid path, return the original path
              break;
            } 
            else 
            {
              curr_path = join(prev_path, next_path);
              if( !isLink(next_path) ) 
                break;
            }
          }
          // if real path could not be determined with the given maximum number
          // of loop iterations (endless cycle?) or one of the symbolic links
          // could not be read, just return original path as absolute path
          if( isLink(next_path) ) 
            return absPath(path);
        }
        // memorize previous path used as base for abspath()
        prev_path = curr_path;
      }
*/
    #endif
    // normalize path after all symbolic links were resolved
    return normPath(curr_path);
  }

  bool isLink(const std::string &path)
  {
    #if defined(_WIN32)
      std::cerr << "Windows doesn't support ways to distinguish between hard and soft links.\n";
      return false;
    #else
      struct stat info;
      if (lstat(path.c_str(), &info) != 0) 
        return false;
      return S_ISLNK(info.st_mode);
    #endif
  }

  bool isSymbolicLink(const std::string &path)
  {
    return isLink(path);
  }

  bool makeSymbolicLink(const std::string &input_fileName, const std::string &ouput_fileName)
  {
    if (!cbica::fileExists(input_fileName))
    {
      std::cerr << "Supplied file name wasn't found.\n";
      exit(EXIT_FAILURE);
    }
    #if defined(_WIN32)
      if( IsUserAnAdmin() )
      {
        if( CreateSymbolicLink(input_fileName.c_str(), ouput_fileName.c_str(), 0) != 0 )
          return true;
        else
          return false;
      }
      else
      {
        std::cerr << "Windows doesn't let non-admins create soft links.\n";
        return false;
      }
    #else
    if( symlink(input_fileName.c_str(), ouput_fileName.c_str()) == 0 )
      return true;
    else 
      return false;
    #endif
  }

  bool setEnvironmentVariable(const std::string &variable_name, const std::string &variable_value)
  {
    std::string totalVariable = variable_name + "=" + variable_value;
    try
    {
      #if defined(_WIN32)
        int test = _putenv(totalVariable.c_str());
      #else
        putenv(cbica::constCharToChar(totalVariable));
      #endif
    }
    catch(const std::exception &e)
    {
      std::cerr << "Exception caught: " << e.what() << std::endl;
      return false;
    }

    return true;
  }
  
  bool deleteEnvironmentVariable(const std::string &variable_name)
  {
    return cbica::setEnvironmentVariable(variable_name, "");
  }

  std::vector< std::string > filesInDirectory( const std::string &dirName )
  {
    if (!cbica::directoryExists(dirName))
    {
      std::cerr << "Supplied directory name wasn't found.\n";
      exit(EXIT_FAILURE);
    }
    std::vector< std::string > allFiles;
    std::string dirName_wrap = cbica::replaceString(dirName, "\\", "/");
    if (dirName_wrap[dirName_wrap.length()-1] != '/')
    {
      dirName_wrap.append("/");
    }
    #if defined(_WIN32)
    {
      dirName_wrap.append("*.*");
      char* search_path = cbica::constCharToChar(dirName_wrap.c_str());
      WIN32_FIND_DATA fd; 
      HANDLE hFind = ::FindFirstFile(search_path, &fd); 
      if(hFind != INVALID_HANDLE_VALUE) 
      { 
        do 
        { 
          if(! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
          {
            allFiles.push_back(fd.cFileName);
          }
        } while(::FindNextFile(hFind, &fd)); 
        ::FindClose(hFind); 
      } 
      return allFiles;

    }
    #else
    {
      DIR *dp;
      struct dirent *dirp;
      if((dp  = opendir(dirName.c_str())) == NULL) 
      {
        std::cerr << "Error(" << errno << ") occurred while opening directory '" << 
          dirName << "'\n";
      }
      
      while ((dirp = readdir(dp)) != NULL) 
      {
        allFiles.push_back(std::string(dirp->d_name));
      }
      closedir(dp);
      return allFiles;
    }
    #endif
  }
  
  std::vector<std::string> subdirectoriesInDirectory(const std::string &dirName, bool recursiveSearch)
  {
    if (!cbica::directoryExists(dirName))
    {
      std::cerr << "Supplied directory name wasn't found.\n";
      exit(EXIT_FAILURE);
    }
    std::vector< std::string > allDirectories;
    std::string dirName_wrap = cbica::replaceString(dirName, "\\", "/");
    if (dirName_wrap[dirName_wrap.length() - 1] != '/')
    {
      dirName_wrap.append("/");
    }
#if defined(_WIN32)
    dirName_wrap.append("*.*");
    char* search_path = cbica::constCharToChar(dirName_wrap.c_str());
    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if ((fd.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && (fd.cFileName[0] != '.'))
        {
          allDirectories.push_back(std::string(fd.cFileName));
          if (recursiveSearch)
          {
            std::vector<std::string> tempVector = subdirectoriesInDirectory(dirName + "/" + std::string(fd.cFileName), true);
            for (size_t i = 0; i < tempVector.size(); i++)
            {
              allDirectories.push_back(std::string(fd.cFileName) + "/" + tempVector[i]);
            }
          }
        }
      } while (FindNextFile(hFind, &fd) != 0);
      ::FindClose(hFind);
    }
    return allDirectories;
#else
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(dirName.c_str())) == NULL)
    {
      std::cerr << "Error(" << errno << ") occurred while opening directory '" << dirName << "'\n";
    }

    while ((dirp = readdir(dp)) != NULL)
    {
      if (recursiveSearch && (dirp->d_type == DT_DIR) && dirp->d_name[0] != '.')
      {
        std::vector<std::string> tempVector = subdirectoriesInDirectory(dirName + "/" + dirp->d_name, true);
        for (size_t i = 0; i < tempVector.size(); i++)
          allDirectories.push_back(std::string(dirp->d_name) + "/" + tempVector[i]);
      }

      if ( (strcmp(dirp->d_name, ".") == 0) || (strcmp(dirp->d_name, "..") == 0) )
        continue;

      allDirectories.push_back(dirp->d_name);
    }
    closedir(dp);
    return allDirectories;
#endif
  }
  
  size_t numberOfRowsInFile(const std::string &csvFileName, const std::string &delim)
  {
    std::ifstream inFile(csvFileName.c_str());

    // new lines will be skipped     
    inFile.unsetf(std::ios_base::skipws);
    
    // count the "\n"s with an algorithm specialized for counting
    return std::count(std::istream_iterator<char>(inFile), std::istream_iterator<char>(), *constCharToChar(delim));
  }

  size_t numberOfColsInFile(const std::string &csvFileName, const std::string & delim)
  {
    std::vector< std::string > rowVec;
    //std::ifstream inFile(csvFileName.c_str());
    //std::string line;
    //std::getline(inFile, line, '\n');
    //line.erase(std::remove(line.begin(), line.end(), '"'), line.end());
    //
    //// read a single row
    //rowVec = stringSplit(line, delim);
    
    return rowVec.size();
  }

  std::vector< CSVDict > parseCSVFile( const std::string &csvFileName, const std::string &inputColumns, const std::string &inputLabels, 
    bool checkFile, const std::string &rowsDelimiter, const std::string &colsDelimiter, const std::string &optionsDelimiter)
  {
    return parseCSVFile(cbica::getFilenamePath(csvFileName), csvFileName, inputColumns, inputLabels, checkFile, rowsDelimiter, colsDelimiter, optionsDelimiter);
  }

  std::vector< CSVDict > parseCSVFile(const std::string &dataDir, const std::string &csvFileName, 
    const std::string &inputColumns, const std::string &inputLabels, bool checkFile,
    const std::string &rowsDelimiter, const std::string &colsDelimiter, const std::string &optionsDelimiter)
  {
    // if CSV file doesn't exist, exit with meaningful message
    if (!cbica::fileExists(csvFileName))
    {
      std::cerr << "Supplied file name wasn't found.\n";
      exit(EXIT_FAILURE);
    }

    bool labelsConsidered = true;
    if (inputLabels.empty())
    {
      labelsConsidered = false;
    }

    // store number of rows in the file - this is used to make the program parallelize-able 
    const size_t numberOfRows = numberOfRowsInFile(csvFileName);
    
    std::vector< std::string > inputColumnsVec = stringSplit(inputColumns, optionsDelimiter), inputLabelsVec; // columns to consider as images
    if (labelsConsidered)
    {
      inputLabelsVec = stringSplit(inputLabels, optionsDelimiter); // columns to consider as labels
    }

    // initialize return dictionary
    std::vector< CSVDict > return_CSVDict;
    return_CSVDict.resize(numberOfRows - 1);

    std::vector< size_t > inputColumnIndeces, // indeces in the CSV file which correspond to images
      inputLabelIndeces; // indeces in the CSV file which correspond to labels

    inputColumnIndeces.resize(inputColumnsVec.size());
    inputLabelIndeces.resize(inputLabelsVec.size());

    std::vector< std::vector < std::string > > allRows; // store the entire data of the CSV file as a vector of colums and rows (vector< rows <cols> >)

    std::ifstream inFile(csvFileName.c_str());

    for (size_t i = 0; i < numberOfRows; i++)
    {
      std::string line;
      std::getline(inFile, line, *constCharToChar(rowsDelimiter));
      line.erase(std::remove(line.begin(), line.end(), '"'), line.end());
      allRows.push_back(stringSplit(line, colsDelimiter));
    }

    inFile.close(); // at this point, the entire data from the CSV file has been read and stored in allRows

    // populate information about which indeces to store for data and labels from first row (rowCounter = 0)
    for (size_t i = 0; i < allRows[0].size(); i++)
    {
      for (size_t j = 0; j < inputColumnsVec.size(); j++)
      {
        inputColumnIndeces[j] = std::find(allRows[0].begin(), allRows[0].end(), inputColumnsVec[j]) - allRows[0].begin();
      }
      if (labelsConsidered)
      {
        for (size_t j = 0; j < inputLabelsVec.size(); j++)
        {
          inputLabelIndeces[j] = std::find(allRows[0].begin(), allRows[0].end(), inputLabelsVec[j]) - allRows[0].begin();
#ifndef _WIN32
          inputLabelIndeces[j] -= inputLabelIndeces[j]; // this is done because gcc, for some weird reason, sets the absolute value for the indeces
#endif
        }
      }
    }

    // organize the data
    int threads = omp_get_max_threads(); // obtain maximum number of threads available on machine  
    // if the total number of rows in CSV file are less than the available number of threads on machine (happens for testing),
    // use only the number of rows where meaningful data is present - this avoids extra thread overhead
    threads > static_cast<int>(numberOfRows) ? threads = static_cast<int>(numberOfRows - 1) : threads = threads; 
//#pragma omp parallel for num_threads(threads)
    for (int rowCounter = 1; rowCounter < static_cast<int>(allRows.size()); rowCounter++)
    {
      return_CSVDict[rowCounter - 1].inputImages.resize(inputColumnIndeces.size()); // pre-initialize size to ensure thread-safety
      if (labelsConsidered)
      {
        return_CSVDict[rowCounter - 1].inputLabels.resize(inputLabelIndeces.size()); // pre-initialize size to ensure thread-safety
      }
      else
      {
        return_CSVDict[rowCounter - 1].inputLabels.resize(1);
      }
      for (size_t i = 0; i < inputColumnIndeces.size(); i++)
      {
        std::string fileToAdd = dataDir + allRows[rowCounter][inputColumnIndeces[i]];
        if (!checkFile) // this case should only be used for testing purposes
        {
          return_CSVDict[rowCounter - 1].inputImages[i] = fileToAdd;
        }
        else
        {
          if (fileExists(fileToAdd))
          {
            return_CSVDict[rowCounter - 1].inputImages[i] = fileToAdd;
          }
          else
          {
            std::cerr << "File name in list does not exist. Location: row = " << rowCounter << ", col = " << inputColumnIndeces[i] << "\n";
            exit(EXIT_FAILURE);
          }
        }
        for (size_t j = 0; j < inputLabelIndeces.size(); j++)
        {
          if (labelsConsidered)
          {
            return_CSVDict[rowCounter - 1].inputLabels[j] = std::atof(allRows[rowCounter][inputLabelIndeces[j]].c_str());
          }
          else
          {
            return_CSVDict[rowCounter - 1].inputLabels[j] = 1;
          }
        }
      }
      
    }

    return return_CSVDict;
  }

  inline std::string iterateOverStringAndSeparators(const std::string &inputString, size_t &count, int enum_separator = 10)
  {
    std::string returnString = "";
    if (enum_separator == 10) // to get description
    {
      returnString = inputString.substr(count + 1); // get all characters after the separator was detected
    }    
    else // for everything other than description
    {
      char testChar = inputString[count], separatorChar = *cbica::constCharToChar(getSeparator(enum_separator));
      size_t position, // position to start getting the substring
        separatorChecker = 2; // the configuration file needs the difference between two types of strings to be a single space (apart from the separator string)
      if (testChar == separatorChar)  
      {
        count++;
        position = count;
        //stringStream.clear();
        //stringStream << inputString[count];
        //std::string testStr = stringStream.str(), testSep = getSeparator(enum_separator);
        //while (stringStream.str() != getSeparator(enum_separator))
        //{
        //  stringStream << inputString[count];
        //  returnString += stringStream.str();
        //  stringStream.clear();
        //  count++;
        //}
        testChar = inputString[count];
        while (testChar != separatorChar)
        {
          count++;
          testChar = inputString[count];
        }
        
        returnString = inputString.substr(position, count - position);
      }
      else // a small check as a contingency plan
      {
        while (separatorChecker > 0)
        {
          separatorChecker--;
          count++;
          testChar = inputString[count];
        }
      }
    }

    return returnString;
  }

  std::vector< Parameter > readConfigFile(const std::string &path_to_config_file, bool getDescription)
  {
    std::vector< Parameter > returnVector;
    std::ifstream inputFile(path_to_config_file.c_str());
    if (!inputFile) 
    {
      std::cerr << "File '" << path_to_config_file << "' not found.\n";
      exit(EXIT_FAILURE);
    }
    std::string line;
    while (std::getline(inputFile, line)) 
    {
      std::string parameter, parameterDataType, parameterDataRange, parameterDescription = "";
      for (size_t i = 0; i < line.length(); i++)
      {
        parameter = iterateOverStringAndSeparators(line, i, 
#ifdef _WIN32
          Separator::
#endif
          Param);
        i = i + 2;
        parameterDataType = iterateOverStringAndSeparators(line, i,
#ifdef _WIN32
          Separator::
          #endif
          DataType);
        i = i + 2;
        parameterDataRange = iterateOverStringAndSeparators(line, i,
#ifdef _WIN32
          Separator::
#endif
          DataRange);
        if (getDescription)
        {
          i = i + 1;
          parameterDescription = iterateOverStringAndSeparators(line, i, 10);
        }
        i = line.length();
        returnVector.push_back(Parameter("", parameter, parameterDataType, parameterDataRange, parameterDescription));
      }
    }

    inputFile.close();
    return returnVector;
  }

  std::string getCurrentLocalDate()
  {
    time_t timer;
    // obtain current time
    time(&timer);
    tm *time_struct = NULL;
    char buffer[200];

    // obtain current local date
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &timer);
    sprintf_s(buffer, "%d:%02d:%02d",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
#else
    time_struct = localtime(&timer_wrap);
    sprintf(buffer, "%d:%02d:%02d",
      time_struct->tm_year + 1900, time_struct->tm_mon + 1, time_struct->tm_mday);
#endif

    return buffer;
  }

  std::string getCurrentLocalTime()
  {
    time_t timer;
    // obtain current time
    time(&timer);
    tm *time_struct = NULL;
    char buffer[200];

    // obtain current local date
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &timer);
    sprintf_s(buffer, "%02d:%02d:%02d",
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#else
    time_struct = localtime(&timer_wrap);
    sprintf(buffer, "%02d:%02d:%02d",
      time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
#endif

    return buffer;
  }

  std::string getCurrentLocalDateAndTime()
  {
    time_t timer;
    // obtain current time
    time(&timer);
    tm *time_struct = NULL;
    char buffer[200];

    // obtain current local date
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &timer);
    sprintf_s(buffer, "%d:%02d:%02d,%02d:%02d:%02d",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#else
    time_struct = localtime(&timer_wrap);
    sprintf(buffer, "%d:%02d:%02d,%02d:%02d:%02d",
      time_struct->tm_year + 1900, time_struct->tm_mon + 1, time_struct->tm_mday,
      time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
#endif

    return buffer;
  }

  std::string getCurrentGMTDate()
  {
    time_t timer;
    // obtain current time
    time(&timer);
    tm *time_struct = NULL;
    char buffer[200];

    // obtain current local date
#ifdef _WIN32
    struct tm timeinfo;
    gmtime_s(&timeinfo, &timer);
    sprintf_s(buffer, "%d:%02d:%02d",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
#else
    time_struct = gmtime(&timer_wrap);
    sprintf(buffer, "%d:%02d:%02d",
      time_struct->tm_year + 1900, time_struct->tm_mon + 1, time_struct->tm_mday);
#endif

    return buffer;
  }

  std::string getCurrentGMT()
  {
    time_t timer;
    // obtain current time
    time(&timer);
    tm *time_struct = NULL;
    char buffer[200];

    // obtain current local date
#ifdef _WIN32
    struct tm timeinfo;
    gmtime_s(&timeinfo, &timer);
    sprintf_s(buffer, "%02d:%02d:%02d",
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#else
    time_struct = gmtime(&timer_wrap);
    sprintf(buffer, "%02d:%02d:%02d",
      time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
#endif

    return buffer;
  }

  std::string getCurrentGMTDateAndTime()
  {
    time_t timer;
    // obtain current time
    time(&timer);
    tm *time_struct = NULL;
    char buffer[200];

    // obtain current local date
#ifdef _WIN32
    struct tm timeinfo;
    gmtime_s(&timeinfo, &timer);
    sprintf_s(buffer, "%d:%02d:%02d,%02d:%02d:%02d",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#else
    time_struct = gmtime(&timer_wrap);
    sprintf(buffer, "%d:%02d:%02d,%02d:%02d:%02d",
      time_struct->tm_year + 1900, time_struct->tm_mon + 1, time_struct->tm_mday,
      time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
#endif

    return buffer;
  }

  //====================================== String stuff ====================================//

  bool splitFileName( const std::string &dataFile, std::string &path,
     std::string &baseName, std::string &extension )
  {
    std::string dataFile_wrap = dataFile;
    std::vector< std::string > compressionFormats;
    compressionFormats.push_back(".gz");
    compressionFormats.push_back(".bz");
    compressionFormats.push_back(".zip");
    compressionFormats.push_back(".bz2");
    
    // check for compression formats
    for (size_t i = 0; i < compressionFormats.size(); i++)
    {
      if (dataFile_wrap.find(compressionFormats[i]) != std::string::npos)
      {
        dataFile_wrap = cbica::replaceString(dataFile_wrap, compressionFormats[i], "");
        std::string tempExt;
        cbica::splitFileName(dataFile_wrap, path, baseName, tempExt);
        extension = tempExt + compressionFormats[i];
      }
    }
    if (!path.empty() && !baseName.empty() && !extension.empty())
    {
      return true;
    }
    else
    {
      //! Initialize pointers to file and user names
#if (_MSC_VER >= 1700)
      char basename_var[FILENAME_MAX], ext[FILENAME_MAX], path_name[FILENAME_MAX], drive_letter[FILENAME_MAX];
      //_splitpath(dataFile_wrap.c_str(), NULL, path_name, basename_var, ext);
      _splitpath_s(dataFile.c_str(), drive_letter, FILENAME_MAX, path_name, FILENAME_MAX, basename_var, FILENAME_MAX, ext, FILENAME_MAX);
#else
      char *basename_var, *ext, *path_name;
      path_name = dirname(cbica::constCharToChar(dataFile_wrap.c_str()));
      basename_var = basename(cbica::constCharToChar(dataFile_wrap.c_str()));
      ext = strrchr(cbica::constCharToChar(dataFile_wrap.c_str()), '.');
#endif

      //path sanity check
      if (path_name == NULL)
      {
        std::cerr << "No filename path has been detected.\n";
        exit(EXIT_FAILURE);
      }
      else
      {
        path =
#ifdef _WIN32
          std::string(drive_letter) +
#endif
          std::string(path_name);
      }
      path = cbica::replaceString(path, "\\", "/"); // normalize path for Windows

      //base name sanity check
      if (basename_var == NULL)
      {
        std::cerr << "No filename base has been detected.\n";
        exit(EXIT_FAILURE);
      }
      else
      {
        baseName = std::string(basename_var);
      }

      //extension sanity check
      if (ext == NULL)
      {
        extension = "";
      }
      else
      {
        extension = std::string(ext);
      }

#if (_MSC_VER >= 1700)
      path_name[0] = NULL;
      basename_var[0] = NULL;
      ext[0] = NULL;
      drive_letter[0] = NULL;
#endif
      if (path[path.length() - 1] != '/')
      {
        path += "/";
      }

      return true;
    }
  }

  std::vector<std::string> stringSplit( const std::string &str, const std::string &delim )
  {
    std::vector<std::string> results;

    for (size_t i = 0; i < str.length(); i++)
    {
      std::string tempString = "";
      while ((str[i] != *delim.c_str()) && (i < str.length()))
      {
        tempString += str[i];
        i++;
      }
      results.push_back(tempString);
    }

    return results;
  }

  std::string replaceString( const std::string &entireString, 
                                    const std::string &toReplace, 
                                    const std::string &replaceWith )
  {
    std::string return_string = entireString;
    for( size_t pos = 0; ; pos += replaceWith.length() ) 
    {
    pos = return_string.find( toReplace, pos );
      if( pos == std::string::npos ) 
        break;
      
      return_string.erase( pos, toReplace.length() );
      return_string.insert( pos, replaceWith );
    }
    return return_string;
    /*
    if( entireString.length() < toReplace.length() )
      std::cerr << "Length of string to search < length of string to replace. Please check.\n";

    return(return_string.replace(entireString.find(toReplace), toReplace.length(), replaceWith));
    */
  }
  
  char* constCharToChar(const std::string &input)
  {
    char *s = new char[input.size()+1];
#ifdef _WIN32
    strcpy_s(s, input.size() + 1, input.c_str());
#else
    std::strcpy(s, input.c_str());
#endif
    return s;
  }

  char* constCharToChar( const char *input )
  {
    return cbica::constCharToChar(std::string(input));
  }

  std::string computeMD5Sum(const std::string &fileName)
  {
    //gdcm::MD5 md5Computer;
    //char digStr[_MAX_PATH];
    //md5Computer.ComputeFile(fileName.c_str(), digStr);
    //return std::string(digStr);
    return "";
  }


}