#ifndef EE_UI_UICOMMAND_HPP
#define EE_UI_UICOMMAND_HPP

#include <atomic>
#include <eepp/core/observablevalue.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

template <typename Target> class UICommandShortcutBinding;

/**
 * @brief One action shared by UI endpoints and keyboard shortcuts.
 *
 * @code
 * auto save = bindCommand(
 *     [&] { saveDocument(); }, canSave, *saveButton, *uiScene,
 *     { KEY_S, KeyMod::getDefaultModifier() } );
 * @endcode
 *
 * For a single button with no other representation, an ordinary onClick() callback remains the
 * clearer choice.
 */
class UICommand {
  private:
	struct SourceConnectionBase {
		virtual ~SourceConnectionBase() = default;
	};

	template <typename Connection> struct SourceConnection final : SourceConnectionBase {
		explicit SourceConnection( Connection connection ) :
			connection( std::move( connection ) ) {}
		Connection connection;
	};

	struct State {
		explicit State( std::function<void()> execute ) : execute( std::move( execute ) ) {}
		bool tryExecute() {
			if ( !enabled.get() || executing )
				return false;
			executing = true;
			execute();
			executing = false;
			return true;
		}
		std::function<void()> execute;
		ObservableValue<bool> enabled{ true };
		// Enabled sources expose different scoped connection classes. Erasure happens once when
		// constructing the command and has no execution- or notification-path cost.
		std::unique_ptr<SourceConnectionBase> enabledSourceConnection;
		bool executing{ false };
	};

  public:
	/** @brief Creates an always-enabled command that invokes @p execute. */
	explicit UICommand( std::function<void()> execute ) :
		mState( std::make_shared<State>( std::move( execute ) ) ) {}
	template <typename Source>
	/**
	 * @brief Creates a command whose enabled state follows @p enabled.
	 *
	 * The source must expose get() and observe() for bool values and must outlive the command if
	 * further enabled-state updates are expected.
	 */
	UICommand( std::function<void()> execute, Source& enabled ) :
		mState( std::make_shared<State>( std::move( execute ) ) ) {
		mState->enabled = enabled.get();
		std::weak_ptr<State> weakState = mState;
		auto enabledConnection = enabled.observe( [weakState]( const bool& value ) {
			if ( auto state = weakState.lock() )
				state->enabled = value;
		} );
		mState->enabledSourceConnection =
			std::make_unique<SourceConnection<decltype( enabledConnection )>>(
				std::move( enabledConnection ) );
	}
	UICommand( const UICommand& ) = delete;
	UICommand& operator=( const UICommand& ) = delete;
	UICommand( UICommand&& ) noexcept = default;
	UICommand& operator=( UICommand&& ) noexcept = default;

	/**
	 * @brief Attempts to run the action.
	 * @return true when it ran, or false when disabled or already executing.
	 */
	bool execute() {
		auto state = mState;
		return state && state->tryExecute();
	}

	/** @return Observable enabled state shared by every endpoint bound to this command. */
	ObservableValue<bool>& enabled() { return mState->enabled; }

  private:
	std::shared_ptr<State> mState;
	friend class UICommandBinding;
	template <typename Target> friend class UICommandShortcutBinding;
};

/** @brief Scoped synchronization of a command with one widget endpoint. */
class UICommandBinding {
  public:
	UICommandBinding() = default;

