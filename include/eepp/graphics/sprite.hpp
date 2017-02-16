#ifndef EE_GRAPHICSCSPRITE_HPP
#define EE_GRAPHICSCSPRITE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textureatlas.hpp>

namespace EE { namespace Graphics {

/** @brief A Sprite controller class, can hold and control sprites animations. */
class EE_API Sprite {
	public:
		/// Event ID - Sprite - User Data
		typedef cb::Callback3< void, Uint32, Sprite *, void * > SpriteCallback;

		/** @brief SpriteEvents The events that can be reported by the Sprite */
		enum SpriteEvents {
			SPRITE_EVENT_LAST_FRAME,
			SPRITE_EVENT_FIRST_FRAME,
			SPRITE_EVENT_END_ANIM_TO,
			SPRITE_EVENT_USER			// User Events
		};

		/** Instanciate an empty sprite */
		Sprite();

		/** Creates an animated Sprite from a animation name. It will search for a pattern name.
		* For example search for name "car" with extensions "png", i will try to find car00.png car01.png car02.png, and so on, it will continue if find something, otherwise it will stop ( it will always search at least for car00.png and car01.png ).
		* @param name First part of the sub texture name
		* @param extension Extension of the sub texture name ( if have one, otherwise is empty )
		* @param SearchInTextureAtlas If you want only to search in a especific atlas ( NULL if you want to search in all atlases )
		* @note Texture atlases saves the SubTextures names without extension by default.
		* @see TextureAtlasManager::GetSubTexturesByPattern
		*/
		Sprite( const std::string& name, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		/** Creates a Sprite from a SubTexture
		**	@param SubTexture The subtexture to use */
		Sprite( SubTexture * SubTexture );

		/** Creates a Sprite instance that holds a new SubTexture from a texture already loaded.
		*	@param TexId The texture Id used to create the SubTexture
		*	@param DestSize The destination size of the SubTexture created
		*	@param Offset The offset added to the position of the frame ( the SubTexture )
		*	@param TexSector The sector of the texture used by the SubTexture to be rendered
		*/
		Sprite( const Uint32& TexId, const Sizef &DestSize = Sizef(0,0), const Vector2i &offset = Vector2i(0,0), const Recti& TexSector = Recti(0,0,0,0) );

		virtual ~Sprite();

		Sprite& operator =( const Sprite& Other );

		/** Set the x axis position */
		void x( const Float& x );

		/** @return The x axis position */
		Float x() const;

		/** Set the y axis position */
		void y( const Float& y );

		/** @return The y axis position */
		Float y() const;

		/** Set the Angle for the rendered sprite */
		void angle( const Float& angle );

		/** @return The Angle for the rendered sprite */
		Float angle() const;

		/** Rotates the sprite. Adds the new angle to the current rotation. Same as:
		**	@code sprite.Angle( sprite.Angle() + angle ); @endcode */
		void rotate( const Float& angle );

		/** Set the Scale for the rendered sprite */
		void scale( const Float& scale );

		/** Set the Scale for the rendered sprite */
		void scale( const Vector2f& scale );

		/** @return The Scale for the rendered sprite */
		const Vector2f& scale() const;

		/**	@brief Set the local origin of the sprite
		**	The origin of an object defines the center point for
		**	all transformations (scale, rotation).
		**	The coordinates of this point must be relative to the
		**	top-left corner of the sprite.
		**	The default origin point is the center of the sprite. */
		void origin( const OriginPoint& origin );

		/** @return The local origin of the sprite */
		const OriginPoint& origin() const;

		/** Set the Frame Number Sprite Size
		* @param Size The new size
		* @param FrameNum If the Frame Number is 0 it will use the Current Frame Number
		* @param SubFrame If the Sub Frame Number is 0 it will use the Current Sub Frame Number
		*/
		void size( const Sizef& size, const unsigned int& FrameNum, const unsigned int& SubFrame );

		/** Set the current SubTexture Size ( destination size ) */
		void size( const Sizef& size );

		/** @return the Frame Number Sprite Size
		* @param FrameNum If the Frame Number is 0 it will use the Current Frame Number
		* @param SubFrame If the Sub Frame Number is 0 it will use the Current Sub Frame Number
		*/
		Sizef size( const unsigned int& FrameNum, const unsigned int& SubFrame );

		/** @return The current Frame Size */
		Sizef size();

		/** Set the sprite animation speed ( AnimSpeed equals to Animation Frames per Second ) */
		void animSpeed( const Float& animSpeed );

		/** @return The sprite animation speed ( AnimSpeed equals to Animation Frames per Second ) */
		Float animSpeed() const;

		/** @return If the animation is paused */
		bool animPaused() const;

		/** Set the animation paused or not */
		void animPaused( const bool& Pause );

		/** Set the sprite color */
		void color( const ColorA& color);

		/** @return The sprite color */
		const ColorA& color() const;

		/** Set the sprite Color Alpha */
		void alpha( const Uint8& alpha );

		/** @return The sprite Color Alpha */
		const Uint8& alpha() const;

		/** Set the Current Frame */
		void currentFrame( unsigned int CurFrame );

		/** @return The Current Frame */
		const unsigned int& currentFrame() const;

		/** @return The Exact Current FrameData
		* @return The Float fpoint of the current frame, the exact position of the interpolation.
		*/
		const Float& exactCurrentFrame() const;

		/** Set the exact current FrameData */
		void exactCurrentFrame( const Float& currentFrame );

		/** Set the Current Sub Frame */
		void currentSubFrame( const unsigned int &CurSubFrame );

		/** @return The Current Sub Frame */
		const unsigned int& currentSubFrame() const;

		/** Set the Render Type */
		void renderMode( const EE_RENDER_MODE& Effect );

		/** @return The Render Type */
		const EE_RENDER_MODE& renderMode() const;

		/** Set the Blend Mode */
		void blendMode( const EE_BLEND_MODE& Blend );

		/** @return The Blend Mode */
		const EE_BLEND_MODE& blendMode() const;

		/** Reset the sprite as a new one. */
		void reset();

		/** @return The AABB (axis-aligned bounding box) */
		eeAABB getAABB();

		/** Set the sprite position */
		void position( const Float& x, const Float& y );

		/** Set the sprite position from a Vector */
		void position( const Vector2f& NewPos );

		/** @return The Position of the sprite */
		const Vector2f position() const;

		/** Update the colors of every vertex rendered of the sprite ( this will override the default color )
		* @param Color0 The Left - Top vertex color
		* @param Color1 The Left - Bottom vertex color
		* @param Color2 The Right - Bottom vertex color
		* @param Color3 The Right - Top vertex color
		*/
		void updateVertexColors( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 );

		/** This will disable the vertex colors */
		void disableVertexColors();

		/** Creates an static sprite (no animation)
		* @param SubTexture The sprite SubTexture
		* @return True if success
		*/
		bool createStatic( SubTexture * SubTexture );

		/** Creates an static sprite (no animation). It creates a new SubTexture.
		* @param TexId The internal Texture Id
		* @param DestSize The destination size of the SubTexture created
		* @param Offset The offset added to the position of the frame ( the SubTexture )
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return True if success
		*/
		bool createStatic(const Uint32& TexId, const Sizef &DestSize = Sizef(0,0), const Vector2i &offset = Vector2i(0,0), const Recti& TexSector = Recti(0,0,0,0) );

		/** Creates an animated sprite
		* @param SubFramesNum The number of subframes of the sprite
		*/
		void createAnimation( const unsigned int& SubFramesNum = 1 );

		/** Add a frame to the sprite (on the current sub frame)
		* @param TexId The internal Texture Id
		* @param DestSize The destination size of the frame
		* @param Offset The offset added to the position of the frame
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return The frame position or 0 if fails
		*/
		unsigned int addFrame( const Uint32& TexId, const Sizef& DestSize = Sizef(0,0), const Vector2i& offset = Vector2i(0,0), const Recti& TexSector = Recti(0,0,0,0) );

		/** Add a frame to the sprite (on the current sub frame)
		* @param SubTexture The SubTexture used in the frame
		* @return The frame position or 0 if fails
		*/
		unsigned int addFrame( SubTexture * SubTexture );

		/** Add a vector of SubTexture as an animation.
		* @param SubTextures The Frames
		*/
		bool addFrames( const std::vector<SubTexture*> SubTextures );

		/** @see TextureAtlasManager::GetSubTexturesByPattern */
		bool addFramesByPattern( const std::string& name, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		bool addFramesByPatternId( const Uint32& SubTextureId, const std::string& extension, TextureAtlas * SearchInTextureAtlas );

		/** Add a frame on an specific subframe to the sprite
		* @param TexId The internal Texture Id
		* @param NumFrame The Frame Number
		* @param NumSubFrame The Sub Frame Number
		* @param DestSize The destination size of the frame
		* @param Offset The offset added to the x position of the frame
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return True if success
		*/
		bool addSubFrame( const Uint32& TexId, const unsigned int& NumFrame, const unsigned int& NumSubFrame, const Sizef& DestSize = Sizef(0,0), const Vector2i& offset = Vector2i(0,0), const Recti& TexSector = Recti(0,0,0,0) );

		/** Add a frame on an specific subframe to the sprite
		* @param SubTexture The SubTexture used in the frame
		* @param NumFrame The Frame Number
		* @param NumSubFrame The Sub Frame Number
		* @return True if success
		*/
		bool addSubFrame( SubTexture * SubTexture, const unsigned int& NumFrame, const unsigned int& NumSubFrame );

		/** Draw the sprite to the screen */
		void draw();

		/** Draw the sprite to the screen forcing the Blend Mode and the Render Type
		* @param Blend The Blend Mode
		* @param Effect The Render Type
		*/
		void draw( const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect );

		/** Draw the sprite to the screen forcing the Blend Mode
		* @param Blend The Blend Mode
		*/
		void draw( const EE_BLEND_MODE& Blend );

		/** Draw the sprite to the screen forcing the Render Type
		* @param Effect The Render Type
		*/
		void draw( const EE_RENDER_MODE& Effect );

		/** Set the number of repetitions of the animation. Any number below 0 the animation will loop. */
		void setRepetitions( const int& Repeations );

		/** Set if the class auto-animate the sprite ( default it's active ) */
		void autoAnimate( const bool& Autoanim );

		/** @return If the class is auto-animated */
		bool autoAnimate() const;

		/** @return The four vertex position of the Sprite */
		Quad2f getQuad();

		/** @return The Offset of the current frame */
		Vector2i offset();

		/** Set the Offset of the current frame */
		void offset( const Vector2i& offset );

		/** Reverse the animation from last frame to first mFrames. */
		void reverseAnim( const bool& Reverse );

		/** @return If the animation is reversed */
		bool reverseAnim() const;

		/** @return The current last frame */
		unsigned int getEndFrame();

		/** @return The number of frames */
		Uint32 getNumFrames();

		/** Will set Reverse active and set the first frame as the last frame */
		void setReverseFromStart();

		/** @return The Current SubTexture */
		SubTexture * getCurrentSubTexture();

		/** @return The SubTexture Frame from the current sub frame */
		SubTexture * getSubTexture( const unsigned int& frame );

		/** @return The SubTexture Frame from the SubFrame */
		SubTexture * getSubTexture( const unsigned int& frame, const unsigned int& SubFrame );

		/** Start playing from
		** @param GoTo Frame that goes from 1 to Number of Frames
		*/
		void goToAndPlay( Uint32 GoTo );

		/** Go to a frame and stop
		** @param GoTo Frame that goes from 1 to Number of Frames
		*/
		void goToAndStop( Uint32 GoTo );

		/** Animate to frame and when reach the frame stops */
		void animToFrameAndStop( Uint32 GoTo );

		/** Set the sprite events callback */
		void setEventsCallback( const SpriteCallback& Cb, void * UserData = NULL );

		/** Removes the current callback */
		void clearCallback();

		/** Creates a copy of the current sprite and return it */
		Sprite * copy();

		/** Update the sprite animation */
		void update( const Time& ElapsedTime );

		/** Update the sprite animation using the current elapsed time provided by Engine */
		void update();

		/** Fire a User Event in the sprite */
		void fireEvent( const Uint32& Event );
	protected:
		enum SpriteFlags {
			SPRITE_FLAG_AUTO_ANIM				= ( 1 << 0 ),
			SPRITE_FLAG_REVERSE_ANIM			= ( 1 << 1 ),
			SPRITE_FLAG_ANIM_PAUSED				= ( 1 << 2 ),
			SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP	= ( 1 << 3 ),
			SPRITE_FLAG_EVENTS_ENABLED			= ( 1 << 4 )
		};

		Uint32				mFlags;
		Vector2f			mPos;
		OriginPoint		mOrigin;
		Float				mAngle;
		Vector2f			mScale;
		Float				mAnimSpeed;

		ColorA			mColor;
		ColorA *			mVertexColors;

		int				mRepeations; //!< Number of repetions of the animation, default -1 that equals to loop.

		EE_BLEND_MODE		mBlend;
		EE_RENDER_MODE		mEffect;

		unsigned int				mCurrentFrame;
		Float				mfCurrentFrame;
		unsigned int				mCurrentSubFrame;
		unsigned int				mSubFrames;
		unsigned int				mAnimTo;

		SpriteCallback		mCb;
		void *				mUserData;

		class Frame {
			public:
				std::vector<SubTexture *> Spr;
		};
		std::vector<Frame> mFrames;

		unsigned int framePos();

		void clearFrame();

		unsigned int getFrame( const unsigned int& FrameNum );

		unsigned int getSubFrame( const unsigned int& SubFrame );
};

}}

#endif
