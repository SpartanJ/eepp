#ifndef ECODE_LINTERPLUGIN_HPP
#define ECODE_LINTERPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <memory>
#include <set>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

enum class LinterType { Notice, Warning, Error };

enum class MatchOrigin { Linter, Diagnostics };

struct Linter {
	std::vector<std::string> languages;
	std::vector<std::string> files;
	std::vector<std::string> warningPattern;
	bool columnsStartAtZero{ false };
	bool deduplicate{ false };
	bool useTmpFolder{ false };
	bool hasNoErrorsExitCode{ false };
	struct {
		int line{ 1 };
		int col{ 2 };
		int message{ 3 };
		int type{ -1 };
	} warningPatternOrder;
	std::string command;
	std::vector<Int64> expectedExitCodes{};
	int noErrorsExitCode{ 0 };
	std::string url;
	bool isNative{ false };
};

struct LinterMatch {
	std::string text;
	TextRange range;
	LinterType type{ LinterType::Error };
	String::HashType lineHash{ 0 };
	MatchOrigin origin{ MatchOrigin::Linter };
	std::unordered_map<UICodeEditor*, Rectf> box;
	std::unordered_map<UICodeEditor*, Rectf> lensBox;
	LSPDiagnostic diagnostic;
};

class LinterPlugin : public Plugin {
  public:
	static PluginDefinition Definition() {
		return { "linter",
				 "Linter",
				 "Use static code analysis tool used to flag programming errors, bugs, "
				 "stylistic errors, and suspicious constructs.",
				 LinterPlugin::New,
				 { 0, 2, 8 },
				 LinterPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~LinterPlugin();

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	virtual String::HashType getConfigFileHash() { return mConfigHash; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	void drawAfterLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							const Float& fontSize, const Float& lineHeight );

	void minimapDrawBefore( UICodeEditor* /*editor*/, const DocumentLineRange&,
							const DocumentViewLineRange&, const Vector2f& /*linePos*/,
							const Vector2f& /*lineSize*/, const Float& /*charWidth*/,
							const Float& /*gutterWidth*/,
							const DrawTextRangesFn& /* drawTextRanges */ );

	void update( UICodeEditor* );

	bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& flags );

	bool onMouseLeave( UICodeEditor*, const Vector2i&, const Uint32& );

	bool onMouseClick( UICodeEditor*, const Vector2i&, const Uint32& flags );

	const Time& getDelayTime() const;

	void setDelayTime( const Time& delayTime );

	bool getEnableLSPDiagnostics() const;

	void setEnableLSPDiagnostics( bool enableLSPDiagnostics );

	bool getErrorLens() const;

	void setErrorLens( bool errorLens );

	const std::vector<Linter>& getLinters() const;

	Linter getLinterForLang( const std::string& lang );

	virtual bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
									  const Vector2i& position, const Uint32& flags );

	void registerNativeLinter( const std::string& cmd,
							   const std::function<void( std::shared_ptr<TextDocument> doc,
														 const std::string& file )>& nativeLinter );

	void unregisterNativeLinter( const std::string& cmd );

	void preDraw( UICodeEditor*, const Vector2f&, const Float&, const TextPosition& );

  protected:
	std::vector<Linter> mLinters;
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::unordered_set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	std::unordered_map<TextDocument*, std::unique_ptr<Clock>> mDirtyDoc;
	std::unordered_map<TextDocument*, std::map<Int64, std::vector<LinterMatch>>> mMatches;
	Time mDelayTime{ Seconds( 0.5f ) };
	Mutex mDocMutex;
	Mutex mMatchesMutex;
	std::mutex mWorkMutex;
	std::condition_variable mWorkerCondition;
	Int32 mWorkersCount{ 0 };
	std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
	std::mutex mRunningProcessesMutex;
	std::unordered_map<TextDocument*, Process*> mRunningProcesses;
	std::unordered_map<std::string, std::function<void( std::shared_ptr<TextDocument> doc,
														const std::string& file )>>
		mNativeLinters;

	bool mHoveringMatch{ false };
	bool mEnableLSPDiagnostics{ true };
	bool mErrorLens{ true };
	bool mGoToIgnoreWarnings{ false };
	bool mOldWordWrap{ false };
	std::set<std::string> mLanguagesDisabled;
	std::set<std::string> mLSPLanguagesDisabled;
	String::HashType mConfigHash{ 0 };
	UIIcon* mLightbulbIcon{ nullptr };
	std::string mErrorMsg;
	Rectf mQuickFixRect;
	std::string mOldMaxWidth;

	LinterPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	void lintDoc( std::shared_ptr<TextDocument> doc );

	void runLinter( std::shared_ptr<TextDocument> doc, const Linter& linter,
					const std::string& path );

	Linter supportsLinter( std::shared_ptr<TextDocument> doc );

	void setDocDirty( TextDocument* doc );

	void setDocDirty( UICodeEditor* editor );

	void invalidateEditors( TextDocument* doc );

	SyntaxStyleType getMatchString( const LinterType& type );

	void loadLinterConfig( const std::string& path, bool updateConfigFile );

	size_t linterFilePatternPosition( const std::vector<std::string>& patterns );

	PluginRequestHandle processMessage( const PluginMessage& notification );

	TextDocument* getDocumentFromURI( const URI& uri );

	void eraseMatchesFromOrigin( TextDocument* doc, const MatchOrigin& origin );

	void insertMatches( TextDocument* doc, std::map<Int64, std::vector<LinterMatch>>& matches );

	void setMatches( TextDocument* doc, const MatchOrigin& origin,
					 std::map<Int64, std::vector<LinterMatch>>& matches );

	void tryHideHoveringMatch( UICodeEditor* editor );

	void goToNextError( UICodeEditor* editor );

	void goToPrevError( UICodeEditor* editor );

	void registerNativeLinters();
};

} // namespace ecode

#endif // ECODE_LINTERPLUGIN_HPP
