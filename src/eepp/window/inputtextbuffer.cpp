#include <eepp/window/inputtextbuffer.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace Window {

InputTextBuffer::InputTextBuffer( const bool& active, const bool& supportNewLine, const bool& supportFreeEditing, EE::Window::Window * window, const Uint32& maxLength ) :
	mWindow( window ),
	mFlags(0),
	mCallback(0),
	mPromptPos(0),
	mMaxLength(INPUT_LENGHT_MAX),
	mSelCurInit(-1),
	mSelCurEnd(-1)
{
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	this->active( active );

	this->supportFreeEditing( supportFreeEditing );

	this->supportNewLine( supportNewLine );

	autoPrompt( true );

	supportCopyPaste( true );

	mMaxLength = maxLength;
}

InputTextBuffer::InputTextBuffer( EE::Window::Window * window ) :
	mWindow( window ),
	mFlags(0),
	mCallback(0),
	mPromptPos(0),
	mMaxLength(INPUT_LENGHT_MAX),
	mSelCurInit(-1),
	mSelCurEnd(-1)
{
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	active( true );

	supportFreeEditing( true );

	supportNewLine( false );

	autoPrompt( true );

	textSelectionEnabled( false );

	supportCopyPaste( true );
}

InputTextBuffer::~InputTextBuffer() {
	if ( 0 != mCallback &&
		Engine::existsSingleton() &&
		Engine::instance()->existsWindow( mWindow ) )
	{
		mWindow->getInput()->popCallback( mCallback );
	}

	mText.clear();
}

void InputTextBuffer::start() {
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	if ( Engine::instance()->existsWindow( mWindow ) ) {
		mCallback = mWindow->getInput()->pushCallback( cb::Make1( this, &InputTextBuffer::update ) );
	}
}

void InputTextBuffer::promptToLeftFirstNoChar() {
	if ( !mText.size() )
		return;

	if ( mPromptPos - 2 > 0 ) {
		for ( Int32 i = mPromptPos - 2; i > 0; i-- ) {
			if ( !String::isLetter( mText[i] ) && !String::isNumber( mText[i] ) && '\n' != mText[i] ) {
				mPromptPos = i + 1;
				break;
			} else if ( i - 1 == 0 ) {
				mPromptPos = 0;
				break;
			}
		}
	} else {
		mPromptPos = 0;
	}

	resetSelection();
}

void InputTextBuffer::promptToRightFirstNoChar() {
	Int32 s = static_cast<Int32> ( mText.size() );

	if ( 0 == s )
		return;

	for ( Int32 i = mPromptPos; i < s; i++ ) {
		if ( !String::isLetter( mText[i] ) && !String::isNumber( mText[i] ) && '\n' != mText[i] ) {
			mPromptPos = i + 1;
			break;
		} else if ( i + 1 == s ) {
			mPromptPos = s;
		}
	}

	resetSelection();
}

void InputTextBuffer::resetSelection() {
	if ( textSelectionEnabled() ) {
		selCurInit( -1 );
		selCurEnd( -1 );
	}
}

