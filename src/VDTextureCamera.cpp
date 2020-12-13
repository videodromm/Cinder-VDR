
/*
** ---- TextureCamera ------------------------------------------------
*/
#include "VDTextureCamera.h"
using namespace videodromm;


TextureCamera::TextureCamera() {
	mType = TextureType::CAMERA;
	mFirstCameraDeviceName = "";
	printDevices();

	try {
		mCapture = Capture::create(1280, 720);
		mCapture->start();
	}
	catch (ci::Exception& exc) {
		CI_LOG_EXCEPTION("Failed to init capture ", exc);
	}
}
/*bool TextureCamera::fromXml(const XmlTree& xml) {
	// init		
	mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown(mFlipV));
	// retrieve attributes specific to this type of texture
	mPath = xml.getAttributeValue<string>("path", "");
	mName = "camera";
	return true;
}
XmlTree	TextureCamera::toXml() const {
	XmlTree xml = VDTexture::toXml();

	// add attributes specific to this type of texture
	xml.setAttribute("camera", mFirstCameraDeviceName);
	xml.setAttribute("flipv", mFlipV);
	xml.setAttribute("fliph", mFlipH);
	xml.setAttribute("path", mPath);
	return xml;
}*/
ci::gl::Texture2dRef TextureCamera::getTexture(unsigned int aIndex) {
	if (mCapture && mCapture->checkNewFrame()) {
		if (!mTexture) {
			// Capture images come back as top-down, and it's more efficient to keep them that way
			mTexture = gl::Texture::create(*mCapture->getSurface(), gl::Texture::Format().loadTopDown());
		}
		else {
			mTexture->update(*mCapture->getSurface());
		}
	}
	return mTexture;
}
void TextureCamera::printDevices() {
	for (const auto& device : Capture::getDevices()) {
		console() << "Device: " << device->getName() << " "
#if defined( CINDER_COCOA_TOUCH )
			<< (device->isFrontFacing() ? "Front" : "Rear") << "-facing"
#endif
			<< std::endl;
		mFirstCameraDeviceName = device->getName();
	}
}
TextureCamera::~TextureCamera(void) {

}

