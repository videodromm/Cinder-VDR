#include "VDTexture.h"

#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"

using namespace ci;
using namespace ci::app;

namespace videodromm {
	VDTexture::VDTexture(TextureType aType)
		: mPath("")
		, mName("")
		, mWidth(1280)
		, mHeight(720)
	{
		mBoundsLocked = true;
		mXLeft = 0;
		mYTop = 0;
		mPosition = 1;
		mSpeed = 0.01f;
		mXRight = mOriginalWidth = mWidth;
		mYBottom = mOriginalHeight = mHeight;
		mAreaWidth = mWidth;
		mAreaHeight = mHeight;
		mAssetsPath = "";
		mSyncToBeat = false;
		mPlaying = true;
		if (mName.length() == 0) {
			mName = mPath;
		}
		// init the texture whatever happens next
		if (mPath.length() > 0) {
			mTexture = ci::gl::Texture::create(ci::loadImage(mPath), ci::gl::Texture::Format().loadTopDown());
			mInputSurface = Surface(loadImage(mPath));
		}
		else {
			mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown());
			mInputSurface = Surface(mWidth, mHeight, true);
		}
		fboFmt.setColorTextureFormat(fmt);
		mFbo = gl::Fbo::create(mWidth, mHeight, fboFmt);

	}
	VDTexture::~VDTexture(void) {

	}
	
	bool VDTexture::fromJson(const JsonTree& json)
	{
		return true;
	}
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
		mSpeed *= -1.0f;
	}
	float VDTexture::getSpeed() {
		return mSpeed;
	}
	void VDTexture::setSpeed(float speed) {
		mSpeed = speed;
	}

	void VDTexture::setPlayheadPosition(int position) {
	}
	int VDTexture::getMaxFrame() {
		return 1;
	}
	bool VDTexture::loadFromFullPath(const std::string& aPath) {
		// initialize texture
		mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown());
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
	bool VDTexture::getLockBounds() {
		return mBoundsLocked;
	}
	void VDTexture::toggleLockBounds() {
		mBoundsLocked = !mBoundsLocked;
	};
	unsigned int VDTexture::getTextureWidth() {
		int aw = mTexture->getActualWidth();
		int w = mTexture->getWidth();
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

	ci::gl::TextureRef VDTexture::getTexture() {
		return mTexture;
	}
	ci::gl::Texture2dRef VDTexture::getCachedTexture(const std::string& aFilename) {
		return mTexture;
	}
	/*
	**   Child classes
	*/
	/*
	**   TextureImage for jpg, png
	*/
	TextureImage::TextureImage() {
		mType = IMAGE;
	}
	

	bool TextureImage::fromJson(const JsonTree& json)
	{
		VDTexture::fromJson(json);
		// retrieve attributes specific to this type of texture
		mPath = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "0.jpg";
		mAssetsPath = (json.hasChild("assetspath")) ? json.getValueForKey<string>("assetspath") : "";

		// find filename without full path
		int slashIndex = mPath.find_last_of("\\");

		if (slashIndex != std::string::npos) {
			mName = mPath.substr(slashIndex + 1);
		}
		else {
			mName = mPath;
		}
		if (mPath.length() > 0) {
			loadFromFullPath(mPath);
		}
		return true;
	}
	bool TextureImage::loadFromFullPath(const std::string& aPath) {
		
		if (fs::exists(aPath)) {
			mTexture = ci::gl::Texture::create(loadImage(aPath));
			mInputSurface = Surface(loadImage(aPath));
		}
		else {
			fs::path fullPath = getAssetPath("") / mAssetsPath / aPath;
			if (fs::exists(fullPath)) {
				mTexture = ci::gl::Texture::create(loadImage(fullPath.string()));
				mInputSurface = Surface(loadImage(aPath));
				mName = fullPath.filename().string();
			}
			else {
				mTexture = ci::gl::Texture::create(mWidth, mHeight);
				mInputSurface = Surface(mWidth, mHeight, true);
			}
		}
		mXLeft = 0;
		mYTop = 0;
		mXRight = mOriginalWidth = mTexture->getWidth();
		mYBottom = mOriginalHeight = mTexture->getHeight();
		return true;
	}

	ci::gl::Texture2dRef TextureImage::getTexture() {
		Area area(mXLeft, mYTop, mXRight, mYBottom);
		mProcessedSurface = mInputSurface.clone(area);
		mTexture = gl::Texture2d::create(mProcessedSurface, ci::gl::Texture::Format().loadTopDown());
		return mTexture;
	}
	ci::gl::Texture2dRef TextureImage::getCachedTexture(const std::string& aFilename) {
		return TextureImage::getTexture();
	}
	TextureImage::~TextureImage(void) {
	}

	/*
	** TextureImageSequence
	*/
	TextureImageSequence::TextureImageSequence(VDAnimationRef aVDAnimation) {
		// constructor
		mVDAnimation = aVDAnimation;
		mType = SEQUENCE;
		playheadFrameInc = 0.0f;
		mLoadingFilesComplete = true;
		mLoadingPaused = false;
		mFramesLoaded = 0;
		mCurrentLoadedFrame = 0;
		mNextIndexFrameToTry = 0;
		mPlaying = false;
		mExt = "png";
		mPrefix = "none";
		mNextIndexFrameToTry = 0;
		mPosition = 0;
		mNumberOfDigits = 4;
		mLastCachedFilename = "a (1).jpg";
		startGlobal = Clock::now();
		mStatus = "...";
	}
	bool TextureImageSequence::loadFromFullPath(const std::string& aPath)
	{
		bool validFile = false; // if no valid files in the folder, we keep existing vector
		string anyImagefileName = "0.jpg";
		string folder = "";
		string fileName;
		fs::path fullPath = aPath;
		if (fs::exists(fullPath)) {
			try {
				bool firstIndexFound = false;
				// loading 2000 files takes a while, I load only the first one
				for (fs::directory_iterator it(fullPath); it != fs::directory_iterator(); ++it)
				{
					// if file(1).jpg filename, mNumberOfDigits is 2
					if (mNumberOfDigits == 2) {
						break;
					}
					if (fs::is_regular_file(*it))
					{
						fileName = it->path().filename().string();
						mName = fileName;
						if (fileName.find_last_of(".") != std::string::npos) {
							int dotIndex = fileName.find_last_of(".");
							mExt = fileName.substr(dotIndex + 1);
							if (mExt == "png" || mExt == "jpg") {
								anyImagefileName = fileName;
							}
							// get the prefix for the image sequence, the files are named from p0000.jpg to p2253.jpg for instance, sometimes only 3 digits : l000 this time
							// find the first digit
							int firstDigit = fileName.find_first_of("0123456789");
							// if valid image sequence (contains a digit)
							if (firstDigit > -1) {
								mNumberOfDigits = dotIndex - firstDigit;
								int prefixIndex = fileName.find_last_of(".") - mNumberOfDigits;//-4 or -3
								mPrefix = fileName.substr(0, prefixIndex);
								if (!firstIndexFound) {
									firstIndexFound = true;
									mNextIndexFrameToTry = std::stoi(fileName.substr(prefixIndex, dotIndex));
								}
							}
						}
						// only if proper image sequence
						if (firstIndexFound) {
							if (mExt == "png" || mExt == "jpg") {
								if (!validFile) {
									// we found a valid file
									validFile = true;
									mSequenceTextures.clear();
									// TODO only store folder relative to assets, not full path 
									size_t found = fullPath.string().find_last_of("/\\");
									mPath = fullPath.string().substr(found + 1);

									// reset playhead to the start
									//mPlayheadPosition = 0;
									mLoadingFilesComplete = false;
									loadNextImageFromDisk();
									mPlaying = true;
								}
							}
						}

					}
				}
				CI_LOG_V("successfully loaded " + mPath);
			}
			catch (Exception &exc) {
				CI_LOG_EXCEPTION("error loading ", exc);
			}

			// init: if no valid file found we take the default 0.jpg
			if (!validFile) {
				// might want to remove default file as we are now using a boolean to notify the caller
				if (anyImagefileName.length() > 0) {
					mTexture = ci::gl::Texture::create(loadImage(loadAsset(anyImagefileName)), ci::gl::Texture::Format().loadTopDown());
					//mSequenceTextures[anyImagefileName] = mTexture;
					mSequenceTextures.push_back(ci::gl::Texture::create(loadImage(loadAsset(anyImagefileName)), gl::Texture::Format().loadTopDown()));
					mLoadingFilesComplete = true;
					mFramesLoaded = 1;
				}
			}
		}
		return validFile;
	}
	bool TextureImageSequence::fromJson(const JsonTree& json)
	{
		bool rtn = false;
		// init		
		mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown());
		// retrieve attributes specific to this type of texture
		mPath = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "intro";
		mAssetsPath = (json.hasChild("assetspath")) ? json.getValueForKey<string>("assetspath") : "";

		mName = mPath;
		if (mPath.length() > 0) {
			if (fs::exists(mPath)) {
				rtn = loadFromFullPath(mPath);
			}
			else {
				fs::path fullPath = getAssetPath("") / mAssetsPath / mPath;// TODO
				rtn = loadFromFullPath(fullPath.string());
			}
		}
		return rtn;
	}
	
	void TextureImageSequence::loadNextImageFromDisk() {
		// can't pause if UI not ready anyways if (!mLoadingPaused) {

		if (!mLoadingFilesComplete) {
			// thank you Omar!
			char restOfFileName[32];
			switch (mNumberOfDigits)
			{
			case 2:
				sprintf(restOfFileName, "%d)", mNextIndexFrameToTry);
				break;
			case 3:
				sprintf(restOfFileName, "%03d", mNextIndexFrameToTry);
				break;
			case 4:
				sprintf(restOfFileName, "%04d", mNextIndexFrameToTry);
				break;
			default:
				break;
			}

			string fileNameToLoad = mPrefix + restOfFileName + "." + mExt;
			fs::path fileToLoad = getAssetPath("") / mPath / fileNameToLoad;
			if (fs::exists(fileToLoad)) {
				// start profiling
				auto start = Clock::now();

				if (mCachedTextures[fileNameToLoad]) {
					mSequenceTextures.push_back(mCachedTextures[fileNameToLoad]);
				}
				else {
					// 20190727 TODO CHECK
					mSequenceTextures.push_back(ci::gl::Texture::create(loadImage(fileToLoad), gl::Texture::Format().loadTopDown()));
					// 20191014
					mCachedTextures[fileNameToLoad] = ci::gl::Texture::create(loadImage(fileToLoad), gl::Texture::Format().loadTopDown());
				}

				/*mTexture = ci::gl::Texture::create(loadImage(fileToLoad));
				mXLeft = 0;
				mYTop = 0;
				mXRight = mOriginalWidth = mTexture->getWidth();
				mYBottom = mOriginalHeight = mTexture->getHeight();
				mInputSurface = Surface(loadImage(fileToLoad));
				//mInputSurface = Surface(mWidth, mHeight, true);
				Area area(mXLeft, mYTop, mXRight, mYBottom);
				mProcessedSurface = mInputSurface.clone(area);
				mTexture = gl::Texture2d::create(mProcessedSurface, ci::gl::Texture::Format().loadTopDown(mFlipV));
				mSequenceTextures.push_back(mTexture); */
				mCurrentLoadedFrame = mFramesLoaded;
				mFramesLoaded++;
				auto end = Clock::now();
				auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				int milli = msdur.count();

				mStatus = toString(milli) + "ms " + fileNameToLoad;
			}
			else {
				//mStatus = fileToLoad.string() + " does not exist";
				/* 20191018
				if (mFramesLoaded > 0) {
					// if frames have been loaded we hit the last file of the image sequence at this point
					mStatus = "last image loaded";
					mLoadingFilesComplete = true;
				} */
			}
			// increment counter for next filename
			mNextIndexFrameToTry++;
			if (mNextIndexFrameToTry > 530) {
				mLoadingFilesComplete = true;
				//mStatus = "stop at 530";
				auto endGlobal = Clock::now();
				auto msdurGlobal = std::chrono::duration_cast<std::chrono::milliseconds>(endGlobal - startGlobal);
				int milliGlobal = msdurGlobal.count();
				mStatus = toString(milliGlobal) + "ms total";
			}
			/* 20191018 obsolete
			if (mNextIndexFrameToTry > 9999 && mNumberOfDigits == 4) mLoadingFilesComplete = true;
			if (mNextIndexFrameToTry > 999 && mNumberOfDigits == 3) mLoadingFilesComplete = true; */
			CI_LOG_V(mStatus);
		}
		//}
	}
	void TextureImageSequence::updateSequence() {
		int newPosition;

		if (mSequenceTextures.size() > 0) {
			// Call on each frame to update the playhead
			if (mPlaying) {
				//mSpeed = mVDAnimation->getFloatUniformValueByName("speed");
				playheadFrameInc += mSpeed;
				newPosition = mPosition + (int)playheadFrameInc;
				if (playheadFrameInc > 1.0f) playheadFrameInc = 0.0f;
				if (newPosition < 0) newPosition = mSequenceTextures.size() - 1;
				if (newPosition > mSequenceTextures.size() - 1) newPosition = 0;
			}
			else {
				if (mSyncToBeat) {
					//newPosition = (int)(((int)(mVDAnimation->iBar / mVDAnimation->iPhaseIndex)) % mSequenceTextures.size());
					// TODO newPosition = (int)(((int)(mVDSettings->iPhase / mVDAnimation->iPhaseIndex)) % mSequenceTextures.size());
					//newPosition = (int)(mVDAnimation->getIntUniformValueByName("iPhase") % mSequenceTextures.size());
					// TODO CHECK 20190803 newPosition = (int)(mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IBEAT) % mSequenceTextures.size());
				}
				else {
					newPosition = mPosition;
				}
			}
			mPosition = max(0, min(newPosition, (int)mSequenceTextures.size() - 1));
		}
	}

	ci::gl::Texture2dRef TextureImageSequence::getTexture() {

		if (mSequenceTextures.size() > 0) {

			if (mPosition > mFramesLoaded) {
				//error
				mPosition = 0;
			}
			//20191017 slowdown loading not to freeze ui
			if (!mLoadingFilesComplete) {
				// too slow if (currentSecond != (int)getElapsedSeconds()) { currentSecond = (int)getElapsedSeconds();
				loadNextImageFromDisk();
				//}
			}

			if (mPlaying) {
				updateSequence();
			}
			mTexture = mSequenceTextures[mPosition];
		}
		return mTexture;
	}
	ci::gl::Texture2dRef TextureImageSequence::getCachedTexture(const std::string& aFilename) {

		if (mCachedTextures[aFilename]) {
			CI_LOG_V(aFilename + " in cache");
			mLastCachedFilename = aFilename;
			mTexture = mCachedTextures[aFilename];
		}
		else {
			fs::path fullPath = getAssetPath("") / mPath / aFilename;
			if (fs::exists(fullPath)) {
				// start profiling
				auto start = Clock::now();
				mCachedTextures[aFilename] = ci::gl::Texture::create(loadImage(fullPath), gl::Texture::Format().loadTopDown());
				auto end = Clock::now();
				auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				int milli = msdur.count();
				mLastCachedFilename = aFilename;
				mTexture = mCachedTextures[aFilename];
				//mStatus = aFilename + " cached in ms " + toString(milli);
				//CI_LOG_V(mStatus);
			}
			else {
				// we want the last texture repeating
				//CI_LOG_V(aFilename + " does not exist");
				mTexture = mCachedTextures[mLastCachedFilename];
			}
		}
		return mTexture;
	}
	/*ci::gl::Texture2dRef TextureImageSequence::getNextTexture() {

		if (mSequenceTextures.size() > 1) {

			if (mPosition+1 > mFramesLoaded) {
				//error
				mPosition = 0;
			}
			mTexture = mSequenceTextures[mPosition+1];
		}
		return mTexture;
	}*/
	// Stops playback and resets the playhead to zero
	void TextureImageSequence::stopSequence() {

		mPlaying = false;
		mPosition = 0;
	}

	int TextureImageSequence::getMaxFrame() {

		return mFramesLoaded;
	}

	// Seek to a new position in the sequence
	void TextureImageSequence::setPlayheadPosition(int position) {

		mPosition = max(0, min(position, (int)mSequenceTextures.size() - 1));
		if (!mLoadingFilesComplete) {
			loadNextImageFromDisk();
		}
	}

	bool TextureImageSequence::isLoadingFromDisk() {
		return !mLoadingFilesComplete;
	}

	void TextureImageSequence::toggleLoadingFromDisk() {
		mLoadingPaused = !mLoadingPaused;
	}
	TextureImageSequence::~TextureImageSequence(void) {
	}

	/*
	** ---- TextureCamera ------------------------------------------------
	*/
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
	TextureCamera::TextureCamera() {
		mType = CAMERA;
		mFirstCameraDeviceName = "";
		printDevices();

		try {
			mCapture = Capture::create(1280, 720);
			mCapture->start();
		}
		catch (ci::Exception &exc) {
			CI_LOG_EXCEPTION("Failed to init capture ", exc);
		}
	}
	bool TextureCamera::fromJson(const JsonTree& json) {
		// init		
		mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown());
		// retrieve attributes specific to this type of texture
		mPath = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "cam";
		mName = "camera";
		return true;
	}
	
	ci::gl::Texture2dRef TextureCamera::getTexture() {
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
	ci::gl::Texture2dRef TextureCamera::getCachedTexture(const std::string& aFilename) {
		return TextureCamera::getTexture();
	}
	void TextureCamera::printDevices() {
		for (const auto &device : Capture::getDevices()) {
			console() << "Device: " << device->getName() << " "
#if defined( CINDER_COCOA_TOUCH )
				<< (device->isFrontFacing() ? "Front" : "Rear") << "-facing"
#endif
				<< endl;
			mFirstCameraDeviceName = device->getName();
		}
	}
	TextureCamera::~TextureCamera(void) {

	}
#endif

	/*
	** ---- TextureShared ------------------------------------------------
	*/
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
	TextureShared::TextureShared() {
		mType = SHARED;
#if defined( CINDER_MAC )
		mClientSyphon.setup();
		mClientSyphon.setServerName("Reymenta client");
		mClientSyphon.bind();
#endif
	}
	bool TextureShared::fromJson(const JsonTree& json)
	{
		// init		
		mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown());
		// retrieve attributes specific to this type of texture
		mPath = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "shared";
		mName = "shared";
		return true;
	}
	

	ci::gl::Texture2dRef TextureShared::getTexture() {
#if defined( CINDER_MSW )

		mTexture = mSpoutIn.receiveTexture();
		// set name for UI
		mName = mSpoutIn.getSenderName();

#endif
#if defined( CINDER_MAC )
		mClientSyphon.draw(vec2(0.f, 0.f));
#endif
		return mTexture;
	}
	ci::gl::Texture2dRef TextureShared::getCachedTexture(const std::string& aFilename) {
		return TextureShared::getTexture();
	}
	TextureShared::~TextureShared(void) {
#if defined( CINDER_MSW )
		mSpoutIn.getSpoutReceiver().ReleaseReceiver();
#endif

	}
