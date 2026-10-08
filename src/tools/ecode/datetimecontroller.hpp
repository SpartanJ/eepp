#ifndef ECODE_DATETIMECONTROLLER_HPP
#define ECODE_DATETIMECONTROLLER_HPP

#include <eepp/ui/doc/textdocument.hpp>
#include <string>

namespace EE { namespace UI {
class UIMessageBox;
}} // namespace EE::UI

using namespace EE::UI::Doc;

namespace ecode {

class PluginContextProvider;

class DateTimeController {
  public:
	explicit DateTimeController( PluginContextProvider* context );

	void registerCommands( TextDocument& doc );

	static bool isValidDateFormat( const std::string& format );

	static std::string formatCurrentDate( const std::string& format );

  private:
	PluginContextProvider* mContext{ nullptr };

	void insertDate( const std::string& format );

	void setCustomDateFormat();

	void setFocusEditorOnClose( EE::UI::UIMessageBox* msgBox );
};

} // namespace ecode

#endif
