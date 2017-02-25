#include <eepp/ui/uiskinsimple.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace UI {

UISkinSimple::UISkinSimple( const std::string& Name ) :
	UISkin( Name, SkinSimple )
{
	for ( Int32 i = 0; i < UISkinState::StateCount; i++ )
		mSubTexture[ i ] = NULL;

	setSkins();
}

UISkinSimple::~UISkinSimple() {
}

void UISkinSimple::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	SubTexture * tSubTexture = mSubTexture[ State ];
	mTempColor		= mColor[ State ];

	if ( NULL != tSubTexture ) {
		tSubTexture->setDestSize( Sizef( Width, Height ) );

		if ( mTempColor.Alpha != Alpha ) {
			mTempColor.Alpha = (Uint8)( (Float)mTempColor.Alpha * ( (Float)Alpha / 255.f ) );
		}

		tSubTexture->draw( X, Y, mTempColor );

		tSubTexture->resetDestSize();
	}
}

void UISkinSimple::setSkin( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	std::string Name( mName + "_" + UISkin::getSkinStateName( State ) );

	mSubTexture[ State ] = TextureAtlasManager::instance()->getSubTextureByName( Name );
}

SubTexture * UISkinSimple::getSubTexture( const Uint32& State ) const {
	eeASSERT ( State < UISkinState::StateCount );

	return mSubTexture[ State ];
}

void UISkinSimple::stateNormalToState( const Uint32& State ) {
	if ( NULL == mSubTexture[ State ] )
		mSubTexture[ State ] = mSubTexture[ UISkinState::StateNormal ];
}

UISkinSimple * UISkinSimple::clone( const std::string& NewName, const bool& CopyColorsState ) {
	UISkinSimple * SkinS = eeNew( UISkinSimple, ( NewName ) );

	if ( CopyColorsState ) {
		SkinS->mColorDefault = mColorDefault;

		memcpy( &SkinS->mColor[0], &mColor[0], UISkinState::StateCount * sizeof(ColorA) );
	}

	memcpy( &SkinS->mSubTexture[0], &mSubTexture[0], UISkinState::StateCount * sizeof(SubTexture*) );

	return SkinS;
}

UISkin * UISkinSimple::clone() {
	return clone( mName, true );
}

Sizei UISkinSimple::getSize( const Uint32 & state ) {
	if ( NULL != mSubTexture[ state ] ) {
		return mSubTexture[ state ]->getSize();
	}

	return Sizei();
}

}}
