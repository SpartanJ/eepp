#include <eepp/core/small_vector.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/ui/webresourcecache.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
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

// Room for the raster scale that appendRasterScale() writes at the end of a key.
constexpr std::size_t RasterScaleCapacity = 32;

// Key fields shared by every raster scale of a request. The key is complete after
// appendRasterScale(), and has room for it.
std::string makeRequestKeyBase( CachePartitionId partition, const WebResourceRequest& request ) {
	const std::string uri = canonicalURI( request.uri );
	std::size_t size = 64 + uri.size() + request.body.size() + RasterScaleCapacity;
	for ( const auto& header : request.headers )
		size += header.first.size() + header.second.size() + 2;
	std::string key;
	key.reserve( size );

	char buffer[96];
	int length = std::snprintf(
		buffer, sizeof( buffer ), "%llu\n%u\n%u\n%d\n",
		static_cast<unsigned long long>( partition ), static_cast<Uint32>( request.kind ),
		static_cast<Uint32>( request.method ), request.validateCertificate );
	key.append( buffer, length );
	key += uri;
	key += '\n';
	for ( const auto& header : request.headers ) {
		// Documents can change with authentication, while static resources remain reusable as
		// cookies evolve. Referer changes with navigation history but does not identify content.
		if ( ( String::iequals( header.first, "cookie" ) &&
			   request.kind != WebResourceKind::Document ) ||
			 String::iequals( header.first, "referer" ) )
			continue;
		for ( char c : header.first )
			key += static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
		key += ':';
		key += header.second;
		key += '\n';
	}
	key += request.body;
	length = std::snprintf( buffer, sizeof( buffer ), "\n%d:%d:%d:", request.clampToEdge,
							request.mipmaps, request.compressTexture );
	key.append( buffer, length );
	return key;
}

void appendRasterScale( std::string& key, Float svgScale ) {
	char buffer[RasterScaleCapacity];
	const int length = std::snprintf( buffer, sizeof( buffer ), "%.9g", svgScale );
	key.append( buffer, length );
}

