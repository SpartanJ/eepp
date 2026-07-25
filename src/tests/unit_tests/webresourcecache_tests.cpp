#include "utest.hpp"

#include <eepp/ui/webresourcecache.hpp>

#include <vector>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;
using namespace EE::UI;

namespace {

Http::Response response( Http::Response::Status status, const std::string& body = {} ) {
	Http::Response::FieldTable fields;
	return Http::Response::createFakeResponse( fields, status, body );
}

WebResourceRequest requestFor( const char* uri ) {
	WebResourceRequest request;
	request.uri = URI( uri );
	request.kind = WebResourceKind::StyleSheet;
	return request;
}

} // namespace

UTEST( WebResourceCache, coalescesSubscribersInOnePartition ) {
	auto cache = WebResourceCache::New();
	std::vector<WebResourceCache::FetchCompletion> completions;
	int fetches = 0;
	cache->setFetcher( [&fetches, &completions]( const WebResourceRequest&,
												 WebResourceCache::FetchCompletion completion ) {
		++fetches;
		completions.emplace_back( std::move( completion ) );
	} );

	constexpr CachePartitionId partition = 7;
	auto first = cache->createSession( partition );
	auto second = cache->createSession( partition );
	auto firstGeneration = cache->beginNavigation( first, URI( "https://example.com/a" ) );
	auto secondGeneration = cache->beginNavigation( second, URI( "https://example.com/b" ) );
	int deliveries = 0;
	bool allSuccessful = true;
	cache->requestData( first, firstGeneration, requestFor( "https://example.com/site.css#one" ),
						[&deliveries, &allSuccessful]( const WebResourceResult& result ) {
							allSuccessful &= result.success;
							++deliveries;
						} );
	cache->requestData( second, secondGeneration, requestFor( "https://example.com/site.css#two" ),
						[&deliveries, &allSuccessful]( const WebResourceResult& result ) {
							allSuccessful &= result.success;
							++deliveries;
						} );

	EXPECT_EQ( 1, fetches );
	EXPECT_EQ( 1u, completions.size() );
	auto ok = Http::Response::Status::Ok;
	completions.front()( response( ok, "body" ) );
	EXPECT_EQ( 2, deliveries );
	EXPECT_TRUE( allSuccessful );
	cache->requestData( first, firstGeneration, requestFor( "https://example.com/site.css" ),
						[&deliveries]( const WebResourceResult& ) { ++deliveries; } );
	EXPECT_EQ( 1, fetches );
	EXPECT_EQ( 3, deliveries );
	EXPECT_EQ( 0u, cache->getInFlightCount() );
}

UTEST( WebResourceCache, staleDocumentSubscriberDoesNotBlockCurrentDocument ) {
	auto cache = WebResourceCache::New();
	WebResourceCache::FetchCompletion completion;
	cache->setFetcher(
		[&completion]( const WebResourceRequest&, WebResourceCache::FetchCompletion cb ) {
			completion = std::move( cb );
		} );
	auto session = cache->createSession( 11 );
	auto oldGeneration = cache->beginNavigation( session, URI( "https://example.com/old" ) );
	int staleDeliveries = 0;
	int currentDeliveries = 0;
	cache->requestData( session, oldGeneration, requestFor( "https://example.com/shared.css" ),
						[&staleDeliveries]( const WebResourceResult& ) { ++staleDeliveries; } );
	auto currentGeneration =
		cache->beginNavigation( session, URI( "https://example.com/current" ) );
	cache->requestData( session, currentGeneration, requestFor( "https://example.com/shared.css" ),
						[&currentDeliveries]( const WebResourceResult& ) { ++currentDeliveries; } );
	auto ok = Http::Response::Status::Ok;
	completion( response( ok, "body" ) );
	EXPECT_EQ( 0, staleDeliveries );
	EXPECT_EQ( 1, currentDeliveries );
}

