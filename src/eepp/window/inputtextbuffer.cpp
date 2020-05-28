#include <eepp/window/clipboard.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/inputtextbuffer.hpp>

namespace EE { namespace Window {

// TODO: Deprecate this. It's horrendous. Use TextDocument instead.

InputTextBuffer* InputTextBuffer::New( const bool& active, const bool& newLineEnabled,
									   const bool& freeEditing, EE::Window::Window* window,
									   const Uint32& maxLength ) {
	return eeNew( InputTextBuffer, ( active, newLineEnabled, freeEditing, window, maxLength ) );
}

InputTextBuffer* InputTextBuffer::New( EE::Window::Window* window ) {
	return eeNew( InputTextBuffer, ( window ) );
}

InputTextBuffer::InputTextBuffer( const bool& active, const bool& newLineEnabled,
								  const bool& freeEditing, EE::Window::Window* window,
								  const Uint32& maxLength ) :
	mWindow( window ),
	mFlags( 0 ),
	mCallback( 0 ),
	mPromptPos( 0 ),
	mMaxLength( UINT32_MAX ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 ) {
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	this->setActive( active );

	this->setFreeEditing( freeEditing );

	this->isNewLineEnabled( newLineEnabled );

	autoPrompt( true );

	supportCopyPaste( true );

	mMaxLength = maxLength;
}

InputTextBuffer::InputTextBuffer( EE::Window::Window* window ) :
	mWindow( window ),
	mFlags( 0 ),
	mCallback( 0 ),
	mPromptPos( 0 ),
	mMaxLength( UINT32_MAX ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 ) {
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	setActive( true );

	setFreeEditing( true );

	isNewLineEnabled( false );

	autoPrompt( true );

	setTextSelectionEnabled( false );

	supportCopyPaste( true );
}

InputTextBuffer::~InputTextBuffer() {
	if ( 0 != mCallback && Engine::existsSingleton() &&
		 Engine::instance()->existsWindow( mWindow ) ) {
		mWindow->getInput()->popCallback( mCallback );
	}

	mText.clear();
}

void InputTextBuffer::start() {
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	if ( Engine::instance()->existsWindow( mWindow ) ) {
		mCallback =
			mWindow->getInput()->pushCallback( cb::Make1( this, &InputTextBuffer::update ) );
	}
}

void InputTextBuffer::promptToLeftFirstNoChar() {
	if ( !mText.size() )
		return;

	if ( mPromptPos - 2 > 0 ) {
		for ( Int32 i = mPromptPos - 2; i > 0; i-- ) {
			if ( !String::isLetter( mText[i] ) && !String::isNumber( mText[i] ) &&
				 '\n' != mText[i] ) {
				setCursorPos( i + 1 );
				break;
			} else if ( i - 1 == 0 ) {
				setCursorPos( 0 );
				break;
			}
		}
	} else {
		setCursorPos( 0 );
	}

	resetSelection();
}

void InputTextBuffer::promptToRightFirstNoChar() {
	Int32 s = static_cast<Int32>( mText.size() );

	if ( 0 == s )
		return;

	for ( Int32 i = mPromptPos; i < s; i++ ) {
		if ( !String::isLetter( mText[i] ) && !String::isNumber( mText[i] ) && '\n' != mText[i] ) {
			setCursorPos( i + 1 );
			break;
		} else if ( i + 1 == s ) {
			setCursorPos( s );
		}
	}

	resetSelection();
}

void InputTextBuffer::resetSelection() {
	if ( isTextSelectionEnabled() ) {
		selCurInit( -1 );
		selCurEnd( -1 );
		onSelectionChange();
	}
}

void InputTextBuffer::eraseToPrevNoChar() {
	if ( !mText.size() || !mPromptPos )
		return;

	String::StringBaseType c;

	do {
		if ( mPromptPos < (int)mText.size() ) {
			mText.erase( mPromptPos - 1, 1 );
			setCursorPos( mPromptPos - 1 );
		} else {
			mText.resize( mText.size() - 1 );
			setCursorPos( mText.size() );
		}

		if ( mPromptPos <= 0 ) {
			setCursorPos( 0 );
			break;
		}

		c = mText[mPromptPos - 1];
	} while ( String::isLetter( c ) || String::isNumber( c ) );

	resetSelection();

	setChangedSinceLastUpdate( true );
	onBufferChange();
}

void InputTextBuffer::eraseToNextNoChar() {
	if ( !mText.size() )
		return;

	Int32 tPromptPos = mPromptPos;
	Int32 c;
	Int32 size = (Int32)mText.size();

	do {
		tPromptPos++;

		if ( tPromptPos < size ) {
			c = mText[tPromptPos];
		} else {
			break;
		}
	} while ( String::isLetter( c ) || String::isNumber( c ) );

	if ( tPromptPos <= size ) {
		String iniStr( mText.substr( 0, mPromptPos ) );
		String endStr( mText.substr( tPromptPos ) );

		setBuffer( iniStr + endStr );

		resetSelection();
		onBufferChange();
	}
}

bool InputTextBuffer::isIgnoredChar( const Uint32& c ) {
	if ( mIgnoredChars.size() ) {
		for ( std::size_t i = 0; i < mIgnoredChars.size(); i++ ) {
			if ( mIgnoredChars[i] == c )
				return true;
		}
	}

	return false;
}

bool InputTextBuffer::validChar( const Uint32& c ) {
	if ( canAdd() && String::isCharacter( c ) ) {
		bool Ignored = false;

		if ( onlyNumbersAllowed() && !String::isNumber( c, dotsInNumbersAllowed() ) ) {
			Ignored = true;
		}

		if ( isIgnoredChar( c ) ) {
			Ignored = true;
		}

		if ( !Ignored ) {
			return true;
		}
	}

	return false;
}

void InputTextBuffer::tryAddChar( const Uint32& c ) {
	if ( isFreeEditingEnabled() ) {
		if ( validChar( c ) ) {
			removeSelection();

			if ( autoPrompt() ) {
				mText += c;
				setCursorPos( (int)mText.size() );
			} else {
				String::insertChar( mText, mPromptPos, c );
				setCursorPos( mPromptPos + 1 );
			}

			setChangedSinceLastUpdate( true );
			onBufferChange();
		}
	} else {
		if ( canAdd() && String::isCharacter( c ) ) {
			Input* Input = mWindow->getInput();

			if ( !Input->isMetaPressed() && !Input->isAltPressed() && !Input->isControlPressed() ) {
				if ( !( onlyNumbersAllowed() && !String::isNumber( c, dotsInNumbersAllowed() ) ) ) {
					mText += c;
				}
			}
		}
	}
}

void InputTextBuffer::removeSelection() {
	if ( isTextSelectionEnabled() && -1 != mSelCurInit && -1 != mSelCurEnd ) {
		Int32 size = (Int32)mText.size();

		if ( mSelCurInit <= size && mSelCurInit <= size ) {
			Int32 init = eemin( mSelCurInit, mSelCurEnd );
			Int32 end = eemax( mSelCurInit, mSelCurEnd );
			String iniStr( mText.substr( 0, init ) );
			String endStr( mText.substr( end ) );

			setBuffer( iniStr + endStr );

			setCursorPos( init );

			resetSelection();
			onBufferChange();
		} else {
			resetSelection();
		}
	}
}

void InputTextBuffer::update( InputEvent* Event ) {
	if ( isActive() ) {
		Input* Input = mWindow->getInput();

		Uint32 keyChar = 0;

		if ( Event->Type == InputEvent::TextInput || Event->Type == InputEvent::KeyDown ||
			 Event->Type == InputEvent::KeyUp ) {
			keyChar = InputHelper::convertKeyCharacter( Event->key.keysym.sym,
														Event->key.keysym.unicode );
		}

		if ( isFreeEditingEnabled() ) {
			switch ( Event->Type ) {
				case InputEvent::TextInput: {
					tryAddChar( Event->text.text );
					break;
				}
				case InputEvent::KeyDown: {
					if ( isTextSelectionEnabled() ) {
						if ( mSelCurInit >= 0 && mSelCurInit != mSelCurEnd ) {
							if ( ( Event->key.keysym.mod & KEYMOD_CTRL ) &&
								 ( Event->key.keysym.sym == KEY_C ||
								   Event->key.keysym.sym == KEY_X ) ) {
								Int32 init = eemin( mSelCurInit, mSelCurEnd );
								Int32 end = eemax( mSelCurInit, mSelCurEnd );
								std::string clipStr( mText.substr( init, end - init ).toUtf8() );
								mWindow->getClipboard()->setText( clipStr );
							} else {
								Keycode sym = Event->key.keysym.sym;
								if ( !( Input->isShiftPressed() &&
										( sym == KEY_UP || sym == KEY_DOWN || sym == KEY_LEFT ||
										  sym == KEY_RIGHT || sym == KEY_INSERT ||
										  sym == KEY_HOME || sym == KEY_END ) ) ) {
									resetSelection();
								}
							}

							if ( ( ( ( Event->key.keysym.sym & KEYMOD_LCTRL ) &&
									 ( Event->key.keysym.sym == KEY_X ||
									   Event->key.keysym.sym == KEY_V ) ) ||
								   Event->key.keysym.sym == KEY_DELETE ) ||
								 keyChar == KEY_BACKSPACE ) {
								removeSelection();
							}
						}

						if ( ( Event->key.keysym.mod & KEYMOD_CTRL ) &&
							 Event->key.keysym.sym == KEY_A ) {
							selCurInit( 0 );
							selCurEnd( mText.size() );
							setCursorPos( mSelCurEnd );
							onSelectionChange();
						}
					}

					if ( Input->isShiftPressed() || Input->isControlPressed() ) {
						if ( !onlyNumbersAllowed() &&
							 ( ( ( Event->key.keysym.mod & KEYMOD_SHIFT ) &&
								 keyChar == KEY_INSERT ) ||
							   ( ( Event->key.keysym.mod & KEYMOD_CTRL ) &&
								 Event->key.keysym.sym == KEY_V ) ) ) {
							String txt = mWindow->getClipboard()->getWideText();

							if ( !setSupportNewLine() ) {
								size_t pos = txt.find_first_of( '\n' );

								if ( pos != std::string::npos )
									txt = txt.substr( 0, pos );
							}

							if ( txt.size() ) {
								if ( mText.size() + txt.size() < mMaxLength ) {
									if ( autoPrompt() ) {
										mText += txt;
										setCursorPos( (int)mText.size() );
									} else {
										mText.insert( mPromptPos, txt );
										mPromptPos += txt.size();
									}

									autoPrompt( false );
								}

								setChangedSinceLastUpdate( true );
								onBufferChange();
							}
						}

						if ( Input->isControlPressed() ) {
							if ( keyChar == KEY_LEFT ) {
								promptToLeftFirstNoChar();
								break;
							} else if ( keyChar == KEY_RIGHT ) {
								promptToRightFirstNoChar();
								break;
							} else if ( keyChar == KEY_BACKSPACE ) {
								eraseToPrevNoChar();
								break;
							} else if ( keyChar == KEY_DELETE ) {
								eraseToNextNoChar();
								break;
							}
						}
					}

					if ( ( keyChar == KEY_BACKSPACE || keyChar == KEY_DELETE ) ) {
						if ( mText.size() ) {
							if ( mPromptPos < (int)mText.size() ) {
								if ( keyChar == KEY_BACKSPACE ) {
									if ( mPromptPos > 0 ) {
										mText.erase( mPromptPos - 1, 1 );
										setCursorPos( mPromptPos - 1 );
									}
								} else {
									mText.erase( mPromptPos, 1 );
								}
							} else if ( keyChar == KEY_BACKSPACE ) {
								mText.resize( mText.size() - 1 );
								autoPrompt( true );
							}

							resetSelection();

							setChangedSinceLastUpdate( true );
							onBufferChange();
						}
					} else if ( ( keyChar == KEY_RETURN || keyChar == KEY_KP_ENTER ) ) {
						if ( setSupportNewLine() && canAdd() ) {
							String::insertChar( mText, mPromptPos, '\n' );

							setCursorPos( mPromptPos + 1 );

							resetSelection();

							setChangedSinceLastUpdate( true );
							onBufferChange();
						}

						if ( mEnterCall )
							mEnterCall();

					} else if ( keyChar == KEY_LEFT ) {
						if ( ( mPromptPos - 1 ) >= 0 ) {
							setCursorPos( mPromptPos - 1 );
							autoPrompt( false );
							shiftSelection( mPromptPos + 1 );
						}
					} else if ( keyChar == KEY_RIGHT ) {
						if ( ( mPromptPos + 1 ) < (int)mText.size() ) {
							setCursorPos( mPromptPos + 1 );
							autoPrompt( false );
							shiftSelection( mPromptPos - 1 );
						} else if ( ( mPromptPos + 1 ) == (int)mText.size() ) {
							autoPrompt( true );
						}
					} else if ( keyChar == KEY_UP ) {
						movePromptRowUp( false );
					} else if ( keyChar == KEY_DOWN ) {
						movePromptRowDown( true );
					} else if ( keyChar == KEY_PAGEUP ) {
						movePromptRowUp( true );
					} else if ( keyChar == KEY_PAGEDOWN ) {
						movePromptRowDown( false );
					} else if ( keyChar == KEY_TAB ) {
						tryAddChar( keyChar );
					}

					break;
				}
				case InputEvent::KeyUp: {
					if ( setSupportNewLine() ) {
						int lPromtpPos = mPromptPos;

						if ( keyChar == KEY_END ) {
							for ( Uint32 i = mPromptPos; i < mText.size(); i++ ) {
								if ( mText[i] == '\n' ) {
									setCursorPos( i );
									autoPrompt( false );
									break;
								}

								if ( i == ( mText.size() - 1 ) ) {
									setCursorPos( mText.size() );
									autoPrompt( false );
								}
							}

							shiftSelection( lPromtpPos );
						}

						if ( keyChar == KEY_HOME ) {
							if ( 0 != mPromptPos ) {
								for ( Int32 i = (Int32)mPromptPos - 1; i >= 0; i-- ) {
									if ( mText[i] == '\n' ) {
										setCursorPos( i + 1 );
										autoPrompt( false );
										break;
									}

									if ( i == 0 ) {
										setCursorPos( 0 );
										autoPrompt( false );
									}
								}
							}

							shiftSelection( lPromtpPos );
						}
					} else {
						int lPromtpPos = mPromptPos;

						if ( keyChar == KEY_END ) {
							autoPrompt( true );

							shiftSelection( lPromtpPos );
						}

						if ( keyChar == KEY_HOME ) {
							setCursorPos( 0 );
							autoPrompt( false );

							shiftSelection( lPromtpPos );
						}
					}
					break;
				}
			}
		} else {
			if ( Event->Type == InputEvent::TextInput ) {
				tryAddChar( Event->text.text );
			} else if ( Event->Type == InputEvent::KeyDown ) {
				if ( keyChar == KEY_BACKSPACE && mText.size() > 0 ) {
					mText.resize( mText.size() - 1 );
				} else if ( ( keyChar == KEY_RETURN || keyChar == KEY_KP_ENTER ) &&
							!Input->isMetaPressed() && !Input->isAltPressed() &&
							!Input->isControlPressed() ) {
					if ( setSupportNewLine() && canAdd() )
						mText += '\n';

					if ( mEnterCall )
						mEnterCall();
				}

				setChangedSinceLastUpdate( true );
				onBufferChange();
			}
		}
	}
}

void InputTextBuffer::shiftSelection( const int& lastPromtpPos ) {
	if ( !isTextSelectionEnabled() )
		return;

	Input* Input = mWindow->getInput();

	if ( Input->isShiftPressed() && !Input->isControlPressed() ) {
		if ( selCurInit() != getCursorPosition() ) {
			selCurEnd( getCursorPosition() );
			onSelectionChange();
		} else {
			if ( selCurInit() != getCursorPosition() ) {
				selCurInit( getCursorPosition() );
				onSelectionChange();
			} else {
				resetSelection();
				return;
			}
		}

		if ( -1 == selCurInit() ) {
			selCurInit( lastPromtpPos );
			onSelectionChange();
		}

		if ( -1 == selCurEnd() ) {
			selCurEnd( lastPromtpPos );
			onSelectionChange();
		}
	}
}

void InputTextBuffer::movePromptRowDown( const bool& breakit ) {
	if ( isFreeEditingEnabled() && setSupportNewLine() ) {
		int lPromtpPos = mPromptPos;

		Uint32 dNLPos = 0;
		getCurPosLinePos( dNLPos );
		Uint32 dCharsTo = mPromptPos - dNLPos;

		Uint32 dLastLinePos = 0;
		Uint32 dCharLineCount = 0;

		for ( Uint32 i = mPromptPos; i < mText.size(); i++ ) {
			if ( mText[i] == '\n' ) {
				if ( breakit ) {
					if ( 0 == dLastLinePos ) {
						dLastLinePos = i + 1;

						dCharLineCount = 0;
					} else {
						break;
					}
				} else {
					dLastLinePos = i + 1;

					dCharLineCount = 0;
				}
			} else {
				dCharLineCount++;
			}
		}

		if ( 0 != dLastLinePos ) {
			if ( dCharLineCount < dCharsTo ) {
				setCursorPos( dLastLinePos + dCharLineCount );
			} else {
				setCursorPos( dLastLinePos + dCharsTo );
			}

			autoPrompt( false );
		}

		shiftSelection( lPromtpPos );
	}
}

void InputTextBuffer::movePromptRowUp( const bool& breakit ) {
	if ( isFreeEditingEnabled() && setSupportNewLine() ) {
		int lPromtpPos = mPromptPos;

		Uint32 uNLPos = 0;
		Uint32 uLineNum = getCurPosLinePos( uNLPos );
		Uint32 uCharsTo = mPromptPos - uNLPos;

		if ( uLineNum >= 1 ) {
			Uint32 uLastLinePos = 0;
			Uint32 uCharLineCount = 0;
			uNLPos = ( uNLPos - 1 );

			for ( Uint32 i = 0; i < uNLPos; i++ ) {
				if ( mText[i] == '\n' ) {
					if ( !breakit ) {
						uLastLinePos = i + 1;

						uCharLineCount = 0;
					} else {
						break;
					}
				} else {
					uCharLineCount++;
				}
			}

			if ( uCharLineCount < uCharsTo ) {
				setCursorPos( uLastLinePos + uCharLineCount );
			} else {
				setCursorPos( uLastLinePos + uCharsTo );
			}

			autoPrompt( false );
		}

		shiftSelection( lPromtpPos );
	}
}

void InputTextBuffer::clear() {
	mText.clear();
	autoPrompt( true );
}

void InputTextBuffer::setReturnCallback( EnterCallback EC ) {
	mEnterCall = EC;
}

void InputTextBuffer::setBuffer( const String& str ) {
	if ( mText != str ) {
		mText = str;
		setChangedSinceLastUpdate( true );
	}
}

int InputTextBuffer::getCursorPosition() const {
	return mPromptPos;
}

void InputTextBuffer::setCursorPosition( const Uint32& pos ) {
	if ( isFreeEditingEnabled() ) {
		if ( pos < mText.size() ) {
			autoPrompt( false );
		}

		mPromptPos = pos;
	}
}

void InputTextBuffer::setCursorPos( const Uint32& pos ) {
	if ( isFreeEditingEnabled() && mPromptPos != (Int32)pos ) {
		setCursorPosition( pos );

		onCursorPositionChange();
	}
}

Uint32 InputTextBuffer::getCurPosLinePos( Uint32& LastNewLinePos ) {
	if ( isFreeEditingEnabled() ) {
		Uint32 nl = 0;
		LastNewLinePos = 0;
		for ( int i = 0; i < mPromptPos; i++ ) {
			if ( mText[i] == '\n' ) {
				nl++;
				LastNewLinePos = i + 1;
			}
		}
		return nl;
	}
	return 0;
}

void InputTextBuffer::pushIgnoredChar( const Uint32& ch ) {
	mIgnoredChars.push_back( ch );
}

bool InputTextBuffer::canAdd() {
	return mText.size() < mMaxLength;
}

void InputTextBuffer::setMaxLength( const Uint32& Max ) {
	mMaxLength = Max;

	if ( mText.size() > mMaxLength )
		mText.resize( mMaxLength );
}

const Uint32& InputTextBuffer::getMaxLength() const {
	return mMaxLength;
}

String InputTextBuffer::getBuffer() const {
	return mText;
}

bool InputTextBuffer::changedSinceLastUpdate() {
	return 0 != ( mFlags & ( 1 << CHANGE_SINCE_LAST_UPDATE ) );
}

void InputTextBuffer::setChangedSinceLastUpdate( const bool& Changed ) {
	BitOp::writeBitKey( &mFlags, CHANGE_SINCE_LAST_UPDATE, Changed == true );
}

void InputTextBuffer::autoPrompt( const bool& set ) {
	BitOp::writeBitKey( &mFlags, PROMPT_AUTO_POS, set == true );

	if ( set ) {
		mPromptPos = (int)mText.size();

		onCursorPositionChange();
	}
}

bool InputTextBuffer::autoPrompt() {
	return 0 != ( mFlags & ( 1 << PROMPT_AUTO_POS ) );
}

bool InputTextBuffer::isActive() const {
	return 0 != ( mFlags & ( 1 << ACTIVE ) );
}

void InputTextBuffer::setActive( const bool& active ) {
	BitOp::writeBitKey( &mFlags, ACTIVE, active == true );
}

bool InputTextBuffer::setSupportNewLine() {
	return 0 != ( mFlags & ( 1 << SUPPORT_NEW_LINE ) );
}

void InputTextBuffer::isNewLineEnabled( const bool& enabled ) {
	BitOp::writeBitKey( &mFlags, SUPPORT_NEW_LINE, enabled == true );
}

void InputTextBuffer::setAllowOnlyNumbers( const bool& onlynums, const bool& allowdots ) {
	BitOp::writeBitKey( &mFlags, ALLOW_ONLY_NUMBERS, onlynums == true );
	BitOp::writeBitKey( &mFlags, ALLOW_DOT_IN_NUMBERS, allowdots == true );
}

bool InputTextBuffer::onlyNumbersAllowed() {
	return 0 != ( mFlags & ( 1 << ALLOW_ONLY_NUMBERS ) );
}

bool InputTextBuffer::dotsInNumbersAllowed() {
	return 0 != ( mFlags & ( 1 << ALLOW_DOT_IN_NUMBERS ) );
}

bool InputTextBuffer::isFreeEditingEnabled() const {
	return 0 != ( mFlags & ( 1 << FREE_EDITING ) );
}

void InputTextBuffer::setFreeEditing( const bool& enabled ) {
	BitOp::writeBitKey( &mFlags, FREE_EDITING, enabled == true );
}

void InputTextBuffer::supportCopyPaste( const bool& support ) {
	BitOp::writeBitKey( &mFlags, SUPPORT_COPY_PASTE, support == true );
}

bool InputTextBuffer::supportCopyPaste() {
	return 0 != ( mFlags & ( 1 << SUPPORT_COPY_PASTE ) );
}

bool InputTextBuffer::isTextSelectionEnabled() {
	return 0 != ( mFlags & ( 1 << TEXT_SELECTION_ENABLED ) );
}

void InputTextBuffer::setTextSelectionEnabled( const bool& enabled ) {
	BitOp::writeBitKey( &mFlags, TEXT_SELECTION_ENABLED, enabled == true );
}

void InputTextBuffer::cursorToEnd() {
	setCursorPos( mText.size() );
}

void InputTextBuffer::selCurInit( const Int32& init ) {
	mSelCurInit = init;
}

void InputTextBuffer::selCurEnd( const Int32& end ) {
	mSelCurEnd = end;
}

const Int32& InputTextBuffer::selCurInit() const {
	return mSelCurInit;
}

const Int32& InputTextBuffer::selCurEnd() const {
	return mSelCurEnd;
}

void InputTextBuffer::setCursorPositionChangeCallback(
	const CursorPositionChangeCallback& cursorPositionChangeCallback ) {
	mCursorPositionChangeCallback = cursorPositionChangeCallback;
}

void InputTextBuffer::setBufferChangeCallback( const BufferChangeCallback& bufferChangeCallback ) {
	mBufferChangeCallback = bufferChangeCallback;
}

void InputTextBuffer::setSelectionChangeCallback(
	const SelectionChangeCallback& selectionChangeCallback ) {
	mSelectionChangeCallback = selectionChangeCallback;
}

void InputTextBuffer::onCursorPositionChange() {
	if ( mCursorPositionChangeCallback ) {
		mCursorPositionChangeCallback();
	}
}

void InputTextBuffer::onSelectionChange() {
	if ( mSelectionChangeCallback ) {
		mSelectionChangeCallback();
	}
}

void InputTextBuffer::onBufferChange() {
	if ( mBufferChangeCallback ) {
		mBufferChangeCallback();
	}
}

}} // namespace EE::Window
