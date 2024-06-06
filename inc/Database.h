#ifndef DATABASE_H
#define DATABASE_H

// Standard Library Inclusions
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>

// External Inclusions
#include "sqlite3.h"

// Project Inclusions
#include "SystemUtilities.h"
#include "ThreadSafeQueue.h"
#include "FileRecord.h"

// definitions
namespace fs = std::filesystem;

char *concat_cstrs(int num_strings, ...);

class Database {
private:
	sqlite3 *db;
public:
	
	Database (void);
	Database (const char *db_name);
	
	void init (void);
	
	bool table_is_valid (const char *table_name);
	int num_rows(const char *table_name);

	bool entry_exists (const char *table_name, const char *file_path);

	void insert_file  (struct FileRecord *file);
	void insert_files (ThreadSafeQueue<struct FileRecord *> *files);

	std::vector<struct FileRecord> search_by_name (const char *query);
};

#endif // DATABASE_H