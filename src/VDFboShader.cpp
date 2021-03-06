#include "VDFboShader.h"

using namespace videodromm;

//namespace videodromm {
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
	for (size_t i{ 0 }; i < 4; i++)
	{
		mInputTextureList[i] = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	}
	// load default fragment shader
	mShaderName = mShaderFileName = "inputImage.fs";
	mShaderFragmentString = mVDParams->getDefaultShaderFragmentString();
	/*fs::path mDefaultFragmentFilePath = getAssetPath("") / mShaderFileName;
	if (!fs::exists(mDefaultFragmentFilePath)) {
		mError = mDefaultFragmentFilePath.string() + " does not exist";
		CI_LOG_V(mError);
		mShaderFragmentString = "void main(void) { vec2 uv = gl_FragCoord.xy / RENDERSIZE.xy; vec3 rgb = IMG_NORM_PIXEL(inputImage, uv).xyz; fragColor=vec4(rgb, 1.0);}";
	}
	else {
		mShaderFragmentString = loadString(loadFile(mDefaultFragmentFilePath));
	}*/
	// load default vertex shader
	/*fs::path mDefaultVertexFilePath = getAssetPath("") / "defaultvertex.fs";
	if (!fs::exists(mDefaultVertexFilePath)) {
		mError = mDefaultVertexFilePath.string() + " does not exist";
		CI_LOG_V(mError);
		mDefaultVertexString = "#version 150\nuniform mat4 ciModelViewProjection; in vec4 ciPosition; in vec4 ciColor; in vec2 ciTexCoord0; out vec4 vertColor; out vec2 vertTexCoord0;"
								"void main() { vertColor = ciColor; vertTexCoord0 = ciTexCoord0; gl_Position = ciModelViewProjection * ciPosition; }";
	}
	else {
		mDefaultVertexString = loadString(loadFile(mDefaultVertexFilePath));
	}*/
	//string textureFileName = "0.jpg"; 
	//mTextureName = mCurrentSeqFilename = mLastCachedFilename = textureFileName;
	//mInputTextureIndex = 0;


	shaderInclude = loadString(loadAsset("shadertoy.vd"));

	mInputTextureIndex = 0;
	mInputTextureName = "none";
	//mInputTextureRef = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	mInputTextureRef = mVDAnimation->getAudioTexture();

	if (json.hasChild("shader")) {
		JsonTree shaderJsonTree(json.getChild("shader"));
		mShaderName = mShaderFileName = (shaderJsonTree.hasChild("shadername")) ? shaderJsonTree.getValueForKey<string>("shadername") : "inputImage.fs";
		mShaderFragmentString = (shaderJsonTree.hasChild("shadertext")) ? shaderJsonTree.getValueForKey<string>("shadertext") : "";
		shaderType = (json.hasChild("shadertype")) ? json.getValueForKey<string>("shadertype") : "fs";
	}
	/*if (json.hasChild("texture")) {

		JsonTree textureJsonTree(json.getChild("texture"));
		//tmp
		//string tx = (textureJsonTree.hasChild("texturename")) ? textureJsonTree.getValueForKey<string>("texturename") : "0.jpg";
		mAssetsPath = (textureJsonTree.hasChild("assetspath")) ? textureJsonTree.getValueForKey<string>("assetspath") : aAssetsPath;
		createInputTexture(textureJsonTree);
	}*/

	// init texture
	//mTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	//mTexture = ci::gl::Texture::create(loadImage(loadAsset("0.jpg")), ci::gl::Texture::Format().loadTopDown()); //TODO json
	mRenderedTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	isReady = false;

	// init texture
	// init the fbo whatever happens next
	fboFmt.setColorTextureFormat(fmt);
	mFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), fboFmt);
	mMsg = "";
	mError = "";
	mActive = true;
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
		mError = "VDFbo constructor failed";
	}
}
VDFboShader::~VDFboShader(void) {
}
/*unsigned int VDFboShader::createInputTexture(const JsonTree &json) {
	unsigned int rtn = 0;
	VDTextureRef texRef = VDTexture::create(mVDParams, json);
	mInputTextureList.push_back(texRef);
	rtn = mInputTextureList.size() - 1;
	return rtn;

}*/
bool VDFboShader::loadFragmentShaderFromFile(const string& aFileOrPath) {
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
	mIsHydraTex = false;
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
	CI_LOG_V("setFragmentShaderString, loading " + mName);
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
		mMsg = mName + " ok";
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
		// TODO 20200105 
		if (mInputTextureIndex == 0) {
			mInputTextureRef = mVDAnimation->getAudioTexture();
		}

		gl::ScopedFramebuffer fbScp(mFbo);
		if (mVDUniforms->getUniformValue(mVDUniforms->ICLEAR)) {
			gl::clear(Color::black());
		}

		mInputTextureRef->bind(0);
		mInputTextureRef->bind(253);
		if (mIsHydraTex) {
			for (size_t i{ 0 }; i < 4; i++)
			{
				mInputTextureList[i]->bind(254 + i);
			}

		}
		std::string name;

		int texNameEndIndex = 0;
		int channelIndex = 0;
		/* hydra
			uniform float time;
			uniform vec2 resolution;
			varying vec2 uv;
			uniform sampler2D prevBuffer;
		*/
		mUniforms = mShader->getActiveUniforms();
		for (const auto& uniform : mUniforms) {

			name = uniform.getName();
			//CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", type:" + toString(uniform.getType()) + ", Location:" + toString(uniform.getLocation()));
			//if (mVDAnimation->isExistingUniform(name)) {
			int uniformType = uniform.getType();
			switch (uniformType)
			{

			case GL_FLOAT: // float 5126 0x1406
				if (name == "TIME" || name == "time") {
					mShader->uniform(name, mVDUniforms->getUniformValue(mVDUniforms->ITIME));
				}
				else {
					if (mVDUniforms->isExistingUniform(name)) {
						mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
					}
					else {
						int l = uniform.getLocation();						
						mShader->uniform(name, mUniformValueByLocation[l]);
					}
				}
				break;
			case GL_SAMPLER_2D: // sampler2D 35678 0x8B5E
				texNameEndIndex = name.find("iChannel");
				if (texNameEndIndex != std::string::npos && texNameEndIndex != -1) {
					mInputTextureName = name.substr(0, texNameEndIndex + 7);
					mShader->uniform(mInputTextureName + toString(channelIndex), (uint32_t)(253 + channelIndex));
					channelIndex++;
				}
				else {
					if (name == "inputImage") {
						mShader->uniform(name, (uint32_t)(0));
					}
					else {
						texNameEndIndex = name.find("tex");
						if (texNameEndIndex != std::string::npos && texNameEndIndex != -1) {
							// hydra fbo
							mIsHydraTex = true;
							mInputTextureName = name.substr(0, texNameEndIndex + 3);
							// 20210116 TODO 
							mShader->uniform(name, (uint32_t)(254 + channelIndex));
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
							mShader->uniform(name, (uint32_t)(0));
						}
					}
				}
				/*for (size_t i{ 1 }; i < 14; i++)
				{
					mShader->uniform(name, (uint32_t)(253 + i));
				}*/
				break;
			case GL_FLOAT_VEC2:// vec2 35664 0x8B50
				if (name == "RENDERSIZE" || name == "resolution") {
					//mShader->uniform(name, vec2(mTexture->getWidth(), mTexture->getHeight()));
					mShader->uniform(name, vec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
				}
				else {
					mShader->uniform(name, mVDUniforms->getVec2UniformValueByName(name));
				}
				break;
			case GL_FLOAT_VEC3:// vec3 35665 0x8B51
				mShader->uniform(name, mVDUniforms->getVec3UniformValueByName(name));
				break;
			case GL_FLOAT_VEC4:// vec4 35666 0x8B52
				mShader->uniform(name, mVDUniforms->getVec4UniformValueByName(name));
				break;
			case GL_INT: // int 5124 0x1404
				// IBEAT 51
				// IBAR 52
				// IBARBEAT 53
				mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
				break;
			case GL_BOOL: // boolean 35670 0x8B56
				//createBoolUniform(name, mVDAnimation->getUniformIndexForName(name), getBoolUniformValueByName(name)); // get same index as vdanimation
				mShader->uniform(name, mVDUniforms->getUniformValueByName(name));
				break;
			case GL_FLOAT_MAT4: // 35676 0x8B5C ciModelViewProjection
				break;
			default:
				break;
			}
		}

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
/*ci::gl::Texture2dRef VDFboShader::getInputTexture() {
	return mInputTextureList[mInputTextureIndex]->getTexture();
}*/
bool									VDFboShader::isValid() {
	return mValid;
};

std::string								VDFboShader::getShaderName() {
	return mShaderName;
};


/*void									VDFboShader::setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename) {
	mTextureList[mInputTextureIndex]->setImageInputTexture(aTextureRef, aTextureFilename);
};*/

std::vector<ci::gl::GlslProg::Uniform>	VDFboShader::getUniforms() {
	return mUniforms;
};
