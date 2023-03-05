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
	enum class FormatterType { Inplace, Output, Native };

	struct NativeFormatterResult {
		bool success;
		std::string result;
		std::string err;
	};

	struct Formatter {
		std::vector<std::string> files;
		std::string command;
		FormatterType type{ FormatterType::Output };
		std::vector<std::string> languages{};
	};

	static PluginDefinition Definition() {
		return {
			"autoformatter",	  "Auto Formatter", "Enables the code formatter/prettifier plugin.",
			FormatterPlugin::New, { 0, 2, 0 },		FormatterPlugin::NewSync };
	}

	static UICodeEditorPlugin* New( PluginManager* pluginManager );

	static UICodeEditorPlugin* NewSync( PluginManager* pluginManager );

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

	virtual bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
									  const Vector2i& position, const Uint32& flags );

	const std::vector<Formatter>& getFormatters() const;

	Formatter getFormatterForLang( const std::string& lang, const std::vector<std::string>& ext );

  protected:
	PluginManager* mManager{ nullptr };
	std::shared_ptr<ThreadPool> mPool;
	std::vector<Formatter> mFormatters;
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::mutex mWorkMutex;
	std::condition_variable mWorkerCondition;
	std::map<std::string, std::function<NativeFormatterResult( const std::string& file )>>
		mNativeFormatters;
	Int32 mWorkersCount{ 0 };
	std::string mConfigPath;
	std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
	std::map<TextDocument*, bool> mIsAutoFormatting;
	std::map<std::string, LSPServerCapabilities> mCapabilities;
	Mutex mCapabilitiesMutex;

	bool mAutoFormatOnSave{ false };
	bool mShuttingDown{ false };
	bool mReady{ false };

	FormatterPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	void loadFormatterConfig( const std::string& path, bool updateConfigFile );

	void formatDoc( UICodeEditor* editor );

	void runFormatter( UICodeEditor* editor, const Formatter& formatter, const std::string& path );

	size_t formatterFilePatternPosition( const std::vector<std::string>& patterns );

	FormatterPlugin::Formatter supportsFormatter( std::shared_ptr<TextDocument> doc );

	bool supportsLSPFormatter( std::shared_ptr<TextDocument> doc );

	bool formatDocWithLSP( std::shared_ptr<TextDocument> doc );

	void registerNativeFormatters();

	bool tryRequestCapabilities( const std::shared_ptr<TextDocument>& doc );

	PluginRequestHandle processResponse( const PluginMessage& msg );
};

} // namespace ecode

#endif // ECODE_FORMATTERPLUGIN_HPP
