#include "UIState.h"

void UIState::search (const char *query) {
	this->search_query = std::string(query);
}