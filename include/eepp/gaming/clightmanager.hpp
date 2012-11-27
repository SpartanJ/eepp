#ifndef EE_GAMINGCLIGHTMANAGER_HPP
#define EE_GAMINGCLIGHTMANAGER_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/clight.hpp>
#include <list>

namespace EE { namespace Gaming {

class cMap;

class EE_API cLightManager {
	public:
		typedef std::list<cLight*> LightsList;

		cLightManager( cMap * Map, bool ByVertex );

		virtual ~cLightManager();

		virtual void Update();

		void AddLight( cLight * Light );

		void RemoveLight( const eeVector2f& OverPos );

		void RemoveLight( cLight * Light );

		Uint32 Count();

		const eeColorA * GetTileColor( const eeVector2i& TilePos );

		const eeColorA * GetTileColor( const eeVector2i& TilePos, const Uint32& Vertex );

		eeColorA GetColorFromPos( const eeVector2f& Pos );

		const bool& IsByVertex() const;

		LightsList& GetLights();

		cLight * GetLightOver( const eeVector2f& OverPos, cLight * LightCurrent = NULL );
	protected:
		cMap *				mMap;
		Int32				mNumVertex;
		eeColorA****		mTileColors;
		LightsList			mLights;
		bool				mIsByVertex;

		void AllocateColors();

		void DeallocateColors();

		void DestroyLights();

		virtual void UpdateByVertex();

		virtual void UpdateByTile();
};

}}

#endif
