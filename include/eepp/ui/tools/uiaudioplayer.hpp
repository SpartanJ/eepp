#ifndef EE_UI_TOOLS_UIAUDIOPLAYER_HPP
#define EE_UI_TOOLS_UIAUDIOPLAYER_HPP

#include <eepp/ui/uirelativelayout.hpp>

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

	Music* mMusic{ nullptr };
	UIProgressBar* mProgressBar{ nullptr };
	UIWidget* mStatusBtn{ nullptr };
	UIWidget* mVolumeBtn{ nullptr };
	UITextView* mTimeView{ nullptr };
	std::string mFilePath;
};

} // namespace EE::UI::Tools

#endif
