#include "VDFboShader.h"

using namespace videodromm;
/* hydra
uniform float time;
uniform vec2 resolution;
varying vec2 uv;
uniform sampler2D prevBuffer;
*/

VDFboShader::VDFboShader(VDUniformsRef aVDUniforms, VDAnimationRef aVDAnimation, const JsonTree &json, unsigned int aFboIndex, const std::string& aAssetsPath)
	:mVDUniforms{ aVDUniforms },
	mVDAnimation{ aVDAnimation }
{
	CI_LOG_V("VDFboShader constructor");
	// Params
	mVDParams = VDParams::create();
	mAssetsPath = aAssetsPath;
	mFboIndex = aFboIndex;
	std::string shaderType = "fs";
	// hydra
	mIsHydraTex = false;
	/* 20211227 check is hydra needs init:
	for (size_t i{ 0 }; i < 4; i++)
	{
		mInputTextureList[i] = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mLoadTopDown));
	}*/
	// load default fragment shader
	mShaderName = mShaderFileName = "inputImage.fs";
	mShaderFragmentString = mVDParams->getDefaultShaderFragmentString();
	shaderInclude = loadString(loadAsset("shadertoy.vd"));

	//mInputTextureIndex = 0;
	// 20211107 only if no texture ?
	setFboTextureAudioMode();
	CI_LOG_V(json.serialize());
	if (json.hasChild("shader")) {
		JsonTree shaderJsonTree(json.getChild("shader"));
		mShaderName = mShaderFileName = (shaderJsonTree.hasChild("shadername")) ? shaderJsonTree.getValueForKey<string>("shadername") : "inputImage.fs";
		mShaderFragmentString = (shaderJsonTree.hasChild("shadertext")) ? shaderJsonTree.getValueForKey<string>("shadertext") : "";
		shaderType = (json.hasChild("shadertype")) ? json.getValueForKey<string>("shadertype") : "fs";
	}
	if (json.hasChild("texture")) {

		JsonTree textureJsonTree(json.getChild("texture"));
		createInputTexture(textureJsonTree);
	}

	// init texture
	mRenderedTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mLoadTopDown));
	isReady = false;

	// init texture
	// init the fbo whatever happens next
	fboFmt.setColorTextureFormat(fmt);
	mFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
	mFboMsg = "";
	mFboError = "";
	if (mShaderFragmentString.length() > 0) {
		mValid = setFragmentShaderString(mShaderFragmentString, mShaderName);
	}
	else {
		mValid = loadFragmentShaderFromFile(mShaderName);
	}

	if (mValid) {
		CI_LOG_V("VDFbo constructor success");
	}
	else {
		mFboError = "VDFbo constructor failed";
	}
}
VDFboShader::~VDFboShader(void) {
}
unsigned int VDFboShader::createInputTexture(const JsonTree &json) {
	unsigned int rtn = 0;
	//unsigned int listIndex = 0;
	mCurrentFilename = mTextureName = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "0.jpg";
	mTypestr = (json.hasChild("texturetype")) ? json.getValueForKey<string>("texturetype") : "UNKNOWN";
	mTextureMode = (json.hasChild("texturemode")) ? json.getValueForKey<int>("texturemode") : VDTextureMode::UNKNOWN;
	mTextureCount = (json.hasChild("texturecount")) ? json.getValueForKey<int>("texturecount") : 1;
	mPreloadTextures = (json.hasChild("preloadtextures")) ? json.getValueForKey<bool>("preloadtextures") : false;
	msTotal = 0;
	CI_LOG_V("createInputTexture: mCurrentFilename " + toString(mTextureCount) + " mTextureCount: " + mCurrentFilename + " mPreloadTextures: " + toString(mPreloadTextures));

	switch (mTextureMode)
	{
	case VDTextureMode::AUDIO: // audio
		setFboTextureAudioMode();
		break;
	case VDTextureMode::SEQUENCE: // img seq loaded when ableton runs
		// init with number 1 then getFboTexture will load next images
		mInputTextureList[0].isValid = false; // remove audio texture
		loadNextTexture(1);
		break;
	case VDTextureMode::TEXT: // text
		mInputTextureList[0].isValid = false; // remove audio texture
		
		break;
	case VDTextureMode::PARTS: // img parts
		mInputTextureList[0].isValid = false; // remove audio texture
		for (size_t i{ 0 }; i < mTextureCount; i++)
		{
			loadNextTexture(i);
		}
		break;
	case VDTextureMode::NDI: // video streamed over the network via NDI (Windows only)
		mInputTextureList[0].isValid = false; // remove audio texture
#if defined( CINDER_MSW )
		{
			// "texturename" (if present) is the preferred NDI sender name to connect to; absent
			// means connect to the first source found (see CinderNDIReceiver::setup()'s own
			// behavior for an empty name) - re-checked directly against json rather than
			// mTextureName, since that member already defaulted to the generic "0.jpg" sentinel
			// above when no texturename was given, which isn't a real NDI sender name
			std::string preferredSender = json.hasChild("texturename") ? json.getValueForKey<string>("texturename") : "";
			mNdiReceiver.setup(preferredSender);
			mCurrentFilename = mTextureName = "ndi in";
		}
#else
		// TODO: NDI is Windows-only (Cinder-NDI's vendored SDK ships no Mac binaries)
		mFboError = "NDI input not available on this platform";
#endif
		break;
	case VDTextureMode::SHARED: // shared
		#if defined( CINDER_MSW )
		mInputTextureList[0].texture = mSpoutIn.receiveTexture();
		// set name for UI
		mInputTextureList[0].name = mSpoutIn.getSenderName();
		mCurrentFilename = mTextureName = "spout in";// mSpoutIn.getSenderName();
		mInputTextureList[0].ms = 0;
		mInputTextureList[0].isValid = true;
		#elif defined( CINDER_MAC )
		if (!mSyphonInitialized) {
			mClientSyphon.setup();
			// texturename (if any, from the fbo*.json "texturename" field) selects which Syphon
			// server to bind to; empty/"audio" (the default when not specified) means "any"
			if (!mTextureName.empty() && mTextureName != "audio") {
				mClientSyphon.setServerName(mTextureName);
			}
			try {
				mGlslVideoTexture = gl::GlslProg::create(gl::GlslProg::Format()
					.vertex(loadAsset("video_texture.vs.glsl"))
					.fragment(loadAsset("video_texture.fs.glsl")));
			}
			catch (const std::exception& ex) {
				CI_LOG_E("<< Syphon blit GlslProg error >> " << ex.what());
			}
			gl::Fbo::Format blitFmt;
			mSyphonBlitFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), blitFmt);
			mSyphonInitialized = true;
		}
		mCurrentFilename = mTextureName = "syphon in";
		mInputTextureList[0].ms = 0;
		mInputTextureList[0].isValid = true;
		#else
		// Spout is Windows-only; fallback to audio texture on non-Windows builds.
		setFboTextureAudioMode();
		#endif
		break;

	default:
		colonIndex = mTextureName.find(":"); 
		if (colonIndex != std::string::npos) {
			// full path
			texFileOrPath = mTextureName;
		}
		else {
			// relative path 
			texFileOrPath = getAssetPath("") / mAssetsPath / mTextureName;
		}
		mExt = "";
		dotIndex = texFileOrPath.filename().string().find_last_of(".");
		if (dotIndex != std::string::npos)  mExt = texFileOrPath.filename().string().substr(dotIndex + 1);
		// image
		if (mExt == "jpg" || mExt == "png") {
			mFboMsg = "jpg or png";
			mTextureMode = VDTextureMode::IMAGE;
			mTypestr = "image";
			loadImageFile(texFileOrPath.string(), 0);
		}
		else {
			// video
			if (mExt == "mp4") {
				bool fileExists = fs::exists(texFileOrPath);
				if (!fileExists) {
					mFboError = texFileOrPath.string() + " video does not exist, trying with parent folder";
					CI_LOG_V(mFboError);
					/*texFileOrPath = getAssetPath("") / mTextureName;
					fileExists = fs::exists(texFileOrPath);
					if (!fileExists) {
						mFboError = texFileOrPath.string() + " video does not exist in parent folder";
						CI_LOG_V(mFboError);
					}*/
				}
				if (fileExists) {
					loadVideoFile(texFileOrPath.string());
					mTypestr = "video";
				}
				else {
					// default to audio
					mFboMsg = "audio, video does not exist: " + mTextureName;
					setFboTextureAudioMode();
				}
			}
			else {
				// default to audio
				mFboMsg = "audio, not image or video: " + mTextureName;
				setFboTextureAudioMode();
			}
		}
		break;
	}
	int slashIndex = mTextureName.find_last_of("\\");
	(slashIndex != std::string::npos) ? mFboStatus = mTextureName.substr(slashIndex + 1) : mFboStatus = mTextureName;
	rtn = mInputTextureList.size() - 1;
	return rtn;

}
bool VDFboShader::loadFragmentShaderFromFile(const string& aFileOrPath, bool isAudio) {
	mValid = false;
	bool fileExists = true;
	if (aFileOrPath.length() > 0) {
		if (fs::exists(aFileOrPath)) {
			// it's a full path
			mFragFilePath = aFileOrPath;
		}
		else {
			// try in assets folder			
			if (!fs::exists(mFragFilePath)) {
				mFragFilePath = getAssetPath("") / mAssetsPath / aFileOrPath;
				if (!fs::exists(mFragFilePath)) {
					mFragFilePath = getAssetPath("") / aFileOrPath;
					if (!fs::exists(mFragFilePath)) {
						fileExists = false;
						mFboError = "VDFboShader file does not exist in assets root or current subfolder:" + aFileOrPath;
					}
				}
			}
		}
	}
	else {
		mFboError = "VDFboShader file empty";
	}
	if (fileExists) {
		// file exists
		mValid = loadFragmentStringFromFile();
	}
	if (isAudio && mValid) {
		setFboTextureAudioMode();
	}
	return mValid;
}
// private
bool VDFboShader::loadFragmentStringFromFile() {
	mValid = false;
	// load fragment shader
	mFileNameWithExtension = mFragFilePath.filename().string();
	CI_LOG_V("loadFragmentStringFromFile, loading " << mFileNameWithExtension);
	mValid = setFragmentShaderString(loadString(loadFile(mFragFilePath)), mFileNameWithExtension);
	CI_LOG_V(mFragFilePath.string() << " loaded and compiled");
	return mValid;
}
bool VDFboShader::setFragmentShaderString(const std::string& aFragmentShaderString, const std::string& aName) {
	std::string mOriginalFragmentString = aFragmentShaderString;
	std::string mOutputFragmentString = aFragmentShaderString;
	mFboError = "";
	mName = aName;
	mIsHydraTex = false;
	isReady = false;
	// we would like a name without extension
	if (mName.length() == 0) {
		mName = toString((int)getElapsedSeconds());
	}
	else {
		int dotIndex = mName.find_last_of(".");
		int slashIndex = mName.find_last_of("\\");

		if (dotIndex != std::string::npos && dotIndex > slashIndex) {
			mName = mName.substr(slashIndex + 1, dotIndex - slashIndex - 1);
		}
	}
	mShaderName = mName + ".fs";

	std::string mNotFoundUniformsString = "/* " + mName + "\n";

	// load fragment shader
	CI_LOG_V("setFragmentShaderString, loading " << mName);
	try
	{
		std::size_t foundUniform = mOriginalFragmentString.find("uniform ");
		if (foundUniform == std::string::npos) {
			CI_LOG_V("setFragmentShaderString, no uniforms found, we add from shadertoy.vd");
			mOutputFragmentString = "/* " + mName + " */\n" + shaderInclude + mOriginalFragmentString;
		}
		else {
			mOutputFragmentString = "/* " + mName + " */\n" + mOriginalFragmentString;
		}

		// try to compile a first time to get active mUniforms
		mShader = gl::GlslProg::create(mVDParams->getDefaultVertexString(), mOutputFragmentString);
		// update only if success
		mShaderFragmentString = mOutputFragmentString;
		mFboMsg = mName;
		mFboStatus = "";
		mValid = true;
	}
	catch (gl::GlslProgCompileExc& exc)
	{
		mFboStatus = mName + std::string(exc.what());
		CI_LOG_V("setFragmentShaderString, unable to compile fragment shader -95:" << mFboError << " frag: " << mName << " status " << mFboStatus);
	}
	catch (const std::exception& e)
	{
		mFboStatus = mName + std::string(e.what());
		CI_LOG_V("setFragmentShaderString, error on live fragment shader:" << mFboError << " frag:" << mName);
	}
	return mValid;
}
void VDFboShader::loadImageFile(const std::string& aFile, unsigned int aCurrentIndex) {
	if (!mInputTextureList[aCurrentIndex].isValid || aCurrentIndex == 0) {
		fs::path texFileOrPath = aFile;
		bool fileExists = fs::exists(texFileOrPath);
		if (fileExists) {
			// start profiling
			auto start = Clock::now();
			int slashIndex = aFile.find_last_of("\\");
			if (slashIndex != std::string::npos) {
				mCurrentFilename = aFile.substr(slashIndex + 1);
			}
			else {
				mCurrentFilename = aFile;
			}
			try
			{
				mInputTextureList[aCurrentIndex].texture = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
			}
			catch (const std::exception& ex)
			{
				CI_LOG_E("<< loadImageFile >> " << mCurrentFilename << " error: " << ex.what());
			}
			mInputTextureList[aCurrentIndex].name = mCurrentFilename;
			mInputTextureList[aCurrentIndex].isValid = true;
			auto end = Clock::now();
			auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			mInputTextureList[aCurrentIndex].ms = msdur.count();
			msTotal += mInputTextureList[aCurrentIndex].ms;
			//mInputTextureListIndexes[mCurrentImageSequenceIndex] = aTexIndex;// = listIndex
			mFboMsg = mCurrentFilename = mCurrentFilename + " " + toString(mInputTextureList[aCurrentIndex].ms) + "ms";
		}
		else {
			// default to audio
			mFboMsg = "audio, jpg or png does not exist: " + mTextureName;
			setFboTextureAudioMode();
		}
	}
	else {
		// in cache
		mFboMsg = mInputTextureList[aCurrentIndex].name + " cached";
	}
}
bool VDFboShader::loadVideoFile(const std::string& aFile) {
#if defined( CINDER_MSW )
	// reuses the preferred audio output device (see VDAnimation's audio device selection) so the
	// movie's audio lands on the device the user picked. Safe to call again on an already-playing
	// mVideo - ciWMFVideoPlayer::loadMovie() just re-opens the player via OpenURL(), no close() needed.
	mIsVideoLoaded = mVideo.loadMovie(aFile, mVDAnimation->getPreferredAudioOutputDevice());
	// diagnostic: ciWMFVideoPlayer::loadMovie() returns true whenever mPlayer exists, regardless
	// of whether OpenURL() actually succeeded internally or whether a usable video texture will
	// ever appear - this is the one piece of ground truth available without a debugger attached,
	// to tell "loaded but never got a texture" apart from "never actually loaded"
	CI_LOG_I("loadVideoFile " << aFile << " loadMovie()=" << mIsVideoLoaded
		<< " width=" << mVideo.getWidth() << " height=" << mVideo.getHeight()
		<< " hasTexture()=" << mVideo.hasTexture());
	mVideoTextureWarningLogged = false;
	if (mIsVideoLoaded) {
		mVideo.setLoop(true);
		mVideo.play();
		// ciWMFVideoPlayer's shared texture is GL_TEXTURE_RECTANGLE (see the comment on
		// mVideoBlitFbo) - lazily set up the same rect-to-2D blit already used for Syphon on Mac
		if (!mGlslVideoTexture) {
			try {
				mGlslVideoTexture = gl::GlslProg::create(gl::GlslProg::Format()
					.vertex(loadAsset("video_texture.vs.glsl"))
					.fragment(loadAsset("video_texture.fs.glsl")));
			}
			catch (const std::exception& ex) {
				CI_LOG_E("<< video blit GlslProg error >> " << ex.what());
			}
		}
		if (!mVideoBlitFbo) {
			gl::Fbo::Format blitFmt;
			mVideoBlitFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), blitFmt);
		}
	}
	else {
		mFboError = "failed to load movie: " + aFile;
		CI_LOG_E(mFboError);
	}
