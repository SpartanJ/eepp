#include "utest.hpp"
#include <eepp/graphics/textureregion.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::System;

namespace {

void updateSpinBoxLayout( UIApplication& app ) {
	for ( int i = 0; i < 5; ++i ) {
		app.getUI()->update( Milliseconds( 16 ) );
		app.getWindow()->clear();
		app.getUI()->draw();
		app.getWindow()->display( false, false );
	}
}

} // namespace

UTEST( SpinBoxSizing, WrappedHeightFollowsLiveFontAndInputPaddingChanges ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Wrapped spinbox sizing" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		ASSERT_TRUE( app.getUI() != nullptr );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		app.getUI()->applyDefaultFont( nullptr, PixelDensity::dpToPxI( 12 ) );
		auto* layout = UILinearLayout::NewVertical();
		layout->setParent( app.getUI() );
		layout->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		layout->setSize( 240, 240 );
		auto* spin = UISpinBox::New();
		spin->setParent( layout );
		spin->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
		spin->setSize( 180, 0 );
		auto* sibling = UIWidget::New();
		sibling->setParent( layout );
		sibling->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		sibling->setSize( 180, 14 );
		updateSpinBoxLayout( app );
		const Float originalHeight = spin->getPixelsSize().getHeight();
		auto* input = spin->getTextInput();
		EXPECT_EQ( input->getPixelsSize().getHeight(), originalHeight );
		app.getUI()->applyDefaultFont( nullptr, PixelDensity::dpToPxI( 36 ) );
		updateSpinBoxLayout( app );
		EXPECT_GT( spin->getPixelsSize().getHeight(), originalHeight );
		EXPECT_EQ( input->getPixelsSize().getHeight(), spin->getPixelsSize().getHeight() );
		EXPECT_EQ( input->getPixelsPosition().y, 0.f );
		EXPECT_GE( sibling->getWorldBounds().Top, spin->getWorldBounds().Bottom );
		app.getUI()->applyDefaultFont( nullptr, PixelDensity::dpToPxI( 12 ) );
		updateSpinBoxLayout( app );
		EXPECT_EQ( spin->getPixelsSize().getHeight(), originalHeight );
		EXPECT_EQ( input->getPixelsSize().getHeight(), originalHeight );
		const Rectf padding = input->getPixelsPadding();
		input->setPaddingPixelsTop( padding.Top + 5 );
		input->setPaddingPixelsBottom( padding.Bottom + 7 );
		updateSpinBoxLayout( app );
		EXPECT_EQ( spin->getPixelsSize().getHeight(), originalHeight + 12 );
		EXPECT_EQ( input->getPixelsSize().getHeight(), spin->getPixelsSize().getHeight() );
		EXPECT_GE( sibling->getWorldBounds().Top, spin->getWorldBounds().Bottom );
	}
}

UTEST( SpinBoxSizing, InputFillsFixedAndMatchedHeightAcrossFontChanges ) {
	UIApplication app(
		WindowSettings( 800, 600, "Fixed spinbox sizing" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	ASSERT_TRUE( app.getUI() != nullptr );
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* spin = UISpinBox::New();
	spin->setParent( app.getUI() );
	spin->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	spin->setSize( 180, 80 );
	for ( Uint32 fontSize : { 12u, 36u, 12u } ) {
		app.getUI()->applyDefaultFont( nullptr, fontSize );
		updateSpinBoxLayout( app );
		EXPECT_EQ( spin->getPixelsSize().getHeight(), 80.f );
		EXPECT_EQ( spin->getTextInput()->getPixelsSize().getHeight(), 80.f );
		EXPECT_EQ( spin->getTextInput()->getPixelsPosition().y, 0.f );
	}
	// An explicit height may be smaller than the font's natural line height.
	spin->setSize( 180, 12 );
	app.getUI()->applyDefaultFont( nullptr, 36 );
	updateSpinBoxLayout( app );
	EXPECT_EQ( spin->getPixelsSize().getHeight(), 12.f );
	EXPECT_EQ( spin->getTextInput()->getPixelsSize().getHeight(), 12.f );
	spin->setLayoutHeightPolicy( SizePolicy::MatchParent );
	spin->setPixelsSize( 180, 64 );
	updateSpinBoxLayout( app );
	EXPECT_EQ( spin->getPixelsSize().getHeight(), 64.f );
	EXPECT_EQ( spin->getTextInput()->getPixelsSize().getHeight(), 64.f );
}

UTEST( SpinBoxSizing, WrappedHeightAddsPaddingAfterSkinMinimum ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Wrapped spinbox skin height" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		auto* spin = UISpinBox::New();
		spin->setParent( app.getUI() );
		spin->setSize( 180, 0 );
		auto* reference = UITextInput::New();
		reference->setParent( app.getUI() );
		updateSpinBoxLayout( app );
		auto skin = UISkin::New( "spinbox-sizing-input" );
		auto region = TextureRegion::New();
		region->setOriDestSize( { 100, 60 } );
		skin->setStateDrawable( UIState::StateFlagNormal, region );
		for ( auto* input : { spin->getTextInput(), reference } ) {
			input->setSkin( *skin );
			input->setFontSize( PixelDensity::dpToPxI( 12 ) );
			input->setPaddingPixelsTop( 10 );
			input->setPaddingPixelsBottom( 10 );
		}
		updateSpinBoxLayout( app );
		EXPECT_EQ( spin->getPixelsSize().getHeight(), reference->getPixelsSize().getHeight() );
		EXPECT_EQ( spin->getPixelsSize().getHeight(), PixelDensity::dpToPx( 60 ) + 20 );
		spin->getTextInput()->setFontSize( PixelDensity::dpToPxI( 72 ) );
		updateSpinBoxLayout( app );
		EXPECT_GT( spin->getPixelsSize().getHeight(), reference->getPixelsSize().getHeight() );
		spin->getTextInput()->setFontSize( PixelDensity::dpToPxI( 12 ) );
		updateSpinBoxLayout( app );
		EXPECT_EQ( spin->getPixelsSize().getHeight(), reference->getPixelsSize().getHeight() );
	}
}
