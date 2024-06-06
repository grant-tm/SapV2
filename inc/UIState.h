
#ifndef UISTATE_H
#define UISTATE_H

#include <string>
#include <vector>

#include "Database.h"
#include "ThreadSafeQueue.h"
#include "FileRecord.h"

class UIState {
public:
    sqlite3 *db = nullptr;
    std::string search_query;
    std::vector<struct FileRecord> files;

public:
    void search (const char *query);

};

#endif //UISTATE_H