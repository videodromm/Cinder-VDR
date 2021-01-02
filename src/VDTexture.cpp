#include "VDTexture.h"

#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"

//using namespace ci;
//using namespace ci::app;
using namespace videodromm;
//namespace videodromm {
VDTextureRef VDTexture::create(VDParamsRef aVDParams, const JsonTree& json)
{
	CI_LOG_V("VDTexture create");
	return std::shared_ptr<VDTexture>(new VDTexture(aVDParams, json));
}
VDTexture::VDTexture(VDParamsRef aVDParams, const JsonTree& json) {
	// Params
	mVDParams = aVDParams;
	CI_LOG_V("VDTexture constructor");

	mTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mFlipV));
	mInputSurface = Surface(mVDParams->getFboWidth(), mVDParams->getFboHeight(), true);

	fboFmt.setColorTextureFormat(fmt);
	mFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
	// From rewrite
	mName = mCurrentSeqFilename = mLastCachedFilename = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "0.jpg";
	mTypestr = (json.hasChild("texturetype")) ? json.getValueForKey<string>("texturetype") : "UNKNOWN";
	mMode = (json.hasChild("texturemode")) ? json.getValueForKey<int>("texturemode") : 0;
	mAssetsPath = (json.hasChild("assetspath")) ? json.getValueForKey<string>("assetspath") : "";
	mType = TextureType::UNKNOWN;
	mStatus = "";
	mLastCachedFilename = mName;
	if (mName == "" || mName == "audio") {
		mName = mTypestr = "audio";
		mType = TextureType::AUDIO;
		//mTexture = mVDAnimation->getAudioTexture();

	}
	fs::path texFileOrPath = getAssetPath("") / mAssetsPath / mName;
	if (fs::exists(texFileOrPath)) {
		if (fs::is_directory(texFileOrPath)) {
			mType = TextureType::SEQUENCE;
			mTypestr = "sequence";
			mExt = "jpg";
			mCurrentSeqFilename = mName + " (1)." + mExt;
			mLastCachedFilename = mName + " (1)." + mExt;

			fs::path jpgPath = getAssetPath("") / mAssetsPath / mName / mCurrentSeqFilename;
			if (!fs::exists(jpgPath)) {
				// try with png
				mExt = "png";
				mCurrentSeqFilename = mName + " (1)." + mExt;
				mLastCachedFilename = mName + " (1)." + mExt;
			}

		}
		else {
			mExt = "";
			int dotIndex = texFileOrPath.filename().string().find_last_of(".");
			if (dotIndex != std::string::npos)  mExt = texFileOrPath.filename().string().substr(dotIndex + 1);
			if (mExt == "jpg" || mExt == "png") {
				mTexture = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
				mType = TextureType::IMAGE;
				mTypestr = "image";
			}
			else {
				/*if (ext == "mp4" || ext == "wmv" || ext == "avi" || ext == "mov") {
					if (!mVideo.isStopped()) {
						mVideo.stop();
					}

					mIsVideoLoaded = mVideo.loadMovie(texFileOrPath);
					mType = MOVIE;
					mVideoDuration = mVideo.getDuration();
					mVideoPos = mVideo.getPosition();
					mVideo.play();
				}*/
			}
		}
	}
	else {
		mName = mTypestr = "audio";
		mType = TextureType::AUDIO;

		//mTexture = mVDAnimation->getAudioTexture(); // init with audio texture
	}
	mStatus = mName;

}
VDTexture::VDTexture(TextureType aType)
	: mPath("")
	, mName("")
	, mFlipV(false)
	, mFlipH(true)
	, mWidth(1280)
	, mHeight(720)
{
	mBoundsLocked = true;
	mXLeft = 0;
	mYTop = 0;
	mPosition = 1;
	mXRight = mOriginalWidth = mWidth;
	mYBottom = mOriginalHeight = mHeight;
	mAreaWidth = mWidth;
	mAreaHeight = mHeight;
	mFolder = "";
	mSyncToBeat = false;
	mPlaying = true;
	if (mName.length() == 0) {
		mName = mPath;
	}
	// init the texture whatever happens next
	if (mPath.length() > 0) {
		mTexture = ci::gl::Texture::create(ci::loadImage(mPath), ci::gl::Texture::Format().loadTopDown(mFlipV));
		mInputSurface = Surface(loadImage(mPath));
	}
	else {
		mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown(mFlipV));
		mInputSurface = Surface(mWidth, mHeight, true);
	}
	fboFmt.setColorTextureFormat(fmt);
	mFbo = gl::Fbo::create(mWidth, mHeight, fboFmt);

}
VDTexture::~VDTexture(void) {

}
/*VDTextureList VDTexture::readSettings(VDAnimationRef aVDAnimation, const DataSourceRef &source)
{
	XmlTree			doc;
	VDTextureList	vdtexturelist;
	bool            isValid = true;
	// try to load the specified xml file
	try { doc = XmlTree(source); }
	catch (...) { return vdtexturelist; }

	// check if this is a valid file
	bool isOK = doc.hasChild("textures");
	//if (!isOK) return vdtexturelist;

	//
	if (isOK) {

		XmlTree texturesXml = doc.getChild("textures");

		// iterate textures
		for (XmlTree::ConstIter child = texturesXml.begin("texture"); child != texturesXml.end(); ++child) {
			// create texture of the correct type
			std::string texturetype = child->getAttributeValue<std::string>("texturetype", "unknown");
			XmlTree detailsXml = child->getChild("details");

			//std::string path = detailsXml.getAttributeValue<std::string>("path", "");
			if (texturetype == "image") {
				TextureImageRef t(new TextureImage());
				t->fromXml(detailsXml);
				vdtexturelist.push_back(t);
			}
			else if (texturetype == "imagesequence") {
				TextureImageSequenceRef t(new TextureImageSequence(aVDAnimation));
				t->fromXml(detailsXml);
				vdtexturelist.push_back(t);
			}
			else if (texturetype == "camera") {
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
					TextureCameraRef t(new TextureCamera());
					t->fromXml(detailsXml);
					vdtexturelist.push_back(t);
#else
					// camera not supported on this platform
					CI_LOG_V("camera not supported on this platform");
					isValid = false;
#endif
				}
				else if (texturetype == "shared") {
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
					TextureSharedRef t(new TextureShared());
					t->fromXml(detailsXml);
					vdtexturelist.push_back(t);
#else
					// shared textures not supported on this platform
					CI_LOG_V("shared textures not supported on this platform");
					isValid = false;
#endif
				}
				else if (texturetype == "audio") {
					TextureAudioRef t(new TextureAudio(aVDAnimation));
					t->fromXml(detailsXml);
					vdtexturelist.push_back(t);
				}
				else if (texturetype == "stream") {
					TextureStreamRef t(new TextureStream(aVDAnimation));
					t->fromXml(detailsXml);
					vdtexturelist.push_back(t);
				}
				else {
					// unknown texture type
					CI_LOG_V("unknown texture type");
				}
			}
			if (!isValid)
			{
				TextureImageRef t(new TextureImage());
				XmlTree		xml;
				xml.setTag("details");
				xml.setAttribute("path", "0.jpg");
				xml.setAttribute("width", 1280);
				xml.setAttribute("height", 720);
				t->fromXml(xml);
				vdtexturelist.push_back(t);
			}
		}
		else {
			// malformed XML
			CI_LOG_V("malformed XML");
			TextureImageRef t(new TextureImage());
			XmlTree		xml;
			xml.setTag("details");
			xml.setAttribute("path", "0.jpg");
			xml.setAttribute("width", 1280);
			xml.setAttribute("height", 720);
			t->fromXml(xml);
			vdtexturelist.push_back(t);
		}

		return vdtexturelist;
	}

	void VDTexture::writeSettings(const VDTextureList &vdtexturelist, const ci::DataTargetRef &target) {

		// create config document and root <textures>
		XmlTree			doc;
		doc.setTag("textures");
		doc.setAttribute("version", "1.0");

		//
		for (unsigned i {0}; i < vdtexturelist.size(); ++i) {
			// create <texture>
			XmlTree			texture;
			texture.setTag("texture");
			texture.setAttribute("id", i + 1);
			switch (vdtexturelist[i]->mType) {
			case IMAGE: texture.setAttribute("texturetype", "image"); break;
			case SEQUENCE: texture.setAttribute("texturetype", "imagesequence"); break;
			case SHARED: texture.setAttribute("texturetype", "shared"); break;
			case CAMERA: texture.setAttribute("texturetype", "camera"); break;
			case AUDIO: texture.setAttribute("texturetype", "audio"); break;
			default: texture.setAttribute("texturetype", "unknown"); break;
			}
			// details specific to texture type
			texture.push_back(vdtexturelist[i]->toXml());

			// add texture doc
			//texture.setAttribute("path", vdtexturelist[i]->mPath);
			doc.push_back(texture);
		}

		// write file
		doc.write(target);
	}
	XmlTree	VDTexture::toXml() const
	{
		XmlTree		xml;
		xml.setTag("details");
		xml.setAttribute("path", mPath);
		xml.setAttribute("width", mWidth);
		xml.setAttribute("height", mHeight);

		return xml;
	}

	bool VDTexture::fromXml(const XmlTree &xml)
	{
		return true;
	}*/
