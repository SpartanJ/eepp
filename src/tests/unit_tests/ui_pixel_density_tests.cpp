#include "utest.hpp"

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uiviewpager.hpp>
#include <eepp/ui/uiwidgettable.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;

namespace {

struct RestoreDensity {
	Float previous{ PixelDensity::getPixelDensity() };

	~RestoreDensity() { PixelDensity::setPixelDensity( previous ); }
};

struct DensityApplication {
	RestoreDensity density;
	UIApplication app{
		WindowSettings( 1200, 1000, "Pixel Density Regression", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 2.f ) };
};

class PixelSetterWidget : public UIWidget {
  public:
	PixelSetterWidget() : UIWidget( "pixel-setter-test" ) {}

	int paddingChanges{ 0 };
	int marginChanges{ 0 };

  protected:
	void onPaddingChange() {
		++paddingChanges;
		UIWidget::onPaddingChange();
	}

	void onMarginChange() {
		++marginChanges;
		UIWidget::onMarginChange();
	}
};

class DensityListBox : public UIListBox {
  public:
	void recalculateRowHeight() {
		mRowHeight = 0;
		setRowHeight();
	}
};

class DensityWidgetTable : public UIWidgetTable {
  public:
	void refreshSkinPadding() {
		autoPadding();
		containerResize();
	}
};

} // namespace

UTEST( PixelDensityRegression, PixelPaddingUpdatesAndRepeatedValues ) {
	DensityApplication fixture;
	auto* widget = eeNew( PixelSetterWidget, () );
	widget->setParent( fixture.app.getUI() );
	const Rectf logical( 4, 6, 8, 10 );
	widget->setPadding( logical );
	int changes = widget->paddingChanges;
	// A pixel value equal to the previous DP value must still update the widget.
	widget->setPaddingPixels( logical );
	EXPECT_TRUE( widget->getPixelsPadding() == logical );
	EXPECT_TRUE( widget->getPadding() == Rectf( 2, 3, 4, 5 ) );
	EXPECT_EQ( widget->paddingChanges, changes + 1 );
	widget->setPaddingPixels( logical );
	EXPECT_EQ( widget->paddingChanges, changes + 1 );

	using Setter = UIWidget* ( UIWidget::* )( const Float& );
	Setter setters[] = { &UIWidget::setPaddingPixelsLeft, &UIWidget::setPaddingPixelsTop,
						 &UIWidget::setPaddingPixelsRight, &UIWidget::setPaddingPixelsBottom };
	for ( auto setter : setters ) {
		widget->setPadding( Rectf( 4, 4, 4, 4 ) );
		changes = widget->paddingChanges;
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->paddingChanges, changes + 1 );
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->paddingChanges, changes + 1 );
		( widget->*setter )( 5 );
		const Rectf pixels = widget->getPixelsPadding();
		const Rectf dp = widget->getPadding();
		EXPECT_TRUE( dp == PixelDensity::pxToDp( pixels ).ceil() );
		EXPECT_EQ( pixels.Left + pixels.Top + pixels.Right + pixels.Bottom, 29.f );
	}
}

UTEST( PixelDensityRegression, PixelMarginsUpdateAndRepeatedValues ) {
	DensityApplication fixture;
	auto* widget = eeNew( PixelSetterWidget, () );
	widget->setParent( fixture.app.getUI() );
	const Rectf logical( 4, 6, 8, 10 );
	widget->setLayoutMargin( logical );
	int changes = widget->marginChanges;
	widget->setLayoutPixelsMargin( logical );
	EXPECT_TRUE( widget->getLayoutPixelsMargin() == logical );
	EXPECT_TRUE( widget->getLayoutMargin() == Rectf( 2, 3, 4, 5 ) );
	EXPECT_EQ( widget->marginChanges, changes + 1 );
	widget->setLayoutPixelsMargin( logical );
	EXPECT_EQ( widget->marginChanges, changes + 1 );

	using Setter = UIWidget* ( UIWidget::* )( const Float& );
	Setter setters[] = { &UIWidget::setLayoutPixelsMarginLeft, &UIWidget::setLayoutPixelsMarginTop,
						 &UIWidget::setLayoutPixelsMarginRight,
						 &UIWidget::setLayoutPixelsMarginBottom };
	for ( auto setter : setters ) {
		widget->setLayoutMargin( Rectf( 4, 4, 4, 4 ) );
		changes = widget->marginChanges;
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->marginChanges, changes + 1 );
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->marginChanges, changes + 1 );
		( widget->*setter )( 5 );
		const Rectf pixels = widget->getLayoutPixelsMargin();
		EXPECT_TRUE( widget->getLayoutMargin() == PixelDensity::pxToDp( pixels ).ceil() );
		EXPECT_EQ( pixels.Left + pixels.Top + pixels.Right + pixels.Bottom, 29.f );
	}
}

