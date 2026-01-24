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

		mMixetteTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(false));
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
			mVDSettings->setErrorMsg(mError);
		}

		mGlslMixette = gl::GlslProg::create(mVDParams->getDefaultVertexString(), loadString(loadFile(mMixetteFilePath)));
	} // constructor

	bool VDMix::save()
	{
		int rfrf;
		/*Json doc;

		Json settings = Json::makeArray("settings");
		settings.addChild(ci::Json("assetspath", mAssetsPath));
		doc.pushBack(settings);
		doc.write(writeFile(mixPath), Json::WriteOptions());*/
		return true;
	}
	

	void VDMix::restore(const fs::path& aFilePath)
	{
		// check to see if json file exists
		if (!fs::exists(aFilePath)) {
			return;
		}
		try {
			const ci::Json json = loadJson( app::loadAsset( aFilePath ) );
			
			if( json.contains( "assetspath" ) && json["assetspath"].is_string() ) {
				mAssetsPath = json["assetspath"].get<std::string>();
				
			}
			for( auto& feature : json["uniforms"] ) {
				const string& name = feature.contains( "name" ) ? feature["name"] : "";
				unsigned int index = feature.contains( "index" ) ? feature["index"] : 0;
				float value  = feature.contains( "value" ) ?  feature["value"] : 0;
				mVDUniforms->setUniformValue( index,  value );
				
			}
		/*Json doc(loadFile(aFilePath));
			if (doc.hasChild("settings")) {
				Json settings(doc.getChild("settings"));
				if (settings.hasChild("assetspath")) mAssetsPath = settings.getValueForKey<string>("assetspath");
			}
			if (doc.hasChild("uniforms")) {
				Json uniforms(doc.getChild("uniforms"));
				for (unsigned int i = 0; i < 100; i++)
				{
					if (uniforms.hasChild(mVDUniforms->getUniformName(i))) {
						mVDUniforms->setUniformValue(i, uniforms.getValueForKey<float>(mVDUniforms->getUniformName(i)));
					}
				}
			}
			
*/
			
		}
		catch (const ci::Exception& exc) { // Json::exception& exc
			CI_LOG_W(exc.what());
		}
	}
	unsigned int VDMix::getValidFboIndex(unsigned int aFboIndex) {
		return math<int>::min(aFboIndex, (unsigned int)mFboShaderList.size() - 1);
	}
	
	unsigned int VDMix::createFboShaderTexture(const Json &json, unsigned int aFboIndex, const std::string& aFolder) {
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
	std::vector<ci::gl::GlslProg::Uniform> VDMix::getFboShaderUniforms(unsigned int aFboShaderIndex) {
		return mFboShaderList[aFboShaderIndex]->getUniforms();
	}

	float VDMix::getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex) {
		return mFboShaderList[aFboShaderIndex]->getUniformValueByLocation(aLocationIndex);
	};
	void VDMix::setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue) {
		mFboShaderList[aFboShaderIndex]->setUniformValueByLocation(aLocationIndex, aValue);
	};

	unsigned int VDMix::findAvailableIndex(unsigned int aFboShaderIndex, const Json &json) {
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
		int rfrf;
		// received from websocket, tested with hydra
		//Json		json;
		//Json shader = ci::Json::makeArray("shader");
		//shader.addChild(ci::Json("shadername", aName));
		//shader.pushBack(ci::Json("shadertype", "fs"));
		//shader.pushBack(ci::Json("shadertext", aFragmentShaderString));
		//json.addChild(shader);
		//Json texture = ci::Json::makeArray("texture");
		//texture.addChild(ci::Json("texturename", "audio"));
		//texture.pushBack(ci::Json("texturetype", "audio"));
		//texture.pushBack(ci::Json("texturemode", VDTextureMode::AUDIO));
		//json.addChild(texture);
		//int rtn = findAvailableIndex(aFboShaderIndex, json); // 20240518 was 0
		//mFboShaderList[rtn]->setFragmentShaderString(aFragmentShaderString, aName);
		return 1; // rtn;
	}
	int VDMix::loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex) {
		int rfrf;
		const ci::Json json
			= { { "shader", { { "shadername", "todo.txt" }, { "shadertype", "fs" }, { "shadertext", "todo" } } }, { "texture", { { "texturename", "audio" }, { "texturetype", "audio" }, { "texturemode", VDTextureMode::AUDIO } } }

				  };
		
		//Json shader = ci::Json::makeArray("shader");
		//shader.addChild(ci::Json("shadername", "todo.txt"));
		//shader.pushBack(ci::Json("shadertype", "fs"));
		//shader.pushBack(ci::Json("shadertext", "todo"));
		//json.addChild(shader);
		//Json texture = ci::Json::makeArray("texture");
		//texture.addChild(ci::Json("texturename", "audio"));
		//texture.pushBack(ci::Json("texturetype", "audio"));
		//texture.pushBack(ci::Json("texturemode", VDTextureMode::AUDIO));
		//json.addChild(texture);

		// if aFboShaderIndex is out of bounds try to find invalid fbo index or create a new fbo until MAX
		int rtn = findAvailableIndex(aFboShaderIndex, json);

		mFboShaderList[rtn]->loadFragmentShaderFromFile(aFilePath, true);//isAudio=true for dnd glsl files
		mVDSettings->setMsg("loaded " + mFboShaderList[rtn]->getShaderName() + "\n try at " + toString(aFboShaderIndex) + " valid at " + toString(rtn));
		return rtn;
	}
	ci::gl::TextureRef VDMix::getMixetteTexture(unsigned int aFboIndex) {

		gl::ScopedFramebuffer fbScp(mMixetteFbo);
		// clear out the FBO with black
		gl::clear(Color::black());

		// nasty bug! bind to 100+f
		/* int f = 0;
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
		new test:*/
		/* done in next for loop:
		int f = 0;
		for (auto &fbo : mFboShaderList) {
			if (mFboShaderList[f]->isValid()) {// white mix bug && mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0 + f) > 0.05f) {
				//fbo->getTexture()->bind(f); not in right order
				mFboShaderList[f]->getTexture()->bind(100 + f);
			}
			f++;
		}*/
		gl::ScopedGlslProg prog(mGlslMixette);
		mGlslMixette->uniform("iResolution", vec3(mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONX), mVDUniforms->getUniformValue(mVDUniforms->IRESOLUTIONY), 1.0));
		mGlslMixette->uniform("iBlendmode", (int)mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE));
		int i = 0;
		for (auto &fbo : mFboShaderList) {
			if (fbo->isValid()) {// white mix bug
				if (mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + i) > 0.01f) mFboShaderList[i]->getTexture()->bind(100 + i);
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
	void VDMix::loadImageFile(const std::string& aFile, unsigned int aFboIndex) {
		int rtn = math<int>::min(aFboIndex, mFboShaderList.size() - 1);
		fs::path texFileOrPath = aFile;
		if (fs::exists(texFileOrPath)) {

			std::string ext = "";
			int dotIndex = texFileOrPath.filename().string().find_last_of(".");
			if (dotIndex != std::string::npos)  ext = texFileOrPath.filename().string().substr(dotIndex + 1);
			//if (ext == "jpg" || ext == "png") {
			//	
			//		// no fbos, create one
			//		Json		json;
			//		Json texture = ci::Json::makeArray("texture");
			//		texture.addChild(ci::Json("texturename", aFile));
			//		texture.pushBack(ci::Json("texturetype", "image"));
			//		texture.pushBack(ci::Json("texturemode", 1));
			//		texture.pushBack(ci::Json("texturecount", 1));
			//		json.addChild(texture);
			//		Json shader = ci::Json::makeArray("shader");
			//		shader.addChild(ci::Json("shadername", "inputImage.fs"));
			//		shader.pushBack(ci::Json("shadertype", "fs"));
			//		json.addChild(shader);
			//		createFboShaderTexture(json, aFboIndex);
			//		
			//}
		}
	}
	void VDMix::loadVideoFile(const std::string& aFile, unsigned int aFboIndex) {
		int rtn = math<int>::min(aFboIndex, mFboShaderList.size() - 1);
		fs::path texFileOrPath = aFile;
		if (fs::exists(texFileOrPath)) {

			std::string ext = "";
			int dotIndex = texFileOrPath.filename().string().find_last_of(".");
			if (dotIndex != std::string::npos)  ext = texFileOrPath.filename().string().substr(dotIndex + 1);
			//if (ext == "mp4") {
			//	// 20220321  tmp if (mFboShaderList.size() < 1) {
			//		// no fbos, create one
			//		Json		json;
			//		Json texture = ci::Json::makeArray("texture");
			//		texture.addChild(ci::Json("texturename", aFile));
			//		texture.pushBack(ci::Json("texturetype", "video"));
			//		texture.pushBack(ci::Json("texturemode", 3));
			//		texture.pushBack(ci::Json("texturecount", 1));
			//		json.addChild(texture);
			//		Json shader = ci::Json::makeArray("shader");
			//		shader.addChild(ci::Json("shadername", "inputVideo.fs"));
			//		shader.pushBack(ci::Json("shadertype", "fs"));
			//		json.addChild(shader);
			//		createFboShaderTexture(json, aFboIndex);
			//		
			//}
		}
	}

#pragma endregion textures

} // namespace videodromm
