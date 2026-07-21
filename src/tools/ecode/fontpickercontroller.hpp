#ifndef ECODE_FONTPICKERCONTROLLER_HPP
#define ECODE_FONTPICKERCONTROLLER_HPP

#include <functional>
#include <string>

namespace ecode {

class App;

class FontPickerController {
  public:
	explicit FontPickerController( App* app ) : mApp( app ) {}

	void openFontDialog( std::string& fontPath, bool loadingMonoFont, bool terminalFont = false,
						 std::function<void()> onFinish = {} );

  private:
	App* mApp{ nullptr };
};

} // namespace ecode

#endif
