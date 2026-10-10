#include "VDMix.h"


//using namespace ci;
//using namespace ci::app;

namespace videodromm {

	VDMix::VDMix(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
	{
		// Params
		mVDParams = VDParams::create();

		CI_LOG_V("VDMix readSettings");
		// Settings
		mVDSettings = aVDSettings;
		// Animation
		mVDAnimation = aVDAnimation;
		// Uniforms
		mVDUniforms = aVDUniforms;

		mDefaultTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(false));

		mixPath = getAssetPath("") / "mix.json";
		if (fs::exists(mixPath))
		{
			restore(mixPath);
		}
		else
		{
			// Create json file if it doesn't already exist.
			std::ofstream oStream(mixPath.string());
			oStream.close();
			save();
		}

		// init fbo format
		fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
		fmt.setBorderColor(Color::black());
		// uncomment this to enable 4x antialiasing
		//fboFmt.setSamples( 4 );
		fboFmt.setColorTextureFormat(fmt);

		mMixetteTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(false));
		mMixetteFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
		fs::path mixetteFilePath = getAssetPath("") / "mixette.glsl";
		if (!fs::exists(mixetteFilePath)) {
			mError = mixetteFilePath.string() + " does not exist";
			CI_LOG_W(mError);
		}
		mGlslMixette = gl::GlslProg::create(mVDParams->getDefaultVertexString(), loadString(loadFile(mixetteFilePath)));

	} // constructor

	bool VDMix::save()
	{
		JsonTree doc;

		JsonTree settings = JsonTree::makeArray("settings");
		// "assetspath" stays the bare folder name, readable by the vs2017 apps; the parent folder
		// (e.g. "glsl") goes to "assetsfolder", which they ignore
		std::string assetsPath = mAssetsPath;
		std::string assetsFolder;
		std::size_t slash = assetsPath.find_last_of("/\\");
		if (slash != std::string::npos) {
			assetsFolder = assetsPath.substr(0, slash);
			assetsPath = assetsPath.substr(slash + 1);
		}
		settings.addChild(ci::JsonTree("assetspath", assetsPath));
		if (!assetsFolder.empty()) settings.addChild(ci::JsonTree("assetsfolder", assetsFolder));
		doc.pushBack(settings);
		doc.write(writeFile(mixPath), JsonTree::WriteOptions());
		return true;
	}
	

	void VDMix::restore(const fs::path& aFilePath)
	{
		// check to see if json file exists
		if (!fs::exists(aFilePath)) {
			return;
		}
		try {
			JsonTree doc(loadFile(aFilePath));
			if (doc.hasChild("settings")) {
				JsonTree settings(doc.getChild("settings"));
				if (settings.hasChild("assetspath")) mAssetsPath = settings.getValueForKey<string>("assetspath");
				// optional parent folder under assets (e.g. "glsl"); without it, VDSession::resolveFolder()
				// still finds a bare name under assets/glsl or the assets root
				if (settings.hasChild("assetsfolder") && !mAssetsPath.empty()) {
					std::string assetsFolder = settings.getValueForKey<string>("assetsfolder");
					if (!assetsFolder.empty()) mAssetsPath = assetsFolder + "/" + mAssetsPath;
				}
			}
			if (doc.hasChild("uniforms")) {
				JsonTree uniforms(doc.getChild("uniforms"));
				for (unsigned int i = 0; i < 100; i++)
				{
					if (uniforms.hasChild(mVDUniforms->getUniformName(i))) {
						mVDUniforms->setUniformValue(i, uniforms.getValueForKey<float>(mVDUniforms->getUniformName(i)));
					}
				}
			}
			

			/* 20211227 mTextureList moved to fboshader if (doc.hasChild("camera")) {
				JsonTree settings(doc.getChild("camera"));
				if (settings.hasChild("texturename")) {
					TextureCameraRef tc(TextureCamera::create());
					mTextureList.push_back(tc);
					// init with shader, colors inverted
					JsonTree jsonInverted;
					JsonTree shaderInverted = ci::JsonTree::makeArray("shader");
					shaderInverted.addChild(ci::JsonTree("shadername", "inverted"));
					shaderInverted.pushBack(ci::JsonTree("shadertype", "fs"));
					shaderInverted.pushBack(ci::JsonTree("shadertext", mVDParams->getInvertedDefaultShaderFragmentString()));
					jsonInverted.addChild(shaderInverted);
					JsonTree textureInverted = ci::JsonTree::makeArray("texture");
					textureInverted.addChild(ci::JsonTree("texturename", "audio"));
					textureInverted.pushBack(ci::JsonTree("texturetype", "audio"));
					textureInverted.pushBack(ci::JsonTree("texturemode", 0));
					jsonInverted.addChild(textureInverted);
					mMixFboShader = VDFboShader::create(mVDUniforms, mVDAnimation, jsonInverted, 0, mAssetsPath);
					mFboShaderList.push_back(mMixFboShader);
					setFboInputTexture(getFboShaderListSize() - 1, 1);
				}
			}
			if (doc.hasChild("shared")) {
				JsonTree settings(doc.getChild("shared"));
				if (settings.hasChild("name")) {
					ts = TextureShared::create();
					mTextureList.push_back(ts);
				}
			}*/
		}
		catch (const JsonTree::ExcJsonParserError& exc) {
			CI_LOG_W(exc.what());
		}
	}
	unsigned int VDMix::getValidFboIndex(unsigned int aFboIndex) {
		return math<int>::min(aFboIndex, (unsigned int)mFboShaderList.size() - 1);
	}
	
	unsigned int VDMix::createFboShaderTexture(const JsonTree &json, unsigned int aFboIndex, const std::string& aFolder) {
		unsigned int rtn = 0;
		if (aFolder != "") mAssetsPath = aFolder;
		VDFboShaderRef fboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, aFboIndex, mAssetsPath);
		if (mFboShaderList.size() == 0 || aFboIndex == 0) { // 20220321 tmp
			mFboShaderList.push_back(fboShader);
			rtn = (unsigned int)mFboShaderList.size() - 1;
		}
		else if (aFboIndex <= mFboShaderList.size() - 1) {
			rtn = aFboIndex;
			mFboShaderList[rtn] = fboShader;
		}
		else {
			mFboShaderList.push_back(fboShader);
			rtn = (unsigned int)mFboShaderList.size() - 1;
		}
		return rtn;
	}
	ci::gl::TextureRef VDMix::getMixetteTexture(unsigned int aFboIndex) {

		gl::ScopedFramebuffer fbScp(mMixetteFbo);
		// clear out the FBO with black
		gl::clear(Color::black());

		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mMixetteFbo->getSize());
		// fbo-sized matrices, not the window's (a resized window truncated the mix)
		gl::ScopedMatrices scpMtx;
		gl::setMatricesWindow(mMixetteFbo->getSize());

		// VDFboShader::getTexture() doesn't just return a cached texture ref - it actively
		// re-renders that fbo's own shader every time it's called (calls getFboTexture()
		// unconditionally), and that render binds *this fbo's own input texture* to GL texture
		// unit 0 internally, as part of its normal single-input-texture draw. Interleaving that
		// with binding each fbo's *output* to its mixette channel (as this loop used to do,
		// getTexture() immediately followed by ->bind(i)) meant every fbo rendered *after* fbo 0
		// stomped fbo 0's unit-0 binding via its own internal render, before the mixette shader
		// ever ran - so channel 0 ended up sampling whatever the *last* fbo to render had bound
		// for its own purposes, not fbo 0's output. Confirmed via the throttled diagnostic below:
		// fbo 0 was always isValid()=1, weight=1 (correct), so the bug was never the C++ state
		// here - it was this ordering. Fixed by rendering every fbo first (collecting the
		// resulting texture refs) and only binding them to their mixette channel afterward, once
		// no further fbo render can still clobber unit 0.
		std::vector<ci::gl::Texture2dRef> renderedTextures(mFboShaderList.size());
		int i = 0;
		for (auto &fbo : mFboShaderList) {
			if (fbo->isValid() && mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + i) > 0.01f) {
				renderedTextures[i] = fbo->getTexture();
			}
			i++;
		}
		i = 0;
		for (auto &tex : renderedTextures) {
			if (tex) tex->bind(i);
			i++;
		}
		gl::ScopedGlslProg prog(mGlslMixette);
		mGlslMixette->uniform("iResolution", vec3(mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONX), mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONY), 1.0));
		mGlslMixette->uniform("iBlendmode", (int)mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE));
		i = 0;
		for (auto &fbo : mFboShaderList) {
			if (fbo->isValid()) {
				mGlslMixette->uniform("iChannel" + toString(i), i);
				mGlslMixette->uniform("iWeight" + toString(i), mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + i));
			}
			i++;
		}

		gl::drawSolidRect(Rectf(0, 0, mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONX), mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONY)));
		mMixetteTexture = mMixetteFbo->getColorTexture();
		return mMixetteTexture;
	}
	std::vector<ci::gl::GlslProg::Uniform> VDMix::getFboShaderUniforms(unsigned int aFboShaderIndex) {
		return mFboShaderList[aFboShaderIndex]->getUniforms();
	}

	float VDMix::getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex) {
		return mFboShaderList[aFboShaderIndex]->getUniformValueByLocation(aLocationIndex);
	};
	void VDMix::setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue) {
		mFboShaderList[aFboShaderIndex]->setUniformValueByLocation(aLocationIndex, aValue);
	};

	unsigned int VDMix::findAvailableIndex(unsigned int aFboShaderIndex, const JsonTree &json) {
		unsigned int rtn = aFboShaderIndex;
		unsigned int iSecond = (unsigned int)getElapsedSeconds();
		CI_LOG_V(" mCurrentSecond " + toString(mCurrentSecond) + " getElapsedSeconds " + toString(iSecond) + " mCurrentIndex " + toString(mCurrentIndex));
		// For hydra, several shaders can be received at once
		if (aFboShaderIndex == 0) {
			if (iSecond != mCurrentSecond) {
				mCurrentSecond = iSecond;
				mCurrentIndex = 0;
			}
			else {
				mCurrentIndex++;
			}
			aFboShaderIndex = mCurrentIndex;
		}

		// init the list the first time
		if (mFboShaderList.size() == 0) {
			// create fbo
			VDFboShaderRef fboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, 0, mAssetsPath);
			mFboShaderList.push_back(fboShader);
			rtn = (unsigned int)mFboShaderList.size() - 1;
		}
		else {
			// change current existing fbo
			if (aFboShaderIndex < mFboShaderList.size()) {
				rtn = aFboShaderIndex;
			}
			else {
				// add to list until MAXSHADERS is reached
				if (aFboShaderIndex < MAXSHADERS) {
					// create fbo
					mFboShaderList.push_back(VDFboShader::create(mVDUniforms, mVDAnimation, json, mFboShaderList.size(), mAssetsPath));
					rtn = mFboShaderList.size() - 1;
				}
				else {
					// reuse existing, last one if no invalid found
					rtn = mFboShaderList.size() - 1;
					unsigned int found = -1;
					for (auto &fbo : mFboShaderList) {
						found++;
						if (!fbo->isValid()) {
							rtn = found;
							break;
						}
					}
				}
			}
		}
		return rtn;
	}

	bool VDMix::setFragmentShaderString(const string& aFragmentShaderString, const std::string& aName, unsigned int aFboShaderIndex) {
		// received from websocket, tested with hydra
		JsonTree		json;
		JsonTree shader = ci::JsonTree::makeArray("shader");
		shader.addChild(ci::JsonTree("shadername", aName));
		shader.pushBack(ci::JsonTree("shadertype", "fs"));
		shader.pushBack(ci::JsonTree("shadertext", aFragmentShaderString));
		json.addChild(shader);
		JsonTree texture = ci::JsonTree::makeArray("texture");
		texture.addChild(ci::JsonTree("texturename", "audio"));
		texture.pushBack(ci::JsonTree("texturetype", "audio"));
		texture.pushBack(ci::JsonTree("texturemode", VDTextureMode::AUDIO));
		json.addChild(texture);
		int rtn = findAvailableIndex(aFboShaderIndex, json); // 20240518 was 0
		mFboShaderList[rtn]->setFragmentShaderString(aFragmentShaderString, aName);
		return rtn;
	}
	bool VDMix::setFragmentShaderStringAtIndex(const string& aFragmentShaderString, const std::string& aName, unsigned int aFboShaderIndex) {
		if (mFboShaderList.size() == 0) return false;
		unsigned int index = getValidFboIndex(aFboShaderIndex);
		return mFboShaderList[index]->setFragmentShaderString(aFragmentShaderString, aName);
	}
	unsigned int VDMix::findFirstZeroWeightFboIndex() {
		unsigned int count = (unsigned int)mFboShaderList.size();
		if (count > MAXSHADERS) count = MAXSHADERS;
		for (unsigned int f = 0; f < count; f++) {
			if (mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + f) == 0.0f) {
				return f;
			}
		}
		return count > 0 ? count - 1 : 0;
	}
	int VDMix::loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex) {
		JsonTree		json;
		JsonTree shader = ci::JsonTree::makeArray("shader");
		shader.addChild(ci::JsonTree("shadername", "todo.txt"));
		shader.pushBack(ci::JsonTree("shadertype", "fs"));
		shader.pushBack(ci::JsonTree("shadertext", "todo"));
		json.addChild(shader);
		JsonTree texture = ci::JsonTree::makeArray("texture");
		texture.addChild(ci::JsonTree("texturename", "audio"));
		texture.pushBack(ci::JsonTree("texturetype", "audio"));
		texture.pushBack(ci::JsonTree("texturemode", VDTextureMode::AUDIO));
		json.addChild(texture);

		// if aFboShaderIndex is out of bounds try to find invalid fbo index or create a new fbo until MAX
		int rtn = findAvailableIndex(aFboShaderIndex, json);

		mFboShaderList[rtn]->loadFragmentShaderFromFile(aFilePath, true);//isAudio=true for dnd glsl files
		mVDSettings->setMsg("loaded " + mFboShaderList[rtn]->getShaderName() + "\n try at " + toString(aFboShaderIndex) + " valid at " + toString(rtn));
		return rtn;
	}
	
