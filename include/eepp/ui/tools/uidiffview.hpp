#ifndef EE_UI_TOOLS_UIDIFFVIEW_HPP
#define EE_UI_TOOLS_UIDIFFVIEW_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>

namespace EE { namespace UI { namespace Tools {

class UIDiffEditorPlugin;

class EE_API UIDiffView : public UIWidget {
  public:
	static UIDiffView* New();

	UIDiffView();
	virtual ~UIDiffView();

	virtual Uint32 getType() const override;
	virtual bool isType( const Uint32& type ) const override;

	void loadFromPatch( const std::string& patchText );
	void loadFromStrings( const std::string& oldText, const std::string& newText );
	void loadFromFile( const std::string& oldFilePath, const std::string& newFilePath );

	UICodeEditor* getEditor() const { return mEditor; }

	enum class DiffLineType { Common, Added, Removed, Header };
	struct DiffLine {
		DiffLineType type{ DiffLineType::Common };
		String text;
		Int64 oldLineNum{ 0 };
		Int64 newLineNum{ 0 };
	};

	const std::vector<DiffLine>& getDiffLines() const { return mLines; }

  protected:
	virtual void onSizeChange() override;

	UICodeEditor* mEditor{ nullptr };
	std::shared_ptr<UIDiffEditorPlugin> mPlugin;
	std::vector<DiffLine> mLines;

	void buildEditor();
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UIDIFFVIEW_HPP
