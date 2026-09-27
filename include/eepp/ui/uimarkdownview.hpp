#ifndef EE_UI_UIMARKDOWNVIEW_HPP
#define EE_UI_UIMARKDOWNVIEW_HPP

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uitextselectioncontroller.hpp>

namespace EE { namespace UI {

class EE_API UIMarkdownView : public UILinearLayout {
  public:
	static UIMarkdownView* New();

	UIMarkdownView();

	virtual ~UIMarkdownView();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual UITextSelectionController* getTextSelectionController();

	virtual const UITextSelectionController* getTextSelectionController() const;

	void loadFromString( std::string_view markdown );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	UITextSelectionController mTextSelectionController;

	virtual void scheduledUpdate( const Time& time );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onMessage( const NodeMessage* message );
};

}} // namespace EE::UI

#endif
