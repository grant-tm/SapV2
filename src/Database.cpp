#include "Database.h"

//-----------------------------------------------------------------------------
// concat_cstrs
// ----------------------------------------------------------------------------
// Concatenate num_strings c-style strings in order.
// This function allocates memory that must be freed.
//-----------------------------------------------------------------------------
char *concat_cstrs
(
    int num_strings, 
    ...
){
    // calculate total length of final string
    va_list args;
    int total_length = 0;
    va_start(args, num_strings);
    for (int i = 0; i < num_strings; i++) {
        const char *str = va_arg(args, const char *);
        total_length += strlen(str);
    }
    va_end(args);

    // allocate memory for the resulting string
    char *result = new char[total_length + 1];
    result[0] = '\0';

    // Concatenate all strings into the result
    va_start(args, num_strings);
    for (int i = 0; i < num_strings; ++i) {
        const char *str = va_arg(args, const char *);
        strcat_s(result, total_length + 1, str);
    }
    va_end(args);

    return result;
}

//-----------------------------------------------------------------------------
// Database::Database (default)
// ----------------------------------------------------------------------------
// Default constructor for a Database.
// This is not intended to be used.
//-----------------------------------------------------------------------------
Database::Database
(
    void
){
    this->db = nullptr;
}

//-----------------------------------------------------------------------------
// Database::Database (const char *)
// ----------------------------------------------------------------------------
// Opens or creates an sqlite database named db_name.
//-----------------------------------------------------------------------------
Database::Database
(
    const char *db_name
){
    int flags = SQLITE_OPEN_READWRITE | 
                SQLITE_OPEN_CREATE | 
                SQLITE_OPEN_FULLMUTEX;
    
    if (sqlite3_open_v2(db_name, &this->db, flags, NULL) == SQLITE_OK) {
        this->init();
    } 
    else {
        errlog("Database::Database: Cannot open database.\n");
        this->db = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Database::init
// ----------------------------------------------------------------------------
// Sets up the audio_files table if it doesn't already exist.
//-----------------------------------------------------------------------------
void
Database::init
(
    void
){
    const char *sql =
        "CREATE TABLE IF NOT EXISTS audio_files"\
        "("\
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

    char *err_msg = nullptr;
    if (sqlite3_exec(this->db, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        sqlite3_free(err_msg);
        errlog("Database::init: Error creating table.\n");
    }
}

//-----------------------------------------------------------------------------
// Database::table_is_valid
// ----------------------------------------------------------------------------
// Checks if a database table exists.
//-----------------------------------------------------------------------------
bool 
Database::table_is_valid 
(
    const char *table_name
){
    // prepare statement to select 1 element from db and table
    char *sql = concat_cstrs(3, "SELECT 1 FROM ", table_name, " LIMIT 1;");
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        errlog("Database::table_is_valid: Failed to prepare statement.\n");
    }
    delete[] sql;
    
    // execute statement: db table is valid if execution return matches row
    bool valid = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return valid;
}

//-----------------------------------------------------------------------------
// Database::num_rows
// ----------------------------------------------------------------------------
// Returns the number of rows in a database table.
//-----------------------------------------------------------------------------
int 
Database::num_rows
( 
    const char *table_name
){
    // prepare statement to select the count of all rows in the table
    sqlite3_stmt* stmt;
    char *sql = concat_cstrs(2, "SELECT COUNT(*) FROM ", table_name);
    if (sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        errlog("Database::num_rows: Failed to prepare statement.\n");
    }
    delete[] sql;

    // execute the SELECT command
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int row_count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return row_count;
    } else {
        sqlite3_finalize(stmt);
        errlog("Database::num_rows: Failed to execute sql statement.\n");
        return 0;
    }
}

//-----------------------------------------------------------------------------
// Database::entry_exists
// ----------------------------------------------------------------------------
// determines if an entry already exists in a database table file paths are 
// used as unique identifiers of table entries
//-----------------------------------------------------------------------------
bool 
Database::entry_exists
(
    const char *table_name,
    const char *file_path
){

    // prepare the SQL statement to select 1 entry with matching file_path
    sqlite3_stmt* stmt;
    char *sql = concat_cstrs(
        3, 
        "SELECT 1 FROM ", 
        table_name, 
        " WHERE file_path = ? LIMIT 1;"
    );
    if (sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        errlog("Database::entry_exists: Failed to prepare SELECT statement\n");
    }
    delete[] sql;

    // bind the file path to the select statment
    int result = sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_TRANSIENT);
    if (result != SQLITE_OK) {
        errlog("Database:entry_exists: Failed to bind values.\n");
    }

    // execute the SQL statement to check if the file exists
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

//-----------------------------------------------------------------------------
// Database::insert_file
// ----------------------------------------------------------------------------
// This function works inserts a single file into the database
//-----------------------------------------------------------------------------
void 
Database::insert_file(
    struct FileRecord *file
){
    // create statement to insert all members of explorer file struct
    const char *sql = "INSERT OR IGNORE INTO audio_files ("\
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
        "auto_key"\
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        errlog("Database::insert_files: Error preparing statement.\n");
        return;
    }

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
        errlog("Database::insert_file: Error inserting data.\n");
    }

    // reset the statement for reuse
    sqlite3_reset(stmt); 
}

