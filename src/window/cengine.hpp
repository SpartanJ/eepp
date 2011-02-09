#ifndef EE_WINDOWCENGINE_H
#define EE_WINDOWCENGINE_H

#include "base.hpp"
#include "cbackend.hpp"
#include "cwindow.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifdef CreateWindow
#undef CreateWindow
#endif
#endif

namespace EE { namespace Window {

/** @brief The window management class. Here the engine start working. (Singleton Class). */
class EE_API cEngine : public tSingleton<cEngine> {
	friend class tSingleton<cEngine>;
	public:
		~cEngine();

		cWindow * CreateWindow( WindowSettings Settings, ContextSettings Context );

		void DestroyWindow( cWindow * window );

		cWindow * GetCurrentWindow() const;

		void SetCurrentWindow( cWindow * window );

		Uint32	GetWindowCount() const;

		bool Running() const;

		void Close();

		eeFloat Elapsed() const;

		const Uint32& GetWidth() const;

		const Uint32& GetHeight() const;

		bool ExistsWindow( cWindow * window );
	protected:
		friend class cWindow;

		Backend::cBackend *	mBackend;
		std::list<cWindow*>	mWindows;
		cWindow *			mWindow;

		cEngine();

		void Destroy();
};

}}

#endif
