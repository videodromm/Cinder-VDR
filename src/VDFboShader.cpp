#include "VDFboShader.h"

using namespace videodromm;

//namespace videodromm {
VDFboShader::VDFboShader(VDUniformsRef aVDUniforms)
	:mVDUniforms{ aVDUniforms }
{
	CI_LOG_V("VDFboShader constructor");
	// Params
	mVDParams = VDParams::create();

	std::string shaderFileName = "inputImage.fs";
	mShaderName = mShaderFileName;
	std::string shaderType = "fs";
	mShaderFragmentString = "";
	//string textureFileName = "0.jpg"; 
	//mTextureName = mCurrentSeqFilename = mLastCachedFilename = textureFileName;
	//mInputTextureIndex = 0;

	mShaderName = mShaderFileName = "inputImage.fs";
	mShaderFragmentString = "";
	shaderType = "fs";
	shaderInclude = loadString(loadAsset("shadertoy.vd"));
	// init texture
	//mTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	mTexture = ci::gl::Texture::create(loadImage(loadAsset("0.jpg")), ci::gl::Texture::Format().loadTopDown());
	mRenderedTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	isReady = false;

	// init texture
	// init the fbo whatever happens next
	fboFmt.setColorTextureFormat(fmt);
	mFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
	mMsg = "";
	mError = "";
	mActive = true;
}
VDFboShader::~VDFboShader(void) {
}

bool VDFboShader::loadFragmentStringFromFile(const string& aFileOrPath) {
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
				mFragFilePath = getAssetPath("") / aFileOrPath;
				if (!fs::exists(mFragFilePath)) {
					fileExists = false;
					mError = "VDFboShader file does not exist in assets root or current subfolder:" + aFileOrPath;
				}
			}
		}
	}
	else {
		mError = "VDFboShader file empty";
	}
	if (fileExists) {
		// file exists
		mValid = loadFragmentStringFromFile();
	}
	return mValid;
}
// private
bool VDFboShader::loadFragmentStringFromFile() {
	mValid = false;
	// load fragment shader
	mFileNameWithExtension = mFragFilePath.filename().string();
	CI_LOG_V("loadFragmentStringFromFile, loading " + mFileNameWithExtension);
	mValid = setFragmentShaderString(loadString(loadFile(mFragFilePath)), mFileNameWithExtension);
	CI_LOG_V(mFragFilePath.string() + " loaded and compiled");
	return mValid;
}
bool VDFboShader::setFragmentShaderString(const std::string& aFragmentShaderString, const std::string& aName) {
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

	// load fragment shader
	CI_LOG_V("setFragmentShaderString, loading" + mName);
	try
	{
		std::size_t foundUniform = mOriginalFragmentString.find("uniform ");
		if (foundUniform == std::string::npos) {
			CI_LOG_V("setFragmentShaderString, no uniforms found, we add from shadertoy.inc");
			mOutputFragmentString = "/* " + mName + " */\n" + shaderInclude + mOriginalFragmentString;
		}
		else {
			mOutputFragmentString = "/* " + mName + " */\n" + mOriginalFragmentString;
		}
		fs::path mDefaultVertexFilePath = getAssetPath("") / "defaultvertex.fs";
		if (!fs::exists(mDefaultVertexFilePath)) {
			mError = mDefaultVertexFilePath.string() + " does not exist";
			CI_LOG_V(mError);
		}
		// try to compile a first time to get active mUniforms
		mShader = gl::GlslProg::create(loadString(loadFile(mDefaultVertexFilePath)), mOutputFragmentString);
		// update only if success
		mShaderFragmentString = mOutputFragmentString;
		mMsg = mName + " compiled";
		mValid = true;
	}
	catch (gl::GlslProgCompileExc& exc)
	{
		mError = mName + std::string(exc.what());
		CI_LOG_V("setFragmentShaderString, unable to compile live fragment shader:" + mError + " frag:" + mName);
	}
	catch (const std::exception& e)
	{
		mError = mName + std::string(e.what());
		CI_LOG_V("setFragmentShaderString, error on live fragment shader:" + mError + " frag:" + mName);
	}
	return mValid;
}

