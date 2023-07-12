#ifndef EE_MAPS_CLAYER_HPP
#define EE_MAPS_CLAYER_HPP

#include <eepp/maps/base.hpp>

namespace EE { namespace Maps {

class TileMap;

class EE_MAPS_API MapLayer {
  public:
	typedef std::map<std::string, std::string> PropertiesMap;

	virtual ~MapLayer();

	virtual void draw( const Vector2f& Offset = Vector2f( 0, 0 ) ) = 0;

	virtual void update( const Time& dt ) = 0;

	const Uint32& getFlags() const;

	Uint32 getFlag( const Uint32& Flag );

	void setFlag( const Uint32& Flag );

	void clearFlag( const Uint32& Flag );

	const Uint32& getType() const;

	TileMap* getMap() const;

	const Vector2f& getOffset() const;

	void setOffset( const Vector2f& offset );

	void setName( const std::string& name );

	const std::string& getName() const;

	const String::HashType& getId() const;

	void addProperty( std::string Text, std::string Value );

	void editProperty( std::string Text, std::string Value );

	void removeProperty( std::string Text );

	void clearProperties();

	PropertiesMap& getProperties();

	void setVisible( const bool& visible );

	bool isVisible();

	bool getLightsEnabled();

	void setLightsEnabled( const bool& enabled );

  protected:
	friend class TileMap;

	TileMap* mMap;
	Uint32 mType;
	Uint32 mFlags;
	Vector2f mOffset;
	String::HashType mNameHash;
	std::string mName;
	PropertiesMap mProperties;

	MapLayer( TileMap* map, Uint32 type, Uint32 flags, std::string name = "",
			  Vector2f offset = Vector2f( 0, 0 ) );
};

}} // namespace EE::Maps

#endif
