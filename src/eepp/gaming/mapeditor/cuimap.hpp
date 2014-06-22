#ifndef EE_GAMINGCUIMAP_HPP
#define EE_GAMINGCUIMAP_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/cuimessagebox.hpp>
#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/clightmanager.hpp>
#include <eepp/gaming/clight.hpp>
#include <eepp/graphics/primitives.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming {

class cGameObjectObject;

namespace MapEditor {

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
		typedef cb::Callback2<void, Uint32, Polygon2f> ObjAddCb;
		typedef cb::Callback2<cUIMessageBox*, const String&, const String&> AlertCb;
		typedef cb::Callback0<void> OnMapLoadCb;
		typedef cb::Callback0<void> UpdateScrollCb;

		cUIMap( const cUIComplexControl::CreateParams& Params, cUITheme * Theme, cMap * Map = NULL );

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

		void SetAlertCb( AlertCb Cb );

		void SetUpdateScrollCb( UpdateScrollCb Cb );

		void ClearLights();

		void ClampToTile( const bool& clamp );

		const bool& ClampToTile() const;

		void EditingObjMode( EDITING_OBJ_MODE mode );

		void CurLayer( cLayer * layer );

		void CreateObjPopUpMenu();

		void SetTileBox( cUITextBox * tilebox );

		void ReplaceMap( cMap * newMap );
	protected:		
		enum EDITING_MODE {
			EDITING_LIGHT = 1,
			EDITING_OBJECT
		};

		cMap *				mMap;
		cLayer *			mCurLayer;
		Uint32				mEditingMode;
		Primitives			mP;

		Uint32				mEditingObjMode;

		cLight *			mAddLight;
		cLight *			mSelLight;

		LightSelectCb		mLightSelCb;
		LightRadiusChangeCb	mLightRadiusChangeCb;
		ObjAddCb			mAddObjectCallback;

		bool				mClampToTile;

		bool				mObjRECTEditing;
		Rectf				mObjRECT;

		bool				mObjPolyEditing;
		Polygon2f			mObjPoly;

		bool				mObjDragging;

		cGameObjectObject *	mSelObj;
		Vector2f			mObjDragDist;

		AlertCb				mAlertCb;
		cUITheme *			mTheme;

		Uint32				mSelPointIndex;
		Rectf				mSelPointRect;
		bool				mSelPoint;

		cUITextBox *		mTileBox;
		Vector2i			mLastMouseTilePos;

		UpdateScrollCb		mUpdateScrollCb;

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual Uint32 OnMouseMove( const Vector2i& Pos, const Uint32 Flags );

		virtual void OnSizeChange();

		virtual void UpdateScreenPos();

		virtual void OnAlphaChange();

		virtual Uint32 OnDrag( const Vector2i& Pos );

		void ObjItemClick( const cUIEvent * Event );

		void MapDraw();

		void TryToSelectLight();

		void ManageObject( Uint32 Flags );

		Vector2f GetMouseMapPos();

		void SelectPolyObj();

		void SelectPolyPoint();

		void SetPointRect( Vector2f p );

		void DragPoly( Uint32 Flags, Uint32 PFlags );
};

}}}

#endif
