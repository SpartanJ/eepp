#ifndef EE_WINDOWCENGINE_H
#define EE_WINDOWCENGINE_H

#include "base.hpp"
#include "cbackend.hpp"
#include "cwindow.hpp"

namespace EE { namespace Window {

/** @brief The window management class. Here the engine start working. (Singleton Class). */
class EE_API cEngine : public tSingleton<cEngine> {
	friend class tSingleton<cEngine>;
	public:
		~cEngine();

		/** Creates a new window.
		* Allegro 5 backend support more than one window creation, SDL backend only 1 window.
		*/
		cWindow * CreateWindow( WindowSettings Settings, ContextSettings Context );

		/** Destroy the window instance, and set as current other window running ( if any ).
		* This function is only usefull for multi-window environment. Avoid using it with one window context.
		*/
		void DestroyWindow( cWindow * window );

		/** @return The current Window context. */
		cWindow * GetCurrentWindow() const;

		/** Set the window as the current. */
		void SetCurrentWindow( cWindow * window );

		/** @return The number of windows created. */
		Uint32	GetWindowCount() const;

		/** @return If any window is created. */
		bool Running() const;

		/** @return The current window elapsed time. */
		eeFloat Elapsed() const;

		/** @return The current window width. */
		const Uint32& GetWidth() const;

		/** @return The current window height */
		const Uint32& GetHeight() const;

		/** @return If the window instance is inside the window list. */
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
