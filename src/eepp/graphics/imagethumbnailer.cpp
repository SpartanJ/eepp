#include <eepp/graphics/imagethumbnailer.hpp>

#include "turbojpeg.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <eepp/core/containers.hpp>
#include <eepp/core/lrucache.hpp>
#include <eepp/graphics/exif.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sha1.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <limits>
#include <mutex>
#include <vector>
#if defined( EE_PLATFORM_POSIX )
#include <sys/stat.h>
#endif

namespace EE::Graphics {

using namespace EE::System;
using Private::TurboJpeg;
namespace {

Sizei fitted( Uint32 width, Uint32 height, Sizei box ) {
	const double scale = std::min( { 1., double( box.x ) / width, double( box.y ) / height } );
	return { std::max( 1, int( width * scale ) ), std::max( 1, int( height * scale ) ) };
}

template <int Channels>
Image areaReduce( const Image& source, Sizei size, CancellationToken token ) {
	constexpr bool alpha = Channels == 2 || Channels == 4;
	constexpr int colors = alpha ? Channels - 1 : Channels;
	Image output( size.x, size.y, Channels, Color::Transparent, false );
	const Uint64 width = source.getWidth(), height = source.getHeight();
	const Uint64 area = width * height;
	for ( int y = 0; y < size.y; ++y ) {
		if ( token.isCancellationRequested() )
			return {};
		const Uint64 top = Uint64( y ) * height, bottom = Uint64( y + 1 ) * height;
		const Uint64 firstY = top / size.y, endY = ( bottom + size.y - 1 ) / size.y;
		for ( int x = 0; x < size.x; ++x ) {
			const Uint64 left = Uint64( x ) * width, right = Uint64( x + 1 ) * width;
			const Uint64 firstX = left / size.x, endX = ( right + size.x - 1 ) / size.x;
			Uint64 sums[Channels]{};
			for ( Uint64 sy = firstY; sy < endY; ++sy ) {
				const Uint64 wy =
					std::min( bottom, ( sy + 1 ) * size.y ) - std::max( top, sy * size.y );
				const auto* pixel = source.getPixelsPtr() + ( sy * width + firstX ) * Channels;
				for ( Uint64 sx = firstX; sx < endX; ++sx, pixel += Channels ) {
					Uint64 weight = wy * ( std::min( right, ( sx + 1 ) * size.x ) -
										   std::max( left, sx * size.x ) );
					if constexpr ( alpha ) {
						weight *= pixel[Channels - 1];
						sums[Channels - 1] += weight;
					}
					for ( int c = 0; c < colors; ++c )
						sums[c] += pixel[c] * weight;
				}
			}
			auto* dst = output.getPixels() + ( size_t( y ) * size.x + x ) * Channels;
			const Uint64 denominator = alpha ? sums[Channels - 1] : area;
			for ( int c = 0; c < colors; ++c )
				dst[c] = denominator ? ( sums[c] + denominator / 2 ) / denominator : 0;
			if constexpr ( alpha )
				dst[Channels - 1] = ( sums[Channels - 1] + area / 2 ) / area;
		}
	}
	return output;
}

bool readBytes( IOStream& stream, void* target, size_t size ) {
	auto* dst = static_cast<char*>( target );
	while ( size ) {
		const auto read = stream.read( dst, size );
		if ( read <= 0 || Uint64( read ) > size )
			return false;
		dst += read;
		size -= read;
	}
	return true;
}

} // namespace

Image ImageThumbnailer::reduce( const Image& image, Sizei box, CancellationToken token ) {
	if ( !image.getPixelsPtr() || !image.getWidth() || !image.getHeight() || box.x <= 0 ||
		 box.y <= 0 || box.x > 1024 || box.y > 1024 || token.isCancellationRequested() )
		return {};
	const auto size = fitted( image.getWidth(), image.getHeight(), box );
	if ( Uint32( size.x ) == image.getWidth() && Uint32( size.y ) == image.getHeight() )
		return Image( image );
	switch ( image.getChannels() ) {
		case 1:
			return areaReduce<1>( image, size, token );
		case 2:
			return areaReduce<2>( image, size, token );
		case 3:
			return areaReduce<3>( image, size, token );
		case 4:
			return areaReduce<4>( image, size, token );
		default:
			return {};
	}
}

struct ImageThumbnailer::Impl {
	struct Waiter {
		TaskResolver<Result> resolve;
		CancellationToken token;
		CancellationRegistration registration;
	};

