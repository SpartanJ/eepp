#include "datetimecontroller.hpp"
#include "appconfig.hpp"
#include "plugins/plugincontextprovider.hpp"
#include <cctype>
#include <ctime>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <string_view>

namespace ecode {

DateTimeController::DateTimeController( PluginContextProvider* context ) : mContext( context ) {}

void DateTimeController::registerCommands( EE::UI::Doc::TextDocument& doc ) {
	doc.setCommand( "insert-date-dd-mm-yyyy", [this] { insertDate( "%d.%m.%Y" ); } );
	doc.setCommand( "insert-date-mm-dd-yyyy", [this] { insertDate( "%m.%d.%Y" ); } );
	doc.setCommand( "insert-date-yyyy-mm-dd", [this] { insertDate( "%Y/%m/%d" ); } );
	doc.setCommand( "insert-date-time-dd-mm-yyyy", [this] { insertDate( "%d.%m.%Y %H:%M:%S" ); } );
	doc.setCommand( "insert-date-time-mm-dd-yyyy", [this] { insertDate( "%m.%d.%Y %H:%M:%S" ); } );
	doc.setCommand( "insert-date-time-yyyy-mm-dd", [this] { insertDate( "%Y/%m/%d %H:%M:%S" ); } );
	doc.setCommand( "insert-date-custom",
					[this] { insertDate( mContext->getConfig().editor.customDateFormat ); } );
	doc.setCommand( "set-custom-date-format", [this] { setCustomDateFormat(); } );
}

bool DateTimeController::isValidDateFormat( const std::string& format ) {
	if ( String::trim( format ).empty() )
		return false;

	for ( size_t i = 0; i < format.size(); ++i ) {
		if ( format[i] != '%' )
			continue;

		if ( ++i >= format.size() )
			return false;

		while ( i < format.size() && ( format[i] == '_' || format[i] == '-' || format[i] == '0' ||
									   format[i] == '^' || format[i] == '#' ) )
			++i;

		while ( i < format.size() && std::isdigit( static_cast<unsigned char>( format[i] ) ) )
			++i;

		if ( i < format.size() && ( format[i] == 'E' || format[i] == 'O' ) )
			++i;

		if ( i >= format.size() )
			return false;

		static constexpr std::string_view VALID_SPECIFIERS =
			"aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%";
		if ( VALID_SPECIFIERS.find( format[i] ) == std::string_view::npos )
			return false;
	}

	return true;
}

std::string DateTimeController::formatCurrentDate( const std::string& format ) {
	std::time_t rawtime;
	std::time( &rawtime );
	std::tm* timeinfo = std::localtime( &rawtime );
	if ( nullptr == timeinfo )
		return {};

	char buf[256];
	if ( std::strftime( buf, sizeof( buf ), format.c_str(), timeinfo ) == 0 )
		return {};
	return std::string( buf );
}

void DateTimeController::insertDate( const std::string& format ) {
	if ( !mContext->getSplitter()->curEditorExistsAndFocused() )
		return;

	UICodeEditor* editor = mContext->getSplitter()->getCurEditor();
	if ( nullptr == editor || editor->isLocked() )
		return;

	if ( !isValidDateFormat( format ) )
		return;

	std::string date( formatCurrentDate( format ) );
	if ( !date.empty() )
		editor->getDocument().textInput( String::fromUtf8( date ) );
}

void DateTimeController::setCustomDateFormat() {
	UIMessageBox* msgBox =
		UIMessageBox::New( UIMessageBox::INPUT,
						   mContext
							   ->i18n( "set_custom_date_format_message",
									   "Set Custom Date Format:\nUses strftime format specifiers." )
							   .unescape() );
	msgBox->setTitle( mContext->i18n( "set_custom_date_format", "Set Custom Date Format" ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setText( mContext->getConfig().editor.customDateFormat );
	msgBox->getTextInput()->on( Event::OnTextChanged, [msgBox]( const Event* ) {
		msgBox->getTextInput()->removeClass( "error" );
	} );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		std::string format( msgBox->getTextInput()->getText().toUtf8() );
		if ( !isValidDateFormat( format ) || formatCurrentDate( format ).empty() ) {
			msgBox->getTextInput()->addClass( "error" );
			mContext->errorMsgBox(
				mContext->i18n( "invalid_date_format", "Invalid date format." ) );
			return;
		}
		mContext->getConfig().editor.customDateFormat = std::move( format );
		msgBox->closeWindow();
	} );
	setFocusEditorOnClose( msgBox );
}

void DateTimeController::setFocusEditorOnClose( EE::UI::UIMessageBox* msgBox ) {
	UICodeEditor* editor = mContext->getSplitter()->getCurEditor();
	msgBox->on( Event::OnWindowClose, [editor]( const Event* ) {
		if ( editor && SceneManager::existsSingleton() &&
			 !SceneManager::instance()->isShuttingDown() )
			editor->setFocus();
	} );
}

} // namespace ecode
