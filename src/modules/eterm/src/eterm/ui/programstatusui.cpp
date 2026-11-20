#include <eterm/ui/programstatusui.hpp>
#include <eterm/ui/uiterminal.hpp>

#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/window/window.hpp>

namespace eterm::UI {

using namespace EE;
using namespace EE::UI;
using namespace eterm::Terminal;

namespace {

/** How a tab shows a program status. Each style class has one icon, and none means no status. */
struct ProgramStatusVisual {
	const char* icon{ nullptr };
	const char* styleClass{ nullptr };
};

constexpr ProgramStatusVisual ProgramStatusWorking{ "loader-2", "program_status_working" };
constexpr ProgramStatusVisual ProgramStatusBlocked{ "warning", "program_status_blocked" };
constexpr ProgramStatusVisual ProgramStatusDone{ "ok", "program_status_done" };
constexpr ProgramStatusVisual ProgramStatusError{ "error", "program_status_error" };
constexpr ProgramStatusVisual ProgramStatusPaused{ "pause-fill", "program_status_paused" };

constexpr const char* ProgramStatusProgressClass = "program_status_progress";
constexpr const char* ProgramStatusUnreadClass = "program_status_unread";

constexpr size_t MaxProgramStatusMessageLength = 256;

/** Results wait for the user. Other states are live: looking at a blocked program doesn't unblock
 * it, and OSC 9;4 errors expire by themselves. */
bool isProgramStatusResult( const PresentedProgramStatus& status ) {
	return status.source == ProgramStatusSource::Osc7501 &&
		   ( status.state == ProgramState::Done || status.state == ProgramState::Error );
}

ProgramStatusVisual programStatusVisual( const PresentedProgramStatus& status ) {
	if ( status.source == ProgramStatusSource::Osc9_4 ) {
		switch ( status.legacyState ) {
			case LegacyProgressState::Normal:
			case LegacyProgressState::Indeterminate:
				return ProgramStatusWorking;
			case LegacyProgressState::Error:
				return ProgramStatusError;
			case LegacyProgressState::PausedOrWarning:
				return ProgramStatusPaused;
			case LegacyProgressState::Inactive:
				break;
		}
		return {};
	}
	if ( status.source != ProgramStatusSource::Osc7501 )
		return {};
	switch ( status.state ) {
		case ProgramState::Working:
			return ProgramStatusWorking;
		case ProgramState::Blocked:
			return ProgramStatusBlocked;
		case ProgramState::Done:
			return ProgramStatusDone;
		case ProgramState::Error:
			return ProgramStatusError;
		case ProgramState::Idle:
			break;
	}
	return {};
}

} // namespace

ProgramStatusUI::ProgramStatusUI( const char* applicationName ) :
	mApplicationName( applicationName ) {}

void ProgramStatusUI::watch( UITerminal* terminal, const ProgramStatusPreferences& preferences,
							 const char* defaultIcon ) {
	if ( !terminal )
		return;

	mStates.insert_or_assign( terminal, State{ {}, {}, &preferences, defaultIcon } );
	const std::weak_ptr<const bool> lifetime = mLifetime;
	terminal->getTerm()->pushEventCallback(
		[this, lifetime, terminal]( const TerminalDisplay::Event& event ) {
			if ( event.type == TerminalDisplay::EventType::PROGRAM_STATUS_CHANGE &&
				 !lifetime.expired() )
				statusChanged( terminal );
		} );
	terminal->on( Event::OnFocusWithin, [this, lifetime, terminal]( const Event* ) {
		if ( !lifetime.expired() )
			refresh( terminal );
	} );
	terminal->on( Event::OnClose, [this, lifetime, terminal]( const Event* ) {
		if ( !lifetime.expired() )
			mStates.erase( terminal );
	} );
}

bool ProgramStatusUI::active( const UITerminal* terminal ) {
	if ( !terminal )
		return false;
	auto* scene = terminal->getUISceneNode();
	auto* window = scene ? scene->getWindow() : nullptr;
	return window && window->hasInputFocus() && terminal->hasFocusWithin();
}

void ProgramStatusUI::statusChanged( UITerminal* terminal ) {
	if ( !terminal )
		return;

	const auto found = mStates.find( terminal );
	if ( found == mStates.end() )
		return;
	auto& state = found->second;
	const auto& preferences = *state.preferences;
	const auto& status = terminal->getTerm()->getProgramStatus();
	if ( status ) {
		const bool transition = ProgramStatusAlerts::isTransition(
			state.previous ? &state.previous->presentation : nullptr, status->presentation,
			preferences.allStates );
		// Track suppressed changes too, so focus and settings never replay old alerts.
		state.previous = status;
		if ( transition && ( preferences.notifications || preferences.sounds ) &&
			 ( preferences.whenActive || !active( terminal ) ) ) {
			auto* scene = terminal->getUISceneNode();
			if ( !scene )
				return;
			const String title =
				String( mApplicationName ) + " · " +
				String::fromUtf8( ProgramStatusTracker::sanitizeForDisplay( terminal->getTitle() ) )
					.substr( 0, 80 );
			String message = describe( scene, status->presentation );
			if ( message.empty() )
				message = scene->i18n( "program_status_idle", "Idle" );
			mAlerts.deliver( title.toUtf8(), message.toUtf8(), preferences.notifications,
							 preferences.sounds, preferences.sound, preferences.volume );
		}
	}
	refresh( terminal );
}

void ProgramStatusUI::refresh( UITerminal* terminal ) {
	if ( !terminal )
		return;

	const auto found = mStates.find( terminal );
	if ( found == mStates.end() )
		return;
	auto* owner = UITabWidget::fromOwnedNode( terminal );
	auto* tab = owner ? owner->getTabFromOwnedWidget( terminal ) : nullptr;
	auto* scene = tab ? tab->getUISceneNode() : nullptr;
	if ( !scene )
		return;
	auto& state = found->second;
	static const PresentedProgramStatus noStatus;
	const auto& status = terminal->getTerm()->getProgramStatus();
	const auto& presented = state.preferences->show && status ? status->presentation : noStatus;

	const bool isResult = isProgramStatusResult( presented );
	bool seen = false;
	if ( isResult ) {
		seen = state.seen && state.seen->presentation == presented;
		if ( !seen && active( terminal ) ) {
			state.seen = status;
			seen = true;
		}
	} else if ( state.preferences->show ) {
		// Anything else in between makes the next result a new one, even with the same text.
		state.seen.reset();
	}
	const bool unread = isResult && !seen;
	// A seen error keeps its icon until the program clears it: only done means "not seen yet".
	const auto visual = seen && presented.state == ProgramState::Done
							? ProgramStatusVisual{}
							: programStatusVisual( presented );
	if ( unread != tab->hasClass( ProgramStatusUnreadClass ) ) {
		if ( unread )
			tab->addClass( ProgramStatusUnreadClass );
		else
			tab->removeClass( ProgramStatusUnreadClass );
	}

	// Progress updates keep the class, so they only reach the tooltip.
	bool changed = false;
	for ( const auto& other : { ProgramStatusWorking, ProgramStatusBlocked, ProgramStatusDone,
								ProgramStatusError, ProgramStatusPaused } ) {
		if ( other.styleClass != visual.styleClass && tab->hasClass( other.styleClass ) ) {
			tab->removeClass( other.styleClass );
			changed = true;
		}
	}
	if ( visual.styleClass && !tab->hasClass( visual.styleClass ) ) {
		tab->addClass( visual.styleClass );
		changed = true;
	}
	if ( changed ) {
		auto* icon = scene->findIcon( visual.icon ? visual.icon : state.defaultIcon );
		if ( !icon )
			icon = scene->findIcon( state.defaultIcon );
		tab->setIcon( icon ? icon->createDrawable( PixelDensity::dpToPxI( 12 ) ) : nullptr );
	}

	// The bar's width is the only thing CSS can't know. Without the class, the theme's tab remains.
	if ( visual.styleClass && presented.percentage ) {
		tab->getUIStyle()->setStyleSheetVariable( StyleSheetVariable(
			"--program-status-progress",
			String::format( "%d%%", static_cast<int>( *presented.percentage ) ) ) );
		if ( !tab->hasClass( ProgramStatusProgressClass ) )
			tab->addClass( ProgramStatusProgressClass );
		tab->reloadStyle( false, false, true, true, true );
	} else if ( tab->hasClass( ProgramStatusProgressClass ) ) {
		tab->removeClass( ProgramStatusProgressClass );
	}
	String description = describe( scene, presented );
	if ( unread )
		description += " · " + scene->i18n( "program_status_unread", "Not seen yet" );
	tab->setTooltipText( description );
}

String ProgramStatusUI::describe( UISceneNode* scene, const PresentedProgramStatus& status ) {
	if ( !scene )
		return {};

	String description;
	if ( status.source == ProgramStatusSource::Osc9_4 ) {
		switch ( status.legacyState ) {
			case LegacyProgressState::Normal:
				description = scene->i18n( "program_status_progress", "Progress" );
				break;
			case LegacyProgressState::Indeterminate:
				description = scene->i18n( "program_status_running", "Running" );
				break;
			case LegacyProgressState::Error:
				description = scene->i18n( "program_status_progress_error", "Progress error" );
				break;
			case LegacyProgressState::PausedOrWarning:
				// The protocol doesn't tell them apart: terminals disagree on this state's meaning.
				description = scene->i18n( "program_status_paused_or_warning", "Paused / warning" );
				break;
			case LegacyProgressState::Inactive:
				break;
		}
	} else if ( status.source == ProgramStatusSource::Osc7501 ) {
		switch ( status.state ) {
			case ProgramState::Working:
				description = scene->i18n( "program_status_working", "Working" );
				break;
			case ProgramState::Blocked:
				switch ( status.kind ) {
					case ProgramBlockKind::Permission:
						description = scene->i18n( "program_status_blocked_permission",
												   "Needs your permission" );
						break;
					case ProgramBlockKind::Question:
						description = scene->i18n( "program_status_blocked_question",
												   "Has a question for you" );
						break;
					case ProgramBlockKind::Auth:
						description =
							scene->i18n( "program_status_blocked_auth", "Needs authentication" );
						break;
					case ProgramBlockKind::None:
						description = scene->i18n( "program_status_blocked", "Waiting for you" );
						break;
				}
				break;
			case ProgramState::Done:
				description = scene->i18n( "program_status_done", "Done" );
				break;
			case ProgramState::Error:
				description = scene->i18n( "program_status_error", "Failed" );
				break;
			case ProgramState::Idle:
				break;
		}
	}
	if ( description.empty() )
		return description;

	// The fixed label leads, so a program's own text can't pose as the terminal's report.
	const auto addPart = [&description]( const String& part ) {
		if ( !part.empty() )
			description += " · " + part;
	};
	addPart( String::fromUtf8( status.app ) );
	addPart( String::fromUtf8( status.title ) );
	addPart( String::fromUtf8( status.message ).substr( 0, MaxProgramStatusMessageLength ) );
	if ( status.percentage )
		addPart( String::format( "%d%%", static_cast<int>( *status.percentage ) ) );
	return description;
}

void ProgramStatusUI::installStyles( UISceneNode* scene ) {
	if ( !scene )
		return;

	const auto marker = String::hash( "eterm-program-status" );
	scene->getStyleSheet().removeAllWithMarker( marker );
	scene->combineStyleSheet( R"css(
		Tab.program_status_working {
			--program-status-color: var(--primary);
		}
		Tab.program_status_blocked,
		Tab.program_status_paused {
			--program-status-color: var(--theme-warning);
		}
		Tab.program_status_done {
			--program-status-color: var(--theme-success);
		}
		Tab.program_status_error {
			--program-status-color: var(--theme-error);
		}
		/* A variable of the tab doesn't reach its icon, and the theme's hover rule is specific. */
		Tab.program_status_working > Tab::icon {
			tint: var(--primary) !important;
		}
		Tab.program_status_blocked > Tab::icon,
		Tab.program_status_paused > Tab::icon {
			tint: var(--theme-warning) !important;
		}
		Tab.program_status_done > Tab::icon {
			tint: var(--theme-success) !important;
		}
		Tab.program_status_error > Tab::icon {
			tint: var(--theme-error) !important;
		}
		Tab.program_status_unread.program_status_done > Tab::text {
			color: var(--theme-success) !important;
		}
		Tab.program_status_unread.program_status_error > Tab::text {
			color: var(--theme-error) !important;
		}
		/* The icon has eight equal spokes: a turn would look still, and 45 degrees wraps unnoticed. */
		@keyframes program-status-spin {
			from { rotation: 0; }
			to { rotation: 45; }
		}
		Tab.program_status_working > Tab::icon {
			animation: program-status-spin 1s steps(6) infinite;
		}
		Tab.program_status_progress,
		Tab.program_status_progress:selected {
			background-image: rectangle(solid, var(--program-status-color));
			background-size: var(--program-status-progress) 2dp;
			background-position: left bottom;
		}
	)css",
							  false, marker );
}

} // namespace eterm::UI
