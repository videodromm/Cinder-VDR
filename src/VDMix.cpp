#include "VDMix.h"


//using namespace ci;
//using namespace ci::app;

namespace videodromm {

	VDMix::VDMix(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
		/*: mFlipV(false)
		, mFlipH(false)*/
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
		// TODO TMP mDefaultTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
		// check to see if mix.json file exists and restore if it does
		mixPath = getAssetPath("") / mixFileName;
		if (fs::exists(mixPath))
		{
			restore();
		}
		else
		{
			// Create json file if it doesn't already exist.
			std::ofstream oStream(mixPath.string());
			oStream.close();
			save();
		}

		//mDefaultTexture = ci::gl::Texture::create(loadImage(loadAsset("0.jpg")));
		//loadImageFile("0.jpg", 0);
		TextureAudioRef t(TextureAudio::create(mVDAnimation));

		mTextureList.push_back(t);

		mMixetteTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
		// init fbo format
		fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
		fmt.setBorderColor(Color::black());
		// uncomment this to enable 4x antialiasing
		//fboFmt.setSamples( 4 );
		fboFmt.setColorTextureFormat(fmt);
		mMixetteFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
		fs::path mMixetteFilePath = getAssetPath("") / "mixette.glsl";
		if (!fs::exists(mMixetteFilePath)) {
			mError = mMixetteFilePath.string() + " does not exist";
			CI_LOG_V(mError);
			mVDSettings->mErrorMsg = mError + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
		}

		mGlslMixette = gl::GlslProg::create(mVDParams->getDefaultVertexString(), loadString(loadFile(mMixetteFilePath)));

		// init with mix shader of 2 fbos with crossfade
		JsonTree		json;
		JsonTree shader = ci::JsonTree::makeArray("shader");
		shader.addChild(ci::JsonTree("shadername", "mix"));
		shader.pushBack(ci::JsonTree("shadertype", "fs"));
		shader.pushBack(ci::JsonTree("shadertext", mVDParams->getDefaultShaderFragmentString()));
		json.addChild(shader);
		JsonTree texture = ci::JsonTree::makeArray("texture");
		texture.addChild(ci::JsonTree("texturename", "audio"));
		texture.pushBack(ci::JsonTree("texturetype", "audio"));
		texture.pushBack(ci::JsonTree("texturemode", 0));
		json.addChild(texture);
		mMixFboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, 0, mAssetsPath);
		mFboShaderList.push_back(mMixFboShader);
		loadFbos();

		// initialize the textures list with audio texture
		/*mTexturesFilepath = getAssetPath("") / mVDSettings->mAssetsPath / "textures.xml";
		initTextureList();


			// render fbo
		mRenderFbo = gl::Fbo::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, fboFmt);
		// mix fbo to render
		warpMixToRender = 0;
		//mWarpSettings = getAssetPath("") / mVDSettings->mAssetsPath / "warps.xml";
		mWarpJson = getAssetPath("") / mVDSettings->mAssetsPath / "warps.json";

		mCurrentBlend = 0;
		for (size_t i {0}; i < mVDAnimation->getBlendModesCount(); i++)
		{
			mBlendFbos[i] = gl::Fbo::create(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight(), fboFmt);
		}


		*/

	} // constructor

	bool VDMix::save()
	{
		JsonTree doc;

		JsonTree settings = JsonTree::makeArray("settings");
		settings.addChild(ci::JsonTree("assetspath", mAssetsPath));
		doc.pushBack(settings);
		doc.write(writeFile(mixPath), JsonTree::WriteOptions());
		return true;
	}

	void VDMix::restore()
	{
		// check to see if json file exists
		if (!fs::exists(mixPath)) {
			return;
		}
		try {
			JsonTree doc(loadFile(mixPath));
			if (doc.hasChild("settings")) {
				JsonTree settings(doc.getChild("settings"));
				if (settings.hasChild("assetspath")) mAssetsPath = settings.getValueForKey<string>("assetspath");
			}
		}
		catch (const JsonTree::ExcJsonParserError& exc) {
			CI_LOG_W(exc.what());
		}
	}

	void VDMix::loadFbos() {

		int f = 0;
		bool found = true;
		string shaderFileName;
		string textureFileName;
		while (found) {
			string jsonFileName = "fbo" + toString(f) + ".json";
			fs::path jsonFile = getAssetPath("") / mAssetsPath / jsonFileName;
			if (fs::exists(jsonFile)) {
				JsonTree json(loadFile(jsonFile));
				createFboShaderTexture(json, f);
				f++;
			}
			else {
				found = false;
			}
		} //while

	}
	/*unsigned int VDMix::fboFromJson(const JsonTree& json, unsigned int aFboIndex) {
		unsigned int rtn = 0;

		rtn = createFboShaderTexture(json, aFboIndex);
		return rtn;
	}*/
	unsigned int VDMix::createFboShaderTexture(const JsonTree &json, unsigned int aFboIndex) {
		unsigned int rtn = 0;
		VDFboShaderRef fboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, aFboIndex, mAssetsPath);
		if (aFboIndex == 0) {
			mFboShaderList.push_back(fboShader);
			rtn = mFboShaderList.size() - 1;
		}
		else {
			rtn = math<int>::min(aFboIndex, mFboShaderList.size() - 1);
			mFboShaderList[rtn] = fboShader;
		}

		return rtn;
	}
	std::vector<ci::gl::GlslProg::Uniform> VDMix::getFboShaderUniforms(unsigned int aFboShaderIndex) {
		return mFboShaderList[aFboShaderIndex]->getUniforms();
	}

	int VDMix::getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex) {
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
		}
		aFboShaderIndex = mCurrentIndex;

		// init the list the first time
		if (mFboShaderList.size() == 0) {
			// create fbo
			VDFboShaderRef fboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, 0, mAssetsPath);
			mFboShaderList.push_back(fboShader);
			rtn = mFboShaderList.size() - 1;
		}
		else {
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

				
				/*if (aFboShaderIndex > mFboShaderList.size() - 1) {

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

				}*/


			}
		}

		return rtn;
	}

	bool VDMix::setFragmentShaderString(const string& aFragmentShaderString, const std::string& aName) {
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
		texture.pushBack(ci::JsonTree("texturemode", 0));
		json.addChild(texture);
		int rtn = findAvailableIndex(0, json);
		mFboShaderList[rtn]->setFragmentShaderString(aFragmentShaderString, aName + toString(rtn));
		return rtn;
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
		texture.pushBack(ci::JsonTree("texturemode", 0));
		json.addChild(texture);

		// if aFboShaderIndex is out of bounds try to find invalid fbo index or create a new fbo until MAX
		int rtn = findAvailableIndex(aFboShaderIndex, json);

		mFboShaderList[rtn]->loadFragmentShaderFromFile(aFilePath);
		mVDSettings->mMsg = "loaded " + mFboShaderList[rtn]->getShaderName() + "\n try at " + toString(aFboShaderIndex) + " valid at " + toString(rtn) + "\n" + mVDSettings->mMsg.substr(0, mVDSettings->mMsgLength);
		return rtn;
	}
	ci::gl::TextureRef VDMix::getMixetteTexture(unsigned int aFboIndex) {

		gl::ScopedFramebuffer fbScp(mMixetteFbo);
		// clear out the FBO with black
		gl::clear(Color::black());

		// nasty bug! bind to 100+f
		int f = 0;
		for (auto &fbo : mFboShaderList) {
			if (mFboShaderList[f]->isValid()) {// white mix bug && mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0 + f) > 0.05f) {
				//fbo->getTexture()->bind(f); not in right order
				mFboShaderList[f]->getTexture()->bind(100 + f);
			}
			f++;
		}
		gl::ScopedGlslProg prog(mGlslMixette);
		mGlslMixette->uniform("iResolution", vec3(mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONX), mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONY), 1.0));
		int i = 0;
		for (auto &fbo : mFboShaderList) {
			if (fbo->isValid()) {// white mix bug && mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0 + i) > 0.1f) {
				mGlslMixette->uniform("iChannel" + toString(i), 100 + i);
				mGlslMixette->uniform("iWeight" + toString(i), mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + i));
			}
			i++;
		}

		gl::drawSolidRect(Rectf(0, 0, mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONX), mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONY)));
		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mMixetteFbo->getSize());
		mMixetteTexture = mMixetteFbo->getColorTexture();
		return mMixetteTexture;// mMixetteFbo->getColorTexture();
	}
