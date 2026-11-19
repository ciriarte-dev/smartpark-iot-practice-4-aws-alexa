#pragma once
#include <vector>
#include <cstring>
#include <map>
#include <string>
class Preferences {
public:
    inline static std::vector<unsigned char> disk;
    inline static std::map<std::string, std::vector<unsigned char>> other;
    inline static bool writable=true;
    bool begin(const char*,bool) { return true; }
    bool isKey(const char* key) { return !record(key).empty(); }
    size_t getBytesLength(const char* key) { return record(key).size(); }
    size_t getBytes(const char* key,void* p,size_t n) {
        auto& bytes = record(key);
        if(bytes.size()!=n) return 0;
        std::memcpy(p,bytes.data(),n);
        return n;
    }
    size_t putBytes(const char* key,const void* p,size_t n) {
        if(!writable) return 0;
        auto& bytes = record(key);
        bytes.resize(n);
        std::memcpy(bytes.data(),p,n);
        return n;
    }
private:
    static std::vector<unsigned char>& record(const char* key) {
        return std::strcmp(key, "budget") == 0 ? disk : other[key];
    }
};
