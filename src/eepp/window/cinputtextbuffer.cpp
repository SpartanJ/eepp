#include <eepp/window/cinputtextbuffer.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/window/cclipboard.hpp>

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
		mWindow = Engine::instance()->GetCurrentWindow();
	}

	Active( active );

	SupportFreeEditing( supportFreeEditing );

	SupportNewLine( supportNewLine );

	AutoPrompt( true );

	SupportCopyPaste( true );

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
		mWindow = Engine::instance()->GetCurrentWindow();
	}

	Active( true );

	SupportFreeEditing( true );

	SupportNewLine( false );

	AutoPrompt( true );

	TextSelectionEnabled( false );

	SupportCopyPaste( true );
}

InputTextBuffer::~InputTextBuffer() {
	if ( 0 != mCallback &&
		Engine::ExistsSingleton() &&
		Engine::instance()->ExistsWindow( mWindow ) )
	{
		mWindow->GetInput()->PopCallback( mCallback );
	}

	mText.clear();
}

void InputTextBuffer::Start() {
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->GetCurrentWindow();
	}

	if ( Engine::instance()->ExistsWindow( mWindow ) ) {
		mCallback = mWindow->GetInput()->PushCallback( cb::Make1( this, &InputTextBuffer::Update ) );
	}
}

void InputTextBuffer::PromptToLeftFirstNoChar() {
	if ( !mText.size() )
		return;

	if ( mPromptPos - 2 > 0 ) {
		for ( Int32 i = mPromptPos - 2; i > 0; i-- ) {
			if ( !String::IsLetter( mText[i] ) && !String::IsNumber( mText[i] ) && '\n' != mText[i] ) {
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

	ResetSelection();
}

void InputTextBuffer::PromptToRightFirstNoChar() {
	Int32 s = static_cast<Int32> ( mText.size() );

	if ( 0 == s )
		return;

	for ( Int32 i = mPromptPos; i < s; i++ ) {
		if ( !String::IsLetter( mText[i] ) && !String::IsNumber( mText[i] ) && '\n' != mText[i] ) {
			mPromptPos = i + 1;
			break;
		} else if ( i + 1 == s ) {
			mPromptPos = s;
		}
	}

	ResetSelection();
}

void InputTextBuffer::ResetSelection() {
	if ( TextSelectionEnabled() ) {
		SelCurInit( -1 );
		SelCurEnd( -1 );
	}
}

void InputTextBuffer::EraseToPrevNoChar() {
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
	} while  ( String::IsLetter( c ) || String::IsNumber( c ) );

	ResetSelection();

	ChangedSinceLastUpdate( true );
}

void InputTextBuffer::EraseToNextNoChar() {
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
	} while ( String::IsLetter( c ) || String::IsNumber( c ) );

	if ( tPromptPos <= size ) {
		String iniStr( mText.substr( 0, mPromptPos ) );
		String endStr( mText.substr( tPromptPos ) );

		Buffer( iniStr + endStr );

		ResetSelection();
	}
}

bool InputTextBuffer::IsIgnoredChar( const Uint32& c ) {
	if ( mIgnoredChars.size() ) {
		for ( std::size_t i = 0; i < mIgnoredChars.size(); i++ ) {
			if ( mIgnoredChars[i] == c )
				return true;
		}
	}

	return false;
}

bool InputTextBuffer::ValidChar( const Uint32& c ) {
	if ( CanAdd() && String::IsCharacter( c ) ) {
		bool Ignored = false;

		if ( AllowOnlyNumbers() && !String::IsNumber( c, AllowDotsInNumbers() ) ) {
			Ignored = true;
		}

		if ( IsIgnoredChar( c ) ) {
			Ignored = true;
		}

		if ( !Ignored ) {
			return true;
		}
	}

	return false;
}

void InputTextBuffer::TryAddChar( const Uint32& c ) {
	if ( SupportFreeEditing() ) {
		if ( ValidChar( c ) ) {
			RemoveSelection();

			ChangedSinceLastUpdate( true );

			if ( AutoPrompt() ) {
				mText += c;
				mPromptPos = (int)mText.size();
			} else {
				String::InsertChar( mText, mPromptPos, c );
				mPromptPos++;
			}
		}
	} else {
		if ( CanAdd() && String::IsCharacter(c) ) {
			Input * Input = mWindow->GetInput();

			if ( !Input->MetaPressed() && !Input->AltPressed() && !Input->ControlPressed() ) {
				if ( !( AllowOnlyNumbers() && !String::IsNumber( c, AllowDotsInNumbers() ) ) ) {
					mText += c;
				}
			}
		}
	}
}

void InputTextBuffer::RemoveSelection() {
	if ( TextSelectionEnabled() && -1 != mSelCurInit && -1 != mSelCurEnd ) {
		Int32 size = (Int32)mText.size();

		if ( mSelCurInit <= size && mSelCurInit <= size ) {
			Int32 init		= eemin( mSelCurInit, mSelCurEnd );
			Int32 end		= eemax( mSelCurInit, mSelCurEnd );
			String iniStr( mText.substr( 0, init ) );
			String endStr( mText.substr( end ) );

			Buffer( iniStr + endStr );

			CurPos( init );

			ResetSelection();
		} else {
			ResetSelection();
		}
	}
}

void InputTextBuffer::Update( InputEvent* Event ) {
	if ( Active() ) {
		Input * Input = mWindow->GetInput();

		Uint32 c = eeConvertKeyCharacter( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );

		if ( SupportFreeEditing() ) {
			switch ( Event->Type ) {
				case InputEvent::TextInput:
				{
					TryAddChar( Event->text.text );
					break;
				}
				case InputEvent::KeyDown:
				{
					if ( TextSelectionEnabled() ) {
						if ( mSelCurInit >= 0 && mSelCurInit != mSelCurEnd ) {
							if ( ( Event->key.keysym.mod & KEYMOD_CTRL ) && ( Event->key.keysym.sym == KEY_C || Event->key.keysym.sym == KEY_X ) ) {
								Int32 init		= eemin( mSelCurInit, mSelCurEnd );
								Int32 end		= eemax( mSelCurInit, mSelCurEnd );
								std::string clipStr( mText.substr( init, end - init ).ToUtf8() );
								mWindow->GetClipboard()->SetText( clipStr );
							} else if (	( Event->key.keysym.sym >= KEY_UP && Event->key.keysym.sym <= KEY_END ) &&
										!( Event->key.keysym.sym >= KEY_NUMLOCK && Event->key.keysym.sym <= KEY_COMPOSE )
							) {
								if ( ! ( Input->ShiftPressed() && ( Event->key.keysym.sym >= KEY_UP && Event->key.keysym.sym <= KEY_END ) ) ) {
									ResetSelection();
								}
							}

							if ( ( ( Event->key.keysym.sym & KEYMOD_LCTRL ) &&
								   ( Event->key.keysym.sym == KEY_X || Event->key.keysym.sym == KEY_V ) ) ||
								Event->key.keysym.sym == KEY_DELETE
							) {
								RemoveSelection();
							}
						}

						if ( ( Event->key.keysym.mod & KEYMOD_CTRL ) && Event->key.keysym.sym == KEY_A ) {
							SelCurInit( 0 );
							SelCurEnd( mText.size() );
							CurPos( mSelCurEnd );
						}
					}

					if ( Input->ShiftPressed() || Input->ControlPressed() ) {
						if ( !AllowOnlyNumbers() &&
							(	( ( Event->key.keysym.mod & KEYMOD_SHIFT ) && c == KEY_INSERT ) ||
								( ( Event->key.keysym.mod & KEYMOD_CTRL ) && Event->key.keysym.sym == KEY_V ) )
							)
						{
							String txt = mWindow->GetClipboard()->GetWideText();

							if ( !SupportNewLine() ) {
								size_t pos = txt.find_first_of( '\n' );

								if ( pos != std::string::npos )
									txt = txt.substr( 0, pos );
							}

							if ( txt.size() ) {
								ChangedSinceLastUpdate( true );

								if ( mText.size() + txt.size() < mMaxLength ) {
									if ( AutoPrompt() ) {
										mText += txt;
										mPromptPos = (int)mText.size();
									} else {
										mText.insert( mPromptPos, txt );
										mPromptPos += txt.size();
									}

									AutoPrompt( false );
								}
							}
						}

						if ( Input->ControlPressed() ) {
							if ( c == KEY_LEFT ) {
								PromptToLeftFirstNoChar();
								break;
							} else if ( c == KEY_RIGHT ) {
								PromptToRightFirstNoChar();
								break;
							} else if ( c == KEY_BACKSPACE ) {
								EraseToPrevNoChar();
								break;
							} else if ( c == KEY_DELETE ) {
								EraseToNextNoChar();
								break;
							}
						}
					}

					if ( ( c == KEY_BACKSPACE || c == KEY_DELETE ) ) {
						if ( mText.size() ) {
							ChangedSinceLastUpdate( true );

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
								AutoPrompt( true );
							}

							ResetSelection();
						}
					} else if ( ( c == KEY_RETURN || c == KEY_KP_ENTER ) ) {
						if ( SupportNewLine() && CanAdd() ) {
							String::InsertChar( mText, mPromptPos, '\n' );

							mPromptPos++;

							ResetSelection();

							ChangedSinceLastUpdate( true );
						}

						if ( mEnterCall.IsSet() )
							mEnterCall();

					} else if ( c == KEY_LEFT ) {
						if ( ( mPromptPos - 1 ) >= 0 ) {
							mPromptPos--;
							AutoPrompt( false );
							ShiftSelection( mPromptPos + 1 );
						}
					} else if ( c == KEY_RIGHT ) {
						if ( ( mPromptPos + 1 ) < (int)mText.size() ) {
							mPromptPos++;
							AutoPrompt(false);
							ShiftSelection( mPromptPos - 1 );
						} else if ( ( mPromptPos + 1 ) == (int)mText.size() ) {
							AutoPrompt( true );
						}
					} else if ( c == KEY_UP ) {
						MovePromptRowUp( false );
					} else if ( c == KEY_DOWN ) {
						MovePromptRowDown( true );
					} else if ( c == KEY_PAGEUP ) {
						MovePromptRowUp( true );
					} else if ( c == KEY_PAGEDOWN ) {
						MovePromptRowDown( false );
					} else if ( c == KEY_TAB ) {
						TryAddChar( c );
					}

					break;
				}
				case InputEvent::KeyUp:
				{
					if ( SupportNewLine() ) {
						int lPromtpPos = mPromptPos;

						if ( c == KEY_END ) {
							for ( Uint32 i = mPromptPos; i < mText.size(); i++ )  {
								if ( mText[i] == '\n' ) {
									mPromptPos = i;
									AutoPrompt( false );
									break;
								}

								if ( i == ( mText.size() - 1 ) ) {
									mPromptPos = mText.size();
									AutoPrompt( false );
								}
							}

							ShiftSelection( lPromtpPos );
						}

						if ( c == KEY_HOME ) {
							if ( 0 != mPromptPos ) {
								for ( Int32 i = (Int32)mPromptPos - 1; i >= 0; i-- )  {
									if ( i >= 0 ) {
										if ( mText[i] == '\n' ) {
											mPromptPos = i + 1;
											AutoPrompt( false );
											break;
										}

										if ( i == 0 ) {
											mPromptPos = 0;
											AutoPrompt( false );
										}
									}
								}
							}

							ShiftSelection( lPromtpPos );
						}
					} else {
						int lPromtpPos = mPromptPos;

						if ( c == KEY_END ) {
							AutoPrompt( true );

							ShiftSelection( lPromtpPos );
						}

						if ( c == KEY_HOME ) {
							mPromptPos = 0;
							AutoPrompt(false);

							ShiftSelection( lPromtpPos );
						}
					}
					break;
				}
			}
		} else {
			if ( Event->Type == InputEvent::TextInput ) {
				TryAddChar( Event->text.text );
			} else if ( Event->Type == InputEvent::KeyDown ) {
				ChangedSinceLastUpdate( true );

				if ( c == KEY_BACKSPACE && mText.size() > 0 ) {
					mText.resize( mText.size() - 1 );
				} else if ( ( c == KEY_RETURN || c == KEY_KP_ENTER ) && !Input->MetaPressed() && !Input->AltPressed() && !Input->ControlPressed() ) {
					if ( SupportNewLine() && CanAdd() )
						mText += '\n';

					if ( mEnterCall.IsSet() )
						mEnterCall();
				}
			}
		}
	}
}

