#include "../../tools/ecode/plugins/aiassistant/localmodeldiscovery.hpp"
#include "utest.h"
#include <condition_variable>
#include <eepp/core/containers.hpp>
#include <eepp/system/threadpool.hpp>
#include <mutex>
#include <stdexcept>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;
using namespace ecode;

namespace {

Http::Response response( const std::string& body,
						 Http::Response::Status status = Http::Response::Ok ) {
	return Http::Response::createFakeResponse( {}, status, body );
}

LLMProviders providers() {
	LLMProviders result;
	for ( const auto* name : { "local", "other" } ) {
		auto& provider = result[name];
		provider.name = name;
		provider.fetchModelsUrl = "http://localhost/models";
		provider.models = LocalModelDiscovery::parse( name, R"({"data":[{"id":"old"}]})" ).value();
		LLMModel configured;
		configured.name = "configured";
		provider.models.emplace_back( std::move( configured ) );
	}
	return result;
}

} // namespace

UTEST( LocalModelDiscovery, parsesSupportedWireFormats ) {
	for (
		const auto* body :
		{ R"({"data":[{"model":"test","name":"Test","max_context_length":4096}]})",
		  R"({"models":[{"id":"test","display_name":"Test","max_context_length":4096}]})",
		  R"({"data":null,"models":[{"key":"test","name":"Test","max_context_length":4096}]})" } ) {
		auto result = LocalModelDiscovery::parse( "local", body );
		ASSERT_TRUE( result );
		ASSERT_EQ( 1u, result.value().size() );
		const auto& model = result.value().front();
		EXPECT_STREQ( "test", model.name.c_str() );
		EXPECT_STREQ( "local", model.provider.c_str() );
		ASSERT_TRUE( model.displayName );
		EXPECT_STREQ( "Test", model.displayName->c_str() );
		ASSERT_TRUE( model.maxOutputTokens );
		EXPECT_EQ( 4096u, *model.maxOutputTokens );
		EXPECT_TRUE( model.isEphemeral );
		EXPECT_EQ(
			hashCombine( std::hash<std::string>()( "test" ), std::hash<std::string>()( "local" ) ),
			model.hash );
	}
}

UTEST( LocalModelDiscovery, malformedDocumentIsFailureRatherThanEmptySuccess ) {
	for ( const auto* body : { "{", "null", "[]", "{}", R"({"data":{}})" } ) {
		auto result = LocalModelDiscovery::parse( "local", body );
		EXPECT_FALSE( result );
	}
	auto empty = LocalModelDiscovery::parse( "local", R"({"data":[]})" );
	ASSERT_TRUE( empty );
	EXPECT_TRUE( empty.value().empty() );
}

UTEST( LocalModelDiscovery, malformedEntriesAndOptionalFieldsAreSkipped ) {
	auto result = LocalModelDiscovery::parse(
		"local", R"({"data":[null,{"id":42},{"id":""},{"name":"Nameless"},
			{"id":"a","name":null,"display_name":"A","max_context_length":-1},
			{"id":"b","max_context_length":"4096"}]})" );
	ASSERT_TRUE( result );
	ASSERT_EQ( 2u, result.value().size() );
	const auto& a = result.value()[0];
	EXPECT_STREQ( "a", a.name.c_str() );
	ASSERT_TRUE( a.displayName );
	EXPECT_STREQ( "A", a.displayName->c_str() );
	EXPECT_FALSE( a.maxOutputTokens );
	EXPECT_STREQ( "b", result.value()[1].name.c_str() );
	EXPECT_FALSE( result.value()[1].maxOutputTokens );
}

UTEST( LocalModelDiscovery, mergesOncePreservingStaticAndFailedProviders ) {
	auto data = providers();
	auto endpoints = LocalModelDiscovery::snapshot( data );
	int calls = 0;
	auto batch = LocalModelDiscovery::discover( endpoints, {}, [&]( const std::string& ) {
		return ++calls == 1
				   ? response( R"({"models":[{"id":"new"},{"id":"new"},{"id":"configured"}]})" )
				   : response( "", Http::Response::ConnectionFailed );
	} );
	ASSERT_EQ( 2u, batch.size() );
	LocalModelDiscovery::apply( data, std::move( batch ) );
	ASSERT_EQ( 2u, data["local"].models.size() );
	EXPECT_STREQ( "configured", data["local"].models[0].name.c_str() );
	EXPECT_FALSE( data["local"].models[0].isEphemeral );
	EXPECT_STREQ( "new", data["local"].models[1].name.c_str() );
	EXPECT_STREQ( "old", data["other"].models[0].name.c_str() );
	LocalModelDiscovery::Batch empty;
	empty.push_back( { "local", LocalModelDiscovery::parse( "local", R"({"models":[]})" ) } );
	LocalModelDiscovery::apply( data, std::move( empty ) );
	ASSERT_EQ( 1u, data["local"].models.size() );
	EXPECT_STREQ( "configured", data["local"].models.front().name.c_str() );
}

