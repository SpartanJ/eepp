#ifndef EE_UI_UNDOSTACK_HPP
#define EE_UI_UNDOSTACK_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/noncopyable.hpp>

#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace EE { namespace UI {

// Absolutely inspired by Qt implementation

class EE_API UndoCommand : public NonCopyable {
  public:
	explicit UndoCommand( UndoCommand* parent = nullptr );
	explicit UndoCommand( const std::string& text, UndoCommand* parent = nullptr );
	virtual ~UndoCommand();

	virtual void undo();

	virtual void redo();

	virtual int id() const;
	virtual bool mergeWith( const UndoCommand* other );

	std::string text() const;
	std::string actionText() const;
	void setText( const std::string& text );

	bool isObsolete() const;
	void setObsolete( bool obsolete );

	int childCount() const;
	const UndoCommand* child( int index ) const;

  protected:
	friend class UndoStack;

	std::vector<UndoCommand*> mChilds;
	std::string mText;
	std::string mActionText;
	int mId{ -1 };
	bool mObsolete{ false };
};

class EE_API UndoStack : public NonCopyable {
  public:
	UndoStack() : mIndex( 0 ), mCleanIndex( 0 ), mUndoLimit( 0 ) {}

	~UndoStack();

	void clear();

	void push( UndoCommand* cmd );

	bool canUndo() const;
	bool canRedo() const;
	std::string undoText() const;
	std::string redoText() const;

	int count() const;
	int index() const;
	std::string text( int idx ) const;

	bool isClean() const;
	int cleanIndex() const;

	void beginMacro( const std::string& text );
	void endMacro();

	void setUndoLimit( int limit );
	int undoLimit() const;

	const UndoCommand* command( int index ) const;

	void setClean();
	void resetClean();
	void setIndex( int idx );
	void undo();
	void redo();

	enum EventType {
		IndexChanged,
		CleanChanged,
		CanUndoChanged,
		CanRedoChanged,
		UndoTextChanged,
		RedoTextChanged,
	};

	class EventIndexChanged;
	class EventCleanChanged;
	class EventCanUndoChanged;
	class EventCanRedoChanged;
	class EventUndoTextChanged;
	class EventRedoTextChanged;

	using EventId = Uint32;

	class Event {
	  public:
		Event( EventType type ) : type( type ) {}

		EventId getCallbackId() const { return cbId; }

		const EventType& getType() const { return type; }

		const EventIndexChanged* asIndexChanged() const {
			return static_cast<const EventIndexChanged*>( this );
		}

		const EventCleanChanged* asCleanChanged() const {
			return static_cast<const EventCleanChanged*>( this );
		}

		const EventCanUndoChanged* asCanUndoChanged() const {
			return static_cast<const EventCanUndoChanged*>( this );
		}

		const EventCanRedoChanged* asCanRedoChanged() const {
			return static_cast<const EventCanRedoChanged*>( this );
		}

		const EventUndoTextChanged* asUndoTextChanged() const {
			return static_cast<const EventUndoTextChanged*>( this );
		}

		const EventRedoTextChanged* asRedoTextChanged() const {
			return static_cast<const EventRedoTextChanged*>( this );
		}

	  protected:
		friend class UndoStack;
		EventType type;
		EventId cbId;
	};

	class EventIndexChanged : public Event {
	  public:
		EventIndexChanged( int idx ) : Event( EventType::IndexChanged ), mIndex( idx ) {}

		int index() const { return mIndex; }

	  protected:
		int mIndex;
	};

	class EventCleanChanged : public Event {
	  public:
		EventCleanChanged( bool clean ) : Event( EventType::CleanChanged ), mClean( clean ) {}

		bool clean() const { return mClean; }

	  protected:
		bool mClean;
	};

	class EventCanUndoChanged : public Event {
	  public:
		EventCanUndoChanged( bool canUndo ) :
			Event( EventType::CanUndoChanged ), mCanUndo( canUndo ) {}

		bool canUndo() const { return mCanUndo; }

	  protected:
		bool mCanUndo;
	};

	class EventCanRedoChanged : public Event {
	  public:
		EventCanRedoChanged( bool canRedo ) :
			Event( EventType::CanRedoChanged ), mCanRedo( canRedo ) {}

		bool canRedo() const { return mCanRedo; }

	  protected:
		bool mCanRedo;
	};

	class EventUndoTextChanged : public Event {
	  public:
		EventUndoTextChanged( const std::string& undoText ) :
			Event( EventType::UndoTextChanged ), mUndoText( undoText ) {}

		const std::string& undoText() const { return mUndoText; }

	  protected:
		std::string mUndoText;
	};

	class EventRedoTextChanged : public Event {
	  public:
		EventRedoTextChanged( const std::string& RedoText ) :
			Event( EventType::RedoTextChanged ), mRedoText( RedoText ) {}

		const std::string& RedoText() const { return mRedoText; }

	  protected:
		std::string mRedoText;
	};

	using EventCb = std::function<void( const Event* event )>;

	EventId addEventListener( EventType type, EventCb cb );

	EventId on( EventType type, EventCb cb );

	void removeEventListener( EventId id );

  protected:
	std::deque<UndoCommand*> mCommands;
	std::deque<UndoCommand*> mMacroStack;
	UnorderedMap<EventType, std::map<EventId, EventCb>> mEventsCbs;
	EventId mNextEventId{ 0 };
	int mIndex;
	int mCleanIndex;
	int mUndoLimit;

	bool checkUndoLimit();

	void setIndex( int idx, bool clean );

	void sendEvent( const Event* event );
	void indexChanged( int idx );
	void cleanChanged( bool clean );
	void canUndoChanged( bool canUndo );
	void canRedoChanged( bool canRedo );
	void undoTextChanged( const std::string& undoText );
	void redoTextChanged( const std::string& redoText );
};

}} // namespace EE::UI

#endif // EE_UI_UNDOSTACK_HPP