UTEST( PixelDensityRegression, LoaderPagerAndMergeToolbarUseLogicalDefaults ) {
	DensityApplication fixture;
	auto* loader = UILoader::New();
	loader->setParent( fixture.app.getUI() );
	EXPECT_EQ( loader->getOutlineThickness(), 8.f );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( fixture.app.getUI() );
	EXPECT_EQ( viewer->getLoader()->getOutlineThickness(), 6.f );
	EXPECT_EQ( viewer->getLoader()->getPixelsSize().x, 128.f );
	auto* pager = UIViewPager::New();
	pager->setParent( fixture.app.getUI() );
	EXPECT_EQ( pager->getDragResistance(), 8.f );
	auto* merge = UIMergeView::New();
	merge->setParent( fixture.app.getUI() );
	auto* button = merge->addToolbarAction( "dpi-test", "Test", {}, "", [] {} );
	EXPECT_EQ( button->getLayoutMargin().Right, 4.f );
	EXPECT_EQ( button->getLayoutPixelsMargin().Right, 8.f );
}

UTEST( PixelDensityRegression, ListBoxAutomaticRowHeightAddsLogicalPadding ) {
	DensityApplication fixture;
	auto* list = eeNew( DensityListBox, () );
	list->setParent( fixture.app.getUI() );
	list->getReferenceItem()->setFontSize( 16 );
	list->recalculateRowHeight();
	const auto& style = list->getReferenceItem()->getFontStyleConfig();
	const Uint32 fontHeight =
		style.getFont() ? style.getFont()->getFontHeight( style.getFontCharacterSize() ) : 12;
	EXPECT_EQ( list->getRowHeight(), static_cast<Uint32>( fontHeight / 2.f + 4 ) );
}

UTEST( PixelDensityRegression, MessageBoxControlsUseLogicalDimensions ) {
	DensityApplication fixture;
	auto* editBox = UIMessageBox::New( UIMessageBox::TEXT_EDIT, "Edit" );
	EXPECT_EQ( editBox->getTextEdit()->getSize().x, 600.f );
	EXPECT_EQ( editBox->getTextEdit()->getSize().y, 200.f );
	EXPECT_EQ( editBox->getTextEdit()->getPixelsSize().x, 1200.f );
	auto* dropdownBox = UIMessageBox::New( UIMessageBox::DROPDOWNLIST, "Choose" );
	EXPECT_EQ( dropdownBox->getDropDownList()->getSize().x, 200.f );
	EXPECT_EQ( dropdownBox->getDropDownList()->getPixelsSize().x, 400.f );
	auto* comboBox = UIMessageBox::New( UIMessageBox::COMBOBOX, "Choose" );
	EXPECT_EQ( comboBox->getComboBox()->getSize().x, 200.f );
	EXPECT_EQ( comboBox->getComboBox()->getPixelsSize().x, 400.f );
}

UTEST( PixelDensityRegression, FontPickerUsesLogicalSceneInset ) {
	DensityApplication fixture;
	fixture.app.getUI()->setSize( 600, 450 );
	const Sizef scene = fixture.app.getUI()->getSize();
	auto* dialog = UIFontPickerDialog::New();

	EXPECT_EQ( dialog->getMinWindowSize().x, scene.x - 32.f );
	EXPECT_EQ( dialog->getMinWindowSize().y, scene.y - 32.f );
}

UTEST( PixelDensityRegression, EditorLineNumberPaddingScalesExactlyOnce ) {
	DensityApplication fixture;
	auto* editor = UICodeEditor::New();
	editor->setParent( fixture.app.getUI() );
	editor->setLineNumberPadding( 3, 5 );
	EXPECT_EQ( editor->getLineNumberPaddingLeft(), 6.f );
	EXPECT_EQ( editor->getLineNumberPaddingRight(), 10.f );
	editor->setLineNumberPadding( 3, 5 );
	EXPECT_EQ( editor->getLineNumberPaddingLeft(), 6.f );
	EXPECT_EQ( editor->getLineNumberPaddingRight(), 10.f );
	EXPECT_EQ( editor->getLineNumberWidth(),
			   eeceil( editor->getLineNumberDigits() * editor->getGlyphWidth() + 16.f ) );
}

UTEST( PixelDensityRegression, DiffGuttersUsePhysicalGlyphWidthAndGrowWithLineNumbers ) {
	DensityApplication fixture;
	auto* diff = UIDiffView::New();
	diff->setParent( fixture.app.getUI() );
	for ( bool large : { false, true, false } ) {
		diff->loadFromPatch( large
								 ? "--- a/test\n+++ b/test\n@@ -100000,1 +100000,1 @@\n-old\n+new\n"
								 : "--- a/test\n+++ b/test\n@@ -1,1 +1,1 @@\n-old\n+new\n" );
		const int digits = large ? 6 : 5;
		EXPECT_EQ( diff->getEditor()->getPluginsGutterSpace(),
				   diff->getEditor()->getGlyphWidth() * ( digits * 2 + 1 ) );
		diff->setViewMode( UIDiffView::ViewMode::SideBySide );
		for ( auto* editor : { diff->getLeftEditor(), diff->getRightEditor() } ) {
			EXPECT_EQ( editor->getPluginsGutterSpace(), editor->getGlyphWidth() * digits );
			editor->setFontSize( 18 );
			EXPECT_EQ( editor->getPluginsGutterSpace(), editor->getGlyphWidth() * digits );
		}
		diff->setViewMode( UIDiffView::ViewMode::Unified );
	}
}

