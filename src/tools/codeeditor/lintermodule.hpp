#ifndef EE_TOOLS_LINTER_HPP
#define EE_TOOLS_LINTER_HPP

#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

enum class LinterType {
	Notice,
	Warning,
	Error
};

struct Linter {
	std::vector<std::string> files;
	std::string warningPattern;
	struct {
		int line{ 1 };
		int col{ 2 };
		int message{ 3 };
		int type{ -1 };
	} warningPatternOrder;
	std::string command;
};

struct LinterMatch {
	std::string text;
	TextPosition pos;
	String::HashType lineCache;
	Rectf box;
	LinterType type{ LinterType::Error };
};

class LinterModule : public UICodeEditorModule {
  public:
	LinterModule( const std::string& lintersPath, std::shared_ptr<ThreadPool> pool );

	virtual ~LinterModule();

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	void drawAfterLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							const Float& fontSize, const Float& lineHeight );

	void update( UICodeEditor* );

	bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& );

	bool onMouseLeave( UICodeEditor*, const Vector2i&, const Uint32& );

	const Time& getDelayTime() const;

	void setDelayTime( const Time& delayTime );

  protected:
	std::shared_ptr<ThreadPool> mPool;
	std::vector<Linter> mLinters;
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	std::unordered_map<TextDocument*, std::unique_ptr<Clock>> mDirtyDoc;
	std::unordered_map<TextDocument*, std::map<Int64, LinterMatch>> mMatches;
	Time mDelayTime{ Seconds( 0.5f ) };
	Mutex mDocMutex;
	Mutex mMatchesMutex;

	bool mReady{ false };
	bool mClosing{ false };

	void load( const std::string& lintersPath );

	void lintDoc( TextDocument* doc );

	void runLinter( TextDocument* doc, const Linter& linter, const std::string& path );

	Linter supportsLinter( TextDocument* doc );

	void setDocDirty( TextDocument* doc );

	void setDocDirty( UICodeEditor* editor );

	void invalidateEditors( TextDocument* doc );

};

#endif // EE_TOOLS_LINTER_HPP