#else
	// TODO: Mac movie playback not implemented yet (ciWMFVideoPlayer is Windows-only)
	mIsVideoLoaded = false;
	mFboError = "movie playback not available on this platform: " + aFile;
#endif
	// basename only, matching loadImageFile()'s convention - also what registerFboActiveTextureInGlobalPool()
	// reads via getTextureName(0), so the shared pool/Textures-panel title reflects the video, not
	// whatever this slot's name was left at before (e.g. a previous image on the same fbo)
	int slashIndex = aFile.find_last_of("\\");
	mCurrentFilename = mTextureName = (slashIndex != std::string::npos) ? aFile.substr(slashIndex + 1) : aFile;
	mInputTextureList[0].name = mCurrentFilename;
	mTextureMode = VDTextureMode::MOVIE;
	return mIsVideoLoaded;
}
// next in sequence
void VDFboShader::loadNextTexture(unsigned int aCurrentIndex) {
	if (mCurrentImageSequenceIndex != aCurrentIndex) {
		mCurrentImageSequenceIndex = aCurrentIndex;
		// try with jpg (space) explorer
		mCurrentFilename = mTextureName + " (" + toString(mCurrentImageSequenceIndex) + ").jpg";
		fs::path texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;
		fileExists = fs::exists(texFileOrPath);

		if (!fileExists) {
			// try with jpg (-) photoshop
			mCurrentFilename = mTextureName + "-(" + toString(mCurrentImageSequenceIndex) + ").jpg";
			texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;
			fileExists = fs::exists(texFileOrPath);
		}
		if (!fileExists) {
			// try with png (space) explorer
			mCurrentFilename = mTextureName + " (" + toString(mCurrentImageSequenceIndex) + ").png";
			texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;
			fileExists = fs::exists(texFileOrPath);
		}
		if (!fileExists) {
			// try with png (-) photoshop
			mCurrentFilename = mTextureName + "-(" + toString(mCurrentImageSequenceIndex) + ").png";
			texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;
			fileExists = fs::exists(texFileOrPath);
		}
		if (fileExists) {
			loadImageFile(texFileOrPath.string(), mCurrentImageSequenceIndex);
		}
	}
	
}
ci::gl::Texture2dRef VDFboShader::getFboTexture() {
	std::string uniformName;
	if (mValid) {
		switch (mTextureMode)
		{
		case VDTextureMode::TEXT: 
			mFboMsg = "text";
			mFont = Font("Gill Sans Ultra Bold", mVDUniforms->getUniformValue(mVDUniforms->ISMOOTH) * 256.0f);
			mSize = vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight());
			//gl::ScopedViewport scopedViewport(getWindowSize());
			//mTextbox = TextBox().alignment(TextBox::CENTER).font(mFont).size(ivec2(mSize.x, TextBox::GROW)).text(mTextureName);
			mTextbox = TextBox().alignment(TextBox::CENTER).font(mFont).size(mSize).text(mTextureName);
			mTextbox.setColor(Color(1.0f, 1.0f, 1.0f));
			mTextbox.setBackgroundColor(ColorA(0.0f, 0.0f, 0.0f, mVDUniforms->getUniformValue(mVDUniforms->IALPHA)));
			//ivec2 sz = mTextbox.measure();
			mTextTexture = gl::Texture2d::create(mTextbox.render());
			break; 
		case VDTextureMode::AUDIO: // audio
			setFboTextureAudioMode();
			break;
		case VDTextureMode::SEQUENCE:
			if (mSequenceManualControl) {
				// independent playback, driven by the UI's play/pause/speed/reverse/scrub
				// controls instead of the shared IBARBEAT uniform
				double now = ci::app::getElapsedSeconds();
				double dt = now - mLastSequenceUpdateTime;
				mLastSequenceUpdateTime = now;
				if (mSequencePlaying && mTextureCount > 0 && dt > 0.0 && dt < 1.0) {
					const float kBaseFps = 15.0f; // speed == 1.0 maps to this frame rate
					mSequenceAccumulator += (float)dt * kBaseFps * mSequenceSpeed * (mSequenceReversed ? -1.0f : 1.0f);
					while (mSequenceAccumulator >= 1.0f) {
						mSequenceAccumulator -= 1.0f;
						loadNextTexture((mCurrentImageSequenceIndex + 1) % mTextureCount);
					}
					while (mSequenceAccumulator <= -1.0f) {
						mSequenceAccumulator += 1.0f;
						loadNextTexture((mCurrentImageSequenceIndex - 1 + mTextureCount) % mTextureCount);
					}
				}
				// manual control never runs the mPreloadTextures progress-tracking branch below
				// (that one only updates mFboStatus while !mSequenceManualControl) - without this,
				// mFboStatus stays frozen forever at whatever preload progress string it last had
				// before manual control was engaged (e.g. play/pause/speed/scrub touched), even
				// though the sequence keeps advancing correctly - "always shows 0/522 ... although
				// images are loaded" was this: the images were fine, only the status text was stale
				mFboStatus = toString(mCurrentImageSequenceIndex) + "/" + toString(mTextureCount);
			}
			else {
				// image at IBARBEAT must be loaded before bind()
				loadNextTexture((int)mVDUniforms->getUniformValue(mVDUniforms->IBARBEAT));
				if (mPreloadTextures) {
					//CI_LOG_E("IBARBEAT " << (unsigned int)mVDUniforms->getUniformValue(mVDUniforms->IBARBEAT) << " IBEAT " << (unsigned int)mVDUniforms->getUniformValue(mVDUniforms->IBEAT));
					if (mVDUniforms->getUniformValue(mVDUniforms->IBEAT) > 1) {
						// try to load next images
						if (mCacheImageIndex < mTextureCount) {
							mCacheImageIndex++;
							mFboStatus = " " + toString(mCacheImageIndex) + "/" + toString(mTextureCount);
							loadNextTexture(mCacheImageIndex);
						}
					}
					else {
						mFboStatus = toString(mCacheImageIndex) + "/" + toString(mTextureCount) + " loaded in " + toString(msTotal) + "ms";
					}
				}
			}

			break;
		case VDTextureMode::NDI:
			mFboMsg = "ndi";
#if defined( CINDER_MSW )
			mNdiReceiver.update();
			if (mNdiReceiver.isReady()) {
				auto videoTexture = mNdiReceiver.getVideoTexture();
				if (videoTexture.first) {
					mInputTextureList[0].texture = videoTexture.first;
					mInputTextureList[0].name = mNdiReceiver.getCurrentSenderName();
					mInputTextureList[0].isValid = true;
				}
			}
#endif
			break;
		case VDTextureMode::SHARED:
			#if defined( CINDER_MSW )
			if (mInputTextureList[0].isValid) {
				mInputTextureList[0].texture = mSpoutIn.receiveTexture();
				mInputTextureList[0].name = mSpoutIn.getSenderName();
				mInputTextureList[0].isValid = true;
			}
			#elif defined( CINDER_MAC )
			if (mSyphonInitialized) {
				mClientSyphon.bind();
				ci::gl::TextureRef rectTex = mClientSyphon.getTexture();
				if (rectTex && mGlslVideoTexture && mSyphonBlitFbo) {
					gl::ScopedFramebuffer blitFbScp(mSyphonBlitFbo);
					gl::ScopedViewport blitVp(ivec2(0), mSyphonBlitFbo->getSize());
					gl::ScopedMatrices blitMat;
					gl::setMatricesWindow(mSyphonBlitFbo->getSize());
					rectTex->bind(0);
					gl::ScopedGlslProg blitShader(mGlslVideoTexture);
					mGlslVideoTexture->uniform("uSampler", 0);
					mGlslVideoTexture->uniform("uVideoSize", vec2((float)rectTex->getWidth(), (float)rectTex->getHeight()));
					gl::drawSolidRect(Rectf(0, 0, (float)mSyphonBlitFbo->getWidth(), (float)mSyphonBlitFbo->getHeight()));
					rectTex->unbind(0);
					mInputTextureList[0].texture = mSyphonBlitFbo->getColorTexture();
					mInputTextureList[0].name = "syphon: " + mClientSyphon.getServerName();
					mInputTextureList[0].isValid = true;
				}
				mClientSyphon.unbind();
			}
			#else
			setFboTextureAudioMode();
			#endif
			break;
		case VDTextureMode::MOVIE:
			mFboMsg = "video";
#if defined( CINDER_MSW )
			if (mIsVideoLoaded) {
				mVideo.update();
				if (mVideoReversed) {
					// see reverse()'s comment: native negative-rate playback isn't reliable here,
					// so reverse is simulated by manually stepping the position backward
					float fps = mVideo.getFrameRate();
					if (fps > 0.0f) {
						float newPos = mVideo.getPosition() - (1.0f / fps);
						if (newPos < 0.0f) {
							newPos = mVideo.isLooping() ? (mVideo.getDuration() + newPos) : 0.0f;
						}
						mVideo.setPosition(newPos);
					}
				}
				if (mVideo.hasTexture() && mGlslVideoTexture && mVideoBlitFbo) {
					// mVideo.getTexture() is GL_TEXTURE_RECTANGLE (see mVideoBlitFbo's comment in
					// VDFboShader.h) - every shader in this codebase expects a normal sampler2D
					// with normalized UVs, so blit it into a plain GL_TEXTURE_2D first, exactly
					// like the Syphon case below does for its own GL_TEXTURE_RECTANGLE_ARB input.
					// The DX/GL interop object backing this texture must be locked around any GL
					// access to it (same as ciWMFVideoPlayer::draw() already does internally) -
					// without this the GL side never observes the D3D-decoded frames and the
					// texture stays black, even though hasTexture()/loadMovie() all report success.
					ci::gl::TextureRef rectTex = mVideo.getTexture();
					mVideo.lockSharedTexture();
					gl::ScopedFramebuffer blitFbScp(mVideoBlitFbo);
					gl::ScopedViewport blitVp(ivec2(0), mVideoBlitFbo->getSize());
					gl::ScopedMatrices blitMat;
					gl::setMatricesWindow(mVideoBlitFbo->getSize());
					rectTex->bind(0);
					gl::ScopedGlslProg blitShader(mGlslVideoTexture);
					mGlslVideoTexture->uniform("uSampler", 0);
					mGlslVideoTexture->uniform("uVideoSize", vec2((float)rectTex->getWidth(), (float)rectTex->getHeight()));
					// destination rect drawn with its Y span swapped (height->0 instead of 0->height):
					// the decoded video frame comes out top-down (ciWMFVideoPlayer sets loadTopDown(true)
					// on mTex), the opposite of Syphon's own input to this same shader/blit-FBO pattern -
					// this is the one difference between the two call sites, and correcting it here
					// (not in the shared video_texture.fs.glsl, which Syphon also uses and is already
					// correct) keeps Syphon/images/post/fx.glsl untouched while fixing video specifically
					gl::drawSolidRect(Rectf(0, (float)mVideoBlitFbo->getHeight(), (float)mVideoBlitFbo->getWidth(), 0));
					rectTex->unbind(0);
					mVideo.unlockSharedTexture();
					mInputTextureList[0].texture = mVideoBlitFbo->getColorTexture();
					mInputTextureList[0].isValid = true;
				}
				else if (!mVideoTextureWarningLogged) {
					// one-shot (not per-frame) so this is findable in the log without flooding it -
					// if hasTexture() is still false here, ciWMFVideoPlayer never got a usable
					// shared texture for this file, independently of anything in this block
					mVideoTextureWarningLogged = true;
					CI_LOG_W("MOVIE mode: no video texture after load - hasTexture()=" << mVideo.hasTexture()
						<< " width=" << mVideo.getWidth() << " height=" << mVideo.getHeight()
						<< " glslLoaded=" << (mGlslVideoTexture != nullptr) << " blitFboLoaded=" << (mVideoBlitFbo != nullptr));
				}
			}
#endif
			break;
		}
		gl::ScopedFramebuffer fbScp(mFbo);
		if (mVDUniforms->getUniformValue(mVDUniforms->ICLEAR)) {
			gl::clear(Color::black());
		}
		// bind texture
		switch (mTextureMode)
		{
		case VDTextureMode::TEXT:
			// nothing
			break;
		case VDTextureMode::PARTS:
			for (size_t i{ 0 }; i < mInputTextureList.size(); i++)
			{
				if (mInputTextureList[i].texture) mInputTextureList[i].texture->bind(i);
			}
			break;
		case VDTextureMode::SEQUENCE: {
			unsigned int seqIdx = mSequenceManualControl ? (unsigned int)mCurrentImageSequenceIndex : (unsigned int)mVDUniforms->getUniformValue(mVDUniforms->IBARBEAT);
			if (mInputTextureList[seqIdx].isValid && mInputTextureList[seqIdx].texture) {
				mInputTextureList[seqIdx].texture->bind(0);
			}
			break;
		}
		default:
			// MOVIE falls through to here too - it populates mInputTextureList[0] above like
			// every other mode in this branch, so no special-casing needed at bind time
			if (mIsHydraTex) {
				mInputTextureList[0].texture->bind(253);
				for (size_t i{ 0 }; i < 4; i++)
				{
					mInputTextureList[i].texture->bind(254 + i);
				}
			}
			else {// case VDTextureMode::SHARED, AUDIO, MOVIE,..
				unsigned int activeIdx = getValidTexIndex(mInputTextureIndex);
				if (mInputTextureList[activeIdx].isValid && mInputTextureList[activeIdx].texture) {
					mInputTextureList[activeIdx].texture->bind(0);
				}
			}
			break;
		}

		// before setting uniforms!
		gl::ScopedGlslProg glslScope(mShader);
		int texNameEndIndex = 0;
		int channelIndex = 0;
		mUniforms = mShader->getActiveUniforms();
		for (const auto& uniform : mUniforms) {

			uniformName = uniform.getName();
			//CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", type:" + toString(uniform.getType()) + ", Location:" + toString(uniform.getLocation()));
			//if (mVDAnimation->isExistingUniform(name)) {
			int uniformType = uniform.getType();
			switch (uniformType)
			{

			case GL_FLOAT: // float 5126 0x1406
				if (uniformName == "TIME" || uniformName == "time") {
					mShader->uniform(uniformName, mVDUniforms->getUniformValue(mVDUniforms->ITIME));
				}
				else {
					if (mVDUniforms->isExistingUniform(uniformName)) {
						mShader->uniform(uniformName, mVDUniforms->getUniformValueByName(uniformName));
					}
					else {
						int l = uniform.getLocation();
						mShader->uniform(uniformName, mUniformValueByLocation[l]);
					}
				}
				break;
			case GL_SAMPLER_2D: // sampler2D 35678 0x8B5E
				texNameEndIndex = uniformName.find("iChannel");
				if (texNameEndIndex != std::string::npos && texNameEndIndex != -1) {
					// NASTY BUG! mShader->uniform(name, (uint32_t)(channelIndex));						
					mShader->uniform(uniformName, channelIndex);
					channelIndex++;
				}
				else {
					texNameEndIndex = uniformName.find("tex");
					if (texNameEndIndex != std::string::npos && texNameEndIndex != -1) {
						// hydra fbo
						mIsHydraTex = true;
						// 20210116 TODO 
						mShader->uniform(uniformName, 254 + channelIndex);
						/*
							osc(1,0.5,2).mult(shape(3)).out(o0)
							osc(2,0.5,2).mult(shape(4)).out(o1)
							osc(3,0.5).mult(shape(5)).out(o2)
							osc(4,0.5,2).mult(shape(6)).out(o3)
							src(o2).scale(1.05).rotate(0.1).blend(o1,0.1).blend(o3,0.1).blend(o0,0.1).out(o2)
							render(o2)
						*/
						channelIndex++;
					}
					else {
						// onezero, inputImage
						mShader->uniform(uniformName, 0);
					}
				}

				break;
			case GL_FLOAT_VEC2:// vec2 35664 0x8B50
				if (uniformName == "RENDERSIZE" || uniformName == "resolution") {
					mShader->uniform(uniformName, vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
				}
				else {
					mShader->uniform(uniformName, mVDUniforms->getVec2UniformValueByName(uniformName));
				}
				break;
			case GL_FLOAT_VEC3:// vec3 35665 0x8B51
				mShader->uniform(uniformName, mVDUniforms->getVec3UniformValueByName(uniformName));
				break;
			case GL_FLOAT_VEC4:// vec4 35666 0x8B52
				/*if (uniformName == "iMouse") {
					// mx my =0.0...
					mShader->uniform(uniformName, vec4(mVDUniforms->getUniformValue(mVDUniforms->IMOUSEX), mVDUniforms->getUniformValue(mVDUniforms->IMOUSEY), mVDUniforms->getUniformValue(mVDUniforms->IMOUSEZ), mVDUniforms->getUniformValue(mVDUniforms->IMOUSEW)));
				}
				else {*/
					if (uniformName == "iDate") {
						mShader->uniform(uniformName, vec4(mVDUniforms->getUniformValue(mVDUniforms->IDATEX), mVDUniforms->getUniformValue(mVDUniforms->IDATEY), mVDUniforms->getUniformValue(mVDUniforms->IDATEZ), mVDUniforms->getUniformValue(mVDUniforms->IDATEW)));
						//CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", IDATEX:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEX)) + ", IDATEY:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEY)) + ", IDATEZ:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEZ)) + ", IDATEW:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEW)));
					}
					else {
						mShader->uniform(uniformName, mVDUniforms->getVec4UniformValueByName(uniformName));
					}
				//}
				break;
			case GL_INT: // int 5124 0x1404
				// IBEAT 51
				// IBAR 52
				// IBARBEAT 53
				mShader->uniform(uniformName, mVDUniforms->getUniformValueByName(uniformName));
				break;
			case GL_BOOL: // boolean 35670 0x8B56
				//createBoolUniform(name, mVDAnimation->getUniformIndexForName(name), getBoolUniformValueByName(name)); // get same index as vdanimation
				mShader->uniform(uniformName, mVDUniforms->getUniformValueByName(uniformName));
				break;
			case GL_FLOAT_MAT4: // 35676 0x8B5C ciModelViewProjection
				break;
			default:
				break;
			}
		}

		gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
		// 20220421 TODO use with shader, not directly
		if (mTextureMode == VDTextureMode::MOVIE)
		{			
			gl::ScopedColor scopedColor(Colorf::white());
			gl::ScopedModelMatrix scopedModelMatrix;
			// video disabled
			// 20220421 TODO for not 720p:  gl::scale(vec3(1.0f));
			
		}
		if (mTextureMode == VDTextureMode::TEXT)
		{			
			gl::ScopedColor scopedColor(Colorf::white());
			gl::ScopedModelMatrix scopedModelMatrix;
			if (mTextTexture)
				gl::draw(mTextTexture);
		}

		mRenderedTexture = mFbo->getColorTexture();
		if (!isReady) {
			saveThumbnail(false);
		/*	std::string filename = mName + ".jpg";
			fs::path fr = getAssetPath("") / "thumbs" / filename;

			if (!fs::exists(fr)) {
				CI_LOG_V(fr.string() << " does not exist, creating");
				Surface s8(mRenderedTexture->createSource());
				writeImage(writeFile(fr), s8);
			}*/
		}
	}
	return mRenderedTexture;
}
void VDFboShader::saveThumbnail(bool overwrite) {
	std::string filename = mName + ".jpg";
	fs::path fr = getAssetPath("") / "thumbs" / filename;

	if (!fs::exists(fr) || overwrite) {
		//CI_LOG_V(fr.string() << " does not exist, creating");
		Surface s8(mRenderedTexture->createSource());
		writeImage(writeFile(fr), s8);
	}
}

ci::gl::Texture2dRef VDFboShader::getTexture() {
	if (mValid) {
		if (!isReady) {
			// render once for thumb
			getFboTexture();
			isReady = true;
		}
		getFboTexture();
	}
	return mRenderedTexture;
}
ci::gl::Texture2dRef VDFboShader::getRenderedTexture() {

	return mRenderedTexture;
}

bool									VDFboShader::isValid() {
	return mValid;
};

std::string								VDFboShader::getShaderName() {
	return mShaderName;
};

std::vector<ci::gl::GlslProg::Uniform>	VDFboShader::getUniforms() {
	return mUniforms;
};