UTEST( PixelDensityRegression, DiffImagesUseNativePixelsAndOnlyShrinkToFit ) {
	DensityApplication fixture;
	fixture.app.getUI()->setThreadPool( ThreadPool::createShared( 2 ) );
	const std::string oldPath( Sys::getTempPath() + "eepp-diff-density-old.png" );
	const std::string newPath( Sys::getTempPath() + "eepp-diff-density-new.png" );
	// Odd dimensions expose rounding errors when undoing fractional pixel density.
	Image oldImage( 81, 51, 4, Color::Red );
	Image newImage( 81, 51, 4, Color::Blue );
	ASSERT_TRUE( oldImage.saveToFile( oldPath, Image::SaveType::PNG ) );
	ASSERT_TRUE( newImage.saveToFile( newPath, Image::SaveType::PNG ) );

	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* diff = UIDiffView::New();
		diff->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		diff->setPixelsSize( 400, 200 );
		diff->loadFromFile( oldPath, newPath );
		auto* left = diff->getLeftImageViewer();
		auto* right = diff->getRightImageViewer();
		ASSERT_TRUE( left && right );
		const auto waitForImages = [&] {
			Clock clock;
			while ( ( !left->getImage()->getDrawable() || !right->getImage()->getDrawable() ) &&
					clock.getElapsedTime() < Seconds( 5 ) ) {
				SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
				Sys::sleep( Milliseconds( 1 ) );
			}
			return left->getImage()->getDrawable() && right->getImage()->getDrawable();
		};
		ASSERT_TRUE( waitForImages() );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 81.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 51.f, viewer->getImage()->getPixelsSize().y );
		}

		diff->setPixelsSize( 80, 100 );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 40.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 25.f, viewer->getImage()->getPixelsSize().y );
		}
		diff->setPixelsSize( 400, 20 );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 31.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 20.f, viewer->getImage()->getPixelsSize().y );
		}

		diff->setPixelsSize( 400, 200 );
		diff->setViewMode( UIDiffView::ViewMode::Unified );
		const auto viewers = diff->findAllByType<UIImageViewer>( UI_TYPE_IMAGE_VIEWER );
		ASSERT_EQ( size_t{ 3 }, viewers.size() );
		auto* visualDiff = viewers[2]->getImage();
		ASSERT_TRUE( visualDiff->getDrawable() );
		EXPECT_EQ( 81.f, visualDiff->getPixelsSize().x );
		EXPECT_EQ( 51.f, visualDiff->getPixelsSize().y );
		diff->setPixelsSize( 40, 100 );
		EXPECT_EQ( 40.f, visualDiff->getPixelsSize().x );
		EXPECT_EQ( 25.f, visualDiff->getPixelsSize().y );
		diff->setPixelsSize( 400, 200 );
		EXPECT_EQ( 81.f, visualDiff->getPixelsSize().x );
		EXPECT_EQ( 51.f, visualDiff->getPixelsSize().y );
		diff->setViewMode( UIDiffView::ViewMode::SideBySide );
		ASSERT_TRUE( waitForImages() );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 81.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 51.f, viewer->getImage()->getPixelsSize().y );
		}
		eeDelete( diff );
	}
	FileSystem::fileRemove( oldPath );
	FileSystem::fileRemove( newPath );
}

UTEST( PixelDensityRegression, TableContainerPaddingAndOriginStayLogical ) {
	DensityApplication fixture;
	auto* table = eeNew( DensityWidgetTable, () );
	table->setParent( fixture.app.getUI() );
	table->unsetFlags( UI_AUTO_PADDING );
	table->setSize( 200, 100 );
	table->setPadding( Rectf( 3, 4, 5, 6 ) );
	EXPECT_TRUE( table->getContainerPadding() == Rectf( 3, 4, 5, 6 ) );
	EXPECT_EQ( table->getContainer()->getPosition().x, 3.f );
	EXPECT_EQ( table->getContainer()->getPosition().y, 4.f );
	EXPECT_EQ( table->getContainer()->getPixelsPosition().x, 6.f );
	EXPECT_EQ( table->getContainer()->getPixelsPosition().y, 8.f );
	unsigned char pixels[20 * 30 * 4] = {};
	auto texture = TextureFactory::instance()->loadFromPixels( pixels, 20, 30, 4 );
	auto skin = UISkin::New( "table-dpi-test" );
	skin->setStateDrawable( UIState::StateFlagNormal, NinePatch::New( texture, 2, 4, 6, 8, 2.f ) );
	table->setSkin( skin.get() );
	table->setFlags( UI_AUTO_PADDING );
	table->refreshSkinPadding();
	const Rectf expected = PixelDensity::pxToDp( skin->getBorderSize() ) + table->getPadding();
	EXPECT_TRUE( table->getContainerPadding() == expected );
	EXPECT_EQ( table->getContainer()->getPosition().x, expected.Left );
	EXPECT_EQ( table->getContainer()->getPosition().y, expected.Top );
}
