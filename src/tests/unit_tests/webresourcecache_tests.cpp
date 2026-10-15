#include "httpserver.hpp"
#include "utest.hpp"

#include <eepp/ui/webresourcecache.hpp>

#include <optional>
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

UTEST( WebResourceCache, documentCacheSeparatesCookieSessions ) {
	auto cache = WebResourceCache::New();
	int fetches = 0;
	cache->setFetcher(
		[&]( const WebResourceRequest&, WebResourceCache::FetchCompletion completion ) {
			++fetches;
			auto ok = Http::Response::Status::Ok;
			completion( response( ok, "document" ) );
		} );
	auto session = cache->createSession();
	auto generation = cache->beginNavigation( session, URI( "https://news.example/" ) );
	auto request = requestFor( "https://news.example/" );
	request.kind = WebResourceKind::Document;
	cache->requestData( session, generation, request, {} );
	request.headers["Cookie"] = "user=logged-in";
	cache->requestData( session, generation, request, {} );
	cache->requestData( session, generation, request, {} );
	EXPECT_EQ( 2, fetches );
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

UTEST( WebResourceCache, transportReusesAcrossNavigationAndSharedSessionsButIsolatesPartitions ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned id ) {
		return HTTPTest::Server::respond( socket, String::toString( id ) );
	} );
	ASSERT_TRUE( server.isListening() );
	auto cache = WebResourceCache::New();
	const auto partition = cache->createPartition();
	const auto first = cache->createSession( partition );
	const auto peer = cache->createSession( partition );
	const auto privateSession = cache->createSession();
	struct Completion {
		std::mutex mutex;
		std::condition_variable condition;
		std::string data;
		bool done{ false };
	};
	auto fetch = [&]( DocumentSessionId session, const char* path, WebResourceKind kind ) {
		const auto uri = server.uri( path );
		const auto generation = cache->beginNavigation( session, uri );
		auto completion = std::make_shared<Completion>();
		WebResourceRequest request;
		request.uri = uri;
		request.kind = kind;
		cache->requestData( session, generation, std::move( request ),
							[completion]( const WebResourceResult& result ) {
								std::lock_guard<std::mutex> lock( completion->mutex );
								if ( result.success && result.data )
									completion->data = *result.data;
								completion->done = true;
								completion->condition.notify_all();
							} );
		std::unique_lock<std::mutex> lock( completion->mutex );
		if ( !completion->condition.wait_for( lock, std::chrono::seconds( 3 ),
											  [&] { return completion->done; } ) )
			completion->data = "timeout";
		return completion->data;
	};
	EXPECT_TRUE( fetch( first, "/document", WebResourceKind::Document ) == "1" );
	EXPECT_TRUE( fetch( first, "/style", WebResourceKind::StyleSheet ) == "1" );
	EXPECT_TRUE( fetch( first, "/font", WebResourceKind::Font ) == "1" );
	EXPECT_TRUE( fetch( peer, "/peer", WebResourceKind::Document ) == "1" );
	EXPECT_TRUE( fetch( privateSession, "/private", WebResourceKind::Document ) == "2" );
	cache->destroySession( first );
	EXPECT_TRUE( fetch( peer, "/peer-next", WebResourceKind::Document ) == "1" );
	cache->destroySession( peer );
	EXPECT_TRUE( server.waitForClosed( 1 ) );
	EXPECT_TRUE( fetch( privateSession, "/private-next", WebResourceKind::Document ) == "2" );
	EXPECT_EQ( server.accepted.load(), 2u );
	cache->destroySession( privateSession );
	EXPECT_TRUE( server.waitForClosed( 2 ) );
}

