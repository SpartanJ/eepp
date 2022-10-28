#include "lspclientmanager.hpp"

namespace ecode {

LSPClientManager::LSPClientManager() {}

void LSPClientManager::load( const PluginManager* pluginManager ) {}

size_t LSPClientManager::clientCount() const {
	return mClients.size();
}

} // namespace ecode
