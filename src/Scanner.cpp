#include "..\inc\Scanner.h"

// These delimiters are used to automatically generate tags from filenames
bool inline char_is_delimiter (char ch) {
    return (ch == '_' || 
            ch == ' ' || 
            ch == '-' || 
            ch == '.' || 
            ch == '(' || 
            ch == ')' || 
            ch == '[' || 
            ch == ']');
}

std::string to_string(const fs::path &p) {
    std::u8string u8str = p.u8string();
    return std::string(u8str.begin(), u8str.end());
}

// to_lower converts characters to their lowercase equivalent.
// This function is used to convert tags to lowercase in generate_auto_tags()
char inline to_lower (char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// generate_auto_tags generates tags from filename based on a set of delimiters
// the delimiters are set in char_is_delimiter()
// tags must be 3 characters or greater
// tags are returned in a vector of strings
std::vector<std::string> generate_auto_tags(const std::string& file_name) {
    
    std::vector<std::string> tags;
    std::string token;
    
    // for all the characters in the filename:
    // nondelimiter characters are accumualted in 'token'
    // when a delimiter is hit, save token if long enough

    for (char ch : file_name) {
        if (!char_is_delimiter(ch)) [[likely]] {
            // tags are lowercase
            token += to_lower(ch);
            continue;
        }
        else if (!token.empty()) [[unlikely]] {
            // tags must be 3 or more characters
            if(token.size() > 2) {
                tags.push_back(token);
            }
            token.clear();
        }
    }

    // push the reminaing token, if one
    if (!token.empty() && token.size() > 2) {
        tags.push_back(token);
    }
    
    return tags;
}

// concatenate strings in a vector of strings into one space delimited string
std::string concatenate_tags(const std::vector<std::string>& tags) {
    std::string result;
    for (const auto& tag : tags) {
        if (!result.empty()) [[likely]] {
            result += " ";
        }
        result += tag;
    }
    return result;
}

// given a directory entry, find and record attributes in FileRecord struct
struct FileRecord *process_file (sqlite3 *db, 
                                    const fs::directory_entry& file) {
    
    // allocate memory for entry parameters
    struct FileRecord *db_entry = new struct FileRecord;

    // identification
    db_entry->file_path = to_string(file.path());
    db_entry->file_name = to_string(file.path().filename());
    db_entry->file_size = static_cast<int>(fs::file_size(file.path()));
    
    // calculate file duration
    db_entry->duration = 0;
    
    // default user tags
    db_entry->num_user_tags = 0;
    db_entry->user_tags = "";
    
    // generate auto tags
    std::vector<std::string> tags = generate_auto_tags(db_entry->file_name);
    db_entry->num_auto_tags = tags.size();
    db_entry->auto_tags = concatenate_tags(tags);
    
    // default user bpm and key
    db_entry->user_bpm = 0;
    db_entry->user_key = 0;
    
    // TODO: predict bpm and key
    db_entry->auto_bpm = 0;
    std::string path = to_string(file.path());
    db_entry->auto_key = 0/*kdet_detect_key(path)*/;

    return db_entry;
}

// check if file extension is .mp3 or .wav
inline bool validate_file_extension (const fs::directory_entry *file) {    
    auto extension = to_string(file->path().extension());
    if (extension == ".mp3" || extension == ".wav") {
        return true;
    } else {
        return false;
    }
}

// Find the sub-directories in the top layer in a directory
// 
// This is used as a heuristic for dividing file scanning work between threads.
// However, splitting work by sub-directory may not evenly divide work, and bare 
// files in the root directory won't be scanned without changes to 
// scan_directory() 
std::vector<fs::path> find_sub_dirs (const fs::path& root_path) {
    std::vector<fs::path> sub_dirs;
    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (entry.is_directory()) {
            sub_dirs.push_back(entry.path());
        }
    }
    return sub_dirs;
}

bool path_is_ascii(std::string path) {
    for (char c : path) {
        if (static_cast<unsigned char>(c) > 127) {
            return false;
        }
    }
    return true;
}

// Check if file meets the requirements to be analyzed and included in the db
// Files must exist, be a regular file, and have a .mp3 or .wav extension.
bool requires_processing (sqlite3* db, const fs::directory_entry *file) {
    if (file->is_regular_file() /*&& path_is_ascii(file->path().string()*/ && validate_file_extension(file) && 
            !db_entry_exists(db, "audio_files", to_string(file->path())))[[unlikely]]{
        return true;
    
    } else {
        return false;
    }
}

