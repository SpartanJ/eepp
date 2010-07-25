#include "cengine.hpp"
#include "cinput.hpp"
#include "../graphics/ctexturefactory.hpp"
#include "../graphics/cglobalbatchrenderer.hpp"
#include "../graphics/cshaderprogrammanager.hpp"
#include "../ui/cuimanager.hpp"

using namespace EE::Graphics;

#define T(A, B, C, D)	(int)((A<<24)|(B<<16)|(C<<8)|(D<<0))
#define FORMAT_PREFIX	"SDL_scrap_0x"

namespace EE { namespace Window {

#if EE_PLATFORM == EE_PLATFORM_LINUX
static int clipboard_filter(const SDL_Event *event) {
	/* Post all non-window manager specific events */
	if ( event->type != SDL_SYSWMEVENT ) {
		return(1);
	}

	Display* SDL_Display = cEngine::instance()->GetVideoInfo()->info.info.x11.display;

	/* Handle window-manager specific clipboard events */
	switch ( event->syswm.msg->event.xevent.type ) {
		/* Copy the selection from XA_CUT_BUFFER0 to the requested property */
		case SelectionRequest: {
			XSelectionRequestEvent *req;
			XEvent sevent;
			int seln_format;
			unsigned long nbytes;
			unsigned long overflow;
			unsigned char *seln_data;

			req = &event->syswm.msg->event.xevent.xselectionrequest;
			sevent.xselection.type = SelectionNotify;
			sevent.xselection.display = req->display;
			sevent.xselection.selection = req->selection;
			sevent.xselection.target = None;
			sevent.xselection.property = None;
			sevent.xselection.requestor = req->requestor;
			sevent.xselection.time = req->time;

			if ( XGetWindowProperty(SDL_Display, DefaultRootWindow(SDL_Display), XA_CUT_BUFFER0, 0, INT_MAX/4, False, req->target, &sevent.xselection.target, &seln_format, &nbytes, &overflow, &seln_data) == Success ) {
				if ( sevent.xselection.target == req->target ) {
					if ( sevent.xselection.target == XA_STRING ) {
						if ( seln_data[nbytes-1] == '\0' )
							--nbytes;
					}
					XChangeProperty(SDL_Display, req->requestor, req->property, sevent.xselection.target, seln_format, PropModeReplace, seln_data, nbytes);
					sevent.xselection.property = req->property;
				}
				XFree(seln_data);
			}
			XSendEvent(SDL_Display,req->requestor, False, 0, &sevent);
			XSync(SDL_Display, False);
		}
		break;
	}

	/* Post the event for X11 clipboard reading above */
	return(1);
}
#endif

cEngine::cEngine() : mInit(false), mBackColor(0, 0, 0), mCursor(NULL), mShowCursor(true) {
	mFrames.FPS.LastCheck = 0;
	mFrames.FPS.Current = 0;
	mFrames.FPS.Count = 0;
	mFrames.FPS.Limit = 0;
	mFrames.FPS.Error = 0;
	mFrames.ElapsedTime = 0;
}

cEngine::~cEngine() {
	if ( mCursor != NULL ) {
		SDL_FreeCursor(mCursor);
	}

	cGlobalBatchRenderer::DestroySingleton();
	cTextureFactory::DestroySingleton();

	#ifdef EE_SHADERS
	cShaderProgramManager::DestroySingleton();
	#endif

	UI::cUIManager::DestroySingleton();

	cInput::DestroySingleton();
	SDL_Quit();
	cLog::DestroySingleton();
}

bool cEngine::Init(const Uint32& Width, const Uint32& Height, const Uint8& BitColor, const bool& Windowed, const bool& Resizeable, const bool& VSync, const bool& DoubleBuffering, const bool& UseDesktopResolution, const bool& NoFrame ) {
	try {
		mInit = false;

		mVideoInfo.Width = Width;
		mVideoInfo.Height = Height;
		mVideoInfo.ColorDepth = BitColor;
		mVideoInfo.Windowed = Windowed;
		mVideoInfo.Resizeable = Resizeable;
		mVideoInfo.DoubleBuffering = DoubleBuffering;
		mVideoInfo.VSync = VSync;
		mVideoInfo.NoFrame = NoFrame;
		mVideoInfo.LineSmooth = true;
		mVideoInfo.SupShaders = false;
		mOldWinPos = eeVector2i( 0, 0 );

		if ( SDL_Init(SDL_INIT_VIDEO) != 0 ) {
			cLog::Instance()->Write( "Unable to initialize SDL: " + std::string( SDL_GetError() ) );
			return false;
		}

		mVideoInfo.Flags = SDL_OPENGL | SDL_HWPALETTE;
		const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();

		mVideoInfo.DeskWidth = videoInfo->current_w;
		mVideoInfo.DeskHeight = videoInfo->current_h;

		if ( UseDesktopResolution ) {
			mVideoInfo.Width = mVideoInfo.DeskWidth;
			mVideoInfo.Height = mVideoInfo.DeskHeight;
		}

		if (videoInfo->hw_available)
			mVideoInfo.Flags |= SDL_HWSURFACE;
		else
			mVideoInfo.Flags |= SDL_SWSURFACE;

		if ( videoInfo->blit_hw ) 	// This checks if hardware blits can be done
			mVideoInfo.Flags |= SDL_HWACCEL;

		if ( mVideoInfo.Resizeable )
			mVideoInfo.Flags |= SDL_RESIZABLE;

		if ( mVideoInfo.NoFrame )
			mVideoInfo.Flags |= SDL_NOFRAME;

		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); 	// Depth
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, (mVideoInfo.DoubleBuffering ? 1 : 0) ); 	// Double Buffering
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
		SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, (mVideoInfo.VSync ? 1 : 0)  );  // VSync