#pragma region blendmodes


#pragma region textures

	void VDMix::loadImageFile(const std::string& aFile, unsigned int aTextureIndex) {
		int rtn = math<int>::min(aTextureIndex, mFboShaderList.size() - 1);
		fs::path texFileOrPath = aFile;
		if (fs::exists(texFileOrPath)) {

			std::string ext = "";
			int dotIndex = texFileOrPath.filename().string().find_last_of(".");
			if (dotIndex != std::string::npos)  ext = texFileOrPath.filename().string().substr(dotIndex + 1);
			if (ext == "jpg" || ext == "png") {
				JsonTree		json;
				JsonTree texture = ci::JsonTree::makeArray("texture");
				texture.addChild(ci::JsonTree("texturename", aFile));
				texture.pushBack(ci::JsonTree("texturetype", "image"));
				json.addChild(texture);
				TextureImageRef t(TextureImage::create());
				t->fromJson(texture);
				mTextureList.push_back(t);

				if (mFboShaderList.size() < 1) {
					// no fbos, create one
					JsonTree shader = ci::JsonTree::makeArray("shader");
					shader.addChild(ci::JsonTree("shadername", "inputImage.fs"));
					shader.pushBack(ci::JsonTree("shadertype", "fs"));
					json.addChild(shader);
					createFboShaderTexture(json);
				}
				else {
					mFboShaderList[rtn]->setInputTextureRef(mTextureList[mTextureList.size() - 1]->getTexture());
				}
			}
		}
	}
	bool VDMix::loadImageSequence(const string& aFolder, unsigned int aTextureIndex) {
		/*if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		CI_LOG_V("loadImageSequence " + aFolder + " at textureIndex " + toString(aTextureIndex));
		// add texture xml
		XmlTree			textureXml;
		textureXml.setTag("texture");
		textureXml.setAttribute("id", "0");
		textureXml.setAttribute("texturetype", "sequence");
		textureXml.setAttribute("path", aFolder);
		TextureImageSequenceRef t(new TextureImageSequence(mVDAnimation));
		if (t->fromXml(textureXml)) {
			mTextureList.push_back(t);
			return true;
		}
		else {
			return false;
		}*/
		return false;
	}
	/*ci::gl::TextureRef VDMix::getInputTexture(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getTexture();
	}
	string VDMix::getInputTextureName(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getName();
	}
	unsigned int VDMix::getInputTexturesCount() {
		return mTextureList.size();
	}

	void VDMix::loadImageFile(const string& aFile, unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		CI_LOG_V("loadImageFile " + aFile + " at textureIndex " + toString(aTextureIndex));
		mTextureList[aTextureIndex]->loadFromFullPath(aFile);
	}
	void VDMix::loadAudioFile(const string& aFile) {
		mTextureList[0]->loadFromFullPath(aFile);
	}


	// sequence
	bool VDMix::isSequence(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return (mTextureList[aTextureIndex]->getType() == mTextureList[aTextureIndex]->SEQUENCE);
	}
	bool VDMix::isLoadingFromDisk(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return (mTextureList[aTextureIndex]->isLoadingFromDisk());
	}
	void VDMix::toggleLoadingFromDisk(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->toggleLoadingFromDisk();
	}
	void VDMix::syncToBeat(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->syncToBeat();
	}
	void VDMix::reverse(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->reverse();
	}
	float VDMix::getSpeed(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getSpeed();
	}
	void VDMix::setSpeed(unsigned int aTextureIndex, float aSpeed) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->setSpeed(aSpeed);
	}
	int VDMix::getPosition(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getPosition();
	}
	void VDMix::setPlayheadPosition(unsigned int aTextureIndex, int aPosition) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->setPlayheadPosition(aPosition);
	}
	int VDMix::getMaxFrame(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getMaxFrame();
	}*/
#pragma endregion textures


	/*unsigned int VDMix::getFboBlendCount() {
		return mBlendFbos.size();
	}
	ci::gl::TextureRef VDMix::getFboThumb(unsigned int aBlendIndex) {
		//if (aBlendIndex > mBlendFbos.size() - 1) aBlendIndex = mBlendFbos.size() - 1;
		return mBlendFbos[aBlendIndex]->getColorTexture();
	}
	void VDMix::useBlendmode(unsigned int aBlendIndex) {
		//if (aBlendIndex > mBlendFbos.size() - 1) aBlendIndex = 0;
		mVDSettings->iBlendmode = aBlendIndex;
	}
	ci::gl::TextureRef VDMix::getMixTexture(unsigned int aMixFboIndex) {
		if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = 0;
		if (!mMixFbos[aMixFboIndex].texture) {
			// should never happen
			mMixFbos[aMixFboIndex].texture = gl::Texture2d::create(mVDParams->getFboWidth(), mVDParams->getFboHeight());
		}
		if (!mMixFbos[aMixFboIndex].fbo) {
			// should never happen
			mMixFbos[aMixFboIndex].fbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
		}
		// texturing sharing
		if (mSharedOutputActive && mSharedFboIndex == aMixFboIndex) {
#if defined( CINDER_MSW )
			// spout
			if (mSpoutInitialized) {
				mSpoutSender.SendTexture(mMixFbos[mSharedFboIndex].texture->getId(), mMixFbos[mSharedFboIndex].texture->getTarget(), mVDParams->getFboWidth(), mVDParams->getFboHeight());
			}
#endif
#if defined( CINDER_MAC )
			// syphon
			mSyphonServer.publishTexture(mMixFbos[mSharedFboIndex].texture);
#endif
		}
		return mMixFbos[aMixFboIndex].texture;
	}*/
	// spout output
	/*void VDMix::toggleSharedOutput(unsigned int aMixFboIndex) {
		if (aMixFboIndex < mMixFbos.size()) {
			mSharedFboIndex = aMixFboIndex;
		}
		mSharedOutputActive = !mSharedOutputActive;
#if defined( CINDER_MSW )
		if (mSharedOutputActive && !mSpoutInitialized) {
			// Initialize a sender
			mSpoutInitialized = mSpoutSender.CreateSender(mSenderName, mVDParams->getFboWidth(), mVDParams->getFboHeight());
		}
#endif
	}

	ci::gl::TextureRef VDMix::getFboTexture(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
		return mFboList[aFboIndex]->getFboTexture();
	}
	ci::gl::TextureRef VDMix::getFboRenderedTexture(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
		return mFboList[aFboIndex]->getRenderedTexture();
	}

	void VDMix::renderBlend()
	{
		if (mCurrentBlend > mBlendFbos.size() - 1) mCurrentBlend = 0;
		gl::ScopedFramebuffer scopedFbo(mBlendFbos[mCurrentBlend]);
		gl::clear(Color::black());
		// texture binding must be before ScopedGlslProg
		mFboList[mWarpList[warpMixToRender]->getAFboIndex()]->getRenderedTexture()->bind(0);
		mFboList[mWarpList[warpMixToRender]->getBFboIndex()]->getRenderedTexture()->bind(1);
		gl::ScopedGlslProg glslScope(mGlslBlend);
		gl::drawSolidRect(Rectf(0, 0, mBlendFbos[mCurrentBlend]->getWidth(), mBlendFbos[mCurrentBlend]->getHeight()));
	}*/
