#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureloader.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Graphics;

namespace EE::UI::Tools {

Uint32 UIImageViewer::displayOptionsFromString( std::string_view str ) {
	Uint32 opt = 0;
	String::readBySeparator(
		str,
		[&opt]( std::string_view chunk ) {
			if ( "name" == chunk )
				opt |= DisplayOptions::DisplayName;
			else if ( "path" == chunk )
				opt |= DisplayOptions::DisplayPath;
			else if ( "dimensions" == chunk )
				opt |= DisplayOptions::DisplayDimensions;
			else if ( "size" == chunk )
				opt |= DisplayOptions::DisplaySize;
			else if ( "type" == chunk )
				opt |= DisplayOptions::DisplayType;
			else if ( "pos" == chunk || "position" == chunk || "gallery_position" == chunk ||
					  "gallery-position" == chunk )
				opt |= DisplayOptions::DisplayGalleryPosition;
		},
		'|' );
	return opt;
}

std::string UIImageViewer::displayOptionsToString( Uint32 opt ) {
	std::string str;
	if ( opt & DisplayOptions::DisplayName )
		str += "name|";
	if ( opt & DisplayOptions::DisplayPath )
		str += "path|";
	if ( opt & DisplayOptions::DisplayDimensions )
		str += "dimensions|";
	if ( opt & DisplayOptions::DisplaySize )
		str += "size|";
	if ( opt & DisplayOptions::DisplayType )
		str += "type|";
	if ( opt & DisplayOptions::DisplayGalleryPosition )
		str += "pos|";
	if ( str.size() )
		str.pop_back();
	return str;
}

UIImageViewer* UIImageViewer::New() {
	return eeNew( UIImageViewer, () );
}

UIImageViewer::UIImageViewer() : UIWidget( "imageviewer" ) {
	mImage = UIImage::New();
	mImage->setParent( this );
	mImage->setVisible( false );
	mImage->setDragEnabled( true );
	mImage->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mImage->setScaleType( UIScaleType::FitInside );
	mLoader = UILoader::New();
	mLoader->setParent( this );
	mLoader->setIndeterminate( true );
	mLoader->setVisible( false );
	mLoader->setEnabled( false );
	mLoader->setSize( { 64, 64 } );
	mLoader->setOutlineThickness( PixelDensity::dpToPx( 6 ) );
	mTextView = UITextView::New();
	mTextView->setParent( this );
	mTextView->setEnabled( false );
	setClipType( ClipType::ContentBox );
}

UIImageViewer::~UIImageViewer() {
	mClosing = true;
	mGalleryLoaderShouldAbort = true;
	while ( mLoading )
		Sys::sleep( Milliseconds( 1 ) );
}

Uint32 UIImageViewer::getType() const {
	return UI_TYPE_IMAGE_VIEWER;
}

bool UIImageViewer::isType( const Uint32& type ) const {
	return UIImageViewer::getType() == type ? true : UIWidget::isType( type );
}

void UIImageViewer::loadImageAsync( std::string_view path, bool isContents, bool loadGallery ) {
	if ( !getUISceneNode()->hasThreadPool() )
		return;
	mLoader->setVisible( true );
	std::string ipath( path );

	mLoading++;

	getUISceneNode()->getThreadPool()->run( [path = std::move( ipath ), this, isContents,
											 loadGallery] {
		ScopedOp op( [] {}, [this] { mLoading--; } );

		auto folder( FileSystem::fileRemoveFileName( path ) );
		FileSystem::dirAddSlashAtEnd( folder );
		mHasGallery = false;
		if ( loadGallery && !isContents ) {
			Lock l( mGalleryMutex );
			bool sameFolder = folder == mGalleryPath;
			bool newFolder = false;
			if ( !sameFolder && FileSystem::isDirectory( folder ) ) {
				mHasGallery = true;
				newFolder = true;
				mGalleryPath = folder;
				mGalleryFiles =
					FileSystem::filesGetInPath( mGalleryPath, true, true, false, [this] {
						return mGalleryLoaderShouldAbort.load();
					} );
				std::erase_if( mGalleryFiles, []( const std::string& filename ) {
					return !Image::isImageExtension( filename );
				} );
			} else if ( sameFolder ) {
				mHasGallery = true;
			}

			if ( sameFolder || newFolder ) {
				auto fileName( FileSystem::fileNameFromPath( path ) );
				auto it = std::find( mGalleryFiles.begin(), mGalleryFiles.end(), fileName );
				if ( it != mGalleryFiles.end() ) {
					mGalleryImageIndex = std::distance( mGalleryFiles.begin(), it );
				} else {
					mGalleryImageIndex = 0;
				}
			}
		} else {
			Lock l( mGalleryMutex );
			mGalleryPath = folder;
			mGalleryFiles = { FileSystem::fileNameFromPath( path ) };
			mGalleryImageIndex = 0;
		}

		Image::Format format =
			isContents ? Image::getFormat( reinterpret_cast<const unsigned char*>( path.c_str() ),
										   path.size() )
					   : Image::getFormat( path );

		if ( format == Image::Format::Unknown )
			return;

		Drawable* image = nullptr;

		if ( mClosing )
			return;

		if ( format != Image::Format::GIF ) {
			image = isContents
						? TextureFactory::instance()->loadFromMemory(
							  reinterpret_cast<const unsigned char*>( path.c_str() ), path.size() )
						: TextureFactory::instance()->loadFromFile( path );
		} else {
			IOStream* stream = isContents
								   ? (IOStream*)new IOStreamMemory( path.c_str(), path.size() )
								   : (IOStream*)new IOStreamFile( path );
			Sprite* sprite = Sprite::fromGif( *stream );
			sprite->setAutoAnimate( false );
			image = sprite;
			delete stream;
		}

		if ( mClosing )
			return;

		if ( image ) {

			if ( isContents )
				mCurFileSize = path.size();
			else
				mCurFileSize = FileSystem::fileSize( path );

			mCurFileType = format;

			runOnMainThread( [this, image] {
				mImage->setDrawable( image, true );
				updateTextDisplay();
				auto s( image->getPixelsSize() );
				auto scale( s.x > mSize.x || s.y > mSize.y
								? eemin( mSize.x / s.getWidth(), mSize.y / s.getHeight() )
								: 1.f );
				mImage->setPixelsSize( eefloor( image->getPixelsSize().x * scale ),
									   eefloor( image->getPixelsSize().y * scale ) );
				resetImageView();
				sendCommonEvent( Event::OnResourceLoaded );
			} );
		}

		runOnMainThread( [this] { mLoader->setVisible( false ); } );
	} );
}

void UIImageViewer::onSizeChange() {
	mLoader->center();
}

void UIImageViewer::reset() {
	mImage->setDrawable( nullptr )->setVisible( false );
}

Uint32 UIImageViewer::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::MouseDown ) {
		if ( ( msg->getFlags() & EE_BUTTON_LMASK ) && getEventDispatcher() &&
			 getEventDispatcher()->getNodeDragging() == nullptr ) {
			mImage->setFocus();
			mImage->startDragging( getEventDispatcher()->getMousePosf() );
			return 1;
		}
		if ( msg->getFlags() & EE_BUTTON_RMASK ) {
			Line2f line( getEventDispatcher()->getMousePosf(),
						 mImage->getScreenPos() + mImage->getPixelsSize() * 0.5f );
			mImage->setRotation( line.getAngle() );
			return 1;
		}
		if ( msg->getFlags() & EE_BUTTON_MMASK ) {
			if ( !mMouseMiddlePressing ) {
				mMouseMiddlePressing = true;
				mInitialScale = mImage->getScale().x;
				mMouseMiddleStartClick = getEventDispatcher()->getMousePosf();
				Vector2f localPos( mImage->convertToNodeSpace( mMouseMiddleStartClick ) );
				auto size( mImage->getPixelsSize() );
				localPos.x = eeclamp( localPos.x, 0.f, size.x );
				localPos.y = eeclamp( localPos.y, 0.f, size.y );
				mImage->setScaleOriginPointPixels( { localPos.x, localPos.y } );
			}
		}
	} else if ( msg->getMsg() == NodeMessage::MouseMove ) {
		if ( msg->getFlags() & EE_BUTTON_MMASK ) {
			mMouseMiddlePressing = true;
			Vector2f v1( mMouseMiddleStartClick );
			Vector2f v2( getEventDispatcher()->getMousePosf() );
			Line2f l1( v1, v2 );
			Float dist = eeabs( v1.y - v2.y ) * 0.01f;
			Float ang = l1.getAngle();
			if ( dist ) {
				if ( ang >= 0.0f && ang <= 180.0f ) {
					Float scale = eemax( mInitialScale - dist, mMinScale );
					mImage->setScale( scale );
				} else {
					Float scale = eemin( mInitialScale + dist, mMaxScale );
					mImage->setScale( scale );
				}
			}
		}
	} else if ( msg->getMsg() == NodeMessage::MouseUp ) {
		if ( msg->getFlags() & EE_BUTTON_MMASK ) {
			mMouseMiddlePressing = false;
		}

		if ( getInput()->isKeyModPressed() ) {
			Vector2f localPos( mImage->convertToNodeSpace( getEventDispatcher()->getMousePosf() ) );
			auto size( mImage->getPixelsSize() );
			localPos.x = eeclamp( localPos.x, 0.f, size.x );
			localPos.y = eeclamp( localPos.y, 0.f, size.y );
			OriginPoint origin( { localPos.x, localPos.y } );

			if ( msg->getFlags() & EE_BUTTON_WUMASK ) {
				mImage->setScale( eemin( mMaxScale, mImage->getScale().x + mMinScale ), origin );
			} else if ( msg->getFlags() & EE_BUTTON_WDMASK ) {
				mImage->setScale( eemax( mMinScale, mImage->getScale().x - mMinScale ), origin );
			}
		} else {
			if ( msg->getFlags() & EE_BUTTON_WUMASK ) {
				if ( mGalleryImageIndex - 1 >= 0 ) {
					loadImageAsync( mGalleryPath + mGalleryFiles[--mGalleryImageIndex] );
				}
			} else if ( msg->getFlags() & EE_BUTTON_WDMASK ) {
				if ( mGalleryImageIndex + 1 < (Int64)mGalleryFiles.size() ) {
					loadImageAsync( mGalleryPath + mGalleryFiles[++mGalleryImageIndex] );
				}
			}
		}
	}
	return 0;
}

