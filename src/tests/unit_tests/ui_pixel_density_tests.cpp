#include "utest.hpp"

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uithememanager.hpp>
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

template <typename Base> class PasswordInputProbe : public Base {
  public:
	PasswordInputProbe() : Base() {}

	Text& visibleText() { return this->getVisibleTextCache(); }
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

UTEST( PixelDensityRegression, PasswordTextUsesResolvedFontSize ) {
	DensityApplication fixture;
	static_assert( sizeof( PasswordInputProbe<UITextInput> ) == sizeof( UITextInput ) );
	static_assert( sizeof( PasswordInputProbe<UIHTMLTextInput> ) == sizeof( UIHTMLTextInput ) );
	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		PixelDensity::setPixelDensity( density );
		auto checkInput = [&]( auto* input ) {
			input->setParent( fixture.app.getUI() );
			input->setFontSize( PixelDensity::dpToPx( 16.f ) );
			input->setText( "secret" );
			input->setMode( UITextInput::TextInputMode::Password );
			EXPECT_EQ( input->visibleText().getCharacterSize(), input->getFontSize() );
			EXPECT_TRUE( input->visibleText().getFont() == input->getFont() );
			Text expected( input->visibleText().getString(), input->getFont(),
						   input->getFontSize() );
			EXPECT_NEAR( input->visibleText().getTextWidth(), expected.getTextWidth(), 0.01f );

			input->setFontSize( PixelDensity::dpToPx( 20.f ) );
			EXPECT_EQ( input->visibleText().getCharacterSize(), input->getFontSize() );
			input->setMode( UITextInput::TextInputMode::Normal );
			input->setMode( UITextInput::TextInputMode::Password );
			EXPECT_EQ( input->visibleText().getCharacterSize(), input->getFontSize() );
		};
		checkInput( eeNew( PasswordInputProbe<UITextInput>, () ) );
		checkInput( eeNew( PasswordInputProbe<UIHTMLTextInput>, () ) );
	}
}

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

	using Setter = UIWidget* (UIWidget::*)( const Float& );
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

	using Setter = UIWidget* (UIWidget::*)( const Float& );
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

UTEST( PixelDensityRegression, ButtonLayoutIncludesTrailingMargin ) {
	DensityApplication fixture;
	fixture.app.getUI()->setStyleSheet( StyleSheet{} );
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* button = UIPushButton::New();
		button->setParent( fixture.app.getUI()->getRoot() );
		button->removeSkin();
		button->unsetFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
		button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		button->setPadding( Rectf::Zero );
		// Keep two visible items: their outer margins must remain part of the packed extent.
		button->setSize( 38, 24 );
		button->getIcon()->setSize( 12, 12 );
		auto* text = button->getTextView();
		text->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		text->setSize( 20, 10 );
		text->applyProperty( StyleSheetProperty( "margin-inline-start", "2dp" ) );
		text->applyProperty( StyleSheetProperty( "margin-inline-end", "4dp" ) );
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			button->setDirection( direction );
			for ( auto alignment : { UIHorizontalAlignment::Left, UIHorizontalAlignment::Center,
									 UIHorizontalAlignment::Right } ) {
				button->setHorizontalAlignment( alignment );
				EXPECT_EQ( button->getContentSize().x, PixelDensity::dpToPx( 38.f ) );
				EXPECT_EQ( button->updateLayout().x, PixelDensity::dpToPx( 38.f ) );
				EXPECT_EQ( text->getPixelsPosition().x,
						   PixelDensity::dpToPx( button->isRTL() ? 4.f : 14.f ) );
			}
		}
		eeDelete( button );
	}
}

