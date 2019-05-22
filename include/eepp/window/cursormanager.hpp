#ifndef EE_WINDOWCCURSORMANAGER_HPP
#define EE_WINDOWCCURSORMANAGER_HPP

#include <eepp/window/base.hpp>

#include <eepp/graphics/image.hpp>
#include <eepp/graphics/texture.hpp>
using namespace EE::Graphics;

#include <eepp/window/window.hpp>
#include <eepp/window/cursor.hpp>

#include <set>

namespace EE { namespace Window {

class EE_API CursorManager {
	public:
		CursorManager( EE::Window::Window * window );

		virtual ~CursorManager();

		/** Creates a cursor from a texture
		* @param tex The texture pointer to use as cursor
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual Cursor *		create( Texture * tex, const Vector2i& hotspot, const std::string& name ) = 0;

		/** Creates a cursor from a image
		* @param img The image path
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual Cursor *		create( Image * img, const Vector2i& hotspot, const std::string& name ) = 0;

		/** Creates a cursor from a image path
		* @param path The image pointer to use as cursor
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual Cursor *		create( const std::string& path, const Vector2i& hotspot, const std::string& name ) = 0;

		/** Adds the cursor to the cursor manager */
		virtual Cursor *		add( Cursor * cursor );

		/** Removes the cursor from the cursor manager
		* @param cursor The cursor pointer
		* @param Delete Indicates if the cursor must be delete after being removed from the cursor manager
		*/
		virtual void			remove( Cursor * cursor, bool Delete = false ) = 0;

		/** Removes the cursor by its name
		* @param name The cursor name
		* @param Delete Indicates if the cursor must be delete after being removed from the cursor manager
		*/
		virtual void			remove( const std::string& name, bool Delete = false );

		/** Removes the cursor by its id
		* @param id The cursor pointer id
		* @param Delete Indicates if the cursor must be delete after being removed from the cursor manager
		*/
		virtual void			remove( const Uint32& id, bool Delete = false );

		/** @return The cursor pointer by its name */
		virtual Cursor *		get( const std::string& name );

		/** @return The cursor pointer by its id */
		virtual Cursor *		getById( const Uint32& id );

		/** Set the the current cursor by its name */
		virtual void			set( const std::string& name );

		/** Set the the current cursor by its id */
		virtual void			setById( const Uint32& id );

		/** Set the the current cursor by its cursor pointer */
		virtual void			set( Cursor * cursor ) = 0;

		/** Set the cursor using a system cursor */
		virtual void			set( Cursor::SysType syscurid ) = 0;

		/** Set the cursor as the global cursor used in eepp
		**	@see SetGlobalCursor */
		virtual void			set( Cursor::Type cursor );

		/** A Global Cursor is a cursor setted to be used in eepp. It's the system cursor of the engine.
		**	The global cursor can be a Cursor ( user created cursor ) or a system cursor ( the OS cursor ).
		**	The system cursor is used by default, but can be override it with this function. */
		virtual void			setGlobalCursor( Cursor::Type cursor, Cursor * fromCursor );

		/** @see SetGlobalCursor */
		virtual void			setGlobalCursor( Cursor::Type cursor, Cursor::SysType fromCursor );

		/** Force to show the cursor */
		virtual void			show() = 0;

		/** Hides the cursor */
		virtual void			hide() = 0;

		/** Force to reset the state of the current seted cursor */
		virtual void			reload() = 0;

		/** Set to show/hide the cursor */
		virtual void			setVisible( bool visible ) = 0;

		/** @return If the cursor is visible in the window */
		virtual bool			getVisible();

		/** @return A pointer to the curent cursor */
		Cursor *				getCurrent() const;

		/** @return The current system cursor */
		Cursor::SysType		getCurrentSysCursor() const;

		/** @return True if the current cursor seted is a system cursor */
		bool					currentIsSysCursor() const;
	protected:
		typedef	std::set<Cursor*> CursorsList;
		class GlobalCursor
		{
			public:
				GlobalCursor() :
					SysCur( Cursor::SysCursorNone ),
					Cur( NULL )
				{
				}

				GlobalCursor( Cursor::SysType sysCur, Cursor * cur ) :
					SysCur( sysCur ),
					Cur( cur )
				{
				}

				Cursor::SysType	SysCur;
				Cursor *			Cur;
		};
		GlobalCursor			mGlobalCursors[ Cursor::CursorCount ];
		EE::Window::Window *				mWindow;
		Cursor *				mCurrent;
		Cursor::SysType		mSysCursor;
		CursorsList				mCursors;
		bool					mCurSysCursor;
		bool					mVisible;

		void initGlobalCursors();
};

}}

#endif