void UIImageViewer::resetImageView() {
	mImage->setScale( 1 );
	mImage->setPixelsPosition( Vector2f::Zero );
	mImage->setRotation( 0 );
	mImage->setVisible( true );
	mImage->center();
}

Uint32 UIImageViewer::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_KP_MINUS ) {
		mImage->setScale(
			eemax( mMinScale, mImage->getScale().x -
								  (Float)getUISceneNode()->getElapsed().asSeconds() * 8.f ) );
		return 1;
	} else if ( event.getKeyCode() == KEY_KP_PLUS ) {
		mImage->setScale(
			eemin( mMaxScale, mImage->getScale().x +
								  (Float)getUISceneNode()->getElapsed().asSeconds() * 8.f ) );
		return 1;
	} else if ( event.getKeyCode() == KEY_T ) {
		resetImageView();
	}
	return UIWidget::onKeyDown( event );
}

const std::string& UIImageViewer::getGalleryPath() const {
	return mGalleryPath;
}

const std::string& UIImageViewer::getImageName() const {
	static std::string EMPTY = "";
	return mGalleryImageIndex >= 0 && mGalleryImageIndex < (Int64)mGalleryFiles.size()
			   ? mGalleryFiles[mGalleryImageIndex]
			   : EMPTY;
}

std::string UIImageViewer::getImagePath() const {
	return getImageName().empty() ? getImageName() : ( mGalleryPath + getImageName() );
}

