#include "thumbnails.hpp"
#include "app.hpp"
#include "pane.hpp"
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uitablecell.hpp>

using namespace EE::Graphics;
using namespace EE::UI;
using namespace EE::VFS;

namespace eefm {

struct Thumbnails::View : std::enable_shared_from_this<View> {
	struct Entry {
		std::shared_ptr<const URI> uri;
		CancellationSource cancellation;
		std::shared_ptr<const Image> image;
		DrawablePtr drawable;
		Uint64 size;
		FileRevision revision;
		bool seen{ false };

		~Entry() { cancellation.cancel(); }
	};

	URI directory;
	UnorderedMap<std::string, std::shared_ptr<Entry>> entries;
	FileTableView* widget;
	int pixels{ 0 };
	unsigned uploads{ 0 };
	bool redrawPending{ false };

	void redrawSoon() {
		if ( redrawPending || !widget )
			return;
		redrawPending = true;
		widget->runOnMainThread(
			[weak = weak_from_this()] {
				if ( auto view = weak.lock(); view && view->widget ) {
					view->redrawPending = false;
					view->widget->invalidateDraw();
				}
			},
			Milliseconds( 16 ) );
	}
};

Thumbnails::Thumbnails( App& app ) : mApp( app ) {}

Thumbnails::~Thumbnails() {
	shutdown();
}

void Thumbnails::attach( FileTab* tab ) {
	auto state = std::make_shared<View>();
	state->widget = tab->view;
	mViews[tab->view] = state;
	tab->view->onVisibleRows = [this, tab]( size_t first, size_t end ) {
		updateVisible( tab, first, end );
	};
	tab->view->setOnUpdateCellCb(
		[this, tab]( UITableCell* cell, Model* ) { updateCell( tab, cell ); } );
	tab->view->on( Event::OnVisibleChange, [weak = std::weak_ptr<View>( state )]( const Event* ) {
		if ( auto current = weak.lock();
			 current && current->widget && !current->widget->isVisible() ) {
			current->widget->releaseThumbnailIcons();
			current->entries.clear();
		}
	} );
	tab->view->on( Event::OnClose,
				   [this, widget = tab->view, weak = std::weak_ptr<View>( state )]( const Event* ) {
					   if ( auto current = weak.lock() ) {
						   current->widget = nullptr;
						   current->entries.clear();
					   }
					   mViews.erase( widget );
				   } );
}

DrawablePtr Thumbnails::icon( FileTab* tab, const VFSFileInfo& info ) const {
	if ( !mApp.config().files.thumbnails )
		return {};
	auto view = mViews.find( tab->view );
	if ( view == mViews.end() )
		return {};
	auto entry = view->second->entries.find( info.name );
	return entry != view->second->entries.end() && entry->second->revision == info.revision
			   ? entry->second->drawable
			   : DrawablePtr{};
}

void Thumbnails::configure() {
	if ( mService ) {
		mService->setDiskCacheEnabled( mApp.config().files.cacheThumbnails );
		mService->setCacheFormat( mApp.config().files.thumbnailCacheFormat );
	}
	for ( const auto& entry : mViews ) {
		entry.second->entries.clear();
		entry.first->releaseThumbnailIcons();
		entry.first->invalidateDraw();
	}
}

void Thumbnails::updateVisible( FileTab* tab, size_t first, size_t end ) {
	if ( mStopped )
		return;
	// Hidden ancestors (inactive tabs or a closed split pane) need no background requests.
	for ( auto& [widget, state] : mViews ) {
		for ( auto* node = static_cast<Node*>( widget ); node; node = node->getParent() ) {
			if ( !node->isVisible() ) {
				widget->releaseThumbnailIcons();
				state->entries.clear();
				break;
			}
		}
	}
	auto state = mViews.at( tab->view );
	state->uploads = 0;
	if ( !mApp.config().files.thumbnails ||
		 ( tab->model->uri().getScheme() != "file" && !mApp.config().files.remoteThumbnails ) ) {
		state->entries.clear();
		return;
	}
	const int pixels = std::clamp( int( tab->view->getIconSize() ), 1, 1024 );
	// Buckets are physical pixels, so a density change naturally requests the right resolution.
	int bucket = 128;
	while ( bucket < pixels && bucket < 1024 )
		bucket *= 2;
	if ( state->directory != tab->model->uri() || state->pixels != bucket ) {
		state->entries.clear();
		state->directory = tab->model->uri();
		state->pixels = bucket;
	}
	for ( auto& item : state->entries )
		item.second->seen = false;
	if ( !mService ) {
		ImageThumbnailer::Config config;
		config.diskCache = mApp.config().files.cacheThumbnails;
		config.cacheFormat = mApp.config().files.thumbnailCacheFormat;
		mService = std::make_unique<ImageThumbnailer>( std::move( config ) );
	}
	for ( size_t row = first; row < end && row < tab->model->rowCount(); ++row ) {
		const auto& info = tab->model->entry( row );
		if ( !info.isRegularFile() || !Image::isImageExtension( info.name ) )
			continue;
		auto found = state->entries.find( info.name );
		if ( found != state->entries.end() &&
			 ( found->second->revision != info.revision ||
			   found->second->size != info.size().value_or( 0 ) ) ) {
			state->entries.erase( found );
			found = state->entries.end();
		}
		if ( found != state->entries.end() ) {
			found->second->seen = true;
			continue;
		}
		if ( state->entries.size() >= 128 )
			break;
		auto entry = std::make_shared<View::Entry>();
		entry->uri = std::make_shared<const URI>( tab->model->entryURI( row ) );
		entry->revision = info.revision;
		entry->size = info.size().value_or( 0 );
		entry->seen = true;
		state->entries.emplace( info.name, entry );
		URI identity = *entry->uri;
		auto user = identity.getUserInfo();
		const auto colon = user.find( ':' );
		if ( colon != std::string::npos )
			identity.setUserInfo( user.substr( 0, colon ) );
		ImageThumbnailer::Source source;
		source.key = identity.toString();
		source.revision = info.revision.toHexString();
		// The same provider supplies listing and verification revisions; no scheme-specific cache
		// logic.
		source.open = [vfs = mApp.vfs(),
					   uri = entry->uri]( CancellationToken token ) -> std::unique_ptr<IOStream> {
			auto opened = std::move( vfs->openStream( *uri, {}, token, 128 * 1024 * 1024 ) ).get();
			return opened && *opened ? std::move( opened->value() ) : nullptr;
		};
		source.currentRevision = [vfs = mApp.vfs(), uri = entry->uri] {
			auto info = std::move( vfs->stat( *uri ) ).get();
			return info && *info ? info->value().revision.toHexString() : std::string{};
		};
		mService
			->request( std::move( source ), { bucket, bucket }, entry->cancellation.token(), 100 )
			.then( mApp.executor(), [weak = std::weak_ptr<View::Entry>( entry ),
									 view = std::weak_ptr<View>( state )](
										ImageThumbnailer::Result&& result ) {
				auto current = weak.lock();
				auto owner = view.lock();
				if ( !current || !owner || !owner->widget ||
					 current->cancellation.token().isCancellationRequested() )
					return;
				current->image = std::move( result.image );
				if ( result.error == ImageThumbnailer::Error::Busy ||
					 result.error == ImageThumbnailer::Error::Changed ) {
					for ( auto it = owner->entries.begin(); it != owner->entries.end(); ++it ) {
						if ( it->second == current ) {
							owner->entries.erase( it );
							break;
						}
					}
				}
				owner->redrawSoon();
			} );
	}
	for ( auto it = state->entries.begin(); it != state->entries.end(); ) {
		if ( !it->second->seen )
			it = state->entries.erase( it );
		else
			++it;
	}
}

void Thumbnails::updateCell( FileTab* tab, UITableCell* cell ) {
	if ( cell->getCurIndex().column() != VFSDirectoryModel::Name )
		return;
	auto* icon = cell->getIcon();
	if ( !mApp.config().files.thumbnails || mStopped ) {
		icon->removeClass( "image_thumbnail" );
		return;
	}
	const auto row = cell->getCurIndex().row();
	if ( row < 0 || size_t( row ) >= tab->model->rowCount() )
		return;
	auto state = mViews.at( tab->view );
	const auto& info = tab->model->entry( row );
	auto found = state->entries.find( info.name );
	if ( found == state->entries.end() ) {
		icon->removeClass( "image_thumbnail" );
		return;
	}
	auto& entry = *found->second;
	if ( entry.image && !entry.drawable ) {
		if ( state->uploads >= 2 ) {
			state->redrawSoon();
			return;
		}
		++state->uploads;
		const auto& image = *entry.image;
		auto texture = TextureFactory::instance()->loadFromPixels(
			image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false,
			Texture::ClampMode::ClampToEdge, false, false );
		if ( texture )
			entry.drawable = TextureDrawable::New( std::move( texture ) );
		entry.image.reset();
	}
	if ( entry.drawable ) {
		cell->setIcon( entry.drawable );
		icon->addClass( "image_thumbnail" );
	} else {
		icon->removeClass( "image_thumbnail" );
	}
}

void Thumbnails::clearCache() {
	if ( !mService )
		mService = std::make_unique<ImageThumbnailer>();
	mService->setDiskCacheEnabled( mApp.config().files.cacheThumbnails );
	mService->setCacheFormat( mApp.config().files.thumbnailCacheFormat );
	mService->clearCache();
}

void Thumbnails::shutdown() {
	if ( mStopped )
		return;
	mStopped = true;
	for ( auto& item : mViews )
		item.second->entries.clear();
	if ( mService )
		mService->shutdown();
}

} // namespace eefm