void queue_files(sqlite3 *db, const fs::path &dir_path, ThreadSafeQueue<fs::directory_entry> *proc_queue) {
    std::vector<std::thread> threads;
    std::mutex thread_mtx;
    int max_threads = 12;
    int thread_count = 0;

    try {
        for (const auto &entry : fs::directory_iterator(dir_path)) {
            if (requires_processing(db, &entry)) {
                proc_queue->push(entry);
            }
            else if (entry.is_directory()) {
                std::lock_guard<std::mutex> lock(thread_mtx);
                if (thread_count < max_threads) {
                    threads.emplace_back(&queue_files, db, entry.path(), proc_queue);
                    ++thread_count;
                }
                else {
                    queue_files(db, entry.path(), proc_queue);
                }
            }
        }

        for (auto &t : threads) {
            if (t.joinable()) {
                t.join();
                std::lock_guard<std::mutex> lock(thread_mtx);
                --thread_count;
            }
        }
    }
    catch (const std::exception &e) {
        fprintf(stderr, "queue_files: Exception caught: %s\n", e.what());
    }
    catch (...) {
        fprintf(stderr, "queue_files: Unknown exception caught\n");
    }
}

void queue_all_files (sqlite3 *db, const fs::path &dir_path, 
                ThreadSafeQueue<fs::directory_entry> *proc_queue ) {
    queue_files(db, dir_path, proc_queue);
    proc_queue->stop_producing();
}

void process_and_queue (sqlite3 *db, fs::directory_entry file, 
    ThreadSafeQueue<struct FileRecord *> *insrt_queue) {
    
    struct FileRecord *procd_file = process_file(db, file);
    insrt_queue->push(procd_file);
}

void process_queued_files (sqlite3 *db, 
        ThreadSafeQueue<fs::directory_entry> *proc_queue,
        ThreadSafeQueue<struct FileRecord *> *insrt_queue) {

    while (!proc_queue->empty() || proc_queue->is_producing()) {
        
        fs::directory_entry file;
        proc_queue->try_pop(file);
        
        if (!file.path().empty()) {
            struct FileRecord *procd_file = process_file(db, file);
            insrt_queue->push(procd_file);
        }
    }

    insrt_queue->stop_producing();
}

void insert_processed_files (sqlite3* db, 
    ThreadSafeQueue<struct FileRecord *> *insrt_queue) {
    
    while (insrt_queue->is_producing()) {
        if(insrt_queue->size() >= TRANSACTION_SIZE) {
            db_insert_files(db, insrt_queue);
        }
    }
    while (!insrt_queue->empty()) {
        db_insert_files(db, insrt_queue);
    }
}

// scan_directory scans, processes, and inserts audio files into the database
// Three threads are created and the following producer-consumer pipeline runs:
// 1. dir_path -> proc_queue
//    queue_all_files recursively drills down dir_pathchecking for
//    .mp3 and .wav files that are not in the database (see requires_processing)
//    Files that require processing are queued in proc_queue.
// 2. proc_queue -> insrt_queue
//    process_queued_files pops files from the queue as fs::directory_entry
//    objects, transforms them into struct FileRecord * objects, and pushes 
//    them in the insrt_queue.
// 3. insrt_queue -> database
//    insert_processed_files pops FileRecord objects off the insert queue and
//    inserts their data as entries in the database. Insertions are broken up
//    into transactions for faster insertion (see DBINT::db_insert_files)
void scan_directory (sqlite3 *db, const fs::path& dir_path) {
    
    ThreadSafeQueue<fs::directory_entry> proc_queue;
    proc_queue.start_producing();
    
    ThreadSafeQueue<struct FileRecord *> insrt_queue;
    insrt_queue.start_producing();

    std::vector<std::thread> threads;

    //fprintf(stderr, "queueing files\n");
    /*queue_all_files(db, dir_path, &proc_queue);
    fprintf(stderr, "processing queued files\n");
    process_queued_files(db, &proc_queue, &insrt_queue);
    fprintf(stderr, "inserting files\n");
    insert_processed_files(db, &insrt_queue);*/
    
    threads.emplace_back(&queue_all_files, db, dir_path, &proc_queue);
    threads.emplace_back(&process_queued_files, db, &proc_queue, &insrt_queue);
    threads.emplace_back(&insert_processed_files, db, &insrt_queue);

    // Join all threads
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}