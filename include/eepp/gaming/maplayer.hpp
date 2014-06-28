#ifndef EE_GAMINGCLAYER_HPP
#define EE_GAMINGCLAYER_HPP

#include <eepp/gaming/base.hpp>

namespace EE { namespace Gaming {

class TileMap;

class EE_API MapLayer {
	public:
		typedef std::map<std::string, std::string> PropertiesMap;

		virtual ~MapLayer();

		virtual void Draw( const Vector2f& Offset = Vector2f(0,0) ) = 0;

		virtual void Update() = 0;

		const Uint32& Flags() const;

		Uint32 FlagGet( const Uint32& Flag );

		void FlagSet( const Uint32& Flag );

		void FlagClear( const Uint32& Flag );

		const Uint32& Type() const;

		TileMap * Map() const;

		const Vector2f& Offset() const;

		void Offset( const Vector2f& offset );

		void Name( const std::string& name );

		const std::string& Name() const;

		const Uint32& Id() const;

		void AddProperty( std::string Text, std::string Value );

		void EditProperty( std::string Text, std::string Value );

		void RemoveProperty( std::string Text );

		void ClearProperties();

		PropertiesMap& GetProperties();

		void Visible( const bool& visible );

		bool Visible();

		bool LightsEnabled();

		void LightsEnabled( const bool& enabled );
	protected:
		friend class TileMap;

		TileMap *			mMap;
		Uint32			mType;
		Uint32			mFlags;
		Vector2f		mOffset;
		Uint32			mNameHash;
		std::string		mName;
		PropertiesMap	mProperties;

		MapLayer( TileMap * map, Uint32 type, Uint32 flags, std::string name = "", Vector2f offset = Vector2f(0,0) );
};

}}

#endif
