#include "VDFboShader.h"

using namespace videodromm;

//namespace videodromm {
VDFboShader::VDFboShader(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
	:mVDSettings{ aVDSettings },
	mVDUniforms{ aVDUniforms },
	mVDAnimation{ aVDAnimation }
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
	mError = "";
	mActive = true;

	if (mValid) {
		CI_LOG_V("VDFboShader constructor success");
	}
	else {
		mVDSettings->mErrorMsg = "VDFboShader constructor failed\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
	}
}
VDFboShader::~VDFboShader(void) {
}
bool VDFboShader::setFragmentShaderString(unsigned int aShaderIndex, const std::string& aFragmentShaderString, const std::string& aName) {


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
	CI_LOG_V("setFragmentString, loading" + mName);
	try
	{
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

ci::gl::Texture2dRef VDFboShader::getFboTexture() {

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
		}for (size_t i{ 1 }; i < 14; i++)
		{
			mTextureList[0]->getTexture(i)->bind(253 + i);
		}*/
		mTexture->bind(253);// TODO  +i);
		std::string name;
		std::string texName;
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
			CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", type:" + toString( uniform.getType() ));
			//if (mVDAnimation->isExistingUniform(name)) {
				int uniformType = uniform.getType();
				switch (uniformType)
				{
				case 0: // because of int uniformType = mVDUniforms->getUniformTypeByName(name);
					if (name == "TIME" || name == "time") {
						mShader->uniform(name, mVDAnimation->getUniformValueByName("TIME"));
					}
					else {
						if (mVDAnimation->isExistingUniform(name)) {
							mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
						}
						else {
							//mVDUniforms->createFloatUniform
							//createFloatUniform(name, mVDAnimation->getUniformIndexForName(name), getIntUniformValueByName(name), mVDAnimation->getMinUniformValueByName(name), mVDAnimation->getMaxUniformValueByName(name));
							mShader->uniform(name, mVDAnimation->getUniformValue(0));

						}
					}
					break;
				case GL_FLOAT: // float 5126 GL_FLOAT 0x1406
					if (name == "TIME" || name == "time") {
						mShader->uniform(name, mVDAnimation->getUniformValueByName("TIME"));
					}
					else {
					if (mVDAnimation->isExistingUniform(name)) {
							mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
						}
						else {
							//mVDUniforms->createFloatUniform
							//createFloatUniform(name, mVDAnimation->getUniformIndexForName(name), getIntUniformValueByName(name), mVDAnimation->getMinUniformValueByName(name), mVDAnimation->getMaxUniformValueByName(name));
							mShader->uniform(name, mVDAnimation->getUniformValue(0));

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
					mVDSettings->mErrorMsg = mError + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
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
		mShader->uniform("time", mVDAnimation->getUniformValue(0));

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

ci::gl::Texture2dRef VDFboShader::getFboShaderTexture() {
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

bool									VDFboShader::isValid() {
	return mValid;
};

std::string								VDFboShader::getName() { return mName; };
std::string								VDFboShader::getShaderName() { return mShaderName; };

void									VDFboShader::setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename) {
	// TODO 20200630
	mTexture = aTextureRef;
};

std::vector<ci::gl::GlslProg::Uniform>	VDFboShader::getUniforms() { return mUniforms; };
