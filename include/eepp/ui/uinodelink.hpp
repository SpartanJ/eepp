#pragma once

#include <eepp/ui/uiwidget.hpp>

namespace EE::UI {

class EE_API UINodeLink : public UIWidget {
  public:
	static UINodeLink* New();

	static UINodeLink* NewLink( UIWidget* link );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UINodeLink* setNodeLink( UIWidget* link );

	UIWidget* getNodeLink() const;

  protected:
	UINodeLink();

	UINodeLink( UIWidget* link );

	UIWidget* mNodeLink{ nullptr };
};

} // namespace EE::UI
