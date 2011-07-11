#ifndef EE_WINDOWCINPUTAl_HPP 
#define EE_WINDOWCINPUTAl_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include "../../cinput.hpp"
#include <allegro5/allegro.h>

namespace EE { namespace Window { namespace Backend { namespace Al {

class cWindowAl;

class EE_API cInputAl : public cInput {
	public:
		virtual ~cInputAl();
		
		void Update();

		bool GrabInput();

		void GrabInput( const bool& Grab );

		void InjectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class cWindowAl;

		ALLEGRO_EVENT_QUEUE *	mQueue;
		bool					mGrab;

		cInputAl( Window::cWindow * window );
		
		virtual void Init();

		ALLEGRO_DISPLAY * GetDisplay();

		cWindowAl * GetWindowAl() const;

		void InitializeTables();

		Uint32 SetMod( Uint32 Mod );

		Uint32 mKeyCodesTable[ ALLEGRO_KEY_MAX ];

		Int32 mZ;
};

}}}}

#endif

#endif
