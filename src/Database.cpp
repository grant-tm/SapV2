#include "..\inc\Database.h"

// checks if the given database table exists
bool db_table_valid (sqlite3* db, const std::string& table_name) {
    
    // prepare statement to select 1 element from db and table
    std::string sql = "SELECT 1 FROM " + table_name + " LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_table_valid: Failed to prepare statement.\n");
    }
    
    // execute statement: db table is valid if execution return matches row
    bool valid = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return valid;
}

// print the number of rows in a databse table
int db_get_num_rows (sqlite3 *db, const std::string& table_name) {

    // prepare statement to select the count of all rows in the table
    sqlite3_stmt* stmt;
    std::string sql = "SELECT COUNT(*) FROM " + table_name;  
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        panicf("db_print_num_rows: Failed to prepare statement.\n");
    }

    // execute the SELECT command
    if (sqlite3_step(stmt) == SQLITE_ROW) [[likely]] {
        int row_count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return row_count;
    } else [[unlikely]] {
        sqlite3_finalize(stmt);
        panicf("db_print_num_rows: Failed to execute SELECT COUNT(*)\n.");
    }
}

// prints the first n entries in a database table
void db_print_n_rows(sqlite3* db, const std::string& table_name, int num_rows) {
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT * FROM " + table_name + " LIMIT ?;";
    
    // prepare sql statement to select the first n entries in the databse
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_print_n_rows: Failed to prepare SELECT statement.\n");
    }

    // bind the limit value (number of entries to select) to the statement
    if (sqlite3_bind_int(stmt, 1, num_rows) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        panicf("db_print_n_rows: Failed to bind limit\n.");
    }

    // execute the SQL statement and print the result
    int columnCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < columnCount; ++col) {
            
            // select column name and content
            const char* col_name = sqlite3_column_name(stmt, col);
            const char* col_text = (const char*)sqlite3_column_text(stmt, col);
            
            // print the column contents
            fprintf(stderr, "%s: ", col_name);
            if (col_text) [[likely]] {
                fprintf(stderr, "%s ", col_name);
            } else {
                fprintf(stderr, "NULL ");
            }
        }
        fprintf(stderr, "\n");
    }
}

// determines if an entry already exists in a database table
// file paths are used as unique identifiers of table entries
bool db_entry_exists (sqlite3* db, const std::string& table_name, 
                    std::string file_path) {

    // prepare the SQL statement to select 1 entry with matching file_path
    sqlite3_stmt* stmt;
    std::string sql = "SELECT 1 FROM " + table_name + 
                      " WHERE file_path = ? LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_entry_exists: Failed to prepare SELECT statement\n");
    }

    // bind the file path to the select statment
    if (sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, 
        SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        panicf("db_entry_exists: Failed to bind values.\n");
    }

    // execute the SQL statement to check if the file exists
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

// set up the audio_files table if it doesn't already exist
void db_initialize (sqlite3 *db) {
    
    const char* sql = "CREATE TABLE IF NOT EXISTS audio_files ("\
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
                        "file_path TEXT UNIQUE,"\
                        "file_name TEXT NOT NULL,"\
                        "file_size INTEGER,"\
                        "duration REAL,"\
                        "num_user_tags INTEGER,"\
                        "user_tags TEXT NOT NULL,"\
                        "num_auto_tags INTEGER,"\
                        "auto_tags TEXT NOT NULL,"\
                        "user_bpm INTEGER,"\
                        "user_key INTEGER,"\
                        "auto_bpm INTEGER,"\
                        "auto_key INTEGER"\
                    ");";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        sqlite3_free(err_msg);
        panicf("db_initialize: Error creating table\n");
    }
}

// this function works in conjunction with db_insert_files to submit files in
// transactions. This allows the reuse of a sqlite3_stmt, which is much
// faster than inserting entries one at a time
void db_insert_file(sqlite3 *db, const struct FileRecord *file, 
                    sqlite3_stmt *stmt) {
    
    // bind the FileRecord data to the INSERT statement arguments
    sqlite3_bind_text(stmt, 1, file->file_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, file->file_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, file->file_size);
    sqlite3_bind_double(stmt, 4, file->duration);
    sqlite3_bind_int(stmt, 5, file->num_user_tags);
    sqlite3_bind_text(stmt, 6, file->user_tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, file->num_auto_tags);
    sqlite3_bind_text(stmt, 8, file->auto_tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, file->user_bpm);
    sqlite3_bind_int(stmt, 10, file->user_key);
    sqlite3_bind_int(stmt, 11, file->auto_bpm);
    sqlite3_bind_int(stmt, 12, file->auto_key);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        panicf("db_insert_file: Error inserting data.\n");
    }

    // reset the statement for reuse
    sqlite3_reset(stmt); 
}

// db_insert_files inserts entries in the audio_files database table
// data to insert comes from a vector of FileRecord structs
void db_insert_files (sqlite3 *db, ThreadSafeQueue<struct FileRecord *> *files) {

    // create statement to insert all members of explorer file struct
    const char* sql =   "INSERT OR IGNORE INTO audio_files ("\
                            "file_path,"\
                            "file_name,"\
                            "file_size,"\
                            "duration,"\
                            "num_user_tags,"\
                            "user_tags,"\
                            "num_auto_tags,"\
                            "auto_tags,"\
                            "user_bpm,"\
                            "user_key,"\
                            "auto_bpm,"\
                            "auto_key)"\
                            " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_insert_files: Error preparing statemen.\n");
    } 

    // insert files in a single transaction
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    while (!files->empty()) {
        struct FileRecord* file;
        files->wait_pop(file);
        db_insert_file(db, file, stmt);
        delete file;
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);
}

// Function to search files by name
std::vector<FileRecord> db_search_files_by_name (sqlite3* db, 
                                            const std::string& search_query) {
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT "\
                    "file_path, "\
                    "file_name, "\
                    "file_size, "\
                    "duration, "\
                    "num_user_tags, "\
                    "user_tags, "\
                    "num_auto_tags, "\
                    "auto_tags, "\
                    "user_bpm, "\
                    "user_key, "\
                    "auto_bpm, "\
                    "auto_key "\
                    "FROM audio_files WHERE file_name LIKE ?;";
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("search_files_by_name: Failed to prepare statement.\n");
    }
    
    // Bind the search query with wildcard characters for pattern matching
    std::string query_param = "%" + search_query + "%";
    sqlite3_bind_text(stmt, 1, query_param.c_str(), -1, SQLITE_STATIC);
    
    // Execute the statement and process the results
    std::vector<FileRecord> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        struct FileRecord file;
        
        file.file_path = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 0));
        file.file_name = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 1));
        file.file_size = sqlite3_column_int(stmt, 2);
        file.duration = sqlite3_column_int(stmt, 3);
        file.num_user_tags = sqlite3_column_int(stmt, 4);
        file.user_tags = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 5));
        file.num_auto_tags = sqlite3_column_int(stmt, 6);
        file.auto_tags = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 7));
        file.user_bpm = sqlite3_column_int(stmt, 8);
        file.user_key = sqlite3_column_int(stmt, 9);
        file.auto_bpm = sqlite3_column_int(stmt, 10);
        file.auto_key = sqlite3_column_int(stmt, 11);

        results.push_back(file);
    }
    
    sqlite3_finalize(stmt);
    return results;
}