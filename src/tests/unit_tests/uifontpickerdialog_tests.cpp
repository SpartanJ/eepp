#include "utest.hpp"
#include <algorithm>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Abstract;
using namespace EE::UI::Tools;

static std::string expectedStretchLabel( FontStretch stretch ) {
	switch ( stretch ) {
		case FontStretch::UltraCondensed:
			return "Ultra Condensed";
		case FontStretch::ExtraCondensed:
			return "Extra Condensed";
		case FontStretch::Condensed:
			return "Condensed";
		case FontStretch::SemiCondensed:
			return "Semi Condensed";
		case FontStretch::Normal:
			return "";
		case FontStretch::SemiExpanded:
			return "Semi Expanded";
		case FontStretch::Expanded:
			return "Expanded";
		case FontStretch::ExtraExpanded:
			return "Extra Expanded";
		case FontStretch::UltraExpanded:
			return "Ultra Expanded";
	}
	return "";
}

template <typename Predicate> static void pumpUntil( UISceneNode* sceneNode, Predicate predicate ) {
	for ( size_t i = 0; i < 1000 && !predicate(); i++ ) {
		sceneNode->update( Time::Zero );
		Sys::sleep( Milliseconds( 5 ) );
	}
}

class TestFontPickerDialog : public UIFontPickerDialog {
  public:
	static TestFontPickerDialog* New( Uint32 flags = DefaultFlags ) {
		return eeNew( TestFontPickerDialog, ( flags ) );
	}

	FontTrueTypeWeakPtr getPreviewFontHandle() const { return mPreviewFont; }
	void selectCustomSize( Uint32 size ) { selectSize( size ); }
	void releasePreviewFont() { clearPreviewFont(); }
	Uint32 getPreviewTextSize() const { return mPreviewText->getFontSize(); }
	Uint32 getPreviewInputSize() const { return mPreviewInput->getFontSize(); }
	String getDetails() const { return mDetailsText->getText(); }
	void setFontsForTest( std::vector<FontDesc> fonts ) { setFonts( std::move( fonts ) ); }
	void markSelectedFontExternalForTest() {
		FontDesc selectedFont = mSelection.font;
		mExternalFontKeys.insert( selectedFont.getFileKey() );
		updateFamilies();
		setSelectedFont( selectedFont );
	}

  protected:
	TestFontPickerDialog( Uint32 flags ) : UIFontPickerDialog( flags ) {}
};

UTEST( UIFontPickerDialog, PreselectsExternalFontPath ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );
	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	dialog->setSelectedFont( fontPath );

	EXPECT_STDSTREQ( fontPath, dialog->getSelection().font.path );
	EXPECT_FALSE( dialog->getSelection().font.family.empty() );
	EXPECT_FALSE( dialog->getFamilyList()->getSelection().isEmpty() );
	EXPECT_FALSE( dialog->getStyleList()->getSelection().isEmpty() );
	EXPECT_TRUE( dialog->getFamilyList()->getSelection().first().data().toString().find(
					 "[External]" ) != std::string::npos );

	UIFontPickerDialog* selectionDialog = UIFontPickerDialog::New();
	UIFontSelection selection;
	selection.font.path = fontPath;
	selectionDialog->setSelection( selection );

	EXPECT_STDSTREQ( fontPath, selectionDialog->getSelection().font.path );
	EXPECT_FALSE( selectionDialog->getSelection().font.family.empty() );
}