void VDTexture::toggleLoadingFromDisk() {

}
bool VDTexture::isLoadingFromDisk() {
	return false;
}
// play/pause (sequence/movie)
void VDTexture::togglePlayPause() {

	mPlaying = !mPlaying;
}

// sync to beat
void VDTexture::syncToBeat() {

	mSyncToBeat = !mSyncToBeat;
}
void VDTexture::reverse() {
}
float VDTexture::getSpeed() {
	return 1;
}
void VDTexture::setSpeed(float speed) {
}

void VDTexture::setPlayheadPosition(int position) {
}
int VDTexture::getMaxFrame() {
	return 1;
}
bool VDTexture::loadFromFullPath(const std::string& aPath) {
	// initialize texture
	mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown(mFlipV));
	return true;
}
void VDTexture::lockBounds(bool lock, unsigned int aWidth, unsigned int aHeight) {
	mBoundsLocked = lock;
	mAreaWidth = aWidth;
	mAreaHeight = aHeight;
}
void VDTexture::setXLeft(int aXleft) {
	mXLeft = aXleft;
	if (mBoundsLocked) {
		mXRight = mXLeft + mAreaWidth;
	}
};
void VDTexture::setYTop(int aYTop) {
	mYTop = aYTop;
	if (mBoundsLocked) {
		mYBottom = mYTop + mAreaHeight;
	}
};
void VDTexture::setXRight(int aXRight) {
	mXRight = aXRight;
	if (mBoundsLocked) {
		mXLeft = mXRight - mAreaWidth;
	}
};
void VDTexture::setYBottom(int aYBottom) {
	mYBottom = aYBottom;
	if (mBoundsLocked) {
		mYTop = mYBottom - mAreaHeight;
	}
}
void VDTexture::flipV() {
	mFlipV = !mFlipV;
}
void VDTexture::flipH() {
	mFlipH = !mFlipH;
}
bool VDTexture::getLockBounds() {
	return mBoundsLocked;
}
void VDTexture::toggleLockBounds() {
	mBoundsLocked = !mBoundsLocked;
};
unsigned int VDTexture::getTextureWidth() {
	return mWidth;
};

unsigned int VDTexture::getTextureHeight() {
	return mHeight;
};
unsigned int VDTexture::getOriginalWidth() {
	return mOriginalWidth;
};
unsigned int VDTexture::getOriginalHeight() {
	return mOriginalHeight;
};
ci::ivec2 VDTexture::getSize() {
	return mTexture->getSize();
}

ci::Area VDTexture::getBounds() {
	return mTexture->getBounds();
}

GLuint VDTexture::getId() {
	return mTexture->getId();
}

std::string VDTexture::getName() {
	return mName;
}

ci::gl::TextureRef VDTexture::getTexture(unsigned int aIndex) {
	return mTexture;
}
/*
**   Child classes
*/


