#ifndef ECODE_FORMATTERPLUGIN_HPP
#define ECODE_FORMATTERPLUGIN_HPP

#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class FormatterPlugin : public UICodeEditorPlugin {
  public:
	struct NativeFormatterResult {
		bool success;
		std::string result;
	};

	FormatterPlugin( const std::string& formatterPath, std::shared_ptr<ThreadPool> pool );

	virtual ~FormatterPlugin();

	std::string getTitle() { return "Auto Formatter"; }

	std::string getDescription() { return "Enables the code formatter/prettifier plugin."; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	bool getAutoFormatOnSave() const;

	void setAutoFormatOnSave( bool autoFormatOnSave );

	void registerNativeFormatter(
		const std::string& cmd,
		const std::function<NativeFormatterResult( const std::string& file )>& nativeFormatter );

	void unregisterNativeFormatter( const std::string& cmd );
  protected:
	enum class FormatterType { Inplace, Output, Native };

	struct Formatter {
		std::vector<std::string> files;
		std::string command;
		FormatterType type{ FormatterType::Output };
	};

	std::shared_ptr<ThreadPool> mPool;
	std::vector<Formatter> mFormatters;
	std::set<UICodeEditor*> mEditors;
	std::mutex mWorkMutex;
	std::condition_variable mWorkerCondition;
	std::map<std::string, std::function<NativeFormatterResult( const std::string& file )>>
		mNativeFormatters;
	Int32 mWorkersCount{ 0 };

	bool mAutoFormatOnSave{ false };
	bool mShuttingDown{ false };
	bool mReady{ false };

	void load( const std::string& formatterPath );

	void formatDoc( UICodeEditor* editor );

	void runFormatter( UICodeEditor* editor, const Formatter& formatter, const std::string& path );

	FormatterPlugin::Formatter supportsFormatter( std::shared_ptr<TextDocument> doc );

	void registerNativeFormatters();
};

} // namespace ecode

#endif // ECODE_FORMATTERPLUGIN_HPP