ci::gl::Texture2dRef VDFboShader::getFboTexture() {

	if (mValid) {

		gl::ScopedFramebuffer fbScp(mFbo);
		if (mVDUniforms->getBoolUniformValueByIndex(mVDUniforms->ICLEAR)) {
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
		}for (size_t i{ 1 }; i < 14; i++)
		{
			mTextureList[0]->getTexture(i)->bind(253 + i);
		}*/
		mTexture->bind(253);// TODO  +i);
		std::string name;
		
		int texNameEndIndex = 0;
		int texIndex = 0;
		int channelIndex = 0;
		/* hydra
			uniform float time;
			uniform vec2 resolution;
			varying vec2 uv;
			uniform sampler2D prevBuffer;
		*/
		mUniforms = mShader->getActiveUniforms();
		for (const auto& uniform : mUniforms) {
			
			name = uniform.getName(); // TODO uniform.getType()
			CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", type:" + toString(uniform.getType()) + ", Location:" + toString(uniform.getLocation()));
			//if (mVDAnimation->isExistingUniform(name)) {
				int uniformType = uniform.getType();
				switch (uniformType)
				{
				
				case GL_FLOAT: // float 5126 GL_FLOAT 0x1406
					if (name == "TIME" || name == "time") {
						mShader->uniform(name, mVDUniforms->getUniformValueByName("TIME"));
					}
					else {
					if (mVDUniforms->isExistingUniform(name)) {
							mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
						}
						else {
							//mVDUniforms->createFloatUniform
							//createFloatUniform(name, mVDAnimation->getUniformIndexForName(name), getIntUniformValueByName(name), mVDAnimation->getMinUniformValueByName(name), mVDAnimation->getMaxUniformValueByName(name));
							//mShader->uniform(name, mVDAnimation->getUniformValue(0));
							int l = uniform.getLocation();
							float v = getUniformValueByLocation(l);
							mShader->uniform(l, mUniformValueByLocation[l]);
						}
					}
					break;
				case GL_SAMPLER_2D: // sampler2D 35678 GL_SAMPLER_2D 0x8B5E
					texNameEndIndex = name.find_last_of("iChannel");
					if (texNameEndIndex != std::string::npos) {
						mTextureName = name.substr(0, texNameEndIndex + 1);
						texIndex = 0;// (int)(name.substr(texNameEndIndex + 1));
						CI_LOG_V(toString(texNameEndIndex) + mTextureName);
						mShader->uniform(mTextureName + toString(channelIndex), (uint32_t)(253 + channelIndex));
						channelIndex++;
					}
					else {
						mShader->uniform(name, (uint32_t)(0));
					}
					//mShader->uniform(name, 253);
					for (size_t i{ 1 }; i < 14; i++)
					{
						mShader->uniform(name, (uint32_t)(253 + i));
					}
					break;
				case GL_FLOAT_VEC2://GL_FLOAT_VEC2: // vec2 35664 GL_FLOAT_VEC2 0x8B50
					if (name == "RENDERSIZE" || name == "resolution") {
						//mShader->uniform(name, vec2(mTexture->getWidth(), mTexture->getHeight()));
						mShader->uniform(name, vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
					}
					else {
						mShader->uniform(name, mVDUniforms->getVec2UniformValueByName(name));
					}
					break;
				case GL_FLOAT_VEC3://GL_FLOAT_VEC3: // vec3 35665 GL_FLOAT_VEC3 0x8B51
					mShader->uniform(name, mVDUniforms->getVec3UniformValueByName(name));
					break;
				case GL_FLOAT_VEC4://GL_FLOAT_VEC4: // vec4 35666 GL_FLOAT_VEC4 0x8B52
					mShader->uniform(name, mVDUniforms->getVec4UniformValueByName(name));
					break;
				case GL_INT: // int 5124 GL_INT 0x1404
					// IBEAT 51
					// IBAR 52
					// IBARBEAT 53
					mShader->uniform(name, mVDUniforms->getIntUniformValueByName(name));
					break;
				case GL_BOOL: // boolean 35670 GL_BOOL 0x8B56
					//createBoolUniform(name, mVDAnimation->getUniformIndexForName(name), getBoolUniformValueByName(name)); // get same index as vdanimation
					mShader->uniform(name, mVDUniforms->getBoolUniformValueByName(name));
					break;
				case GL_FLOAT_MAT4: // 35676 GL_FLOAT_MAT4 0x8B5C ciModelViewProjection
					break;
				default:
					break;
				}
			/*}
			else {
				if (name != "ciModelViewProjection") {//type 35676 GL_FLOAT_MAT4 0x8B5C
					mError = "fbo uniform not found " + name;
					mMsg = mError + "\n" + mMsg.substr(0, mMsgLength);
					CI_LOG_E(mError);
				}
				else {
					mError = "should not happen: " + name;
				}
			}*/
		}
		//mShader->uniform("RENDERSIZE", vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
		//mShader->uniform("TIME", (float)getElapsedSeconds());// mVDAnimation->getUniformValue(0));
		mShader->uniform("resolution", vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
		mShader->uniform("time", mVDUniforms->getUniformValue(0));

		gl::ScopedGlslProg glslScope(mShader);
		// TODO: test gl::ScopedViewport sVp(0, 0, mFbo->getWidth(), mFbo->getHeight());	

		gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
		mRenderedTexture = mFbo->getColorTexture();
		if (!isReady) {
			std::string filename = mName + ".jpg";
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
ci::gl::Texture2dRef VDFboShader::getInputTexture() {
	return mTexture;
}
bool									VDFboShader::isValid() {
	return mValid;
};

std::string								VDFboShader::getShaderName() { 
	return mShaderName; 
};

void									VDFboShader::setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename) {
	// TODO 20200630
	mTexture = aTextureRef;
};

std::vector<ci::gl::GlslProg::Uniform>	VDFboShader::getUniforms() { 
	return mUniforms; 
};
