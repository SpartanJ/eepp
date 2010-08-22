#ifndef EECSPRITE_H
#define EECSPRITE_H

#include "base.hpp"
#include "../window/cengine.hpp"
#include "ctexturefactory.hpp"
#include "cshape.hpp"
#include "cshapegroup.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

/** @brief A Sprite controller class, can hold and control sprites animations. */
class EE_API cSprite {
	public:
		cSprite();

		~cSprite();

		/** Set the x axis position */
		void X(const eeFloat &X) { mX = X; }

		/** Get the x axis position */
		eeFloat X() const { return mX; }

		/** Set the y axis position */
		void Y(const eeFloat& Y) { mY = Y; }

		/** Get the y axis position */
		eeFloat Y() const { return mY; }

		/** Set the Angle for the rendered sprite */
		void Angle( const eeFloat &Angle) { mAngle = Angle; }

		/** Get the Angle for the rendered sprite */
		eeFloat Angle() const { return mAngle; }

		/** Set the Scale for the rendered sprite */
		void Scale( const eeFloat &Scale) { mScale = Scale; }

		/** Get the Scale for the rendered sprite */
		eeFloat Scale() const { return mScale; }

		/** Set the Frame Number Sprite Width
		* @param FrameNum If the Frame Number is 0 use the Current Frame Number
		* @param SubFrame If the Sub Frame Number is 0 use the Current Sub Frame Number
		*/
		void Width( const eeFloat& Width, const eeUint& FrameNum = 0, const eeUint& SubFrame = 0 );

		/** Get the Frame Number Sprite Width */
		eeFloat Width( const eeUint& FrameNum = 0, const eeUint& SubFrame = 0 );

		/** Set the Frame Number Sprite Height
		* @param FrameNum If the Frame Number is 0 use the Current Frame Number
		* @param SubFrame If the Sub Frame Number is 0 use the Current Sub Frame Number
		*/
		void Height( const eeFloat& Height, const eeUint& FrameNum, const eeUint& SubFrame );

		/** Get the Frame Number Sprite Height */
		eeFloat Height( const eeUint& FrameNum = 0, const eeUint& SubFrame = 0 );

		/** Set the sprite animation speed */
		void AnimSpeed( const eeFloat& AnimSpeed) { mAnimSpeed = AnimSpeed; }

		/** Get the sprite animation speed */
		eeFloat AnimSpeed() const { return mAnimSpeed; }

		/** @return If the animation is paused */
		bool AnimPause() const { return mAnimPaused; }

		/** Set the animation paused or not */
		void AnimPause( const bool& Pause )	{ mAnimPaused = Pause; }

		/** Set the sprite color */
		void Color( const eeRGBA& Color) { mColor = Color; };

		/** Get the sprite color */
		eeRGBA Color() const { return mColor; }

		/** Set the sprite Color Alpha */
		void Alpha( const Uint8& Alpha) {
			mAlpha = Alpha;
			mColor.Alpha = (Uint8)mAlpha;
		}

		/** Get the sprite Color Alpha */
		eeFloat Alpha() const { return mAlpha; }

		/** Get if the sprite it's scaled from the center */
		bool ScaleCentered() const { return mScaleCentered; }

		/** Set if the sprite it's scaled centered or scaled from the Left - Top position of the sprite ( default True ) */
		void ScaleCentered(const bool &ScaleCentered) { mScaleCentered = ScaleCentered; }

		/** Set the Current Frame */
		void CurrentFrame( const eeFloat& CurFrame );

		/** Get the Current Frame */
		eeUint CurrentFrame() const { return mFrameData.CurrentFrame; }

		/** Get the Exact Current FrameData
		* @return The eeFloat fpoint of the current frame, the exact position of the interpolation.
		*/
		eeFloat ExactCurrentFrame() const { return mFrameData.fCurrentFrame; }

		/** Set the exact current FrameData */
		void ExactCurrentFrame( const eeFloat& CurrentFrame ) { mFrameData.fCurrentFrame = CurrentFrame; }

		/** Set the Current Sub Frame */
		void CurrentSubFrame( const eeUint &CurSubFrame );

		/** Get the Current Sub Frame */
		eeUint CurrentSubFrame() const { return mFrameData.CurrentSubFrame; }

		/** Set the Render Type */
		void SetRenderType( const EE_RENDERTYPE& Effect ) { mEffect = Effect; }

		/** Set the Blend Mode */
		void SetRenderAlphas( const EE_RENDERALPHAS& Blend ) { mBlend = Blend; }

		/** Reset the sprite as a new one. */
		void Reset();

		/** Get the current Source RECT of the sprite. The texture coordinates of the sprite. */
		eeRecti SprSrcRect();