		Uint32 mTmpFlags = mVideoInfo.Flags;
    	if (!mVideoInfo.Windowed)
    		mTmpFlags |= SDL_FULLSCREEN;

		if ( SDL_VideoModeOK(mVideoInfo.Width, mVideoInfo.Height, mVideoInfo.ColorDepth, mTmpFlags) )
			mVideoInfo.Screen = SDL_SetVideoMode(mVideoInfo.Width, mVideoInfo.Height, mVideoInfo.ColorDepth, mTmpFlags);
		else {
			cLog::Instance()->Write( "Video Mode Unsopported for this videocard: " );
			return false;
		}

		mInitialWidth = mVideoInfo.Width;
		mInitialHeight = mVideoInfo.Height;

		mVideoInfo.WWidth = mVideoInfo.Width;
		mVideoInfo.WHeight = mVideoInfo.Height;

		mVideoInfo.Maximized = false;

		if ( mVideoInfo.Screen == NULL ) {
			cLog::Instance()->Write( "Unable to set video mode: " + std::string( SDL_GetError() ) );
			return false;
		}

		SDL_VERSION(&mVideoInfo.info.version);
		SDL_GetWMInfo (&mVideoInfo.info);

		#if EE_PLATFORM == EE_PLATFORM_LINUX
		/* Enable the special window hook events */
		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
		SDL_SetEventFilter(clipboard_filter);
		/*#elif EE_PLATFORM == EE_PLATFORM_WIN32
		if ( mVideoInfo.Windowed )
			ChangeRes(mVideoInfo.Width, mVideoInfo.Height, mVideoInfo.Windowed );*/
		#endif

		if ( mVideoInfo.ColorDepth == 16 ) {
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 4);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 4);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 4);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 4);
		} else {
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		}

		if ( mVideoInfo.Windowed )
			mOldWinPos = GetWindowPosition();

		mVideoInfo.SupARB_point = ( GetExtension("GL_ARB_point_parameters") && GetExtension("GL_ARB_point_sprite") );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		mDefaultView.SetView( 0, 0, mVideoInfo.Width, mVideoInfo.Height );
		mCurrentView = &mDefaultView;

		ResetGL2D();

		#ifdef EE_SHADERS
		mVideoInfo.SupShaders = GetExtension("GL_ARB_shading_language_100") && GetExtension("GL_ARB_shader_objects") && GetExtension("GL_ARB_vertex_shader") && GetExtension("GL_ARB_fragment_shader");

		if ( mVideoInfo.SupShaders ) {
			glewInit();
		}
		#endif

		SetWindowCaption("EEPP");

		cLog::instance()->Write( "Engine Initialized Succesfully.\nGL Vendor: " + GetVendor() + "\nGL Renderer: " + GetRenderer() + "\nGL Version: " + GetVersion() );
		mInit = true;
		return true;
	} catch (...) {
		cLog::instance()->Write( "Error on cEngine::Init" );
		return false;
	}
}

void cEngine::SetViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	glViewport( x, GetHeight() - Height - y, Width, Height );
	glOrtho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

void cEngine::SetView( const cView& View ) {
	mCurrentView = &View;

	eeRecti RView = mCurrentView->GetView();
	SetViewport( RView.Left, RView.Top, RView.Right, RView.Bottom );
}

