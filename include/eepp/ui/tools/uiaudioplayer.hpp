#ifndef EE_UI_TOOLS_UIAUDIOPLAYER_HPP
#define EE_UI_TOOLS_UIAUDIOPLAYER_HPP

#include <eepp/core/task.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <memory>
#include <optional>

namespace EE::Audio {
class Music;
}

namespace EE::UI {
class UIProgressBar;
class UITextView;
} // namespace EE::UI

using namespace EE::UI;
using namespace EE::Audio;

namespace EE::UI::Tools {

class EE_API UIAudioPlayer : public UIRelativeLayout {
  public:
	static UIAudioPlayer* New();

	virtual ~UIAudioPlayer();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	Music* getAudio() const;

	virtual void scheduledUpdate( const Time& time ) override;

	void loadFromPath( const std::string& path, bool autoPlay = true );

	/** Opens an owned stream synchronously. For slow streams, prepare a Music on a worker and
	 * adopt it with setAudio() on the UI thread instead. */
	bool loadFromStream( std::unique_ptr<IOStream> stream, bool autoPlay = true );

	/** Adopts an already opened decoder without performing file I/O on the UI thread. */
	void setAudio( std::unique_ptr<Music> audio, bool autoPlay = true );

	/** Stops playback and releases the current file or memory buffer. */
	void reset();

	const std::string& getFilePath() const;

	void play();

	void pause();

	void stop();

	UIProgressBar* getProgressBar() const { return mProgressBar; }

	UIWidget* getStatusBtn() const { return mStatusBtn; }

	UIWidget* getVolumeBtn() const { return mVolumeBtn; }

	UITextView* getTimeView() const { return mTimeView; }

  protected:
	UIAudioPlayer();

	std::shared_ptr<Music> mMusic;
	std::optional<Task<void>> mControls;
	UIProgressBar* mProgressBar{ nullptr };
	UIWidget* mStatusBtn{ nullptr };
	UIWidget* mVolumeBtn{ nullptr };
	UITextView* mTimeView{ nullptr };
	std::string mFilePath;

  private:
	enum class Command : Uint8 { Toggle, Play, Pause, Stop, Seek };

	/** Serializes potentially blocking decoder controls on the scene's worker pool. The jobs
	 * own their decoder, so replacing or closing the player does not wait on remote I/O. */
	void control( Command command, Time offset = Time::Zero );

	template <typename Action> void enqueue( Action&& action );
};

} // namespace EE::UI::Tools

#endif
