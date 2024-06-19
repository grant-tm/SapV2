#ifndef DATABASE_H
#define DATABASE_H

// Standard Library Inclusions
#include <iostream>
#include <windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <codecvt>
#include <locale>

// External Inclusions
#include "sqlite3.h"

// Project Inclusions
#include "SystemUtilities.h"
#include "ThreadSafeQueue.h"
#include "FileRecord.h"

// definitions
namespace fs = std::filesystem;

char *concat_cstrs(int num_strings, ...);
const char *wchar_to_char(const wchar_t *);

class Database {
public:
	
	Database (void);
	Database (const char *db_name);
	
	void init (void);
	
	bool table_is_valid (const char *table_name);
	int num_rows (const char *table_name);

	bool entry_exists (const char *table_name, std::wstring *file_path);

	void insert_file  (struct FileRecord *file);
	void insert_files (ThreadSafeQueue<struct FileRecord *> *files);

	void search_by_name (std::vector<struct FileRecord> *serach_result, const char *query);

private:

	sqlite3 *db;

};

#endif // DATABASE_H