const cView& cEngine::GetDefaultView() const {
	return mDefaultView;
}

const cView& cEngine::GetView() const {
    return *mCurrentView;
}

void cEngine::ResetGL2D() {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	SetBackColor( mBackColor );

	glShadeModel( GL_SMOOTH );
	SetLineSmooth( mVideoInfo.LineSmooth );

	glEnable( GL_TEXTURE_2D ); 						// Enable Textures

	SetView( mDefaultView );

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );

	cTextureFactory::instance()->SetBlendFunc( ALPHA_BLENDONE );  // This is to fix a little bug on windows when the resolution change. I don't know why it happens, but this line fix it.
	cTextureFactory::instance()->SetBlendFunc( ALPHA_NORMAL );

	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
}

void cEngine::CalculateFps() {
	if ( eeGetTicks() - mFrames.FPS.LastCheck >= 1000 ) {
		mFrames.FPS.Current = mFrames.FPS.Count;
		mFrames.FPS.Count = 0;
		mFrames.FPS.LastCheck =	eeGetTicks();
	}
	mFrames.FPS.Count++;
}

void cEngine::LimitFps() {
	if ( mFrames.FPS.Limit > 0 ) {
		mFrames.FPS.Error = 0;
		eeFloat RemainT = 1000.f / mFrames.FPS.Limit - ( mFrames.ElapsedTime * 0.1f );

		if ( RemainT < 0 ) {
			mFrames.FPS.Error = 0;
		} else {
			mFrames.FPS.Error += 1000 % (Int32)mFrames.FPS.Limit;

			if ( mFrames.FPS.Error > (Int32)mFrames.FPS.Limit ) {
				++RemainT;
				mFrames.FPS.Error -= (Int32)mFrames.FPS.Limit;
			}

			if ( RemainT > 0 )
				SDL_Delay( (Int32) RemainT );
		}
	}
}

void cEngine::Display() {
	cGlobalBatchRenderer::instance()->Draw();

	if ( mCurrentView->NeedUpdate() )
		SetView( *mCurrentView );

	glFlush();
	SDL_GL_SwapBuffers();
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	GetElapsedTime();
	CalculateFps();
	LimitFps();
}

void cEngine::ChangeRes( const Uint16& width, const Uint16& height, const bool& Windowed ) {
	try {
		cLog::Instance()->Write( "Switching from %s to %s. Width: %d Height %d.", mVideoInfo.Windowed == true ? "windowed" : "fullscreen", Windowed == true ? "windowed" : "fullscreen", width, height );

		#if EE_PLATFORM == EE_PLATFORM_WIN32 || EE_PLATFORM == EE_PLATFORM_APPLE
		bool Reload = mVideoInfo.Windowed != Windowed;

		if ( Reload )
			cTextureFactory::instance()->GrabTextures();
		#endif

		mVideoInfo.Windowed = Windowed;
		mVideoInfo.Width = width;
		mVideoInfo.Height = height;

		if ( Windowed ) {
			mVideoInfo.WWidth = width;
			mVideoInfo.WHeight = height;
		}

		mDefaultView.SetView( 0, 0, mVideoInfo.Width, mVideoInfo.Height );

		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); 	// Depth
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, (mVideoInfo.DoubleBuffering ? 1 : 0) );
		SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, (mVideoInfo.VSync ? 1 : 0)  );  // VSync

		if (Windowed)
			mVideoInfo.Screen = SDL_SetVideoMode( mVideoInfo.Width, mVideoInfo.Height, mVideoInfo.ColorDepth, mVideoInfo.Flags );
		else
			mVideoInfo.Screen = SDL_SetVideoMode( mVideoInfo.Width, mVideoInfo.Height, mVideoInfo.ColorDepth, mVideoInfo.Flags | SDL_FULLSCREEN );

		ResetGL2D();

		#if EE_PLATFORM == EE_PLATFORM_WIN32 || EE_PLATFORM == EE_PLATFORM_APPLE
		if ( Reload ) {
			cTextureFactory::instance()->ReloadAllTextures();
			cShaderProgramManager::instance()->Reload();
		}
		#endif

		if ( UI::cUIManager::Instance() != NULL )
			UI::cUIManager::instance()->ResizeControl();

		if ( !mVideoInfo.Screen )
			mInit = false;
	} catch (...) {
		cLog::Instance()->Write( "Unable to change resolution: " + std::string( SDL_GetError() ) );
        cLog::Instance()->Save();
		mInit = false;
	}
}

