#ifndef EE_GAMINGCLAYER_HPP
#define EE_GAMINGCLAYER_HPP

#include "base.hpp"

namespace EE { namespace Gaming {

class cMap;

class cLayer {
	public:
		typedef std::map<std::string, std::string> PropertiesMap;

		virtual ~cLayer();

		virtual void Draw( const eeVector2f& Offset = eeVector2f(0,0) ) = 0;

		virtual void Update() = 0;

		const Uint32& Flags() const;

		Uint32 FlagGet( const Uint32& Flag );

		void FlagSet( const Uint32& Flag );

		void FlagClear( const Uint32& Flag );

		const Uint32& Type() const;

		cMap * Map() const;

		const eeVector2f& Offset() const;

		void Offset( const eeVector2f& offset );

		const std::string& Name() const;

		const Uint32& Id() const;

		void AddProperty( std::string Text, std::string Value );

		void EditProperty( std::string Text, std::string Value );

		void RemoveProperty( std::string Text );

		void ClearProperties();

		PropertiesMap& GetProperties();
	protected:
		friend class cMap;

		cMap *			mMap;
		Uint32			mType;
		Uint32			mFlags;
		eeVector2f		mOffset;
		Uint32			mNameHash;
		std::string		mName;
		PropertiesMap	mProperties;

		cLayer( cMap * map, Uint32 type, Uint32 flags, std::string name = "", eeVector2f offset = eeVector2f(0,0) );
};

}}

#endif
