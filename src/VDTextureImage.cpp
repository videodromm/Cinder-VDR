/*
**   TextureImage for jpg, png
*/

#include "VDTextureImage.h"
using namespace videodromm;

TextureImage::TextureImage() {
	mType = TextureType::IMAGE;
}
/*
XmlTree	TextureImage::toXml() const {
	XmlTree xml = VDTexture::toXml();

	// add attributes specific to this type of texture
	xml.setAttribute("path", mPath);
	xml.setAttribute("flipv", mFlipV);
	xml.setAttribute("fliph", mFlipH);
	return xml;
}

bool TextureImage::fromXml(const XmlTree& xml)
{
	VDTexture::fromXml(xml);
	// retrieve attributes specific to this type of texture
	mFlipV = xml.getAttributeValue<bool>("flipv", "true"); // default true
	mFlipH = xml.getAttributeValue<bool>("fliph", "true"); // default true
	mPath = xml.getAttributeValue<string>("path", "");
	mFolder = xml.getAttributeValue<string>("folder", "");
	mName = mPath;
	if (mPath.length() > 0) {
		fs::path fullPath = getAssetPath("") / mFolder / mPath;
		loadFromFullPath(fullPath.string());
	}
	return true;
}*/
bool TextureImage::loadFromFullPath(std::string aPath) {
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
}

