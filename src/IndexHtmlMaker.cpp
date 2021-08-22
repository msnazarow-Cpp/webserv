//
// Created by msnazarow on 19.08.2021.
//

#include "IndexHtmlMaker.hpp"
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <iomanip>
#include <cstring>

std::string IndexHtmlMaker::makeIndexFile(std::string rootDirectory, std::string requestDirectory) {
    DIR *dir;
    struct stat info;
    struct dirent *ent;
    std::ofstream file(".html");
    file.setf(std::ios::left);

    if (requestDirectory[requestDirectory.size() - 1] != '/'){
        requestDirectory.push_back('/');
    }
    if (rootDirectory[rootDirectory.size() - 1] != '/'){
        rootDirectory.push_back('/');
    }
    std::string bufDirectory = rootDirectory + requestDirectory;
    if (!(file << " <html>\n<head><title>Index of " << requestDirectory << "</title></head>\n<body bgcolor=\"white\">\n"))
        throw IndexHtmlMakerException();
    file << "<h1>Index of " << requestDirectory << "</h1><hr><pre>\n";
    if ((dir = opendir (bufDirectory.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if ((ent->d_name[0] != '.' || !strcmp(ent->d_name, "..")) && !stat((bufDirectory + ent->d_name).c_str(), &info)) {
                char buf[20];
                if(S_ISDIR(info.st_mode)){
                    strcat(ent->d_name,"/");
                }
                strftime(buf, sizeof(buf), "%Y-%m-%d.%X", localtime(&info.st_ctime));
                std::string out = std::string("<a href=\"") + requestDirectory +  ent->d_name + "\">" + ent->d_name + "</a>";
                file << out << std::setw(25 - strlen(ent->d_name)) << "" << std::setw(25) << buf << std::setw(25)
                     << info.st_size << "\n";
            }
        }
        closedir (dir);
        file << "</pre><hr></body>\n</html>\n";
    }
    else
        throw IndexHtmlMakerException();
    return (".html");
}
