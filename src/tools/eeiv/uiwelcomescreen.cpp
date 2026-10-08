#include "uiwelcomescreen.hpp"
#include "eeiv.hpp"

#include <eepp/ui/uiscenenode.hpp>

static const auto LAYOUT = R"xml(
<style>
<![CDATA[
#welcome_screen {
	background-color: var(--back);
}
#welcome_screen TextView {
	focusable: false;
	color: var(--font-hint);
}
#welcome_screen #welcome_title {
	font-size: 28dp;
	font-style: bold|shadow;
	color: var(--font-hint);
	text-shadow-color: light-dark(#00000019, #00000066);
}
#welcome_screen #welcome_hint {
	margin-top: 8dp;
}
#welcome_screen PushButton {
	min-width: 200dp;
	background-color: var(--list-back);
}
#welcome_screen PushButton:hover {
	border-color: var(--primary);
}
#welcome_screen PushButton:pressed {
	background-color: var(--primary);
	color: var(--font-selected-pressed);
}
#welcome_screen #welcome_logo {
	focusable: false;
	background-image: icon(ecode,256dp);
	background-position: center center;
	background-tint: var(--font-hint);
	background-size: 100% 100%;
}
#welcome_screen #welcome_logo:hover {
	background-tint: var(--primary);
}
]]>
</style>
<vbox lw="wc" lh="wc" lg="center">
	<image id="welcome_logo" lw="wc" min-width="128dp" lh="128dp" lg="center" />
	<tv id="welcome_title" text="eeiv" lg="center" />
	<tv id="welcome_hint" text="Open an image or a folder to get started" lg="center" margin-bottom="24dp" />
	<PushButton id="open_image" text="Open Image" lg="center" margin-bottom="8dp" />
	<PushButton id="open_folder" text="Open Folder" lg="center" />
</vbox>
)xml";

UIWelcomeScreen* UIWelcomeScreen::New( App* app ) {
	return eeNew( UIWelcomeScreen, ( app ) );
}

UIWelcomeScreen::UIWelcomeScreen( App* app ) : UIRelativeLayout(), mApp( app ) {
	setId( "welcome_screen" );
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	getUISceneNode()->loadLayoutFromString( LAYOUT, this, String::hash( "UIWelcomeScreen" ) );

	find( "open_image" )->onClick( [this]( auto ) { mApp->openFileDialog(); } );
	find( "open_folder" )->onClick( [this]( auto ) { mApp->openFolderDialog(); } );
}
