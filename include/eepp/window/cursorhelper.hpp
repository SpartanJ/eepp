#ifndef EE_WINDOWCURSORHELPER_HPP
#define EE_WINDOWCURSORHELPER_HPP

namespace EE { namespace Window {

/** @enum EE_SYSTEM_CURSOR list the system cursors that can be used */
enum EE_SYSTEM_CURSOR {
	SYS_CURSOR_ARROW = 0, /**< Arrow */
	SYS_CURSOR_IBEAM,     /**< I-beam */
	SYS_CURSOR_WAIT,      /**< Wait */
	SYS_CURSOR_CROSSHAIR, /**< Crosshair */
	SYS_CURSOR_WAITARROW, /**< Small wait cursor (or Wait if not available) */
	SYS_CURSOR_SIZENWSE,  /**< Double arrow pointing northwest and southeast */
	SYS_CURSOR_SIZENESW,  /**< Double arrow pointing northeast and southwest */
	SYS_CURSOR_SIZEWE,    /**< Double arrow pointing west and east */
	SYS_CURSOR_SIZENS,    /**< Double arrow pointing north and south */
	SYS_CURSOR_SIZEALL,   /**< Four pointed arrow pointing north, south, east, and west */
	SYS_CURSOR_NO,        /**< Slashed circle or crossbones */
	SYS_CURSOR_HAND,      /**< Hand */
	SYS_CURSOR_COUNT,
	SYS_CURSOR_NONE
};

enum EE_CURSOR_TYPE {
	EE_CURSOR_ARROW = 0, /**< Arrow */
	EE_CURSOR_IBEAM,     /**< I-beam */
	EE_CURSOR_WAIT,      /**< Wait */
	EE_CURSOR_CROSSHAIR, /**< Crosshair */
	EE_CURSOR_WAITARROW, /**< Small wait cursor (or Wait if not available) */
	EE_CURSOR_SIZENWSE,  /**< Double arrow pointing northwest and southeast */
	EE_CURSOR_SIZENESW,  /**< Double arrow pointing northeast and southwest */
	EE_CURSOR_SIZEWE,    /**< Double arrow pointing west and east */
	EE_CURSOR_SIZENS,    /**< Double arrow pointing north and south */
	EE_CURSOR_SIZEALL,   /**< Four pointed arrow pointing north, south, east, and west */
	EE_CURSOR_NO,        /**< Slashed circle or crossbones */
	EE_CURSOR_HAND,      /**< Hand */
	EE_CURSOR_COUNT
};

}}

#endif
 