UTEST( LocalModelDiscovery, snapshotSurvivesProviderReplacementAndIgnoresIneligibleProviders ) {
	auto data = providers();
	auto endpoints = LocalModelDiscovery::snapshot( data );
	data.clear();
	auto batch = LocalModelDiscovery::discover( endpoints, {}, []( const std::string& url ) {
		return response( url.empty() ? "{}" : R"({"data":[{"id":"new"}]})" );
	} );
	LocalModelDiscovery::apply( data, std::move( batch ) );
	EXPECT_TRUE( data.empty() );
	data = providers();
	data["local"].enabled = false;
	data["other"].fetchModelsUrl.reset();
	EXPECT_TRUE( LocalModelDiscovery::snapshot( data ).empty() );
	batch = LocalModelDiscovery::discover(
		endpoints, {}, []( const std::string& ) { return response( R"({"data":[]})" ); } );
	LocalModelDiscovery::apply( data, std::move( batch ) );
	EXPECT_EQ( 2u, data["local"].models.size() );
	EXPECT_EQ( 2u, data["other"].models.size() );
}

UTEST( LocalModelDiscovery, providerReplacementCarriesOverDiscoveredModels ) {
	auto previous = providers();
	auto replacement = providers();
	for ( auto& [_, provider] : replacement )
		provider.models.erase( provider.models.begin() ); // Catalogs carry no discovered models.
	replacement["other"].fetchModelsUrl = "http://localhost:1234/models";
	LocalModelDiscovery::carryOver( replacement, previous );
	ASSERT_EQ( 2u, replacement["local"].models.size() );
	EXPECT_STREQ( "configured", replacement["local"].models[0].name.c_str() );
	EXPECT_STREQ( "old", replacement["local"].models[1].name.c_str() );
	EXPECT_TRUE( replacement["local"].models[1].isEphemeral );
	// A changed endpoint may serve different models; wait for its own discovery.
	ASSERT_EQ( 1u, replacement["other"].models.size() );
	EXPECT_STREQ( "configured", replacement["other"].models[0].name.c_str() );
	// A restored, undiscovered model has the identity discovery will produce.
	EXPECT_EQ( previous["local"].models[0].hash,
			   LocalModelDiscovery::makeModel( "local", "old" ).hash );
}

UTEST( LocalModelDiscovery, cancellationBeforeAndDuringFetch ) {
	const auto endpoints = LocalModelDiscovery::snapshot( providers() );
	CancellationSource cancelled;
	cancelled.cancel();
	int calls = 0;
	auto batch =
		LocalModelDiscovery::discover( endpoints, cancelled.token(), [&]( const std::string& ) {
			++calls;
			return response( R"({"data":[]})" );
		} );
	EXPECT_TRUE( batch.empty() );
	EXPECT_EQ( 0, calls );

	// Hold a real worker inside the fetch while its owner cancels, without timing assumptions.
	ThreadPool pool( 1 );
	CancellationSource source;
	std::mutex mutex;
	std::condition_variable changed;
	bool entered = false;
	bool release = false;
	auto task = pool.submit( [&] {
		return LocalModelDiscovery::discover( endpoints, source.token(), [&]( const std::string& ) {
			std::unique_lock<std::mutex> lock( mutex );
			++calls;
			entered = true;
			changed.notify_one();
			changed.wait( lock, [&] { return release; } );
			return response( R"({"data":[{"id":"obsolete"}]})" );
		} );
	} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		changed.wait( lock, [&] { return entered; } );
		source.cancel();
		release = true;
	}
	changed.notify_one();
	task.wait();
	std::move( task ).then(
		[&]( LocalModelDiscovery::Batch&& result ) { batch = std::move( result ); } );
	EXPECT_TRUE( batch.empty() );
	EXPECT_EQ( 1, calls );
}

#if defined( __cpp_exceptions ) || defined( _CPPUNWIND )
UTEST( LocalModelDiscovery, fetchExceptionDoesNotEscapeWorker ) {
	auto batch = LocalModelDiscovery::discover( LocalModelDiscovery::snapshot( providers() ), {},
												[]( const std::string& ) -> Http::Response {
													throw std::runtime_error( "fetch failed" );
												} );
	ASSERT_EQ( 2u, batch.size() );
	EXPECT_FALSE( batch[0].models );
	EXPECT_TRUE( batch[0].models.error() == LocalModelDiscovery::Error::Http );
	EXPECT_FALSE( batch[1].models );
}
#endif