	struct Job {
		Source source;
		std::string key;
		std::vector<Waiter> waiters;
		std::mutex mutex;
		CancellationSource cancellation;
		Uint64 epoch;
		Sizei box;
		int priority;
		CacheFormat format;

		void cancelIfUnused() {
			bool unused;
			{
				std::lock_guard<std::mutex> lock( mutex );
				unused = std::all_of( waiters.begin(), waiters.end(), []( const Waiter& waiter ) {
					return waiter.token.isCancellationRequested();
				} );
			}
			if ( unused )
				cancellation.cancel();
		}
	};

	Config config;
	std::mutex mutex, diskMutex;
	std::condition_variable memoryAvailable;
	UnorderedMap<std::string, std::shared_ptr<Job>> jobs;
	WeightedLRUCache<std::string, std::shared_ptr<const Image>> cache;
	std::deque<std::shared_ptr<Job>> queue;
	std::unique_ptr<ThreadPool> pool;
	Uint64 decoding{ 0 }, unknown{ 0 }, writes{ 0 }, diskUsed{ 0 };
	std::atomic<Uint64> epoch{ 0 };
	Uint32 running{ 0 };
	std::atomic<bool> diskEnabled{ true };
	std::atomic<CacheFormat> diskFormat{ CacheFormat::QOI };
	bool stopped{ false };
	bool diskKnown{ false };

	explicit Impl( Config options ) :
		config( std::move( options ) ), cache( config.memoryCacheBytes ) {
		diskEnabled = config.diskCache;
		diskFormat = cacheFormatFromString( cacheFormatToString( config.cacheFormat ) );
		config.workers = std::clamp( config.workers, 1u, 4u );
		config.maxPending = std::max( 1u, config.maxPending );
		if ( config.cachePath.empty() ) {
			config.cachePath = Sys::getCachePath( "eepp" );
			FileSystem::dirAddSlashAtEnd( config.cachePath );
			config.cachePath += "thumbnails/v1/";
		}
		FileSystem::dirAddSlashAtEnd( config.cachePath );
		pool = ThreadPool::createUnique( config.workers );
	}

	static std::string keyFor( const Source& source, Sizei box ) {
		SHA1::Context hash;
		SHA1::init( hash );
		// Length-prefix fields, avoiding ambiguous concatenations and persisting no raw URIs.
		const auto append = [&]( std::string_view value ) {
			char length[std::numeric_limits<Uint64>::digits10 + 2];
			auto* end = std::to_chars( length, length + sizeof( length ) - 1, value.size() ).ptr;
			*end++ = ':';
			SHA1::update( hash, length, end - length );
			SHA1::update( hash, value.data(), value.size() );
		};
		append( "eepp-thumbnail-1" );
		append( source.key );
		append( source.revision );
		char dimension[std::numeric_limits<int>::digits10 + 2];
		for ( int value : { box.x, box.y } ) {
			const auto* end =
				std::to_chars( dimension, dimension + sizeof( dimension ), value ).ptr;
			append( std::string_view( dimension, end - dimension ) );
		}
		return SHA1::result( hash ).toHexString();
	}

	std::shared_ptr<const Image> readCache( const Job& job ) {
		if ( !diskEnabled || !job.source.persistent || job.source.revision.empty() )
			return {};
		std::lock_guard<std::mutex> lock( diskMutex );
		const auto path = String::format( "%s%s.%s", config.cachePath, job.key,
										  cacheFormatToString( job.format ) );
		IOStreamFile file( path );
		if ( !file.isOpen() || file.getSize() <= 0 || file.getSize() > 8 * 1024 * 1024 )
			return {};
		std::vector<Uint8> bytes( file.getSize() );
		if ( !readBytes( file, bytes.data(), bytes.size() ) )
			return {};
		const auto format = Image::getFormat( bytes.data(), bytes.size() );
		if ( !( ( job.format == CacheFormat::QOI && format == Image::Format::QOI ) ||
				( job.format == CacheFormat::PNG && format == Image::Format::PNG ) ||
				( job.format == CacheFormat::WebP && format == Image::Format::WEBP ) ||
				( job.format == CacheFormat::TGA && format == Image::Format::TGA ) ) )
			return {};
		if ( job.format == CacheFormat::QOI ) {
			constexpr Uint8 end[] = { 0, 0, 0, 0, 0, 0, 0, 1 };
			if ( bytes.size() < 22 || std::memcmp( bytes.data() + bytes.size() - 8, end, 8 ) != 0 )
				return {};
		}
		int width = 0, height = 0, channels = 0;
		if ( !Image::getInfoFromMemory( bytes.data(), bytes.size(), &width, &height, &channels ) ||
			 width <= 0 || height <= 0 || width > job.box.x || height > job.box.y || channels < 1 ||
			 channels > 4 )
			return {};
		auto image = std::make_shared<Image>( bytes.data(), bytes.size() );
		return image->getPixelsPtr() && image->getWidth() == Uint32( width ) &&
					   image->getHeight() == Uint32( height )
				   ? image
				   : nullptr;
	}

