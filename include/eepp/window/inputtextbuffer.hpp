#ifndef EE_WINDOWCINPUTTEXTBUFFER_H
#define EE_WINDOWCINPUTTEXTBUFFER_H

#include <climits>
#include <eepp/window/base.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Window {

/** @brief A class to keep a buffer of the user writed text */
class EE_API InputTextBuffer {
  public:
	typedef std::function<void()> EnterCallback;
	typedef std::function<void()> CursorPositionChangeCallback;
	typedef std::function<void()> BufferChangeCallback;
	typedef std::function<void()> SelectionChangeCallback;

	static InputTextBuffer* New( const bool& active, const bool& newLineEnabled,
								 const bool& freeEditing, EE::Window::Window* window = NULL,
								 const Uint32& maxLength = UINT32_MAX );

	static InputTextBuffer* New( EE::Window::Window* window = NULL );

	InputTextBuffer( const bool& active, const bool& newLineEnabled, const bool& freeEditing,
					 EE::Window::Window* window = NULL, const Uint32& maxLength = UINT32_MAX );

	InputTextBuffer( EE::Window::Window* window = NULL );

	~InputTextBuffer();

	/** @return The current buffer */
	String getBuffer() const;

	/** Set a new current buffer */
	void setBuffer( const String& str );

	/** @return If input buffer is active */
	bool isActive() const;

	/** Set the state of the input buffer */
	void setActive( const bool& active );

	/** @return If new line is supported */
	bool setSupportNewLine();

	/** Support new line consist of allowing to add a new line when key return is pressed. */
	void isNewLineEnabled( const bool& enabled );

	/** @return If Free Editing is enabled */
	bool isFreeEditingEnabled() const;

	/** Free editing consist on the capability of moving the cursor position over the buffer, to
	 * write over the buffer, and not only after the last character. */
	void setFreeEditing( const bool& enabled );

	/** Block all the inserts, allow only numeric characters. */
	void setAllowOnlyNumbers( const bool& onlynums, const bool& allowdots = false );

	/** @return If is only allowing numbers */
	bool onlyNumbersAllowed();

	/** @return If is only allowing numbers, it allow floating point numbers? */
	bool dotsInNumbersAllowed();

	/** @return If text selection feature is enabled */
	bool isTextSelectionEnabled();

	/** Enable text selection */
	void setTextSelectionEnabled( const bool& enabled );

	/** Start the input buffer */
	void start();

	/** Clear the buffer */
	void clear();

	/** Internal callback, don't call it */
	void update( InputEvent* Event );

	/** A callback for the key return */
	void setReturnCallback( EnterCallback EC );

	/** @return If something changed since last update */
	bool changedSinceLastUpdate();

	/** Set if changed since last update */
	void setChangedSinceLastUpdate( const bool& Changed );

	/** @return The Cursor Position (where is the cursor editing) */
	int getCursorPosition() const;

	/** Set the cursor position */
	void setCursorPosition( const Uint32& pos );

	/** This function locates the cursor line position for the correct rendering of it.
	 * @param LastNewLinePos This will return the position of the closest "\n" to the current Cursor
	 * Pos
	 * @return On which line is the cursor
	 */
	Uint32 getCurPosLinePos( Uint32& LastNewLinePos );

	/** Push the char you want to ignore */
	void pushIgnoredChar( const Uint32& ch );

	/** Set the new max length */
	void setMaxLength( const Uint32& Max );

	/** @return The Max Length */
	const Uint32& getMaxLength() const;

	/** Support copy paste */
	void supportCopyPaste( const bool& support );

	/** @return Support copy paste */
	bool supportCopyPaste();

	/** Set the cursor to the last character of the buffer. */
	void cursorToEnd();

	/** Set the selection cursor initial position */
	void selCurInit( const Int32& init );

	/** Set the selection cursor final position */
	void selCurEnd( const Int32& end );

	/** @return The selection cursor initial position */
	const Int32& selCurInit() const;

	/** @return The selection cursor final position */
	const Int32& selCurEnd() const;

	/** Event callback when the cursor position changes. */
	void setCursorPositionChangeCallback(
		const CursorPositionChangeCallback& cursorPositionChangeCallback );

	/** Event callback when the text buffer changes. */
	void setBufferChangeCallback( const BufferChangeCallback& bufferChangeCallback );

	/** Event callback when the selection changes. */
	void setSelectionChangeCallback( const SelectionChangeCallback& selectionChangeCallback );

	void selectAll();
  protected:
	enum Flags {
		SUPPORT_NEW_LINE = 0,
		ALLOW_ONLY_NUMBERS = 1,
		ALLOW_DOT_IN_NUMBERS = 2,
		ACTIVE = 3,
		CHANGE_SINCE_LAST_UPDATE = 4,
		FREE_EDITING = 5,
		PROMPT_AUTO_POS = 6,
		SUPPORT_COPY_PASTE = 7,
		TEXT_SELECTION_ENABLED = 8
	};

	EE::Window::Window* mWindow;
	String mText;
	Uint32 mFlags;
	Uint32 mCallback;
	int mPromptPos;
	EnterCallback mEnterCall;
	CursorPositionChangeCallback mCursorPositionChangeCallback;
	BufferChangeCallback mBufferChangeCallback;
	SelectionChangeCallback mSelectionChangeCallback;
	Uint32 mMaxLength;
	std::vector<Uint32> mIgnoredChars;
	Int32 mSelCurInit;
	Int32 mSelCurEnd;

	void autoPrompt( const bool& set );

	bool autoPrompt();

	bool canAdd();

	void movePromptRowDown( const bool& breakit );

	void movePromptRowUp( const bool& breakit );

	void promptToLeftFirstNoChar();

	void promptToRightFirstNoChar();

	void eraseToPrevNoChar();

	void eraseToNextNoChar();

	bool isIgnoredChar( const String::StringBaseType& c );

	bool validChar(const String::StringBaseType& c );

	void tryAddChar( const String::StringBaseType& c );

	void shiftSelection( const int& lastPromtpPos );

	void removeSelection();

	void resetSelection();

	void onCursorPositionChange();

	void onSelectionChange();

	void onBufferChange();

	/** Set the cursor position */
	void setCursorPos( const Uint32& pos );
};

}} // namespace EE::Window

#endif
