#ifndef EE_MAPS_CGAMEOBJECTOBJECT_HPP
#define EE_MAPS_CGAMEOBJECTOBJECT_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobject.hpp>
#include <eepp/graphics/subtexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Maps {

class MapLayer;

class EE_API GameObjectObject : public GameObject {
	public:
		typedef std::map<std::string, std::string> PropertiesMap;

		GameObjectObject( Uint32 DataId, const Rectf& rect, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~GameObjectObject();

		virtual void draw();

		virtual Vector2f getPosition() const;

		virtual Sizei getSize();

		virtual void setPosition( Vector2f pos );

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type );

		virtual Uint32 getDataId();

		virtual void setDataId( Uint32 Id );

		void addProperty( std::string Text, std::string Value );

		void editProperty( std::string Text, std::string Value );

		void removeProperty( std::string Text );

		void setProperties( const PropertiesMap& prop );

		void clearProperties();

		PropertiesMap& getProperties();

		Uint32 getPropertyCount();

		const std::string& getName() const;

		void setName( const std::string& name );

		const std::string& getTypeName() const;

		void setTypeName( const std::string& type );

		virtual bool pointInside( const Vector2f& p );

		Polygon2f& getPolygon();

		const bool& isSelected() const;

		void setSelected( const bool& sel );

		virtual void setPolygonPoint( Uint32 index, Vector2f p );

		virtual GameObjectObject * clone();
	protected:
		Rectf			mRect;
		Polygon2f		mPoly;
		Vector2f		mPos;
		Uint32			mDataId;
		bool			mSelected;
		std::string		mName;
		std::string		mType;
		PropertiesMap	mProperties;
};

class GameObjectPolyData {
	public:
		std::string		Name;
		std::string		Type;
		GameObjectObject::PropertiesMap	Properties;
		Polygon2f		Poly;
};

}}

#endif