	/**
	 * @brief Binds @p widget clicks and enabled state to @p command.
	 *
	 * Keep this object alive while the endpoint should remain active. The widget is not retained.
	 */
	UICommandBinding( UICommand& command, UIWidget& widget ) {
		auto commandState = command.mState;
		auto state = std::make_shared<BindingState>();
		state->widget = &widget;
		widget.setEnabled( commandState->enabled.get() );
		std::weak_ptr<UICommand::State> weakCommand = commandState;
		std::weak_ptr<BindingState> weakBinding = state;
		state->connections += widget.connect( Event::MouseClick, [weakCommand]( const Event* ) {
			if ( auto command = weakCommand.lock() )
				command->tryExecute();
		} );
		state->connections += widget.connect( Event::OnClose, [weakBinding]( const Event* ) {
			if ( auto binding = weakBinding.lock() ) {
				binding->widget = nullptr;
				binding->enabledConnection.disconnect();
			}
		} );
		state->enabledConnection =
			commandState->enabled.observe( [weakBinding]( const bool& enabled ) {
				if ( auto binding = weakBinding.lock(); binding && binding->widget )
					binding->widget->setEnabled( enabled );
			} );
		mState = std::move( state );
	}
	UICommandBinding( const UICommandBinding& ) = delete;
	UICommandBinding& operator=( const UICommandBinding& ) = delete;
	UICommandBinding( UICommandBinding&& ) noexcept = default;
	UICommandBinding& operator=( UICommandBinding&& ) noexcept = default;

  private:
	struct BindingState {
		UIWidget* widget{ nullptr };
		EventConnectionList connections;
		ObservableValue<bool>::Connection enabledConnection;
	};
	std::shared_ptr<BindingState> mState;
};

/** @return A scoped binding between an existing command and one clickable widget. */
inline UICommandBinding bindCommand( UICommand& command, UIWidget& widget ) {
	return UICommandBinding( command, widget );
}

/**
 * @brief Scoped binding of a command to a shortcut consumed by UISceneNode or UIWindow.
 *
 * The previous command mapped to the shortcut is restored when this binding disconnects. The
 * generated command registration and shortcut are removed safely when the target closes first.
 */
template <typename Target> class UICommandShortcutBinding {
  public:
	UICommandShortcutBinding() = default;

	/**
	 * @brief Registers @p shortcut on @p target as another endpoint for @p command.
	 *
	 * If the shortcut was already mapped, that mapping is restored when this binding disconnects.
	 */
	UICommandShortcutBinding( UICommand& command, Target& target,
							  const KeyBindings::Shortcut& shortcut ) {
		auto state = std::make_shared<State>();
		state->target = &target;
		state->shortcut = shortcut;
		state->previousCommand = target.getKeyBindings().getCommandFromKeyBind( shortcut );
		state->commandName = "eepp-ui-command-" + String::toString( ++sNextCommandId );
		std::weak_ptr<UICommand::State> weakCommand = command.mState;
		target.setKeyBindingCommand( state->commandName, [weakCommand] {
			if ( auto command = weakCommand.lock() )
				command->tryExecute();
		} );
		target.getKeyBindings().addKeybind( shortcut, state->commandName );
		std::weak_ptr<State> weakState = state;
		state->targetConnection = target.connect( Event::OnClose, [weakState]( const Event* ) {
			if ( auto state = weakState.lock() )
				state->target = nullptr;
		} );
		mState = std::move( state );
	}
	~UICommandShortcutBinding() { disconnect(); }
	UICommandShortcutBinding( const UICommandShortcutBinding& ) = delete;
	UICommandShortcutBinding& operator=( const UICommandShortcutBinding& ) = delete;
	UICommandShortcutBinding( UICommandShortcutBinding&& other ) noexcept :
		mState( std::move( other.mState ) ) {}
	UICommandShortcutBinding& operator=( UICommandShortcutBinding&& other ) noexcept {
		if ( this != &other ) {
			disconnect();
			mState = std::move( other.mState );
		}
		return *this;
	}

	/** @brief Removes this shortcut endpoint and restores any previous mapping. */
	void disconnect() {
		if ( !mState )
			return;
		if ( mState->target ) {
			auto& keyBindings = mState->target->getKeyBindings();
			if ( keyBindings.getCommandFromKeyBind( mState->shortcut ) == mState->commandName ) {
				keyBindings.removeKeybind( mState->shortcut );
				if ( !mState->previousCommand.empty() )
					keyBindings.addKeybind( mState->shortcut, mState->previousCommand );
			}
			mState->target->removeKeyBindingCommand( mState->commandName );
		}
		mState.reset();
	}
	explicit operator bool() const { return mState && mState->target; }

  private:
	struct State {
		Target* target{ nullptr };
		KeyBindings::Shortcut shortcut;
		std::string commandName;
		std::string previousCommand;
		EventConnection targetConnection;
	};
	inline static std::atomic<Uint64> sNextCommandId{ 0 };
	std::shared_ptr<State> mState;
};