		/** Get the current Destination RECT of the sprite. The sprite coordinates on the screen. (the AABB, axis-aligned bounding box) */
		eeRectf SprDestRectf();

		/** Get the current Destination RECT of the sprite. The sprite coordinates on the screen (the AABB, axis-aligned bounding box). */
		eeRecti SprDestRECT();

		/** @return The AABB (axis-aligned bounding box) */
		eeAABB GetAABB();

		/** Update the sprite position */
		void UpdatePos( const eeFloat& x, const eeFloat& y );

		/** Update the sprite position from a Vector */
		void UpdatePos( const eeVector2f& NewPos );

		/** Update the sprite size of a frame number */
		void UpdateSize( const eeFloat& Width, const eeFloat& Height, const eeUint& FrameNum = 0 );

		/** Update the scale of the sprite */
		void UpdateScale( const eeFloat& Scale ) { if(Scale>0) mScale = Scale; }

		/** Update the colors of every vertex rendered of the sprite ( this will override the default color )
		* @param Color0 The Left - Top vertex color
		* @param Color1 The Left - Bottom vertex color
		* @param Color2 The Right - Bottom vertex color
		* @param Color3 The Right - Top vertex color
		*/
		void UpdateVertexColors( const eeRGBA& Color0, const eeRGBA& Color1, const eeRGBA& Color2, const eeRGBA& Color3 );

		/** Update the sprite
		* @param x Set the x axis position
		* @param y Set the y axis position
		* @param Scale Set the Scale
		* @param Angle Set the Angle
		* @param Alpha Set the Color Alpha
		* @param Color Set the Color
		*/
		void Update( const eeFloat& x, const eeFloat& y, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0.0f, const Uint8& Alpha = 255, const eeRGBA& Color = eeRGBA(255,255,255,255) );

		/** Get the internal texture id from a frame number and a sub frame number
		* @param FrameNum The Frame Number
		* @param SubFrameNum The Sub Frame Number
		* @return The Internal Texture Id
		*/
		Uint32 GetTexture( const eeUint& FrameNum = 0, const eeUint& SubFrameNum = 0 );

		/** Creates an static sprite (no animation)
		* @param Shape The sprite shape
		* @return True if success
		*/
		bool CreateStatic( cShape * Shape );

		/** Creates an static sprite (no animation)
		* @param TexId The internal Texture Id
		* @param DestWidth The default destination width of the sprite
		* @param DestHeight The default destination height of the sprite
		* @param offSetX The default offset on axis x added to the x position
		* @param offSetY The default offset on axis y added to the y position
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return True if success
		*/
		bool CreateStatic( const Uint32& TexId, const eeFloat& DestWidth = 0, const eeFloat& DestHeight = 0, const eeFloat& offSetX = 0, const eeFloat& offSetY = 0, const eeRecti& TexSector = eeRecti(0,0,0,0) );

		/** Creates an animated sprite
		* @param SubFramesNum The number of subframes of the sprite
		*/
		void CreateAnimation( const eeUint& SubFramesNum = 1 );

		/** Add a frame to the sprite (on the current sub frame)
		* @param TexId The internal Texture Id
		* @param DestWidth The destination width of the frame
		* @param DestHeight The destination height of the frame
		* @param offSetX The offset on axis x added to the x position of the frame
		* @param offSetY The offset on axis y added to the y position of the frame
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return The frame position or 0 if fails
		*/
		eeUint AddFrame( const Uint32& TexId, const eeFloat& DestWidth = 0, const eeFloat& DestHeight = 0, const eeFloat& offSetX = 0, const eeFloat& offSetY = 0, const eeRecti& TexSector = eeRecti(0,0,0,0) );

		/** Add a frame to the sprite (on the current sub frame)
		* @param Shape The Shape used in the frame
		* @return The frame position or 0 if fails
		*/
		eeUint AddFrame( cShape * Shape );

		/** Add a vector of shapes as an animation.
		* @param Shapes The Frames
		*/
		bool AddFrames( const std::vector<cShape*> Shapes );

		/** @see cShapeGroupManager::GetShapesByPattern */
		bool AddFramesByPattern( const std::string& name, const std::string& extension = "", cShapeGroup * SearchInShapeGroup = NULL );

		/** Add a frame on an specific subframe to the sprite
		* @param TexId The internal Texture Id
		* @param NumFrame The Frame Number
		* @param NumSubFrame The Sub Frame Number
		* @param DestWidth The destination width of the frame
		* @param DestHeight The destination height of the frame
		* @param offSetX The offset on axis x added to the x position of the frame
		* @param offSetY The offset on axis y added to the y position of the frame
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return True if success
		*/
		bool AddSubFrame( const Uint32& TexId, const eeUint& NumFrame, const eeUint& NumSubFrame, const eeFloat& DestWidth = 0, const eeFloat& DestHeight = 0, const eeFloat& offSetX = 0, const eeFloat& offSetY = 0, const eeRecti& TexSector = eeRecti(0,0,0,0) );