UTEST( UIFontPickerDialog, ExternalFontBecomesDistinctStyleOnFamilyNameCollision ) {
	const std::string managedFontPath =
		Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( managedFontPath ) );

	const std::string externalPath = Sys::getTempPath() + "UIFontPickerDialogExternal-" +
									 String::toString( Sys::getProcessID() ) + "." +
									 FileSystem::fileExtension( managedFontPath );
	ASSERT_TRUE( FileSystem::fileCopy( managedFontPath, externalPath ) );

	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	ResourceScope& resourceScope = *app.getUI()->getResourceScope();
	const std::string externalName(
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( externalPath ) ) );
	FontTrueTypePtr managedFont = FontTrueType::New( externalName, managedFontPath, resourceScope );
	ASSERT_TRUE( managedFont && managedFont->loaded() );
	const std::string family = managedFont->getInfo().family;
	ASSERT_FALSE( family.empty() );

	TestFontPickerDialog* dialog = TestFontPickerDialog::New();
	dialog->setSelectedFont( externalPath );
	pumpUntil( app.getUI(), [dialog] { return dialog->getButtonOK()->isEnabled(); } );

	EXPECT_STDSTREQ( externalPath, dialog->getSelection().font.path );
	EXPECT_FALSE( dialog->getFamilyList()->getSelection().isEmpty() );
	EXPECT_STDSTREQ( family, dialog->getFamilyList()->getSelection().first().data().toString() );
	EXPECT_TRUE( dialog->getStyleList()->getSelection().first().data().toString().find(
					 "[External:" ) != std::string::npos );

	size_t matchingFamilies = 0;
	Model* familyModel = dialog->getFamilyList()->getModel();
	ASSERT_TRUE( familyModel != nullptr );
	for ( size_t row = 0; row < familyModel->rowCount(); row++ ) {
		const std::string label = familyModel->index( row ).data().toString();
		matchingFamilies += label == family;
	}
	EXPECT_EQ( 1u, matchingFamilies );

	bool foundSystemStyle = false;
	bool foundExternalStyle = false;
	Model* styleModel = dialog->getStyleList()->getModel();
	ASSERT_TRUE( styleModel != nullptr );
	for ( size_t row = 0; row < styleModel->rowCount(); row++ ) {
		const std::string label = styleModel->index( row ).data().toString();
		foundSystemStyle |= label.find( "[External:" ) == std::string::npos;
		foundExternalStyle |= label.find( "[External:" ) != std::string::npos;
	}
	EXPECT_TRUE( foundSystemStyle );
	EXPECT_TRUE( foundExternalStyle );
	EXPECT_EQ( managedFont.get(), resourceScope.findFont( externalName ).get() );

	TestFontPickerDialog* hiddenStyleDialog =
		TestFontPickerDialog::New( UIFontPickerDialog::ShowSize );
	hiddenStyleDialog->setSelectedFont( externalPath );
	pumpUntil( app.getUI(),
			   [hiddenStyleDialog] { return hiddenStyleDialog->getButtonOK()->isEnabled(); } );
	EXPECT_STDSTREQ( externalPath, hiddenStyleDialog->getSelection().font.path );
	EXPECT_FALSE( hiddenStyleDialog->getStyleList()->getParent()->isVisible() );
	EXPECT_TRUE( hiddenStyleDialog->getFamilyList()->getSelection().first().data().toString().find(
					 "[External:" ) != std::string::npos );

	dialog->releasePreviewFont();
	hiddenStyleDialog->releasePreviewFont();
	FileSystem::fileRemove( externalPath );
}

UTEST( UIFontPickerDialog, PreviewFontsDoNotPopulateSceneFontCatalog ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const std::string koreanFontPath =
		Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	const std::string hebrewFontPath =
		Sys::getProcessPath() + "assets/fonts/NotoSansHebrew-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( koreanFontPath ) );
	ASSERT_TRUE( FileSystem::fileExists( hebrewFontPath ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	ResourceScope& sceneScope = *app.getUI()->getResourceScope();
	const size_t sceneFontCount = sceneScope.getFonts().size();

	dialog->setSelectedFont( koreanFontPath );
	dialog->setSelectedFont( hebrewFontPath );

	EXPECT_EQ( sceneFontCount, sceneScope.getFonts().size() );
}

UTEST( UIFontPickerDialog, SelectingFontDoesNotCreateMetricOnlyPage ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );
	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	dialog->setSelectedFont( fontPath );
	app.getUI()->draw();
	GlobalBatchRenderer::instance()->draw();

	bool foundPreviewPage = false;
	for ( const auto& texture : TextureFactory::instance()->snapshotTextures() ) {
		if ( String::startsWith( texture.displayName, "@font:TrueType:Noto Sans KR:" ) ) {
			foundPreviewPage = true;
			EXPECT_FALSE( String::endsWith( texture.displayName, ":10" ) );
		}
	}
	EXPECT_TRUE( foundPreviewPage );
}

UTEST( UIFontPickerDialog, TeardownWithActivePreviewDoesNotInvalidateDestroyedLayouts ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UILinearLayout::New()->setParent( app.getUI()->getRoot() );
	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );
	UIFontPickerDialog::New()->setSelectedFont( fontPath );
}

