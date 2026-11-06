#ifndef EPROC_PERFORMANCE_LAYOUT_HPP
#define EPROC_PERFORMANCE_LAYOUT_HPP

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

namespace eproc {

using namespace EE;
using namespace EE::UI;
using namespace EE::Scene;

class PerformanceLayout final : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static PerformanceLayout* New() { return eeNew( PerformanceLayout, () ); }

	PerformanceLayout() :
		UILinearLayout( "eproc-performance-layout", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getInput() ) {}

	KeyBindings* getOwnKeyBindings() override { return &getKeyBindings(); }

  protected:
	Uint32 onKeyDown( const KeyEvent& event ) override {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

} // namespace eproc

#endif // EPROC_PERFORMANCE_LAYOUT_HPP