void UIImageViewer::setDisplayOptions( Uint32 opt ) {
	if ( opt != mDisplayOptions ) {
		mDisplayOptions = opt;
		updateTextDisplay();
	}
}

Uint32 UIImageViewer::getDisplayOptions() const {
	return mDisplayOptions;
}

void UIImageViewer::updateTextDisplay() {
	bool hasValidPath = mHasGallery && !mGalleryPath.empty() && mGalleryImageIndex >= 0 &&
						mGalleryImageIndex < (Int64)mGalleryFiles.size();

	String str;

	if ( hasValidPath && ( mDisplayOptions & DisplayOptions::DisplayGalleryPosition ) ) {
		str += String::format( "%d / %d", mGalleryImageIndex + 1, mGalleryFiles.size() );
		str += "\n";
	}

	if ( hasValidPath && ( mDisplayOptions & DisplayOptions::DisplayName ) ) {
		str += i18n( "filename_colon", "File name:" );
		str += " ";
		str += mGalleryFiles[mGalleryImageIndex];
		str += "\n";
	}

	if ( hasValidPath && ( mDisplayOptions & DisplayOptions::DisplayPath ) ) {
		str += i18n( "filepath_colon", "File path:" );
		str += " ";
		str += mGalleryPath + mGalleryFiles[mGalleryImageIndex];
		str += "\n";
	}

	if ( ( mDisplayOptions & DisplayOptions::DisplayDimensions ) && mImage->getDrawable() ) {
		str += i18n( "dimensions_colon", "Dimensions:" );
		str += " ";
		str += String::format( "%d x %d", (int)mImage->getDrawable()->getPixelsSize().x,
							   (int)mImage->getDrawable()->getPixelsSize().y );
		str += "\n";
	}

	if ( hasValidPath && ( mDisplayOptions & DisplayOptions::DisplaySize ) ) {
		str += i18n( "filesize_colon", "File size:" );
		str += " ";
		str += FileSystem::sizeToString( mCurFileSize );
		str += "\n";
	}

	if ( hasValidPath && ( mDisplayOptions & DisplayOptions::DisplayType ) ) {
		str += i18n( "filetype_colon", "File type:" );
		str += " ";
		str += Image::formatToString( mCurFileType );
		str += "\n";
	}

	mTextView->setText( str );
}

bool UIImageViewer::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::DisplayOptions: {
			setDisplayOptions( displayOptionsFromString( attribute.asString() ) );
			break;
		}
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::string UIImageViewer::getPropertyString( const PropertyDefinition* propertyDef,
											  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::DisplayOptions:
			return displayOptionsToString( mDisplayOptions );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIImageViewer::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::DisplayOptions };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

} // namespace EE::UI::Tools