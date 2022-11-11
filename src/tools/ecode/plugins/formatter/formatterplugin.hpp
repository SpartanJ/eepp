#ifndef ECODE_FORMATTERPLUGIN_HPP
#define ECODE_FORMATTERPLUGIN_HPP

#include "../pluginmanager.hpp"
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
		std::string err;
	};

	static PluginDefinition Definition() {
		return { "autoformatter",
				 "Auto Formatter",
				 "Enables the code formatter/prettifier plugin.",
				 FormatterPlugin::New,
				 { 0, 1, 0 } };
	}

	static UICodeEditorPlugin* New( PluginManager* pluginManager );

	virtual ~FormatterPlugin();

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	bool isReady() const { return mReady; }

	bool getAutoFormatOnSave() const;

	void setAutoFormatOnSave( bool autoFormatOnSave );

	void registerNativeFormatter(
		const std::string& cmd,
		const std::function<NativeFormatterResult( const std::string& file )>& nativeFormatter );

	void unregisterNativeFormatter( const std::string& cmd );

	bool hasFileConfig();

	std::string getFileConfigPath();

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
	std::string mConfigPath;
	std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */

	bool mAutoFormatOnSave{ false };
	bool mShuttingDown{ false };
	bool mReady{ false };
	Uint32 mOnDocumentSaveCb{ 0 };

	FormatterPlugin( PluginManager* pluginManager );

	void load( PluginManager* pluginManager );

	void loadFormatterConfig( const std::string& path );

	void formatDoc( UICodeEditor* editor );

	void runFormatter( UICodeEditor* editor, const Formatter& formatter, const std::string& path );

	size_t formatterFilePatternPosition( const std::vector<std::string>& patterns );

	FormatterPlugin::Formatter supportsFormatter( std::shared_ptr<TextDocument> doc );

	void registerNativeFormatters();
};

} // namespace ecode

#endif // ECODE_FORMATTERPLUGIN_HPP
