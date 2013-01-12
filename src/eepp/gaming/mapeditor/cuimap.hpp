#ifndef EE_GAMINGCUIMAP_HPP
#define EE_GAMINGCUIMAP_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/clightmanager.hpp>
#include <eepp/gaming/clight.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class EE_API cUIMap : public cUIComplexControl {
	public:
		enum EDITING_OBJ_MODE {
			SELECT_OBJECTS,
			EDIT_POLYGONS,
			INSERT_OBJECT,
			INSERT_POLYGON,
			INSERT_POLYLINE
		};

		typedef cb::Callback1<void, cLight *> LightSelectCb;
		typedef cb::Callback1<void, cLight *> LightRadiusChangeCb;
		typedef cb::Callback2<void, Uint32, eePolygon2f> ObjAddCb;

		cUIMap( const cUIComplexControl::CreateParams& Params, cMap * Map = NULL );

		virtual ~cUIMap();

		virtual void Draw();

		virtual void Update();

		cMap * Map() const;

		void EditingLights( const bool& editing );

		bool EditingLights();

		void EditingObjects( const bool& editing );

		void EditingDisabled();

		bool EditingObjects();

		cLight * GetSelectedLight();

		cLight * GetAddLight();

		void AddLight( cLight * Light );

		void SetLightSelectCb( LightSelectCb Cb );

		void SetLightRadiusChangeCb( LightRadiusChangeCb Cb );

		void SetAddObjectCallback( ObjAddCb Cb );

		void ClearLights();

		void ClampToTile( const bool& clamp );

		const bool& ClampToTile() const;

		void EditingObjMode( EDITING_OBJ_MODE mode );
	protected:		
		enum EDITING_MODE {
			EDITING_LIGHT = 1,
			EDITING_OBJECT
		};

		cMap *				mMap;
		Uint32				mEditingMode;
		cPrimitives			mP;

		Uint32				mEditingObjMode;

		cLight *			mAddLight;
		cLight *			mSelLight;

		LightSelectCb		mLightSelCb;
		LightRadiusChangeCb	mLightRadiusChangeCb;
		ObjAddCb			mAddObjectCallback;

		bool				mClampToTile;

		bool				mObjRECTEditing;
		eeRectf				mObjRECT;

		bool				mObjPolyEditing;
		eePolygon2f			mObjPoly;

		virtual Uint32 OnMouseMove( const eeVector2i& Pos, const Uint32 Flags );

		virtual void OnSizeChange();

		virtual void UpdateScreenPos();

		virtual void OnAlphaChange();

		void MapDraw();

		void TryToSelectLight();

		void ManageObject( Uint32 Flags );

		eeVector2f GetMouseMapPos();
};

}}}

#endif
