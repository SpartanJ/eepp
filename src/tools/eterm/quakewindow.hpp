#ifndef ETERM_QUAKEWINDOW_HPP
#define ETERM_QUAKEWINDOW_HPP

#include "appconfig.hpp"
#include <array>
#include <eepp/globalshortcuts.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/system/clock.hpp>
#include <functional>
#include <optional>

namespace eterm {

class QuakeWindow {
  public:
	enum class GeometrySetting { Width, Height, Alignment };
	enum class ShortcutSlot : Uint8 { Primary, Secondary };

	QuakeWindow( EE::Window::Window& window, UISceneNode& scene, AppConfig& config,
				 const Rect& geometry, const Sizei& explicitSize,
				 std::function<void()> onShortcutsChanged );

	static Rect geometry( const QuakeConfig& config, const Sizei& explicitSize, int displayIndex );

	void start( const std::string& shortcut );

	void toggle();

	static bool isShortcutSyntaxValid( const EE::Window::Input& input, const std::string& shortcut,
									   ShortcutSlot slot );

	const std::string& shortcut( ShortcutSlot slot ) const;

	std::array<String, 2> shortcutLabels() const;

	void changeShortcut( ShortcutSlot slot, std::string shortcut );

	bool handleShortcutSettings( ShortcutSlot slot );

	void geometryChanged( GeometrySetting setting );

	void onFocusLost();

	/** The opening slide is waiting for its first frame. */
	bool needsOpeningFrame() const;

	void mapForOpeningFrame();

	void openingFramePresented();

	/** Advance the native window slide. True while the loop needs animation ticks. */
	bool update();

  private:
	enum class OpeningFrame : Uint8 { None, PendingMap, AwaitingSwap };

	struct Slide {
		int from;
		int to;
		int last;
		int durationMs;
		bool showing;
		OpeningFrame openingFrame{ OpeningFrame::None };
	};
	void applyGeometry();

	bool selectPointerDisplay();

	enum class ShortcutRequest : Uint8 { Startup, Change, Restore };

	struct ShortcutEdit {
		std::string text;
		ShortcutSlot slot;
	};

	/** A configured shortcut that eterm did not submit. */
	enum class SlotProblem : Uint8 { None, Invalid, Duplicate };

	void submitShortcuts( ShortcutRequest request, std::optional<ShortcutEdit> edit = {} );

	void finishShortcuts(
		ShortcutRequest request, std::optional<ShortcutEdit> edit, std::string parseError,
		const Result<GlobalShortcuts::ServiceSnapshot, GlobalShortcuts::Error>& result );

	void onStateChanged( const GlobalShortcuts::ServiceSnapshot& state );

	/** Logs a problem and shows a dialog only when no shortcut can toggle the console. */
	void showUnavailable( const std::string& message, bool startupBackendUnavailable = false );

	/** Reports a failed edit that the user just requested. */
	void showEditFailure( const std::string& message );

	void show();

	void hide();

	void showImmediately();

	void move( int top );

	EE::Window::Window& mWindow;
	UISceneNode& mScene;
	AppConfig& mConfig;
	Scene::MainThreadExecutor mExecutor;
	GlobalShortcuts::Service mShortcuts;
	std::array<std::string, 2> mShortcutText;
	std::array<SlotProblem, 2> mSlotProblems{};
	std::string mLoggedIssue;
	std::function<void()> mOnShortcutsChanged;
	Rect mGeometry;
	Sizei mExplicitSize;
	Clock mClock;
	std::optional<Slide> mSlide;
	int mDisplayIndex;
	bool mCanPosition{ false };
	bool mGeometryDirty{ false };
	GlobalShortcuts::Backend mShortcutBackend{ GlobalShortcuts::Backend::None };
	bool mShortcutPending{ false };
	bool mReportedFailure{ false };
};

} // namespace eterm

#endif
