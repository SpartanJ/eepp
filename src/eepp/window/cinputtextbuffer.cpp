#include <eepp/window/cinputtextbuffer.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/window/cclipboard.hpp>

namespace EE { namespace Window {

cInputTextBuffer::cInputTextBuffer( const bool& active, const bool& supportNewLine, const bool& supportFreeEditing, Window::cWindow * window, const Uint32& maxLength ) :
	mWindow( window ),
	mFlags(0),
	mCallback(0),
	mPromptPos(0),
	mMaxLength(INPUT_LENGHT_MAX)
{
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	Active( active );

	SupportFreeEditing( supportFreeEditing );

	SupportNewLine( supportNewLine );

	AutoPrompt( true );

	SupportCopyPaste( true );

	mMaxLength = maxLength;
}

cInputTextBuffer::cInputTextBuffer( cWindow * window ) :
	mWindow( window ),
	mFlags(0),
	mCallback(0),
	mPromptPos(0),
	mMaxLength(INPUT_LENGHT_MAX)
{
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	Active( true );

	SupportFreeEditing( true );

	SupportNewLine( false );

	AutoPrompt( true );

	SupportCopyPaste( true );
}

cInputTextBuffer::~cInputTextBuffer() {
	if ( 0 != mCallback &&
		cEngine::ExistsSingleton() &&
		cEngine::instance()->ExistsWindow( mWindow ) )
	{
		mWindow->GetInput()->PopCallback( mCallback );
	}

	mText.clear();
}

void cInputTextBuffer::Start() {
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	if ( cEngine::instance()->ExistsWindow( mWindow ) ) {
		mCallback = mWindow->GetInput()->PushCallback( cb::Make1( this, &cInputTextBuffer::Update ) );
	}
}

void cInputTextBuffer::PromptToLeftFirstNoChar() {
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
}

void cInputTextBuffer::PromptToRightFirstNoChar() {
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
}

void cInputTextBuffer::EraseToNextNoChar() {
	if ( !mText.size() || !mPromptPos )
		return;

	String::StringBaseType c;

	do {
		if ( mPromptPos < (eeInt)mText.size() ) {
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

	ChangedSinceLastUpdate( true );
}

bool cInputTextBuffer::IsIgnoredChar( const Uint32& c ) {
	if ( mIgnoredChars.size() ) {
		for ( eeUint i = 0; i < mIgnoredChars.size(); i++ ) {
			if ( mIgnoredChars[i] == c )
				return true;
		}
	}

	return false;
}

bool cInputTextBuffer::ValidChar( const Uint32& c ) {
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

void cInputTextBuffer::TryAddChar( const Uint32& c ) {
	if ( SupportFreeEditing() ) {
		if ( ValidChar( c ) ) {
			ChangedSinceLastUpdate( true );

			if ( AutoPrompt() ) {
				mText += c;
				mPromptPos = (eeInt)mText.size();
			} else {
				String::InsertChar( mText, mPromptPos, c );
				mPromptPos++;
			}
		}
	} else {
		if ( CanAdd() && String::IsCharacter(c) ) {
			cInput * Input = mWindow->GetInput();

			if ( !Input->MetaPressed() && !Input->AltPressed() && !Input->ControlPressed() ) {
				if ( !( AllowOnlyNumbers() && !String::IsNumber( c, AllowDotsInNumbers() ) ) ) {
					mText += c;
				}
			}
		}
	}
}

void cInputTextBuffer::Update( InputEvent* Event ) {
	if ( Active() ) {
		cInput * Input = mWindow->GetInput();

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
										mPromptPos = (eeInt)mText.size();
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
							} else if ( ( c == KEY_BACKSPACE || c == KEY_DELETE ) ) {
								EraseToNextNoChar();
								break;
							}
						}
					}

					if ( ( c == KEY_BACKSPACE || c == KEY_DELETE ) ) {
						if ( mText.size() ) {
							ChangedSinceLastUpdate( true );

							if ( mPromptPos < (eeInt)mText.size() ) {
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
						}
					} else if ( ( c == KEY_RETURN || c == KEY_KP_ENTER ) ) {
						if ( SupportNewLine() && CanAdd() ) {
							String::InsertChar( mText, mPromptPos, '\n' );

							mPromptPos++;

							ChangedSinceLastUpdate( true );
						}

						if ( mEnterCall.IsSet() )
							mEnterCall();

					} else if ( c == KEY_LEFT ) {
						if ( ( mPromptPos - 1 ) >= 0 ) {
							mPromptPos--;
							AutoPrompt( false );
						}
					} else if ( c == KEY_RIGHT ) {
						if ( ( mPromptPos + 1 ) < (eeInt)mText.size() ) {
							mPromptPos++;
							AutoPrompt(false);
						} else if ( ( mPromptPos + 1 ) == (eeInt)mText.size() ) {
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
						}
					} else {
						if ( c == KEY_END ) {
							AutoPrompt( true );
						}

						if ( c == KEY_HOME ) {
							mPromptPos = 0;
							AutoPrompt(false);
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

void cInputTextBuffer::MovePromptRowDown( const bool& breakit ) {
	if ( SupportFreeEditing() && SupportNewLine() ) {
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
	}
}

void cInputTextBuffer::MovePromptRowUp( const bool& breakit ) {
	if ( SupportFreeEditing() && SupportNewLine() ) {
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
	}
}

void cInputTextBuffer::Clear() {
	mText.clear();
	AutoPrompt( true );
}

void cInputTextBuffer::SetReturnCallback( EnterCallback EC ) {
	mEnterCall = EC;
}

void cInputTextBuffer::Buffer( const String& str ) {
	if ( mText != str ) {
		mText = str;
		ChangedSinceLastUpdate( true );
	}
}

eeInt cInputTextBuffer::CurPos() const {
	return mPromptPos;
}

void cInputTextBuffer::CurPos( const Uint32& pos ) {
	if ( SupportFreeEditing() && pos < mText.size() ) {
		mPromptPos = pos;
		AutoPrompt( false );
	}
}

Uint32 cInputTextBuffer::GetCurPosLinePos( Uint32& LastNewLinePos ) {
	if ( SupportFreeEditing() ) {
		Uint32 nl = 0;
		LastNewLinePos = 0;
		for ( eeInt i = 0; i < mPromptPos; i++ )  {
			if ( mText[i] == '\n' ) {
				nl++;
				LastNewLinePos = i + 1;
			}
		}
		return nl;
	}
	return 0;
}

void cInputTextBuffer::PushIgnoredChar( const Uint32& ch ) {
	mIgnoredChars.push_back( ch );
}

bool cInputTextBuffer::CanAdd() {
	return mText.size() < mMaxLength;
}

void cInputTextBuffer::MaxLength( const Uint32& Max ) {
	mMaxLength = Max;

	if ( mText.size() > mMaxLength )
		mText.resize( mMaxLength );
}

const Uint32& cInputTextBuffer::MaxLength() const {
	return mMaxLength;
}

String cInputTextBuffer::Buffer() const {
	return mText;
}

bool cInputTextBuffer::ChangedSinceLastUpdate() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_CHANGE_SINCE_LAST_UPDATE ) );
}

void cInputTextBuffer::ChangedSinceLastUpdate( const bool& Changed ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_CHANGE_SINCE_LAST_UPDATE, Changed == true );
}

void cInputTextBuffer::AutoPrompt( const bool& set ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_PROMPT_AUTO_POS, set == true );

	if ( set ) {
		mPromptPos		= (eeInt)mText.size();
	}
}

bool cInputTextBuffer::AutoPrompt() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_PROMPT_AUTO_POS ) );
}

