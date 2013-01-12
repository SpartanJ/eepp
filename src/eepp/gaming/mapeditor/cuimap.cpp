#include <eepp/gaming/mapeditor/cuimap.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cUIMap::cUIMap( const cUIComplexControl::CreateParams& Params, cMap * Map ) :
	cUIComplexControl( Params ),
	mMap( Map ),
	mEditingMode( 0 ),
	mEditingObjMode( INSERT_OBJECT ),
	mAddLight( NULL ),
	mSelLight( NULL ),
	mClampToTile(true),
	mObjRECTEditing(false),
	mObjPolyEditing(false)
{
	if ( NULL == Map ) {
		mMap = eeNew( cMap, () );
	}

	mMap->SetDrawCallback( cb::Make0( this, &cUIMap::MapDraw ) );
}

cUIMap::~cUIMap() {
	eeSAFE_DELETE( mMap );
}

cMap * cUIMap::Map() const {
	return mMap;
}

void cUIMap::Draw() {
	cUIComplexControl::Draw();

	if ( NULL != mMap ) {
		mMap->Draw();
	}
}

void cUIMap::UpdateScreenPos() {
	cUIComplexControl::UpdateScreenPos();

	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
	}
}

void cUIMap::Update() {
	cUIComplexControl::Update();

	if ( NULL != mMap ) {
		mMap->Update();

		if ( mEnabled && mVisible && IsMouseOver() ) {
			Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

			if ( EDITING_LIGHT == mEditingMode ) {
				if ( NULL != mSelLight ) {
					if ( Flags & EE_BUTTONS_WUWD ) {
						if ( Flags & EE_BUTTON_WUMASK ) {
							mSelLight->Radius( mSelLight->Radius() + 10 );
						} else if ( Flags & EE_BUTTON_WDMASK ) {
							mSelLight->Radius( mSelLight->Radius() - 10 );
						}

						if ( mLightRadiusChangeCb.IsSet() )
							mLightRadiusChangeCb( mSelLight );
					} else if ( Flags & EE_BUTTON_RMASK ) {
						if ( mSelLight == mAddLight ) {
							mMap->GetLightManager()->RemoveLight( mAddLight );

							eeSAFE_DELETE( mAddLight );

							mSelLight = NULL;
						} else if ( NULL != mSelLight ) {
							if ( mSelLight->GetAABB().Contains( mMap->GetMouseMapPosf() ) ) {
								mMap->GetLightManager()->RemoveLight( mSelLight );

								eeSAFE_DELETE( mSelLight );
							}
						}
					} else if ( Flags & EE_BUTTON_LMASK ) {
						if ( mSelLight == mAddLight ) {
							mAddLight = NULL;
						} else {
							TryToSelectLight();
						}
					}

					Flags = cUIManager::instance()->GetInput()->PressTrigger();

					if ( Flags & EE_BUTTON_MMASK ) {
						mSelLight->Position( mMap->GetMouseMapPosf() );
					}
				} else {
					if ( Flags & EE_BUTTON_LMASK ) {
						TryToSelectLight();
					}
				}
			} else if ( EDITING_OBJECT == mEditingMode ) {
				ManageObject( Flags );
			}
		}
	}
}

eeVector2f cUIMap::GetMouseMapPos() {
	eeVector2f mp( mMap->GetMouseMapPosf() );

	if ( mClampToTile ) {
		eeVector2i mpc( mMap->GetTileCoords( mMap->GetMouseTilePos() + 1 ) );
		mp = eeVector2f( mpc.x, mpc.y );
	}

	return mp;
}

void cUIMap::ManageObject( Uint32 Flags ) {
	Uint32 PFlags = cUIManager::instance()->GetInput()->PressTrigger();

	switch ( mEditingObjMode )
	{
		case INSERT_OBJECT:
		{
			if ( PFlags & EE_BUTTON_LMASK ) {
				eeVector2f mp( GetMouseMapPos() );

				if ( !mObjRECTEditing ) {
					mObjRECTEditing = true;
					mObjRECT		= eeRectf( mp, eeSizef(0,0) );
				} else {
					if ( mObjRECT.Pos().x < mp.x && mObjRECT.Pos().y < mp.y ) {
						mObjRECT		= eeRectf( mObjRECT.Pos(), eeSizef( mp - mObjRECT.Pos() ) );
					}
				}
			}

			if ( Flags & EE_BUTTON_LMASK ){
				if ( mObjRECTEditing ) {
					mAddObjectCallback( GAMEOBJECT_TYPE_OBJECT, eePolygon2f( mObjRECT ) );
					mObjRECTEditing = false;
				}
			}

			break;
		}
		case INSERT_POLYLINE:
		case INSERT_POLYGON:
		{
			if ( Flags & EE_BUTTON_LMASK ) {
				mObjPoly.PushBack( GetMouseMapPos() );
			} else if ( Flags & EE_BUTTON_RMASK ) {
				mAddObjectCallback( ( INSERT_POLYGON == mEditingObjMode ) ? GAMEOBJECT_TYPE_POLYGON : GAMEOBJECT_TYPE_POLYLINE, mObjPoly );

				mObjPoly.Clear();
			}

			break;
		}
		default:
		{
		}
	}
}

void cUIMap::TryToSelectLight() {
	cLight * tLight = mSelLight;
	mSelLight = mMap->GetLightManager()->GetLightOver( mMap->GetMouseMapPosf(), mSelLight );

	if ( NULL != mSelLight && mSelLight != tLight ) {
		if ( mLightSelCb.IsSet() )
			mLightSelCb( mSelLight );
	}
}

