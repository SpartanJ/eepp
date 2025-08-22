#ifndef EE_UI_UIBUILDSETTINGS_HPP
#define EE_UI_UIBUILDSETTINGS_HPP

#include "projectbuild.hpp"
#include <eepp/ui/uidatabind.hpp>
#include <eepp/ui/uirelativelayout.hpp>

namespace EE { namespace UI {
class UIDropDownList;
}} // namespace EE::UI
using namespace EE::UI;

namespace ecode {

class UIBuildSettings : public UIRelativeLayout {
  public:
	static UIBuildSettings*

	New( ProjectBuild& build, ProjectBuildConfiguration& config, bool isNew,
		 const std::function<void( const std::string& oldName, const std::string& newName )> onBuildNameChange );

	virtual ~UIBuildSettings();

	void setTab( UITab* tab );

	UITab* getTab() const;

  protected:
	friend class UIBuildStep;

	ProjectBuild& mBuild;
	ProjectBuildConfiguration& mConfig;
	UIDataBindHolder<int, bool, std::string> mDataBindHolder;
	UITab* mTab{ nullptr };
	String mOldName;
	std::unordered_map<UIWidget*, std::vector<Uint32>> mCbs;
	ProjectBuildOutputParserConfig mTmpOpCfg;
	bool mIsNew{ false };
	bool mCanceled{ false };
	std::function<void( const std::string& oldName, const std::string& newName )> mNewNameFn;

	explicit UIBuildSettings( ProjectBuild& build, ProjectBuildConfiguration& config, bool isNew,
		 const std::function<void( const std::string& oldName, const std::string& newName )> onBuildNameChange );

	void moveStepUp( size_t stepNum, bool isClean );

	void moveStepDown( size_t stepNum, bool isClean );

	void moveStepDir( size_t stepNum, bool isClean, int dir );

	void deleteStep( size_t stepNum, bool isClean );

	void updateOS();

	void refreshTab();

	void bindTable( const std::string& name, const std::string& key, ProjectBuildKeyVal& data );

	void runSelect( Uint32 index = 0xFFFFFFFF );

	void runRemove( bool all, UIDropDownList* runList, UIDropDownList* panelRunListDDL );

	void runUpdate( bool recreateList, UIDropDownList* runList, UIDropDownList* panelRunListDDL );

	Uint32 runIndex() const;

	void runSetup();
};

} // namespace ecode

#endif // EE_UI_UIBUILDSETTINGS_HPP