Uint32 cEngine::FPS() const {
	return mFrames.FPS.Current;
}

void cEngine::SetBackColor(const eeColor& Color) {
	mBackColor = Color;
	glClearColor( static_cast<eeFloat>( mBackColor.R() ) / 255.0f, static_cast<eeFloat>( mBackColor.G() ) / 255.0f, static_cast<eeFloat>( mBackColor.B() ) / 255.0f, 255.0f );
}

void cEngine::SetWindowCaption(const std::string& Caption) {
	SDL_WM_SetCaption( Caption.c_str(), NULL );
}

void cEngine::GetElapsedTime() {
	mFrames.ElapsedTime = static_cast<eeFloat> ( mFrames.FrameElapsed.Elapsed() );
}

eeFloat cEngine::Elapsed() const {
	return mFrames.ElapsedTime;
}

bool cEngine::Windowed() const {
	return mVideoInfo.Windowed;
}

void cEngine::ToggleFullscreen() {
	#if EE_PLATFORM == EE_PLATFORM_WIN32
		bool WasMaximized = mVideoInfo.Maximized;

		if ( mVideoInfo.Windowed )
			ChangeRes( mVideoInfo.Width, mVideoInfo.Height, !mVideoInfo.Windowed );
		else
			ChangeRes( mVideoInfo.WWidth, mVideoInfo.WHeight, !mVideoInfo.Windowed );

		if ( WasMaximized )
			MaximizeWindow();
	#else
		mVideoInfo.Windowed = !mVideoInfo.Windowed;
		SDL_WM_ToggleFullScreen( mVideoInfo.Screen );
	#endif

	ShowCursor(mShowCursor);
}

void cEngine::ShowCursor(const bool& showcursor) {
	mShowCursor = showcursor;
	if ( mShowCursor )
		SDL_ShowCursor(SDL_ENABLE);
	else
		SDL_ShowCursor(SDL_DISABLE);
}

void cEngine::SetFrameRateLimit(const Uint32& FrameRateLimit) {
	mFrames.FPS.Limit = (eeFloat)FrameRateLimit;
}

bool cEngine::TakeScreenshot( std::string filepath, const EE_SAVETYPE& Format ) {
	bool CreateNewFile = false;
	std::string File, Ext;

	if ( filepath.size() ) {
		File = filepath.substr( filepath.find_last_of("/\\") + 1 );
		Ext = File.substr( File.find_last_of(".") + 1 );

		if ( IsDirectory( filepath ) || !Ext.size() )
			CreateNewFile = true;
	} else {
		filepath = AppPath();
		CreateNewFile = true;
	}

	if ( CreateNewFile ) { // Search if file path is given, and if have and extension
		bool find = false;
		Int32 FileNum = 1;
		std::string TmpPath = filepath, Ext;

		if ( !IsDirectory( filepath ) )
			MakeDir( filepath );

		switch ( Format ) {
			case EE_SAVE_TYPE_TGA:	Ext = ".tga";	break;
			case EE_SAVE_TYPE_BMP:	Ext = ".bmp";	break;
			case EE_SAVE_TYPE_PNG:	Ext = ".png";	break;
			case EE_SAVE_TYPE_DDS:	Ext = ".dds";	break;
		}

		while ( !find && FileNum < 10000 ) {
			if ( FileNum < 10 ) {
				TmpPath = filepath + "000" + intToStr(FileNum) + Ext;
			} else if ( FileNum < 100 ) {
				TmpPath = filepath + "00" + intToStr(FileNum) + Ext;
			} else if ( FileNum < 1000 ) {
				TmpPath = filepath + "0" + intToStr(FileNum) + Ext;
			} else {
				TmpPath = intToStr(FileNum) + Ext;
			}

			FileNum++;

			if ( !FileExists( TmpPath ) )
				find = true;

			if ( FileNum == 10000 && find == false )
				return false;
		}

		return 0 != SOIL_save_screenshot(TmpPath.c_str(), Format, 0, 0, mVideoInfo.Width, mVideoInfo.Height );
	} else {
		std::string Direc = filepath.substr( 0, filepath.find_last_of("/\\") );
		if ( !IsDirectory( Direc ) )
			MakeDir( Direc );

		return 0 != SOIL_save_screenshot(filepath.c_str(), Format, 0, 0, mVideoInfo.Width, mVideoInfo.Height );
	}
}

