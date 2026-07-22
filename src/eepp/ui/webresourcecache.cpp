#include <eepp/core/small_vector.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/ui/webresourcecache.hpp>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

using namespace EE::Graphics;
using namespace EE::Network;
using namespace EE::System;

namespace EE { namespace UI {

namespace {

std::atomic<Uint64> sNextDocumentSessionId{ 1 };
std::atomic<Uint64> sNextCachePartitionId{ 1 };

std::string canonicalURI( URI uri ) {
	uri.setFragment( "" );
	uri.normalize();
	return uri.toString();
}

std::string makeRequestKey( CachePartitionId partition, const WebResourceRequest& request ) {
	std::string key =
		String::format( "%llu\n%u\n%u\n%s\n", static_cast<unsigned long long>( partition ),
						static_cast<Uint32>( request.kind ), static_cast<Uint32>( request.method ),
						canonicalURI( request.uri ).c_str() );
	for ( const auto& header : request.headers ) {
		std::string name = String::toLower( header.first );
		// The partition identifies the cookie/authentication context. Cookie values evolve while a
		// document is being used, and Referer changes with navigation history; including either one
		// would duplicate an otherwise reusable resource on Back/Forward navigation.
		if ( name == "cookie" || name == "referer" )
			continue;
		key += name;
		key += ':';
		key += header.second;
		key += '\n';
	}
	key += request.body;
	key += String::format( "\n%d:%d:%d:%.4f", request.clampToEdge, request.mipmaps,
						   request.compressTexture, request.svgScale );
	return key;
}

} // namespace

bool WebOriginKey::operator==( const WebOriginKey& other ) const {
	return scheme == other.scheme && host == other.host && port == other.port;
}

WebOriginKey WebOriginKey::fromURI( const URI& uri ) {
	return { String::toLower( uri.getScheme() ), String::toLower( uri.getHost() ), uri.getPort() };
}

struct WebResourceCache::Impl {
	struct Subscriber {
		DocumentSessionId session{ 0 };
		Uint64 generation{ 0 };
		Callback callback;
	};

	struct Entry {
		WebResourceLoadState state{ WebResourceLoadState::Empty };
		WebResourceRequest request;
		std::shared_ptr<const std::string> data;
		TexturePtr texture;
		std::vector<Subscriber> subscribers;
		SmallVector<DocumentSessionId, 4> activeLeases;
		Time lastUsed;
		Time expiresAt;
		Time retryAt;
		std::size_t retainedBytes{ 0 };
		int status{ 0 };
		std::string error;
	};

	struct Session {
		CachePartitionId partition{ 0 };
		Uint64 generation{ 0 };
		WebOriginKey origin;
	};

	mutable std::mutex mutex;
	Clock clock;
	UnorderedMap<DocumentSessionId, Session> sessions;
	UnorderedMap<std::string, Entry> entries;
	Time ttl{ Seconds( 30 ) };
	Time retryDelay{ Seconds( 1 ) };
	std::size_t byteBudget{ 64 * 1024 * 1024 };
	std::size_t retainedBytes{ 0 };
	std::size_t inFlightCount{ 0 };
	Fetcher fetcher;

	Time now() const { return clock.getElapsedTime(); }

	bool isCurrent( const Subscriber& subscriber ) const {
		auto it = sessions.find( subscriber.session );
		return it != sessions.end() && it->second.generation == subscriber.generation;
	}

	void releaseSessionLeases( DocumentSessionId session ) {
		const Time releasedAt = now();
		for ( auto& cacheEntry : entries ) {
			Entry& entry = cacheEntry.second;
			auto lease = std::find( entry.activeLeases.begin(), entry.activeLeases.end(), session );
			if ( lease != entry.activeLeases.end() ) {
				entry.activeLeases.erase( lease );
				if ( entry.activeLeases.empty() ) {
					entry.lastUsed = releasedAt;
					entry.expiresAt = releasedAt + ttl;
				}
			}
		}
	}

