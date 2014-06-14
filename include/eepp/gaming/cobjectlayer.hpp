#ifndef EE_GAMINGCOBJECTLAYER_HPP
#define EE_GAMINGCOBJECTLAYER_HPP

#include <eepp/gaming/clayer.hpp>
#include <eepp/gaming/cgameobject.hpp>
#include <list>

namespace EE { namespace Gaming {

class cMap;

class EE_API cObjectLayer : public cLayer {
	public:
		enum SEARCH_TYPE {
			SEARCH_OBJECT = 1,
			SEARCH_POLY,
			SEARCH_ALL
		};

		typedef std::list<cGameObject*> ObjList;

		virtual ~cObjectLayer();

		virtual void Draw( const Vector2f &Offset = Vector2f(0,0) );

		virtual void Update();

		virtual void AddGameObject( cGameObject * obj );

		virtual void RemoveGameObject( cGameObject * obj );

		virtual void RemoveGameObject( const Vector2i& pos );

		virtual cGameObject * GetObjectOver( const Vector2i& pos, SEARCH_TYPE type = SEARCH_ALL );

		virtual Uint32 GetObjectCount() const;
	protected:
		friend class cMap;

		ObjList		mObjects;

		cObjectLayer( cMap * map, Uint32 flags, std::string name = "", Vector2f offset = Vector2f(0,0) );

		void AllocateLayer();

		void DeallocateLayer();

		ObjList& GetObjectList();
};

}}

#endif