std::vector< std::pair<unsigned int, unsigned int> > cEngine::GetPossibleResolutions() const {
	SDL_Rect **modes = SDL_ListModes( NULL, SDL_OPENGL | SDL_HWPALETTE | SDL_HWACCEL | SDL_FULLSCREEN );

	if(modes == (SDL_Rect **)0)
		cLog::Instance()->Write("No VideoMode Found");

	std::vector< std::pair<unsigned int, unsigned int> > result;
	if( modes != (SDL_Rect **)-1 )
		for(unsigned int i = 0; modes[i]; ++i)
			result.push_back( std::pair<unsigned int, unsigned int>(modes[i]->w, modes[i]->h) );

	return result;
}

void cEngine::SetGamma( const eeFloat& Red, const eeFloat& Green, const eeFloat& Blue ) {
	if ( Red >= 0.1f && Red <= 10.0f && Green >= 0.1f && Green <= 10.0f && Blue >= 0.1f && Blue <= 10.0f )
		SDL_SetGamma( Red, Green, Blue );
}

void cEngine::SetLineSmooth( const bool& Enable ) {
	mVideoInfo.LineSmooth = Enable;
	if ( Enable )
		glEnable( GL_LINE_SMOOTH );
	else
		glDisable( GL_LINE_SMOOTH );
}

void cEngine::SetPolygonMode( const EE_FILLMODE& Mode ) {
	if ( Mode == DRAW_FILL )
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	else
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
}

void cEngine::ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	glScissor( x, GetHeight() - Height - y, Width, Height );
	glEnable( GL_SCISSOR_TEST );
}

void cEngine::ClipDisable() {
	glDisable( GL_SCISSOR_TEST );
}

std::string cEngine::GetVendor() {
	return std::string( reinterpret_cast<const char*> ( glGetString( GL_VENDOR ) ) );
}

std::string cEngine::GetRenderer() {
	return std::string( reinterpret_cast<const char*> ( glGetString( GL_RENDERER ) ) );
}

std::string cEngine::GetVersion() {
	return std::string( reinterpret_cast<const char*> ( glGetString( GL_VERSION ) ) );
}

bool cEngine::GetExtension( const std::string& Ext ) {
	char *Exts = (char *)glGetString(GL_EXTENSIONS);

	if ( strstr( Exts, Ext.c_str() ) )
		return true;

	return false;
}

SDL_Cursor* cEngine::CreateCursor( const Uint32& TexId, const eeVector2i& HotSpot ) {
	cTexture* Tex = cTextureFactory::instance()->GetTexture( TexId );

	if ( Tex == NULL )
		return NULL;

	//the width must be a multiple of 8 (SDL requirement)
	#if EE_PLATFORM == EE_PLATFORM_APPLE
	size_t cursor_width = 16;
	#else
	size_t cursor_width = static_cast<size_t>(Tex->Width());
	if ( (cursor_width % 8) != 0 ) {
		cursor_width += 8 - ( cursor_width % 8 );
	}
	#endif

	std::vector<Uint8> data( ( cursor_width * static_cast<Uint8>( Tex->Height() ) ) / 8, 0);
	std::vector<Uint8> mask( data.size(), 0 );

	//see http://sdldoc.csn.ul.ie/sdlcreatecursor.php for documentation on
	//the format that data has to be in to pass to SDL_CreateCursor
	Tex->Lock();
	for( Int32 y = 0; y != Tex->Height(); ++y) {
		for( Int32 x = 0; x != Tex->Width(); ++x) {
			Uint8 trans = 0;
			Uint8 black = 0;

			const size_t index = y * cursor_width + x;

			if ( (size_t)x < cursor_width ) {
				eeColorA Col = Tex->GetPixel( x, y );

				const size_t shift = 7 - ( index % 8 );

				trans = ( Col.A() < 128 ? 0 : 1) << shift;
				black = (trans == 0 || ( Col.R() + Col.G() + Col.B() ) /3 > 128 ? 0 : 1) << shift;

				data[index/8] |= black;
				mask[index/8] |= trans;
			}
		}
	}
	Tex->Unlock( false, false );

	return SDL_CreateCursor( &data[0], &mask[0], cursor_width, static_cast<Int16>( Tex->Height() ), HotSpot.x, HotSpot.y );
}

void cEngine::SetCursor( const Uint32& TexId, const eeVector2i& HotSpot ) {
	mCursor = CreateCursor( TexId, HotSpot );
	if ( mCursor )
		SDL_SetCursor( mCursor );
}