#pragma endregion blendmodes


#pragma region warps
	/*void VDMix::setCurrentEditIndex(unsigned int aIndex) {
		mCurrentEditIndex = aIndex;
	}
	unsigned int VDMix::getSoloOrActiveIndex() {
		unsigned int rtn = 0;
		if (mSolo > -1) {
			rtn = mSolo;
		}
		else {
			if (mCurrentEditIndex > mWarpList.size() - 1) mCurrentEditIndex = 0;
			rtn = mCurrentEditIndex;
		}
		return rtn;
	}*/
	/*void VDMix::createWarp(string wName, unsigned int aFboIndex, unsigned int aShaderIndex, unsigned int bFboIndex, unsigned int bShaderIndex, float xFade) {
		int newIndex = mMixFbos.size();
		// ensure bounds are valid
		aFboIndex = math<int>::min(aFboIndex, getFboListSize() - 1);
		bFboIndex = math<int>::min(bFboIndex, getFboListSize() - 1);

		aShaderIndex = math<int>::min(aShaderIndex, getShadersCount() - 1);
		bShaderIndex = math<int>::min(bShaderIndex, getShadersCount() - 1);
		mMixFbos[newIndex].fbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
		mMixFbos[newIndex].texture = gl::Texture2d::create(mVDParams->getFboWidth(), mVDParams->getFboHeight());
		mMixFbos[newIndex].name = wName;

		mWarpList.push_back(WarpBilinear::create());// 20170103 was WarpPerspectiveBilinear
		Warp::handleResize(mWarpList);
		Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), mVDParams->getFboHeight())); // create small new warps
		Warp::handleResize(mWarpList);

		//int i = mWarpList.size() - 1; // must have at least 1 warp!
		//mWarpList[newIndex]->ABCrossfade = xFade;
		mWarpList[newIndex]->setAFboIndex(aFboIndex);
		mWarpList[newIndex]->setAShaderIndex(aShaderIndex);
		mWarpList[newIndex]->setAShaderFilename(mShaderList[aShaderIndex]->getName());
		mWarpList[newIndex]->setBFboIndex(bFboIndex);
		mWarpList[newIndex]->setBShaderIndex(bShaderIndex);
		mWarpList[newIndex]->setBShaderFilename(mShaderList[bShaderIndex]->getName());
		mWarpList[newIndex]->setMixFboIndex(newIndex);
		mWarpList[newIndex]->setName(toString(mWarpList[newIndex]->getMixFboIndex()) + wName);
		updateWarpName(newIndex);
	}
	void VDMix::setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) {
		if (aWarpIndex < mWarpList.size() && aWarpFboIndex < mFboList.size()) {
			mWarpList[aWarpIndex]->setAFboIndex(aWarpFboIndex);
			updateWarpName(aWarpIndex);
		}
	}*/
	/*void VDMix::setWarpBFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) {
		if (aWarpIndex < mWarpList.size() && aWarpFboIndex < mFboList.size()) {
			mWarpList[aWarpIndex]->setBFboIndex(aWarpFboIndex);
			updateWarpName(aWarpIndex);
		}
	}
	unsigned int VDMix::getWarpAShaderIndex(unsigned int aWarpIndex) {
		if (aWarpIndex > mWarpList.size() - 1) aWarpIndex = 0;
		return mWarpList[aWarpIndex]->getAShaderIndex();
	}
	unsigned int VDMix::getWarpBShaderIndex(unsigned int aWarpIndex) {
		if (aWarpIndex > mWarpList.size() - 1) aWarpIndex = 0;
		return mWarpList[aWarpIndex]->getBShaderIndex();
	}
	void VDMix::setWarpAShaderIndex(unsigned int aWarpIndex, unsigned int aWarpShaderIndex) {
		if (aWarpIndex < mWarpList.size() && aWarpShaderIndex < mShaderList.size()) {
			mWarpList[aWarpIndex]->setAShaderIndex(aWarpShaderIndex);
			mWarpList[aWarpIndex]->setAShaderFilename(mShaderList[aWarpShaderIndex]->getName());
			updateWarpName(aWarpShaderIndex);
		}
	}
	void VDMix::setWarpBShaderIndex(unsigned int aWarpIndex, unsigned int aWarpShaderIndex) {
		if (aWarpIndex < mWarpList.size() && aWarpShaderIndex < mShaderList.size()) {
			mWarpList[aWarpIndex]->setBShaderIndex(aWarpShaderIndex);
			mWarpList[aWarpIndex]->setBShaderFilename(mShaderList[aWarpShaderIndex]->getName());
			updateWarpName(aWarpShaderIndex);
		}
	}

	void VDMix::updateWarpName(unsigned int aWarpIndex) {
		if (aWarpIndex < mWarpList.size()) {
			mWarpList[aWarpIndex]->setName(toString(mWarpList[aWarpIndex]->getMixFboIndex()) + mFboList[mWarpList[aWarpIndex]->getAFboIndex()]->getName().substr(0, 5) + "/" + mFboList[mWarpList[aWarpIndex]->getBFboIndex()]->getName().substr(0, 5));
		}
	}*/
	/*void VDMix::save()
	{
		CI_LOG_V("VDMix save: " + mWarpJson.string());

		// save warp settings
		Warp::save(mWarpList, writeFile(mWarpJson));

	}
	void VDMix::load()
	{
		CI_LOG_V("VDMix load: " + mWarpJson.string());

		// load warps
		if (fs::exists(mWarpJson)) mWarpList = Warp::load(loadFile(mWarpJson));
		// create corresponding mMixFbos
		unsigned int m = mMixFbos.size();
		while (mMixFbos.size() < mWarpList.size())
		{
			CI_LOG_V("mMixFbos.size() < mWarpList.size(), we create a new mixFbo");
			mMixFbos[m].fbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
			mMixFbos[m].texture = gl::Texture2d::create(mVDParams->getFboWidth(), mVDParams->getFboHeight());
			mMixFbos[m].name = "new";
			m++;
		}
		// ensure all indexes are valid
		for (auto &warp : mWarpList) {
			// create the fbos and shaders
			// 20180416 warp->setAShaderIndex(createShaderFbo(warp->getAShaderFilename(), 0));
			// 20180416 warp->setBShaderIndex(createShaderFbo(warp->getBShaderFilename(), 1));
			// ensure all indexes are valid
			if (warp->getAFboIndex() > mFboList.size() - 1) warp->setAFboIndex(0);
			if (warp->getBFboIndex() > mFboList.size() - 1) warp->setBFboIndex(0);
			if (warp->getAShaderIndex() > mShaderList.size() - 1) warp->setAShaderIndex(0);
			if (warp->getBShaderIndex() > mShaderList.size() - 1) warp->setBShaderIndex(0);
			if (warp->getMixFboIndex() > mMixFbos.size() - 1) warp->setMixFboIndex(0);
		}
	}*/

	// Render the scene into the FBO
	/*ci::gl::Texture2dRef VDMix::getRenderTexture()
	{
		gl::ScopedFramebuffer fbScp(mRenderFbo);
		gl::clear(Color::black());
		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mRenderFbo->getSize());

		// animate
		if (mWarpAnimationActive) {
			mWarpActiveIndex++;
			if (mWarpActiveIndex > mWarpList.size() - 1) mWarpActiveIndex = 0;
			mSolo = mWarpActiveIndex;
		}
		// if solo then only render this solo warp
		if (mSolo > -1) {
			mWarpList[mSolo]->draw(getMixTexture(mSolo), getMixTexture(mSolo)->getBounds());
		}
		else {
			// iterate over the warps and draw their content
			int i = 0;
			for (auto &warp : mWarpList) {
				//warp->draw(mMixes[0]->getMixTexture(mWarpFboIndex), Area(0, 0, mMixes[0]->getFboTextureWidth(mWarpFboIndex), mMixes[0]->getFboTextureHeight(mWarpFboIndex)));
				// if (warp->isActive()) warp->draw(getMixTexture(i), Area(-30, 44, 640, 480));
				// before 20180326: if (warp->isActive()) warp->draw(getMixTexture(i), getMixTexture(i)->getBounds());
				// 20180326:
				if (warp->isActive()) warp->draw(getMixTexture(i));
				i++;
			}
		}
		mRenderedTexture = mRenderFbo->getColorTexture();
		return mRenderedTexture;
	}
	void VDMix::toggleWarpAnimationActive() {
		// reset solo
		(mWarpAnimationActive) ? mSolo = -1 : mWarpActiveIndex = 0;
		mWarpAnimationActive = !mWarpAnimationActive;
	}

#pragma endregion warps
	void VDMix::renderMix() {
		gl::ScopedFramebuffer scopedFbo(mMixFbos[warpMixToRender].fbo);
		gl::clear(Color::black());
		// render A and B fbos
		mFboList[mWarpList[warpMixToRender]->getAFboIndex()]->getFboTexture();
		mFboList[mWarpList[warpMixToRender]->getBFboIndex()]->getFboTexture();
		// texture binding must be before ScopedGlslProg
		mFboList[mWarpList[warpMixToRender]->getAFboIndex()]->getRenderedTexture()->bind(0);
		mFboList[mWarpList[warpMixToRender]->getBFboIndex()]->getRenderedTexture()->bind(1);
		if (mTextureList.size() > 2) mTextureList[2]->getTexture()->bind(2);
		// TODO 20190606 temp disabled because if FLICKERING!!
		//if (mTextureList.size() > 3) mTextureList[3]->getTexture()->bind(3);
		//if (mTextureList.size() > 4) mTextureList[4]->getTexture()->bind(4);
		//if (mTextureList.size() > 5) mTextureList[5]->getTexture()->bind(5);
		gl::ScopedGlslProg glslScope(mGlslMix);
		mGlslMix->uniform("iCrossfade", mVDAnimation->getUniformValue(mVDUniforms->IXFADE)); // mWarpList[warpMixToRender]->ABCrossfade);

		gl::drawSolidRect(Rectf(0, 0, mMixFbos[mWarpList[warpMixToRender]->getMixFboIndex()].fbo->getWidth(), mMixFbos[mWarpList[warpMixToRender]->getMixFboIndex()].fbo->getHeight()));

		// save to a texture
		mMixFbos[warpMixToRender].texture = mMixFbos[warpMixToRender].fbo->getColorTexture();
		warpMixToRender++;
		if (warpMixToRender >= mWarpList.size()) {
			warpMixToRender = 0;
		}
		if (mSolo > -1) {
			warpMixToRender = mSolo;
			CI_LOG_V(" solo " + toString(mSolo));
		}
	}

	string VDMix::getMixFboName(unsigned int aMixFboIndex) {
		if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = mMixFbos.size() - 1;
		mMixFbos[aMixFboIndex].name = mFboList[mWarpList[aMixFboIndex]->getAFboIndex()]->getShaderName() + "/" + mFboList[mWarpList[aMixFboIndex]->getAFboIndex()]->getShaderName();
		return mMixFbos[aMixFboIndex].name;
	}

	void VDMix::resize() {
		// tell the warps our window has been resized, so they properly scale up or down
		Warp::handleResize(mWarpList);
		mRenderFbo = gl::Fbo::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, fboFmt);
	}
	void VDMix::updateAudio() {
		mTextureList[0]->getTexture();
	}
	void VDMix::update() {
		// update audio texture
		if (mVDSettings->mUseAudio) updateAudio();

		// check if xFade changed
		//if (mVDSettings->xFadeChanged) {
		//	mVDSettings->xFadeChanged = false;
		//	for (auto &warp : mWarpList) {
		//		// create the fbos and shaders
		//		warp->ABCrossfade = mVDSettings->xFade;
		//	}
		//}

		mGlslMix->uniform("iBlendmode", mVDSettings->iBlendmode);
		mGlslMix->uniform("TIME", mVDAnimation->getUniformValue(mVDUniforms->ITIME));
		// was vec3(mVDParams->getFboWidth(), mVDParams->getFboHeight(), 1.0)):
		mGlslMix->uniform("iResolution", vec3(mVDAnimation->getUniformValueByName("iResolutionX"), mVDAnimation->getUniformValueByName("iResolutionY"), 1.0));
		//mGlslMix->uniform("iChannelResolution", mVDSettings->iChannelResolution, 4);
		// 20180318 mGlslMix->uniform("iMouse", mVDAnimation->getVec4UniformValueByName("iMouse"));
		mGlslMix->uniform("iMouse", vec3(mVDAnimation->getUniformValue(mVDUniforms->IMOUSEX), mVDAnimation->getUniformValue(mVDUniforms->IMOUSEY), mVDAnimation->getUniformValue(mVDUniforms->IMOUSEZ)));
		mGlslMix->uniform("iDate", mVDAnimation->getVec4UniformValueByName("iDate"));
		//mGlslMix->uniform("iWeight0", mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0)); // fbo mix
		//mGlslMix->uniform("iWeight1", mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT1)); // texture
		//mGlslMix->uniform("iWeight2", mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT2)); // texture
		//mGlslMix->uniform("iWeight3", mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT3)); // texture
		//mGlslMix->uniform("iWeight4", mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT4)); // texture
		mGlslMix->uniform("iWeight0", mVDAnimation->getUniformValueByName("iWeight0")); // fbo mix
		mGlslMix->uniform("iWeight1", mVDAnimation->getUniformValueByName("iWeight1")); // texture
		mGlslMix->uniform("iWeight2", mVDAnimation->getUniformValueByName("iWeight2")); // texture
		mGlslMix->uniform("iWeight3", mVDAnimation->getUniformValueByName("iWeight3")); // texture
		mGlslMix->uniform("iWeight4", mVDAnimation->getUniformValueByName("iWeight4")); // texture
		mGlslMix->uniform("iChannel0", 0); // fbo shader
		mGlslMix->uniform("iChannel1", 1); // fbo shader
		mGlslMix->uniform("iChannel2", 2); // texture 1
		mGlslMix->uniform("iChannel3", 3); // texture 2
		mGlslMix->uniform("iChannel4", 4); // texture 3
		mGlslMix->uniform("iRatio", mVDAnimation->getUniformValueByName("iRatio"));
		mGlslMix->uniform("iRenderXY", mVDSettings->mRenderXY);
		mGlslMix->uniform("iZoom", mVDAnimation->getUniformValueByName("iZoom"));
		mGlslMix->uniform("iAlpha", mVDAnimation->getUniformValue(mVDUniforms->IFA) * mVDSettings->iAlpha);
		mGlslMix->uniform("iChromatic", mVDAnimation->getUniformValueByName("iChromatic"));
		mGlslMix->uniform("iRotationSpeed", mVDAnimation->getUniformValue(9));
		mGlslMix->uniform("iCrossfade", mVDAnimation->getUniformValueByName("iCrossfade")); //mWarpList[warpMixToRender]->ABCrossfade);
		mGlslMix->uniform("iPixelate", mVDAnimation->getUniformValueByName("iPixelate"));
		mGlslMix->uniform("iExposure", mVDAnimation->getUniformValueByName("iExposure"));
		mGlslMix->uniform("iToggle", (int)mVDAnimation->getBoolUniformValueByName("iToggle"));
		mGlslMix->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
		mGlslMix->uniform("iBackgroundColor", mVDAnimation->getVec3UniformValueByName("iBackgroundColor"));// vec3(mVDAnimation->getUniformValue(5), mVDAnimation->getUniformValue(6), mVDAnimation->getUniformValue(7)));
		mGlslMix->uniform("iVignette", (int)mVDAnimation->getBoolUniformValueByName("iVignette"));
		mGlslMix->uniform("iVAmount", mVDAnimation->getUniformValueByName("iVAmount"));
		mGlslMix->uniform("iVFallOff", mVDAnimation->getUniformValueByName("iVFallOff"));
		mGlslMix->uniform("iInvert", (int)mVDAnimation->getBoolUniformValueByName("iInvert"));
		mGlslMix->uniform("iTempoTime", mVDAnimation->getUniformValueByName("iTempoTime"));
		mGlslMix->uniform("iGlitch", (int)mVDAnimation->getBoolUniformValueByName("iGlitch"));
		mGlslMix->uniform("iTrixels", mVDAnimation->getUniformValueByName("iTrixels"));
		mGlslMix->uniform("iRedMultiplier", mVDAnimation->getUniformValueByName("iRedMultiplier"));
		mGlslMix->uniform("iGreenMultiplier", mVDAnimation->getUniformValueByName("iGreenMultiplier"));
		mGlslMix->uniform("iBlueMultiplier", mVDAnimation->getUniformValueByName("iBlueMultiplier"));
		mGlslMix->uniform("iFlipV", (int)mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPV));
		mGlslMix->uniform("iFlipH", (int)mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPH));
		mGlslMix->uniform("iPixelX", mVDAnimation->getUniformValueByName("iPixelX"));
		mGlslMix->uniform("iPixelY", mVDAnimation->getUniformValueByName("iPixelY"));
		mGlslMix->uniform("iXorY", mVDSettings->iXorY);
		mGlslMix->uniform("iBadTv", mVDAnimation->getUniformValueByName("iBadTv"));
		mGlslMix->uniform("iFps", mVDAnimation->getUniformValue(mVDUniforms->IFPS));
		mGlslMix->uniform("iContour", mVDAnimation->getUniformValueByName("iContour"));
		mGlslMix->uniform("iSobel", mVDAnimation->getUniformValueByName("iSobel"));
		renderMix();

		// blendmodes preview
		if (mVDAnimation->renderBlend()) {
			mCurrentBlend = getElapsedFrames() % mVDAnimation->getBlendModesCount();
			mGlslBlend->uniform("iBlendmode", mCurrentBlend);
			mGlslBlend->uniform("TIME", mVDAnimation->getUniformValue(mVDUniforms->ITIME));
			mGlslBlend->uniform("iResolution", vec3(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight(), 1.0));
			//mGlslBlend->uniform("iChannelResolution", mVDSettings->iChannelResolution, 4);
			// 20180318 mGlslBlend->uniform("iMouse", mVDAnimation->getVec4UniformValueByName("iMouse"));
			mGlslBlend->uniform("iWeight0", mVDAnimation->getUniformValueByName("iWeight0"));
			mGlslBlend->uniform("iWeight1", mVDAnimation->getUniformValueByName("iWeight1"));
			mGlslBlend->uniform("iWeight2", mVDAnimation->getUniformValueByName("iWeight2"));
			mGlslBlend->uniform("iWeight3", mVDAnimation->getUniformValueByName("iWeight3"));
			mGlslBlend->uniform("iWeight4", mVDAnimation->getUniformValueByName("iWeight4"));
			mGlslBlend->uniform("iMouse", vec3(mVDAnimation->getUniformValue(mVDUniforms->IMOUSEX), mVDAnimation->getUniformValue(mVDUniforms->IMOUSEY), mVDAnimation->getUniformValue(mVDUniforms->IMOUSEZ)));
			mGlslBlend->uniform("iDate", mVDAnimation->getVec4UniformValueByName("iDate"));
			mGlslBlend->uniform("iChannel0", 0); // fbo shader
			mGlslBlend->uniform("iChannel1", 1); // fbo shader
			mGlslBlend->uniform("iChannel2", 2); // texture 1
			mGlslBlend->uniform("iChannel3", 3); // texture 2
			mGlslBlend->uniform("iChannel4", 4); // texture 3
			mGlslBlend->uniform("iAudio0", 0);
			mGlslBlend->uniform("iFreq0", mVDAnimation->getUniformValueByName("iFreq0"));
			mGlslBlend->uniform("iFreq1", mVDAnimation->getUniformValueByName("iFreq1"));
			mGlslBlend->uniform("iFreq2", mVDAnimation->getUniformValueByName("iFreq2"));
			mGlslBlend->uniform("iFreq3", mVDAnimation->getUniformValueByName("iFreq3"));
			mGlslBlend->uniform("iChannelTime", mVDSettings->iChannelTime, 4);
			mGlslBlend->uniform("iColor", vec3(mVDAnimation->getUniformValue(mVDUniforms->IFR), mVDAnimation->getUniformValue(mVDUniforms->IFG), mVDAnimation->getUniformValue(mVDUniforms->IFB)));
			mGlslBlend->uniform("iBackgroundColor", mVDAnimation->getVec3UniformValueByName("iBackgroundColor"));//vec3(mVDAnimation->getUniformValue(5), mVDAnimation->getUniformValue(6), mVDAnimation->getUniformValue(7)));
			mGlslBlend->uniform("iSteps", (int)mVDAnimation->getUniformValueByName("iSteps"));
			mGlslBlend->uniform("iRatio", mVDAnimation->getUniformValueByName("iRatio"));
			mGlslBlend->uniform("width", 1);
			mGlslBlend->uniform("height", 1);
			mGlslBlend->uniform("iRenderXY", mVDSettings->mRenderXY);
			mGlslBlend->uniform("iZoom", mVDAnimation->getUniformValue(12));
			mGlslBlend->uniform("iAlpha", mVDAnimation->getUniformValue(mVDUniforms->IFA) * mVDSettings->iAlpha);
			mGlslBlend->uniform("iChromatic", mVDAnimation->getUniformValueByName("iChromatic"));
			mGlslBlend->uniform("iRotationSpeed", mVDAnimation->getUniformValue(9));
			mGlslBlend->uniform("iCrossfade", 0.5f);// blendmode only work if different than 0 or 1.0 mVDAnimation->getUniformValue( mVDUniforms->IXFADE]);
			mGlslBlend->uniform("iPixelate", mVDAnimation->getUniformValueByName("iPixelate"));
			mGlslBlend->uniform("iExposure", mVDAnimation->getUniformValueByName("iExposure"));
			mGlslBlend->uniform("iDeltaTime", mVDAnimation->getUniformValue(mVDUniforms->IDELTATIME));
			mGlslBlend->uniform("iFade", (int)mVDSettings->iFade);
			mGlslBlend->uniform("iToggle", (int)mVDAnimation->getBoolUniformValueByName("iToggle"));
			mGlslBlend->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
			mGlslBlend->uniform("iTransition", mVDSettings->iTransition);
			mGlslBlend->uniform("iAnim", mVDSettings->iAnim.value());
			mGlslBlend->uniform("iRepeat", (int)mVDSettings->iRepeat);
			mGlslBlend->uniform("iVignette", (int)mVDAnimation->getBoolUniformValueByName("iVignette"));
			mGlslBlend->uniform("iVAmount", mVDAnimation->getUniformValueByName("iVAmount"));
			mGlslBlend->uniform("iVFallOff", mVDAnimation->getUniformValueByName("iVFallOff"));
			mGlslBlend->uniform("iInvert", (int)mVDAnimation->getBoolUniformValueByName("iInvert"));
			mGlslBlend->uniform("iDebug", (int)mVDSettings->iDebug);
			mGlslBlend->uniform("iShowFps", (int)mVDSettings->iShowFps);
			mGlslBlend->uniform("iFps", mVDAnimation->getUniformValue(mVDUniforms->IFPS));
			mGlslBlend->uniform("iTempoTime", mVDAnimation->getUniformValueByName("iTempoTime"));
			mGlslBlend->uniform("iGlitch", (int)mVDAnimation->getBoolUniformValueByName("iGlitch"));
			mGlslBlend->uniform("iTrixels", mVDAnimation->getUniformValueByName("iTrixels"));
			mGlslBlend->uniform("iSeed", mVDSettings->iSeed);
			mGlslBlend->uniform("iRedMultiplier", mVDAnimation->getUniformValueByName("iRedMultiplier"));
			mGlslBlend->uniform("iGreenMultiplier", mVDAnimation->getUniformValueByName("iGreenMultiplier"));
			mGlslBlend->uniform("iBlueMultiplier", mVDAnimation->getUniformValueByName("iBlueMultiplier"));
			mGlslBlend->uniform("iFlipH", (int)mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPH));
			mGlslBlend->uniform("iFlipV", (int)mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPV));
			mGlslBlend->uniform("iPixelX", mVDAnimation->getUniformValueByName("IPIXELX"));
			mGlslBlend->uniform("iPixelY", mVDAnimation->getUniformValueByName("IPIXELY"));
			mGlslBlend->uniform("iXorY", mVDSettings->iXorY);
			mGlslBlend->uniform("iBadTv", mVDAnimation->getUniformValueByName("iBadTv"));
			mGlslBlend->uniform("iContour", mVDAnimation->getUniformValueByName("iContour"));
			mGlslBlend->uniform("iSobel", mVDAnimation->getUniformValueByName("iSobel"));
			renderBlend();
		}
	}*/
