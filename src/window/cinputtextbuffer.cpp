#include "cinputtextbuffer.hpp"

namespace EE { namespace Window {

cInputTextBuffer::cInputTextBuffer( const bool& Active, const bool& SupportNewLine, const bool& SupportFreeEditing, const Uint32& MaxLenght ) :
	mActive( Active ),
	mFlags(0),
	mChangeSinceLastUpdate(false),
	mCallback(0),
	mPromptPos(0),
	mPromptAutoPos(true)
{
	this->SupportNewLine( SupportNewLine );

	mPromtPosSupport = SupportFreeEditing;
	mMaxLenght = MaxLenght;
	SetAutoPromp();
}

cInputTextBuffer::cInputTextBuffer() :
	mActive(true),
	mFlags(0),
	mChangeSinceLastUpdate(false),
	mCallback(0),
	mPromtPosSupport(true),
	mMaxLenght(0xFFFFFFFF)
{
	SupportNewLine( false );
	SetAutoPromp();
}

cInputTextBuffer::~cInputTextBuffer() {
	if ( mCallback && NULL != cInput::ExistsSingleton() )
		cInput::instance()->PopCallback( mCallback );

	mText.clear();
}

void cInputTextBuffer::Start() {
	mCallback = cInput::instance()->PushCallback( cb::Make1( this, &cInputTextBuffer::Update ) );
}

void cInputTextBuffer::Update( EE_Event* Event ) {
	if ( mActive ) {
		mChangeSinceLastUpdate = false;
		Int32 c = convertKeyCharacter( Event );

		if ( mPromtPosSupport ) {
			switch(Event->type) {
				case SDL_KEYDOWN:
					if ( ( c == KEY_BACKSPACE || c == KEY_DELETE ) ) {
						if ( mText.size() ) {
							mChangeSinceLastUpdate = true;

							if ( mPromptPos < (eeInt)mText.size() ) {
								if ( c == KEY_BACKSPACE ) {
									if ( mPromptPos > 0 ) {
										mText.erase(mPromptPos-1,1);
										mPromptPos--;
									}
								} else
									mText.erase(mPromptPos,1);
							} else if ( c == KEY_BACKSPACE ) {
								mText.resize( mText.size() - 1 );
								SetAutoPromp();
							}
						}
					} else if ( ( c == KEY_RETURN || c == KEY_KP_ENTER ) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
						if ( SupportNewLine() && CanAdd() ) {
							InsertChar( mText, mPromptPos, L'\n' );

							mPromptPos++;

							mChangeSinceLastUpdate = true;
						}

						if ( mEnterCall.IsSet() )
							mEnterCall();

 					} else if ( c == KEY_LEFT ) {
						if ( ( mPromptPos - 1 ) >= 0 ) {
							mPromptPos--;
							SetAutoPromp( false );
						}
					} else if ( c == KEY_RIGHT ) {
						if ( ( mPromptPos + 1 ) < (eeInt)mText.size() ) {
							mPromptPos++;
							SetAutoPromp(false);
						} else if ( ( mPromptPos + 1 ) == (eeInt)mText.size() ) {
							SetAutoPromp();
						}
					} else if ( c == KEY_UP ) {
						MovePromptRowUp( false );
					} else if ( c == KEY_DOWN ) {
						MovePromptRowDown( true );
					} else if ( c == KEY_PAGEUP ) {
						MovePromptRowUp( true );
					} else if ( c == KEY_PAGEDOWN ) {
						MovePromptRowDown( false );
					} else if ( CanAdd() && isCharacter(c) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
						bool Ignored = false;

						if ( AllowOnlyNumbers() && !isNumber( c, AllowDotsInNumbers() ) ) {
							Ignored = true;
						}

						if ( mIgnoredChars.size() ) {
							for ( eeUint i = 0; i < mIgnoredChars.size(); i++ ) {
								if ( mIgnoredChars[i] == (Uint32)c )
									Ignored = true;
							}
						}

						if ( !Ignored ) {
							mChangeSinceLastUpdate = true;

							if ( mPromptAutoPos ) {
								mText += c;
								mPromptPos = (eeInt)mText.size();
							} else {
								InsertChar( mText, mPromptPos, c );
								mPromptPos++;
							}
						}
					}

					break;
				case SDL_KEYUP:
					if ( SupportNewLine() ) {
						if ( Event->key.keysym.sym == SDLK_END ) {
							for ( Uint32 i = mPromptPos; i < mText.size(); i++ )  {
								if ( mText[i] == L'\n' ) {
									mPromptPos = i;
									SetAutoPromp( false );
									break;
								}

								if ( i == ( mText.size() - 1 ) ) {
									mPromptPos = mText.size();
									SetAutoPromp( false );
								}
							}
						}

						if ( Event->key.keysym.sym == SDLK_HOME ) {
							if ( 0 != mPromptPos ) {
								for ( Int32 i = (Int32)mPromptPos - 1; i >= 0; i-- )  {
									if ( i >= 0 ) {
										if ( mText[i] == L'\n' ) {
											mPromptPos = i + 1;
											SetAutoPromp( false );
											break;
										}

										if ( i == 0 ) {
											mPromptPos = 0;
											SetAutoPromp( false );
										}
									}
								}
							}
						}
					} else {
						if ( Event->key.keysym.sym == SDLK_END ) {
							SetAutoPromp();
						}

						if ( Event->key.keysym.sym == SDLK_HOME ) {
							mPromptPos = 0;
							SetAutoPromp(false);
						}
					}
					break;
			}
		} else {
			if (Event->type == SDL_KEYDOWN) {
				mChangeSinceLastUpdate = true;

				if ( c == KEY_BACKSPACE && mText.size() > 0 ) {
					mText.resize( mText.size() - 1 );
				} else if ( (c == KEY_RETURN || c == KEY_KP_ENTER) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
					if ( SupportNewLine() && CanAdd() )
						mText += L'\n';

					if ( mEnterCall.IsSet() )
						mEnterCall();
				} else if ( CanAdd() && isCharacter(c) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
					if ( !( AllowOnlyNumbers() && !isNumber( c, AllowDotsInNumbers() ) ) ) {
						mText += c;
					}
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
			if ( mText[i] == L'\n' ) {
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

			SetAutoPromp( false );
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
				if ( mText[i] == L'\n' ) {
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

			SetAutoPromp( false );
		}
	}
}

void cInputTextBuffer::Clear() {
	mText.clear();
	SetAutoPromp();
}

void cInputTextBuffer::SetReturnCallback( EnterCallback EC ) {
	mEnterCall = EC;
}

bool cInputTextBuffer::ChangedSinceLastUpdate() {
	return mChangeSinceLastUpdate;
}

void cInputTextBuffer::ChangedSinceLastUpdate( const bool& Changed ) {
	mChangeSinceLastUpdate = Changed;
}

void cInputTextBuffer::SetAutoPromp( const bool& set ) {
	if ( set ) {
		mPromptAutoPos = true;
		mPromptPos = (eeInt)mText.size();
	} else {
		mPromptAutoPos = false;
	}
}

void cInputTextBuffer::Buffer( const std::wstring& str ) {
	if ( mText != str ) {
		mText = str;
		mChangeSinceLastUpdate = true;
	}
	
	if ( mPromtPosSupport )
		SetAutoPromp();
}

eeInt cInputTextBuffer::CurPos() const {
	return mPromptPos;
}

void cInputTextBuffer::CurPos( const Uint32& pos ) {
	if ( mPromtPosSupport && pos < mText.size() ) {
		mPromptPos = pos;
		SetAutoPromp(false);
	}
}

Uint32 cInputTextBuffer::GetCurPosLinePos( Uint32& LastNewLinePos ) {
	if ( mPromtPosSupport ) {
		Uint32 nl = 0;
		LastNewLinePos = 0;
		for ( eeInt i = 0; i < mPromptPos; i++ )  {
			if ( mText[i] == L'\n' ) {
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
	return mText.size() < mMaxLenght;
}

void cInputTextBuffer::MaxLenght( const Uint32& Max ) {
	mMaxLenght = Max;

	if ( mText.size() > mMaxLenght )
		mText.resize( mMaxLenght );
}

const Uint32& cInputTextBuffer::MaxLenght() const {
	return mMaxLenght;
}

std::wstring cInputTextBuffer::Buffer() const {
	return mText;
}

bool cInputTextBuffer::Active() const {
	return mActive;
}

void cInputTextBuffer::Active( const bool& Active ) {
	mActive = Active;
}

bool cInputTextBuffer::SupportNewLine() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_SUPPORT_NEW_LINE ) );
}

void cInputTextBuffer::SupportNewLine( const bool& SupportNewLine ) {
	Write32BitKey( &mFlags, INPUT_TB_SUPPORT_NEW_LINE, SupportNewLine == true );
}

void cInputTextBuffer::AllowOnlyNumbers( const bool& onlynums, const bool& allowdots ) {
	Write32BitKey( &mFlags, INPUT_TB_ALLOW_ONLY_NUMBERS, onlynums == true );
	Write32BitKey( &mFlags, INPUT_TB_ALLOW_DOT_IN_NUMBERS, allowdots == true );
}

bool cInputTextBuffer::AllowOnlyNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_ONLY_NUMBERS ) );
}

bool cInputTextBuffer::AllowDotsInNumbers() {
	return 0 != ( mFlags & ( 1 << INPUT_TB_ALLOW_DOT_IN_NUMBERS ) );
}

bool cInputTextBuffer::SupportFreeEditing() const {
	return mPromtPosSupport;
}

void cInputTextBuffer::SupportFreeEditing( const bool& SupportNewLine ) {
	mPromtPosSupport = SupportNewLine;
}

}}
