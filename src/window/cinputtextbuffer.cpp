#include "cinputtextbuffer.hpp"

namespace EE { namespace Window {

cInputTextBuffer::cInputTextBuffer( const bool& Active, const bool& SupportNewLine, const bool& SupportFreeEditing, const Uint32& MaxLenght ) :
	mChangeSinceLastUpdate(false),
	mCallback(0),
	mPromptPos(0),
	mPromptAutoPos(true),
	mEnter(false)
{
	mActive = Active;
	mSupportNewLine = SupportNewLine;
	mPromtPosSupport = SupportFreeEditing;
	mMaxLenght = MaxLenght;
	SetAutoPromp();
}

cInputTextBuffer::cInputTextBuffer() :
	mActive(true),
	mSupportNewLine(true),
	mChangeSinceLastUpdate(false),
	mCallback(0),
	mPromtPosSupport(true),
	mEnter(false),
	mMaxLenght(0xFFFFFFFF)
{
	SetAutoPromp();
}

cInputTextBuffer::~cInputTextBuffer() {
	if ( mCallback && NULL != cInput::ExistsSingleton() )
		cInput::instance()->PopCallback( mCallback );

	mText.clear();
}

void cInputTextBuffer::Start() {
	mCallback = cInput::instance()->PushCallback( std::bind1st( std::mem_fun(&cInputTextBuffer::Update), this) );
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
						if ( mSupportNewLine && CanAdd() ) {
							InsertChar( mText, mPromptPos, L'\n' );
							mPromptPos++;
						}

						if ( mEnter )
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
					} else if ( CanAdd() && isCharacter(c) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
						bool Ignored = false;

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
								mPromptPos = mText.size();
							} else {
								InsertChar( mText, mPromptPos, c );
								mPromptPos++;
							}
						}
					}

					break;
				case SDL_KEYUP:
					if ( Event->key.keysym.sym == SDLK_END ) {
						SetAutoPromp();
					}

					if ( Event->key.keysym.sym == SDLK_HOME ) {
						mPromptPos = 0;
						SetAutoPromp(false);
					}
					break;
			}
		} else {
			if (Event->type == SDL_KEYDOWN) {
				mChangeSinceLastUpdate = true;

				if ( c == KEY_BACKSPACE && mText.size() > 0 ) {
					mText.resize( mText.size() - 1 );
				} else if ( (c == KEY_RETURN || c == KEY_KP_ENTER) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
					if ( mSupportNewLine && CanAdd() )
						mText += L'\n';

					if ( mEnter )
						mEnterCall();
				} else if ( CanAdd() && isCharacter(c) && !cInput::instance()->MetaPressed() && !cInput::instance()->AltPressed() && !cInput::instance()->ControlPressed() ) {
					mText += c;
				}
			}
		}
	}
}

void cInputTextBuffer::Clear() {
	mText.clear();
	SetAutoPromp();
}

void cInputTextBuffer::SetReturnCallback( EnterCallback EC ) {
	mEnter = true;
	mEnterCall = EC;
}

bool cInputTextBuffer::ChangedSinceLastUpdate() {
	return mChangeSinceLastUpdate;
}

void cInputTextBuffer::SetAutoPromp( const bool& set ) {
	if ( set ) {
		mPromptAutoPos = true;
		mPromptPos = mText.size();
	} else {
		mPromptAutoPos = false;
	}
}

void cInputTextBuffer::Buffer( const std::wstring& str ) {
	mText = str;
	mChangeSinceLastUpdate = true;

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

}}
