#ifndef EE_UI_DOC_UNDOSTACK_HPP
#define EE_UI_DOC_UNDOSTACK_HPP

#include <deque>
#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/doc/textrange.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class TextDocument;

enum class TextUndoCommandType { Insert, Remove, Selection };

class EE_API TextUndoCommand {
  public:
	TextUndoCommand( const Uint64& id, const TextUndoCommandType& type, const Time& timestamp );

	virtual ~TextUndoCommand();

	const Uint64& getId() const;

	const TextUndoCommandType& getType() const;

	const Time& getTimestamp() const;

  protected:
	Uint64 mId;
	TextUndoCommandType mType;
	Time mTimestamp;
};

class EE_API TextUndoCommandInsert : public TextUndoCommand {
  public:
	TextUndoCommandInsert( const Uint64& id, const size_t& cursorIdx, const String& text,
						   const TextPosition& position, const Time& timestamp );

	const String& getText() const;

	const TextPosition& getPosition() const;

	size_t getCursorIdx() const;

  protected:
	String mText;
	TextPosition mPosition;
	size_t mCursorIdx;
};

class EE_API TextUndoCommandRemove : public TextUndoCommand {
  public:
	TextUndoCommandRemove( const Uint64& id, const size_t& cursorIdx, const TextRange& range,
						   const Time& timestamp );

	const TextRange& getRange() const;

	size_t getCursorIdx() const;

  protected:
	TextRange mRange;
	size_t mCursorIdx;
};

class EE_API TextUndoCommandSelection : public TextUndoCommand {
  public:
	TextUndoCommandSelection( const Uint64& id, const size_t& cursorIdx,
							  const TextRanges& selection, const Time& timestamp );

	const TextRanges& getSelection() const;

	size_t getCursorIdx() const;

  protected:
	TextRanges mSelection;
	size_t mCursorIdx;
};

using UndoStackContainer = std::deque<TextUndoCommand*>;

class EE_API UndoStack {
  public:
	UndoStack( TextDocument* owner, const Uint32& maxStackSize = 10000 );

	~UndoStack();

	void clear();

	void clearUndoStack();

	void clearRedoStack();

	void undo();

	void redo();

	bool hasUndo() const;

	bool hasRedo() const;

	const Uint32& getMaxStackSize() const;

	const Time& getMergeTimeout() const;

	void setMergeTimeout( const Time& mergeTimeout );

	Uint64 getCurrentChangeId() const;

  protected:
	friend class TextDocument;

	TextDocument* mDoc;
	Uint32 mMaxStackSize;
	Uint64 mChangeIdCounter;
	UndoStackContainer mUndoStack;
	UndoStackContainer mRedoStack;
	Time mMergeTimeout;

	void pushUndo( UndoStackContainer& undoStack, TextUndoCommand* cmd );

	void pushInsert( UndoStackContainer& undoStack, const String& string, const size_t& cursorIdx,
					 const TextPosition& position, const Time& time );

	void pushRemove( UndoStackContainer& undoStack, const size_t& cursorIdx, const TextRange& range,
					 const Time& time );

	void pushSelection( UndoStackContainer& undoStack, const size_t& cursorIdx,
						const TextRanges& selection, const Time& time );

	UndoStackContainer& getUndoStackContainer();

	UndoStackContainer& getRedoStackContainer();

	void popUndo( UndoStackContainer& undoStack, UndoStackContainer& redoStack );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_UNDOSTACK_HPP
