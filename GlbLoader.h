#ifndef GLBLOADER_H
#define GLBLOADER_H

#include <vector>
#include <string>
#include <json/json.h>

using json = nlohmann::json;

class GLBLoader {
public:
    static bool LoadGLB(const char* file, json& outJSON, std::vector<unsigned char>& outData);

private:
    static std::string get_file_contents(const char* filename);
};

#endif