	static bool isCacheFile( const FileInfo& file ) {
		const auto& name = file.getFileName();
		if ( !file.isRegularFile() || name.size() < 44 || name[40] != '.' ||
			 !std::all_of( name.begin(), name.begin() + 40, []( unsigned char ch ) {
				 return ( ch >= '0' && ch <= '9' ) || ( ch >= 'a' && ch <= 'f' );
			 } ) )
			return false;
		const auto extension = std::string_view( name ).substr( 41 );
		return extension == "png" || extension == "qoi" || extension == "webp" ||
			   extension == "tga";
	}

	static bool saveCacheImage( CacheFormat format, const Image& image, const std::string& path ) {
		Image expanded;
		const Image* input = &image;
		if ( image.getChannels() < 3 &&
			 ( format == CacheFormat::QOI || format == CacheFormat::WebP ) ) {
			const auto channels = image.getChannels() == 2 ? 4u : 3u;
			expanded.create( image.getWidth(), image.getHeight(), channels, Color::Transparent,
							 false );
			for ( size_t i = 0; i < size_t( image.getWidth() ) * image.getHeight(); ++i ) {
				const auto* src = image.getPixelsPtr() + i * image.getChannels();
				auto* dst = expanded.getPixels() + i * channels;
				dst[0] = dst[1] = dst[2] = src[0];
				if ( channels == 4 )
					dst[3] = src[1];
			}
			input = &expanded;
		}
		// The stack view borrows immutable pixels solely to set encoder options without a copy.
		Image writer( input->getPixels(), input->getWidth(), input->getHeight(),
					  input->getChannels() );
		writer.avoidFreeImage( true );
		if ( format == CacheFormat::WebP ) {
			Image::FormatConfiguration options;
			options.webpSaveLossless( true );
			options.webpSaveQuality( 0 );
			options.webpCompressionMethod( 0 );
			writer.setImageFormatConfiguration( options );
		}
		return writer.saveToFile( path,
								  Image::extensionToSaveType( cacheFormatToString( format ) ) );
	}

	void pruneDisk() {
		auto entries = FileSystem::filesInfoGetInPath( config.cachePath );
		Uint64 total = 0;
		std::erase_if( entries, []( const FileInfo& entry ) { return !isCacheFile( entry ); } );
		for ( const auto& entry : entries )
			total += entry.getSize();
		std::sort( entries.begin(), entries.end(), []( const FileInfo& a, const FileInfo& b ) {
			return a.getModificationTime() < b.getModificationTime();
		} );
		const Uint64 target =
			total > config.diskCacheBytes ? config.diskCacheBytes / 10 * 9 : config.diskCacheBytes;
		for ( const auto& entry : entries ) {
			if ( total <= target )
				break;
			if ( FileSystem::fileRemove( entry.getFilepath() ) )
				total -= entry.getSize();
		}
		diskUsed = total;
		diskKnown = true;
	}

	void writeCache( const Job& job, const Image& image ) {
		if ( !diskEnabled || !job.source.persistent || job.source.revision.empty() ||
			 job.cancellation.token().isCancellationRequested() || job.epoch != epoch )
			return;
		std::lock_guard<std::mutex> lock( diskMutex );
		if ( !FileSystem::makeDir( config.cachePath, true, 0700 ) )
			return;
		const auto path = String::format( "%s%s.%s", config.cachePath, job.key,
										  cacheFormatToString( job.format ) );
		const Uint64 previousSize = FileSystem::fileSize( path );
		const auto temporary = String::format(
			"%s.%llu.%llu.tmp", path, static_cast<unsigned long long>( Sys::getProcessID() ),
			static_cast<unsigned long long>( ++writes ) );
		ScopedOp removeTemporary( nullptr, [&] { FileSystem::fileRemove( temporary ); } );
		{
			IOStreamFile file( temporary, "wb" );
			if ( !file.isOpen() )
				return;
#if defined( EE_PLATFORM_POSIX )
			chmod( temporary.c_str(), 0600 );
#endif
		}
		const bool saved = saveCacheImage( job.format, image, temporary );
		if ( saved && !job.cancellation.token().isCancellationRequested() && job.epoch == epoch &&
			 FileSystem::fileMove( temporary, path ) ) {
			diskUsed = diskUsed >= previousSize ? diskUsed - previousSize : 0;
			diskUsed += FileSystem::fileSize( path );
			if ( !diskKnown || diskUsed > config.diskCacheBytes || writes % 32 == 0 )
				pruneDisk();
		}
	}

