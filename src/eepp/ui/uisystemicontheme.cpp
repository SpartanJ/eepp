#include "icons/systemicons.hpp"

#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

#include <algorithm>
#include <cstdlib>
#include <mutex>

using namespace EE::Graphics;
using namespace EE::System;

namespace EE { namespace UI {

namespace {

std::string lowerExtension( const std::string& name ) {
	size_t dot = name.find_last_of( '.' );
	if ( dot == std::string::npos || dot + 1 == name.size() )
		return {};
	return String::toLower( name.substr( dot + 1 ) );
}

/** Reads and rasterizes an icon file (SVG or PNG) at size pixels. Any thread. */
std::unique_ptr<Image> loadIconFile( const std::string& path, int size ) {
	std::unique_ptr<Image> image;
	if ( lowerExtension( path ) == "svg" ) {
		std::string svg;
		if ( !FileSystem::fileGet( path, svg ) )
			return nullptr;
		Image::FormatConfiguration format;
		int width = 0, height = 0, channels = 0;
		if ( !Image::getInfoFromMemory( reinterpret_cast<const unsigned char*>( svg.data() ),
										svg.size(), &width, &height, &channels, format ) ||
			 width <= 0 || height <= 0 )
			return nullptr;
		format.svgScale( size / static_cast<Float>( std::max( width, height ) ) );
		image = std::make_unique<Image>( reinterpret_cast<const Uint8*>( svg.data() ),
										 static_cast<unsigned int>( svg.size() ), 4, format );
	} else {
		image = std::make_unique<Image>( path, 4 );
	}
	if ( !image->getPixelsPtr() || image->getWidth() == 0 || image->getHeight() == 0 )
		return nullptr;
	Private::fitImage( *image, size );
	return image;
}

} // namespace

namespace Private {

void fitImage( Image& image, int size ) {
	unsigned int largest = std::max( image.getWidth(), image.getHeight() );
	if ( largest == 0 || largest == static_cast<unsigned int>( size ) )
		return;
	Float scale = size / static_cast<Float>( largest );
	image.resize( std::max( 1u, static_cast<unsigned int>( image.getWidth() * scale + 0.5f ) ),
				  std::max( 1u, static_cast<unsigned int>( image.getHeight() * scale + 0.5f ) ) );
}

} // namespace Private

// ---------------------------------------------------------------------------------------------
// UISystemIconTheme
// ---------------------------------------------------------------------------------------------

/** Platform icon source. Thread-safe: loads run concurrently on the worker. */
class UISystemIconTheme::Backend {
  public:
	explicit Backend( std::string themeName ) : mRequestedTheme( std::move( themeName ) ) {}

	/** Detects the theme and indexes it (freedesktop). Blocking, once. */
	void init() {
		std::call_once( mInit, [this] {
#ifdef EE_SYSTEM_ICONS_FREEDESKTOP
			std::string name = mRequestedTheme.empty()
								   ? Private::FreedesktopIconLookup::detectThemeName()
								   : mRequestedTheme;
			mLookup = std::make_unique<Private::FreedesktopIconLookup>( name );
#endif
		} );
	}

	std::string themeName() {
		init();
		return mLookup ? mLookup->getThemeName() : std::string();
	}

	std::unique_ptr<Image> load( const Request& request, int size ) {
		init();
#if EE_PLATFORM == EE_PLATFORM_WIN
		return Private::loadWindowsIcon( request, size );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
		std::vector<const char*> names;
		for ( const auto& name : request.names )
			names.push_back( name.c_str() );
		unsigned char* pixels = eeppSystemIconRGBA(
			static_cast<int>( request.kind ), request.value.c_str(), request.isDirectory ? 1 : 0,
			names.data(), static_cast<int>( names.size() ), size );
		if ( !pixels )
			return nullptr;
		auto image = std::make_unique<Image>( static_cast<const Uint8*>( pixels ), size, size, 4 );
		std::free( pixels );
		return image;
#else
		if ( !mLookup || !mLookup->isValid() )
			return nullptr;
		std::string path = mLookup->find( request.names, size );
		return path.empty() ? nullptr : loadIconFile( path, size );
#endif
	}

  private:
	std::string mRequestedTheme;
	std::once_flag mInit;
	std::unique_ptr<Private::FreedesktopIconLookup> mLookup;
};

/** An icon whose images are loaded on demand, per size; shows the fallback until then. */
class UISystemIconTheme::Icon : public UIIcon {
  public:
	Icon( const std::string& key, Request request, UIIcon* fallback,
		  std::weak_ptr<UISystemIconTheme> theme ) :
		UIIcon( key ),
		mRequest( std::move( request ) ),
		mFallback( fallback ),
		mTheme( std::move( theme ) ) {}

	const DrawablePtr& getSource( const int& size ) const override {
		static const DrawablePtr empty;
		auto found = mSizes.find( size );
		if ( found != mSizes.end() )
			return found->second;
		if ( !mMissing && size > 0 &&
			 std::find( mRequested.begin(), mRequested.end(), size ) == mRequested.end() ) {
			mRequested.push_back( size );
			if ( auto theme = mTheme.lock() )
				theme->load( const_cast<Icon*>( this ), size );
		}
		if ( !mSizes.empty() )
			return UIIcon::getSource( size );
		return mFallback ? mFallback->getSource( size ) : empty;
	}

