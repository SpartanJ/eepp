#include "quakewindow.hpp"
#include <algorithm>
#include <cmath>
#include <eepp/math/easing.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/displaymanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <iostream>
#include <iterator>

namespace eterm {

namespace {

constexpr auto OpeningFrameTimeoutMs = 250;

} // namespace

namespace {

using namespace EE::System::GlobalShortcuts;
using namespace EE::Window;
using ShortcutBackend = EE::System::GlobalShortcuts::Backend;

constexpr const char* ToggleIds[] = { "toggle-quake-window", "toggle-quake-window-secondary" };

size_t slotIndex( QuakeWindow::ShortcutSlot slot ) {
	return static_cast<size_t>( slot );
}

void showShortcutMessage( const String& message ) {
	auto* dialog = UIMessageBox::New( UIMessageBox::OK, message );
	dialog->setTitle( "eterm Quake mode" );
	dialog->center();
	dialog->showWhenReady();
}

const BoundShortcut* findBinding( const ServiceSnapshot& state, const char* id ) {
	const auto binding = std::find_if( state.effective.begin(), state.effective.end(),
									   [id]( const auto& shortcut ) { return shortcut.id == id; } );
	return binding == state.effective.end() ? nullptr : &*binding;
}

// The portal can report a declared shortcut that the user left without a trigger.
bool isUsable( const BoundShortcut& binding ) {
	return binding.status == BindingStatus::Bound && !binding.error &&
		   ( binding.effectiveTrigger || !binding.triggerDescription.empty() );
}

bool anyUsable( const ServiceSnapshot& state ) {
	return std::any_of( state.effective.begin(), state.effective.end(), isUsable );
}

std::string bindingIssue( const ServiceSnapshot& state ) {
	for ( const auto& binding : state.effective ) {
		if ( binding.error )
			return binding.error->message;
		if ( !isUsable( binding ) )
			return "The desktop did not grant the Quake shortcut";
	}
	return {};
}

Options shortcutOptions() {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	if ( Sys::getEnv( "FLATPAK_ID" ).empty() && Sys::getEnv( "SNAP" ).empty() ) {
		for ( const auto& base : Sys::getDataSearchPaths() ) {
			if ( FileSystem::fileExists( base + "/applications/eterm.desktop" ) )
				return { "eterm" };
		}
	}
#endif
	return {};
}

Result<std::optional<Trigger>, Error> parseTrigger( const Input& input, const std::string& text ) {
	if ( text.empty() )
		return Ok( std::optional<Trigger>{} );
	const auto parts = String::split( text, '+', true );
	const auto invalid = [] {
		return Err(
			Error{ ErrorCode::InvalidShortcut, "Expected a shortcut such as Mod+Shift+F11" } );
	};
	if ( parts.empty() )
		return invalid();

	Uint32 modifiers = KEYMOD_NONE;
	for ( auto it = parts.begin(); it != std::prev( parts.end() ); ++it ) {
		const auto mod = KeyMod::getKeyMod( String::trim( *it, " \t" ) );
		if ( !mod )
			return invalid();
		modifiers |= mod;
	}
	for ( const Uint32 group : { KEYMOD_SHIFT, KEYMOD_CTRL, KEYMOD_ALT, KEYMOD_META } ) {
		if ( modifiers & group )
			modifiers |= group;
	}

	const auto key = input.getKeyFromName( String::trim( parts.back(), " \t" ) );
	if ( key == KEY_UNKNOWN )
		return invalid();
	return Ok( std::optional<Trigger>( Trigger{ key, modifiers } ) );
}

} // namespace

QuakeWindow::QuakeWindow( EE::Window::Window& window, UISceneNode& scene, AppConfig& config,
						  const Rect& geometry, const Sizei& explicitSize,
						  std::function<void()> onShortcutsChanged ) :
	mWindow( window ),
	mScene( scene ),
	mConfig( config ),
	mExecutor( &scene, [] { Input::tryWakeUpEventLoop(); } ),
	mShortcuts( mExecutor, shortcutOptions() ),
	mOnShortcutsChanged( std::move( onShortcutsChanged ) ),
	mGeometry( geometry ),
	mExplicitSize( explicitSize ),
	mDisplayIndex( config.windowState.displayIndex ) {}

Rect QuakeWindow::geometry( const QuakeConfig& config, const Sizei& explicitSize,
							int displayIndex ) {
	auto* manager = Engine::instance()->getDisplayManager();
	const int count = manager ? manager->getDisplayCount() : 0;
	auto* display =
		count > 0 ? manager->getDisplayIndex( std::clamp( displayIndex, 0, count - 1 ) ) : nullptr;
	Rect bounds;
	if ( display ) {
		bounds = display->getUsableBounds();
		if ( bounds.getWidth() <= 0 || bounds.getHeight() <= 0 )
			bounds = display->getBounds();
	}
	if ( bounds.getWidth() <= 0 || bounds.getHeight() <= 0 )
		bounds = Rect( Vector2i::Zero, { 1280, 720 } );

	const auto dimension = []( int available, int requested, int percent ) {
		return requested > 0 ? std::min( requested, available )
							 : std::max( 1, available * percent / 100 );
	};
	const Sizei size{
		dimension( bounds.getWidth(), explicitSize.getWidth(), config.widthPercent ),
		dimension( bounds.getHeight(), explicitSize.getHeight(), config.heightPercent ) };
	const int freeWidth = bounds.getWidth() - size.getWidth();
	const int left = bounds.Left + ( config.horizontalAlignment == UIHorizontalAlignment::Left ? 0
									 : config.horizontalAlignment == UIHorizontalAlignment::Right
										 ? freeWidth
										 : freeWidth / 2 );
	return Rect( { left, bounds.Top }, size );
}

void QuakeWindow::start( const std::string& shortcut ) {
	if ( !mConfig.quake.launchHidden )
		show();
	mShortcutText = { shortcut, mConfig.quake.secondaryShortcut };
	mShortcuts.setActivationCallback( [this]( const Activation& activation ) {
		if ( activation.id == ToggleIds[0] || activation.id == ToggleIds[1] )
			toggle();
	} );
	mShortcuts.setStateChangedCallback(
		[this]( const ServiceSnapshot& state ) { onStateChanged( state ); } );
	submitShortcuts( ShortcutRequest::Startup );
}

bool QuakeWindow::isShortcutSyntaxValid( const Input& input, const std::string& shortcut,
										 ShortcutSlot slot ) {
	return shortcut.empty() ? slot == ShortcutSlot::Secondary
							: static_cast<bool>( parseTrigger( input, shortcut ) );
}

const std::string& QuakeWindow::shortcut( ShortcutSlot slot ) const {
	return mShortcutText[slotIndex( slot )];
}

std::array<String, 2> QuakeWindow::shortcutLabels() const {
	const auto state = mShortcuts.snapshot();
	const auto* input = mWindow.getInput();
	auto format = [input]( Trigger trigger ) {
		// Global Alt is one group. The KeyBindings formatter displays right Alt separately.
		if ( trigger.modifiers & KEYMOD_ALT )
			trigger.modifiers = ( trigger.modifiers & ~KEYMOD_ALT ) | KEYMOD_LALT;
		return String::fromUtf8(
			KeyBindings::fromShortcut( input, { trigger.key, trigger.modifiers }, true ) );
	};

	std::array<String, 2> labels;
	for ( size_t i = 0; i < labels.size(); ++i ) {
		const auto* binding = findBinding( state, ToggleIds[i] );
		if ( binding && isUsable( *binding ) ) {
			if ( !binding->triggerDescription.empty() ) {
				labels[i] = String::fromUtf8( binding->triggerDescription );
				continue;
			}
			if ( binding->effectiveTrigger ) {
				labels[i] = format( *binding->effectiveTrigger );
				continue;
			}
		}
		const auto& configured = mShortcutText[i];
		if ( configured.empty() ) {
			labels[i] = i == 0 ? mScene.i18n( "quake_shortcut_unassigned", "Unassigned" )
							   : mScene.i18n( "quake_shortcut_disabled", "Disabled" );
			continue;
		}
		const auto trigger = parseTrigger( *input, configured );
		labels[i] = trigger && trigger.value() ? format( *trigger.value() )
											   : String::fromUtf8( configured );
		if ( mSlotProblems[i] == SlotProblem::Invalid ) {
			labels[i] += " " + mScene.i18n( "quake_shortcut_invalid", "(invalid)" );
		} else if ( mSlotProblems[i] == SlotProblem::Duplicate ) {
			labels[i] += " " + mScene.i18n( "quake_shortcut_duplicate", "(same as primary)" );
		} else if ( binding && binding->error ) {
			labels[i] +=
				" " + ( binding->error->code == ErrorCode::Conflict
							? mScene.i18n( "quake_shortcut_in_use", "(in use)" )
							: mScene.i18n( "quake_shortcut_unavailable", "(unavailable)" ) );
		} else if ( binding && binding->status == BindingStatus::Bound &&
					state.backend == ShortcutBackend::WaylandPortal ) {
			labels[i] =
				mScene.i18n( "quake_shortcut_not_assigned", "Not assigned in desktop settings" );
		} else if ( ( binding && binding->status == BindingStatus::Unknown ) ||
					( ( !binding || binding->status != BindingStatus::Bound ) &&
					  ( state.state == State::Unavailable || state.state == State::Degraded ) ) ) {
			labels[i] += " " + mScene.i18n( "quake_shortcut_unavailable", "(unavailable)" );
		}
	}
	return labels;
}

void QuakeWindow::changeShortcut( ShortcutSlot slot, std::string shortcut ) {
	if ( handleShortcutSettings( slot ) )
		return;
	// The portal owns assignments for registered IDs. Adding or removing an ID changes the
	// declaration, while editing an existing assignment goes through desktop settings.
	if ( slot == ShortcutSlot::Secondary &&
		 mShortcuts.snapshot().backend == ShortcutBackend::WaylandPortal && !shortcut.empty() &&
		 !mShortcutText[slotIndex( slot )].empty() ) {
		handleShortcutSettings( ShortcutSlot::Primary );
		return;
	}
	mReportedFailure = false;
	submitShortcuts( ShortcutRequest::Change, ShortcutEdit{ std::move( shortcut ), slot } );
}

bool QuakeWindow::handleShortcutSettings( ShortcutSlot slot ) {
	if ( mShortcutPending ) {
		showShortcutMessage( "Wait for the desktop to finish the current shortcut request." );
		return true;
	}
	const auto state = mShortcuts.snapshot();
	if ( slot != ShortcutSlot::Primary || state.backend != ShortcutBackend::WaylandPortal )
		return false;
	if ( !state.capabilities.canConfigureShortcuts ) {
		showShortcutMessage( "This desktop cannot open its shortcut settings from eterm. You can "
							 "assign a desktop shortcut to run eterm --quake." );
	} else {
		mShortcutPending = true;
		mShortcuts.configureShortcuts().thenDroppable(
			mExecutor, [this]( const Result<void, Error>& result ) {
				mShortcutPending = false;
				if ( !result && mWindow.isOpen() )
					showShortcutMessage( String::fromUtf8( result.error().message ) );
			} );
	}
	return true;
}

void QuakeWindow::submitShortcuts( ShortcutRequest request, std::optional<ShortcutEdit> edit ) {
	std::vector<Shortcut> shortcuts;
	shortcuts.reserve( mShortcutText.size() );
	std::array<SlotProblem, 2> problems{};
	std::string parseError;
	for ( size_t i = 0; i < mShortcutText.size(); ++i ) {
		const bool edited = edit && slotIndex( edit->slot ) == i;
		const auto& text = edited ? edit->text : mShortcutText[i];
		if ( i != 0 && text.empty() )
			continue;
		auto trigger = parseTrigger( *mWindow.getInput(), text );
		if ( !trigger ) {
			if ( edited ) {
				showEditFailure( trigger.error().message );
				return;
			}
			problems[i] = SlotProblem::Invalid;
			parseError = trigger.error().message;
			continue;
		}
		if ( trigger.value() && !shortcuts.empty() &&
			 shortcuts.front().preferred == trigger.value() ) {
			if ( edit ) {
				showEditFailure( "Choose two different Quake shortcuts" );
				return;
			}
			problems[i] = SlotProblem::Duplicate;
			parseError = "Choose two different Quake shortcuts";
			continue;
		}
		shortcuts.push_back( { ToggleIds[i],
							   i == 0 ? "Show or hide the eterm Quake window"
									  : "Show or hide the eterm Quake window (secondary)",
							   std::move( trigger ).value() } );
	}

	mSlotProblems = problems;
	mShortcutPending = true;
	mShortcuts.setShortcuts( std::move( shortcuts ) )
		.thenDroppable( mExecutor, [this, request, edit = std::move( edit ),
									parseError = std::move( parseError )](
									   const Result<ServiceSnapshot, Error>& result ) mutable {
			finishShortcuts( request, std::move( edit ), std::move( parseError ), result );
		} );
}

void QuakeWindow::finishShortcuts( ShortcutRequest request, std::optional<ShortcutEdit> edit,
								   std::string parseError,
								   const Result<ServiceSnapshot, Error>& result ) {
	mShortcutPending = false;
	if ( edit ) {
		// Report only the slot the user edited. Other slots show their state in Settings.
		std::string issue;
		const auto index = slotIndex( edit->slot );
		const auto* binding = result ? findBinding( result.value(), ToggleIds[index] ) : nullptr;
		if ( !result ) {
			issue = result.error().message;
		} else if ( edit->text.empty()
						? !binding
						: binding && binding->status == BindingStatus::Bound && !binding->error ) {
			// A portal shortcut left without a trigger is a desktop choice. Its label says so.
			mShortcutText[index] = std::move( edit->text );
			auto& saved = edit->slot == ShortcutSlot::Primary ? mConfig.quake.shortcut
															  : mConfig.quake.secondaryShortcut;
			if ( saved != mShortcutText[index] ) {
				saved = mShortcutText[index];
				if ( !mConfig.savePreferences() )
					Log::error( "Could not save eterm configuration to %s",
								mConfig.getConfigPath() );
			}
		} else {
			issue = binding && binding->error ? binding->error->message
											  : "The desktop did not grant the Quake shortcut";
		}
		mOnShortcutsChanged();
		if ( issue.empty() ) {
			mLoggedIssue.clear();
			if ( anyUsable( result.value() ) )
				mReportedFailure = false;
			return;
		}
		showEditFailure( issue );
		if ( edit->text != mShortcutText[index] && ( mShortcutBackend == ShortcutBackend::X11 ||
													 mShortcutBackend == ShortcutBackend::Windows ||
													 mShortcutBackend == ShortcutBackend::MacOS ) )
			submitShortcuts( ShortcutRequest::Restore );
		return;
	}

	mOnShortcutsChanged();
	if ( !result ) {
		const bool startupBackendUnavailable =
			request == ShortcutRequest::Startup &&
			( result.error().code == ErrorCode::Unsupported ||
			  result.error().code == ErrorCode::BackendUnavailable );
		showUnavailable( result.error().message, startupBackendUnavailable );
		return;
	}
	auto issue = bindingIssue( result.value() );
	if ( issue.empty() )
		issue = std::move( parseError );
	if ( !issue.empty() ) {
		showUnavailable( issue );
	} else {
		mLoggedIssue.clear();
		mReportedFailure = false;
	}
}

void QuakeWindow::geometryChanged( GeometrySetting setting ) {
	if ( setting == GeometrySetting::Width )
		mExplicitSize.setWidth( 0 );
	else if ( setting == GeometrySetting::Height )
		mExplicitSize.setHeight( 0 );
	mGeometryDirty = true;
}

void QuakeWindow::applyGeometry() {
	mGeometryDirty = false;
	const auto next = geometry( mConfig.quake, mExplicitSize, mDisplayIndex );
	if ( next == mGeometry )
		return;
	if ( mSlide && !mSlide->showing )
		mWindow.hideForReopen();
	mSlide.reset();
	const auto size = next.getSize();
	if ( size != mGeometry.getSize() )
		mWindow.setSize( size.getWidth(), size.getHeight() );
	mGeometry = next;
	if ( mWindow.isVisible() )
		move( mGeometry.Top );
	mScene.invalidate( nullptr );
}

bool QuakeWindow::selectPointerDisplay() {
	if ( mShortcutBackend == ShortcutBackend::WaylandPortal )
		return false;
	auto* manager = Engine::instance()->getDisplayManager();
	const int count = manager ? manager->getDisplayCount() : 0;
	if ( count < 2 )
		return false;
	const auto pointer = mWindow.getInput()->queryMousePos() + mWindow.getPosition();
	for ( int i = 0; i < count; ++i ) {
		auto* display = manager->getDisplayIndex( i );
		if ( display && display->getBounds().contains( pointer ) ) {
			if ( i != mDisplayIndex ) {
				mDisplayIndex = i;
				applyGeometry();
				return true;
			}
			break;
		}
	}
	return false;
}

void QuakeWindow::onFocusLost() {
	if ( mShortcutPending )
		return;
	if ( mConfig.quake.hideOnFocusLoss && mWindow.isOpen() && mWindow.isVisible() &&
		 ( !mSlide || mSlide->showing ) )
		hide();
}

void QuakeWindow::onStateChanged( const GlobalShortcuts::ServiceSnapshot& state ) {
	mShortcutBackend = state.backend;
	mOnShortcutsChanged();
	mCanPosition = state.backend == ShortcutBackend::X11 ||
				   state.backend == ShortcutBackend::Windows ||
				   state.backend == ShortcutBackend::MacOS;
	if ( anyUsable( state ) )
		mReportedFailure = false;
	if ( mShortcutPending ||
		 ( state.state != State::Unavailable && state.state != State::Degraded ) )
		return;
	if ( state.error )
		showUnavailable( state.error->message );
	else if ( auto issue = bindingIssue( state ); !issue.empty() )
		showUnavailable( issue );
	else if ( state.lastBackendIssue )
		showUnavailable( state.lastBackendIssue->message );
}

void QuakeWindow::showUnavailable( const std::string& message, bool startupBackendUnavailable ) {
	if ( message != mLoggedIssue ) {
		mLoggedIssue = message;
		std::cerr << "eterm Quake mode: " << message << '\n';
	}
	// While another shortcut toggles the console, Settings shows the problem in its label.
	if ( mReportedFailure || !mWindow.isOpen() || anyUsable( mShortcuts.snapshot() ) )
		return;
	mReportedFailure = true;
	showImmediately();
	if ( startupBackendUnavailable && !mConfig.takeQuakeShortcutUnavailableNotice() )
		return;
	showShortcutMessage( String::fromUtf8(
		"Could not register a Quake mode shortcut.\n\n" + message +
		"\n\nRun eterm --quake to toggle this console. You can assign that command to a "
		"desktop shortcut." ) );
	if ( startupBackendUnavailable && !mConfig.saveWindowState() )
		Log::error( "Could not save eterm window state to %s", mConfig.getStateDirectory() );
}

void QuakeWindow::showEditFailure( const std::string& message ) {
	std::cerr << "eterm Quake mode: " << message << '\n';
	if ( mWindow.isOpen() )
		showShortcutMessage(
			String::fromUtf8( "Could not change the Quake mode shortcut.\n\n" + message ) );
}

void QuakeWindow::toggle() {
	if ( !mWindow.isOpen() )
		return;
	if ( mWindow.hasInputFocus() &&
		 ( mSlide ? mSlide->showing : mWindow.isVisible() && !mWindow.isMinimized() ) )
		hide();
	else
		show();
}

void QuakeWindow::show() {
	if ( !mWindow.isOpen() )
		return;
	if ( mSlide ? mSlide->showing : mWindow.isVisible() && !mWindow.isMinimized() ) {
		if ( !needsOpeningFrame() )
			mWindow.activate();
		return;
	}
	const bool changedDisplay = selectPointerDisplay();
	const int duration = mConfig.quake.animationMs;
	if ( !mCanPosition || duration == 0 ) {
		showImmediately();
		return;
	}
	const int hiddenTop = mGeometry.Top - mGeometry.getHeight();
	const int from = mSlide ? mSlide->last : hiddenTop;
	const bool awaitingFrame = changedDisplay || !mWindow.isVisible() || mWindow.isMinimized();
	move( from );
	mSlide = Slide{ from, mGeometry.Top,
					from, duration,
					true, awaitingFrame ? OpeningFrame::PendingMap : OpeningFrame::None };
	if ( !awaitingFrame ) {
		mWindow.activate();
		mScene.invalidate( nullptr );
		mClock.restart();
	}
}

void QuakeWindow::hide() {
	const int duration = mConfig.quake.animationMs;
	if ( !mCanPosition || duration == 0 ) {
		mSlide.reset();
		mWindow.hideForReopen();
		return;
	}
	const int from = mSlide ? mSlide->last : mGeometry.Top;
	mSlide = Slide{ from, mGeometry.Top - mGeometry.getHeight(), from, duration, false };
	mClock.restart();
}

void QuakeWindow::showImmediately() {
	mSlide.reset();
	move( mGeometry.Top );
	mWindow.activate();
	mScene.invalidate( nullptr );
}

void QuakeWindow::move( int top ) {
	mWindow.setPosition( mGeometry.Left, top );
}

bool QuakeWindow::needsOpeningFrame() const {
	return mSlide && mSlide->openingFrame != OpeningFrame::None;
}

void QuakeWindow::mapForOpeningFrame() {
	if ( !mSlide || mSlide->openingFrame != OpeningFrame::PendingMap )
		return;
	mSlide->openingFrame = OpeningFrame::AwaitingSwap;
	mClock.restart();
	mWindow.activate();
	mScene.invalidate( nullptr );
}

void QuakeWindow::openingFramePresented() {
	if ( needsOpeningFrame() ) {
		mSlide->openingFrame = OpeningFrame::None;
		mClock.restart();
	}
}

bool QuakeWindow::update() {
	// Resize after input and scene dispatch have finished, since setSize runs resize callbacks.
	if ( mGeometryDirty )
		applyGeometry();
	if ( !mSlide )
		return false;
	auto& slide = *mSlide;
	if ( slide.openingFrame == OpeningFrame::PendingMap )
		return true;
	const auto elapsed = mClock.getElapsedTime().asMilliseconds();
	if ( slide.openingFrame == OpeningFrame::AwaitingSwap ) {
		if ( elapsed < OpeningFrameTimeoutMs )
			return true;
		// If the window manager delays the map, leave the window at its resting position.
		move( mGeometry.Top );
		mScene.invalidate( nullptr );
		mSlide.reset();
		return false;
	}
	if ( elapsed >= slide.durationMs || mConfig.quake.animationMs == 0 ) {
		if ( !slide.showing )
			mWindow.hideForReopen();
		else
			mScene.invalidate( nullptr );
		move( mGeometry.Top );
		mSlide.reset();
		return false;
	}
	const auto ease = slide.showing ? easing::cubicOut : easing::cubicIn;
	const auto top = static_cast<int>(
		std::lround( ease( elapsed, slide.from, slide.to - slide.from, slide.durationMs ) ) );
	if ( top != slide.last ) {
		move( top );
		slide.last = top;
		// An uncomposited window can expose new pixels even when the terminal is idle.
		mScene.invalidate( nullptr );
	}
	return true;
}

} // namespace eterm