template <typename Target>
/** @return A scoped binding between an existing command and a keyboard shortcut. */
UICommandShortcutBinding<Target> bindCommand( UICommand& command, Target& target,
											  const KeyBindings::Shortcut& shortcut ) {
	return UICommandShortcutBinding<Target>( command, target, shortcut );
}

/**
 * @brief Owns a command together with its primary widget and shortcut bindings.
 *
 * This is the concise form for the common case where an action is exposed by one clickable widget
 * and one default shortcut. Keep the returned object alive for as long as both bindings are needed.
 */
template <typename ShortcutTarget> class UICommandBindingSet {
  public:
	/** @brief Creates an always-enabled command with widget and shortcut endpoints. */
	UICommandBindingSet( std::function<void()> execute, UIWidget& widget,
						 ShortcutTarget& shortcutTarget, const KeyBindings::Shortcut& shortcut ) :
		mCommand( std::move( execute ) ),
		mWidgetBinding( mCommand, widget ),
		mShortcutBinding( mCommand, shortcutTarget, shortcut ) {}

	template <typename Source>
	/** @brief Creates a command following @p enabled with widget and shortcut endpoints. */
	UICommandBindingSet( std::function<void()> execute, Source& enabled, UIWidget& widget,
						 ShortcutTarget& shortcutTarget, const KeyBindings::Shortcut& shortcut ) :
		mCommand( std::move( execute ), enabled ),
		mWidgetBinding( mCommand, widget ),
		mShortcutBinding( mCommand, shortcutTarget, shortcut ) {}

	UICommandBindingSet( const UICommandBindingSet& ) = delete;
	UICommandBindingSet& operator=( const UICommandBindingSet& ) = delete;
	UICommandBindingSet( UICommandBindingSet&& ) noexcept = default;
	UICommandBindingSet& operator=( UICommandBindingSet&& ) noexcept = default;

	/** @return The owned command for explicit execution or additional endpoint bindings. */
	UICommand& command() { return mCommand; }

	/** @return The owned command. */
	const UICommand& command() const { return mCommand; }

  private:
	UICommand mCommand;
	UICommandBinding mWidgetBinding;
	UICommandShortcutBinding<ShortcutTarget> mShortcutBinding;
};

template <typename ShortcutTarget>
/**
 * @brief Creates an always-enabled command with a primary widget and shortcut.
 * @return A scoped object that owns the command and both endpoint bindings.
 */
UICommandBindingSet<ShortcutTarget> bindCommand( std::function<void()> execute, UIWidget& widget,
												 ShortcutTarget& shortcutTarget,
												 const KeyBindings::Shortcut& shortcut ) {
	return UICommandBindingSet<ShortcutTarget>( std::move( execute ), widget, shortcutTarget,
												shortcut );
}

template <typename Source, typename ShortcutTarget>
/**
 * @brief Creates a conditionally enabled command with a primary widget and shortcut.
 * @return A scoped object that owns the command and both endpoint bindings.
 */
UICommandBindingSet<ShortcutTarget> bindCommand( std::function<void()> execute, Source& enabled,
												 UIWidget& widget, ShortcutTarget& shortcutTarget,
												 const KeyBindings::Shortcut& shortcut ) {
	return UICommandBindingSet<ShortcutTarget>( std::move( execute ), enabled, widget,
												shortcutTarget, shortcut );
}

}} // namespace EE::UI

#endif