	void lease( DocumentSessionId session, Entry& entry ) {
		if ( std::find( entry.activeLeases.begin(), entry.activeLeases.end(), session ) ==
			 entry.activeLeases.end() )
			entry.activeLeases.push_back( session );
		entry.lastUsed = now();
		entry.expiresAt = entry.lastUsed + ttl;
	}

	UnorderedMap<std::string, Entry>::iterator
	eraseEntry( UnorderedMap<std::string, Entry>::iterator it ) {
		retainedBytes -= std::min( retainedBytes, it->second.retainedBytes );
		return entries.erase( it );
	}

	void pruneLocked() {
		Time current = now();
		for ( auto it = entries.begin(); it != entries.end(); ) {
			if ( it->second.state != WebResourceLoadState::Loading &&
				 it->second.activeLeases.empty() && it->second.expiresAt <= current ) {
				it = eraseEntry( it );
			} else {
				++it;
			}
		}

		while ( retainedBytes > byteBudget ) {
			auto oldest = entries.end();
			for ( auto it = entries.begin(); it != entries.end(); ++it ) {
				if ( it->second.state == WebResourceLoadState::Loading ||
					 !it->second.activeLeases.empty() )
					continue;
				if ( oldest == entries.end() || it->second.lastUsed < oldest->second.lastUsed )
					oldest = it;
			}
			if ( oldest == entries.end() )
				break;
			eraseEntry( oldest );
		}
	}
};

WebResourceCachePtr WebResourceCache::New() {
	return std::make_shared<WebResourceCache>();
}

WebResourceCache::WebResourceCache() : mImpl( std::make_unique<Impl>() ) {}

WebResourceCache::~WebResourceCache() {
	clear();
}

CachePartitionId WebResourceCache::createPartition() {
	return sNextCachePartitionId.fetch_add( 1, std::memory_order_relaxed );
}

DocumentSessionId WebResourceCache::createSession( CachePartitionId partition ) {
	if ( partition == 0 )
		partition = createPartition();
	DocumentSessionId id = sNextDocumentSessionId.fetch_add( 1, std::memory_order_relaxed );
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->sessions.emplace( id, Impl::Session{ partition } );
	return id;
}

void WebResourceCache::destroySession( DocumentSessionId session ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	auto it = mImpl->sessions.find( session );
	if ( it == mImpl->sessions.end() )
		return;
	mImpl->releaseSessionLeases( session );
	mImpl->sessions.erase( it );
	mImpl->pruneLocked();
}

Uint64 WebResourceCache::beginNavigation( DocumentSessionId session, const URI& uri ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	auto it = mImpl->sessions.find( session );
	if ( it == mImpl->sessions.end() )
		return 0;
	mImpl->releaseSessionLeases( session );
	it->second.origin = WebOriginKey::fromURI( uri );
	return ++it->second.generation;
}

Uint64 WebResourceCache::getSessionGeneration( DocumentSessionId session ) const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	auto it = mImpl->sessions.find( session );
	return it != mImpl->sessions.end() ? it->second.generation : 0;
}

CachePartitionId WebResourceCache::getSessionPartition( DocumentSessionId session ) const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	auto it = mImpl->sessions.find( session );
	return it != mImpl->sessions.end() ? it->second.partition : 0;
}