	Request mRequest;
	UIIcon* mFallback{ nullptr };
	std::weak_ptr<UISystemIconTheme> mTheme;
	/** Sizes asked for: one or two in practice. */
	mutable SmallVector<int, 4> mRequested;
	/** The platform has no image for it: the fallback is final. */
	bool mMissing{ false };
};

std::shared_ptr<UISystemIconTheme> UISystemIconTheme::New( Executor& worker, Executor& ui,
														   const std::string& themeName ) {
	std::shared_ptr<UISystemIconTheme> theme( new UISystemIconTheme( worker, ui, themeName ) );
	// Detection and indexing start now, so the first icons are ready sooner.
	std::weak_ptr<UISystemIconTheme> weak = theme;
	std::shared_ptr<Backend> backend = theme->mBackend;
	Executor* uiExecutor = &ui;
	worker.execute( [weak, backend, uiExecutor] {
		std::string name = backend->themeName();
		uiExecutor->execute( [weak, name = std::move( name )] {
			if ( auto self = weak.lock() )
				self->mThemeName = name;
		} );
	} );
	return theme;
}

UISystemIconTheme::UISystemIconTheme( Executor& worker, Executor& ui,
									  const std::string& themeName ) :
	mWorker( worker ), mUI( ui ), mBackend( std::make_shared<Backend>( themeName ) ) {}

UISystemIconTheme::~UISystemIconTheme() {}

static std::string namesKey( const std::vector<std::string>& names ) {
	size_t length = 2;
	for ( const auto& name : names )
		length += name.size() + 1;
	std::string key;
	key.reserve( length );
	key += "n:";
	for ( const auto& name : names ) {
		key += name;
		key += '|';
	}
	return key;
}

UIIcon* UISystemIconTheme::findIcon( const std::string& key ) const {
	auto found = mIcons.find( key );
	return found != mIcons.end() ? found->second.get() : nullptr;
}

UIIcon* UISystemIconTheme::icon( std::string key, Request request, UIIcon* fallback ) {
	UIIconPtr icon( eeNew( Icon, ( key, std::move( request ), fallback, weak_from_this() ) ),
					ResourceDeleter<UIIcon>() );
	UIIcon* raw = icon.get();
	mIcons.emplace( std::move( key ), std::move( icon ) );
	return raw;
}

UIIcon* UISystemIconTheme::getIcon( const std::vector<std::string>& names, UIIcon* fallback ) {
	std::string key = namesKey( names );
	if ( UIIcon* found = findIcon( key ) )
		return found;
	Request request;
	request.kind = Request::Kind::Names;
	request.names = names;
	return icon( std::move( key ), std::move( request ), fallback );
}

UIIcon* UISystemIconTheme::getFileIcon( const std::string& fileName, bool isDirectory,
										const std::vector<std::string>& names, UIIcon* fallback ) {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS
	// The shell knows types by extension: one icon per extension.
	std::string extension = isDirectory ? std::string() : lowerExtension( fileName );
	std::string key = isDirectory ? std::string( "d:" ) : "e:" + extension;
	if ( UIIcon* found = findIcon( key ) )
		return found;
	Request request;
	request.kind = Request::Kind::File;
	request.names = names;
	request.isDirectory = isDirectory;
	request.value = extension.empty() ? std::string( "file" ) : "file." + extension;
	return icon( std::move( key ), std::move( request ), fallback );
#else
	(void)fileName;
	(void)isDirectory;
	return getIcon( names, fallback );
#endif
}

UIIcon* UISystemIconTheme::getPathIcon( const std::string& path,
										const std::vector<std::string>& names, UIIcon* fallback ) {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS
	std::string key = "p:" + path;
	if ( UIIcon* found = findIcon( key ) )
		return found;
	Request request;
	request.kind = Request::Kind::Path;
	request.names = names;
	request.value = path;
	request.isDirectory = true;
	return icon( std::move( key ), std::move( request ), fallback );
#else
	(void)path;
	return getIcon( names, fallback );
#endif
}

const std::string& UISystemIconTheme::getThemeName() const {
	return mThemeName;
}

void UISystemIconTheme::setOnIconsLoaded( std::function<void()> callback ) {
	mOnIconsLoaded = std::move( callback );
}

void UISystemIconTheme::load( Icon* target, int size ) {
	std::weak_ptr<UISystemIconTheme> weak = weak_from_this();
	std::shared_ptr<Backend> backend = mBackend;
	Executor* uiExecutor = &mUI;
	// The request is copied: the icon is owned by (and may die with) the theme meanwhile.
	mWorker.execute( [weak, backend, uiExecutor, key = target->getName(),
					  request = target->mRequest, size]() mutable {
		std::shared_ptr<Image> image( backend->load( request, size ) );
		uiExecutor->execute( [weak, key = std::move( key ), size, image = std::move( image )] {
			auto self = weak.lock();
			if ( !self )
				return;
			auto found = self->mIcons.find( key );
			if ( found == self->mIcons.end() )
				return;
			Icon* icon = static_cast<Icon*>( found->second.get() );
			if ( !image ) {
				icon->mMissing = true;
				return;
			}
			TexturePtr texture = TextureFactory::instance()->loadFromPixels(
				image->getPixelsPtr(), image->getWidth(), image->getHeight(),
				image->getChannels() );
			if ( !texture )
				return;
			icon->setSource( size, std::move( texture ) );
			self->onLoaded();
		} );
	} );
}

void UISystemIconTheme::onLoaded() {
	if ( mNotifyPending )
		return;
	// Coalesce: every image delivered in this turn triggers one redraw.
	mNotifyPending = true;
	std::weak_ptr<UISystemIconTheme> weak = weak_from_this();
	mUI.execute( [weak] {
		auto self = weak.lock();
		if ( !self )
			return;
		self->mNotifyPending = false;
		if ( self->mOnIconsLoaded )
			self->mOnIconsLoaded();
	} );
}

}} // namespace EE::UI
