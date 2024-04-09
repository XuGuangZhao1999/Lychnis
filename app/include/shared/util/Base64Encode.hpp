#pragma once
#include <string>

namespace shared{
    namespace util{
        // Funcationality: Encode data in base64 format as strings
        // Input:
        //      Data: image data in base64 format
        //      size: data size
        // Output: the converted string
        std::string base64_encode(const unsigned char* Data, size_t size);
    }// namespace util
}// namespace shared