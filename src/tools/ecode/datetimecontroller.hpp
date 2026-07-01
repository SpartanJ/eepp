#ifndef ECODE_DATETIMECONTROLLER_HPP
#define ECODE_DATETIMECONTROLLER_HPP

#include <eepp/ui/doc/textdocument.hpp>
#include <string>

namespace EE { namespace UI {
class UIMessageBox;
}} // namespace EE::UI

namespace ecode {

class PluginContextProvider;

class DateTimeController {
  public:
	explicit DateTimeController( PluginContextProvider* context );

	void registerCommands( EE::UI::Doc::TextDocument& doc );

  private:
	PluginContextProvider* mContext{ nullptr };

	static bool isValidDateFormat( const std::string& format );

	static std::string formatCurrentDate( const std::string& format );

	void insertDate( const std::string& format );

	void setCustomDateFormat();

	void setFocusEditorOnClose( EE::UI::UIMessageBox* msgBox );
};

} // namespace ecode

#endif