void cUIMap::OnSizeChange() {
	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
		mMap->ViewSize( mSize );
	}

	cUIComplexControl::OnSizeChange();
}

Uint32 cUIMap::OnMouseMove( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( NULL != mMap ) {
		if ( EDITING_LIGHT == mEditingMode && NULL != mAddLight ) {
			mAddLight->Position( mMap->GetMouseMapPosf() );
		}
	}

	return cUIComplexControl::OnMouseMove( Pos, Flags );
}

void cUIMap::AddLight( cLight * Light ) {
	if ( NULL != mMap->GetLightManager() ) {
		if ( NULL != mAddLight ) {
			mMap->GetLightManager()->RemoveLight( mAddLight );

			eeSAFE_DELETE( mAddLight );
		}

		mAddLight = Light;

		mSelLight = Light;

		mMap->GetLightManager()->AddLight( Light );

		if ( mLightSelCb.IsSet() )
			mLightSelCb( mSelLight );
	}
}

void cUIMap::MapDraw() {
	if ( EDITING_LIGHT == mEditingMode ) {
		if ( NULL != mSelLight ) {
			mP.SetColor( eeColorA( 255, 0, 0, (Uint8)mAlpha ) );

			eeVector2f Pos( mSelLight->GetAABB().Left, mSelLight->GetAABB().Top );
			eeAABB AB( mSelLight->GetAABB() );

			mP.FillMode( EE_DRAW_LINE );
			mP.DrawRectangle( eeRectf( Pos, AB.Size() ) );
		}
	} else if ( EDITING_OBJECT == mEditingMode ) {
		switch ( mEditingObjMode ) {
			case INSERT_OBJECT:
			{
				if ( mObjRECTEditing ) {
					mP.FillMode( EE_DRAW_FILL );
					mP.SetColor( eeColorA( 100, 100, 100, 20 ) );
					mP.DrawRectangle( mObjRECT );

					mP.FillMode( EE_DRAW_LINE );
					mP.SetColor( eeColorA( 255, 0, 0, 200 ) );
					mP.DrawRectangle( mObjRECT );
				}

				break;
			}
			case INSERT_POLYGON:
			{
				mP.FillMode( EE_DRAW_FILL );
				mP.SetColor( eeColorA( 50, 50, 50, 50 ) );
				mP.DrawPolygon( mObjPoly );

				mP.FillMode( EE_DRAW_LINE );
				mP.SetColor( eeColorA( 255, 0, 0, 200 ) );
				mP.DrawPolygon( mObjPoly );

				eePolygon2f polyN( mObjPoly );
				polyN.PushBack( GetMouseMapPos() );

				mP.FillMode( EE_DRAW_FILL );
				mP.SetColor( eeColorA( 100, 100, 100, 100 ) );
				mP.DrawPolygon( polyN );

				mP.FillMode( EE_DRAW_LINE );
				mP.SetColor( eeColorA( 255, 255, 0, 200 ) );
				mP.DrawPolygon( polyN );

				break;
			}
			case INSERT_POLYLINE:
			{
				mP.FillMode( EE_DRAW_LINE );
				mP.SetColor( eeColorA( 255, 0, 0, 200 ) );
				mP.DrawPolygon( mObjPoly );

				eePolygon2f polyN( mObjPoly );
				polyN.PushBack( GetMouseMapPos() );

				mP.FillMode( EE_DRAW_LINE );
				mP.SetColor( eeColorA( 255, 255, 0, 200 ) );
				mP.DrawPolygon( polyN );

				break;
			}
		}
	}
}

void cUIMap::EditingLights( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_LIGHT : 0;

	if ( editing && NULL != mMap->GetLightManager() && NULL != mAddLight ) {
		mMap->GetLightManager()->RemoveLight( mAddLight );

		mAddLight = NULL;
	}
}

bool cUIMap::EditingLights() {
	return EDITING_LIGHT == mEditingMode;
}

void cUIMap::EditingObjects( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_OBJECT : 0;
}

bool cUIMap::EditingObjects() {
	return EDITING_OBJECT == mEditingMode;
}

void cUIMap::EditingDisabled() {
	mEditingMode = 0;
}

cLight * cUIMap::GetSelectedLight() {
	return mSelLight;
}

cLight * cUIMap::GetAddLight() {
	return mAddLight;
}

void cUIMap::SetLightSelectCb( LightSelectCb Cb ) {
	mLightSelCb = Cb;
}

void cUIMap::SetLightRadiusChangeCb( LightRadiusChangeCb Cb ) {
	mLightRadiusChangeCb = Cb;
}

void cUIMap::SetAddObjectCallback( ObjAddCb Cb ) {
	mAddObjectCallback = Cb;
}

void cUIMap::ClearLights() {
	mSelLight = NULL;
	mAddLight = NULL;
}

void cUIMap::OnAlphaChange() {
	cUIComplexControl::OnAlphaChange();

	if ( NULL != mMap ) {
		mMap->BackAlpha( (Uint8)mAlpha );
	}
}

void cUIMap::ClampToTile( const bool& clamp ) {
	mClampToTile = clamp;
}

const bool& cUIMap::ClampToTile() const {
	return mClampToTile;
}

void cUIMap::EditingObjMode( EDITING_OBJ_MODE mode ) {
	mObjPoly.Clear();

	mEditingObjMode = mode;
}

}}}
