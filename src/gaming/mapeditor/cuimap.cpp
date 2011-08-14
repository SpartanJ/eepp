#include "cuimap.hpp"
#include "../../ui/cuimanager.hpp"

namespace EE { namespace Gaming { namespace MapEditor {

cUIMap::cUIMap( const cUIComplexControl::CreateParams& Params, cMap * Map ) :
	cUIComplexControl( Params ),
	mMap( Map ),
	mEditingLights( false ),
	mAddLight( NULL ),
	mSelLight( NULL )
{
	if ( NULL == Map ) {
		mMap = eeNew( cMap, () );
		mMap->SetDrawCallback( cb::Make0( this, &cUIMap::MapDraw ) );
	}
}

cUIMap::~cUIMap() {
	eeSAFE_DELETE( mMap );
}

cMap * cUIMap::Map() const {
	return mMap;
}

void cUIMap::Draw() {
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
	if ( NULL != mMap ) {
		mMap->Update();

		if ( mEnabled && mVisible && mEditingLights ) {
			if ( IsMouseOver() ) {
				Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

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
							if ( Contains( mSelLight->GetAABB(), mMap->GetMouseMapPosf() ) ) {
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
			}
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
		if ( mEditingLights && NULL != mAddLight ) {
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

void cUIMap::EditingLights( const bool& editing ) {
	mEditingLights = editing;

	if ( editing && NULL != mMap->GetLightManager() && NULL != mAddLight ) {
		mMap->GetLightManager()->RemoveLight( mAddLight );

		mAddLight = NULL;
	}
}

void cUIMap::MapDraw() {
	if ( mEditingLights && NULL != mSelLight ) {
		if ( Intersect( mMap->GetViewAreaAABB(), mSelLight->GetAABB() ) ) {
			cPrimitives P;
			P.SetColor( eeColorA( 255, 0, 0, 225 ) );

			eeVector2f Pos( mSelLight->GetAABB().Left + mMap->OffsetFixed().x, mSelLight->GetAABB().Top + mMap->OffsetFixed().y );
			eeAABB AB( mSelLight->GetAABB() );
			eeSizef Size( AB.Size() );

			P.DrawRectangle( Pos.x, Pos.y, Size.Width(), Size.Height(), 0, 1, EE_DRAW_LINE );
		}
	}
}

const bool& cUIMap::EditingLights() {
	return mEditingLights;
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

void cUIMap::ClearLights() {
	mSelLight = NULL;
	mAddLight = NULL;
}

}}}
