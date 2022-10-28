#ifndef ECODE_LSPCLIENTMANAGER_HPP
#define ECODE_LSPCLIENTMANAGER_HPP

#include "../pluginmanager.hpp"
#include "lspclient.hpp"
#include <eepp/core.hpp>

using namespace EE;

namespace ecode {

class LSPClientManager {
  public:
	LSPClientManager();

	void load( const PluginManager* pluginManager );

	size_t clientCount() const;

  protected:
	std::map<String::HashType, std::unique_ptr<LSPClient>> mClients;
};

} // namespace ecode

#endif // ECODE_LSPCLIENTMANAGER_HPP