bool cEngine::SetIcon( const Uint32& FromTexId ) {
	cTexture* Tex = cTextureFactory::instance()->GetTexture( FromTexId );

	if ( Tex ) {
		Int32 W = static_cast<Int32>( Tex->Width() );
		Int32 H = static_cast<Int32>( Tex->Height() );
		if ( ( W  % 8 ) == 0 && ( H % 8 ) == 0 ) {
			Tex->Lock();
			const Uint8* Ptr = Tex->GetPixelsPtr();

			Uint32 rmask, gmask, bmask, amask;
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				rmask = 0xff000000;
				gmask = 0x00ff0000;
				bmask = 0x0000ff00;
				amask = 0x000000ff;
			#else
				rmask = 0x000000ff;
				gmask = 0x0000ff00;
				bmask = 0x00ff0000;
				amask = 0xff000000;
			#endif
			SDL_Surface* TempGlyphSheet = SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, 32, rmask, gmask, bmask, amask);
			SDL_LockSurface(TempGlyphSheet);

			Uint32 ssize = TempGlyphSheet->w * TempGlyphSheet->h * 4;
			for (Uint32 i=0; i<ssize; i++) {
				(static_cast<Uint8*>(TempGlyphSheet->pixels))[i+0] = (Ptr)[i];
			}

			SDL_UnlockSurface(TempGlyphSheet);

			Tex->Unlock();

			SDL_WM_SetIcon(TempGlyphSheet, NULL);

			SDL_FreeSurface(TempGlyphSheet);
			return true;
		}
	}
	return false;
}

void cEngine::MinimizeWindow() {
	SDL_WM_IconifyWindow();
}

void cEngine::MaximizeWindow() {
	#if EE_PLATFORM == EE_PLATFORM_WIN32
		WIN_ShowWindow(mVideoInfo.info.window, SW_MAXIMIZE);
	#elif EE_PLATFORM == EE_PLATFORM_LINUX
		// coded by Rafał Maj, idea from Måns Rullgård http://tinyurl.com/68mvk3
		mVideoInfo.info.info.x11.lock_func();

		XEvent xev;
		Atom wm_state =  XInternAtom( mVideoInfo.info.info.x11.display, "_NET_WM_STATE", False);
		Atom maximizeV = XInternAtom( mVideoInfo.info.info.x11.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
		Atom maximizeH = XInternAtom( mVideoInfo.info.info.x11.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

		memset( &xev, 0, sizeof(xev) );
		xev.type = ClientMessage;
		xev.xclient.window = mVideoInfo.info.info.x11.wmwindow;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1;
		xev.xclient.data.l[1] = maximizeV;
		xev.xclient.data.l[2] = maximizeH;
		xev.xclient.data.l[3] = 0;
		XSendEvent( mVideoInfo.info.info.x11.display, DefaultRootWindow(mVideoInfo.info.info.x11.display), 0, SubstructureNotifyMask|SubstructureRedirectMask, &xev);

		XFlush(mVideoInfo.info.info.x11.display);

		mVideoInfo.info.info.x11.unlock_func();
	#elif EE_PLATFORM == EE_PLATFORM_APPLE
		#warning cEngine::MaximizeWindow() not implemented on Apple
	#endif
}

void cEngine::HideWindow() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	mVideoInfo.info.info.x11.lock_func();
    XUnmapWindow( mVideoInfo.info.info.x11.display, mVideoInfo.info.info.x11.wmwindow );
	mVideoInfo.info.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN32
    WIN_ShowWindow( mVideoInfo.info.window, SW_HIDE );
#elif EE_PLATFORM == EE_PLATFORM_APPLE
	#warning cEngine::HideWindow() not implemented on Apple
#endif
}

void cEngine::RaiseWindow() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	mVideoInfo.info.info.x11.lock_func();
    XRaiseWindow( mVideoInfo.info.info.x11.display, mVideoInfo.info.info.x11.wmwindow );
	mVideoInfo.info.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN32
    HWND top;

    if ( !mVideoInfo.Windowed )
        top = HWND_TOPMOST;
    else
        top = HWND_NOTOPMOST;

    SetWindowPos( mVideoInfo.info.window, top, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE) );
#elif EE_PLATFORM == EE_PLATFORM_APPLE
	#warning cEngine::RaiseWindow() not implemented on Apple
#endif
}

void cEngine::ShowWindow() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	mVideoInfo.info.info.x11.lock_func();
	XMapRaised( mVideoInfo.info.info.x11.display, mVideoInfo.info.info.x11.wmwindow );
	mVideoInfo.info.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN32
	WIN_ShowWindow( mVideoInfo.info.window, SW_SHOW );