UTEST( UIFontPickerDialog, ReleasesPreviewFontTexturesOnClose ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const std::string koreanFontPath =
		Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	const std::string hebrewFontPath =
		Sys::getProcessPath() + "assets/fonts/NotoSansHebrew-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( koreanFontPath ) );
	ASSERT_TRUE( FileSystem::fileExists( hebrewFontPath ) );

	TextureFactory* textureFactory = TextureFactory::instance();
	UnorderedSet<ResourceId> baselineIds;
	for ( const auto& texture : textureFactory->snapshotTextures() )
		baselineIds.insert( texture.id );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	dialog->setSelectedFont( koreanFontPath );
	app.getUI()->draw();
	dialog->setSelectedFont( hebrewFontPath );
	app.getUI()->draw();
	GlobalBatchRenderer::instance()->draw();

	std::vector<std::pair<std::string, TextureWeakPtr>> previewTextures;
	for ( const auto& texture : textureFactory->snapshotTextures() ) {
		if ( baselineIds.find( texture.id ) == baselineIds.end() &&
			 ( String::contains( texture.displayName, "Noto Sans KR" ) ||
			   String::contains( texture.displayName, "Noto Sans Hebrew" ) ) )
			previewTextures.emplace_back( texture.displayName, texture.texture );
	}
	ASSERT_FALSE( previewTextures.empty() );

	dialog->close();
	app.getUI()->update( Time::Zero );
	GlobalBatchRenderer::instance()->draw();
	textureFactory->collectReleasedTextures();

	for ( const auto& texture : previewTextures )
		EXPECT_TRUE( texture.second.expired() );
}

UTEST( UIFontPickerDialog, ReleasesEnumeratedSystemFontTexturesOnClose ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	app.getUI()->getResourceScope()->clearImports();

	std::vector<FontDesc> systemFonts;
	UnorderedSet<std::string> families;
	for ( const FontDesc& font : SystemFontResolver::instance()->enumerate() ) {
		if ( font.path.empty() || font.family.empty() || !families.insert( font.family ).second )
			continue;
		systemFonts.emplace_back( font );
		if ( systemFonts.size() == 3 )
			break;
	}
	if ( systemFonts.size() < 2 )
		UTEST_SKIP( "fewer than two distinct system font families available" );

	TextureFactory* textureFactory = TextureFactory::instance();
	TestFontPickerDialog* dialog = TestFontPickerDialog::New();
	app.getUI()->draw();
	GlobalBatchRenderer::instance()->draw();
	std::vector<FontTrueTypeWeakPtr> previewFonts;
	std::vector<TextureWeakPtr> previewTextures;

	for ( const FontDesc& font : systemFonts ) {
		dialog->setSelectedFont( font );
		app.getUI()->draw();
		GlobalBatchRenderer::instance()->draw();
		FontTrueTypePtr previewFont = dialog->getPreviewFontHandle().lock();
		ASSERT_TRUE( previewFont );
		previewFonts.emplace_back( previewFont );
		previewTextures.emplace_back(
			previewFont->getTexture( PixelDensity::dpToPxI( dialog->getSelection().size * 2 ) ) );
		previewTextures.emplace_back( previewFont->getTexture( PixelDensity::dpToPxI( 12 ) ) );
	}

	dialog->close();
	GlobalBatchRenderer::instance()->draw();
	pumpUntil( app.getUI(), [&] {
		textureFactory->collectReleasedTextures();
		textureFactory->purgeExpiredTextures();
		return std::all_of( previewFonts.begin(), previewFonts.end(),
							[]( const FontTrueTypeWeakPtr& font ) { return font.expired(); } ) &&
			   std::all_of( previewTextures.begin(), previewTextures.end(),
							[]( const TextureWeakPtr& texture ) { return texture.expired(); } );
	} );

	for ( const auto& font : previewFonts )
		EXPECT_TRUE( font.expired() );
	for ( const auto& texture : previewTextures )
		EXPECT_TRUE( texture.expired() );
}

UTEST( UIFontPickerDialog, AsyncLoadPreservesExternalFontPreselection ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	app.getUI()->setThreadPool( ThreadPool::createShared( 1 ) );

	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New( UIFontPickerDialog::MonospaceOnly );
	EXPECT_FALSE( dialog->getButtonOK()->isEnabled() );

	dialog->setSelectedFont( fontPath );
	pumpUntil( app.getUI(), [dialog] { return dialog->getButtonOK()->isEnabled(); } );

	EXPECT_TRUE( dialog->getButtonOK()->isEnabled() );
	EXPECT_STDSTREQ( fontPath, dialog->getSelection().font.path );
	EXPECT_FALSE( dialog->getSelection().font.family.empty() );
	EXPECT_FALSE( dialog->getFamilyList()->getSelection().isEmpty() );
	EXPECT_FALSE( dialog->getStyleList()->getSelection().isEmpty() );
	EXPECT_TRUE( dialog->getFamilyList()->getSelection().first().data().toString().find(
					 "[External" ) != std::string::npos );
}