UTEST( WebResourceCache, clearRejectsStaleCompletionForReplacementEntry ) {
	auto cache = WebResourceCache::New();
	std::vector<WebResourceCache::FetchCompletion> completions;
	cache->setFetcher(
		[&completions]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			completions.emplace_back( std::move( completion ) );
		} );
	auto session = cache->createSession( 12 );
	auto generation = cache->beginNavigation( session, URI( "https://example.com/" ) );
	auto request = requestFor( "https://example.com/shared.css" );
	int staleDeliveries = 0;
	std::string deliveredBody;

	cache->requestData( session, generation, request,
						[&staleDeliveries]( const WebResourceResult& ) { ++staleDeliveries; } );
	cache->clear();
	cache->requestData( session, generation, request,
						[&deliveredBody]( const WebResourceResult& result ) {
							if ( result.data )
								deliveredBody = *result.data;
						} );

	ASSERT_EQ( 2u, completions.size() );
	EXPECT_EQ( 1u, cache->getEntryCount() );
	EXPECT_EQ( 1u, cache->getInFlightCount() );
	auto ok = Http::Response::Status::Ok;
	completions[0]( response( ok, "stale" ) );
	EXPECT_EQ( 0, staleDeliveries );
	EXPECT_TRUE( deliveredBody.empty() );
	EXPECT_EQ( 1u, cache->getInFlightCount() );

	completions[1]( response( ok, "fresh" ) );
	EXPECT_TRUE( deliveredBody == "fresh" );
	EXPECT_EQ( 0u, cache->getInFlightCount() );

	int fetches = static_cast<int>( completions.size() );
	cache->requestData( session, generation, request,
						[&deliveredBody]( const WebResourceResult& result ) {
							if ( result.data )
								deliveredBody = *result.data;
						} );
	EXPECT_EQ( fetches, static_cast<int>( completions.size() ) );
	EXPECT_TRUE( deliveredBody == "fresh" );
}

UTEST( WebResourceCache, partitionsDoNotShareEntries ) {
	auto cache = WebResourceCache::New();
	int fetches = 0;
	cache->setFetcher(
		[&fetches]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			++fetches;
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "body" ) );
		} );
	auto first = cache->createSession( 1 );
	auto second = cache->createSession( 2 );
	auto firstGeneration = cache->beginNavigation( first, URI( "https://example.com/a" ) );
	auto secondGeneration = cache->beginNavigation( second, URI( "https://example.com/b" ) );
	cache->requestData( first, firstGeneration, requestFor( "https://example.com/site.css" ), {} );
	cache->requestData( second, secondGeneration, requestFor( "https://example.com/site.css" ),
						{} );
	EXPECT_EQ( 2, fetches );
}

UTEST( WebResourceCache, decodeAndRequestVariantsUseDistinctEntries ) {
	auto cache = WebResourceCache::New();
	int fetches = 0;
	cache->setFetcher(
		[&fetches]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			++fetches;
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "body" ) );
		} );
	auto session = cache->createSession( 5 );
	auto generation = cache->beginNavigation( session, URI( "https://example.com/" ) );
	auto base = requestFor( "https://example.com/resource" );
	cache->requestData( session, generation, base, {} );
	base.svgScale = 2.f;
	cache->requestData( session, generation, base, {} );
	base.headers["Accept-Language"] = "es";
	cache->requestData( session, generation, base, {} );
	EXPECT_EQ( 3, fetches );
}

UTEST( WebResourceCache, navigationHeadersDoNotDuplicateRetainedEntries ) {
	auto cache = WebResourceCache::New();
	int fetches = 0;
	cache->setFetcher(
		[&fetches]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			++fetches;
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "body" ) );
		} );
	auto session = cache->createSession( 9 );
	auto generation = cache->beginNavigation( session, URI( "https://news.example/" ) );
	auto request = requestFor( "https://news.example/logo.png" );
	request.headers["referer"] = "https://news.example/";
	cache->requestData( session, generation, request, {} );

	cache->beginNavigation( session, URI( "https://other.example/" ) );
	generation = cache->beginNavigation( session, URI( "https://news.example/" ) );
	request.headers["Referer"] = "https://other.example/";
	request.headers["Cookie"] = "session=now-present";
	cache->requestData( session, generation, request, {} );

	EXPECT_EQ( 1, fetches );
	EXPECT_EQ( 1u, cache->getEntryCount() );
}

