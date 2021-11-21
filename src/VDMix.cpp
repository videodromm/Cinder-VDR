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
		// TODO TMP mDefaultTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
		// check to see if mix.json file exists and restore if it does
		// must have one texture initialized
		TextureAudioRef t(TextureAudio::create(mVDAnimation));
		mTextureList.push_back(t);

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

		//mDefaultTexture = ci::gl::Texture::create(loadImage(loadAsset("0.jpg")));
		//loadImageFile("0.jpg", 0);

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

		loadFbos();
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
	void VDMix::selectSenderPanel() {
		if (ts) {
			ts->getMaxFrame(); // TODO 20210130 rename or create new fct
		}
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
			if (doc.hasChild("camera")) {
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
			}
		}
		catch (const JsonTree::ExcJsonParserError& exc) {
			CI_LOG_W(exc.what());
		}
	}
	unsigned int VDMix::getValidFboIndex(unsigned int aFboIndex) {
		return math<int>::min(aFboIndex, (unsigned int)mFboShaderList.size() - 1);
	}
	unsigned int VDMix::getValidTexIndex(unsigned int aTexIndex) {
		return math<int>::min(aTexIndex, (unsigned int)mTextureList.size() - 1);
	}
	void VDMix::loadFbos() {
		/* done better in Session
				// 20211107 TODO add default fboshader if mFboShaderList empty? init shader
				JsonTree json;
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
				mFboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, 0, mAssetsPath);
				mFboShaderList.push_back(mFboShader);
				setFboInputTexture(getFboShaderListSize() - 1, 0);
		*/
	}
	/*unsigned int VDMix::fboFromJson(const JsonTree& json, unsigned int aFboIndex) {
		unsigned int rtn = 0;

		rtn = createFboShaderTexture(json, aFboIndex);
		return rtn;
	}*/
	unsigned int VDMix::createFboShaderTexture(const JsonTree &json, unsigned int aFboIndex, const std::string& aFolder) {
		unsigned int rtn = 0;
		if (aFolder != "") mAssetsPath = aFolder;
		VDFboShaderRef fboShader = VDFboShader::create(mVDUniforms, mVDAnimation, json, aFboIndex, mAssetsPath);
		if (mFboShaderList.size() == 0) {
			mFboShaderList.push_back(fboShader);
			rtn = (unsigned int)mFboShaderList.size() - 1;
		}
		else if (aFboIndex < mFboShaderList.size() - 1) {
			rtn = aFboIndex;
			mFboShaderList[rtn] = fboShader;
		}
		else {
			mFboShaderList.push_back(fboShader);
			rtn = (unsigned int)mFboShaderList.size() - 1;
		}
		/*if (aFboIndex < 6) {// 20211120 == 0) {
			mFboShaderList.push_back(fboShader);
			rtn = (unsigned int)mFboShaderList.size() - 1;
		}
		else {
			rtn = getValidFboIndex(aFboIndex);
			mFboShaderList[rtn] = fboShader;
		}*/

		return rtn;
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
		mGlslMixette->uniform("iBlendmode", (int)mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE));
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

#pragma endregion textures



} // namespace videodromm
