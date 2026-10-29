#pragma once

#include "protocol.hpp"
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/result.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/network/http.hpp>
#include <functional>

namespace ecode {

// Owns only discovery data: workers never borrow provider or widget storage.
class LocalModelDiscovery {
  public:
	struct Endpoint {
		std::string provider;
		std::string url;
	};

	enum class Error : EE::Uint8 { Http, InvalidResponse };

	using Models = EE::Result<std::vector<LLMModel>, Error>;

	struct ProviderResult {
		std::string provider;
		Models models;
	};

	using Endpoints = EE::SmallVector<Endpoint, 4>;
	using Batch = EE::SmallVector<ProviderResult, 4>;
	using Fetch = std::function<EE::Network::Http::Response( const std::string& )>;

	static bool isDiscoverable( const LLMProvider& provider );

	// Builds the identity of a discovered model, including models restored before discovery.
	static LLMModel makeModel( std::string provider, std::string name );

	static Endpoints snapshot( const LLMProviders& providers );

	// Cancellation returns the completed prefix. The UI generation gate rejects obsolete batches.
	static Batch discover( const Endpoints& endpoints, const EE::CancellationToken& token,
						   const Fetch& fetch = fetchEndpoint );

	// Only a malformed document fails; malformed entries and optional fields are skipped.
	static Models parse( const std::string& provider, const std::string& body );

	// UI thread only. Failed requests retain the last known models; empty success clears discovery.
	static void apply( LLMProviders& providers, Batch&& batch );

	// UI thread only. Keeps the last discovered models of providers whose endpoint is unchanged,
	// so a provider replacement does not hide them until the next discovery completes.
	static void carryOver( LLMProviders& providers, const LLMProviders& previous );

  private:
	static EE::Network::Http::Response fetchEndpoint( const std::string& url );
};

} // namespace ecode