#pragma region events
	/*
		bool VDMix::handleMouseMove(MouseEvent &event)
		{
			bool handled = true;
			// pass this mouse event to the warp editor first
			if (!Warp::handleMouseMove(mWarpList, event)) {
				// let your application perform its mouseMove handling here
				handled = false;
			}
			event.setHandled(handled);
			return event.isHandled();
		}

		bool VDMix::handleMouseDown(MouseEvent &event)
		{
			bool handled = true;
			// pass this mouse event to the warp editor first
			if (!Warp::handleMouseDown(mWarpList, event)) {
				// let your application perform its mouseDown handling here
				handled = false;
			}
			event.setHandled(handled);
			return event.isHandled();
		}
		bool VDMix::handleMouseDrag(MouseEvent &event)
		{
			bool handled = true;
			// pass this mouse event to the warp editor first
			if (!Warp::handleMouseDrag(mWarpList, event)) {
				// let your application perform its mouseDrag handling here
				handled = false;
			}
			event.setHandled(handled);
			return event.isHandled();
		}
		bool VDMix::handleMouseUp(MouseEvent &event)
		{
			bool handled = true;
			// pass this mouse event to the warp editor first
			if (!Warp::handleMouseUp(mWarpList, event)) {
				// let your application perform its mouseUp handling here
				handled = false;
			}
			event.setHandled(handled);
			return event.isHandled();
		}

		bool VDMix::handleKeyDown(KeyEvent &event)
		{
			bool handled = true;
	#if defined( CINDER_COCOA )
			bool isModDown = event.isMetaDown();
	#else // windows
			bool isModDown = event.isControlDown();
	#endif
			if (isModDown) {
				switch (event.getCode()) {
				case KeyEvent::KEY_s:
					fileWarpsName = "warps" + toString(getElapsedFrames()) + ".json";
					Warp::save(mWarpList, writeFile(mWarpJson));
					break;
				case KeyEvent::KEY_x:
					createWarp();
					// TODO? Warp::handleResize(mWarpList);
					break;
				default:
					handled = false;
					break;
				}
			}
			else {
				// pass this key event to the warp editor first
				if (!Warp::handleKeyDown(mWarpList, event)) {
					handled = false;
				}
			}
			event.setHandled(handled);
			return event.isHandled();
		}
		bool VDMix::handleKeyUp(KeyEvent &event)
		{
			bool handled = true;
			// pass this key event to the warp editor first
			if (!Warp::handleKeyUp(mWarpList, event)) {
				handled = false;
			}
			event.setHandled(handled);
			return event.isHandled();
		}*/