std::string makeRequestKey( CachePartitionId partition, const WebResourceRequest& request,
							Float svgScale ) {
	std::string key = makeRequestKeyBase( partition, request );
	appendRasterScale( key, svgScale );
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

	// The entry can move to another raster-scale key while the HTTP request is in flight.
	struct ImageLoad {
		std::string key;
		std::string sourceKey;
		DocumentSessionId session;
		bool canRetarget{ true };
	};

	struct Entry {
		CachePartitionId partition{ 0 };
		WebResourceLoadState state{ WebResourceLoadState::Empty };
		// A cache key identifies a reusable resource, not one particular asynchronous load. These
		// values identify the load currently allowed to complete into this entry.
		Uint64 operationEpoch{ 0 };
		Uint64 operationId{ 0 };
		WebResourceRequest request;
		std::shared_ptr<ImageLoad> imageLoad;
		std::shared_ptr<const std::string> data;
		TexturePtr texture;
		std::vector<Subscriber> subscribers;
		SmallVector<DocumentSessionId, 4> activeLeases;
		Time lastUsed;
		Time expiresAt;
		Time retryAt;
		std::size_t retainedBytes{ 0 };
		int status{ 0 };
		bool bitmap{ false };
		std::string error;
	};

	struct Session {
		CachePartitionId partition{ 0 };
		Uint64 generation{ 0 };
		WebOriginKey origin;
		UnorderedMap<std::string, std::weak_ptr<ImageLoad>> pendingImages;
	};

	mutable std::mutex mutex;
	Clock clock;
	UnorderedMap<DocumentSessionId, Session> sessions;
	UnorderedMap<std::string, Entry> entries;
	UnorderedMap<CachePartitionId, std::shared_ptr<Http::Pool>> transports;
	Http::ConnectionPoolOptions connectionOptions;
	Time ttl{ Seconds( 30 ) };
	Time retryDelay{ Seconds( 1 ) };
	std::size_t byteBudget{ 64 * 1024 * 1024 };
	std::size_t retainedBytes{ 0 };
	std::size_t inFlightCount{ 0 };
	// clear() advances the epoch so every operation started before it becomes stale, even if a
	// request with the same key immediately recreates an entry.
	Uint64 cacheEpoch{ 1 };
	Uint64 nextOperationId{ 1 };
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
	eraseEntry( UnorderedMap<std::string, Entry>::iterator it,
				std::vector<Entry>& releasedEntries ) {
		retainedBytes -= std::min( retainedBytes, it->second.retainedBytes );
		// Move owning handles out before erasing. releasedEntries outlives the caller's lock guard,
		// so final resource destruction cannot run under the cache mutex.
		releasedEntries.emplace_back( std::move( it->second ) );
		return entries.erase( it );
	}

	void pruneLocked( std::vector<Entry>& releasedEntries ) {
		Time current = now();
		for ( auto it = entries.begin(); it != entries.end(); ) {
			if ( it->second.state != WebResourceLoadState::Loading &&
				 it->second.activeLeases.empty() && it->second.expiresAt <= current ) {
				it = eraseEntry( it, releasedEntries );
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
			eraseEntry( oldest, releasedEntries );
		}
	}

	bool matchesOperation( const Entry& entry, Uint64 operationEpoch, Uint64 operationId ) const {
		return entry.state == WebResourceLoadState::Loading &&
			   entry.operationEpoch == operationEpoch && entry.operationId == operationId;
	}

	void retargetImage( Session& session, const std::string& sourceKey, const std::string& key,
						Float scale ) {
		if ( entries.find( key ) != entries.end() )
			return;
		auto pending = session.pendingImages.find( sourceKey );
		if ( pending == session.pendingImages.end() )
			return;
		auto load = pending->second.lock();
		if ( !load || !load->canRetarget )
			return;
		auto old = entries.find( load->key );
		if ( old == entries.end() || old->second.imageLoad != load )
			return;
		Entry entry = std::move( old->second );
		entries.erase( old );
		load->key = key;
		entry.request.svgScale = scale;
		entries.emplace( key, std::move( entry ) );
	}

	void finishImageLoad( const std::shared_ptr<ImageLoad>& load ) {
		if ( !load )
			return;
		auto session = sessions.find( load->session );
		if ( session == sessions.end() )
			return;
		auto pending = session->second.pendingImages.find( load->sourceKey );
		if ( pending != session->second.pendingImages.end() && pending->second.lock() == load )
			session->second.pendingImages.erase( pending );
	}
};

WebResourceCachePtr WebResourceCache::New() {
	return std::make_shared<WebResourceCache>();
}

WebResourceCachePtr WebResourceCache::New( const Http::ConnectionPoolOptions& options ) {
	return std::make_shared<WebResourceCache>( options );
}

WebResourceCache::WebResourceCache() : WebResourceCache( Http::ConnectionPoolOptions{} ) {}

WebResourceCache::WebResourceCache( const Http::ConnectionPoolOptions& options ) :
	mImpl( std::make_unique<Impl>() ) {
	mImpl->connectionOptions = options;
}

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
	std::vector<Impl::Entry> releasedEntries;
	std::shared_ptr<Http::Pool> releasedTransport;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		auto it = mImpl->sessions.find( session );
		if ( it == mImpl->sessions.end() )
			return;
		const CachePartitionId partition = it->second.partition;
		mImpl->releaseSessionLeases( session );
		mImpl->sessions.erase( it );
		const bool stillUsed = std::any_of(
			mImpl->sessions.begin(), mImpl->sessions.end(),
			[partition]( const auto& item ) { return item.second.partition == partition; } );
		if ( !stillUsed ) {
			// Closing the transport suppresses its callbacks. Remove its pending entries now so
			// a later session in this partition can start fresh instead of joining an orphan load.
			for ( auto entry = mImpl->entries.begin(); entry != mImpl->entries.end(); ) {
				if ( entry->second.partition == partition &&
					 entry->second.state == WebResourceLoadState::Loading ) {
					--mImpl->inFlightCount;
					entry = mImpl->eraseEntry( entry, releasedEntries );
				} else {
					++entry;
				}
			}
			auto transport = mImpl->transports.find( partition );
			if ( transport != mImpl->transports.end() ) {
				releasedTransport = std::move( transport->second );
				mImpl->transports.erase( transport );
			}
		}
		mImpl->pruneLocked( releasedEntries );
	}
	if ( releasedTransport )
		releasedTransport->close();
}

