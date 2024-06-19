#ifndef SCANNER_H
#define SCANNER_H

// Standard Library Inclusions
#include <iostream>
#include <cwctype>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <mutex>

// Project Inclusions
#include "Database.h"
#include "SystemUtilities.h"
#include "ThreadSafeQueue.h"
#include "FileRecord.h"
#include "AudioFile.h"
#include "FourierTX.h"

// Definitions
namespace fs = std::filesystem;
#define TRANSACTION_SIZE 2048

// Delimiter check function
bool char_is_delimiter (char);

// Character to lowercase conversion
char to_lower (char);

// Tag generation function
std::vector<std::string> generate_auto_tags (const std::string &);

// Tag concatenation function
std::string concatenate_tags (const std::vector<std::string> &);

// File processing function
struct FileRecord *process_file (const fs::directory_entry &);

// File extension validation
inline bool validate_file_extension (const fs::directory_entry *);

// Sub-directory finding function
std::vector<fs::path> find_sub_dirs (const fs::path &);

// File processing requirement check
bool requires_processing (Database *, const fs::directory_entry *);

// File queueing functions
void queue_files (Database *, const fs::path &,
                ThreadSafeQueue<fs::directory_entry> *);

void queue_all_files (Database *, const fs::path &, 
                ThreadSafeQueue<fs::directory_entry> *);

// Processing queued files function
void process_queued_files (Database *,
        ThreadSafeQueue<fs::directory_entry> *,
        ThreadSafeQueue<struct FileRecord *> *);

// Insert processed files function
void insert_processed_files (Database *,
    ThreadSafeQueue<struct FileRecord *> *);

// Directory scanning function
void scan_directory (Database *, const fs::path &);

#endif // SCANNER_H
