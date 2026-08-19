#include "llmmodelcatalog.hpp"

#include <eepp/network/http.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace EE;
using namespace EE::Network;

namespace ecode {

static constexpr int CATALOG_VERSION = 3;

static bool supportsTextChat( const json& model ) {
	if ( model.contains( "status" ) && model["status"].is_string() &&
		 model["status"].get<std::string>() == "deprecated" )
		return false;
	if ( !model.contains( "modalities" ) || !model["modalities"].is_object() )
		return true;
	const auto& modalities = model["modalities"];
	const auto hasText = []( const json& values ) {
		return values.is_array() &&
			   std::find( values.begin(), values.end(), "text" ) != values.end();
	};
	return hasText( modalities.value( "input", json::array() ) ) &&
		   hasText( modalities.value( "output", json::array() ) );
}

static json normalizeCatalog( const json& source ) {
	json result = { { "version", CATALOG_VERSION },
					{ "fetched_at", Sys::getUnixTimestamp() },
					{ "providers", json::object() } };
	for ( const auto& [providerId, provider] : source.items() ) {
		if ( !provider.is_object() || !provider.contains( "models" ) ||
			 !provider["models"].is_object() )
			continue;
		auto& output = result["providers"][providerId];
		const std::string displayName = provider.contains( "name" ) && provider["name"].is_string()
											? provider["name"].get<std::string>()
											: providerId;
		output = { { "display_name", displayName }, { "models", json::array() } };
		if ( provider.contains( "env" ) && provider["env"].is_array() )
			output["api_key_env_vars"] = provider["env"];
		const std::string npm = provider.contains( "npm" ) && provider["npm"].is_string()
									? provider["npm"].get<std::string>()
									: "";
		if ( ( npm == "@ai-sdk/openai-compatible" || providerId == "openrouter" ) &&
			 provider.contains( "api" ) && provider["api"].is_string() ) {
			std::string apiUrl = provider["api"].get<std::string>();
			if ( !String::endsWith( apiUrl, "/chat/completions" ) ) {
				if ( !String::endsWith( apiUrl, "/" ) )
					apiUrl += '/';
				apiUrl += "chat/completions";
			}
			output["api_url"] = std::move( apiUrl );
		}
		std::optional<std::pair<double, double>> cheapestCost;
		std::optional<std::size_t> cheapestModel;
		for ( const auto& [modelId, model] : provider["models"].items() ) {
			if ( !model.is_object() || !supportsTextChat( model ) )
				continue;
			const std::string displayName = model.contains( "name" ) && model["name"].is_string()
												? model["name"].get<std::string>()
												: modelId;
			const bool reasoning = model.contains( "reasoning" ) && model["reasoning"].is_boolean()
									   ? model["reasoning"].get<bool>()
									   : false;
			const bool toolCalling =
				model.contains( "tool_call" ) && model["tool_call"].is_boolean()
					? model["tool_call"].get<bool>()
					: false;
			json normalized = { { "name", modelId },
								{ "display_name", displayName },
								{ "reasoning", reasoning },
								{ "tool_calling", toolCalling } };
			if ( model.contains( "reasoning_options" ) && model["reasoning_options"].is_array() )
				normalized["reasoning_options"] = model["reasoning_options"];
			if ( model.contains( "limit" ) && model["limit"].is_object() ) {
				const auto& limit = model["limit"];
				if ( limit.contains( "context" ) && limit["context"].is_number_unsigned() )
					normalized["max_tokens"] = limit["context"];
				if ( limit.contains( "output" ) && limit["output"].is_number_unsigned() )
					normalized["max_output_tokens"] = limit["output"];
			}
			const std::size_t modelIndex = output["models"].size();
			output["models"].push_back( std::move( normalized ) );
			if ( model.contains( "cost" ) && model["cost"].is_object() ) {
				const auto& cost = model["cost"];
				if ( cost.contains( "input" ) && cost["input"].is_number() &&
					 cost.contains( "output" ) && cost["output"].is_number() ) {
					const std::pair<double, double> modelCost{ cost["input"].get<double>(),
															   cost["output"].get<double>() };
					if ( !cheapestCost || modelCost < *cheapestCost ) {
						cheapestCost = modelCost;
						cheapestModel = modelIndex;
					}
				}
			}
		}
		if ( cheapestModel )
			output["models"][*cheapestModel]["cheapest"] = true;
	}
	return result;
}

static bool writeAtomically( const std::string& path, const std::string& data ) {
	FileSystem::makeDir( FileSystem::fileRemoveFileName( path ), true );
	const std::string tempPath = path + ".tmp";
	if ( !FileSystem::fileWrite( tempPath, data ) )
		return false;
	if ( FileSystem::fileExists( path ) )
		FileSystem::fileRemove( path );
	if ( FileSystem::fileMove( tempPath, path ) )
		return true;
	FileSystem::fileRemove( tempPath );
	return false;
}

LLMModelCatalog::LLMModelCatalog( Settings settings ) : mSettings( std::move( settings ) ) {}

LLMModelCatalog::~LLMModelCatalog() {
	mCancelled->store( true );
	if ( mRequestId && Http::Pool::getGlobal().exists( mRequestURI, mProxyURI ) )
		Http::Pool::getGlobal().get( mRequestURI, mProxyURI )->setCancelRequest( mRequestId );
}

std::optional<LLMReasoningConfiguration>
LLMModelCatalog::parseReasoningConfiguration( const json& options ) {
	if ( !options.is_array() )
		return {};
	std::optional<LLMReasoningConfiguration> fallback;
	for ( const auto& option : options ) {
		if ( !option.is_object() || !option.contains( "type" ) || !option["type"].is_string() )
			continue;
		LLMReasoningConfiguration config;
		const std::string type = option["type"].get<std::string>();
		if ( type == "effort" && option.contains( "values" ) && option["values"].is_array() ) {
			config.type = LLMReasoningType::Effort;
			for ( const auto& effort : option["values"] )
				if ( effort.is_string() )
					config.efforts.emplace_back( effort.get<std::string>() );
			if ( !config.efforts.empty() )
				return config;
		} else if ( type == "budget_tokens" ) {
			config.type = LLMReasoningType::TokenBudget;
			if ( option.contains( "min" ) && option["min"].is_number_unsigned() )
				config.minBudgetTokens = option["min"].get<std::uint32_t>();
			if ( option.contains( "max" ) && option["max"].is_number_unsigned() )
				config.maxBudgetTokens = option["max"].get<std::uint32_t>();
			fallback = std::move( config );
		} else if ( type == "toggle" && !fallback ) {
			config.type = LLMReasoningType::Toggle;
			fallback = std::move( config );
		}
	}
	return fallback;
}

bool LLMModelCatalog::applyCatalog( const std::string& data, LLMProviders& providers ) const {
	const auto catalog = json::parse( data, nullptr, false, true );
	if ( !catalog.is_object() || !catalog.contains( "version" ) ||
		 !catalog["version"].is_number_integer() ||
		 catalog["version"].get<int>() != CATALOG_VERSION || !catalog.contains( "providers" ) ||
		 !catalog["providers"].is_object() )
		return false;
	LLMProviders catalogProviders;
	for ( const auto& [providerId, providerData] : catalog["providers"].items() ) {
		if ( !providerData.is_object() || !providerData.contains( "models" ) ||
			 !providerData["models"].is_array() )
			continue;
		const auto& models = providerData["models"];
		auto configuredProvider = providers.find( providerId );
		LLMProvider parsedProvider;
		if ( configuredProvider != providers.end() ) {
			// Keep transport-specific settings that models.dev does not describe, but the
			// catalog is authoritative for the model list and its metadata.
			parsedProvider = configuredProvider->second;
			parsedProvider.models.clear();
		} else {
			if ( !providerData.contains( "api_url" ) || !providerData["api_url"].is_string() )
				continue;
			parsedProvider.name = providerId;
			parsedProvider.apiUrl = providerData["api_url"].get<std::string>();
		}
		if ( providerData.contains( "display_name" ) && providerData["display_name"].is_string() )
			parsedProvider.displayName = providerData["display_name"].get<std::string>();
		if ( providerData.contains( "api_url" ) && providerData["api_url"].is_string() )
			parsedProvider.apiUrl = providerData["api_url"].get<std::string>();
		if ( providerData.contains( "api_key_env_vars" ) &&
			 providerData["api_key_env_vars"].is_array() ) {
			parsedProvider.apiKeyEnvVars.clear();
			for ( const auto& env : providerData["api_key_env_vars"] )
				if ( env.is_string() )
					parsedProvider.apiKeyEnvVars.emplace_back( env.get<std::string>() );
		}
		std::vector<LLMModel> parsed;
		parsed.reserve( models.size() );
		for ( const auto& value : models ) {
			if ( !value.is_object() || !value.contains( "name" ) || !value["name"].is_string() )
				continue;
			LLMModel model;
			model.name = value["name"].get<std::string>();
			model.provider = providerId;
			if ( value.contains( "display_name" ) && value["display_name"].is_string() )
				model.displayName = value["display_name"].get<std::string>();
			if ( value.contains( "max_tokens" ) && value["max_tokens"].is_number_unsigned() )
				model.maxTokens = value["max_tokens"].get<std::size_t>();
			if ( value.contains( "max_output_tokens" ) &&
				 value["max_output_tokens"].is_number_unsigned() )
				model.maxOutputTokens = value["max_output_tokens"].get<std::size_t>();
			model.reasoning = value.contains( "reasoning" ) && value["reasoning"].is_boolean()
								  ? value["reasoning"].get<bool>()
								  : false;
			model.toolCalling =
				value.contains( "tool_calling" ) && value["tool_calling"].is_boolean()
					? value["tool_calling"].get<bool>()
					: false;
			model.cheapest = value.contains( "cheapest" ) && value["cheapest"].is_boolean()
								 ? value["cheapest"].get<bool>()
								 : false;
			if ( value.contains( "reasoning_options" ) )
				model.reasoningConfiguration =
					parseReasoningConfiguration( value["reasoning_options"] );
			model.hash = hashCombine( std::hash<std::string>()( model.name ),
									  std::hash<std::string>()( model.provider ) );
			parsed.emplace_back( std::move( model ) );
		}
		if ( !parsed.empty() ) {
			parsedProvider.models = std::move( parsed );
			catalogProviders.insert_or_assign( providerId, std::move( parsedProvider ) );
		}
	}
	// Local providers discover their models from the running local server and therefore
	// have no useful models.dev catalog entries.
	for ( auto& [providerId, provider] : providers )
		if ( provider.fetchModelsUrl &&
			 catalogProviders.find( providerId ) == catalogProviders.end() )
			catalogProviders.insert_or_assign( providerId, std::move( provider ) );
	if ( catalogProviders.empty() )
		return false;
	providers = std::move( catalogProviders );
	return true;
}

bool LLMModelCatalog::loadCached( LLMProviders& providers ) const {
	if ( !mSettings.enabled )
		return false;
	std::string data;
	return FileSystem::fileGet( mSettings.cachePath, data ) && applyCatalog( data, providers );
}

std::uint64_t
LLMModelCatalog::refreshAsync( LLMProviders providers,
							   std::function<void( LLMProviders )> refreshedCallback ) {
	if ( !mSettings.enabled || mSettings.url.empty() )
		return 0;
	Clock clock;
	std::string cached;
	json cache;
	if ( FileSystem::fileGet( mSettings.cachePath, cached ) )
		cache = json::parse( cached, nullptr, false, true );
	const bool validCache = cache.is_object() && cache.contains( "version" ) &&
							cache["version"].is_number_integer() &&
							cache["version"].get<int>() == CATALOG_VERSION;
	const std::int64_t fetchedAt =
		validCache && cache.contains( "fetched_at" ) && cache["fetched_at"].is_number_integer()
			? cache["fetched_at"].get<std::int64_t>()
			: 0;
	const std::int64_t maxAge =
		static_cast<std::int64_t>( mSettings.refreshIntervalHours ) * 60 * 60;
	if ( fetchedAt > 0 && Sys::getUnixTimestamp() - fetchedAt < maxAge )
		return 0;

	Http::Request::FieldTable headers;
	if ( validCache ) {
		const std::string etag = cache.contains( "etag" ) && cache["etag"].is_string()
									 ? cache["etag"].get<std::string>()
									 : "";
		if ( !etag.empty() )
			headers["If-None-Match"] = etag;
	}
	const Settings settings = mSettings;
	mCancelled->store( false );
	mRequestURI = URI( mSettings.url );
	mProxyURI = Http::getEnvProxyURI();
	mRequestId = Http::getAsync(
		[settings, cache = std::move( cache ), providers = std::move( providers ),
		 refreshedCallback = std::move( refreshedCallback ), clock,
		 cancelled = mCancelled]( const Http&, Http::Request&, Http::Response& response ) mutable {
			if ( cancelled->load() )
				return;
			if ( response.getStatus() == Http::Response::Status::NotModified ) {
				if ( cancelled->load() )
					return;
				cache["fetched_at"] = Sys::getUnixTimestamp();
				writeAtomically( settings.cachePath, cache.dump() );
				return;
			}
			if ( response.getStatus() != Http::Response::Status::Ok ) {
				Log::warning( "LLM model catalog refresh failed with HTTP status %d",
							  static_cast<int>( response.getStatus() ) );
				return;
			}

			if ( cancelled->load() )
				return;
			const auto source = json::parse( response.getBody(), nullptr, false, true );

			if ( !source.is_object() ) {
				Log::warning( "LLM model catalog refresh returned invalid JSON" );
				return;
			}

			if ( cancelled->load() )
				return;
			auto normalized = normalizeCatalog( source );
			normalized["etag"] = response.getField( "ETag" );
			const std::string data = normalized.dump();
			LLMModelCatalog catalog( settings );
			if ( !catalog.applyCatalog( data, providers ) )
				return;
			if ( cancelled->load() )
				return;
			if ( !writeAtomically( settings.cachePath, data ) )
				Log::warning( "Could not persist LLM model catalog cache to %s",
							  settings.cachePath );
			Log::info( "LLMModelCatalog::refresh just refresh models.dev catalog, took %s",
					   clock.getElapsedTime().toString() );
			if ( !cancelled->load() && refreshedCallback )
				refreshedCallback( std::move( providers ) );
		},
		mRequestURI, Seconds( 10 ), {}, headers, "", true, mProxyURI );
	return mRequestId;
}

} // namespace ecode
