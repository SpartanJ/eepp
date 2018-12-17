#ifndef EE_GRAPHICSCSPRITE_HPP
#define EE_GRAPHICSCSPRITE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/textureatlas.hpp>

#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief A Sprite controller class, can hold and control sprites animations. */
class EE_API Sprite : public Drawable {
	public:
		/// Event ID - Sprite - User Data
		typedef std::function<void( Uint32, Sprite *, void * )> SpriteCallback;

		/** @brief SpriteEvents The events that can be reported by the Sprite */
		enum SpriteEvents {
			SPRITE_EVENT_LAST_FRAME,
			SPRITE_EVENT_FIRST_FRAME,
			SPRITE_EVENT_END_ANIM_TO,
			SPRITE_EVENT_USER			// User Events
		};

		static Sprite * New();

		static Sprite * New( const std::string& name, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		static Sprite * New( TextureRegion * TextureRegion );

		static Sprite * New( const Uint32& TexId, const Sizef &DestSize = Sizef(0,0), const Vector2i &offset = Vector2i(0,0), const Rect& TexSector = Rect(0,0,0,0) );

		/** Instanciate an empty sprite */
		Sprite();

		/** Creates an animated Sprite from a animation name. It will search for a pattern name.
		* For example search for name "car" with extensions "png", i will try to find car00.png car01.png car02.png, and so on, it will continue if find something, otherwise it will stop ( it will always search at least for car00.png and car01.png ).
		* @param name First part of the sub texture name
		* @param extension Extension of the sub texture name ( if have one, otherwise is empty )
		* @param SearchInTextureAtlas If you want only to search in a especific atlas ( NULL if you want to search in all atlases )
		* @note Texture atlases saves the TextureRegions names without extension by default.
		* @see TextureAtlasManager::GetTextureRegionsByPattern
		*/
		Sprite( const std::string& name, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		/** Creates a Sprite from a TextureRegion
		**	@param TextureRegion The TextureRegion to use */
		Sprite( TextureRegion * TextureRegion );

		/** Creates a Sprite instance that holds a new TextureRegion from a texture already loaded.
		*	@param TexId The texture Id used to create the TextureRegion
		*	@param DestSize The destination size of the TextureRegion created
		*	@param Offset The offset added to the position of the frame ( the TextureRegion )
		*	@param TexSector The sector of the texture used by the TextureRegion to be rendered
		*/
		Sprite( const Uint32& TexId, const Sizef &DestSize = Sizef(0,0), const Vector2i &offset = Vector2i(0,0), const Rect& TexSector = Rect(0,0,0,0) );

		virtual ~Sprite();

		Sprite& operator =( const Sprite& Other );

		/** Set the Angle for the rendered sprite */
		void setRotation( const Float& rotation );

		/** @return The Angle for the rendered sprite */
		Float getRotation() const;

		/** Rotates the sprite. Adds the new angle to the current rotation. Same as:
		**	@code sprite.Angle( sprite.Angle() + angle ); @endcode */
		void rotate( const Float& angle );

		/** Set the Scale for the rendered sprite */
		void setScale( const Float& scale );

		/** Set the Scale for the rendered sprite */
		void setScale( const Vector2f& scale );

		/** @return The Scale for the rendered sprite */
		const Vector2f& getScale() const;

		/**	@brief Set the local origin of the sprite
		**	The origin of an object defines the center point for
		**	all transformations (scale, rotation).
		**	The coordinates of this point must be relative to the
		**	top-left corner of the sprite.
		**	The default origin point is the center of the sprite. */
		void setOrigin( const OriginPoint& origin );

		/** @return The local origin of the sprite */
		const OriginPoint& getOrigin() const;

		/** Set the Frame Number Sprite Size
		* @param Size The new size
		* @param FrameNum If the Frame Number is 0 it will use the Current Frame Number
		* @param SubFrame If the Sub Frame Number is 0 it will use the Current Sub Frame Number
		*/
		void setSize( const Sizef& size, const unsigned int& FrameNum, const unsigned int& SubFrame );

		/** Set the current TextureRegion Size ( destination size ) */
		void setSize( const Sizef& size );

		/** @return the Frame Number Sprite Size
		* @param FrameNum If the Frame Number is 0 it will use the Current Frame Number
		* @param SubFrame If the Sub Frame Number is 0 it will use the Current Sub Frame Number
		*/
		Sizef setSize( const unsigned int& FrameNum, const unsigned int& SubFrame );

		/** @return The current Frame Size */
		Sizef getSize();

		/** Set the sprite animation speed ( AnimSpeed equals to Animation Frames per Second ) */
		void setAnimationSpeed( const Float& animSpeed );

		/** @return The sprite animation speed ( AnimSpeed equals to Animation Frames per Second ) */
		Float getAnimationSpeed() const;

		/** @return If the animation is paused */
		bool isAnimationPaused() const;

		/** Set the animation paused or not */
		void setAnimationPaused( const bool& Pause );

		/** Set the Current Frame */
		void setCurrentFrame( unsigned int CurFrame );

		/** @return The Current Frame */
		const unsigned int& getCurrentFrame() const;

		/** @return The Exact Current FrameData
		* @return The Float fpoint of the current frame, the exact position of the interpolation.
		*/
		const Float& getExactCurrentFrame() const;

		/** Set the exact current FrameData */
		void setExactCurrentFrame( const Float& currentFrame );

		/** Set the Current Sub Frame */
		void setCurrentSubFrame( const unsigned int &CurSubFrame );

		/** @return The Current Sub Frame */
		const unsigned int& getCurrentSubFrame() const;

		/** Set the Render Type */
		void setRenderMode( const RenderMode& Effect );

		/** @return The Render Type */
		const RenderMode& getRenderMode() const;

		/** Set the Blend Mode */
		void setBlendMode( const BlendMode& Blend );

		/** @return The Blend Mode */
		const BlendMode& getBlendMode() const;

		/** Reset the sprite as a new one. */
		void reset();

		/** @return The AABB (axis-aligned bounding box) */
		Rectf getAABB();

		/** Update the colors of every vertex rendered of the sprite ( this will override the default color )
		* @param Color0 The Left - Top vertex color
		* @param Color1 The Left - Bottom vertex color
		* @param Color2 The Right - Bottom vertex color
		* @param Color3 The Right - Top vertex color
		*/
		void updateVertexColors( const Color& Color0, const Color& Color1, const Color& Color2, const Color& Color3 );

		/** This will disable the vertex colors */
		void disableVertexColors();

		/** Creates an static sprite (no animation)
		* @param TextureRegion The sprite TextureRegion
		* @return True if success
		*/
		bool createStatic( TextureRegion * TextureRegion );

		/** Creates an static sprite (no animation). It creates a new TextureRegion.
		* @param TexId The internal Texture Id
		* @param DestSize The destination size of the TextureRegion created
		* @param Offset The offset added to the position of the frame ( the TextureRegion )
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return True if success
		*/
		bool createStatic(const Uint32& TexId, const Sizef &DestSize = Sizef(0,0), const Vector2i &offset = Vector2i(0,0), const Rect& TexSector = Rect(0,0,0,0) );

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
		unsigned int addFrame( const Uint32& TexId, const Sizef& DestSize = Sizef(0,0), const Vector2i& offset = Vector2i(0,0), const Rect& TexSector = Rect(0,0,0,0) );

		/** Add a frame to the sprite (on the current sub frame)
		* @param TextureRegion The TextureRegion used in the frame
		* @return The frame position or 0 if fails
		*/
		unsigned int addFrame( TextureRegion * TextureRegion );

		/** Add a vector of TextureRegion as an animation.
		* @param TextureRegions The Frames
		*/
		bool addFrames( const std::vector<TextureRegion*> TextureRegions );

		/** @see TextureAtlasManager::GetTextureRegionsByPattern */
		bool addFramesByPattern( const std::string& name, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		bool addFramesByPatternId( const Uint32& TextureRegionId, const std::string& extension, TextureAtlas * SearchInTextureAtlas );

		/** Add a frame on an specific subframe to the sprite
		* @param TexId The internal Texture Id
		* @param NumFrame The Frame Number
		* @param NumSubFrame The Sub Frame Number
		* @param DestSize The destination size of the frame
		* @param Offset The offset added to the x position of the frame
		* @param TexSector The texture sector to be rendered ( default all the texture )
		* @return True if success
		*/
		bool addSubFrame( const Uint32& TexId, const unsigned int& NumFrame, const unsigned int& NumSubFrame, const Sizef& DestSize = Sizef(0,0), const Vector2i& offset = Vector2i(0,0), const Rect& TexSector = Rect(0,0,0,0) );

		/** Add a frame on an specific subframe to the sprite
		* @param TextureRegion The TextureRegion used in the frame
		* @param NumFrame The Frame Number
		* @param NumSubFrame The Sub Frame Number
		* @return True if success
		*/
		bool addSubFrame( TextureRegion * TextureRegion, const unsigned int& NumFrame, const unsigned int& NumSubFrame );

		/** Draw the sprite to the screen */
		void draw();

		/** Draw the sprite to the screen forcing the Blend Mode and the Render Type
		* @param Blend The Blend Mode
		* @param Effect The Render Type
		*/
		void draw( const BlendMode& Blend, const RenderMode& Effect );

		void draw(const Vector2f & position);

		void draw(const Vector2f & position, const Sizef & size);

		virtual bool isStateful() { return false; }

		/** Set the number of repetitions of the animation. Any number below 0 the animation will loop. */
		void setRepetitions( const int& Repeations );

		/** Set if the class auto-animate the sprite ( default it's active ) */
		void setAutoAnimate( const bool& Autoanim );

		/** @return If the class is auto-animated */
		bool getAutoAnimate() const;

		/** @return The four vertex position of the Sprite */
		Quad2f getQuad();

		/** @return The Offset of the current frame */
		Vector2i getOffset();

		/** Set the Offset of the current frame */
		void setOffset( const Vector2i& offset );

		/** Reverse the animation from last frame to first frame. */
		void setReverseAnimation( const bool& Reverse );

		/** @return If the animation is reversed */
		bool getReverseAnimation() const;

		/** @return The current last frame */
		unsigned int getEndFrame();

		/** @return The number of frames */
		Uint32 getNumFrames();

		/** Will set Reverse active and set the first frame as the last frame */
		void setReverseFromStart();

		/** @return The Current TextureRegion */
		TextureRegion * getCurrentTextureRegion();

		/** @return The TextureRegion Frame from the current sub frame */
		TextureRegion * getTextureRegion( const unsigned int& frame );

		/** @return The TextureRegion Frame from the SubFrame */
		TextureRegion * getTextureRegion( const unsigned int& frame, const unsigned int& SubFrame );

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

		/** Creates a copy of the current sprite and returns it */
		Sprite clone();

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
		OriginPoint			mOrigin;
		Float				mRotation;
		Vector2f			mScale;
		Float				mAnimSpeed;

		Color *			mVertexColors;

		int					mRepetitions; //!< Number of repetions of the animation, default -1 that equals to loop.

		BlendMode		mBlend;
		RenderMode		mEffect;

		unsigned int		mCurrentFrame;
		Float				mfCurrentFrame;
		unsigned int		mCurrentSubFrame;
		unsigned int		mSubFrames;
		unsigned int		mAnimTo;

		SpriteCallback		mCb;
		void *				mUserData;

		class Frame {
			public:
				std::vector<TextureRegion *> Spr;
		};
		std::vector<Frame> mFrames;

		unsigned int framePos();

		void clearFrame();

		unsigned int getFrame( const unsigned int& FrameNum );

		unsigned int getSubFrame( const unsigned int& SubFrame );
};

}}

#endif
