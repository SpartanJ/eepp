#ifndef EE_GAMINGCUIMAP_HPP
#define EE_GAMINGCUIMAP_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/clightmanager.hpp>
#include <eepp/gaming/clight.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class EE_API cUIMap : public cUIComplexControl {
	public:
		typedef cb::Callback1<void, cLight *> LightSelectCb;
		typedef cb::Callback1<void, cLight *> LightRadiusChangeCb;

		cUIMap( const cUIComplexControl::CreateParams& Params, cMap * Map = NULL );

		virtual ~cUIMap();

		virtual void Draw();

		virtual void Update();

		cMap * Map() const;

		void EditingLights( const bool& editing );

		const bool& EditingLights();

		cLight * GetSelectedLight();

		cLight * GetAddLight();

		void AddLight( cLight * Light );

		void SetLightSelectCb( LightSelectCb Cb );

		void SetLightRadiusChangeCb( LightRadiusChangeCb Cb );

		void ClearLights();
	protected:
		cMap *				mMap;
		bool				mEditingLights;
		cLight *			mAddLight;
		cLight *			mSelLight;
		LightSelectCb		mLightSelCb;
		LightRadiusChangeCb	mLightRadiusChangeCb;

		virtual Uint32 OnMouseMove( const eeVector2i& Pos, const Uint32 Flags );

		virtual void OnSizeChange();

		virtual void UpdateScreenPos();

		virtual void OnAlphaChange();

		void MapDraw();

		void TryToSelectLight();
};

}}}

#endif