		/** Add a frame on an specific subframe to the sprite
		* @param Shape The Shape used in the frame
		* @param NumFrame The Frame Number
		* @param NumSubFrame The Sub Frame Number
		* @return True if success
		*/
		bool AddSubFrame( cShape * Shape, const eeUint& NumFrame, const eeUint& NumSubFrame );

		/** Draw the sprite to the screen */
		void Draw();

		/** Draw the sprite to the screen forcing the Blend Mode and the Render Type
		* @param Blend The Blend Mode
		* @param Effect The Render Type
		* @param ElapsedTime The Elapsed Time for the animation ( -99999.f will take the elapsed time counted by the engine )
		*/
		void Draw( const EE_RENDERALPHAS& Blend, const EE_RENDERTYPE& Effect, const eeFloat& ElapsedTime = -99999.f );

		/** Draw the sprite to the screen forcing the Blend Mode
		* @param Blend The Blend Mode
		*/
		void Draw( const EE_RENDERALPHAS& Blend );

		/** Draw the sprite to the screen forcing the Render Type
		* @param Effect The Render Type
		*/
		void Draw( const EE_RENDERTYPE& Effect );

		/** Update the Frame Number SrcRECT
		* @param R The new SrcRECT
		* @param FrameNum The Frame Number to change the SrcRECT. Default change the Current mFrames.
		* @param SubFrame The Sub Frame Number to change the SrcRECT. Default change the Current Sub mFrames.
		*/
		void UpdateSprRECT( const eeRecti& R, const eeUint& FrameNum = 0, const eeUint& SubFrame = 0 );

		/** Set the number of repeations of the animation. Any number below 0 the animation will loop. */
		void SetRepeations( const int& Repeations );

		/** Set if the class autoanimate the sprite ( default it's true ) */
		void SetAutoAnimate( const bool& Autoanim );

		/** @return The four vertex position of the Sprite */
		eeQuad2f GetQuad();

		/** @return The OffSetX of the current frame */
		eeFloat OffSetX();

		/** Set the OffSetX of the current frame */
		void OffSetX( const eeFloat& offsetx );

		/** @return The OffSetY of the current frame */
		eeFloat OffSetY();

		/** Set the OffSetY of the current frame */
		void OffSetY( const eeFloat& offsety );

		/** Set the OffSet of the current frame */
		void OffSet( const eeVector2f& offset );

		/** Reverse the animation from last frame to first mFrames. */
		void ReverseAnim( const bool& Reverse ) { mReverseAnim = Reverse; }

		/** @return If the animation is reversed */
		bool ReverseAnim() const { return mReverseAnim; }

		/** @return The current last frame */
		eeUint GetEndFrame();

		/** @return The number of frames */
		eeUint GetNumFrames() { return (eeUint)mFrames.size(); }

		/** Will set Reverse active and set the first frame as the last frame */
		void SetReverseFromStart();

		/** @return The Current Shape */
		cShape * GetCurrentShape();

		/** @return The Shape Frame from the current sub frame */
		cShape* GetShape( const eeUint& frame );

		/** @return The Shape Frame from the SubFrame */
		cShape* GetShape( const eeUint& frame, const eeUint& SubFrame );
	protected:
		eeFloat mX, mY, mAngle, mScale, mAnimSpeed;
		bool mAutoAnim;
		bool mScaleCentered;
		EE_RENDERALPHAS mBlend;
		EE_RENDERTYPE mEffect;

		eeRGBA mColor, mColor0, mColor1, mColor2, mColor3;
		Uint8 mAlpha;
		Int16 mRepeations; //!< Number of repetions of the animation, default -1 that equals to loop.
		bool mReverseAnim;
		bool mAnimPaused;

		struct FrameData {
			eeUint CurrentFrame;
			eeFloat fCurrentFrame;
			eeUint CurrentSubFrame;
			eeUint SubFrames;
		} mFrameData;

		class cFrame {
			public:
				std::vector<cShape *> Spr;
		};
		std::vector<cFrame> mFrames;

		eeUint FramePos();
		void ClearFrame();
		void Animate( const eeFloat& ElapsedTime = -99999.f );

		eeVector2f GetRotationCenter( const eeRectf& DestRECT );

		eeUint GetFrame( const eeUint& FrameNum );
		eeUint GetSubFrame( const eeUint& SubFrame );
};

}}

#endif