	Result generate( Job& job );

	void drain();
};

ImageThumbnailer::Result ImageThumbnailer::Impl::generate( Job& job ) {
	const auto token = job.cancellation.token();
	if ( token.isCancellationRequested() )
		return { {}, Error::Cancelled };
	if ( auto image = readCache( job ) )
		return { std::move( image ), Error::None, true };
	if ( !job.source.open )
		return { {}, Error::IO };
	auto stream = job.source.open( token );
	if ( !stream || !stream->isOpen() || stream->getSize() < 0 )
		return { {}, Error::IO };
	if ( Uint64( stream->getSize() ) > config.maxSourceBytes )
		return { {}, Error::TooLarge };
	const auto metadata = Exif::read( *stream );
	if ( token.isCancellationRequested() )
		return { {}, Error::Cancelled };
	stream->seek( 0 );
	std::vector<Uint8> header( std::min<Uint64>( 128 * 1024, stream->getSize() ) );
	if ( !readBytes( *stream, header.data(), header.size() ) )
		return { {}, Error::IO };
	const auto format = Image::getFormat( header.data(), header.size() );
	if ( format == Image::Format::SVG ) {
		if ( stream->getSize() > 4 * 1024 * 1024 )
			return { {}, Error::TooLarge };
		header.resize( stream->getSize() );
		if ( stream->seek( 0 ) != 0 || !readBytes( *stream, header.data(), header.size() ) )
			return { {}, Error::IO };
	}
	int width = 0, height = 0, channels = 0;
	bool hasInfo =
		Image::getInfoFromMemory( header.data(), header.size(), &width, &height, &channels );
	if ( !hasInfo )
		hasInfo = Image::getInfoFromStream( *stream, &width, &height, &channels );
	if ( !hasInfo || width <= 0 || height <= 0 )
		return { {}, Error::Unsupported };
	const Uint64 pixels = Uint64( width ) * height;
	if ( pixels > config.maxPixels || pixels > ( std::numeric_limits<Uint32>::max() / 4 ) )
		return { {}, Error::TooLarge };
	const Uint64 reservation =
		pixels * ( format == Image::Format::HDR ? 16 : 12 ) + Uint64( stream->getSize() );
	if ( reservation > config.decodeBytes )
		return { {}, Error::TooLarge };
	auto wake = token.onCancel( [this] { memoryAvailable.notify_all(); } );
	{
		std::unique_lock<std::mutex> lock( mutex );
		memoryAvailable.wait( lock, [&] {
			return stopped || token.isCancellationRequested() ||
				   reservation <= config.decodeBytes - decoding;
		} );
		if ( stopped || token.isCancellationRequested() )
			return { {}, Error::Cancelled };
		decoding += reservation;
	}
	ScopedOp releaseMemory( nullptr, [this, reservation] {
		{
			std::lock_guard<std::mutex> lock( mutex );
			decoding -= reservation;
		}
		memoryAvailable.notify_all();
	} );
	Sizei target = job.box;
	if ( static_cast<Uint8>( metadata.orientation ) >= 5 )
		std::swap( target.x, target.y );
	const auto desired = fitted( width, height, target );
	Image image;
	ImageOrientation orientation = metadata.orientation;
	if ( metadata.hasThumbnail() && metadata.thumbnailLength <= 1024 * 1024 ) {
		std::vector<Uint8> bytes( metadata.thumbnailLength );
		if ( stream->seek( metadata.thumbnailOffset ) == ios_size( metadata.thumbnailOffset ) &&
			 readBytes( *stream, bytes.data(), bytes.size() ) ) {
			int w = 0, h = 0, c = 0;
			if ( Image::getInfoFromMemory( bytes.data(), bytes.size(), &w, &h, &c ) && w > 0 &&
				 h > 0 && Uint64( w ) * h <= 4 * 1024 * 1024 &&
				 Uint64( w ) * h * 12 + bytes.size() <= reservation ) {
				const bool swap = static_cast<Uint8>( metadata.thumbnailOrientation ) >= 5;
				const bool sourceSwap = static_cast<Uint8>( metadata.orientation ) >= 5;
				if ( ( swap ? h : w ) >= ( sourceSwap ? desired.y : desired.x ) &&
					 ( swap ? w : h ) >= ( sourceSwap ? desired.x : desired.y ) ) {
					image = Image( bytes.data(), bytes.size() );
					if ( image.getPixelsPtr() )
						orientation = metadata.thumbnailOrientation;
				}
			}
		}
	}
	if ( !image.getPixelsPtr() && format == Image::Format::JPEG && config.useTurboJPEG ) {
		const auto& turbo = TurboJpeg::instance( config.turboJPEGPath );
		if ( turbo.available() )
			image = turbo.decode( *stream, target, config.maxPixels );
	}

	if ( !image.getPixelsPtr() ) {
		if ( stream->seek( 0 ) != 0 )
			return { {}, Error::IO };
		image = Image( *stream, target );
	}
	if ( token.isCancellationRequested() )
		return { {}, Error::Cancelled };
	if ( !stream->isOpen() )
		return { {}, Error::IO };
	if ( !image.getPixelsPtr() )
		return { {}, Error::Unsupported };
	Sizei reduceBox = job.box;
	if ( static_cast<Uint8>( orientation ) >= 5 )
		std::swap( reduceBox.x, reduceBox.y );
	if ( image.getWidth() > Uint32( reduceBox.x ) || image.getHeight() > Uint32( reduceBox.y ) )
		image = ImageThumbnailer::reduce( image, reduceBox, token );
	if ( token.isCancellationRequested() || !image.getPixelsPtr() )
		return { {}, Error::Cancelled };
	// Cache output is normalized once, after reduction; full-size viewers use draw transforms.
	image.applyOrientation( orientation );
	return { std::make_shared<Image>( std::move( image ) ) };
}

void ImageThumbnailer::Impl::drain() {
	for ( ;; ) {
		std::shared_ptr<Job> job;
		{
			std::lock_guard<std::mutex> lock( mutex );
			if ( queue.empty() ) {
				--running;
				return;
			}
			const auto next =
				std::max_element( queue.begin(), queue.end(), []( const auto& a, const auto& b ) {
					return a->priority < b->priority;
				} );
			job = std::move( *next );
			queue.erase( next );
		}
		auto result = generate( *job );
		if ( job->cancellation.token().isCancellationRequested() )
			result = { {}, Error::Cancelled };
		else if ( result && job->source.currentRevision &&
				  job->source.currentRevision() != job->source.revision )
			result = { {}, Error::Changed };
		std::vector<Waiter> waiters;
		{
			std::lock_guard<std::mutex> lock( mutex );
			if ( result && !stopped && job->epoch == epoch && !job->source.revision.empty() )
				cache.put( job->key, result.image,
						   result.image->getMemSize() + sizeof( Image ) + 2 * job->key.size() +
							   128 );
			const auto current = jobs.find( job->key );
			if ( current != jobs.end() && current->second == job )
				jobs.erase( current );
			std::lock_guard<std::mutex> jobLock( job->mutex );
			waiters = std::move( job->waiters );
		}
		for ( auto& waiter : waiters ) {
			waiter.registration.reset();
			waiter.resolve( waiter.token.isCancellationRequested() ? Result{ {}, Error::Cancelled }
																   : result );
		}
		// Publish pixels before doing any cache encoding or disk I/O. Consumers can display the
		// image immediately; persistence remains on this worker and shutdown still joins it.
		if ( result && !result.cached )
			writeCache( *job, *result.image );
	}
}

ImageThumbnailer::ImageThumbnailer() : ImageThumbnailer( Config{} ) {}

ImageThumbnailer::ImageThumbnailer( Config config ) :
	mImpl( std::make_unique<Impl>( std::move( config ) ) ) {}

ImageThumbnailer::~ImageThumbnailer() {
	shutdown();
}

Task<ImageThumbnailer::Result> ImageThumbnailer::request( Source source, Sizei box,
														  CancellationToken token, int priority ) {
	if ( token.isCancellationRequested() )
		return makeReadyTask( Result{ {}, Error::Cancelled } );
	if ( source.key.empty() || box.x <= 0 || box.y <= 0 || box.x > 1024 || box.y > 1024 )
		return makeReadyTask( Result{ {}, Error::Unsupported } );
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	if ( mImpl->stopped )
		return makeReadyTask( Result{ {}, Error::Cancelled } );
	auto key = Impl::keyFor( source, box );
	if ( !source.revision.empty() ) {
		if ( auto cached = mImpl->cache.get( key ) )
			return makeReadyTask( Result{ std::move( *cached ), Error::None, true } );
	} else {
		key += ":" + String::toString( ++mImpl->unknown );
	}
	std::shared_ptr<Impl::Job> job;
	auto existing = mImpl->jobs.find( key );
	if ( existing != mImpl->jobs.end() &&
		 !existing->second->cancellation.token().isCancellationRequested() ) {
		job = existing->second;
		job->priority = std::max( job->priority, priority );
	} else {
		if ( mImpl->queue.size() + mImpl->running >= mImpl->config.maxPending )
			return makeReadyTask( Result{ {}, Error::Busy } );
		job = std::make_shared<Impl::Job>();
		job->source = std::move( source );
		job->key = std::move( key );
		job->box = box;
		job->priority = priority;
		job->format = mImpl->diskFormat;
		job->epoch = mImpl->epoch;
		// Keep cancelled predecessors in the bounded queue so their waiters resolve normally.
		// Completion erases the keyed job only when it still owns that slot.
		mImpl->jobs[job->key] = job;
		mImpl->queue.push_back( job );
	}
	TaskResolver<Result> resolve;
	auto task = Task<Result>::create( resolve );
	size_t index;
	{
		std::lock_guard<std::mutex> jobLock( job->mutex );
		if ( job->waiters.size() >= mImpl->config.maxPending )
			return makeReadyTask( Result{ {}, Error::Busy } );
		index = job->waiters.size();
		job->waiters.push_back( { std::move( resolve ), token, {} } );
	}
	// Register outside the job lock: onCancel can invoke synchronously. The service lock keeps
	// completion from moving waiters until registration has been installed.
	auto registration = token.onCancel( [weak = std::weak_ptr<Impl::Job>( job )] {
		if ( auto current = weak.lock() )
			current->cancelIfUnused();
	} );
	{
		std::lock_guard<std::mutex> jobLock( job->mutex );
		job->waiters[index].registration = std::move( registration );
	}
	if ( mImpl->running < mImpl->config.workers ) {
		++mImpl->running;
		mImpl->pool->run( [impl = mImpl.get()] { impl->drain(); } );
	}
	return task;
}

void ImageThumbnailer::setDiskCacheEnabled( bool enabled ) {
	mImpl->diskEnabled = enabled;
}

void ImageThumbnailer::setCacheFormat( CacheFormat format ) {
	mImpl->diskFormat = cacheFormatFromString( cacheFormatToString( format ) );
}

const char* ImageThumbnailer::cacheFormatToString( CacheFormat format ) {
	switch ( format ) {
		case CacheFormat::PNG:
			return "png";
		case CacheFormat::WebP:
			return "webp";
		case CacheFormat::TGA:
			return "tga";
		default:
			return "qoi";
	}
}

ImageThumbnailer::CacheFormat ImageThumbnailer::cacheFormatFromString( std::string_view name ) {
	if ( String::iequals( name, "png" ) )
		return CacheFormat::PNG;
	if ( String::iequals( name, "webp" ) )
		return CacheFormat::WebP;
	if ( String::iequals( name, "tga" ) )
		return CacheFormat::TGA;
	return CacheFormat::QOI;
}

void ImageThumbnailer::shutdown() {
	std::vector<std::shared_ptr<Impl::Job>> jobs;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		if ( mImpl->stopped )
			return;
		mImpl->stopped = true;
		for ( const auto& entry : mImpl->jobs )
			jobs.push_back( entry.second );
	}
	for ( const auto& job : jobs )
		job->cancellation.cancel();
	mImpl->memoryAvailable.notify_all();
	mImpl->pool.reset();
}

Task<void> ImageThumbnailer::clearCache() {
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	++mImpl->epoch;
	if ( mImpl->stopped )
		return makeReadyTask();
	return mImpl->pool->submit( [impl = mImpl.get()] {
		{
			std::lock_guard<std::mutex> cacheLock( impl->mutex );
			impl->cache.clear();
		}
		std::lock_guard<std::mutex> diskLock( impl->diskMutex );
		impl->diskUsed = 0;
		impl->diskKnown = true;
		for ( const auto& entry : FileSystem::filesInfoGetInPath( impl->config.cachePath ) ) {
			if ( Impl::isCacheFile( entry ) )
				FileSystem::fileRemove( entry.getFilepath() );
		}
	} );
}

} // namespace EE::Graphics
