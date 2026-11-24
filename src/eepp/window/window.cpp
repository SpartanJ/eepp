#include "pixeldensitypolicy.hpp"
#include "windowinitialization.hpp"
#include <SOIL2/src/SOIL2/SOIL2.h>
#include <cmath>
#include <cstdlib>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/version.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/displaymanager.hpp>
#include <eepp/window/draganddrop/dragmanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/runtime.hpp>
#include <eepp/window/terminal/kittyframepresenter.hpp>
#include <eepp/window/window.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/window/platform/physicaldisplaymetrics.hpp>
#elif defined( EE_X11_PLATFORM )
#include "platform/x11scalesettings.hpp"
#endif

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

namespace EE { namespace Window {

bool Window::canUseNativeDrag() const {
	return isOpen() && !Runtime::isOffscreen() && Runtime::mode() != RuntimeMode::Terminal;
}

NativeWindowContext Window::getNativeDragContext() {
	NativeWindowContext context;
	context.owner = this;
	return context;
}

bool Window::isNativeFileTargetSupported() const {
	return false;
}

bool Window::isNativeDataTargetSupported() const {
	return false;
}

void Window::initializeNativeDragEvents() {}

void Window::shutdownNativeDragEvents() {}

Private::NativeDragPreview* Window::createNativeDragPreview( const DragOffer&, bool ) {
	return nullptr;
}

Window::FrameData::FrameData() : FrameElapsed( NULL ), ElapsedTime() {}

Window::FrameData::~FrameData() {
	eeSAFE_DELETE( FrameElapsed );
}

Window::Window( WindowSettings Settings, ContextSettings Context, Clipboard* Clipboard,
				Input* Input, CursorManager* CursorManager ) :
	mClipboard( Clipboard ),
	mInput( Input ),
	mCursorManager( CursorManager ),
	mCurrentView( NULL ),
	mNumCallBacks( 0 ),
	mIME( this ) {
	mWindow.WindowConfig = Settings;
	mWindow.ContextConfig = Context;
	// Resolve the policy without querying native handles before creation.
	mPixelDensityMode = resolvePixelDensityMode( Settings.DensityMode );
	mPixelDensityOverride = std::isfinite( Settings.PixelDensity ) && Settings.PixelDensity > 0.f
								? Settings.PixelDensity
								: PixelDensity::getEnvironmentPixelDensity();
	setFrameRateLimit( Context.FrameRateLimit );
}

void Window::initializePixelDensity( Float pixelDensityOverride ) {
	// Native creation copies WindowSettings again; resolve the reference after that copy.
	mWindow.WindowConfig.PixelDensityReferencePPI =
		resolvePixelDensityReferencePPI( mWindow.WindowConfig.PixelDensityReferencePPI );
	if ( std::isfinite( pixelDensityOverride ) && pixelDensityOverride > 0.f )
		mPixelDensityOverride = pixelDensityOverride;
	refreshPixelDensity();
}

void Window::setPixelDensityOverride( Float density ) {
	mPixelDensityOverride = std::isfinite( density ) && density > 0.f
								? density
								: PixelDensity::getEnvironmentPixelDensity();
	refreshPixelDensity();
}

bool Window::refreshPixelDensity() {
	const Float oldDensity = mPixelDensity;
	const Float displayScale = getDisplayScale();
	const int displayIndex = getCurrentDisplayIndex();
	auto& overrides = mWindow.WindowConfig.DensityOverrides;
	Float densityOverride = mPixelDensityOverride;
	if ( densityOverride <= 0.f && !overrides.empty() ) {
		const auto override = overrides.find( getDisplayDensityKey( displayIndex ) );
		if ( override != overrides.end() && override->second >= 1.f && override->second <= 6.f )
			densityOverride = override->second;
	}
	mPixelDensity = densityOverride > 0.f
						? densityOverride
						: resolveAutomaticPixelDensity( displayScale, displayIndex );
	if ( mPixelDensity != oldDensity || mDisplayScale != displayScale ||
		 mDensityDisplayIndex != displayIndex )
		++mDisplayMetricsVersion;
	mDisplayScale = displayScale;
	mDensityDisplayIndex = displayIndex;
	return mPixelDensity != oldDensity;
}

Float Window::getAutomaticPixelDensity( int displayIndex ) const {
	Float displayScale = getDisplayScale();
	if ( displayIndex == getCurrentDisplayIndex() )
		return resolveAutomaticPixelDensity( displayScale, displayIndex );

	auto* display = Engine::instance()->getDisplayManager()->getDisplayIndex( displayIndex );
	if ( !display )
		return 1.f;
#if EE_PLATFORM == EE_PLATFORM_MACOS
	displayScale = Private::getDisplayBackingScale( display->getBounds() );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	const Float monitorScale = Private::PhysicalDisplayMetrics::getScale( display->getBounds() );
	if ( monitorScale > 0.f )
		displayScale = monitorScale;
#endif
	return resolveAutomaticPixelDensity( displayScale, displayIndex, false );
}

Float Window::resolveAutomaticPixelDensity( Float displayScale, int displayIndex,
											bool currentDisplay ) const {
// Emscripten uses the rounded browser scale for Best Fit and Display Scale.
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( mPixelDensityMode == PixelDensityMode::BestFit )
		return resolveBestFitPixelDensity( displayScale, displayIndex, currentDisplay );
#endif

	Float density = displayScale;
	if ( mPixelDensityMode == PixelDensityMode::PhysicalPPI ) {
		const auto getDisplay = [displayIndex] {
			return Engine::instance()->getDisplayManager()->getDisplayIndex( displayIndex );
		};
		auto* display = currentDisplay ? nullptr : getDisplay();
		Float dpi = currentDisplay ? getPhysicalDisplayDPI()
								   : ( display ? display->getPhysicalDPI() : 0.f );
#if EE_PLATFORM == EE_PLATFORM_MACOS
		if ( dpi > 0.f ) {
			if ( !display )
				display = getDisplay();
			if ( display ) {
				const Float backingScale = currentDisplay ? getScale() : displayScale;
				const Float backingDPI =
					Private::getBackingDisplayDPI( display->getBounds(), backingScale );
				if ( backingDPI > 0.f )
					dpi = backingDPI;
			}
		}
#endif
		if ( std::isfinite( dpi ) && dpi > 0.f )
			density = eemax( 1.f, dpi / getPixelDensityReferencePPI() );
	}
	if ( !std::isfinite( density ) || density <= 0.f )
		density = 1.f;
	const char* rounding = std::getenv( "EEPP_PIXEL_DENSITY_ROUNDING" );
	return rounding && std::string_view( rounding ) == "0"
			   ? density
			   : Private::roundAutomaticPixelDensity( density );
}

Float Window::resolveBestFitPixelDensity( Float displayScale, int displayIndex,
										  bool currentDisplay ) const {
	if ( !std::isfinite( displayScale ) || displayScale <= 0.f )
		displayScale = 1.f;
	if ( Runtime::isOffscreen() || Runtime::mode() == RuntimeMode::Terminal )
		return displayScale;
#if EE_PLATFORM == EE_PLATFORM_MACOS || defined( EE_X11_PLATFORM )
	Float logicalScale = 0.f;
#ifdef EE_X11_PLATFORM
	if ( !usesX11DisplayScaling() )
		return displayScale;
	if ( mX11BestFitScale.systemScale != displayScale ) {
		const auto scales = Private::getX11ScaleSettings();
		mX11BestFitScale = { displayScale, scales.windowScale, scales.logicalScale };
	}
	if ( mX11BestFitScale.windowScale > 1.f && mX11BestFitScale.windowScale <= 6.f )
		return mX11BestFitScale.windowScale;
	logicalScale = mX11BestFitScale.logicalScale > 0.f && mX11BestFitScale.logicalScale <= 6.f
					   ? mX11BestFitScale.logicalScale
					   : displayScale;
#endif
	const char* roundingOption = std::getenv( "EEPP_PIXEL_DENSITY_ROUNDING" );
	const bool rounding = !roundingOption || std::string_view( roundingOption ) != "0";
	auto* display = Engine::instance()->getDisplayManager()->getDisplayIndex( displayIndex );
	if ( display ) {
		Float diagonal = 0.f;
		Float ppi = 0.f;
#if EE_PLATFORM == EE_PLATFORM_MACOS
		const Float backingScale = currentDisplay ? getScale() : displayScale;
		ppi = Private::getBackingDisplayDPI( display->getBounds(), backingScale, &diagonal );
#else
		ppi = currentDisplay ? getPhysicalDisplayDPI() : display->getPhysicalDPI();
		if ( ppi > 0.f ) {
			const auto mode = display->getCurrentMode();
			diagonal =
				std::hypot( static_cast<Float>( mode.Width ), static_cast<Float>( mode.Height ) ) /
				ppi;
		}
#endif
		const Float estimate =
			Private::estimateBestFitPixelDensity( ppi, diagonal, logicalScale, rounding );
		if ( estimate > 0.f )
			return estimate;
	}
	return rounding ? Private::roundAutomaticPixelDensity( displayScale ) : displayScale;
#else
	(void)displayIndex;
	(void)currentDisplay;
	return displayScale;
#endif
}

#ifdef EE_X11_PLATFORM
bool Window::usesX11DisplayScaling() const {
	return false;
}
#endif

Float Window::getPhysicalDisplayDPI() const {
	if ( Runtime::isOffscreen() || Runtime::mode() == RuntimeMode::Terminal )
		return 0.f;
	auto* display =
		Engine::instance()->getDisplayManager()->getDisplayIndex( getCurrentDisplayIndex() );
	return display ? display->getPhysicalDPI() : 0.f;
}

std::string Window::getDisplayDensityKey() const {
	return getDisplayDensityKey( getCurrentDisplayIndex() );
}

const std::string& Window::getDisplayDensityKey( int displayIndex ) const {
	if ( mDisplayDensityKeyIndex != displayIndex ) {
		mDisplayDensityKeyIndex = displayIndex;
		mDisplayDensityKey.clear();
		if ( displayIndex >= 0 && !Runtime::isOffscreen() &&
			 Runtime::mode() != RuntimeMode::Terminal ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
			mDisplayDensityKey =
				Private::PhysicalDisplayMetrics::getPersistentId( getWindowHandler() );
#else
			auto* display =
				Engine::instance()->getDisplayManager()->getDisplayIndex( displayIndex );
			if ( display )
				mDisplayDensityKey = display->getPersistentId();
#endif
		}
	}
	return mDisplayDensityKey;
}

bool Window::setDisplayDensityOverride( const std::string& key, Float density ) {
	if ( key.empty() || ( density != 0.f && !( density >= 1.f && density <= 6.f ) ) )
		return false;
	auto& overrides = mWindow.WindowConfig.DensityOverrides;
	if ( density == 0.f )
		overrides.erase( key );
	else
		overrides[key] = density;
	refreshPixelDensity();
	return true;
}

const DisplayDensityOverrides& Window::getDisplayDensityOverrides() const {
	return mWindow.WindowConfig.DensityOverrides;
}

namespace {

constexpr auto DisplayScaleOverridesSection = "display_scale_overrides";

} // namespace

DisplayDensityOverrides Window::loadDisplayDensityOverrides( System::IniFile& ini ) {
	DisplayDensityOverrides overrides;
	const auto count = ini.getNumValues( DisplayScaleOverridesSection );
	for ( unsigned i = 0; i < count; ++i ) {
		auto key = ini.getValueName( DisplayScaleOverridesSection, i );
		const double value = ini.getValueF( DisplayScaleOverridesSection, key );
		if ( !key.empty() && value >= 1. && value <= 6. )
			overrides.emplace( std::move( key ), static_cast<Float>( value ) );
	}
	return overrides;
}

void Window::saveDisplayDensityOverrides( System::IniFile& ini,
										  const DisplayDensityOverrides& overrides ) {
	ini.deleteKey( DisplayScaleOverridesSection );
	for ( const auto& [key, density] : overrides )
		ini.setValueF( DisplayScaleOverridesSection, key, density );
}

PixelDensityMode Window::getPixelDensityMode() const {
	return mPixelDensityMode;
}

void Window::setPixelDensityMode( PixelDensityMode mode ) {
	mPixelDensityMode = resolvePixelDensityMode( mode );
	mWindow.WindowConfig.DensityMode = mode;
#ifdef EE_X11_PLATFORM
	mX11BestFitScale = {};
#endif
	refreshPixelDensity();
}

Float Window::getPixelDensityReferencePPI() const {
	return mWindow.WindowConfig.PixelDensityReferencePPI;
}

void Window::setPixelDensityReferencePPI( Float reference ) {
	mWindow.WindowConfig.PixelDensityReferencePPI = resolvePixelDensityReferencePPI( reference );
	refreshPixelDensity();
}

Float Window::resolvePixelDensityReferencePPI( Float reference ) {
	if ( std::isfinite( reference ) && reference > 0.f )
		return reference;
	const char* value = std::getenv( "EEPP_PIXEL_DENSITY_REFERENCE_PPI" );
	if ( value && *value ) {
		char* end = nullptr;
		reference = std::strtof( value, &end );
		if ( end != value && *end == '\0' && std::isfinite( reference ) && reference > 0.f )
			return reference;
	}
	return 96.f;
}

PixelDensityMode Window::pixelDensityModeFromString( std::string_view mode ) {
	if ( mode == "best-fit" )
		return PixelDensityMode::BestFit;
	return mode == "physical-ppi"	 ? PixelDensityMode::PhysicalPPI
		   : mode == "display-scale" ? PixelDensityMode::DisplayScale
									 : PixelDensityMode::Default;
}

const char* Window::pixelDensityModeToString( PixelDensityMode mode ) {
	if ( mode == PixelDensityMode::BestFit )
		return "best-fit";
	return mode == PixelDensityMode::PhysicalPPI	? "physical-ppi"
		   : mode == PixelDensityMode::DisplayScale ? "display-scale"
													: "default";
}

PixelDensityMode Window::getNewProfilePixelDensityMode() {
	const char* value = std::getenv( "EEPP_PIXEL_DENSITY_MODE" );
	return value && pixelDensityModeFromString( value ) != PixelDensityMode::Default
			   ? PixelDensityMode::Default
			   : PixelDensityMode::BestFit;
}

PixelDensityMode Window::resolvePixelDensityMode( PixelDensityMode mode ) {
	if ( mode != PixelDensityMode::Default )
		return mode;
	const char* value = std::getenv( "EEPP_PIXEL_DENSITY_MODE" );
	const auto configured = value ? pixelDensityModeFromString( value ) : PixelDensityMode::Default;
	return configured == PixelDensityMode::Default ? PixelDensityMode::BestFit : configured;
}

Uint32 Window::getDisplayMetricsVersion() const {
	return mDisplayMetricsVersion;
}

void Window::updateDisplayMetrics() {
	if ( !mDisplayMetricsDirty || !isOpen() )
		return;
	mDisplayMetricsDirty = false;
	auto context = Engine::instance()->makeWindowCurrent( this );
	const auto version = mDisplayMetricsVersion;
	if ( mDisplayTopologyDirty ) {
		mDisplayDensityKeyIndex = -1;
		mDisplayDensityKey.clear();
#ifdef EE_X11_PLATFORM
		mX11BestFitScale = {};
#endif
	}
	refreshPixelDensity();
	if ( mDisplayTopologyDirty && mDisplayMetricsVersion == version )
		++mDisplayMetricsVersion;
	mDisplayTopologyDirty = false;
	refreshDisplayMetrics();
}

void Window::refreshDisplayMetrics() {}

const Float& Window::getPixelDensity() const {
	return mPixelDensity;
}

Float Window::getPixelDensityOverride() const {
	return mPixelDensityOverride;
}

bool Window::hasPixelDensityOverride() const {
	return mPixelDensityOverride > 0.f;
}

Float Window::getDisplayScale() const {
	return 1.f;
}

Window::~Window() {
	System::Private::ScopedRestoreTarget<Window>::invalidate( this );
	PixelDensity::snapshotBindings( &mPixelDensity );
	if ( WindowInitialization::isPending() )
		WindowInitialization::cancel( this );
	Private::DragManager::invalidateWindow( this );
	eeSAFE_DELETE( mClipboard );
	eeSAFE_DELETE( mInput );
	eeSAFE_DELETE( mCursorManager );
}

Sizei Window::getSize() const {
	return Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
}

Sizei Window::getSizeInScreenCoordinates() const {
	Float scale = getScale();
	return Sizei{ static_cast<int>( mWindow.WindowConfig.Width / scale ),
				  static_cast<int>( mWindow.WindowConfig.Height / scale ) };
}

Vector2f Window::getCenter() const {
	return Sizef( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) * 0.5f;
}

const Uint32& Window::getWidth() const {
	return mWindow.WindowConfig.Width;
}

const Uint32& Window::getHeight() const {
	return mWindow.WindowConfig.Height;
}

const Sizei& Window::getDesktopResolution() const {
	return mWindow.DesktopResolution;
}

void Window::setSize( Uint32 Width, Uint32 Height ) {
	setSize( Width, Height, isWindowed() );
}

void Window::setMinimumSize( Uint32, Uint32 ) {}

bool Window::setModalFor( Window* ) {
	return false;
}

bool Window::isWindowed() const {
	return 0 != !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen );
}

bool Window::isResizeable() const {
	return 0 != ( mWindow.WindowConfig.Style & WindowStyle::Resize );
}

void Window::set2DProjection( const Uint32& Width, const Uint32& Height ) {
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadIdentity();

	GLi->ortho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

void Window::setProjection( const Transform& transform ) {
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( transform.getMatrix() );

	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

Vector2f Window::mapPixelToCoords( const Vector2i& point ) const {
	return mapPixelToCoords( point, getView() );
}

Vector2f Window::mapPixelToCoords( const Vector2i& point, const View& view ) const {
	// First, convert from viewport coordinates to homogeneous coordinates
	Vector2f normalized;
	Rect viewport = getViewport( view );
	normalized.x = -1.f + 2.f * ( point.x - viewport.Left ) / viewport.Right;
	normalized.y = 1.f - 2.f * ( point.y - viewport.Top ) / viewport.Bottom;

	// Then transform by the inverse of the view matrix
	return view.getInverseTransform().transformPoint( normalized );
}

Vector2i Window::mapCoordsToPixel( const Vector2f& point ) const {
	return mapCoordsToPixel( point, getView() );
}

Vector2i Window::mapCoordsToPixel( const Vector2f& point, const View& view ) const {
	// First, transform the point by the view matrix
	Vector2f normalized = view.getTransform().transformPoint( point );

	// Then convert to viewport coordinates
	Vector2i pixel;
	Rect viewport = getViewport( view );
	pixel.x = static_cast<int>( ( normalized.x + 1.f ) / 2.f * viewport.Right + viewport.Left );
	pixel.y = static_cast<int>( ( -normalized.y + 1.f ) / 2.f * viewport.Bottom + viewport.Top );

	return pixel;
}

void Window::setCloseRequestCallback( const WindowRequestCloseCallback& closeRequestCallback ) {
	mCloseRequestCallback = closeRequestCallback;
}

void Window::setQuitCallback( const WindowQuitCallback& quitCallback ) {
	mQuitCallback = quitCallback;
}

void Window::setViewport( const Int32& x, const Int32& y, const Uint32& Width,
						  const Uint32& Height ) {
	GLi->viewport( x, getHeight() - ( y + Height ), Width, Height );
}

Rect Window::getViewport( const View& view ) const {
	float width = static_cast<float>( getSize().getWidth() );
	float height = static_cast<float>( getSize().getHeight() );
	const Rectf& viewport = view.getViewport();

	return Rect( static_cast<int>( 0.5f + width * viewport.Left ),
				 static_cast<int>( 0.5f + height * viewport.Top ),
				 static_cast<int>( 0.5f + width * viewport.Right ),
				 static_cast<int>( 0.5f + height * viewport.Bottom ) );
}

void Window::setView( const View& view, bool forceRefresh ) {
	const View* viewPtr = &view;

	if ( viewPtr != mCurrentView || viewPtr->isDirty() || forceRefresh ) {
		mCurrentView = viewPtr;

		Rect viewport = getViewport( *mCurrentView );

		setViewport( viewport.Left, viewport.Top, viewport.Right, viewport.Bottom );

		setProjection( viewPtr->getTransform() );
	}
}

const View& Window::getDefaultView() const {
	return mDefaultView;
}

const View& Window::getView() const {
	return *mCurrentView;
}

void Window::createView() {
	mDefaultView.reset( Rectf( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) );
	setView( mDefaultView );
}

void Window::setup2D( const bool& KeepView ) {
	EE::Window::Window* curWin = Engine::instance()->getCurrentWindow();
	if ( curWin != this )
		Engine::instance()->setCurrentWindow( this );

	GLi->pixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	GLi->pixelStorei( GL_PACK_ALIGNMENT, 1 );

	setClearColor( mWindow.ClearColor );

	GLi->lineSmooth();

	GLi->enable( GL_TEXTURE_2D ); // Enable Textures
	GLi->disable( GL_DEPTH_TEST );

	if ( GLv_2 == GLi->version() || GLv_ES1 == GLi->version() ) {
		GLi->disable( GL_LIGHTING );
	}

	if ( !KeepView ) {
		setView( mDefaultView, true );
	}

	BlendMode::setMode( BlendMode::Alpha(), true );

	GLi->configurePointSprite();

	if ( GLv_2 == GLi->version() || GLv_ES1 == GLi->version() ) {
		GLi->enableClientState( GL_VERTEX_ARRAY );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
		GLi->enableClientState( GL_COLOR_ARRAY );
	}

	if ( curWin != this )
		Engine::instance()->setCurrentWindow( curWin );
}

const WindowInfo* Window::getWindowInfo() const {
	return &mWindow;
}

void Window::setClearColor( const RGB& Color ) {
	mWindow.ClearColor = Color;
	GLi->clearColor( static_cast<Float>( mWindow.ClearColor.r ) / 255.0f,
					 static_cast<Float>( mWindow.ClearColor.g ) / 255.0f,
					 static_cast<Float>( mWindow.ClearColor.b ) / 255.0f, 255.0f );
}

RGB Window::getClearColor() const {
	return mWindow.ClearColor;
}

bool Window::takeScreenshot( std::string filepath, const Image::SaveType& format ) {
	GlobalBatchRenderer::instance()->draw();

	bool createNewFile = false;
	std::string file;
	std::string ext;

	if ( filepath.size() ) {
		file = filepath.substr( filepath.find_last_of( "/\\" ) + 1 );
		ext = file.substr( file.find_last_of( "." ) + 1 );
		if ( FileSystem::isDirectory( filepath ) || !ext.size() )
			createNewFile = true;
	} else {
		filepath = Sys::getProcessPath();
		createNewFile = true;
	}

	std::string finalPath;

	if ( createNewFile ) { // Search if file path is given, and if have and extension
		bool find = false;
		int fileNum = 1;
		std::string tmpPath = filepath;

		if ( !FileSystem::isDirectory( filepath ) )
			FileSystem::makeDir( filepath );

		ext = "." + Image::saveTypeToExtension( format );

		while ( !find && fileNum < 10000 ) {
			tmpPath = String::format( "%s%05d%s", filepath.c_str(), fileNum, ext.c_str() );
			fileNum++;

			if ( !FileSystem::fileExists( tmpPath ) )
				find = true;

			if ( fileNum == 10000 && find == false )
				return false;
		}

		finalPath = std::move( tmpPath );
	} else {
		std::string Direc = FileSystem::fileRemoveFileName( filepath );
		if ( !FileSystem::isDirectory( Direc ) )
			FileSystem::makeDir( Direc );
		finalPath = filepath;
	}

	return getFrontBufferImage().saveToFile( finalPath, format );
}

Image Window::getFrontBufferImage() {
	GlobalBatchRenderer::instance()->draw();
	Image fb( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, 3 );

	Uint8* pixelData = fb.getPixels();
	if ( !GLi->readPixels( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height,
						   Graphics::Renderer::PixelFormat::RGB24, pixelData,
						   static_cast<size_t>( mWindow.WindowConfig.Width ) * 3 ) )
		return {};

	for ( unsigned int j = 0; j * 2 < fb.getHeight(); ++j ) {
		int index1 = j * fb.getWidth() * fb.getChannels();
		int index2 = ( fb.getHeight() - 1 - j ) * fb.getWidth() * fb.getChannels();
		for ( int i = fb.getWidth() * fb.getChannels(); i > 0; --i ) {
			Uint8 temp = pixelData[index1];
			pixelData[index1] = pixelData[index2];
			pixelData[index2] = temp;
			++index1;
			++index2;
		}
	}

	return fb;
}

bool Window::readFrameBuffer( FrameReadback& readback ) {
	const Uint32 channels = readback.format == FramePixelFormat::RGB24 ? 3 : 4;
	const size_t rowBytes = static_cast<size_t>( readback.size.getWidth() ) * channels;
	if ( nullptr == readback.pixels || readback.size.getWidth() <= 0 ||
		 readback.size.getHeight() <= 0 || readback.stride < rowBytes || readback.position.x < 0 ||
		 readback.position.y < 0 ||
		 static_cast<Uint32>( readback.position.x + readback.size.getWidth() ) > getWidth() ||
		 static_cast<Uint32>( readback.position.y + readback.size.getHeight() ) > getHeight() )
		return false;
	const auto format = readback.format == FramePixelFormat::RGB24
							? Graphics::Renderer::PixelFormat::RGB24
							: Graphics::Renderer::PixelFormat::RGBA32;
	if ( mLogicalFrameBuffer )
		mLogicalFrameBuffer->bindAsReadTarget();
	if ( !GLi->readPixels( readback.position.x, readback.position.y, readback.size.getWidth(),
						   readback.size.getHeight(), format, readback.pixels, readback.stride ) )
		return false;
	readback.origin = FrameOrigin::BottomLeft;
	return true;
}

bool Window::initializeRuntimeRenderTarget() {
	if ( !Runtime::isOffscreen() )
		return true;

	if ( mLogicalFrameBuffer )
		return true;

	// The first window is still being constructed. Texture-backed renderers need it current
	// in Engine as well as SDL before their framebuffer attachment can be uploaded.
	auto context = Engine::instance()->makeWindowCurrent( this );
	mLogicalFrameBuffer =
		Graphics::FrameBuffer::New( getWidth(), getHeight(), true, false, true, 4, this );

	if ( !mLogicalFrameBuffer || !mLogicalFrameBuffer->created() ) {
		Log::error( "Unable to create the %s runtime logical framebuffer", Runtime::modeName() );
		mLogicalFrameBuffer.reset();
		return false;
	}

	mLogicalFrameBuffer->bind();

	setup2D( false );

	if ( Runtime::mode() == RuntimeMode::Terminal ) {
		mFramePresenter = std::make_unique<KittyFramePresenter>();
		if ( !mFramePresenter->initialize( *this ) )
			return false;
	}

	return true;
}

void Window::resizeRuntimeRenderTarget( Uint32 width, Uint32 height ) {
	if ( mLogicalFrameBuffer ) {
		mLogicalFrameBuffer->resize( width, height );
		mLogicalFrameBuffer->bind();
	}
	if ( mFramePresenter )
		mFramePresenter->resized( *this, Sizei( width, height ) );
}

void Window::shutdownRuntimeRenderTarget() {
	if ( mFramePresenter )
		mFramePresenter->shutdown( *this );
	mFramePresenter.reset();
	mLogicalFrameBuffer.reset();
}

bool Window::isRunning() const {
	return mWindow.Created;
}

bool Window::isOpen() const {
	return mWindow.Created;
}

void Window::close() {
	Private::DragManager::invalidateWindow( this );
	mWindow.Created = false;
}

void Window::setDeferNativeResourceDestructionOnClose( bool defer ) {
	mDeferNativeResourceDestructionOnClose = defer;
}

bool Window::getDeferNativeResourceDestructionOnClose() const {
	return mDeferNativeResourceDestructionOnClose;
}

void Window::setFrameRateLimit( Uint32 FrameRateLimit ) {
	if ( FrameRateLimit == ContextSettings::FrameRateLimitScreenRefreshRate ) {
		Display* currentDisplay = nullptr;
		if ( Engine::existsSingleton() && Engine::instance()->getDisplayManager() ) {
			currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(
				getCurrentDisplayIndex() );
		}
		if ( currentDisplay ) {
			FrameRateLimit = currentDisplay->getRefreshRate();
		} else {
			// Fallback to 60 FPS if display info not available
			FrameRateLimit = 60;
		}
	}
	mFrameData.FPS.Limit = (Float)FrameRateLimit;
}

Uint32 Window::getFrameRateLimit() const {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 Window::getFPS() const {
	return mFrameData.FPS.Current;
}

const Time& Window::getElapsed() const {
	return mFrameData.ElapsedTime;
}

void Window::updateElapsedTime() {
	if ( NULL == mFrameData.FrameElapsed ) {
		mFrameData.FrameElapsed = eeNew( Clock, () );
	}

	mFrameData.ElapsedTime = mFrameData.FrameElapsed->getElapsedTimeAndReset();
}

const Time& Window::getSleepTimePerSecond() const {
	return mFrameData.FPS.CurSleepTime;
}

const Time& Window::getRenderTimePerSecond() const {
	return mFrameData.FPS.CurRenderTime;
}

const Sizei& Window::getLastWindowedSize() const {
	return mLastWindowedSize;
}

Sizei Window::getLastWindowedSizeInScreenCoordinates() const {
	Float scale = getScale();
	return Sizei{ static_cast<int>( mLastWindowedSize.getWidth() / scale ),
				  static_cast<int>( mLastWindowedSize.getHeight() / scale ) };
}

bool Window::showMessageBox( const MessageBoxType&, const std::string&, const std::string& ) {
	return false;
}

InputMethod& Window::getIME() {
	return mIME;
}

const InputMethod& Window::getIME() const {
	return mIME;
}

void Window::calculateFps() {
	if ( mFrameData.FPS.LastCheck.getElapsedTime().asSeconds() >= 1.f ) {
		mFrameData.FPS.Current = mFrameData.FPS.Count;
		mFrameData.FPS.Count = 0;
		mFrameData.FPS.CurSleepTime = mFrameData.FPS.SleepTime;
		mFrameData.FPS.SleepTime = Time::Zero;
		mFrameData.FPS.CurRenderTime = mFrameData.FPS.RenderTime;
		mFrameData.FPS.RenderTime = Time::Zero;
		mFrameData.FPS.LastCheck.restart();
	}

	mFrameData.FPS.Count++;
}

void Window::limitFps() {
	if ( mFrameData.FPS.Limit > 0 ) {
		Time frameTime( Milliseconds( 1000.0 / mFrameData.FPS.Limit ) );
		Time remainingTime = frameTime - mFrameData.FPS.FrameTime;

		if ( frameTime - mFrameData.ElapsedTime > Time::Zero ) {
			remainingTime += mFrameData.FPS.Error;
		}

		mFrameData.FPS.Error = Time::Zero;

		if ( remainingTime > Time::Zero ) {
			Clock checkElapsed;

			Sys::sleep( remainingTime );

			Time elapsed = checkElapsed.getElapsedTimeAndReset();
			Time timeDiff = elapsed - remainingTime;

			mFrameData.FPS.SleepTime += elapsed;

			if ( timeDiff > Time::Zero ) {
				mFrameData.FPS.Error += timeDiff;
			}
		}
	}
}

void Window::clear() {
	GLi->clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void Window::display( bool clear ) {
	display( clear, true );
}

void Window::display( bool clear, bool limitFrameRate ) {
	GlobalBatchRenderer::instance()->draw();
	if ( mFramePresenter )
		mFramePresenter->present( *this );
	if ( TextureFactory* textureFactory = TextureFactory::existsSingleton() )
		textureFactory->collectReleasedTextures();
	mIME.updateLocation();

	swapBuffers();
	if ( unlikely( WindowInitialization::isPending() ) )
		WindowInitialization::presented( this );

	if ( mCurrentView->isDirty() )
		setView( *mCurrentView );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( clear )
		this->clear();
#endif

	mFrameData.FPS.FrameTime = mFrameData.FPS.RenderClock.getElapsedTime();
	mFrameData.FPS.RenderTime += mFrameData.FPS.FrameTime;

	updateElapsedTime();

	if ( limitFrameRate )
		limitFps();

	calculateFps();

	mFrameData.FPS.RenderClock.restart();
}

Clipboard* Window::getClipboard() const {
	return mClipboard;
}

Input* Window::getInput() const {
	return mInput;
}

CursorManager* Window::getCursorManager() const {
	return mCursorManager;
}

Uint32 Window::pushResizeCallback( const WindowResizeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[mNumCallBacks] = cb;
	return mNumCallBacks;
}

void Window::popResizeCallback( const Uint32& CallbackId ) {
	mCallbacks[CallbackId] = 0;
	mCallbacks.erase( mCallbacks.find( CallbackId ) );
}

void Window::startOnScreenKeyboard() {}

void Window::stopOnScreenKeyboard() {}

bool Window::isOnScreenKeyboardActive() const {
	return false;
}

void Window::sendVideoResizeCb() {
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin();
		  i != mCallbacks.end(); ++i ) {
		i->second( this );
	}
}

void Window::logSuccessfulInit( const std::string& BackendName ) {
	const char* presenter =
		Runtime::mode() == RuntimeMode::Terminal ? "Kitty graphics protocol" : "None";
	std::string msg( String::format(
		"Engine Initialized Successfully.\n\tVersion: %s (codename: \"%s\")\n\tBuild time: "
		"%s\n\tPlatform: %s\n\tOS: %s\n\tArch: %s\n\tCPU Cores: %d\n\tProcess Path: %s\n\tCurrent "
		"Working Directory: %s\n\tHome Directory: %s\n\tDisk Free Space: %s\n\tRuntime Mode: "
		"%s\n\tHeadless: %s\n\tFrame Presenter: %s\n\tWindow/Input Backend: %s\n\tGL Backend: "
		"%s\n\tGL Vendor: %s\n\tGL Renderer: %s\n\tGL Version: "
		"%s\n\tGL Shading Language Version: %s\n\tResolution: %dx%d\n\tWindow scale: %.2f",
		Version::getVersionName(), Version::getCodename(), Version::getBuildTime(),
		Sys::getPlatform(), Sys::getOSName( true ), Sys::getOSArchitecture(), Sys::getCPUCount(),
		Sys::getProcessPath(), FileSystem::getCurrentWorkingDirectory(), Sys::getUserDirectory(),
		FileSystem::sizeToString( FileSystem::getDiskFreeSpace( Sys::getProcessPath() ) ),
		Runtime::modeName(), Runtime::isOffscreen() ? "Yes" : "No", presenter, BackendName,
		GLi->versionStr(), GLi->getVendor(), GLi->getRenderer(), GLi->getVersion(),
		GLi->getShadingLanguageVersion(), getWidth(), getHeight(), getScale() ) );

#ifndef EE_SILENT
	Log::info( msg );
#else
	Log::instance()->writel( msg );
#endif
}

void Window::logFailureInit( const std::string& ClassName, const std::string& BackendName ) {
	Log::error( "Error on %s::Init. Backend %s failed to start.", ClassName.c_str(),
				BackendName.c_str() );
}

void Window::onCloseRequest() {
	if ( mCloseRequestCallback && !mCloseRequestCallback( this ) )
		return;

	close();
}

void Window::onQuit() {
	if ( mQuitCallback )
		mQuitCallback( this );
}

const std::string& Window::getTitle() const {
	return mWindow.WindowConfig.Title;
}

eeWindowContext Window::getContext() const {
#if defined( EE_GLEW_AVAILABLE ) &&                                   \
	( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) || \
	  EE_PLATFORM == EE_PLATFORM_MACOS )
	return mWindow.Context;
#else
	return 0;
#endif
}

void Window::getMainContext() {}

void Window::setDefaultContext() {
#if defined( EE_GLEW_AVAILABLE ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) )
	setCurrentContext( mWindow.Context );
#endif
}

void Window::minimize() {}

void Window::maximize() {}

bool Window::isMaximized() const {
	return false;
}

bool Window::isMinimized() const {
	return false;
}

void Window::hide() {}

void Window::hideForReopen() {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	// App-requested fullscreen belongs to SDL. Leave it through the backend so its flags
	// agree with eepp before hiding. SDL2 waits for exit; SDL3 can queue it asynchronously.
	const bool backendExitRequested = !isWindowed();
	if ( backendExitRequested )
		toggleFullscreen();
	// Green-button fullscreen is native-only on SDL2 and still needs the deferred hide.
	if ( Private::WindowVisibility::deferHide(
			 getWindowHandler(), getWindowID(),
			 []( Uint32 id ) {
				 if ( !Engine::isEngineRunning() )
					 return;
				 auto* window = Engine::instance()->getWindowID( id );
				 if ( window && window->isOpen() )
					 window->hide();
			 },
			 !backendExitRequested ) )
		return;
#endif
	hide();
}

void Window::raise() {}

void Window::restore() {}

void Window::flash( WindowFlashOperation ) {}

void Window::show() {
	cancelPendingHide();
}

void Window::activate() {
	if ( isMinimized() )
		restore();
	show();
	raise();
}

void Window::cancelPendingHide() {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	Private::WindowVisibility::cancelHide( getWindowHandler() );
#endif
}

void Window::setPosition( int, int ) {}

Vector2i Window::getPosition() const {
	return Vector2i::Zero;
}

void Window::setCurrentContext( eeWindowContext ) {}

void Window::setCurrent() {}

void Window::centerToDisplay() {
	if ( isWindowed() ) {
		setPosition( mWindow.DesktopResolution.getWidth() / 2 - mWindow.WindowConfig.Width / 2,
					 mWindow.DesktopResolution.getHeight() / 2 - mWindow.WindowConfig.Height / 2 );
	}
}

Rect Window::getBorderSize() const {
	return Rect();
}

Float Window::getScale() const {
	return 1.f;
}

void Window::startTextInput() {}

bool Window::isTextInputActive() const {
	return false;
}

void Window::stopTextInput() {}

void Window::setTextInputRect( const Rect& ) {}

void Window::clearComposition() {}

bool Window::hasScreenKeyboardSupport() const {
	return false;
}

bool Window::isScreenKeyboardShown() const {
	return false;
}

bool Window::isThreadedGLContext() const {
	return false;
}

void Window::setGLContextThread() {}

void Window::unsetGLContextThread() {}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
static void eepp_mainloop() {
	Engine::instance()->getCurrentWindow()->getMainLoop()();
}
#endif

void Window::runMainLoop( std::function<void()> func, int fps ) {
	mMainLoop = std::move( func );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_set_main_loop( eepp_mainloop, eemax( (int)mFrameData.FPS.Limit, eemax( 0, fps ) ),
							  1 );
#else
	if ( fps >= 0 )
		setFrameRateLimit( fps );

	while ( isRunning() ) {
		mMainLoop();
	}
#endif
}

int Window::getCurrentDisplayIndex() const {
	return 0;
}

const std::function<void()>& Window::getMainLoop() const {
	return mMainLoop;
}

}} // namespace EE::Window
