#ifndef DATABASE_H
#define DATABASE_H

// Standard Library Inclusions
#include <iostream>
#include <string>
#include <filesystem>

// External Inclusions
#include "sqlite3.h"

// Project Inclusions
#include "SystemUtilities.h"
#include "ThreadSafeQueue.h"
#include "FileRecord.h"

// definitions
namespace fs = std::filesystem;

// Checks if the given database table exists
bool db_table_valid(sqlite3* db, const std::string& table_name);

// Print the number of rows in a database table
int db_get_num_rows(sqlite3* db, const std::string& table_name);

// Prints the first n entries in a database table
void db_print_n_rows(sqlite3* db, const std::string& table_name, int num_rows);

// Determines if an entry already exists in a database table
// File paths are used as unique identifiers of table entries
bool db_entry_exists(sqlite3* db, const std::string& table_name, std::string file_path);

// Set up the audio_files table if it doesn't already exist
void db_initialize(sqlite3* db);

// This function works in conjunction with db_insert_files to submit files in
// transactions. This allows the reuse of a sqlite3_stmt, which is much
// faster than inserting entries one at a time
void db_insert_file(sqlite3* db, const struct FileRecord* file, sqlite3_stmt* stmt);

// Inserts entries in the audio_files database table
// Data to insert comes from a vector of FileRecord structs
void db_insert_files(sqlite3* db, ThreadSafeQueue<struct FileRecord*>* files);

// Function to search files by name
std::vector<FileRecord> db_search_files_by_name(sqlite3* db, const std::string& search_query);

#endif // DATABASE_H