void InputTextBuffer::eraseToPrevNoChar() {
	if ( !mText.size() || !mPromptPos )
		return;

	String::StringBaseType c;

	do {
		if ( mPromptPos < (int)mText.size() ) {
			mText.erase( mPromptPos - 1, 1 );
			mPromptPos--;
		} else {
			mText.resize( mText.size() - 1 );
			mPromptPos = mText.size();
		}

		if ( mPromptPos <= 0 ) {
			mPromptPos = 0;
			break;
		}

		c = mText[ mPromptPos - 1 ];
	} while  ( String::isLetter( c ) || String::isNumber( c ) );

	resetSelection();

	changedSinceLastUpdate( true );
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
			c = mText[ tPromptPos ];
		} else {
			break;
		}
	} while ( String::isLetter( c ) || String::isNumber( c ) );

	if ( tPromptPos <= size ) {
		String iniStr( mText.substr( 0, mPromptPos ) );
		String endStr( mText.substr( tPromptPos ) );

		buffer( iniStr + endStr );

		resetSelection();
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

		if ( allowOnlyNumbers() && !String::isNumber( c, allowDotsInNumbers() ) ) {
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
	if ( supportFreeEditing() ) {
		if ( validChar( c ) ) {
			removeSelection();

			changedSinceLastUpdate( true );

			if ( autoPrompt() ) {
				mText += c;
				mPromptPos = (int)mText.size();
			} else {
				String::insertChar( mText, mPromptPos, c );
				mPromptPos++;
			}
		}
	} else {
		if ( canAdd() && String::isCharacter(c) ) {
			Input * Input = mWindow->getInput();

			if ( !Input->metaPressed() && !Input->altPressed() && !Input->controlPressed() ) {
				if ( !( allowOnlyNumbers() && !String::isNumber( c, allowDotsInNumbers() ) ) ) {
					mText += c;
				}
			}
		}
	}
}

void InputTextBuffer::removeSelection() {
	if ( textSelectionEnabled() && -1 != mSelCurInit && -1 != mSelCurEnd ) {
		Int32 size = (Int32)mText.size();

		if ( mSelCurInit <= size && mSelCurInit <= size ) {
			Int32 init		= eemin( mSelCurInit, mSelCurEnd );
			Int32 end		= eemax( mSelCurInit, mSelCurEnd );
			String iniStr( mText.substr( 0, init ) );
			String endStr( mText.substr( end ) );

			buffer( iniStr + endStr );

			curPos( init );

			resetSelection();
		} else {
			resetSelection();
		}
	}
}

void InputTextBuffer::update( InputEvent* Event ) {
	if ( active() ) {
		Input * Input = mWindow->getInput();

		Uint32 c = eeConvertKeyCharacter( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );

		if ( supportFreeEditing() ) {
			switch ( Event->Type ) {
				case InputEvent::TextInput:
				{
					tryAddChar( Event->text.text );
					break;
				}
				case InputEvent::KeyDown:
				{
					if ( textSelectionEnabled() ) {
						if ( mSelCurInit >= 0 && mSelCurInit != mSelCurEnd ) {
							if ( ( Event->key.keysym.mod & KEYMOD_CTRL ) && ( Event->key.keysym.sym == KEY_C || Event->key.keysym.sym == KEY_X ) ) {
								Int32 init		= eemin( mSelCurInit, mSelCurEnd );
								Int32 end		= eemax( mSelCurInit, mSelCurEnd );
								std::string clipStr( mText.substr( init, end - init ).toUtf8() );
								mWindow->getClipboard()->setText( clipStr );
							} else if (	( Event->key.keysym.sym >= KEY_UP && Event->key.keysym.sym <= KEY_END ) &&
										!( Event->key.keysym.sym >= KEY_NUMLOCK && Event->key.keysym.sym <= KEY_COMPOSE )
							) {
								if ( ! ( Input->shiftPressed() && ( Event->key.keysym.sym >= KEY_UP && Event->key.keysym.sym <= KEY_END ) ) ) {
									resetSelection();
								}
							}

							if ( ( ( Event->key.keysym.sym & KEYMOD_LCTRL ) &&
								   ( Event->key.keysym.sym == KEY_X || Event->key.keysym.sym == KEY_V ) ) ||
								Event->key.keysym.sym == KEY_DELETE
							) {
								removeSelection();
							}
						}

						if ( ( Event->key.keysym.mod & KEYMOD_CTRL ) && Event->key.keysym.sym == KEY_A ) {
							selCurInit( 0 );
							selCurEnd( mText.size() );
							curPos( mSelCurEnd );
						}
					}

					if ( Input->shiftPressed() || Input->controlPressed() ) {
						if ( !allowOnlyNumbers() &&
							(	( ( Event->key.keysym.mod & KEYMOD_SHIFT ) && c == KEY_INSERT ) ||
								( ( Event->key.keysym.mod & KEYMOD_CTRL ) && Event->key.keysym.sym == KEY_V ) )
							)
						{
							String txt = mWindow->getClipboard()->getWideText();

							if ( !supportNewLine() ) {
								size_t pos = txt.find_first_of( '\n' );

								if ( pos != std::string::npos )
									txt = txt.substr( 0, pos );
							}

							if ( txt.size() ) {
								changedSinceLastUpdate( true );

								if ( mText.size() + txt.size() < mMaxLength ) {
									if ( autoPrompt() ) {
										mText += txt;
										mPromptPos = (int)mText.size();
									} else {
										mText.insert( mPromptPos, txt );
										mPromptPos += txt.size();
									}

									autoPrompt( false );
								}
							}
						}

						if ( Input->controlPressed() ) {
							if ( c == KEY_LEFT ) {
								promptToLeftFirstNoChar();
								break;
							} else if ( c == KEY_RIGHT ) {
								promptToRightFirstNoChar();
								break;
							} else if ( c == KEY_BACKSPACE ) {
								eraseToPrevNoChar();
								break;
							} else if ( c == KEY_DELETE ) {
								eraseToNextNoChar();
								break;
							}
						}
					}

					if ( ( c == KEY_BACKSPACE || c == KEY_DELETE ) ) {
						if ( mText.size() ) {
							changedSinceLastUpdate( true );

							if ( mPromptPos < (int)mText.size() ) {
								if ( c == KEY_BACKSPACE ) {
									if ( mPromptPos > 0 ) {
										mText.erase(mPromptPos-1,1);
										mPromptPos--;
									}
								} else {
									mText.erase(mPromptPos,1);
								}
							} else if ( c == KEY_BACKSPACE ) {
								mText.resize( mText.size() - 1 );
								autoPrompt( true );
							}

							resetSelection();
						}
					} else if ( ( c == KEY_RETURN || c == KEY_KP_ENTER ) ) {
						if ( supportNewLine() && canAdd() ) {
							String::insertChar( mText, mPromptPos, '\n' );

							mPromptPos++;

							resetSelection();

							changedSinceLastUpdate( true );
						}

						if ( mEnterCall.IsSet() )
							mEnterCall();

					} else if ( c == KEY_LEFT ) {
						if ( ( mPromptPos - 1 ) >= 0 ) {
							mPromptPos--;
							autoPrompt( false );
							shiftSelection( mPromptPos + 1 );
						}
					} else if ( c == KEY_RIGHT ) {
						if ( ( mPromptPos + 1 ) < (int)mText.size() ) {
							mPromptPos++;
							autoPrompt(false);
							shiftSelection( mPromptPos - 1 );
						} else if ( ( mPromptPos + 1 ) == (int)mText.size() ) {
							autoPrompt( true );
						}
					} else if ( c == KEY_UP ) {
						movePromptRowUp( false );
					} else if ( c == KEY_DOWN ) {
						movePromptRowDown( true );
					} else if ( c == KEY_PAGEUP ) {
						movePromptRowUp( true );
					} else if ( c == KEY_PAGEDOWN ) {
						movePromptRowDown( false );
					} else if ( c == KEY_TAB ) {
						tryAddChar( c );
					}

					break;
				}
				case InputEvent::KeyUp:
				{
					if ( supportNewLine() ) {
						int lPromtpPos = mPromptPos;

						if ( c == KEY_END ) {
							for ( Uint32 i = mPromptPos; i < mText.size(); i++ )  {
								if ( mText[i] == '\n' ) {
									mPromptPos = i;
									autoPrompt( false );
									break;
								}

								if ( i == ( mText.size() - 1 ) ) {
									mPromptPos = mText.size();
									autoPrompt( false );
								}
							}

							shiftSelection( lPromtpPos );
						}

						if ( c == KEY_HOME ) {
							if ( 0 != mPromptPos ) {
								for ( Int32 i = (Int32)mPromptPos - 1; i >= 0; i-- )  {
									if ( i >= 0 ) {
										if ( mText[i] == '\n' ) {
											mPromptPos = i + 1;
											autoPrompt( false );
											break;
										}

										if ( i == 0 ) {
											mPromptPos = 0;
											autoPrompt( false );
										}
									}
								}
							}

							shiftSelection( lPromtpPos );
						}
					} else {
						int lPromtpPos = mPromptPos;

						if ( c == KEY_END ) {
							autoPrompt( true );

							shiftSelection( lPromtpPos );
						}

						if ( c == KEY_HOME ) {
							mPromptPos = 0;
							autoPrompt(false);

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
				changedSinceLastUpdate( true );

				if ( c == KEY_BACKSPACE && mText.size() > 0 ) {
					mText.resize( mText.size() - 1 );
				} else if ( ( c == KEY_RETURN || c == KEY_KP_ENTER ) && !Input->metaPressed() && !Input->altPressed() && !Input->controlPressed() ) {
					if ( supportNewLine() && canAdd() )
						mText += '\n';

					if ( mEnterCall.IsSet() )
						mEnterCall();
				}
			}
		}
	}
}

void InputTextBuffer::shiftSelection( const int& lastPromtpPos ) {
	if ( !textSelectionEnabled() )
		return;

	Input * Input = mWindow->getInput();

	if ( Input->shiftPressed() && !Input->controlPressed() ) {
		if ( selCurInit() != curPos() ) {
			selCurEnd( curPos() );
		} else {
			if ( selCurInit() != curPos() ) {
				selCurInit( curPos() );
			} else {
				resetSelection();
				return;
			}
		}

		if ( -1 == selCurInit() ) {
			selCurInit( lastPromtpPos );
		}

		if ( -1 == selCurEnd() ) {
			selCurEnd( lastPromtpPos );
		}
	}
}

void InputTextBuffer::movePromptRowDown( const bool& breakit ) {
	if ( supportFreeEditing() && supportNewLine() ) {
		int lPromtpPos = mPromptPos;

		Uint32 dNLPos	= 0;
		getCurPosLinePos( dNLPos );
		Uint32 dCharsTo = mPromptPos - dNLPos;

		Uint32 dLastLinePos		= 0;
		Uint32 dCharLineCount	= 0;

		for ( Uint32 i = mPromptPos; i < mText.size(); i++ )  {
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
				mPromptPos = dLastLinePos + dCharLineCount;
			} else {
				mPromptPos = dLastLinePos + dCharsTo;
			}

			autoPrompt( false );
		}

		shiftSelection( lPromtpPos );
	}
}

void InputTextBuffer::movePromptRowUp( const bool& breakit ) {
	if ( supportFreeEditing() && supportNewLine() ) {
		int lPromtpPos = mPromptPos;

		Uint32 uNLPos	= 0;
		Uint32 uLineNum	= getCurPosLinePos( uNLPos );
		Uint32 uCharsTo = mPromptPos - uNLPos;

		if ( uLineNum >= 1 ) {
			Uint32 uLastLinePos		= 0;
			Uint32 uCharLineCount	= 0;
			uNLPos					= ( uNLPos - 1 );

			for ( Uint32 i = 0; i < uNLPos; i++ )  {
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
				mPromptPos = uLastLinePos + uCharLineCount;
			} else {
				mPromptPos = uLastLinePos + uCharsTo;
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

void InputTextBuffer::buffer( const String& str ) {
	if ( mText != str ) {
		mText = str;
		changedSinceLastUpdate( true );
	}
}

int InputTextBuffer::curPos() const {
	return mPromptPos;
}

void InputTextBuffer::curPos( const Uint32& pos ) {
	if ( supportFreeEditing() ) {
		if (  pos < mText.size() ) {
			mPromptPos = pos;
			autoPrompt( false );
		} else {
			cursorToEnd();
		}
	}
}

Uint32 InputTextBuffer::getCurPosLinePos( Uint32& LastNewLinePos ) {
	if ( supportFreeEditing() ) {
		Uint32 nl = 0;
		LastNewLinePos = 0;
		for ( int i = 0; i < mPromptPos; i++ )  {
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

void InputTextBuffer::maxLength( const Uint32& Max ) {
	mMaxLength = Max;

	if ( mText.size() > mMaxLength )
		mText.resize( mMaxLength );
}

const Uint32& InputTextBuffer::maxLength() const {
	return mMaxLength;
}

String InputTextBuffer::buffer() const {
	return mText;
}

bool InputTextBuffer::changedSinceLastUpdate() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_CHANGE_SINCE_LAST_UPDATE ) );
}

void InputTextBuffer::changedSinceLastUpdate( const bool& Changed ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_CHANGE_SINCE_LAST_UPDATE, Changed == true );
}

void InputTextBuffer::autoPrompt( const bool& set ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_PROMPT_AUTO_POS, set == true );

	if ( set ) {
		mPromptPos		= (int)mText.size();
	}
}

bool InputTextBuffer::autoPrompt() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_PROMPT_AUTO_POS ) );
}

bool InputTextBuffer::active() const {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ACTIVE ) );
}