#elif EE_PLATFORM == EE_PLATFORM_APPLE
	#warning cEngine::RaiseWindow() not implemented on Apple
#endif
}

void cEngine::SetWindowPosition(Int16 Left, Int16 Top) {
#if EE_PLATFORM == EE_PLATFORM_LINUX
    XMoveWindow( mVideoInfo.info.info.x11.display, mVideoInfo.info.info.x11.wmwindow, Left, Top);
    XFlush( mVideoInfo.info.info.x11.display );
#elif EE_PLATFORM == EE_PLATFORM_WIN32
	SetWindowPos( mVideoInfo.info.window, NULL, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
#elif EE_PLATFORM == EE_PLATFORM_APPLE
	#warning cEngine::SetWindowPosition() not implemented on Apple
#endif
}

eeVector2i cEngine::GetWindowPosition() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	XWindowAttributes Attrs;
	XGetWindowAttributes( mVideoInfo.info.info.x11.display, mVideoInfo.info.info.x11.wmwindow, &Attrs );

	return eeVector2i( Attrs.x, Attrs.y );
#elif EE_PLATFORM == EE_PLATFORM_WIN32
	RECT r;
	GetWindowRect( mVideoInfo.info.window, &r );
	return eeVector2i( r.left, r.top );
#elif EE_PLATFORM == EE_PLATFORM_APPLE
	return eeVector2i( 0, 0 );
	//#warning cEngine::GetWindowPos() not implemented on Apple
#endif
}

bool cEngine::WindowActive() {
	return 0 != ( SDL_GetAppState() & SDL_APPINPUTFOCUS );
}

bool cEngine::WindowVisible() {
	return 0 != ( SDL_GetAppState() & SDL_APPACTIVE );
}

void cEngine::SetWindowSize( const Uint32& Width, const Uint32& Height ) {
	if ( mVideoInfo.Windowed )
		ChangeRes( Width, Height, true );
}

int cEngine::clipboard_convert_scrap(int type, char *dst, char *src, int srclen) {
	int dstlen;

	dstlen = 0;
	switch (type) {
		case T('T', 'E', 'X', 'T'):
			if ( srclen == 0 )
				srclen = strlen(src);

			if ( dst ) {
				while ( --srclen >= 0 ) {
					#if EE_PLATFORM == EE_PLATFORM_WIN32
					if ( *src == '\r' )
					/* drop extraneous '\r' */;
					else
					#endif
					if ( *src == '\n' ) {
						*dst++ = '\r';
						++dstlen;
					} else {
						*dst++ = *src;
						++dstlen;
					}
					++src;
				}
				*dst = '\0';
				++dstlen;
			} else {
				while ( --srclen >= 0 ) {
					#if EE_PLATFORM == EE_PLATFORM_WIN32
					if ( *src == '\r' )
					/* drop unspected '\r' */;
					else
					#endif
					++dstlen;
					++src;
				}
				++dstlen;
			}
			break;
		default:
			dstlen = *(int *)src;
			if ( dst ) {
				if ( srclen == 0 )
					memcpy(dst, src+sizeof(int), dstlen);
				else
					memcpy(dst, src+sizeof(int), srclen-sizeof(int));
			}
			break;
	}
	return dstlen;
}

eeScrapType cEngine::clipboard_convert_format(int type) {
	switch (type) {
		case T('T', 'E', 'X', 'T'):
			#if EE_PLATFORM == EE_PLATFORM_LINUX
			return XA_STRING;
			#elif EE_PLATFORM == EE_PLATFORM_WIN32
			return CF_TEXT;
			#endif
		default: {
			char format[ sizeof(FORMAT_PREFIX)+8+1 ];
			StrFormat(format, sizeof(FORMAT_PREFIX)+8+1, "%s%08lx", FORMAT_PREFIX, (unsigned long)type);

			#if EE_PLATFORM == EE_PLATFORM_LINUX
			return XInternAtom( mVideoInfo.info.info.x11.display, format, False );
			#elif EE_PLATFORM == EE_PLATFORM_WIN32
				#ifdef UNICODE
				return RegisterClipboardFormat( reinterpret_cast<LPCWSTR>( format ) );
				#else
				return RegisterClipboardFormat( reinterpret_cast<LPCSTR>( format ) );
				#endif
			#endif
		}
	}
}