UTEST( UIFontPickerDialog, AsyncLoadPromotesEnumeratedExternalFontToSystemFamily ) {
	std::vector<FontDesc> fonts = SystemFontResolver::instance()->enumerate();
	auto regular = std::find_if( fonts.begin(), fonts.end(), []( const FontDesc& font ) {
		return font.weight == FontWeight::Normal && !font.italic && !font.family.empty() &&
			   !font.path.empty();
	} );
	if ( regular == fonts.end() )
		UTEST_SKIP( "no regular system font available" );
	const FontDesc regularFont = *regular;

	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	TestFontPickerDialog* dialog = TestFontPickerDialog::New();
	dialog->setFontsForTest( fonts );
	dialog->setSelectedFont( regularFont );
	dialog->markSelectedFontExternalForTest();
	EXPECT_STDSTREQ( regularFont.family + " [External]",
					 dialog->getFamilyList()->getSelection().first().data().toString() );
	EXPECT_TRUE( dialog->getStyleList()->getSelection().first().data().toString().find(
					 "[External:" ) != std::string::npos );

	dialog->setFontsForTest( std::move( fonts ) );
	EXPECT_STDSTREQ( regularFont.path, dialog->getSelection().font.path );
	EXPECT_EQ( regularFont.faceIndex, dialog->getSelection().font.faceIndex );
	EXPECT_EQ( FontWeight::Normal, dialog->getSelection().font.weight );
	EXPECT_FALSE( dialog->getSelection().font.italic );
	EXPECT_STDSTREQ( regularFont.family,
					 dialog->getFamilyList()->getSelection().first().data().toString() );
	EXPECT_TRUE( dialog->getStyleList()->getSelection().first().data().toString().find(
					 "[External:" ) == std::string::npos );

	dialog->releasePreviewFont();
}

UTEST( UIFontPickerDialog, DefaultColorComesFromTheme ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();

	EXPECT_TRUE( dialog->getSelection().color != Color::White );
	EXPECT_TRUE( dialog->getSelection().color.a != 0 );
}

UTEST( UIFontPickerDialog, IncludesDiskFontsLoadedInDefaultScope ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );

	FontTrueTypePtr fontHandle =
		FontTrueType::New( "UIFontPickerDialogManagedNotoSansKR", fontPath );
	FontTrueType* font = fontHandle.get();
	ASSERT_TRUE( font != nullptr );
	ASSERT_TRUE( font->loaded() );
	ASSERT_FALSE( font->getInfo().family.empty() );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	const ModelIndex familyIndex = dialog->getFamilyList()->findRowWithText(
		font->getInfo().family, true, UIAbstractView::FindRowWithTextMatchKind::Equals );
	ASSERT_TRUE( familyIndex.isValid() );
	dialog->getFamilyList()->setSelection( familyIndex );

	bool foundManagedFont = false;
	Model* styleModel = dialog->getStyleList()->getModel();
	ASSERT_TRUE( styleModel != nullptr );
	for ( size_t row = 0; row < styleModel->rowCount(); row++ ) {
		dialog->getStyleList()->setSelection( styleModel->index( row ) );
		if ( dialog->getSelection().font.path == fontPath &&
			 dialog->getSelection().font.faceIndex == font->getFaceIndex() ) {
			foundManagedFont = true;
			break;
		}
	}

	EXPECT_TRUE( foundManagedFont );
}