void InputTextBuffer::active( const bool& Active ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_ACTIVE, Active == true );
}

bool InputTextBuffer::supportNewLine() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_NEW_LINE ) );
}

void InputTextBuffer::supportNewLine( const bool& SupportNewLine ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_SUPPORT_NEW_LINE, SupportNewLine == true );
}

void InputTextBuffer::allowOnlyNumbers( const bool& onlynums, const bool& allowdots ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_ALLOW_ONLY_NUMBERS, onlynums == true );
	BitOp::writeBitKey( &mFlags, INPUT_TB_ALLOW_DOT_IN_NUMBERS, allowdots == true );
}

bool InputTextBuffer::allowOnlyNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_ONLY_NUMBERS ) );
}

bool InputTextBuffer::allowDotsInNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_DOT_IN_NUMBERS ) );
}

bool InputTextBuffer::supportFreeEditing() const {
	return 0 != ( mFlags & ( 1 << INPUT_TB_FREE_EDITING ) );
}

void InputTextBuffer::supportFreeEditing( const bool& Support ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_FREE_EDITING, Support == true );
}

void InputTextBuffer::supportCopyPaste( const bool& support ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_SUPPORT_COPY_PASTE, support == true );
}

bool InputTextBuffer::supportCopyPaste() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_COPY_PASTE ) );
}

bool InputTextBuffer::textSelectionEnabled() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_TEXT_SELECTION_ENABLED ) );
}

void InputTextBuffer::textSelectionEnabled( const bool& enabled ) {
	BitOp::writeBitKey( &mFlags, INPUT_TB_TEXT_SELECTION_ENABLED, enabled == true );
}

void InputTextBuffer::cursorToEnd() {
	mPromptPos = mText.size();
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

}}