UTEST( WebResourceCache, ttlStartsWhenFinalDocumentLeaseIsReleased ) {
	auto cache = WebResourceCache::New();
	cache->setTTL( Time::Zero );
	int fetches = 0;
	cache->setFetcher(
		[&fetches]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			++fetches;
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "body" ) );
		} );
	auto session = cache->createSession();
	auto generation = cache->beginNavigation( session, URI( "https://news.example/" ) );
	auto request = requestFor( "https://news.example/logo.png" );
	cache->requestData( session, generation, request, {} );

	// Simulate a document that remained open beyond the entry's original deadline. Retention
	// starts when navigation releases the final lease, using the policy active at that time.
	cache->setTTL( Seconds( 30 ) );
	cache->beginNavigation( session, URI( "https://other.example/" ) );
	cache->prune();
	EXPECT_EQ( 1u, cache->getEntryCount() );

	generation = cache->beginNavigation( session, URI( "https://news.example/" ) );
	cache->requestData( session, generation, request, {} );
	EXPECT_EQ( 1, fetches );
}

UTEST( WebResourceCache, navigationReleasesLeasesAfterDenseMapGrowth ) {
	auto cache = WebResourceCache::New();
	cache->setFetcher(
		[]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "body" ) );
		} );
	auto session = cache->createSession();
	auto generation = cache->beginNavigation( session, URI( "https://example.com/" ) );
	for ( int i = 0; i < 512; ++i ) {
		cache->requestData(
			session, generation,
			requestFor( String::format( "https://example.com/resource/%d", i ).c_str() ), {} );
	}
	EXPECT_EQ( 512u, cache->getEntryCount() );

	cache->setTTL( Time::Zero );
	cache->beginNavigation( session, URI( "https://other.example/" ) );
	cache->prune();
	EXPECT_EQ( 0u, cache->getEntryCount() );
}

UTEST( WebResourceCache, failedLoadsCanRetry ) {
	auto cache = WebResourceCache::New();
	int fetches = 0;
	cache->setFetcher(
		[&fetches]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			++fetches;
			auto status = fetches == 1 ? Http::Response::Status::ConnectionFailed
									   : Http::Response::Status::Ok;
			completion( response( status, fetches == 1 ? "" : "body" ) );
		} );
	auto session = cache->createSession();
	auto generation = cache->beginNavigation( session, URI( "https://example.com/" ) );
	int successful = 0;
	auto callback = [&successful]( const WebResourceResult& result ) {
		if ( result.success )
			++successful;
	};
	cache->requestData( session, generation, requestFor( "https://example.com/site.css" ),
						callback );
	EXPECT_EQ( 1, fetches );
	EXPECT_EQ( 0, successful );
	cache->requestData( session, generation, requestFor( "https://example.com/site.css" ),
						callback );
	EXPECT_EQ( 1, fetches );
	cache->setRetryDelay( Time::Zero );
	cache->requestData( session, generation, requestFor( "https://example.com/site.css" ),
						callback );
	EXPECT_EQ( 2, fetches );
	EXPECT_EQ( 1, successful );
}

UTEST( WebResourceCache, ttlAndByteBudgetEvictOnlyUnleasedEntries ) {
	auto cache = WebResourceCache::New();
	cache->setFetcher(
		[]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "12345678" ) );
		} );
	auto session = cache->createSession();
	auto generation = cache->beginNavigation( session, URI( "https://example.com/" ) );
	cache->requestData( session, generation, requestFor( "https://example.com/a.css" ), {} );
	EXPECT_EQ( 1u, cache->getEntryCount() );
	cache->setByteBudget( 1 );
	EXPECT_EQ( 1u, cache->getEntryCount() );
	cache->beginNavigation( session, URI( "https://example.com/next" ) );
	cache->prune();
	EXPECT_EQ( 0u, cache->getEntryCount() );

	cache->setByteBudget( 1024 );
	cache->setTTL( Time::Zero );
	generation = cache->getSessionGeneration( session );
	cache->requestData( session, generation, requestFor( "https://example.com/b.css" ), {} );
	EXPECT_EQ( 1u, cache->getEntryCount() );
	cache->beginNavigation( session, URI( "https://example.com/final" ) );
	cache->prune();
	EXPECT_EQ( 0u, cache->getEntryCount() );
}