//-----------------------------------------------------------------------------
// Database::insert_files
// ----------------------------------------------------------------------------
// db_insert_files inserts entries in the audio_files database table data to 
// insert comes from a vector of FileRecord structs
//-----------------------------------------------------------------------------
void 
Database::insert_files (
    ThreadSafeQueue<struct FileRecord *> *files
) {
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
                        "auto_key"\
                        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        panicf("db_insert_files: Error preparing statement.\n");
    } 

    // insert files in a single transaction
    sqlite3_exec(this->db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    while (!files->empty()) {
        
        struct FileRecord* file;
        files->wait_pop(file);
        
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
            fprintf(stderr, "db_insert_file: Error inserting data.\n");
        }
        sqlite3_reset(stmt);
        
        delete file;
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);
}

//-----------------------------------------------------------------------------
// db_search_files_by_name
// ----------------------------------------------------------------------------
// Function to search files by name
//-----------------------------------------------------------------------------
std::vector<struct FileRecord>
Database::search_by_name (
    const char *query
){
    // prepare sql statement
    sqlite3_stmt* stmt;
    const char *sql =   "SELECT "\
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
    
    if (sqlite3_prepare_v2(this->db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        errlog("Database::search_by_name: Failed to prepare sql statement.\n");
    }
    
    // Bind the search query with wildcard characters for pattern matching
    const char *query_param = concat_cstrs(3, "%", query, "%");
    int result = sqlite3_bind_text(stmt, 1, query_param, -1, SQLITE_STATIC);
    if (result != SQLITE_OK) {
        errlog("Database::search_by_name: Failed to bind sql statement.\n");
    }
    delete[] query_param;

    // Execute the statement and process the results
    std::vector<FileRecord> search_result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        struct FileRecord file;
        
        file.file_path = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, 0)
                         );
        file.file_name = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, 1)
                         );
        file.file_size = sqlite3_column_int(stmt, 2);
        file.duration = sqlite3_column_int(stmt, 3);
        file.num_user_tags = sqlite3_column_int(stmt, 4);
        file.user_tags = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, 5)
                        );
        file.num_auto_tags = sqlite3_column_int(stmt, 6);
        file.auto_tags = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, 7)
                         );
        file.user_bpm = sqlite3_column_int(stmt, 8);
        file.user_key = sqlite3_column_int(stmt, 9);
        file.auto_bpm = sqlite3_column_int(stmt, 10);
        file.auto_key = sqlite3_column_int(stmt, 11);

        search_result.push_back(file);
    }
    
    sqlite3_finalize(stmt);
    return search_result;
}