void InputTextBuffer::ShiftSelection( const int& lastPromtpPos ) {
	if ( !TextSelectionEnabled() )
		return;

	Input * Input = mWindow->GetInput();

	if ( Input->ShiftPressed() && !Input->ControlPressed() ) {
		if ( SelCurInit() != CurPos() ) {
			SelCurEnd( CurPos() );
		} else {
			if ( SelCurInit() != CurPos() ) {
				SelCurInit( CurPos() );
			} else {
				ResetSelection();
				return;
			}
		}

		if ( -1 == SelCurInit() ) {
			SelCurInit( lastPromtpPos );
		}

		if ( -1 == SelCurEnd() ) {
			SelCurEnd( lastPromtpPos );
		}
	}
}

void InputTextBuffer::MovePromptRowDown( const bool& breakit ) {
	if ( SupportFreeEditing() && SupportNewLine() ) {
		int lPromtpPos = mPromptPos;

		Uint32 dNLPos	= 0;
		GetCurPosLinePos( dNLPos );
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

			AutoPrompt( false );
		}

		ShiftSelection( lPromtpPos );
	}
}

void InputTextBuffer::MovePromptRowUp( const bool& breakit ) {
	if ( SupportFreeEditing() && SupportNewLine() ) {
		int lPromtpPos = mPromptPos;

		Uint32 uNLPos	= 0;
		Uint32 uLineNum	= GetCurPosLinePos( uNLPos );
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

			AutoPrompt( false );
		}

		ShiftSelection( lPromtpPos );
	}
}