void cEngine::clipboard_get_scrap(int type, int *dstlen, char **dst) {
	eeScrapType format;
	*dstlen = 0;
	format = clipboard_convert_format(type);

#if EE_PLATFORM == EE_PLATFORM_LINUX
    X11Window owner;
    Atom selection;
    Atom seln_type;
    int seln_format;
    unsigned long nbytes;
    unsigned long overflow;
    char *src;

	mVideoInfo.info.info.x11.lock_func();
	owner = XGetSelectionOwner( mVideoInfo.info.info.x11.display , XA_PRIMARY);
	mVideoInfo.info.info.x11.unlock_func();

	if ( ( owner == None ) || ( owner == mVideoInfo.info.info.x11.wmwindow ) ) {
		owner = DefaultRootWindow( mVideoInfo.info.info.x11.display );
		selection = XA_CUT_BUFFER0;
	} else {
		int selection_response = 0;
		SDL_Event event;

        owner = mVideoInfo.info.info.x11.wmwindow;
		mVideoInfo.info.info.x11.lock_func();

		selection = XInternAtom( mVideoInfo.info.info.x11.display, "SDL_SELECTION", False);
		XConvertSelection( mVideoInfo.info.info.x11.display, XA_PRIMARY, format, selection, owner, CurrentTime);

        mVideoInfo.info.info.x11.unlock_func();

		while ( ! selection_response ) {
			SDL_WaitEvent(&event);
			if ( event.type == SDL_SYSWMEVENT ) {
				XEvent xevent = event.syswm.msg->event.xevent;
				if ( (xevent.type == SelectionNotify) && (xevent.xselection.requestor == owner) )
					selection_response = 1;
			}
		}
	}

	mVideoInfo.info.info.x11.lock_func();
    if ( XGetWindowProperty( mVideoInfo.info.info.x11.display, owner, selection, 0, INT_MAX/4, False, format, &seln_type, &seln_format, &nbytes, &overflow, (unsigned char **)&src) == Success ) {
		if ( seln_type == format ) {
			*dstlen = clipboard_convert_scrap(type, NULL, src, nbytes);
			*dst = (char *)realloc(*dst, *dstlen);

			if ( *dst == NULL )
				*dstlen = 0;
			else
				clipboard_convert_scrap(type, *dst, src, nbytes);
		}
		XFree(src);
	}
    mVideoInfo.info.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN32
	if ( IsClipboardFormatAvailable(format) && OpenClipboard( mVideoInfo.info.window ) ) {
		HANDLE hMem;
		char *src;
		hMem = GetClipboardData(format);
		if ( hMem != NULL ) {
			src = (char *)GlobalLock(hMem);
			*dstlen = clipboard_convert_scrap(type, NULL, src, 0);
			*dst = (char *)realloc(*dst, *dstlen);
			if ( *dst == NULL )
			*dstlen = 0;
			else
			clipboard_convert_scrap(type, *dst, src, 0);
			GlobalUnlock(hMem);
		}
		CloseClipboard();
	}
#endif
}

std::string cEngine::GetClipboardText() {
	std::string tStr;
	#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_WIN32
	char *scrap = NULL;
	int scraplen;

	clipboard_get_scrap(T('T','E','X','T'), &scraplen, &scrap);
	if ( scraplen != 0 && strcmp(scrap,"SDL-\r-scrap") ) {
		char *cp;
		int   i;
		for ( cp=scrap, i=0; i<scraplen; ++cp, ++i ) {
			if ( *cp == '\r' )
				*cp = '\n';
		}

		tStr.assign( scrap, scraplen-1 );
	}
	#elif EE_PLATFORM == EE_PLATFORM_APPLE
		#warning cEngine::GetClipboardText() not implemented on Apple
	#endif

	if ( scrap )
		eeSAFE_DELETE_ARRAY( scrap );

	return tStr;
}

std::wstring cEngine::GetClipboardTextWStr() {
	std::wstring tStr;
	#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_WIN32
	char * scrap = NULL;
	int scraplen;

	clipboard_get_scrap(T('T','E','X','T'), &scraplen, &scrap);
	if ( scraplen != 0 && strcmp(scrap,"SDL-\r-scrap") ) {
		tStr.resize( scraplen-1, L' ' );

		char *cp;
		int   i;

		for ( cp = scrap, i = 0; i < scraplen; ++cp, ++i ) {
			if ( *cp == '\r' )
				*cp = '\n';

			unsigned char y = *cp; // convert the nevative values to positives
			tStr[i] = y;
		}
	}
	#elif EE_PLATFORM == EE_PLATFORM_APPLE
		#warning cEngine::GetClipboardTextWStr() not implemented on Apple
	#endif

	if ( scrap )
		eeSAFE_DELETE_ARRAY( scrap );

	return tStr;
}

}}
