#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYSOURCE_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYSOURCE_HPP

#include <eepp/ui/accessibility/accessibility.hpp>

namespace EE { namespace UI {

/** Resolves virtual accessibility elements without materializing UIWidgets for them. */
class EE_API AccessibilitySource {
  public:
	virtual ~AccessibilitySource() = default;

	virtual bool isValid( Uint64 id ) const = 0;

	virtual AccessibilityNodeInfo getInfo( Uint64 id ) const = 0;

	virtual AccessibilityNodeRef getParent( Uint64 id ) const = 0;

	virtual size_t getChildCount( Uint64 id ) const = 0;

	virtual AccessibilityNodeRef getChild( Uint64 id, size_t index ) = 0;

	virtual size_t getRootChildCount() const = 0;

	virtual AccessibilityNodeRef getRootChild( size_t index ) = 0;

	virtual AccessibilityNodeRef hitTest( const Math::Vector2f& position ) = 0;

	virtual bool performAction( Uint64 id, const AccessibilityActionRequest& request ) = 0;

	virtual void invalidate() {}

	virtual void reset() { invalidate(); }
};

}} // namespace EE::UI

#endif
