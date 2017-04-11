#ifndef EE_UI_UILOADER_HPP
#define EE_UI_UILOADER_HPP

#include <eepp/graphics/arcdrawable.hpp>
#include <eepp/graphics/circledrawable.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UILoader : public UIWidget {
	public:
		static UILoader * New();

		UILoader();

		virtual void draw();

		virtual void update();

		UILoader * setOutlineThickness( const Float& thickness );

		const Float& getOutlineThickness() const;

		UILoader * setRadius( const Float& radius );

		const Float& getRadius() const;

		UILoader * setFillColor( const Color& color );

		const Color& getFillColor() const;

		const bool& isIndeterminate() const;

		UILoader * setIndeterminate( const bool& indeterminate );

		UILoader * setProgress( const Float& progress );

		const Float& getProgress() const;

		const Float& getMaxProgress() const;

		UILoader * setMaxProgress( const Float& maxProgress );

		const Float& getAnimationSpeed() const;

		UILoader * setAnimationSpeed( const Float& animationSpeed );

		virtual void loadFromXmlNode( const pugi::xml_node& node);
	protected:
		Float mRadius;
		Float mOutlineThickness;
		ArcDrawable mArc;
		CircleDrawable mCircle;
		Color mColor;
		Float mArcAngle;
		Float mArcStartAngle;
		Float mProgress;
		Float mMaxProgress;
		Float mAnimationSpeed;
		IntPtr mOp;
		bool mIndeterminate;
		bool mNeedsUpdate;

		virtual void onPositionChange();

		virtual void onSizeChange();
};

}}

#endif