#pragma endregion events
		/*void VDMix::setFboInputTexture(unsigned int aFboIndex, unsigned int aInputTextureIndex) {
			if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
			if (aInputTextureIndex > mTextureList.size() - 1) aInputTextureIndex = mTextureList.size() - 1;
			mFboList[aFboIndex]->setInputTexture(mTextureList, aInputTextureIndex);
		}
		unsigned int VDMix::getFboInputTextureIndex(unsigned int aFboIndex) {
			if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
			return mFboList[aFboIndex]->getInputTextureIndex();
		}
		void VDMix::initShaderList() {


		}
		bool VDMix::initTextureList() {
			bool isFirstLaunch = false;
			if (mTextureList.size() == 0) {
				CI_LOG_V("VDSession::init mTextureList");
				isFirstLaunch = true;
				// add an audio texture as first texture
				TextureAudioRef t(new TextureAudio(mVDAnimation));

				// add texture xml
				XmlTree			textureXml;
				textureXml.setTag("texture");
				textureXml.setAttribute("id", "0");
				textureXml.setAttribute("texturetype", "audio");

				t->fromXml(textureXml);
				mTextureList.push_back(t);
				// then read textures.xml
				if (fs::exists(mTexturesFilepath)) {
					// load textures from file if one exists
					//mTextureList = VDTexture::readSettings(mVDAnimation, loadFile(mTexturesFilepath));
					XmlTree			doc;
					try { doc = XmlTree(loadFile(mTexturesFilepath)); }
					catch (...) { CI_LOG_V("could not load textures.xml"); }
					if (doc.hasChild("textures")) {
						XmlTree xml = doc.getChild("textures");
						for (XmlTree::ConstIter textureChild = xml.begin("texture"); textureChild != xml.end(); ++textureChild) {
							CI_LOG_V("texture ");

							string texturetype = textureChild->getAttributeValue<string>("texturetype", "unknown");
							CI_LOG_V("texturetype " + texturetype);
							XmlTree detailsXml = textureChild->getChild("details");
							// read or add the assets path
							string mFolder = detailsXml.getAttributeValue<string>("folder", "");
							if (mFolder.length() == 0) detailsXml.setAttribute("folder", mVDSettings->mAssetsPath);
							// create the texture
							if (texturetype == "image") {
								TextureImageRef t(TextureImage::create());
								t->fromXml(detailsXml);
								mTextureList.push_back(t);
							}
							else if (texturetype == "imagesequence") {
								TextureImageSequenceRef t(new TextureImageSequence(mVDAnimation));
								t->fromXml(detailsXml);
								mTextureList.push_back(t);
							}
							else if (texturetype == "movie") {
	#if defined( CINDER_MSW )
								TextureMovieRef t(new TextureMovie());
								t->fromXml(detailsXml);
								mTextureList.push_back(t);
	#endif
							}
							else if (texturetype == "camera") {
	#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
								TextureCameraRef t(new TextureCamera());
								t->fromXml(detailsXml);
								mTextureList.push_back(t);
	#else
								// camera not supported on this platform
								CI_LOG_V("camera not supported on this platform");
								XmlTree		xml;
								xml.setTag("details");
								xml.setAttribute("path", "0.jpg");
								xml.setAttribute("width", 640);
								xml.setAttribute("height", 480);
								t->fromXml(xml);
								mTextureList.push_back(t);
	#endif
							}
							else if (texturetype == "shared") {
								// TODO CHECK USELESS? #if defined( CINDER_MSW )
								// disabled for SpoutIn/Out problem in same app
								//TextureSharedRef t(new TextureShared());
								//t->fromXml(detailsXml);
								//mTextureList.push_back(t);
								// TODO texturetype = textureChild->setAttribute("texturetype", "image");
								TextureImageRef t(TextureImage::create());
								t->fromXml(detailsXml);
								mTextureList.push_back(t);
								//#endif
							}
							else if (texturetype == "audio") {
								// audio texture done in initTextures
							}
							else if (texturetype == "stream") {
								// stream texture done when websocket texture received
							}
							else {
								// unknown texture type
								CI_LOG_V("unknown texture type");
								TextureImageRef t(new TextureImage());
								XmlTree		xml;
								xml.setTag("details");
								xml.setAttribute("path", "0.jpg");
								xml.setAttribute("width", mVDParams->getFboWidth());
								xml.setAttribute("height", mVDParams->getFboHeight());
								t->fromXml(xml);
								mTextureList.push_back(t);
							}
						}
					}
				}
			}
			return isFirstLaunch;
		}
		void VDMix::fboFlipV(unsigned int aFboIndex) {
			if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
			mFboList[aFboIndex]->flipV();
		}
		bool VDMix::isFboFlipV(unsigned int aFboIndex) {
			if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
			return mFboList[aFboIndex]->isFlipV();
		}
		void VDMix::setFboFragmentShaderIndex(unsigned int aFboIndex, unsigned int aFboShaderIndex) {
			CI_LOG_V("setFboFragmentShaderIndex, before, fboIndex: " + toString(aFboIndex) + " shaderIndex " + toString(aFboShaderIndex));
			if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
			if (aFboShaderIndex > mShaderList.size() - 1) aFboShaderIndex = mShaderList.size() - 1;
			CI_LOG_V("setFboFragmentShaderIndex, after, fboIndex: " + toString(aFboIndex) + " shaderIndex " + toString(aFboShaderIndex));
			mFboList[aFboIndex]->setFragmentShader(aFboShaderIndex, mShaderList[aFboShaderIndex]->getFragmentString(), mShaderList[aFboShaderIndex]->getName());
			// route message
			// LOOP! mVDSocketio->changeFragmentShader(mShaderList[aFboShaderIndex]->getFragmentString());
		}
		unsigned int VDMix::getFboFragmentShaderIndex(unsigned int aFboIndex) {
			unsigned int rtn = mFboList[aFboIndex]->getShaderIndex();
			//CI_LOG_V("getFboFragmentShaderIndex, fboIndex: " + toString(aFboIndex)+" shaderIndex: " + toString(rtn));
			if (rtn > mShaderList.size() - 1) rtn = mShaderList.size() - 1;
			return rtn;
		}*/

		// shaders
		/*void VDMix::updateShaderThumbFile(unsigned int aShaderIndex) {
			for (int i {0}; i < mFboList.size(); i++)
			{
				if (mFboList[i]->getShaderIndex() == aShaderIndex) mFboList[i]->updateThumbFile();
			}
		}
		void VDMix::removeShader(unsigned int aShaderIndex) {
			if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
			mShaderList[aShaderIndex]->removeShader();
		}

		unsigned int VDMix::createShaderFboFromString(const string& aFragmentShaderString, const string& aShaderFilename, const string& aName) {
			unsigned int rtn = 0;
			unsigned int shaderId = 0;
			// create new shader
			VDShaderRef s(new VDShader(mVDSettings, mVDAnimation, aShaderFilename, aFragmentShaderString, aName));
			if (s->isValid()) {
				mShaderList.push_back(s);
				shaderId = mShaderList.size() - 1;
				// each shader element has a fbo
				VDFboRef f(new VDFbo(mVDSettings, mVDAnimation));
				// create fbo xml
				XmlTree			fboXml;
				fboXml.setTag(aShaderFilename);
				fboXml.setAttribute("id", shaderId);
				fboXml.setAttribute("width", mVDParams->getFboWidth());
				fboXml.setAttribute("height", mVDParams->getFboHeight());
				fboXml.setAttribute("shadername", aName);
				// 20180328 fboXml.setAttribute("inputtextureindex", math<int>::min(rtn, mTextureList.size() - 1));
				fboXml.setAttribute("inputtextureindex", 0);

				f->fromXml(fboXml);
				f->setShaderIndex(shaderId);
				f->setFragmentShader(shaderId, mShaderList[shaderId]->getFragmentString(), aName);
				mFboList.push_back(f);
				rtn = mFboList.size() - 1;
				// 20180322 setFboInputTexture(mFboList.size() - 1, math<int>::min(rtn, mTextureList.size() - 1));
				setFboInputTexture(rtn, 0);
			}
			return rtn;
		}
		string VDMix::getFragmentShaderString(unsigned int aShaderIndex) {
			if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
			return mShaderList[aShaderIndex]->getFragmentString();
		}
		//string VDMix::getVertexShaderString(unsigned int aShaderIndex) {
		//	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
		//	return mShaderList[aShaderIndex]->getVertexString();
		//}
		string VDMix::getShaderName(unsigned int aShaderIndex) {
			if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
			return mShaderList[aShaderIndex]->getName();
		}
		ci::gl::TextureRef VDMix::getShaderThumb(unsigned int aShaderIndex) {
			unsigned int found = 0;
			for (int i {0}; i < mFboList.size(); i++)
			{
				if (mFboList[i]->getShaderIndex() == aShaderIndex) found = i;
			}
			return getFboRenderedTexture(found);
		}
		void VDMix::updateStream(string * aStringPtr) {
			int found = -1;
			for (int i {0}; i < mTextureList.size(); i++)
			{
				if (mTextureList[i]->getType() == mTextureList[i]->STREAM) found = i;
			}
			if (found < 0) {
				// create stream texture
				TextureStreamRef t(new TextureStream(mVDAnimation));
				// add texture xml
				XmlTree			textureXml;
				textureXml.setTag("texture");
				textureXml.setAttribute("id", "9");
				textureXml.setAttribute("texturetype", "stream");
				t->fromXml(textureXml);
				mTextureList.push_back(t);
				found = mTextureList.size() - 1;
			}
			mTextureList[found]->loadFromFullPath(*aStringPtr);
		}
		*/

		/*unsigned int VDMix::createShaderFbo(const string& aShaderFilename, unsigned int aFboShaderIndex) {
			// initialize rtn to 0 to force creation
			unsigned int rtn = 0;
			string fName = aShaderFilename;
			if (aShaderFilename.length() > 0) {
				fs::path mFragFile = getAssetPath("") / mVDSettings->mAssetsPath / aShaderFilename;
				if (!fs::exists(mFragFile)) {
					// if file does not exist it may be a full path
					mFragFile = aShaderFilename;
				}
				if (fs::exists(mFragFile)) {
					if (aFboShaderIndex < mFboList.size()) {
						if (mShaderList[aFboShaderIndex]->loadFragmentStringFromFile(mFragFile.string())) {
							mFboList[aFboShaderIndex]->setFragmentShader(aFboShaderIndex, mShaderList[aFboShaderIndex]->getFragmentString(), mShaderList[aFboShaderIndex]->getName());
							mFboList[aFboShaderIndex]->setInputTextureIndex(0);
							rtn = aFboShaderIndex;
						}
					}
					// check if mShaderList contains a shader
					// if (mShaderList.size() > 0) {
					//	fName = mFragFile.filename().string();
					//	// find a removed shader
					//	for (int i = mShaderList.size() - 1; i > 0; i--)
					//	{
					//		if (!mShaderList[i]->isValid() || fName == mShaderList[i]->getName()) { rtn = i; }
					//	}
					//	// find a not used shader if no removed shader
					//	if (rtn == 0) {
					//		// first reset all shaders (excluding the first 8 ones)
					//		for (int i = mShaderList.size() - 1; i > 8; i--)
					//		{
					//			mShaderList[i]->setActive(false);
					//		}
					//		// set active shaders according to warps
					//		for (auto &warp : mWarpList) {
					//			// mShaderList.size() should be > 0 for this part
					//			if (warp->getAShaderIndex() < mShaderList.size() - 1) mShaderList[warp->getAShaderIndex()]->setActive(true);
					//			if (warp->getBShaderIndex() < mShaderList.size() - 1) mShaderList[warp->getBShaderIndex()]->setActive(true);
					//		}
					//		// find inactive shader index
					//		for (int i = mShaderList.size() - 1; i > 8; i--)
					//		{
					//			if (!mShaderList[i]->isActive()) rtn = i;
					//		}
					//	}
					//}
					//// if we found an available slot
					//if (rtn > 0) {
					//	if (rtn < mFboList.size()) {
					//		if (mShaderList[rtn]->loadFragmentStringFromFile(aShaderFilename)) {
					//			mFboList[rtn]->setFragmentShader(rtn, mShaderList[rtn]->getFragmentString(), mShaderList[rtn]->getName());
					//			mFboList[rtn]->setInputTextureIndex(aInputTextureIndex);
					//		}
					//	}
					//}
					//else {
					//	// no slot available, create new shader
					//	// problem to investigate :
					//	rtn = createShaderFboFromString(loadString(loadFile(mFragFile)), aShaderFilename, fName);
					//}
					//mFboList[rtn]->updateThumbFile();
				}
			}
			return rtn;
		}*/
} // namespace videodromm
