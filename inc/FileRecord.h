#ifndef FILE_RECORD_H
#define FILE_RECORD_H

// Standard Library Inclusions
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct FileRecord {

    std::wstring file_path;
    std::wstring file_name;

    size_t file_size;
    
    // user-submitted data
    int num_user_tags;
    std::wstring user_tags;
    int user_bpm;
    int user_key;

    // auto-generated data
    int num_auto_tags;
    std::wstring auto_tags;
    int auto_bpm;
    int auto_key;
};

#endif // FILE_RECORD_H