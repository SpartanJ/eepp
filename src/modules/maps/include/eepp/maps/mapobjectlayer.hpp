#ifndef EE_MAPS_COBJECTLAYER_HPP
#define EE_MAPS_COBJECTLAYER_HPP

#include <eepp/maps/gameobject.hpp>
#include <eepp/maps/maplayer.hpp>
#include <list>

namespace EE { namespace Maps {

class TileMap;

class EE_MAPS_API MapObjectLayer : public MapLayer {
  public:
	enum SEARCH_TYPE { SEARCH_OBJECT = 1, SEARCH_POLY, SEARCH_ALL };

	typedef std::list<GameObject*> ObjList;

	virtual ~MapObjectLayer();

	virtual void draw( const Vector2f& Offset = Vector2f( 0, 0 ) );

	virtual void update( const Time& dt );

	virtual void addGameObject( GameObject* obj );

	virtual void removeGameObject( GameObject* obj );

	virtual void removeGameObject( const Vector2i& pos );

	virtual GameObject* getObjectOver( const Vector2i& pos, SEARCH_TYPE type = SEARCH_ALL );

	virtual Uint32 getObjectCount() const;

  protected:
	friend class TileMap;

	ObjList mObjects;

	MapObjectLayer( TileMap* map, Uint32 flags, std::string name = "",
					Vector2f offset = Vector2f( 0, 0 ) );

	void allocateLayer();

	void deallocateLayer();

	ObjList& getObjectList();
};

}} // namespace EE::Maps

#endif
