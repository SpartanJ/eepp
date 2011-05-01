#ifndef EECSPRITE_H
#define EECSPRITE_H

#include "base.hpp"
#include "ctexturefactory.hpp"
#include "cshape.hpp"
#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

/** @brief A Sprite controller class, can hold and control sprites animations. */
class EE_API cSprite {
	public:
		typedef cb::Callback2< void, Uint32, cSprite * > SpriteCallback;

		enum SpriteEvents {
			SPRITE_EVENT_LAST_FRAME,
			SPRITE_EVENT_FIRST_FRAME,
			SPRITE_EVENT_END_ANIM_TO
		};

		cSprite();

		~cSprite();

		cSprite& operator =( const cSprite& Other );

		/** Set the x axis position */
		void X( const eeFloat& X );

		/** Get the x axis position */
		eeFloat X() const;

		/** Set the y axis position */
		void Y( const eeFloat& Y );

		/** Get the y axis position */
		eeFloat Y() const;

		/** Set the Angle for the rendered sprite */
		void Angle( const eeFloat& Angle );

		/** Get the Angle for the rendered sprite */
		eeFloat Angle() const;

		/** Set the Scale for the rendered sprite */
		void Scale( const eeFloat& Scale );

		/** Get the Scale for the rendered sprite */
		eeFloat Scale() const;

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

		/** Set the sprite animation speed ( AnimSpeed equals to Animation Frames per Second ) */
		void AnimSpeed( const eeFloat& AnimSpeed );

		/** Get the sprite animation speed ( AnimSpeed equals to Animation Frames per Second ) */
		eeFloat AnimSpeed() const;

		/** @return If the animation is paused */
		bool AnimPaused() const;

		/** Set the animation paused or not */
		void AnimPaused( const bool& Pause );

		/** Set the sprite color */
		void Color( const eeColorA& Color);

		/** Get the sprite color */
		const eeColorA& Color() const;

		/** Set the sprite Color Alpha */
		void Alpha( const Uint8& Alpha );

		/** Get the sprite Color Alpha */
		const Uint8& Alpha() const;

		/** Get if the sprite it's scaled from the center */
		bool ScaleCentered() const;

		/** Set if the sprite it's scaled centered or scaled from the Left - Top position of the sprite ( default True ) */
		void ScaleCentered(const bool& ScaleCentered );

		/** Set the Current Frame */
		void CurrentFrame( const eeFloat& CurFrame );

		/** Get the Current Frame */
		const eeUint& CurrentFrame() const;

		/** Get the Exact Current FrameData
		* @return The eeFloat fpoint of the current frame, the exact position of the interpolation.
		*/
		const eeFloat& ExactCurrentFrame() const;

		/** Set the exact current FrameData */
		void ExactCurrentFrame( const eeFloat& CurrentFrame );

		/** Set the Current Sub Frame */
		void CurrentSubFrame( const eeUint &CurSubFrame );

		/** Get the Current Sub Frame */
		const eeUint& CurrentSubFrame() const;

		/** Set the Render Type */
		void SetRenderType( const EE_RENDERTYPE& Effect );

		/** Set the Blend Mode */
		void SetRenderAlphas( const EE_PRE_BLEND_FUNC& Blend );

		/** Reset the sprite as a new one. */
		void Reset();

		/** @return The AABB (axis-aligned bounding box) */
		eeAABB GetAABB();

		/** Update the sprite position */
		void Position( const eeFloat& x, const eeFloat& y );

		/** Update the sprite position from a Vector */
		void Position( const eeVector2f& NewPos );

		/** @return The Position of the sprite */
		const eeVector2f Position() const;

		/** Update the sprite size of a frame number */
		void UpdateSize( const eeFloat& Width, const eeFloat& Height, const eeUint& FrameNum = 0 );

		/** Update the colors of every vertex rendered of the sprite ( this will override the default color )
		* @param Color0 The Left - Top vertex color
		* @param Color1 The Left - Bottom vertex color
		* @param Color2 The Right - Bottom vertex color
		* @param Color3 The Right - Top vertex color
		*/
		void UpdateVertexColors( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3 );

		/** This will disable the vertex colors */
		void DisableVertexColors();

		/** Update the sprite
		* @param x Set the x axis position
		* @param y Set the y axis position
		* @param Scale Set the Scale
		* @param Angle Set the Angle
		* @param Alpha Set the Color Alpha
		* @param Color Set the Color
		*/
		void Update( const eeFloat& x, const eeFloat& y, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0.0f, const Uint8& Alpha = 255, const eeColorA& Color = eeColorA() );

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
		void Draw( const EE_PRE_BLEND_FUNC& Blend, const EE_RENDERTYPE& Effect, const eeFloat& ElapsedTime = -99999.f );

		/** Draw the sprite to the screen forcing the Blend Mode
		* @param Blend The Blend Mode
		*/
		void Draw( const EE_PRE_BLEND_FUNC& Blend );

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
		void ReverseAnim( const bool& Reverse );

		/** @return If the animation is reversed */
		bool ReverseAnim() const;

		/** @return The current last frame */
		eeUint GetEndFrame();

		/** @return The number of frames */
		Uint32 GetNumFrames();

		/** Will set Reverse active and set the first frame as the last frame */
		void SetReverseFromStart();

		/** @return The Current Shape */
		cShape * GetCurrentShape();

		/** @return The Shape Frame from the current sub frame */
		cShape * GetShape( const eeUint& frame );

		/** @return The Shape Frame from the SubFrame */
		cShape * GetShape( const eeUint& frame, const eeUint& SubFrame );

		/** Start playing from
		** @param GoTo Frame that goes from 1 to Number of Frames
		*/
		void GoToAndPlay( Uint32 GoTo );

		/** Go to a frame and stop
		** @param GoTo Frame that goes from 1 to Number of Frames
		*/
		void GoToAndStop( Uint32 GoTo );

		/** Animate to frame and when reach the frame stops */
		void AnimToFrameAndStop( Uint32 GoTo );

		/** Set the sprite events callback */
		void SetEventsCallback( const SpriteCallback& Cb );

		/** Removes the current callback */
		void ClearCallback();

		/** Creates a copy of the current sprite and return it */
		cSprite * Copy();
	protected:
		enum SpriteFlags {
			SPRITE_FLAG_AUTO_ANIM				= ( 1 << 0 ),
			SPRITE_FLAG_SCALE_CENTERED			= ( 1 << 1 ),
			SPRITE_FLAG_REVERSE_ANIM			= ( 1 << 2 ),
			SPRITE_FLAG_ANIM_PAUSED				= ( 1 << 3 ),
			SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP	= ( 1 << 4 ),
			SPRITE_FLAG_EVENTS_ENABLED			= ( 1 << 5 )
		};

		Uint32				mFlags;
		eeVector2f			mPos;
		eeFloat				mAngle;
		eeFloat				mScale;
		eeFloat				mAnimSpeed;

		eeColorA			mColor;
		eeColorA *			mVertexColors;

		eeInt				mRepeations; //!< Number of repetions of the animation, default -1 that equals to loop.

		EE_PRE_BLEND_FUNC	mBlend;
		EE_RENDERTYPE		mEffect;

		eeUint				mCurrentFrame;
		eeFloat				mfCurrentFrame;
		eeUint				mCurrentSubFrame;
		eeUint				mSubFrames;
		eeUint				mAnimTo;

		SpriteCallback		mCb;

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

		void FireEvent( const Uint32& Event );
};

}}

#endif