Uint64 WebResourceCache::beginNavigation( DocumentSessionId session, const URI& uri ) {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	auto it = mImpl->sessions.find( session );
	if ( it == mImpl->sessions.end() )
		return 0;
	mImpl->releaseSessionLeases( session );
	it->second.pendingImages.clear();
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
	this->request( session, generation, std::move( request ), std::move( callback ), false );
}

TexturePtr WebResourceCache::requestTexture( DocumentSessionId session, Uint64 generation,
											 WebResourceRequest request, Callback callback ) {
	request.kind = WebResourceKind::Image;
	return this->request( session, generation, std::move( request ), std::move( callback ), true );
}

TexturePtr WebResourceCache::request( DocumentSessionId session, Uint64 generation,
									  WebResourceRequest request, Callback callback,
									  bool needsTexture, bool automaticScale ) {
	std::string key;
	TexturePtr texture;
	TexturePtr placeholder;
	std::shared_ptr<Impl::ImageLoad> imageLoad;
	WebResourceResult immediate;
	bool hasImmediate = false;
	bool startRequest = false;
	Uint64 operationEpoch = 0;
	Uint64 operationId = 0;
	Fetcher fetcher;
	std::shared_ptr<Http::Pool> transport;
	for ( ;; ) {
		std::unique_lock<std::mutex> lock( mImpl->mutex );
		auto sessionIt = mImpl->sessions.find( session );
		if ( sessionIt == mImpl->sessions.end() || sessionIt->second.generation != generation )
			return {};
		auto& owner = sessionIt->second;
		// Build the shared fields once; the 1x and requested-scale keys differ only in the suffix.
		std::string sourceKey = makeRequestKeyBase( owner.partition, request );
		key.reserve( sourceKey.capacity() );
		key.assign( sourceKey );
		appendRasterScale( sourceKey, 1.f );
		appendRasterScale( key, request.svgScale );
		auto source = mImpl->entries.find( sourceKey );
		if ( source != mImpl->entries.end() && source->second.bitmap &&
			 source->second.state == WebResourceLoadState::Ready ) {
			key = sourceKey;
		} else if ( automaticScale ) {
			mImpl->retargetImage( owner, sourceKey, key, request.svgScale );
		}
		auto existing = mImpl->entries.find( key );
		if ( needsTexture && !placeholder &&
			 ( existing == mImpl->entries.end() || !existing->second.texture ) ) {
			lock.unlock();
			placeholder = TextureFactory::instance()->createEmptyTexture(
				1, 1, 4, Color::Transparent, false,
				request.clampToEdge ? Texture::ClampMode::ClampToEdge
									: Texture::ClampMode::ClampRepeat,
				request.mipmaps, request.compressTexture, canonicalURI( request.uri ) );
			if ( !placeholder )
				return {};
			// The bitmap could have completed while the GPU allocation was in progress.
			continue;
		}
		auto entryIt = mImpl->entries.try_emplace( key ).first;
		auto& entry = entryIt->second;
		entry.partition = owner.partition;
		if ( !entry.texture )
			entry.texture = std::move( placeholder );
		texture = entry.texture;
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
				// Capture both the cache lifetime and this specific fetch. The operation ID also
				// protects against an entry being erased and recreated without a full cache clear.
				entry.operationEpoch = mImpl->cacheEpoch;
				entry.operationId = mImpl->nextOperationId++;
				operationEpoch = entry.operationEpoch;
				operationId = entry.operationId;
				entry.request = request;
				if ( automaticScale ) {
					imageLoad = std::make_shared<Impl::ImageLoad>(
						Impl::ImageLoad{ key, std::move( sourceKey ), session } );
					entry.imageLoad = imageLoad;
					owner.pendingImages[imageLoad->sourceKey] = imageLoad;
				}
				entry.error.clear();
				++mImpl->inFlightCount;
				startRequest = true;
				fetcher = mImpl->fetcher;
				if ( !fetcher ) {
					auto& pool = mImpl->transports[owner.partition];
					if ( !pool )
						pool = std::make_shared<Http::Pool>( mImpl->connectionOptions );
					transport = pool;
				}
			} else if ( entry.imageLoad &&
						( !automaticScale || entry.imageLoad->session != session ) ) {
				// A fixed-scale caller or another scene needs this raster even if the first scene
				// moves.
				entry.imageLoad->canRetarget = false;
			}
		}
		break;
	}
	if ( hasImmediate ) {
		if ( callback )
			callback( immediate );
		return texture;
	}
	if ( !startRequest )
		return texture;

	// Collects the Set-Cookie fields of every response in the redirect chain, attributed to the
	// origin that actually sent each one. Progress callbacks run on the request's worker before its
	// completion, which then owns the trail; it is never accessed concurrently.
	struct CookieTrail {
		URI requested;
		std::vector<WebResourceCookies> cookies;
		bool observedHeaders{ false };
	};
	auto trail = std::make_shared<CookieTrail>();
	trail->requested = request.uri;
	auto weak = weak_from_this();
	auto process = [weak, key, operationEpoch, operationId, trail, imageLoad,
					kind = request.kind]( Http::Response& response ) {
		auto self = weak.lock();
		if ( !self )
			return;

		WebResourceRequest entryRequest;
		TexturePtr texture;
		const auto* bytes = reinterpret_cast<const Uint8*>( response.getBody().data() );
		const size_t length = response.getBody().size();
		bool success = response.isOK() && length != 0;
		const bool isImage = kind == WebResourceKind::Image;
		const bool isSvg =
			success && isImage && Image::getFormat( bytes, length ) == Image::Format::SVG;
		{
			std::lock_guard<std::mutex> lock( self->mImpl->mutex );
			auto entryIt = self->mImpl->entries.find( imageLoad ? imageLoad->key : key );
			if ( entryIt == self->mImpl->entries.end() ||
				 !self->mImpl->matchesOperation( entryIt->second, operationEpoch, operationId ) )
				return;
			// Copy only the inputs needed to prepare the response. Decoding below must not
			// serialize unrelated cache operations or execute device work under the cache mutex.
			entryRequest = entryIt->second.request;
			texture = entryIt->second.texture;
			if ( imageLoad && isSvg )
				imageLoad->canRetarget = false;
		}

		const int status = static_cast<int>( response.getStatus() );
		std::vector<WebResourceCookies> cookies = std::move( trail->cookies );
		// The default transport recorded every response's cookies as its headers arrived. A
		// custom fetcher reports no progress, so its final response belongs to the request URI.
		if ( !trail->observedHeaders && response.hasField( "set-cookie" ) ) {
			cookies.push_back(
				{ trail->requested.getAuthority(), response.getField( "set-cookie" ) } );
		}
		std::shared_ptr<const std::string> data;
		std::unique_ptr<Image> image;
		std::string error;
		std::size_t retainedBytes = 0;
		Float rasterScale = 1.f;
		bool bitmap = false;

		if ( success && isImage ) {
			Image::FormatConfiguration format;
			bitmap = !isSvg;
			if ( !bitmap ) {
				rasterScale = entryRequest.svgScale;
				format.svgScale( rasterScale );
			}
			image = std::make_unique<Image>( bytes, length, 0, format );
			if ( !image->getPixels() || !texture ) {
				success = false;
				error = "Invalid image data";
			}
		} else if ( success ) {
			data = std::make_shared<const std::string>( response.getBody() );
			retainedBytes = data->size();
		} else {
			error = response.getStatusDescription();
		}

		if ( image && success ) {
			// Decoding may take long enough for clear() or replacement to invalidate this load.
			// Revalidate before touching the GPU. Texture::replace() remains outside the lock
			// because it uploads data and can synchronously notify listeners that re-enter cache/UI
			// code.
			{
				std::lock_guard<std::mutex> lock( self->mImpl->mutex );
				auto entryIt = self->mImpl->entries.find( imageLoad ? imageLoad->key : key );
				if ( entryIt == self->mImpl->entries.end() ||
					 !self->mImpl->matchesOperation( entryIt->second, operationEpoch,
													 operationId ) )
					return;
			}
			texture->setRasterScale( rasterScale );
			texture->replace( image.get() );
			retainedBytes = static_cast<std::size_t>( texture->getImageWidth() ) *
							texture->getImageHeight() * 4;
		}

		std::vector<Impl::Subscriber> subscribers;
		std::vector<Impl::Subscriber> discardedSubscribers;
		std::vector<Impl::Entry> releasedEntries;
		WebResourceResult result;
		std::shared_ptr<const std::string> previousData;
		WebResourceRequest completedRequest;
		{
			std::lock_guard<std::mutex> lock( self->mImpl->mutex );
			auto entryIt = self->mImpl->entries.find( imageLoad ? imageLoad->key : key );
			// Key and state are insufficient here: clear() may have removed the old entry and a
			// newer request may already have created another Loading entry for the same key.
			if ( entryIt == self->mImpl->entries.end() ||
				 !self->mImpl->matchesOperation( entryIt->second, operationEpoch, operationId ) )
				return;
			auto& entry = entryIt->second;
			self->mImpl->finishImageLoad( entry.imageLoad );
			entry.imageLoad.reset();
			self->mImpl->inFlightCount--;
			entry.status = status;
			entry.lastUsed = self->mImpl->now();
			entry.expiresAt = entry.lastUsed + self->mImpl->ttl;
			previousData = std::move( entry.data );
			entry.retainedBytes = retainedBytes;
			if ( success ) {
				entry.state = WebResourceLoadState::Ready;
				entry.data = std::move( data );
				entry.bitmap = bitmap;
				entry.error.clear();
				self->mImpl->retainedBytes += entry.retainedBytes;
				result = { true,	   entry.state,	 entry.status, {}, std::move( cookies ),
						   entry.data, entry.texture };
			} else {
				entry.state = WebResourceLoadState::Failed;
				entry.error = std::move( error );
				result = { false, entry.state,	entry.status, entry.error, std::move( cookies ),
						   {},	  entry.texture };
			}
			if ( entry.state == WebResourceLoadState::Failed )
				entry.retryAt = entry.lastUsed + self->mImpl->retryDelay;
			completedRequest = std::move( entry.request );
			entry.operationEpoch = 0;
			entry.operationId = 0;
			std::vector<Impl::Subscriber> pendingSubscribers;
			pendingSubscribers.swap( entry.subscribers );
			for ( auto& subscriber : pendingSubscribers ) {
				if ( self->mImpl->isCurrent( subscriber ) )
					subscribers.emplace_back( std::move( subscriber ) );
				else
					discardedSubscribers.emplace_back( std::move( subscriber ) );
			}
			if ( success && bitmap && completedRequest.svgScale != 1.f ) {
				// The requested scale mattered until the response was identified. Retain
				// bitmaps under the density-independent key for subsequent lookups.
				auto bitmapKey = makeRequestKey( entry.partition, completedRequest, 1.f );
				if ( self->mImpl->entries.find( bitmapKey ) == self->mImpl->entries.end() ) {
					Impl::Entry bitmapEntry = std::move( entry );
					self->mImpl->entries.erase( entryIt );
					self->mImpl->entries.emplace( std::move( bitmapKey ),
												  std::move( bitmapEntry ) );
				}
			}
			self->mImpl->pruneLocked( releasedEntries );
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
		// Attribute cookies when a response's headers arrive, using the client that received it.
		// Predicting the next hop from Location is wrong whenever Http rejects that hop: the
		// redirect response itself then completes the request.
		Http::Request::ProgressCallback progress =
			[trail]( const Http& http, const Http::Request&, const Http::Response& response,
					 const Http::Request::Status& status, std::size_t, std::size_t ) {
				if ( status != Http::Request::Status::HeaderReceived )
					return true;
				trail->observedHeaders = true;
				if ( !response.hasField( "set-cookie" ) )
					return true;
				const URI responder = http.getURI();
				const URI& requested = trail->requested;
				// Same origin keeps the requested authority, as UISceneNode looks cookies up by it.
				const bool sameOrigin = responder.getScheme() == requested.getScheme() &&
										responder.getPort() == requested.getPort() &&
										String::toLower( responder.getHost() ) ==
											String::toLower( requested.getHost() );
				trail->cookies.push_back(
					{ sameOrigin ? requested.getAuthority() : responder.getAuthority(),
					  response.getField( "set-cookie" ) } );
				return true;
			};
		// The partition owns transport lifetime across navigations and across intentionally
		// shared views. Cookies remain request/scene state, never connection state.
		auto client = transport->get( request.uri, request.proxy );
		Http::Request httpRequest( request.uri.getPathAndQuery(), request.method, request.body,
								   request.validateCertificate, request.validateCertificate,
								   request.followRedirect, true );
		httpRequest.setProgressCallback( progress );
		for ( const auto& header : request.headers )
			httpRequest.setField( header.first, header.second );
		client->sendAsyncRequest(
			[complete = std::move( complete )]( const Http&, Http::Request&,
												Http::Response& response ) mutable {
				complete( std::move( response ) );
			},
			httpRequest, request.timeout );
	}
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
	std::vector<Impl::Entry> releasedEntries;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		mImpl->byteBudget = bytes;
		mImpl->pruneLocked( releasedEntries );
	}
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
	std::vector<Impl::Entry> releasedEntries;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		mImpl->pruneLocked( releasedEntries );
	}
}

void WebResourceCache::clear() {
	std::vector<Impl::Entry> releasedEntries;
	decltype( mImpl->transports ) transports;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		// Default transports are cancelled outside the lock. A custom fetcher need not support
		// cancellation, so advance the epoch before resetting entries to reject its late
		// completion.
		++mImpl->cacheEpoch;
		transports.swap( mImpl->transports );
		releasedEntries.reserve( mImpl->entries.size() );
		for ( auto& entry : mImpl->entries ) {
			entry.second.state = WebResourceLoadState::Cancelled;
			releasedEntries.emplace_back( std::move( entry.second ) );
		}
		// Values are moved out first so clearing the container cannot release resource handles
		// while the cache mutex is held.
		mImpl->entries.clear();
		for ( auto& session : mImpl->sessions )
			session.second.pendingImages.clear();
		mImpl->retainedBytes = 0;
		mImpl->inFlightCount = 0;
	}
	for ( auto& transport : transports )
		transport.second->close();
}

void WebResourceCache::setFetcher( Fetcher fetcher ) {
	Fetcher previousFetcher;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		previousFetcher = std::move( mImpl->fetcher );
		mImpl->fetcher = std::move( fetcher );
	}
}

}} // namespace EE::UI
