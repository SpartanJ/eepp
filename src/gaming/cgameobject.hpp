#ifndef EE_GAMINGCGAMEOBJECT_HPP
#define EE_GAMINGCGAMEOBJECT_HPP

#include "base.hpp"
#include "maphelper.hpp"

namespace EE { namespace Gaming {

class cGameObject {
	public:
		cGameObject( const Uint32& Flags );

		virtual ~cGameObject();

		virtual void Draw();

		virtual void Update();

		virtual eeVector2f Pos() const;

		virtual void Pos( eeVector2f pos );

		virtual Uint32 Type() const;

		bool IsType( const Uint32& type );

		virtual bool InheritsFrom( const Uint32& Type );

		bool IsTypeOrInheritsFrom( const Uint32& Type );

		const Uint32& Flags() const;

		Uint32 FlagGet( const Uint32& Flag );

		void FlagSet( const Uint32& Flag );

		void FlagClear( const Uint32& Flag );

		Uint32 IsBlocked() const;
	protected:
		Uint32	mFlags;
};

}}

#endif