UTEST( PixelDensityRegression, ButtonLayoutIgnoresMarginsOfLoneVisibleItem ) {
	DensityApplication fixture;
	fixture.app.getUI()->setStyleSheet( StyleSheet{} );
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* button = UIPushButton::New();
		button->setParent( fixture.app.getUI()->getRoot() );
		button->removeSkin();
		button->unsetFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
		button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		button->setPadding( Rectf( 4, 4, 4, 4 ) );
		button->setSize( 80, 24 );
		auto* icon = button->getIcon();
		icon->setSize( 12, 12 );
		auto* text = button->getTextView();
		text->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		text->setSize( 20, 10 );
		const Rectf declaredMargin( 2, 1, 6, 3 );
		icon->setLayoutMargin( declaredMargin );
		text->setLayoutMargin( declaredMargin );
		for ( auto orientation : { InnerWidgetOrientation::WidgetIconTextBox,
								   InnerWidgetOrientation::WidgetTextBoxIcon,
								   InnerWidgetOrientation::IconTextBoxWidget,
								   InnerWidgetOrientation::IconWidgetTextBox,
								   InnerWidgetOrientation::TextBoxIconWidget,
								   InnerWidgetOrientation::TextBoxWidgetIcon } ) {
			button->setInnerWidgetOrientation( orientation );
			for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
				button->setDirection( direction );
				for ( bool showIcon : { true, false } ) {
					icon->setVisible( showIcon );
					text->setVisible( !showIcon );
					auto* visible = showIcon ? static_cast<UIWidget*>( icon ) : text;
					const Float width = visible->getPixelsSize().x;
					EXPECT_EQ( button->getContentSize().x, width + PixelDensity::dpToPx( 8.f ) );
					EXPECT_EQ( button->updateLayout().x, width + PixelDensity::dpToPx( 8.f ) );
					EXPECT_NEAR( visible->getPixelsPosition().x,
								 ( button->getPixelsSize().x - width ) * 0.5f, 0.01f );
					EXPECT_TRUE( visible->getLayoutMargin() == declaredMargin );
				}
			}
		}
		// An expanded lone label must also receive the space previously reserved for margins.
		button->setExpandTextView( true );
		EXPECT_EQ( button->updateLayout().x, button->getPixelsSize().x );
		EXPECT_EQ( text->getPixelsPosition().x, PixelDensity::dpToPx( 4.f ) );
		EXPECT_EQ( text->getPixelsSize().x, PixelDensity::dpToPx( 72.f ) );
		eeDelete( button );
	}
}

UTEST( PixelDensityRegression, TabCloseInlineEndMarginScalesWithDensity ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	ui->setStyleSheet( R"css(
		tab { skin: none; padding: 0dp; }
		tab::text { padding: 0dp; }
		tab::close { width: 10dp; height: 10dp; margin-inline-end: 4dp; }
	)css" );
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* tabs = UITabWidget::New();
		tabs->setParent( ui->getRoot() );
		tabs->setSize( 300, 100 );
		tabs->setTabsClosable( true );
		auto* tab = tabs->add( "Close margin", UIWidget::New() );
		ui->flushDirtyStyleAndLayout();
		auto* close = tab->getCloseButton();
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			tabs->setDirection( direction );
			tab->updateLayout();
			const Float gap = tab->isRTL() ? close->getPixelsPosition().x
										   : tab->getPixelsSize().x - close->getPixelsPosition().x -
												 close->getPixelsSize().x;
			const Rectf& margin = close->getLayoutPixelsMargin();
			EXPECT_EQ( tab->isRTL() ? margin.Left : margin.Right, PixelDensity::dpToPx( 4.f ) );
			EXPECT_NEAR( gap, PixelDensity::dpToPx( 4.f ), 1.f );
		}
		eeDelete( tabs );
	}
}

UTEST( PixelDensityRegression, BreezeTabIconKeepsBalancedOuterMargins ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* tabs = UITabWidget::New();
		tabs->setParent( ui->getRoot() );
		tabs->setSize( 300, 100 );
		tabs->setTabsClosable( true );
		auto* tab = tabs->add( "breeze.css", UIWidget::New() );
		auto* icon = tab->getIcon();
		icon->setSize( 16, 16 );
		icon->setVisible( true );
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			tabs->setDirection( direction );
			ui->flushDirtyStyleAndLayout();
			tab->updateLayout();
			auto* close = tab->getCloseButton();
			const Float leadingGap =
				tab->isRTL()
					? tab->getPixelsSize().x - icon->getPixelsPosition().x - icon->getPixelsSize().x
					: icon->getPixelsPosition().x;
			const Float trailingGap = tab->isRTL()
										  ? close->getPixelsPosition().x
										  : tab->getPixelsSize().x - close->getPixelsPosition().x -
												close->getPixelsSize().x;
			EXPECT_NEAR( leadingGap, PixelDensity::dpToPx( 4.f ), 1.f );
			EXPECT_NEAR( trailingGap, PixelDensity::dpToPx( 4.f ), 1.f );
		}
		eeDelete( tabs );
	}
}

