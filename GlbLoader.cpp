#include "GLBLoader.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

std::string GLBLoader::get_file_contents(const char* filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open())
    {
        throw std::runtime_error(std::string("Failed to open file: ") + filename);
    }

    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    return contents;
}

bool GLBLoader::LoadGLB(const char* file, json& outJSON, std::vector<unsigned char>& outData)
{
    try
    {
        std::cout << "Loading GLB file: " << file << std::endl;

        std::string binaryData = get_file_contents(file);
        std::vector<unsigned char> data(binaryData.begin(), binaryData.end());

        std::cout << "File size: " << data.size() << " bytes" << std::endl;

        // Проверка magic number "glTF" и версии
        if (data.size() < 12)
        {
            std::cout << "ERROR: GLB file too small" << std::endl;
            return false;
        }

        if (data[0] != 'g' || data[1] != 'l' || data[2] != 'T' || data[3] != 'F')
        {
            std::cout << "ERROR: Invalid GLB file - wrong magic number" << std::endl;
            return false;
        }

        // Проверяем версию
        unsigned int version;
        std::memcpy(&version, &data[4], 4);
        std::cout << "GLB version: " << version << std::endl;

        if (version != 2)
        {
            std::cout << "ERROR: Unsupported GLB version: " << version << std::endl;
            return false;
        }

        // Получаем общую длину файла
        unsigned int totalLength;
        std::memcpy(&totalLength, &data[8], 4);
        std::cout << "Total length: " << totalLength << std::endl;

        // Извлекаем JSON chunk
        unsigned int jsonLength;
        std::memcpy(&jsonLength, &data[12], 4);
        std::cout << "JSON length: " << jsonLength << std::endl;

        // Проверяем тип chunk (должен быть JSON)
        unsigned int jsonType;
        std::memcpy(&jsonType, &data[16], 4);
        std::cout << "JSON type: " << std::hex << jsonType << std::dec << std::endl;

        if (jsonType != 0x4E4F534A) // "JSON" в little-endian
        {
            std::cout << "ERROR: Invalid JSON chunk type" << std::endl;
            return false;
        }

        // Извлекаем JSON строку
        std::string jsonStr(data.begin() + 20, data.begin() + 20 + jsonLength);
        outJSON = json::parse(jsonStr);

        // Ищем BIN chunk - начинаем с позиции после JSON chunk
        unsigned int currentPos = 20 + jsonLength;

        // Выравнивание до 4 байт
        if (currentPos % 4 != 0) {
            currentPos += 4 - (currentPos % 4);
        }

        std::cout << "Searching for BIN chunk starting at: " << currentPos << std::endl;

        // Проверяем, есть ли еще данные для BIN chunk
        while (currentPos + 8 <= data.size())
        {
            unsigned int chunkLength;
            std::memcpy(&chunkLength, &data[currentPos], 4);

            unsigned int chunkType;
            std::memcpy(&chunkType, &data[currentPos + 4], 4);

            std::cout << "Found chunk at " << currentPos << ": type=" << std::hex << chunkType
                << std::dec << ", length=" << chunkLength << std::endl;

            if (chunkType == 0x004E4942) // "BIN" в little-endian
            {
                std::cout << "BIN chunk found!" << std::endl;

                if (currentPos + 8 + chunkLength <= data.size())
                {
                    outData = std::vector<unsigned char>(data.begin() + currentPos + 8,
                        data.begin() + currentPos + 8 + chunkLength);
                    std::cout << "BIN data extracted: " << outData.size() << " bytes" << std::endl;
                    break;
                }
                else
                {
                    std::cout << "ERROR: BIN chunk size exceeds file size" << std::endl;
                    return false;
                }
            }
            else if (chunkType == 0x4E4F534A) // "JSON" chunk (уже обработан)
            {
                // Пропускаем, уже обработали
            }
            else
            {
                std::cout << "Unknown chunk type, skipping..." << std::endl;
            }

            // Переходим к следующему chunk
            currentPos += 8 + chunkLength;

            // Выравнивание до 4 байт
            if (currentPos % 4 != 0) {
                currentPos += 4 - (currentPos % 4);
            }
        }

        if (outData.empty()) {
            std::cout << "WARNING: No BIN data found in GLB file" << std::endl;
        }

        std::cout << "GLB file loaded successfully: " << file << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR loading GLB file " << file << ": " << e.what() << std::endl;
        return false;
    }
}