void WebResourceCache::requestData( DocumentSessionId session, Uint64 generation,
									WebResourceRequest request, Callback callback ) {
	std::string key;
	WebResourceResult immediate;
	bool hasImmediate = false;
	bool startRequest = false;
	Fetcher fetcher;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		auto sessionIt = mImpl->sessions.find( session );
		if ( sessionIt == mImpl->sessions.end() || sessionIt->second.generation != generation )
			return;
		key = makeRequestKey( sessionIt->second.partition, request );
		auto entryIt = mImpl->entries.try_emplace( key ).first;
		auto& entry = entryIt->second;
		mImpl->lease( session, entry );
		if ( entry.state == WebResourceLoadState::Ready ) {
			immediate = { true, entry.state, entry.status, {}, {}, entry.data, entry.texture };
			hasImmediate = true;
		} else if ( entry.state == WebResourceLoadState::Failed && entry.retryAt > mImpl->now() ) {
			immediate = { false, entry.state, entry.status, entry.error, {}, {}, entry.texture };
			hasImmediate = true;
		} else {
			if ( callback )
				entry.subscribers.push_back( { session, generation, std::move( callback ) } );
			if ( entry.state != WebResourceLoadState::Loading ) {
				entry.state = WebResourceLoadState::Loading;
				entry.request = request;
				entry.error.clear();
				++mImpl->inFlightCount;
				startRequest = true;
				fetcher = mImpl->fetcher;
			}
		}
	}
	if ( hasImmediate ) {
		if ( callback )
			callback( immediate );
		return;
	}
	if ( !startRequest )
		return;

	std::shared_ptr<std::string> redirectCookie;
	if ( !fetcher )
		redirectCookie = std::make_shared<std::string>();
	auto weak = weak_from_this();
	auto process = [weak, key, redirectCookie]( Http::Response& response ) {
		auto self = weak.lock();
		if ( !self )
			return;
		std::vector<Impl::Subscriber> subscribers;
		WebResourceResult result;
		{
			std::lock_guard<std::mutex> lock( self->mImpl->mutex );
			auto entryIt = self->mImpl->entries.find( key );
			if ( entryIt == self->mImpl->entries.end() ||
				 entryIt->second.state != WebResourceLoadState::Loading )
				return;
			auto& entry = entryIt->second;
			self->mImpl->inFlightCount--;
			entry.status = static_cast<int>( response.getStatus() );
			entry.lastUsed = self->mImpl->now();
			entry.expiresAt = entry.lastUsed + self->mImpl->ttl;
			std::string setCookie = response.getField( "set-cookie" );
			if ( setCookie.empty() && redirectCookie )
				setCookie = *redirectCookie;
			if ( response.isOK() && !response.getBody().empty() ) {
				if ( entry.request.kind == WebResourceKind::Image ) {
					Image image( reinterpret_cast<const Uint8*>( response.getBody().data() ),
								 response.getBody().size() );
					if ( image.getPixels() && entry.texture ) {
						entry.texture->replace( &image );
						entry.retainedBytes =
							static_cast<std::size_t>( entry.texture->getSize().getWidth() *
													  entry.texture->getSize().getHeight() * 4 );
					} else {
						entry.state = WebResourceLoadState::Failed;
						entry.error = "Invalid image data";
					}
				} else {
					entry.data = std::make_shared<const std::string>( response.getBody() );
					entry.retainedBytes = entry.data->size();
				}
				if ( entry.state != WebResourceLoadState::Failed ) {
					entry.state = WebResourceLoadState::Ready;
					self->mImpl->retainedBytes += entry.retainedBytes;
					result = { true,	   entry.state,	 entry.status, {}, std::move( setCookie ),
							   entry.data, entry.texture };
				} else {
					result = {
						false, entry.state,	 entry.status, entry.error, std::move( setCookie ),
						{},	   entry.texture };
				}
			} else {
				entry.state = WebResourceLoadState::Failed;
				entry.error = response.getStatusDescription();
				result = { false, entry.state,	entry.status, entry.error, std::move( setCookie ),
						   {},	  entry.texture };
			}
			if ( entry.state == WebResourceLoadState::Failed )
				entry.retryAt = entry.lastUsed + self->mImpl->retryDelay;
			entry.request = {};
			subscribers.swap( entry.subscribers );
			self->mImpl->pruneLocked();
			for ( auto it = subscribers.begin(); it != subscribers.end(); ) {
				if ( !self->mImpl->isCurrent( *it ) )
					it = subscribers.erase( it );
				else
					++it;
			}
		}
		for ( auto& subscriber : subscribers ) {
			if ( subscriber.callback )
				subscriber.callback( result );
		}
	};
	auto complete = [process = std::move( process ), kind = request.kind,
					 dispatcher = request.completionDispatcher]( Http::Response response ) mutable {
		if ( kind == WebResourceKind::Image && dispatcher ) {
			auto sharedResponse = std::make_shared<Http::Response>( std::move( response ) );
			dispatcher(
				[process = std::move( process ), sharedResponse = std::move( sharedResponse )]() {
					process( *sharedResponse );
				} );
		} else {
			process( response );
		}
	};

	if ( fetcher ) {
		fetcher( request, std::move( complete ) );
	} else {
		Http::Request::ProgressCallback progress =
			[redirectCookie]( const Http&, const Http::Request&, const Http::Response& response,
							  const Http::Request::Status& status, std::size_t, std::size_t ) {
				if ( status == Http::Request::Status::Redirect &&
					 response.hasField( "set-cookie" ) )
					*redirectCookie = response.getField( "set-cookie" );
				return true;
			};
		Http::requestAsync(
			[complete = std::move( complete )]( const Http&, Http::Request&,
												Http::Response& response ) mutable {
				complete( std::move( response ) );
			},
			request.uri, request.timeout, request.method, progress, request.headers, request.body,
			request.validateCertificate, request.proxy, request.followRedirect );
	}
}

