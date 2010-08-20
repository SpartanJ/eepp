#ifndef EE_GRAPHICSCTEXTUREGROUPLOADER_HPP
#define EE_GRAPHICSCTEXTUREGROUPLOADER_HPP

#include "base.hpp"
#include "packerhelper.hpp"
#include "ctextureloader.hpp"
#include "ctexturefactory.hpp"
#include "../system/cresourceloader.hpp"

namespace EE { namespace Graphics {

using namespace Private;

class EE_API cTextureGroupLoader {
	public:
		cTextureGroupLoader();

		cTextureGroupLoader( const std::string& TextureGroupPath, const bool& Threaded = false );

		~cTextureGroupLoader();

		void 				Update();

		void				Load( const std::string& TextureGroupPath = "" );

		bool				Threaded() const;

		void				Threaded( const bool& threaded );

		const bool&		IsLoaded() const;
	protected:
		cResourceLoader		mRL;
		std::string			mTextureGroupPath;
		bool				mThreaded;
		bool				mLoaded;
		std::string			mAppPath;

		typedef struct sTempTexGroupS {
			sTextureHdr 			Texture;
			std::vector<sShapeHdr>	Shapes;
		} sTempTexGroup;

		std::vector<sTempTexGroup> mTempGroups;

		void CreateShapes();
};

}}

#endif