void InputTextBuffer::Clear() {
	mText.clear();
	AutoPrompt( true );
}

void InputTextBuffer::SetReturnCallback( EnterCallback EC ) {
	mEnterCall = EC;
}

void InputTextBuffer::Buffer( const String& str ) {
	if ( mText != str ) {
		mText = str;
		ChangedSinceLastUpdate( true );
	}
}

int InputTextBuffer::CurPos() const {
	return mPromptPos;
}

void InputTextBuffer::CurPos( const Uint32& pos ) {
	if ( SupportFreeEditing() ) {
		if (  pos < mText.size() ) {
			mPromptPos = pos;
			AutoPrompt( false );
		} else {
			CursorToEnd();
		}
	}
}

Uint32 InputTextBuffer::GetCurPosLinePos( Uint32& LastNewLinePos ) {
	if ( SupportFreeEditing() ) {
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

void InputTextBuffer::PushIgnoredChar( const Uint32& ch ) {
	mIgnoredChars.push_back( ch );
}

bool InputTextBuffer::CanAdd() {
	return mText.size() < mMaxLength;
}

void InputTextBuffer::MaxLength( const Uint32& Max ) {
	mMaxLength = Max;

	if ( mText.size() > mMaxLength )
		mText.resize( mMaxLength );
}

const Uint32& InputTextBuffer::MaxLength() const {
	return mMaxLength;
}

String InputTextBuffer::Buffer() const {
	return mText;
}

bool InputTextBuffer::ChangedSinceLastUpdate() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_CHANGE_SINCE_LAST_UPDATE ) );
}

