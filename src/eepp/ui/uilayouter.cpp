#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uilayouter.hpp>

namespace EE { namespace UI {

void UILayouter::setMatchParentIfNeededVerticalGrowth() {
	mContainer->asType<UILayout>()->setMatchParentIfNeededVerticalGrowth();
}

}} // namespace EE::UI
