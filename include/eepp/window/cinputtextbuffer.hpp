#ifndef EE_WINDOWCINPUTTEXTBUFFER_H
#define EE_WINDOWCINPUTTEXTBUFFER_H

#include <eepp/window/base.hpp>
#include <eepp/window/cinput.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window {

enum INPUT_TEXTBUFFER_FLAGS {
	INPUT_TB_SUPPORT_NEW_LINE			= 0,
	INPUT_TB_ALLOW_ONLY_NUMBERS			= 1,
	INPUT_TB_ALLOW_DOT_IN_NUMBERS		= 2,
	INPUT_TB_ACTIVE						= 3,
	INPUT_TB_CHANGE_SINCE_LAST_UPDATE	= 4,
	INPUT_TB_FREE_EDITING				= 5,
	INPUT_TB_PROMPT_AUTO_POS			= 6,
	INPUT_TB_SUPPORT_COPY_PASTE			= 7,
	INPUT_TB_TEXT_SELECTION_ENABLED		= 8
};

#define INPUT_LENGHT_MAX 0xFFFFFFFF

/** @brief A class to keep a buffer of the user writed text */
class EE_API cInputTextBuffer {
	public:
		typedef cb::Callback0<void> EnterCallback;

		cInputTextBuffer( const bool& active, const bool& supportNewLine, const bool& supportFreeEditing, Window::cWindow * window = NULL, const Uint32& maxLength = INPUT_LENGHT_MAX );

		cInputTextBuffer( Window::cWindow * window = NULL );

		~cInputTextBuffer();

		/** @return The current buffer */
		String Buffer() const;

		/** Set a new current buffer */
		void Buffer( const String& str );

		/** @return If input buffer is active */
		bool Active() const;

		/** Set the state of the input buffer */
		void Active( const bool& Active );

		/** @return If new line is supported */
		bool SupportNewLine();

		/** Support new line consist of allowing to add a new line when key return is pressed. */
		void SupportNewLine( const bool& SupportNewLine );

		/** @return If Free Editing is enabled */
		bool SupportFreeEditing() const;

		/** Free editing consist on the capability of moving the cursor position over the buffer, to write over the buffer, and not only after the last character. */
		void SupportFreeEditing( const bool& Support);

		/** Block all the inserts, allow only numeric characters. */
		void AllowOnlyNumbers( const bool& onlynums, const bool& allowdots = false );

		/** @return If is only allowing numbers */
		bool AllowOnlyNumbers();

		/** @return If is only allowing numbers, it allow floating point numbers? */
		bool AllowDotsInNumbers();

		/** @return If text selection feature is enabled */
		bool TextSelectionEnabled();

		/** Enable text selection */
		void TextSelectionEnabled( const bool& enabled );

		/** Start the input buffer */
		void Start();

		/** Clear the buffer */
		void Clear();

		/** Internal callback, don't call it */
		void Update( InputEvent * Event );

		/** A callback for the key return */
		void SetReturnCallback( EnterCallback EC );

		/** @return If something changed since last update */
		bool ChangedSinceLastUpdate();

		/** Set if changed since last update */
		void ChangedSinceLastUpdate( const bool& Changed );

		/** @return The Cursor Position (where is the cursor editing) */
		int CurPos() const;

		/** Set the cursor position */
		void CurPos( const Uint32& pos );

		/** This function it's for helping the cFont class to locate the cursor position for the correct rendering of it.
		* @param LastNewLinePos This will return the position of the closest "\n" to the current Cursor Pos
		* @return On which line it's the cursor
		*/
		Uint32 GetCurPosLinePos( Uint32& LastNewLinePos );

		/** Push the char you want to ignore */
		void PushIgnoredChar( const Uint32& ch );

		/** Set the new max length */
		void MaxLength( const Uint32& Max );

		/** @return The Max Length */
		const Uint32& MaxLength() const;

		/** Support copy paste */
		void SupportCopyPaste( const bool& support );

		/** @return Support copy paste */
		bool SupportCopyPaste();

		/** Set the cursor to the last character of the buffer. */
		void CursorToEnd();

		/** Set the selection cursor initial position */
		void SelCurInit( const Int32& init );

		/** Set the selection cursor final position */
		void SelCurEnd( const Int32& end );

		/** @return The selection cursor initial position */
		const Int32& SelCurInit() const;

		/** @return The selection cursor final position */
		const Int32& SelCurEnd() const;
	protected:
		cWindow *			mWindow;
		String				mText;
		Uint32				mFlags;
		Uint32				mCallback;
		int				mPromptPos;
		EnterCallback		mEnterCall;
		Uint32				mMaxLength;
		std::vector<Uint32>	mIgnoredChars;
		Int32				mSelCurInit;
		Int32				mSelCurEnd;

		void AutoPrompt( const bool& set );

		bool AutoPrompt();

		bool CanAdd();

		void MovePromptRowDown( const bool& breakit );

		void MovePromptRowUp( const bool& breakit );

		void PromptToLeftFirstNoChar();

		void PromptToRightFirstNoChar();

		void EraseToPrevNoChar();

		void EraseToNextNoChar();

		bool IsIgnoredChar( const Uint32& c );

		bool ValidChar( const Uint32& c );

		void TryAddChar( const Uint32& c );

		void ShiftSelection( const int& lastPromtpPos );

		void RemoveSelection();

		void ResetSelection();
};

}}

#endif
