#ifndef EE_GAMINGCOBJECTLAYER_HPP
#define EE_GAMINGCOBJECTLAYER_HPP

#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/gameobject.hpp>
#include <list>

namespace EE { namespace Gaming {

class TileMap;

class EE_API MapObjectLayer : public MapLayer {
	public:
		enum SEARCH_TYPE {
			SEARCH_OBJECT = 1,
			SEARCH_POLY,
			SEARCH_ALL
		};

		typedef std::list<GameObject*> ObjList;

		virtual ~MapObjectLayer();

		virtual void Draw( const Vector2f &Offset = Vector2f(0,0) );

		virtual void Update();

		virtual void AddGameObject( GameObject * obj );

		virtual void RemoveGameObject( GameObject * obj );

		virtual void RemoveGameObject( const Vector2i& pos );

		virtual GameObject * GetObjectOver( const Vector2i& pos, SEARCH_TYPE type = SEARCH_ALL );

		virtual Uint32 GetObjectCount() const;
	protected:
		friend class TileMap;

		ObjList		mObjects;

		MapObjectLayer( TileMap * map, Uint32 flags, std::string name = "", Vector2f offset = Vector2f(0,0) );

		void AllocateLayer();

		void DeallocateLayer();

		ObjList& GetObjectList();
};

}}

#endif
