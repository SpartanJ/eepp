#ifndef EE_UI_UITIMEPICKER_HPP
#define EE_UI_UITIMEPICKER_HPP

#include <eepp/ui/uidatetimeedit.hpp>

namespace EE { namespace UI {

/** Time-only segmented editor. Time steps wrap within the day; no popup is required. */
class EE_API UITimePicker : public UIDateTimeEdit {
  public:
	static UITimePicker* New();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

  protected:
	UITimePicker();
};

}} // namespace EE::UI

#endif