#endif
	/*
	** ---- TextureAudio ------------------------------------------------
	*/
	TextureAudio::TextureAudio(VDAnimationRef aVDAnimation) {
		mVDAnimation = aVDAnimation;
		mType = AUDIO;
		mLineInInitialized = false;
		mWaveInitialized = false;
		mName = "audio";

		auto fmt = gl::Texture2d::Format().swizzleMask(GL_RED, GL_RED, GL_RED, GL_ONE).internalFormat(GL_RED);
		for (int i = 0; i < 128; ++i) dTexture[i] = (unsigned char)(Rand::randUint() & 0xFF);
		mTexture = gl::Texture::create(dTexture, GL_RED, 64, 2, fmt);
	}
	

	bool TextureAudio::fromJson(const JsonTree& json)
	{
		VDTexture::fromJson(json);
		// retrieve attributes specific to this type of texture
		// prevent linein not present crash mVDAnimation->setUseLineIn(xml.getAttributeValue<bool>("uselinein", "true"));
		mName = (mVDAnimation->getUseLineIn()) ? "line in" : "wave";
		auto fmt = gl::Texture2d::Format().swizzleMask(GL_RED, GL_RED, GL_RED, GL_ONE).internalFormat(GL_RED);
		for (int i = 0; i < 128; ++i) dTexture[i] = (unsigned char)(Rand::randUint() & 0xFF);
		mTexture = gl::Texture::create(dTexture, GL_RED, 64, 2, fmt);
		return true;
	}
	bool TextureAudio::loadFromFullPath(const std::string& aPath)
	{
		CI_LOG_V("TextureAudio::loadFromFullPath: " + aPath);
		try {
			if (fs::exists(aPath)) {
				mName = aPath;

				auto ctx = audio::Context::master(); // was audio::master(); !?!
				mSourceFile = audio::load(loadFile(aPath), audio::master()->getSampleRate());
				if (mVDAnimation->isAudioBuffered()) {
					mBufferPlayerNode = ctx->makeNode(new audio::BufferPlayerNode());
					mBufferPlayerNode->loadBuffer(mSourceFile);
					//mWaveformPlot.load(mBufferPlayerNode->getBuffer(), mFbo->getBounds());
					mBufferPlayerNode->start();
					mBufferPlayerNode >> mMonitorWaveSpectralNode >> ctx->getOutput();
					ctx->enable();
				}
				else {
					mSamplePlayerNode = ctx->makeNode(new audio::FilePlayerNode(mSourceFile, false));
					mSamplePlayerNode->setLoopEnabled(false);
					mSamplePlayerNode >> mMonitorWaveSpectralNode >> ctx->getOutput();
					mSamplePlayerNode->enable();

					mSamplePlayerNode->seek(0);
					auto filePlayer = dynamic_pointer_cast<audio::FilePlayerNode>(mSamplePlayerNode);
					CI_ASSERT_MSG(filePlayer, "expected sample player to be either BufferPlayerNode or FilePlayerNode");
					// in case another wave is playing

					filePlayer->setSourceFile(mSourceFile);

					mSamplePlayerNode->start();

				}

				mVDAnimation->setUseLineIn(false);
			}
		}
		catch (...) {
			CI_LOG_W("could not open wavefile");
		}
		return true;
	}

	ci::gl::Texture2dRef TextureAudio::getTexture() {
		return mVDAnimation->getAudioTexture();
		/*auto fmt = gl::Texture2d::Format().swizzleMask(GL_RED, GL_RED, GL_RED, GL_ONE).internalFormat(GL_RED);
		auto ctx = audio::Context::master();
		if (!mLineInInitialized) {
#if (defined( CINDER_MSW ) || defined( CINDER_MAC ))
			if (mVDAnimation->getUseLineIn()) {
				// linein
				mVDAnimation->preventLineInCrash(); // at next launch
				CI_LOG_W("trying to open mic/line in, if no line follows in the log, the app crashed so put UseLineIn to false in the VDSettings.xml file");
				mLineIn = ctx->createInputDeviceNode(); //crashes if linein is present but disabled, doesn't go to catch block
				CI_LOG_V("mic/line in opened");
				mVDAnimation->saveLineIn();
				mName = mLineIn->getDevice()->getName();
				auto scopeLineInFmt = audio::MonitorSpectralNode::Format().fftSize(mVDAnimation->mFFTWindowSize * 2).windowSize(mVDAnimation->mFFTWindowSize);// CHECK is * 2 needed
				mMonitorLineInSpectralNode = ctx->makeNode(new audio::MonitorSpectralNode(scopeLineInFmt));
				mLineIn >> mMonitorLineInSpectralNode;
				mLineIn->enable();
				mLineInInitialized = true;
			}
		}
#endif
		if (!mWaveInitialized) {
			if (mVDAnimation->getUseAudio()) {
				// also initialize wave monitor
				auto scopeWaveFmt = audio::MonitorSpectralNode::Format().fftSize(mVDAnimation->mFFTWindowSize * 2).windowSize(mVDAnimation->mFFTWindowSize);
				mMonitorWaveSpectralNode = ctx->makeNode(new audio::MonitorSpectralNode(scopeWaveFmt));

				ctx->enable();
				mWaveInitialized = true;
			}
		}

#if (defined( CINDER_MSW ) || defined( CINDER_MAC ))
		if (mVDAnimation->getUseLineIn()) {
			mMagSpectrum = mMonitorLineInSpectralNode->getMagSpectrum();
		}
		else {
#endif
			if (mVDAnimation->getUseAudio()) {
				if (mVDAnimation->isAudioBuffered()) {
					if (mBufferPlayerNode) {
						mMagSpectrum = mMonitorWaveSpectralNode->getMagSpectrum();
					}
				}
				else {
					if (mSamplePlayerNode) {
						mMagSpectrum = mMonitorWaveSpectralNode->getMagSpectrum();
						mPosition = mSamplePlayerNode->getReadPosition();
					}
				}
			}
#if (defined( CINDER_MSW ) || defined( CINDER_MAC ))
		}
#endif
		if (!mMagSpectrum.empty()) {

			
			mVDUniforms = VDUniforms::create();
			mVDUniforms->setUniformValue(mVDUniforms->IMAXVOLUME, 0.0f);
			size_t mDataSize = mMagSpectrum.size();
			if (mDataSize > 0 && mDataSize < mVDAnimation->mFFTWindowSize + 1) {
				float db;
				unsigned char signal[kBands];
				for (size_t i = 0; i < mDataSize; i++) {
					float f = mMagSpectrum[i];
					db = audio::linearToDecibel(f);
					f = db * mVDUniforms->getUniformValue(mVDUniforms->IAUDIOX);
					if (f > mVDUniforms->getUniformValue(mVDUniforms->IMAXVOLUME))
					{
						mVDUniforms->setUniformValue(mVDUniforms->IMAXVOLUME, f);
					}
					mVDAnimation->iFreqs[i] = f;
					// update iFreq uniforms 
					if (i == mVDAnimation->getFreqIndex(0)) mVDUniforms->setUniformValue(mVDUniforms->IFREQ0, f);
					if (i == mVDAnimation->getFreqIndex(1)) mVDUniforms->setUniformValue(mVDUniforms->IFREQ1, f);
					if (i == mVDAnimation->getFreqIndex(2)) mVDUniforms->setUniformValue(mVDUniforms->IFREQ2, f);
					if (i == mVDAnimation->getFreqIndex(3)) mVDUniforms->setUniformValue(mVDUniforms->IFREQ3, f);

					if (i < 16) {
						int ger = f;
						signal[i] = static_cast<unsigned char>(ger);
					}
				}
				// store it as a 512x2 texture
				mTexture = gl::Texture::create(signal, GL_RED, 64, 2, fmt);
				if (mVDAnimation->isAudioBuffered() && mBufferPlayerNode) {
					gl::ScopedFramebuffer fbScp(mFbo);
					gl::clear(Color::black());

					mTexture->bind(0);

					//mWaveformPlot.draw();
					// draw the current play position
					mPosition = mBufferPlayerNode->getReadPosition();
					float readPos = (float)mWidth * mPosition / mBufferPlayerNode->getNumFrames();
					gl::color(ColorA(0, 1, 0, 0.7f));
					gl::drawSolidRect(Rectf(readPos - 2, 0, readPos + 2, (float)mHeight));
					mRenderedTexture = mFbo->getColorTexture();
					return mRenderedTexture;
				}
			}
		}
		else {
			// generate random values
			for (int i = 0; i < 128; ++i) dTexture[i] = (unsigned char)(i);
			mTexture = gl::Texture::create(dTexture, GL_RED, 64, 2, fmt);
		}

		return mTexture;*/
	}
	ci::gl::Texture2dRef TextureAudio::getCachedTexture(const std::string& aFilename) {
		return TextureAudio::getTexture();
	}
	TextureAudio::~TextureAudio(void) {
	}
	/*
	** ---- TextureStream ------------------------------------------------
	*/
	TextureStream::TextureStream(VDAnimationRef aVDAnimation) {
		mVDAnimation = aVDAnimation;
		mType = STREAM;
		mName = "stream";
		mTexture = gl::Texture::create(mWidth, mHeight);
	}
	

	bool TextureStream::fromJson(const JsonTree& json)
	{
		VDTexture::fromJson(json);
		// retrieve attributes specific to this type of texture
		mTexture = gl::Texture::create(mWidth, mHeight);
		return true;
	}
	bool TextureStream::loadFromFullPath(const std::string& aStream)
	{
		bool rtn = false;
		size_t len;
		size_t comma = aStream.find(",");
		if (comma == string::npos) {
			CI_LOG_W("comma not found");
		}
		else {
			len = aStream.size() - comma - 1;
			auto buf = make_shared<Buffer>(fromBase64(&aStream[comma + 1], len));
			try {
				shared_ptr<Surface8u> result(new Surface8u(ci::loadImage(DataSourceBuffer::create(buf), ImageSource::Options(), "jpeg")));
				mTexture = gl::Texture2d::create(*result, ci::gl::Texture::Format());
				rtn = true;
			}
			catch (std::exception &exc) {
				CI_LOG_W("failed to parse streamed data image, what: " << exc.what());
			}
		}
		return rtn;
	}

	ci::gl::Texture2dRef TextureStream::getTexture() {
		return mTexture;
	}
	ci::gl::Texture2dRef TextureStream::getCachedTexture(const std::string& aFilename) {
		return TextureStream::getTexture();
	}
	TextureStream::~TextureStream(void) {
	}
} // namespace videodromm


/*#include "VDTexture.h"

#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"


using namespace videodromm;

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

*/