TexturePtr WebResourceCache::requestTexture( DocumentSessionId session, Uint64 generation,
											 WebResourceRequest request, Callback callback ) {
	request.kind = WebResourceKind::Image;
	std::string key;
	TexturePtr texture;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		auto sessionIt = mImpl->sessions.find( session );
		if ( sessionIt == mImpl->sessions.end() || sessionIt->second.generation != generation )
			return {};
		key = makeRequestKey( sessionIt->second.partition, request );
		auto entry = mImpl->entries.find( key );
		if ( entry != mImpl->entries.end() )
			texture = entry->second.texture;
	}
	if ( !texture ) {
		texture = TextureFactory::instance()->createEmptyTexture(
			1, 1, 4, Color::Transparent, false,
			request.clampToEdge ? Texture::ClampMode::ClampToEdge : Texture::ClampMode::ClampRepeat,
			request.mipmaps, request.compressTexture, canonicalURI( request.uri ) );
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		auto sessionIt = mImpl->sessions.find( session );
		if ( sessionIt == mImpl->sessions.end() || sessionIt->second.generation != generation )
			return {};
		auto entry = mImpl->entries.try_emplace( key ).first;
		if ( !entry->second.texture )
			entry->second.texture = texture;
		else
			texture = entry->second.texture;
	}
	requestData( session, generation, std::move( request ), std::move( callback ) );
	return texture;
}

void WebResourceCache::setTTL( Time ttl ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->ttl = ttl;
}

Time WebResourceCache::getTTL() const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->ttl;
}

void WebResourceCache::setRetryDelay( Time delay ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->retryDelay = delay;
	const Time retryAt = mImpl->now() + delay;
	for ( auto& entry : mImpl->entries ) {
		if ( entry.second.state == WebResourceLoadState::Failed )
			entry.second.retryAt = retryAt;
	}
}

Time WebResourceCache::getRetryDelay() const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->retryDelay;
}

void WebResourceCache::setByteBudget( std::size_t bytes ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->byteBudget = bytes;
	mImpl->pruneLocked();
}

std::size_t WebResourceCache::getByteBudget() const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->byteBudget;
}

std::size_t WebResourceCache::getRetainedBytes() const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->retainedBytes;
}

std::size_t WebResourceCache::getEntryCount() const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->entries.size();
}

std::size_t WebResourceCache::getInFlightCount() const {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->inFlightCount;
}

void WebResourceCache::prune() {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->pruneLocked();
}

void WebResourceCache::clear() {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	for ( auto& entry : mImpl->entries ) {
		entry.second.state = WebResourceLoadState::Cancelled;
		entry.second.subscribers.clear();
	}
	mImpl->entries.clear();
	mImpl->retainedBytes = 0;
	mImpl->inFlightCount = 0;
}

void WebResourceCache::setFetcher( Fetcher fetcher ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->fetcher = std::move( fetcher );
}

}} // namespace EE::UI