UTEST( WebResourceCache, destroyingFinalSessionCancelsRunningTransportWithoutJoiningExecutor ) {
	std::mutex serverMutex;
	std::condition_variable serverCondition;
	bool releaseServer = false;
	HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned ) {
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCondition.wait_for( lock, std::chrono::seconds( 3 ), [&] { return releaseServer; } );
		return HTTPTest::Server::respond( socket );
	} );
	ASSERT_TRUE( server.isListening() );
	auto executor = ThreadPool::createShared( 1 );
	Http::setThreadPool( executor );
	auto cache = WebResourceCache::New();
	const auto partition = cache->createPartition();
	const auto session = cache->createSession( partition );
	const auto uri = server.uri();
	const auto generation = cache->beginNavigation( session, uri );
	std::atomic<unsigned> deliveries{ 0 };
	cache->requestData( session, generation, requestFor( uri.toString().c_str() ),
						[&]( const WebResourceResult& ) { ++deliveries; } );
	const bool started = server.waitForRequests( 1 );
	cache->destroySession( session );
	EXPECT_EQ( cache->getInFlightCount(), 0u );
	std::mutex mutex;
	std::condition_variable condition;
	bool workerAvailable = false;
	executor->run( [&] {
		std::lock_guard<std::mutex> lock( mutex );
		workerAvailable = true;
		condition.notify_all();
	} );
	bool cancelledWithoutServerResponse;
	{
		std::unique_lock<std::mutex> lock( mutex );
		cancelledWithoutServerResponse =
			condition.wait_for( lock, std::chrono::seconds( 2 ), [&] { return workerAvailable; } );
	}
	{
		std::lock_guard<std::mutex> lock( serverMutex );
		releaseServer = true;
	}
	serverCondition.notify_all();
	Http::setThreadPool( nullptr );
	executor.reset();
	EXPECT_TRUE( started );
	EXPECT_TRUE( cancelledWithoutServerResponse );
	EXPECT_EQ( deliveries.load(), 0u );
	// The same explicit partition can be used again after its final session was destroyed.
	const auto replacement = cache->createSession( partition );
	const auto replacementGeneration = cache->beginNavigation( replacement, uri );
	std::mutex resultMutex;
	std::condition_variable resultCondition;
	bool done = false;
	cache->requestData( replacement, replacementGeneration, requestFor( uri.toString().c_str() ),
						[&]( const WebResourceResult& result ) {
							std::lock_guard<std::mutex> lock( resultMutex );
							done = result.success;
							resultCondition.notify_all();
						} );
	{
		std::unique_lock<std::mutex> lock( resultMutex );
		EXPECT_TRUE(
			resultCondition.wait_for( lock, std::chrono::seconds( 2 ), [&] { return done; } ) );
	}
	cache->destroySession( replacement );
}

UTEST( WebResourceCache, verifiedRequestsNeverJoinOrReuseUnverifiedLoads ) {
	auto cache = WebResourceCache::New();
	auto session = cache->createSession();
	auto generation = cache->beginNavigation( session, URI( "https://example.com/" ) );
	std::vector<WebResourceCache::FetchCompletion> completions;
	cache->setFetcher( [&]( const WebResourceRequest&, WebResourceCache::FetchCompletion done ) {
		completions.emplace_back( std::move( done ) );
	} );
	auto request = requestFor( "https://example.com/style.css" );
	request.validateCertificate = false;
	cache->requestData( session, generation, request, {} );
	request.validateCertificate = true;
	cache->requestData( session, generation, request, {} );
	EXPECT_EQ( completions.size(), 2u );
	for ( auto& complete : completions )
		complete( response( Http::Response::Ok, "body" ) );
	cache->requestData( session, generation, request, {} );
	EXPECT_EQ( completions.size(), 2u );
}

