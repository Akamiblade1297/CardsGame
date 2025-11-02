#ifndef STRING_FUNC_CPP
#define STRING_FUNC_CPP

#include <string>
#include <vector>
#include <numeric>
#include <sstream>

std::string join ( const std::vector<std::string>& vec, const std::string& del ) {
     return std::accumulate(vec.begin(), vec.end(), std::string{},
            [del](const std::string& a, const std::string& b) {
                return a + del + b;
            });
}

std::vector<std::string> split( char* str, char del ) {
    std::vector<std::string> str_vec;
    std::stringstream str_stream(str);
    std::string s;
    
    while ( std::getline(str_stream, s, del) ) {
        str_vec.push_back(s);
    }

    return str_vec;
}

#endif
