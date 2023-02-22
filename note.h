#pragma once
//ÉmÅ[Écä‘ÇÃç≈í·ãóó£(micro sec)
#define NOTES_OVERLAP 100000.f
//notes
struct note
{
    unsigned char scale;
    unsigned long long tick;
    unsigned long long length_tick;
    unsigned int  lane;
    double  sec;
    double  length_sec;
    bool is_beaten;
};

inline int read_little_endian_int(const char* buf) {
    return *(int*)buf;
}

inline short read_little_endian_short(const char* buf) {
    return *(short*)buf;
}
