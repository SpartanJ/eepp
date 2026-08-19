#pragma once

#include "protocol.hpp"

#include <atomic>
#include <cstdint>
#include <eepp/network/http.hpp>
#include <functional>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ecode {

class LLMModelCatalog {
  public:
	struct Settings {
		std::string cachePath;
		std::string url{ "https://models.dev/api.json" };
		std::uint32_t refreshIntervalHours{ 24 };
		bool enabled{ true };
	};

	explicit LLMModelCatalog( Settings settings );
	~LLMModelCatalog();
	void cancel();

	bool loadCached( LLMProviders& providers );

	std::uint64_t refreshAsync( LLMProviders providers,
								std::function<void( LLMProviders )> refreshedCallback );

	static std::optional<LLMReasoningConfiguration>
	parseReasoningConfiguration( const nlohmann::json& options );

  private:
	Settings mSettings;
	EE::Network::URI mRequestURI;
	EE::Network::URI mProxyURI;
	std::uint64_t mRequestId{ 0 };
	std::shared_ptr<std::atomic_bool> mCancelled{ std::make_shared<std::atomic_bool>( false ) };
	std::string mCachedData;
	std::string mCachedETag;
	bool mCachedCatalogValid{ false };

	bool applyCatalog( const std::string& data, LLMProviders& providers,
					   std::string* etag = nullptr ) const;
};

} // namespace ecode
