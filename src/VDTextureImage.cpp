/*
**   TextureImage for jpg, png
*/
/*
#include "VDTextureImage.h"
using namespace videodromm;

TextureImage::TextureImage() {
	mType = TextureType::IMAGE;
}
bool TextureImage::loadFromFullPath(const std::string& aPath) {
	if (fs::exists(aPath)) {
		mTexture = ci::gl::Texture::create(loadImage(aPath));
		mInputSurface = Surface(loadImage(aPath));
	}
	else {
		mTexture = ci::gl::Texture::create(mWidth, mHeight);
		mInputSurface = Surface(mWidth, mHeight, true);
	}
	mXLeft = 0;
	mYTop = 0;
	mXRight = mOriginalWidth = mTexture->getWidth();
	mYBottom = mOriginalHeight = mTexture->getHeight();
	return true;
}

ci::gl::Texture2dRef TextureImage::getTexture(unsigned int aIndex) {
	Area area(mXLeft, mYTop, mXRight, mYBottom);
	mProcessedSurface = mInputSurface.clone(area);
	mTexture = gl::Texture2d::create(mProcessedSurface, ci::gl::Texture::Format().loadTopDown(mFlipV));
	return mTexture;
}

TextureImage::~TextureImage(void) {
}*/

