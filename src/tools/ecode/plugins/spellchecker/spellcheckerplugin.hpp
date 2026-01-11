#pragma once

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include <eepp/system/process.hpp>

#include <set>

namespace ecode {

struct SpellCheckerMatch {
	std::string text;
	TextRange range;
	String::HashType lineHash{ 0 };
	std::unordered_map<UICodeEditor*, Rectf> box;
	std::vector<std::string> alternatives;
};

class SpellCheckerPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "spellchecker",
				 "Spell Checker",
				 "Check the spelling of the current documents.",
				 SpellCheckerPlugin::New,
				 { 0, 0, 1 },
				 SpellCheckerPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~SpellCheckerPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	void drawAfterLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							const Float& fontSize, const Float& lineHeight ) override;

	void minimapDrawBefore( UICodeEditor* /*editor*/, const DocumentLineRange&,
							const DocumentViewLineRange&, const Vector2f& /*linePos*/,
							const Vector2f& /*lineSize*/, const Float& /*charWidth*/,
							const Float& /*gutterWidth*/,
							const DrawTextRangesFn& /* drawTextRanges */ ) override;

	bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu, const Vector2i& position,
							  const Uint32& flags ) override;

	const Time& getDelayTime() const;

	void setDelayTime( const Time& delayTime );

  protected:
	Time mDelayTime{ Seconds( 0.5f ) };
	std::unordered_map<TextDocument*, std::unique_ptr<Clock>> mDirtyDoc;
	std::unordered_map<TextDocument*, std::map<Int64, std::vector<SpellCheckerMatch>>> mMatches;
	std::set<std::string> mLanguagesDisabled;
	std::set<std::string> mLSPLanguagesDisabled;
	std::mutex mWorkMutex;
	std::condition_variable mWorkerCondition;
	Int32 mWorkersCount{ 0 };
	std::mutex mRunningProcessesMutex;
	std::unordered_map<TextDocument*, Process*> mRunningProcesses;
	Mutex mMatchesMutex;
	bool mTyposFound{ false };
	Clock mClock;

	SpellCheckerPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	void onDocumentLoaded( TextDocument* ) override;

	void onRegisterDocument( TextDocument* ) override;

	void onUnregisterDocument( TextDocument* ) override;

	void onDocumentChanged( UICodeEditor*, TextDocument* /*oldDoc*/ ) override;

	void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& /*listeners*/ ) override;

	void update( UICodeEditor* ) override;

	void setDocDirty( TextDocument* doc );

	void setDocDirty( UICodeEditor* editor );

	void spellCheckDoc( std::shared_ptr<TextDocument> doc );

	void runSpellChecker( std::shared_ptr<TextDocument> doc, const std::string& path );

	void setMatches( TextDocument* doc, std::map<Int64, std::vector<SpellCheckerMatch>>&& matches );

	void invalidateEditors( TextDocument* doc );

	std::optional<SpellCheckerMatch> getMatchFromTextPosition( UICodeEditor* editor, TextPosition pos );

	std::optional<SpellCheckerMatch> getMatchFromScreenPos( UICodeEditor* editor, Vector2f pos );

	void replaceMatchWithText( const TextRange& range, const std::string& newText, UICodeEditor* );

	void createSpellCheckAlternativesView( UICodeEditor* editor );

	void goToNextError( UICodeEditor* editor );

	void goToPrevError( UICodeEditor* editor );

};

} // namespace ecode
