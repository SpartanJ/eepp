#include "localmodeldiscovery.hpp"
#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/system/time.hpp>
#include <exception>
#include <limits>
#include <nlohmann/json.hpp>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

namespace ecode {

namespace {

const nlohmann::json* findString( const nlohmann::json& entry, const char* key ) {
	auto it = entry.find( key );
	return it != entry.end() && it->is_string() ? &*it : nullptr;
}

void replaceDiscoveredModels( std::vector<LLMModel>& models, std::vector<LLMModel>&& discovered ) {
	models.erase( std::remove_if( models.begin(), models.end(),
								  []( const LLMModel& model ) { return model.isEphemeral; } ),
				  models.end() );
	for ( auto& model : discovered ) {
		// Configured models win over discovered entries, including duplicates in the response.
		if ( std::none_of( models.begin(), models.end(), [&]( const LLMModel& existing ) {
				 return existing.name == model.name;
			 } ) )
			models.emplace_back( std::move( model ) );
	}
}

} // namespace

bool LocalModelDiscovery::isDiscoverable( const LLMProvider& provider ) {
	return provider.enabled && provider.fetchModelsUrl.has_value();
}

LLMModel LocalModelDiscovery::makeModel( std::string provider, std::string name ) {
	LLMModel model;
	model.hash =
		hashCombine( std::hash<std::string>()( name ), std::hash<std::string>()( provider ) );
	model.name = std::move( name );
	model.provider = std::move( provider );
	model.isEphemeral = true;
	return model;
}

LocalModelDiscovery::Endpoints LocalModelDiscovery::snapshot( const LLMProviders& providers ) {
	Endpoints endpoints;
	for ( const auto& [name, provider] : providers ) {
		if ( isDiscoverable( provider ) )
			endpoints.push_back( { name, *provider.fetchModelsUrl } );
	}
	return endpoints;
}

Http::Response LocalModelDiscovery::fetchEndpoint( const std::string& url ) {
	return Http::get( url, Milliseconds( 100 ) );
}

LocalModelDiscovery::Batch LocalModelDiscovery::discover( const Endpoints& endpoints,
														  const CancellationToken& token,
														  const Fetch& fetch ) {
	Batch batch;
	for ( const auto& endpoint : endpoints ) {
		if ( token.isCancellationRequested() )
			break;
		// The parser does not throw on malformed input; only the request can escape the worker.
#if defined( __cpp_exceptions ) || defined( _CPPUNWIND )
		Http::Response response;
		try {
			response = fetch( endpoint.url );
		} catch ( const std::exception& ) {
			batch.push_back( { endpoint.provider, Err( Error::Http ) } );
			continue;
		}
#else
		auto response = fetch( endpoint.url );
#endif
		if ( token.isCancellationRequested() )
			break;
		batch.push_back( { endpoint.provider, response.getStatus() == Http::Response::Ok
												  ? parse( endpoint.provider, response.getBody() )
												  : Models( Err( Error::Http ) ) } );
	}
	return batch;
}

LocalModelDiscovery::Models LocalModelDiscovery::parse( const std::string& provider,
														const std::string& body ) {
	const auto json = nlohmann::json::parse( body, nullptr, false, true );
	if ( !json.is_object() )
		return Err( Error::InvalidResponse );
	auto data = json.find( "data" );
	if ( data == json.end() || !data->is_array() )
		data = json.find( "models" );
	if ( data == json.end() || !data->is_array() )
		return Err( Error::InvalidResponse );

	std::vector<LLMModel> models;
	models.reserve( data->size() );
	for ( const auto& entry : *data ) {
		if ( !entry.is_object() )
			continue;
		const auto* name = findString( entry, "model" );
		if ( !name )
			name = findString( entry, "id" );
		if ( !name )
			name = findString( entry, "key" );
		if ( !name || name->get_ref<const std::string&>().empty() )
			continue;

		auto model = makeModel( provider, name->get_ref<const std::string&>() );
		const auto* displayName = findString( entry, "name" );
		if ( !displayName )
			displayName = findString( entry, "display_name" );
		if ( displayName )
			model.displayName = displayName->get_ref<const std::string&>();
		auto context = entry.find( "max_context_length" );
		if ( context != entry.end() && context->is_number_unsigned() &&
			 context->get<std::uint64_t>() <= std::numeric_limits<std::size_t>::max() )
			model.maxOutputTokens = static_cast<std::size_t>( context->get<std::uint64_t>() );
		models.emplace_back( std::move( model ) );
	}
	return Ok( std::move( models ) );
}

void LocalModelDiscovery::apply( LLMProviders& providers, Batch&& batch ) {
	for ( auto& result : batch ) {
		auto it = providers.find( result.provider );
		if ( !result.models || it == providers.end() || !isDiscoverable( it->second ) )
			continue;
		replaceDiscoveredModels( it->second.models, std::move( result.models.value() ) );
	}
}

void LocalModelDiscovery::carryOver( LLMProviders& providers, const LLMProviders& previous ) {
	for ( auto& [name, provider] : providers ) {
		auto old = previous.find( name );
		if ( old == previous.end() || !isDiscoverable( provider ) ||
			 old->second.fetchModelsUrl != provider.fetchModelsUrl )
			continue;
		std::vector<LLMModel> discovered;
		for ( const auto& model : old->second.models ) {
			if ( model.isEphemeral )
				discovered.push_back( model );
		}
		if ( !discovered.empty() )
			replaceDiscoveredModels( provider.models, std::move( discovered ) );
	}
}

} // namespace ecode
