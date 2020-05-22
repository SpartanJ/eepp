#ifndef EE_UI_DOC_UNDOSTACK_HPP
#define EE_UI_DOC_UNDOSTACK_HPP

#include <deque>
#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <memory>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class TextDocument;

enum class TextUndoCommandType { Insert, Remove, Selection };

class EE_API TextUndoCommand {
  public:
	TextUndoCommand( const TextUndoCommandType& type, const Time& timestamp );

	virtual ~TextUndoCommand();

	const TextUndoCommandType& getType() const;

	const Time& getTimestamp() const;

  protected:
	TextUndoCommandType mType;
	Time mTimestamp;
};

class EE_API TextUndoCommandInsert : public TextUndoCommand {
  public:
	TextUndoCommandInsert( const String& text, const TextPosition& position,
						   const Time& timestamp );

	const String& getText() const;

	const TextPosition& getPosition() const;

  protected:
	String mText;
	TextPosition mPosition;
};

class EE_API TextUndoCommandRemove : public TextUndoCommand {
  public:
	TextUndoCommandRemove( const TextRange& range, const Time& timestamp );

	const TextRange& getRange() const;

  protected:
	TextRange mRange;
};

class EE_API TextUndoCommandSelection : public TextUndoCommand {
  public:
	TextUndoCommandSelection( const TextRange& selection, const Time& timestamp );

	const TextRange& getSelection() const;

  protected:
	TextRange mSelection;
};

using UndoStackContainer = std::deque<std::unique_ptr<TextUndoCommand>>;

class EE_API UndoStack {
  public:
	UndoStack( TextDocument* owner, const Uint32& maxStackSize = 1000 );

	void clearRedoStack();

	void undo();

	void redo();

	const Uint32& getMaxStackSize() const;

	const Time& getMergeTimeout() const;

	void setMergeTimeout( const Time& mergeTimeout );

  protected:
	friend class TextDocument;

	TextDocument* mDoc;
	Uint32 mMaxStackSize;
	UndoStackContainer mUndoStack;
	UndoStackContainer mRedoStack;
	Time mMergeTimeout;

	void pushUndo( UndoStackContainer& undoStack, std::unique_ptr<TextUndoCommand>&& cmd );

	void pushInsert( UndoStackContainer& undoStack, const String& string,
					 const TextPosition& position, const Time& time );

	void pushRemove( UndoStackContainer& undoStack, const TextRange& range, const Time& time );

	void pushSelection( UndoStackContainer& undoStack, const TextRange& selection,
						const Time& time );

	UndoStackContainer& getUndoStackContainer();

	UndoStackContainer& getRedoStackContainer();

	void popUndo( UndoStackContainer& undoStack, UndoStackContainer& redoStack );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_UNDOSTACK_HPP
