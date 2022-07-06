#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE::Scene;

namespace eterm { namespace UI {

UITerminal* UITerminal::New( Font* font, const Float& fontSize, const Sizef& pixelsSize,
							 std::string program, const std::vector<std::string>& args,
							 const std::string& workingDir, const size_t& historySize,
							 IProcessFactory* processFactory, const bool& useFrameBuffer ) {
	auto win = SceneManager::instance()->getUISceneNode()->getWindow();
	auto terminal =
		TerminalDisplay::create( win, font, fontSize, pixelsSize, program, args, workingDir,
								 historySize, processFactory, useFrameBuffer );
	return UITerminal::New( terminal );
}

UITerminal* UITerminal::New( const std::shared_ptr<TerminalDisplay>& terminalDisplay ) {
	return eeNew( UITerminal, ( terminalDisplay ) );
}

UITerminal::~UITerminal() {}

Uint32 UITerminal::getType() const {
	return UI_TYPE_TERMINAL;
}

bool UITerminal::isType( const Uint32& type ) const {
	return getType() == type || UIWidget::isType( type );
}

void UITerminal::draw() {
	mTerm->setPosition( mScreenPosi.asFloat() );
	mTerm->draw();
}

UITerminal::UITerminal( const std::shared_ptr<TerminalDisplay>& terminalDisplay ) :
	UIWidget( "terminal" ), mTerm( terminalDisplay ) {
	mFlags |= UI_TAB_STOP;
	mTerm->pushEventCallback( [&]( const TerminalDisplay::Event& event ) {
		if ( event.type == TerminalDisplay::EventType::TITLE && getParent() ) {
			if ( !mIsCustomTitle && mTitle != event.eventData ) {
				mTitle = event.eventData;
				sendTextEvent( Event::OnTitleChange, mTitle );
			}
		}
	} );
	subscribeScheduledUpdate();
}

const std::shared_ptr<TerminalDisplay>& UITerminal::getTerm() const {
	return mTerm;
}

void UITerminal::scheduledUpdate( const Time& ) {
	mTerm->update();
	if ( mTerm->isDirty() )
		invalidateDraw();
}

const std::string& UITerminal::getTitle() const {
	return mTitle;
}

void UITerminal::setTitle( const std::string& title ) {
	if ( title != mTitle ) {
		mTitle = title;
		mIsCustomTitle = true;
		sendTextEvent( Event::OnTitleChange, title );
	}
}

Uint32 UITerminal::onTextInput( const TextInputEvent& event ) {
	mTerm->onTextInput( event.getChar() );
	return 1;
}

Uint32 UITerminal::onKeyDown( const KeyEvent& event ) {
	mTerm->onKeyDown( event.getKeyCode(), event.getChar(), event.getMod(), event.getScancode() );
	return 1;
}

Uint32 UITerminal::onKeyUp( const KeyEvent& ) {
	return 1;
}

Uint32 UITerminal::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	mTerm->onMouseMove( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) &&
		 mTerm->getTerminal()->getSelectionMode() == TerminalSelectionMode::SEL_IDLE ) {
		mDraggingSel = true;
	} else if ( ( flags & EE_BUTTON_LMASK ) && mDraggingSel ) {
		return 1;
	}
	mTerm->onMouseDown( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	mTerm->onMouseDoubleClick( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) && mDraggingSel )
		mDraggingSel = false;
	mTerm->onMouseUp( position, flags );
	return 1;
}

void UITerminal::onPositionChange() {
	mTerm->setPosition( mScreenPosi.asFloat() );
	UIWidget::onPositionChange();
}

void UITerminal::onSizeChange() {
	mTerm->setSize( getPixelsSize() );
	UIWidget::onSizeChange();
}

Uint32 UITerminal::onFocus() {
	mTerm->setFocus( true );
	invalidateDraw();
	return UIWidget::onFocus();
}

Uint32 UITerminal::onFocusLoss() {
	mTerm->setFocus( false );
	invalidateDraw();
	return UIWidget::onFocusLoss();
}

}} // namespace eterm::UI
