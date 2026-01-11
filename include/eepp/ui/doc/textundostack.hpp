#ifndef EE_UI_DOC_TEXTUNDOSTACK_HPP
#define EE_UI_DOC_TEXTUNDOSTACK_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/doc/textrange.hpp>

#include <eepp/thirdparty/nlohmann/json_fwd.hpp>

#include <deque>
#include <variant>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class TextDocument;
class TextUndoCommand;

enum class TextUndoCommandType { Insert = 1, Remove = 2, Selection = 3 };

class TextUndoCommand {
  public:
	TextUndoCommand( const Uint64& id, const TextUndoCommandType& type, const Time& timestamp );

	virtual ~TextUndoCommand();

	const Uint64& getId() const;

	const TextUndoCommandType& getType() const;

	const Time& getTimestamp() const;

	virtual nlohmann::json toJSON() const = 0;

  protected:
	Uint64 mId;
	TextUndoCommandType mType;
	Time mTimestamp;

	nlohmann::json baseJSON() const;
};

class TextUndoCommandInsert : public TextUndoCommand {
  public:
	TextUndoCommandInsert( const Uint64& id, const size_t& cursorIdx, const String& text,
						   const TextPosition& position, const Time& timestamp );

	const String& getText() const;

	const TextPosition& getPosition() const;

	size_t getCursorIdx() const;

	nlohmann::json toJSON() const;

	static TextUndoCommandInsert fromJSON( nlohmann::json j, Uint64 id );

  protected:
	String mText;
	TextPosition mPosition;
	size_t mCursorIdx;
};

class TextUndoCommandRemove : public TextUndoCommand {
  public:
	TextUndoCommandRemove( const Uint64& id, const size_t& cursorIdx, const TextRange& range,
						   const Time& timestamp );

	const TextRange& getRange() const;

	size_t getCursorIdx() const;

	nlohmann::json toJSON() const;

	static TextUndoCommandRemove fromJSON( nlohmann::json j, Uint64 id );

  protected:
	TextRange mRange;
	size_t mCursorIdx;
};

class TextUndoCommandSelection : public TextUndoCommand {
  public:
	TextUndoCommandSelection( const Uint64& id, const size_t& cursorIdx,
							  const TextRanges& selection, const Time& timestamp );

	const TextRanges& getSelection() const;

	size_t getCursorIdx() const;

	nlohmann::json toJSON() const;

	static TextUndoCommandSelection fromJSON( nlohmann::json j, Uint64 id );

  protected:
	TextRanges mSelection;
	size_t mCursorIdx;
};

using UndoCommandVariant =
	std::variant<TextUndoCommandInsert, TextUndoCommandRemove, TextUndoCommandSelection>;

using UndoStackContainer = std::deque<UndoCommandVariant>;

class EE_API TextUndoStack {
  public:
	TextUndoStack( TextDocument* owner, const Uint32& maxStackSize = 20000 );

	~TextUndoStack();

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

	std::string toJSON( bool inverted );

	void fromJSON( const std::string& json );

  protected:
	friend class TextDocument;

	TextDocument* mDoc;
	Uint32 mMaxStackSize;
	Uint64 mChangeIdCounter;
	UndoStackContainer mUndoStack;
	UndoStackContainer mRedoStack;
	Time mMergeTimeout;

	void pushUndo( UndoStackContainer& undoStack, UndoCommandVariant&& cmd );

	void pushInsert( UndoStackContainer& undoStack, const String& string, const size_t& cursorIdx,
					 const TextPosition& position, const Time& time );

	void pushRemove( UndoStackContainer& undoStack, const size_t& cursorIdx, const TextRange& range,
					 const Time& time );

	void pushSelection( UndoStackContainer& undoStack, const size_t& cursorIdx,
						const TextRanges& selection, const Time& time );

	UndoStackContainer& getUndoStackContainer();

	UndoStackContainer& getRedoStackContainer();

	void popUndo( UndoStackContainer& undoStack, UndoStackContainer& redoStack );

	void limitStackSize( UndoStackContainer& stack );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_UNDOSTACK_HPP