void InputTextBuffer::ChangedSinceLastUpdate( const bool& Changed ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_CHANGE_SINCE_LAST_UPDATE, Changed == true );
}

void InputTextBuffer::AutoPrompt( const bool& set ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_PROMPT_AUTO_POS, set == true );

	if ( set ) {
		mPromptPos		= (int)mText.size();
	}
}

bool InputTextBuffer::AutoPrompt() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_PROMPT_AUTO_POS ) );
}

bool InputTextBuffer::Active() const {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ACTIVE ) );
}

void InputTextBuffer::Active( const bool& Active ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_ACTIVE, Active == true );
}

bool InputTextBuffer::SupportNewLine() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_NEW_LINE ) );
}

void InputTextBuffer::SupportNewLine( const bool& SupportNewLine ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_SUPPORT_NEW_LINE, SupportNewLine == true );
}

void InputTextBuffer::AllowOnlyNumbers( const bool& onlynums, const bool& allowdots ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_ALLOW_ONLY_NUMBERS, onlynums == true );
	BitOp::WriteBitKey( &mFlags, INPUT_TB_ALLOW_DOT_IN_NUMBERS, allowdots == true );
}

bool InputTextBuffer::AllowOnlyNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_ONLY_NUMBERS ) );
}

bool InputTextBuffer::AllowDotsInNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_DOT_IN_NUMBERS ) );
}

bool InputTextBuffer::SupportFreeEditing() const {
	return 0 != ( mFlags & ( 1 << INPUT_TB_FREE_EDITING ) );
}

void InputTextBuffer::SupportFreeEditing( const bool& Support ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_FREE_EDITING, Support == true );
}

void InputTextBuffer::SupportCopyPaste( const bool& support ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_SUPPORT_COPY_PASTE, support == true );
}

bool InputTextBuffer::SupportCopyPaste() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_COPY_PASTE ) );
}

bool InputTextBuffer::TextSelectionEnabled() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_TEXT_SELECTION_ENABLED ) );
}

void InputTextBuffer::TextSelectionEnabled( const bool& enabled ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_TEXT_SELECTION_ENABLED, enabled == true );
}

void InputTextBuffer::CursorToEnd() {
	mPromptPos = mText.size();
}

void InputTextBuffer::SelCurInit( const Int32& init ) {
	mSelCurInit = init;
}

void InputTextBuffer::SelCurEnd( const Int32& end ) {
	mSelCurEnd = end;
}

const Int32& InputTextBuffer::SelCurInit() const {
	return mSelCurInit;
}

const Int32& InputTextBuffer::SelCurEnd() const {
	return mSelCurEnd;
}

}}
