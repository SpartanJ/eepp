#include "icons/systemiconbackend.hpp"

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
	std::shared_ptr<Private::SystemIconBackend> backend = theme->mBackend;
	if ( !backend )
		return theme;
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
	mWorker( worker ), mUI( ui ), mBackend( Private::SystemIconBackend::create( themeName ) ) {}

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
	if ( !mBackend || !mBackend->hasShellIcons() )
		return getIcon( names, fallback );
	// The shell knows types by extension: one icon per extension.
	std::string extension = isDirectory ? std::string() : FileSystem::fileExtension( fileName );
	std::string key = isDirectory ? std::string( "d:" ) : "e:" + extension;
	if ( UIIcon* found = findIcon( key ) )
		return found;
	Request request;
	request.kind = Request::Kind::File;
	request.names = names;
	request.isDirectory = isDirectory;
	request.value = extension.empty() ? std::string( "file" ) : "file." + extension;
	return icon( std::move( key ), std::move( request ), fallback );
}

UIIcon* UISystemIconTheme::getPathIcon( const std::string& path,
										const std::vector<std::string>& names, UIIcon* fallback ) {
	if ( !mBackend || !mBackend->hasShellIcons() )
		return getIcon( names, fallback );
	std::string key = "p:" + path;
	if ( UIIcon* found = findIcon( key ) )
		return found;
	Request request;
	request.kind = Request::Kind::Path;
	request.names = names;
	request.value = path;
	request.isDirectory = true;
	return icon( std::move( key ), std::move( request ), fallback );
}

const std::string& UISystemIconTheme::getThemeName() const {
	return mThemeName;
}

void UISystemIconTheme::setOnIconsLoaded( std::function<void()> callback ) {
	mOnIconsLoaded = std::move( callback );
}

void UISystemIconTheme::load( Icon* target, int size ) {
	std::weak_ptr<UISystemIconTheme> weak = weak_from_this();
	std::shared_ptr<Private::SystemIconBackend> backend = mBackend;
	Executor* uiExecutor = &mUI;
	// The request is copied: the icon is owned by (and may die with) the theme meanwhile.
	mWorker.execute( [weak, backend, uiExecutor, key = target->getName(),
					  request = target->mRequest, size]() mutable {
		std::shared_ptr<Image> image( backend ? backend->load( request, size ) : nullptr );
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