bool cInputTextBuffer::Active() const {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ACTIVE ) );
}

void cInputTextBuffer::Active( const bool& Active ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_ACTIVE, Active == true );
}

bool cInputTextBuffer::SupportNewLine() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_NEW_LINE ) );
}

void cInputTextBuffer::SupportNewLine( const bool& SupportNewLine ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_SUPPORT_NEW_LINE, SupportNewLine == true );
}

void cInputTextBuffer::AllowOnlyNumbers( const bool& onlynums, const bool& allowdots ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_ALLOW_ONLY_NUMBERS, onlynums == true );
	BitOp::WriteBitKey( &mFlags, INPUT_TB_ALLOW_DOT_IN_NUMBERS, allowdots == true );
}

bool cInputTextBuffer::AllowOnlyNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_ONLY_NUMBERS ) );
}

bool cInputTextBuffer::AllowDotsInNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_DOT_IN_NUMBERS ) );
}

bool cInputTextBuffer::SupportFreeEditing() const {
	return 0 != ( mFlags & ( 1 << INPUT_TB_FREE_EDITING ) );
}

void cInputTextBuffer::SupportFreeEditing( const bool& Support ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_FREE_EDITING, Support == true );
}

void cInputTextBuffer::SupportCopyPaste( const bool& support ) {
	BitOp::WriteBitKey( &mFlags, INPUT_TB_SUPPORT_COPY_PASTE, support == true );
}

bool cInputTextBuffer::SupportCopyPaste() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_COPY_PASTE ) );
}

void cInputTextBuffer::CursorToEnd() {
	mPromptPos = mText.size();
}

}}