UTEST( UIFontPickerDialog, StyleLabelsIncludeFontStretch ) {
	std::vector<FontDesc> fonts = SystemFontResolver::instance()->enumerate();
	auto fontIt = std::find_if( fonts.begin(), fonts.end(), []( const FontDesc& font ) {
		return font.stretch != FontStretch::Normal && !font.family.empty() && !font.path.empty();
	} );
	if ( fontIt == fonts.end() )
		UTEST_SKIP( "no non-normal font stretch available" );

	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	const ModelIndex familyIndex = dialog->getFamilyList()->findRowWithText(
		fontIt->family, true, UIAbstractView::FindRowWithTextMatchKind::Equals );
	ASSERT_TRUE( familyIndex.isValid() );
	dialog->getFamilyList()->setSelection( familyIndex );

	const std::string stretchText( expectedStretchLabel( fontIt->stretch ) );
	ASSERT_FALSE( stretchText.empty() );

	bool foundStyle = false;
	Model* styleModel = dialog->getStyleList()->getModel();
	ASSERT_TRUE( styleModel != nullptr );
	for ( size_t row = 0; row < styleModel->rowCount(); row++ ) {
		ModelIndex index = styleModel->index( row );
		dialog->getStyleList()->setSelection( index );
		if ( dialog->getSelection().font.sameFile( *fontIt ) &&
			 dialog->getSelection().font.sameStyle( *fontIt ) ) {
			foundStyle = true;
			EXPECT_TRUE( index.data().toString().find( stretchText ) != std::string::npos );
			break;
		}
	}

	EXPECT_TRUE( foundStyle );
}

UTEST( UIFontPickerDialog, PreservesVisibleStyleAndPrefersRegularWhenHidden ) {
	std::vector<FontDesc> fonts = SystemFontResolver::instance()->enumerate();
	auto italic = std::find_if( fonts.begin(), fonts.end(), [&]( const FontDesc& candidate ) {
		if ( !candidate.italic )
			return false;
		return std::find_if( fonts.begin(), fonts.end(), [&]( const FontDesc& regular ) {
				   return regular.family == candidate.family &&
						  regular.weight == FontWeight::Normal && !regular.italic;
			   } ) != fonts.end();
	} );
	if ( italic == fonts.end() )
		UTEST_SKIP( "no family with regular and italic styles available" );
	const FontDesc italicFont = *italic;

	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	TestFontPickerDialog* visible = TestFontPickerDialog::New( UIFontPickerDialog::ShowStyle );
	visible->setFontsForTest( fonts );
	visible->setSelectedFont( italicFont );
	EXPECT_TRUE( visible->getStyleList()->getParent()->isVisible() );
	EXPECT_EQ( italicFont.weight, visible->getSelection().font.weight );
	EXPECT_TRUE( visible->getSelection().font.italic );

	TestFontPickerDialog* hidden = TestFontPickerDialog::New( UIFontPickerDialog::ShowSize );
	hidden->setFontsForTest( std::move( fonts ) );
	hidden->setSelectedFont( italicFont );
	EXPECT_FALSE( hidden->getStyleList()->getParent()->isVisible() );
	EXPECT_EQ( FontWeight::Normal, hidden->getSelection().font.weight );
	EXPECT_FALSE( hidden->getSelection().font.italic );

	visible->releasePreviewFont();
	hidden->releasePreviewFont();
}

UTEST( UIFontPickerDialog, ApplyButtonEmitsOnApply ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New( UIFontPickerDialog::DefaultFlags |
														  UIFontPickerDialog::ShowApplyButton );
	bool applied = false;
	bool picked = false;
	bool confirmed = false;
	dialog->on( Event::OnApply, [&applied]( const Event* ) { applied = true; } );
	dialog->on( Event::OnConfirm, [&confirmed]( const Event* ) { confirmed = true; } );
	dialog->setOnFontPicked( [&picked]( const UIFontSelection& ) { picked = true; } );

	dialog->getButtonApply()->sendCommonEvent( Event::MouseClick );

	EXPECT_TRUE( applied );
	EXPECT_TRUE( picked );
	EXPECT_FALSE( confirmed );
}

UTEST( UIFontPickerDialog, HidesStyleAndSupportsEveryIntegerSize ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	TestFontPickerDialog* dialog = TestFontPickerDialog::New( UIFontPickerDialog::ShowSize );
	dialog->selectCustomSize( 15 );

	EXPECT_FALSE( dialog->getStyleList()->getParent()->isVisible() );
	EXPECT_EQ( 67u, dialog->getSizeList()->getModel()->rowCount() );
	EXPECT_EQ( 15u, dialog->getSelection().size );
	EXPECT_FALSE( dialog->getSizeList()->getSelection().isEmpty() );
	EXPECT_EQ( 15u, dialog->getSizeList()->getSelection().first().data().asUint() );
	EXPECT_EQ( static_cast<Uint32>( PixelDensity::dpToPxI( 15 ) ), dialog->getPreviewTextSize() );
	EXPECT_EQ( dialog->getPreviewTextSize(), dialog->getPreviewInputSize() );
	EXPECT_TRUE( dialog->getDetails().contains( "15 dp" ) );
	dialog->releasePreviewFont();
}
