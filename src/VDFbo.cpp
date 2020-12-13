#include "VDFbo.h"

using namespace videodromm;

//namespace videodromm {
	VDFbo::VDFbo(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, const JsonTree& json)
		:mVDSettings{ aVDSettings },
		mVDUniforms{ aVDUniforms },
		mVDAnimation{ aVDAnimation },
		mValid(false)
	{
		CI_LOG_V("VDFbo constructor");
		// Params
		mVDParams = VDParams::create();

		std::string shaderFileName = "inputImage.fs";
		mShaderName = mShaderFileName;
		std::string shaderType = "fs";
		mShaderFragmentString = "";
		//string textureFileName = "0.jpg"; 
		//mTextureName = mCurrentSeqFilename = mLastCachedFilename = textureFileName;
		mInputTextureIndex = 0;

		if (json.hasChild("shader")) {
			JsonTree shaderJsonTree(json.getChild("shader"));
			mShaderName = mShaderFileName = (shaderJsonTree.hasChild("shadername")) ? shaderJsonTree.getValueForKey<std::string>("shadername") : "inputImage.fs";
			mShaderFragmentString = (shaderJsonTree.hasChild("shadertext")) ? shaderJsonTree.getValueForKey<std::string>("shadertext") : "";
			shaderType = (json.hasChild("shadertype")) ? json.getValueForKey<std::string>("shadertype") : "fs";
		}
		if (json.hasChild("texture")) {

			JsonTree textureJsonTree(json.getChild("texture"));
			//tmp
			//string tx = (textureJsonTree.hasChild("texturename")) ? textureJsonTree.getValueForKey<string>("texturename") : "0.jpg";

			createInputTexture(textureJsonTree);
		}

		//mShaderName = mShaderFileName = aShaderFilename;
		//mTextureName =  mCurrentSeqFilename = mLastCachedFilename = aTextureFilename;
		shaderInclude = loadString(loadAsset("shadertoy.vd"));

		// init texture
		//mTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
		mRenderedTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
		isReady = false;

		// init texture
		// init the fbo whatever happens next
		fboFmt.setColorTextureFormat(fmt);
		mFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
		mError = "";
		mActive = true;
		mValid = loadFragmentStringFromFile(mShaderName);

		if (mValid) {
			CI_LOG_V("VDFbo constructor success");
		}
		else {
			mVDSettings->mErrorMsg = "VDFbo constructor failed\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
		}
	}
	VDFbo::~VDFbo(void) {
	}

	bool VDFbo::loadFragmentStringFromFile(const std::string& aFileName) {
		mValid = false;

		if (aFileName.length() > 0) {
			/*if (mType == MOVIE) {
				try {
					mShaderName = mShaderFileName = "video_texture.fs.glsl";
					// TODO VDShaderRef shaderToLoad instance
					mShader = gl::GlslProg::create(gl::GlslProg::Format()
						.vertex(loadAsset("video_texture.vs.glsl"))
						.fragment(loadAsset("video_texture.fs.glsl")));
					mValid = true;
					CI_LOG_V("fbo video_texture vtx-frag compiled");
				}
				catch (gl::GlslProgCompileExc &exc) {
					mError = string(exc.what());
					CI_LOG_V("fbo unable to load/compile vtx-frag video_texture shader:" + string(exc.what()));
				}
				catch (const std::exception &e) {
					mError = string(e.what());
					CI_LOG_V("fbo unable to load vtx-frag video_texture shader:" + string(e.what()));
				}
			}
			else {*/
			// load fragment shader
			shaderToLoad = VDShader::create(mVDSettings, mVDAnimation, aFileName, mShaderFragmentString, getInputTexture());
			if (shaderToLoad->isValid()) {
				mShaderFileName = mFileNameWithExtension = shaderToLoad->getFileNameWithExtension();//was mFragFile.filename().string();
				mShaderFragmentString = shaderToLoad->getFragmentString();//was loadString(loadFile(mFragFile));
				mValid = setFragmentString(mShaderFragmentString, shaderToLoad->getFileNameWithExtension());// was mFragFile.filename().string());
			}

			else {
				mError = "Invalid shader file " + aFileName;
				// load default fragment shader
				try {
					mShaderName = mShaderFileName = "default.fs";
					mShader = gl::GlslProg::create(shaderToLoad->getDefaultVextexShaderString(), shaderToLoad->getDefaultFragmentShaderString());
					mValid = true;
					CI_LOG_V("fbo default vtx-frag compiled");
				}
				catch (gl::GlslProgCompileExc& exc) {
					mError = std::string(exc.what());
					CI_LOG_V("fbo unable to load/compile vtx-frag shader:" + std::string(exc.what()));
				}
				catch (const std::exception& e) {
					mError = std::string(e.what());
					CI_LOG_V("fbo unable to load vtx-frag shader:" + std::string(e.what()));
				}
			}
			//}
		}
		else {
			mError = "aFileName empty";
		}
		if (mError.length() > 0) mVDSettings->mFboMsg = mError + "\n" + mVDSettings->mFboMsg.substr(0, mVDSettings->mMsgLength);
		return mValid;
	}

	bool VDFbo::setFragmentString(const std::string& aFragmentShaderString, const std::string& aName) {

		std::string mOriginalFragmentString = aFragmentShaderString;
		std::string mOutputFragmentString = aFragmentShaderString;
		mError = "";
		mName = aName;
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
		// filename to save
		mValid = false;
		// load fragment shader
		CI_LOG_V("setFragmentString, loading" + mName);
		try
		{
			// before compilation save .fs file to inspect errors
			/*fileName = mName + ".fs";
			fs::path receivedFile = getAssetPath("") / "glsl" / "received" / fileName;
			ofstream mFragReceived(receivedFile.string(), std::ofstream::binary);
			mFragReceived << aFragmentShaderString;
			mFragReceived.close();
			CI_LOG_V("file saved:" + receivedFile.string());	*/

			std::size_t foundUniform = mOriginalFragmentString.find("uniform ");
			if (foundUniform == std::string::npos) {
				CI_LOG_V("loadFragmentStringFromFile, no mUniforms found, we add from shadertoy.inc");
				mOutputFragmentString = "/* " + mName + " */\n" + shaderInclude + mOriginalFragmentString;
			}
			else {
				mOutputFragmentString = "/* " + mName + " */\n" + mOriginalFragmentString;
			}
			fs::path mDefaultVertexFilePath = getAssetPath("") / "defaultvertex.fs";
			if (!fs::exists(mDefaultVertexFilePath)) {
				mError = mDefaultVertexFilePath.string() + " does not exist";
				CI_LOG_V(mError);
				mVDSettings->mErrorMsg = mError + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
			}
			// try to compile a first time to get active mUniforms
			mShader = gl::GlslProg::create(loadString(loadFile(mDefaultVertexFilePath)), mOutputFragmentString);
			// update only if success
			mShaderFragmentString = mOutputFragmentString;
			mVDSettings->mMsg = mName + " compiled(fbo)\n" + mVDSettings->mMsg.substr(0, mVDSettings->mMsgLength);
			mValid = true;
		}
		catch (gl::GlslProgCompileExc& exc)
		{
			mError = mName + std::string(exc.what());
			CI_LOG_V("setFragmentString, unable to compile live fragment shader:" + mError + " frag:" + mName);
		}
		catch (const std::exception& e)
		{
			mError = mName + std::string(e.what());
			CI_LOG_V("setFragmentString, error on live fragment shader:" + mError + " frag:" + mName);
		}
		if (mError.length() > 0) mVDSettings->mFboMsg = mError + "\n" + mVDSettings->mFboMsg.substr(0, mVDSettings->mMsgLength);
		return mValid;
	}

	ci::gl::Texture2dRef VDFbo::getFboTexture() {

		if (mValid) {

			gl::ScopedFramebuffer fbScp(mFbo);
			if (mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->ICLEAR)) {
				gl::clear(Color::black());
			}
			/*int f = 0;
			for (auto &fbo : mTextureList) {
				if (fbo->isValid() && mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0 + f) > 0.05f) {
					fbo->getTexture()->bind(f);
				}
				f++;
			}
			int t = 0;
			for (auto &tex : mTextureList) {
				if (tex->isValid() && mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0 + t) > 0.1f) {
					mGlslMixette->uniform("iChannel" + toString(t), t);
					mGlslMixette->uniform("iWeight" + toString(t), mVDAnimation->getUniformValue(mVDUniforms->IWEIGHT0 + t));
				}
				t++;
			}*/
			for (size_t i{1} ; i < 14; i++)
			{
				mTextureList[0]->getTexture(i)->bind(253 + i);
			}
			std::string name;
			std::string texName;
			int texNameEndIndex = 0;
			int texIndex = 0;
			int channelIndex = 0;
			mUniforms = mShader->getActiveUniforms();
			for (const auto& uniform : mUniforms) {
				name = uniform.getName(); // TODO uniform.getType()
				// CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", type:" + toString( uniform.getType() ));
				if (mVDAnimation->isExistingUniform(name)) {
					int uniformType = mVDUniforms->getUniformTypeByName(name);
					switch (uniformType)
					{
					case GL_FLOAT: // float 5126 GL_FLOAT 0x1406
						if (name == "TIME") {
							mShader->uniform(name, mVDAnimation->getUniformValueByName("TIME"));
						}
						else {
							if (mGlobal) {
								{
									mShader->uniform(name, mVDAnimation->getUniformValueByName(name));
								}
							}
							else {
								//createFloatUniform(name, mVDAnimation->getUniformIndexForName(name), getIntUniformValueByName(name), mVDAnimation->getMinUniformValueByName(name), mVDAnimation->getMaxUniformValueByName(name));
								mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
							}
						}
						break;
					case GL_SAMPLER_2D: // sampler2D 35678 GL_SAMPLER_2D 0x8B5E
						texNameEndIndex = name.find_last_of("iChannel");
						if (texNameEndIndex != std::string::npos) {
							texName = name.substr(0, texNameEndIndex + 1);
							texIndex = 0;// (int)(name.substr(texNameEndIndex + 1));
							CI_LOG_V(toString(texNameEndIndex) + texName);
							mShader->uniform(texName + toString(channelIndex), (uint32_t)(253 + channelIndex));
							channelIndex++;
						}
						else {
							mShader->uniform(name, (uint32_t)(0));
						}
						//mShader->uniform(name, 253);
						for (size_t i{1} ; i < 14; i++)
						{
							mShader->uniform(name, (uint32_t)(253 + i));
						}
						break;
					case GL_FLOAT_VEC2://GL_FLOAT_VEC2: // vec2 35664 GL_FLOAT_VEC2 0x8B50
						if (name == "RENDERSIZE") {
							//mShader->uniform(name, vec2(mTexture->getWidth(), mTexture->getHeight()));
							mShader->uniform(name, vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
						}
						else {
							mShader->uniform(name, mVDAnimation->getVec2UniformValueByName(name));
						}
						break;
					case GL_FLOAT_VEC3://GL_FLOAT_VEC3: // vec3 35665 GL_FLOAT_VEC3 0x8B51
						mShader->uniform(name, mVDAnimation->getVec3UniformValueByName(name));
						break;
					case GL_FLOAT_VEC4://GL_FLOAT_VEC4: // vec4 35666 GL_FLOAT_VEC4 0x8B52
						mShader->uniform(name, mVDAnimation->getVec4UniformValueByName(name));
						break;
					case GL_INT: // int 5124 GL_INT 0x1404
						// IBEAT 51
						// IBAR 52
						// IBARBEAT 53
						if (mGlobal) {
							mShader->uniform(name, mVDAnimation->getIntUniformValueByName(name));
						}
						else {
							//createIntUniform(name, mVDAnimation->getUniformIndexForName(name), getIntUniformValueByName(name)); // get same index as vdanimation
							mShader->uniform(name, mVDUniforms->getIntUniformValueByName(name));
						}

						break;
					case GL_BOOL: // boolean 35670 GL_BOOL 0x8B56
						if (mGlobal) {
							mShader->uniform(name, mVDAnimation->getBoolUniformValueByName(name));
						}
						else {
							//createBoolUniform(name, mVDAnimation->getUniformIndexForName(name), getBoolUniformValueByName(name)); // get same index as vdanimation
							mShader->uniform(name, mVDUniforms->getBoolUniformValueByName(name));
						}

						break;
					case GL_FLOAT_MAT4: // 35676 GL_FLOAT_MAT4 0x8B5C ciModelViewProjection


						break;
					default:
						break;
					}
				}
				else {
					if (name != "ciModelViewProjection") {//type 35676 GL_FLOAT_MAT4 0x8B5C
						mError = "fbo uniform not found " + name;
						mVDSettings->mErrorMsg = mError + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
						CI_LOG_E(mError);
					}
					else {
						mError = "should not arrive here " + name;
					}
				}
			}
			mShader->uniform("RENDERSIZE", vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
			mShader->uniform("TIME", (float)getElapsedSeconds());// mVDAnimation->getUniformValue(0));

			gl::ScopedGlslProg glslScope(mShader);
			// TODO: test gl::ScopedViewport sVp(0, 0, mFbo->getWidth(), mFbo->getHeight());	

			gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
			mRenderedTexture = mFbo->getColorTexture();
			if (!isReady) {
				std::string filename = mName + "-" + mTextureList[0]->getName() + ".jpg";
				fs::path fr = getAssetPath("") / "thumbs" / filename;

				if (!fs::exists(fr)) {
					CI_LOG_V(fr.string() + " does not exist, creating");
					Surface s8(mRenderedTexture->createSource());
					writeImage(writeFile(fr), s8);
				}
			}
		}
		return mRenderedTexture;
	}

	ci::gl::Texture2dRef VDFbo::getTexture() {
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
	//! to json
	JsonTree	VDFbo::toJson(bool save, unsigned int aIndex) const
	{
		JsonTree		json;
		JsonTree shader = ci::JsonTree::makeArray("shader");
		shader.addChild(ci::JsonTree("shadername", mShaderFileName));
		shader.pushBack(ci::JsonTree("shadertype", "fs"));
		json.addChild(shader);
		JsonTree texture = ci::JsonTree::makeArray("texture");
		texture.addChild(ci::JsonTree("texturename", mTextureList[0]->getName()));
		// mode is enum not string texture.pushBack(ci::JsonTree("texturetype", mTextureList[0]->getType()));
		texture.pushBack(ci::JsonTree("texturemode", mTextureList[0]->getMode()));
		json.addChild(texture);

		if (save) {
			std::string jsonFileName = mShaderName + ".json";
			fs::path jsonFile = getAssetPath("") / mVDSettings->mAssetsPath / jsonFileName;
			json.write(jsonFile);
			std::string jsonFboFileName = "fbo" + toString(aIndex) + ".json";
			fs::path jsonFboFile = getAssetPath("") / mVDSettings->mAssetsPath / jsonFboFileName;
			json.write(jsonFboFile);
		}
		return json;
	}

	bool									VDFbo::isValid() {
		return mValid;
	};

	std::string								VDFbo::getName() { return mName; };
	std::string								VDFbo::getShaderName() { return mShaderName; };
	std::string								VDFbo::getTextureName() { return mTextureList[0]->getName(); };
	ci::gl::Texture2dRef					VDFbo::getInputTexture() { return mTextureList[0]->getTexture(); };
	std::string								VDFbo::getStatus() { return mTextureList[0]->getStatus(); };
	void									VDFbo::setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename) {
		// TODO 20200630
		mTextureList[0]->setImageInputTexture(aTextureRef, aTextureFilename);
	};
	void									VDFbo::updateThumbFile() {
		isReady = false;
		getTexture();
		if (shaderToLoad) shaderToLoad->getThumbTexture();
	}
	std::vector<ci::gl::GlslProg::Uniform>	VDFbo::getUniforms() { return mUniforms; };
	bool									VDFbo::getGlobal() {
		return mGlobal;
	};
	void									VDFbo::toggleGlobal() {
		mGlobal = !mGlobal;
	};
	unsigned int					VDFbo::createInputTexture(const JsonTree& json) {
		unsigned int rtn = 0;
		VDTextureRef texRef = VDTexture::create(mVDSettings, mVDAnimation, json);
		mTextureList.push_back(texRef);
		rtn = mTextureList.size() - 1;
		return rtn;
	}
//}
/*void									flipV() {
	mVDAnimation->setBoolUniformValueByIndex(mVDUniforms->IFLIPV, !mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPV));
};
void									flipH() {
	mVDAnimation->setBoolUniformValueByIndex(mVDUniforms->IFLIPH, !mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPH));
};
bool									isFlipH() { return mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPV); };
bool									isFlipV() { return mVDAnimation->getBoolUniformValueByIndex(mVDUniforms->IFLIPH); };*/