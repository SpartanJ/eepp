#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIMenuItem : public UIPushButton {
  public:
	static UIMenuItem* New();

	UIMenuItem();

	virtual ~UIMenuItem();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

  protected:
	explicit UIMenuItem( const std::string& tag );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );
};

}} // namespace EE::UI

#endif
