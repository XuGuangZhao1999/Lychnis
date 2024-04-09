#include "shared/util/Base64Encode.hpp"

namespace shared{
    namespace util{
        std::string base64_encode(const unsigned char* Data, size_t size) {
            //编码表  
            const char EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            //返回值  
            std::string str_encode;
            unsigned char tmp[4] = { 0 };
            int line_length = 0;
            for (int i = 0; i < (int)(size / 3); i++)
            {
                tmp[1] = *Data++;
                tmp[2] = *Data++;
                tmp[3] = *Data++;
                str_encode += EncodeTable[tmp[1] >> 2];
                str_encode += EncodeTable[((tmp[1] << 4) | (tmp[2] >> 4)) & 0x3F];
                str_encode += EncodeTable[((tmp[2] << 2) | (tmp[3] >> 6)) & 0x3F];
                str_encode += EncodeTable[tmp[3] & 0x3F];
                if (line_length += 4, line_length == 76)
                {
                    str_encode += "\r\n"; 
                    line_length = 0;
                }
            }
            //对剩余数据进行编码  
            int mod = size % 3;
            if (mod == 1)
            {
                tmp[1] = *Data++;
                str_encode += EncodeTable[(tmp[1] & 0xFC) >> 2];
                str_encode += EncodeTable[((tmp[1] & 0x03) << 4)];
                str_encode += "==";
            }
            else if (mod == 2)
            {
                tmp[1] = *Data++;
                tmp[2] = *Data++;
                str_encode += EncodeTable[(tmp[1] & 0xFC) >> 2];
                str_encode += EncodeTable[((tmp[1] & 0x03) << 4) | ((tmp[2] & 0xF0) >> 4)];
                str_encode += EncodeTable[((tmp[2] & 0x0F) << 2)];
                str_encode += "=";
            }

            return str_encode;
        }   
    } // namespace util
    
}// namespace shared