#pragma region blendmodes


#pragma region textures
	void VDMix::loadImageFile(const std::string& aFile, unsigned int aFboIndex) {
		fs::path texFileOrPath = aFile;
		if (fs::exists(texFileOrPath)) {

			std::string ext = "";
			int dotIndex = texFileOrPath.filename().string().find_last_of(".");
			if (dotIndex != std::string::npos)  ext = texFileOrPath.filename().string().substr(dotIndex + 1);
			if (ext == "jpg" || ext == "png") {
				if (aFboIndex >= mFboShaderList.size()) {
					// beyond any existing fbo: into the pool only, paused (any fbo can pick it)
					addVideoSource(aFile, false);
				}
				else {
					// dropped onto an existing fboshader: replaced by the inputImage.fs passthrough,
					// which shows the video as it is
					std::string name = addVideoSource(aFile, false);
					if (!name.empty()) {
						mFboShaderList[aFboIndex]->loadFragmentShaderFromFile("inputImage.fs");
						mFboShaderList[aFboIndex]->assignVideoInput(getVideoSource(name)->getTexture(), name);
					}
				}
			}
		}
	}

	namespace {
		std::string getExtensionLower(const fs::path& aPath) {
			std::string ext;
			std::string filename = aPath.filename().string();
			std::size_t dotIndex = filename.find_last_of(".");
			if (dotIndex != std::string::npos) ext = filename.substr(dotIndex + 1);
			for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
			return ext;
		}
	}

	bool VDMix::loadTextureIntoFboActiveSlot(unsigned int aFboIndex, const std::string& aFile) {
		if (aFboIndex >= mFboShaderList.size()) return false;
		std::string ext = getExtensionLower(fs::path(aFile));
		if (ext == "jpg" || ext == "png") {
			mFboShaderList[aFboIndex]->loadImageFile(aFile);
			// IMAGE mode: an AUDIO/MOVIE/... mode would keep its own source in slot 0
			mFboShaderList[aFboIndex]->setImageMode();
			return true;
		}
		if (ext == "mp4" || ext == "mov") {
			// into the pool, paused on its first frame (the Play button starts it); the fbo's shader
			// is replaced by the inputImage.fs passthrough, which shows the video as it is
			std::string name = addVideoSource(aFile, false);
			if (name.empty()) return false;
			mFboShaderList[aFboIndex]->loadFragmentShaderFromFile("inputImage.fs");
			mFboShaderList[aFboIndex]->assignVideoInput(getVideoSource(name)->getTexture(), name);
			return true;
		}
		return false;
	}

	bool VDMix::addDroppedTextureOutsideFbos(const std::string& aFile) {
		std::string ext = getExtensionLower(fs::path(aFile));
		// a video gets a pool entry of its own, paused: any fbo can then pick it
		if (ext == "mp4" || ext == "mov") return !addVideoSource(aFile, false).empty();
		return addStandaloneTexture(aFile);
	}

	std::string VDMix::addVideoSource(const std::string& aPath, bool aAutoPlay) {
		std::string name = fs::path(aPath).filename().string();
		if (getVideoSource(name)) return name;
		VDVideoSourceRef source = VDVideoSource::create(aPath, mVDAnimation->getPreferredAudioOutputDevice(),
			ivec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()), aAutoPlay);
		if (!source) return "";
		mVideoSources.push_back(source);
		registerLoadedTexture(name, source->getTexture());
		return name;
	}

	void VDMix::updateVideoSources() {
		// videos named by fbo jsons (a loaded folder): into the pool paused (the shaders may not be
		// loaded yet; started by hand, which also resets ITIME for sync), then assigned
		for (auto& fbo : mFboShaderList) {
			std::string pending = fbo->takePendingVideoFile();
			if (pending.empty()) continue;
			std::string name = addVideoSource(pending, false);
			if (!name.empty()) fbo->assignVideoInput(getVideoSource(name)->getTexture(), name);
			else fbo->setFboTextureAudioMode();
		}
		for (auto& source : mVideoSources) {
			source->update();
			// volume: level x the highest weight of the fbos showing it (the level alone when none
			// does, it keeps playing in the pool), 0 while scrubbing
			float maxWeight = -1.0f;
			for (unsigned int i = 0; i < mFboShaderList.size(); i++) {
				if (!mFboShaderList[i]->isMovie() || mFboShaderList[i]->getTextureName(0) != source->getName()) continue;
				float weight = (i <= 8) ? mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + i) : 1.0f;
				maxWeight = std::max(maxWeight, weight);
			}
			float volume = source->getVolumeLevel() * (maxWeight < 0.0f ? 1.0f : maxWeight);
			source->setOutputVolume(source->isScrubbing() ? 0.0f : volume);
		}
	}

	void VDMix::togglePlayPauseSource(const std::string& aName) {
		VDVideoSourceRef video = getVideoSource(aName);
		if (!video) return;
		if (!video->isPlaying()) {
			for (auto& other : mVideoSources) if (other != video) other->pause();
			if (mVDAnimation->isAudioFilePlaying()) mVDAnimation->toggleAudioFilePlayPause();
			// every Play restarts ITIME at 0, so the shaders are in sync with the video
			mVDAnimation->setAudioFileClock(false);
			mVDUniforms->resetTime();
		}
		video->togglePlayPause();
	}

	void VDMix::togglePlayPause(unsigned int aFboIndex) {
		if (mFboShaderList.empty()) return;
		if (VDVideoSourceRef video = videoFor(aFboIndex)) {
			togglePlayPauseSource(video->getName());
			return;
		}
		VDFboShaderRef fbo = mFboShaderList[getValidFboIndex(aFboIndex)];
		if (!fbo->isPlaying()) {
			// starting the audio file pauses every playing video (one sound at a time)
			if (fbo->isAudioFile()) for (auto& source : mVideoSources) source->pause();
			if (fbo->isAudioFile()) {
				// ITIME follows the file's position from now on (VDAnimation::update()): Play after
				// Pause resumes in sync, Play after the end restarts both at 0
				mVDAnimation->setAudioFileClock(true);
			}
			else {
				// a sequence Play restarts ITIME at 0, for sync
				mVDAnimation->setAudioFileClock(false);
				mVDUniforms->resetTime();
			}
		}
		fbo->togglePlayPause();
	}

	void VDMix::registerLoadedTexture(const std::string& aName, ci::gl::Texture2dRef aTexture) {
		if (!aTexture) return;
		for (auto& entry : mLoadedTextures) {
			if (entry.name == aName) {
				// same name already in the pool - update in place (e.g. a movie's shared texture
				// whose content changes live) rather than adding a duplicate
				entry.texture = aTexture;
				entry.isValid = true;
				return;
			}
		}
		VDTextureStruct newEntry;
		newEntry.name = aName;
		newEntry.texture = aTexture;
		newEntry.ms = 0;
		newEntry.isValid = true;
		mLoadedTextures.push_back(newEntry);
	}

	bool VDMix::addStandaloneTexture(const std::string& aFile) {
		fs::path texFileOrPath = aFile;
		if (!fs::exists(texFileOrPath)) return false;
		std::string ext = getExtensionLower(texFileOrPath);
		// no standalone equivalent for video - a movie needs its own ciWMFVideoPlayer, which only
		// exists per-fbo today (see VDFboShader::loadVideoFile())
		if (ext != "jpg" && ext != "png") return false;
		try {
			ci::gl::Texture2dRef tex = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(false).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
			registerLoadedTexture(texFileOrPath.filename().string(), tex);
			return true;
		}
		catch (const std::exception& ex) {
			CI_LOG_E("<< addStandaloneTexture >> " << aFile << " error: " << ex.what());
			return false;
		}
	}

#pragma endregion textures

} // namespace videodromm