UTEST( PixelDensityRegression, BreezeButtonIconGapRequiresVisibleText ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		for ( bool selectButton : { false, true } ) {
			auto* button = selectButton ? UISelectButton::New() : UIPushButton::New();
			button->setParent( ui->getRoot() );
			button->setText( "Edit build" );
			button->setTextAsFallback( true );
			button->setIcon( DrawablePtr( RectangleDrawable::New( {}, Sizef( 12, 12 ) ) ) );
			auto* icon = button->getIcon();
			icon->setSize( 12, 12 );
			auto* text = button->getTextView();
			for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
				button->setDirection( direction );
				const auto check = [&]( bool labelVisible ) {
					ui->flushDirtyStyleAndLayout();
					button->updateLayout();
					EXPECT_EQ( text->isVisible(), labelVisible );
					const Rectf& margin = icon->getLayoutPixelsMargin();
					EXPECT_EQ( button->isRTL() ? margin.Left : margin.Right,
							   PixelDensity::dpToPx( 4.f ) );
					if ( labelVisible ) {
						const Float gap =
							button->isRTL()
								? icon->getPixelsPosition().x - text->getPixelsPosition().x -
									  text->getPixelsSize().x
								: text->getPixelsPosition().x - icon->getPixelsPosition().x -
									  icon->getPixelsSize().x;
						EXPECT_NEAR( gap, PixelDensity::dpToPx( 4.f ), 1.f );
					} else {
						const Float left = icon->getPixelsPosition().x;
						const Float right =
							button->getPixelsSize().x - left - icon->getPixelsSize().x;
						EXPECT_NEAR( left, right, 1.f );
					}
				};
				check( false );
				button->setTextAsFallback( false );
				check( true );
				button->setText( "" );
				check( false );
				button->setText( "Edit build" );
				check( true );
				text->setVisible( false );
				check( false );
				text->setVisible( true );
				check( true );
				button->setTextAsFallback( true );
			}
			eeDelete( button );
		}
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

UTEST( PixelDensityRegression, CodeEditorMatchesDefaultTextFontSize ) {
	RestoreDensity restoreDensity;
	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		UIApplication app(
			WindowSettings( 1200, 1000, "Editor Default Font Size", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 density ) );
		auto* ui = app.getUI();
		ui->setStyleSheet( "" );
		auto* manager = ui->getUIThemeManager();
		const auto verifyDefault = [&]() {
			auto* text = UITextView::New();
			auto* editor = UICodeEditor::New();
			text->setParent( ui->getRoot() );
			editor->setParent( ui->getRoot() );
			ui->update( Time::Zero );
			EXPECT_EQ( editor->getFontSize(), static_cast<Float>( text->getFontSize() ) );
			editor->fontSizeGrow();
			editor->fontSizeReset();
			EXPECT_EQ( editor->getFontSize(), static_cast<Float>( text->getFontSize() ) );
			editor->applyProperty( StyleSheetProperty( "font-size", "14dp" ) );
			EXPECT_EQ( editor->getFontSize(), PixelDensity::dpToPx( 14.f ) );
			eeDelete( editor );
			eeDelete( text );
		};
		verifyDefault();
		// Theme defaults take priority over the manager fallback, just as for UITextView.
		manager->setDefaultFontSize( PixelDensity::dpToPx( 20.f ) );
		manager->getDefaultTheme()->setDefaultFontSize( PixelDensity::dpToPx( 10.f ) );
		verifyDefault();
		manager->setDefaultTheme( UIThemePtr{} );
		verifyDefault();
	}
}

UTEST( PixelDensityRegression, BreezePreservesConfiguredCodeEditorFontSize ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	auto* editor = UICodeEditor::New();
	editor->setParent( ui->getRoot() );
	// ecode applies its configured size after creation, before deferred CSS is resolved.
	const Float configuredSize = PixelDensity::dpToPx( 17.f );
	editor->setFontSize( configuredSize );
	ui->update( Time::Zero );
	EXPECT_EQ( editor->getFontSize(), configuredSize );
	editor->reloadStyle( true, true, true, true, true );
	ui->update( Time::Zero );
	EXPECT_EQ( editor->getFontSize(), configuredSize );
	// Settings changes must also survive subsequent theme/style reloads.
	const Float changedSize = PixelDensity::dpToPx( 19.f );
	editor->setFontSize( changedSize );
	editor->reloadStyle( true, true, true, true, true );
	ui->update( Time::Zero );
	EXPECT_EQ( editor->getFontSize(), changedSize );
	eeDelete( editor );
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
