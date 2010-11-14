#include "cuitheme.hpp"
#include "cuiskinsimple.hpp"
#include "cuiskincomplex.hpp"
#include "../graphics/ctexturefactory.hpp"
#include "../graphics/cshapegroupmanager.hpp"

namespace EE { namespace UI {

static const char * UI_THEME_ELEMENTS[] = {
	"control",
	"button",
	"textinput",
	"checkbox",
	"checkbox_active",
	"checkbox_inactive",
	"button",
	"radiobutton",
	"radiobutton_active",
	"radiobutton_inactive",
	"hslider",
	"hslider_bg",
	"hslider_button",
	"vslider",
	"vslider_bg",
	"vslider_button",
	"spinbox",
	"spinbox_input",
	"spinbox_btnup",
	"spinbox_btndown",
	"hscrollbar",
	"hscrollbar_slider",
	"hscrollbar_bg",
	"hscrollbar_button",
	"hscrollbar_btnup",
	"hscrollbar_btndown",
	"vscrollbar",
	"vscrollbar_slider",
	"vscrollbar_bg",
	"vscrollbar_button",
	"vscrollbar_btnup",
	"vscrollbar_btndown",
	"progressbar",
	"progressbar_filler",
	"listbox",
	"listboxitem"
};

cUITheme * cUITheme::LoadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt ) {
	Uint32 i;
	bool Found;
	std::string Element;
	std::string RPath( Path );

	DirPathAddSlashAtEnd( RPath );

	if ( !IsDirectory( RPath ) )
		return NULL;

	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	cShapeGroup * tSG = eeNew( cShapeGroup, ( NameAbbr ) );

	cUITheme * tTheme = eeNew( cUITheme, ( Name, NameAbbr ) );

	Uint32 Count = eeARRAY_SIZE( UI_THEME_ELEMENTS );

	for ( i = 0; i < Count; i++ ) {
		Uint32 IsComplex = 0;

		Element = std::string( NameAbbr + "_" + UI_THEME_ELEMENTS[i] );

		Found 	= SearchFilesOfElement( tSG, RPath, Element, IsComplex, ImgExt );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	if ( tSG->Count() )
		cShapeGroupManager::instance()->Add( tSG );
	else
		eeSAFE_DELETE( tSG );

	for ( i = 0; i < ElemFound.size(); i++ ) {
		if ( ElemType[i] )
			tTheme->Add( eeNew( cUISkinComplex, ( ElemFound[i] ) ) );
		else
			tTheme->Add( eeNew( cUISkinSimple, ( ElemFound[i] ) ) );
	}

	return tTheme;
}

cUITheme * cUITheme::LoadFromShapeGroup( cShapeGroup * ShapeGroup, const std::string& Name, const std::string NameAbbr ) {
	Uint32 i;
	bool Found;
	std::string Element;
	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	cUITheme * tTheme = eeNew( cUITheme, ( Name, NameAbbr ) );

	Uint32 Count = eeARRAY_SIZE( UI_THEME_ELEMENTS );

	for ( i = 0; i < Count; i++ ) {
		Uint32 IsComplex = 0;

		Element = std::string( NameAbbr + "_" + UI_THEME_ELEMENTS[i] );

		Found 	= SearchFilesInGroup( ShapeGroup, Element, IsComplex );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	for ( i = 0; i < ElemFound.size(); i++ ) {
		if ( ElemType[i] )
			tTheme->Add( eeNew( cUISkinComplex, ( ElemFound[i] ) ) );
		else
			tTheme->Add( eeNew( cUISkinSimple, ( ElemFound[i] ) ) );
	}

	return tTheme;
}

bool cUITheme::SearchFilesInGroup( cShapeGroup * SG, std::string Element, Uint32& IsComplex ) {
	bool Found = false;
	Uint32 i = 0, s = 0;
	std::string ElemPath;
	std::string ElemFullPath;
	std::string ElemName;
	IsComplex = false;

	// Search Complex Skin
	for ( i = 0; i < cUISkinState::StateCount; i++ ) {
		for ( s = 0; s < cUISkinComplex::SideCount; s++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i ) + "_" + cUISkinComplex::GetSideSuffix( s );

			if ( SG->GetByName( ElemName ) ) { // only load PNG's FTM
				IsComplex = true;
				Found = true;
				break;
			}
		}
	}

	// Seach Simple Skin
	if ( !IsComplex ) {
		for ( i = 0; i < cUISkinState::StateCount; i++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i );

			if ( SG->GetByName( ElemName ) ) { // only load PNG's FTM
				Found = true;
				break;
			}
		}
	}

	return Found;
}

bool cUITheme::SearchFilesOfElement( cShapeGroup * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt ) {
	bool Found = false;
	Uint32 i = 0, s = 0;
	std::string ElemPath;
	std::string ElemFullPath;
	std::string ElemName;
	IsComplex = false;

	// Search Complex Skin
	for ( i = 0; i < cUISkinState::StateCount; i++ ) {
		for ( s = 0; s < cUISkinComplex::SideCount; s++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i ) + "_" + cUISkinComplex::GetSideSuffix( s );
			ElemPath = Path + ElemName;
			ElemFullPath = ElemPath + "." + ImgExt;

			if ( FileExists( ElemFullPath ) ) { // only load PNG's FTM
				SG->Add( eeNew( cShape, ( cTextureFactory::instance()->Load( ElemFullPath ), ElemName ) ) );

				IsComplex = true;
				Found = true;
			}
		}
	}

	// Seach Simple Skin
	if ( !IsComplex ) {
		for ( i = 0; i < cUISkinState::StateCount; i++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i );
			ElemPath = Path + ElemName;
			ElemFullPath = ElemPath + "." + ImgExt;

			if ( FileExists( ElemFullPath ) ) { // only load PNG's FTM
				SG->Add( eeNew( cShape, ( cTextureFactory::instance()->Load( ElemFullPath ), ElemName ) ) );

				Found = true;
			}
		}
	}

	return Found;
}

cUITheme::cUITheme( const std::string& Name, const std::string& Abbr, cFont * DefaultFont ) :
	tResourceManager<cUISkin> ( false ),
	mName( Name ),
	mNameHash( MakeHash( mName ) ),
	mAbbr( Abbr ),
	mFont( DefaultFont )
{
}

cUITheme::~cUITheme() {

}

const std::string& cUITheme::Name() const {
	return mName;
}

void cUITheme::Name( const std::string& name ) {
	mName = name;
	mNameHash = MakeHash( mName );
}

const Uint32& cUITheme::Id() const {
	return mNameHash;
}

const std::string& cUITheme::Abbr() const {
	return mAbbr;
}

cUISkin * cUITheme::Add( cUISkin * Resource ) {
	Resource->Theme( this );

	return tResourceManager<cUISkin>::Add( Resource );
}

void cUITheme::DefaultFont( cFont * Font ) {
	mFont = Font;
}

cFont * cUITheme::DefaultFont() const {
	return mFont;
}

}}
