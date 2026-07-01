#include "utest.hpp"
#include <algorithm>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>

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

	UIFontPickerDialog* selectionDialog = UIFontPickerDialog::New();
	UIFontSelection selection;
	selection.font.path = fontPath;
	selectionDialog->setSelection( selection );

	EXPECT_STDSTREQ( fontPath, selectionDialog->getSelection().font.path );
	EXPECT_FALSE( selectionDialog->getSelection().font.family.empty() );
}

UTEST( UIFontPickerDialog, AsyncLoadPreservesExternalFontPreselection ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	app.getUI()->setThreadPool( ThreadPool::createShared( 1 ) );

	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	EXPECT_FALSE( dialog->getButtonOK()->isEnabled() );

	dialog->setSelectedFont( fontPath );
	pumpUntil( app.getUI(), [dialog] { return dialog->getButtonOK()->isEnabled(); } );

	EXPECT_TRUE( dialog->getButtonOK()->isEnabled() );
	EXPECT_STDSTREQ( fontPath, dialog->getSelection().font.path );
	EXPECT_FALSE( dialog->getSelection().font.family.empty() );
	EXPECT_FALSE( dialog->getFamilyList()->getSelection().isEmpty() );
	EXPECT_FALSE( dialog->getStyleList()->getSelection().isEmpty() );
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

UTEST( UIFontPickerDialog, IncludesDiskFontsLoadedInFontManager ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIFontPickerDialog Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const std::string fontPath = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	ASSERT_TRUE( FileSystem::fileExists( fontPath ) );

	FontTrueType* font = FontTrueType::New( "UIFontPickerDialogManagedNotoSansKR", fontPath );
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
