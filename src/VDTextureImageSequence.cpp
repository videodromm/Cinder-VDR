/*
	** TextureImageSequence
	*/
#include "VDTextureImageSequence.h"
using namespace videodromm;

TextureImageSequence::TextureImageSequence(VDAnimationRef aVDAnimation) {
	// constructor
	mVDAnimation = aVDAnimation;
	mType = TextureType::SEQUENCE;
	playheadFrameInc = 0.0f;
	mLoadingFilesComplete = true;
	mLoadingPaused = false;
	mFramesLoaded = 0;
	mCurrentLoadedFrame = 0;
	mNextIndexFrameToTry = 0;
	mPlaying = false;
	mSpeed = 0.01f;
	mExt = "png";
	mPrefix = "none";
	mNextIndexFrameToTry = 0;
	mPosition = 0;
	mNumberOfDigits = 4;
}
bool TextureImageSequence::loadFromFullPath(std::string aPath)
{
	bool validFile = false; // if no valid files in the folder, we keep existing vector
	std::string anyImagefileName = "0.jpg";
	std::string folder = "";
	std::string fileName;
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
		catch (Exception& exc) {
			CI_LOG_EXCEPTION("error loading ", exc);
		}

		// init: if no valid file found we take the default 0.jpg
		if (!validFile) {
			// might want to remove default file as we are now using a boolean to notify the caller
			if (anyImagefileName.length() > 0) {
				mTexture = ci::gl::Texture::create(loadImage(loadAsset(anyImagefileName)), ci::gl::Texture::Format().loadTopDown(mFlipV));
				mSequenceTextures.push_back(ci::gl::Texture::create(loadImage(loadAsset(anyImagefileName)), gl::Texture::Format().loadTopDown(mFlipV)));
				mLoadingFilesComplete = true;
				mFramesLoaded = 1;
			}
		}
	}
	return validFile;
}
/*bool TextureImageSequence::fromXml(const XmlTree& xml)
{
	bool rtn = false;
	// init		
	mTexture = ci::gl::Texture::create(mWidth, mHeight, ci::gl::Texture::Format().loadTopDown(mFlipV));
	// retrieve attributes specific to this type of texture
	mPath = xml.getAttributeValue<string>("path", "");
	mName = mPath;
	mFlipV = xml.getAttributeValue<bool>("flipv", "false");
	mFlipH = xml.getAttributeValue<bool>("fliph", "false");
	if (mPath.length() > 0) {
		if (fs::exists(mPath)) {
			rtn = loadFromFullPath(mPath);
		}
		else {
			fs::path fullPath = getAssetPath("") / mPath;// TODO / mVDSettings->mAssetsPath
			rtn = loadFromFullPath(fullPath.string());
		}
	}
	return rtn;
}
XmlTree	TextureImageSequence::toXml() const {
	XmlTree xml = VDTexture::toXml();

	// add attributes specific to this type of texture
	xml.setAttribute("path", mPath);
	xml.setAttribute("flipv", mFlipV);
	xml.setAttribute("fliph", mFlipH);
	return xml;
}*/
void TextureImageSequence::loadNextImageFromDisk() {
	if (!mLoadingPaused) {

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

			std::string fileNameToLoad = mPrefix + restOfFileName + "." + mExt;
			fs::path fileToLoad = getAssetPath("") / mPath / fileNameToLoad;
			if (fs::exists(fileToLoad)) {
				// start profiling
				auto start = Clock::now();

				mSequenceTextures.push_back(ci::gl::Texture::create(loadImage(fileToLoad), gl::Texture::Format().loadTopDown()));
				mCurrentLoadedFrame = mFramesLoaded;
				mFramesLoaded++;
				auto end = Clock::now();
				auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				int milli = msdur.count();

				mStatus = fileToLoad.string() + " loaded in ms " + toString(milli);
			}
			else {

				mStatus = fileToLoad.string() + " does not exist";
				if (mFramesLoaded > 0) {
					// if frames have been loaded we hit the last file of the image sequence at this point
					mStatus = "last image loaded";
					mLoadingFilesComplete = true;
				}
			}
			CI_LOG_V(mStatus);
			// increment counter for next filename
			mNextIndexFrameToTry++;
			if (mNextIndexFrameToTry > 9999 && mNumberOfDigits == 4) mLoadingFilesComplete = true;
			if (mNextIndexFrameToTry > 999 && mNumberOfDigits == 3) mLoadingFilesComplete = true;
		}
	}
}
void TextureImageSequence::updateSequence() {
	int newPosition;

	if (mSequenceTextures.size() > 0) {
		// Call on each frame to update the playhead
		if (mPlaying) {
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
				newPosition = (int)(mVDAnimation->getIntUniformValueByName("iPhase") % mSequenceTextures.size());
			}
			else {
				newPosition = mPosition;
			}
		}
		mPosition = math<int>::max(0, math<int>::min(newPosition, (int)mSequenceTextures.size() - 1));
	}
}

ci::gl::Texture2dRef TextureImageSequence::getTexture(unsigned int aIndex) {

	if (aIndex > 0) mPosition = aIndex;
	if (mSequenceTextures.size() > 0) {

		if (mPosition > mFramesLoaded) {
			//error
			mPosition = 0;
		}
		if (!mLoadingFilesComplete) loadNextImageFromDisk();

		if (mPlaying) {
			updateSequence();
		}
		mTexture = mSequenceTextures[mPosition];
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

	mPosition = math<int>::max(0, math<int>::min(position, (int)mSequenceTextures.size() - 1));
	if (!mLoadingFilesComplete) {
		loadNextImageFromDisk();
	}
}
void TextureImageSequence::reverse() {
	mSpeed *= -1.0f;
}
float TextureImageSequence::getSpeed() {
	return mSpeed;
}
void TextureImageSequence::setSpeed(float speed) {
	mSpeed = speed;
}
bool TextureImageSequence::isLoadingFromDisk() {
	return !mLoadingFilesComplete;
}

void TextureImageSequence::toggleLoadingFromDisk() {
	mLoadingPaused = !mLoadingPaused;
}
TextureImageSequence::~TextureImageSequence(void) {
}