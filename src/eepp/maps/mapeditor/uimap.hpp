#ifndef EE_GAMINGCUIMAP_HPP
#define EE_GAMINGCUIMAP_HPP

#include <eepp/maps/base.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/maplightmanager.hpp>
#include <eepp/maps/maplight.hpp>
#include <eepp/graphics/primitives.hpp>

using namespace EE::UI;

namespace EE { namespace Maps {

class GameObjectObject;

namespace Private {

class EE_API UIMap : public UIWindow {
	public:
		static UIMap * New( UITheme * Theme, TileMap * Map = NULL );

		enum EDITING_OBJ_MODE {
			SELECT_OBJECTS,
			EDIT_POLYGONS,
			INSERT_OBJECT,
			INSERT_POLYGON,
			INSERT_POLYLINE
		};

		typedef std::function<void( MapLight * )> LightSelectCb;
		typedef std::function<void( MapLight * )> LightRadiusChangeCb;
		typedef std::function<void( Uint32, Polygon2f )> ObjAddCb;
		typedef std::function<UIMessageBox*( const String&, const String& )> AlertCb;
		typedef std::function<void()> OnMapLoadCb;
		typedef std::function<void()> UpdateScrollCb;

		UIMap( UITheme * Theme, TileMap * Map = NULL );

		virtual ~UIMap();

		virtual void draw();

		virtual void scheduledUpdate( const Time& time );

		TileMap * Map() const;

		void setEditingLights( const bool& editing );

		bool isEditingLights();

		void setEditingObjects( const bool& editing );

		bool isEditingObjects();

		void editingDisable();

		MapLight * getSelectedLight();

		MapLight * getAddLight();

		void addLight( MapLight * Light );

		void setLightSelectCb( LightSelectCb Cb );

		void setLightRadiusChangeCb( LightRadiusChangeCb Cb );

		void setAddObjectCallback( ObjAddCb Cb );

		void setAlertCb( AlertCb Cb );

		void setUpdateScrollCb( UpdateScrollCb Cb );

		void clearLights();

		void setClampToTile( const bool& clamp );

		const bool& getClampToTile() const;

		void setEditingObjMode( EDITING_OBJ_MODE mode );

		void setCurLayer( MapLayer * layer );

		void createObjPopUpMenu();

		void setTileBox( UITextView * tilebox );

		void replaceMap( TileMap * newMap );
	protected:
		enum EDITING_MODE {
			EDITING_LIGHT = 1,
			EDITING_OBJECT
		};

		TileMap *			mMap;
		MapLayer *			mCurLayer;
		Uint32				mEditingMode;
		Primitives			mP;

		Uint32				mEditingObjMode;

		MapLight *			mAddLight;
		MapLight *			mSelLight;

		LightSelectCb		mLightSelCb;
		LightRadiusChangeCb	mLightRadiusChangeCb;
		ObjAddCb			mAddObjectCallback;

		bool				mClampToTile;

		bool				mObjRECTEditing;
		Rectf				mObjRECT;

		bool				mObjPolyEditing;
		Polygon2f			mObjPoly;

		bool				mObjDragging;

		GameObjectObject *	mSelObj;
		Vector2f			mObjDragDist;

		AlertCb				mAlertCb;
		UITheme *			mTheme;

		Uint32				mSelPointIndex;
		Rectf				mSelPointRect;
		bool				mSelPoint;

		UITextView *			mTileBox;
		Vector2i			mLastMouseTilePos;

		UpdateScrollCb		mUpdateScrollCb;

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

		virtual void onSizeChange();

		void onUpdateScreenPos();

		virtual void onAlphaChange();

		virtual Uint32 onDrag( const Vector2f& Pos , const Uint32 & flags );

		void objItemClick( const Event * Event );

		void mapDraw();

		void tryToSelectLight();

		void manageObject( Uint32 flags );

		Vector2f getMouseMapPos();

		void selectPolyObj();

		void selectPolyPoint();

		void setPointRect( Vector2f p );

		void dragPoly( Uint32 flags, Uint32 PFlags );
};

}}}

#endif