UTEST( WebResourceCache, redirectCookiesAreAttributedToTheOriginThatSetThem ) {
	HTTPTest::Server landing( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond( socket, "landed",
										  "Set-Cookie: b=2; Path=/\r\nSet-Cookie: d=4\r\n" );
	} );
	ASSERT_TRUE( landing.isListening() );
	const std::string landingUri = landing.uri( "/final" ).toString();
	HTTPTest::Server origin( [&]( TcpSocket& socket, const std::string& request, unsigned ) {
		if ( request.find( "GET /start " ) == 0 ) {
			return HTTPTest::Server::send( socket, "HTTP/1.1 302 Found\r\nLocation: /next\r\n"
												   "Set-Cookie: a=1\r\nContent-Length: 0\r\n\r\n" );
		}
		return HTTPTest::Server::send( socket,
									   "HTTP/1.1 302 Found\r\nLocation: " + landingUri +
										   "\r\nSet-Cookie: c=3\r\nContent-Length: 0\r\n\r\n" );
	} );
	ASSERT_TRUE( origin.isListening() );

	auto cache = WebResourceCache::New();
	const auto session = cache->createSession();
	const auto uri = origin.uri( "/start" );
	const auto generation = cache->beginNavigation( session, uri );
	std::mutex mutex;
	std::condition_variable condition;
	std::optional<WebResourceResult> received;
	WebResourceRequest request;
	request.uri = uri;
	request.kind = WebResourceKind::Document;
	cache->requestData( session, generation, std::move( request ),
						[&]( const WebResourceResult& result ) {
							std::lock_guard<std::mutex> lock( mutex );
							received = result;
							condition.notify_all();
						} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( condition.wait_for( lock, std::chrono::seconds( 3 ),
										 [&] { return received.has_value(); } ) );
	}
	ASSERT_TRUE( received->success );
	EXPECT_STREQ( received->data->c_str(), "landed" );
	ASSERT_EQ( received->cookies.size(), static_cast<size_t>( 3 ) );
	const std::string originAuthority = origin.uri().getAuthority();
	EXPECT_STDSTREQ( received->cookies[0].authority, originAuthority );
	EXPECT_STREQ( received->cookies[0].setCookie.c_str(), "a=1" );
	EXPECT_STDSTREQ( received->cookies[1].authority, originAuthority );
	EXPECT_STREQ( received->cookies[1].setCookie.c_str(), "c=3" );
	EXPECT_STDSTREQ( received->cookies[2].authority, landing.uri().getAuthority() );
	EXPECT_STREQ( received->cookies[2].setCookie.c_str(), "b=2; Path=/\nd=4" );
	cache->destroySession( session );
}

UTEST( WebResourceCache, rejectedRedirectNeverLendsCookiesToItsTarget ) {
	HTTPTest::Server origin( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::send( socket, "HTTP/1.1 302 Found\r\n"
											   "Location: ftp://victim.example/\r\n"
											   "Set-Cookie: session=injected\r\n"
											   "Content-Length: 0\r\n\r\n" );
	} );
	ASSERT_TRUE( origin.isListening() );
	auto cache = WebResourceCache::New();
	const auto session = cache->createSession();
	const auto uri = origin.uri( "/start" );
	const auto generation = cache->beginNavigation( session, uri );
	std::mutex mutex;
	std::condition_variable condition;
	std::optional<WebResourceResult> received;
	WebResourceRequest request;
	request.uri = uri;
	request.kind = WebResourceKind::Document;
	cache->requestData( session, generation, std::move( request ),
						[&]( const WebResourceResult& result ) {
							std::lock_guard<std::mutex> lock( mutex );
							received = result;
							condition.notify_all();
						} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( condition.wait_for( lock, std::chrono::seconds( 3 ),
										 [&] { return received.has_value(); } ) );
	}
	EXPECT_FALSE( received->success );
	// The cookie belongs to the origin that sent it, exactly once; never to the rejected target.
	ASSERT_EQ( received->cookies.size(), static_cast<size_t>( 1 ) );
	EXPECT_STDSTREQ( received->cookies[0].authority, origin.uri().getAuthority() );
	EXPECT_STREQ( received->cookies[0].setCookie.c_str(), "session=injected" );
	cache->destroySession( session );
}
