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
		mInputTextureList[i] = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mLoadTopDown));
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
	//mInputTextureRef = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mLoadTopDown));
	// 20211107 only if no texture ?
	mInputTextureRef = mVDAnimation->getAudioTexture();

	if (json.hasChild("shader")) {
		JsonTree shaderJsonTree(json.getChild("shader"));
		mShaderName = mShaderFileName = (shaderJsonTree.hasChild("shadername")) ? shaderJsonTree.getValueForKey<string>("shadername") : "inputImage.fs";
		mShaderFragmentString = (shaderJsonTree.hasChild("shadertext")) ? shaderJsonTree.getValueForKey<string>("shadertext") : "";
		shaderType = (json.hasChild("shadertype")) ? json.getValueForKey<string>("shadertype") : "fs";
	}
	if (json.hasChild("texture")) {

		JsonTree textureJsonTree(json.getChild("texture"));
		//tmp
		//string tx = (textureJsonTree.hasChild("texturename")) ? textureJsonTree.getValueForKey<string>("texturename") : "0.jpg";
		createInputTexture(textureJsonTree);
	}

	// init texture
	//mTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mLoadTopDown));
	//mTexture = ci::gl::Texture::create(loadImage(loadAsset("0.jpg")), ci::gl::Texture::Format().loadTopDown(mLoadTopDown)); //TODO json
	mRenderedTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown(mLoadTopDown));
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
unsigned int VDFboShader::createInputTexture(const JsonTree &json) {
	// 20211107
	unsigned int rtn = 0;
	mCurrentFilename = mTextureName = (json.hasChild("texturename")) ? json.getValueForKey<string>("texturename") : "0.jpg";
	mTypestr = (json.hasChild("texturetype")) ? json.getValueForKey<string>("texturetype") : "UNKNOWN";
	mMode = (json.hasChild("texturemode")) ? json.getValueForKey<int>("texturemode") : 0;


	switch (mMode)
	{
	case 6: // audio
		mCurrentFilename = mTextureName = "audio";
		break;
	case 2: // img seq loaded when ableton runs
		/*if (mExt = "rien") {

		//mImage = mVDSession->getInputTexture(mSeqIndex);
		 TODO 20211115 mImage = mVDSession->getCachedTexture(mSeqIndex, "a (" + toString(current) + ").jpg");
	}*/
	// init with number 1 then getFboTexture will load next images
		loadNextTexture(1);
		break;
	case 8: // img parts
		for (size_t i{ 0 }; i < 4; i++)
		{
			mCurrentFilename = mTextureName + " (" + toString(i) + ").jpg";
			//fs::path texFileOrPath = getAssetPath("") / mAssetsPath / currentFilename;
			texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;
			fileExists = fs::exists(texFileOrPath);
			if (fileExists) {
				//mInputTextureRef = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
				mInputTextureList[i] = ci::gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
				//mInputTextureList[i] = ci::gl::Texture::create(loadImage(texFileOrPath));
			}
			else {
				mCurrentFilename = mTextureName + " (" + toString(i) + ").png";
				
				texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;
				fileExists = fs::exists(texFileOrPath);
				if (fileExists) {
					//mInputTextureRef = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
					mInputTextureList[i] = ci::gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
					//mInputTextureList[i] = ci::gl::Texture::create(loadImage(texFileOrPath));
				}
			}
		}
		break;

	default:
		texFileOrPath = getAssetPath("") / mAssetsPath / mTextureName;
		mExt = "";
		dotIndex = texFileOrPath.filename().string().find_last_of(".");
		if (dotIndex != std::string::npos)  mExt = texFileOrPath.filename().string().substr(dotIndex + 1);
		// image
		if (mExt == "jpg" || mExt == "png") {
			// 20211107
			bool fileExists = fs::exists(texFileOrPath);
			if (!fileExists) {
				mError = texFileOrPath.string() + " does not exist, trying with parent folder";
				CI_LOG_V(mError);
				texFileOrPath = getAssetPath("") / mTextureName;
				fileExists = fs::exists(texFileOrPath);
				if (!fileExists) {
					mError = texFileOrPath.string() + " does not exist in parent folder";
					CI_LOG_V(mError);
				}
			}
			if (fileExists) {
				mInputTextureRef = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
				// TODO check topdown mInputTextureRef = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
				mTypestr = "image";
				mCurrentFilename = mTextureName;
				mMode = 1;
			}
			else {
				// default to audio
				mCurrentFilename = mTextureName = "audio";
				mMode = 6;
			}
			//mInputTextureList.push_back(mInputTextureRef);
		}
		else {
			// video
			if (mExt == "mp4") {
				bool fileExists = fs::exists(texFileOrPath);
				if (!fileExists) {
					mError = texFileOrPath.string() + " video does not exist, trying with parent folder";
					CI_LOG_V(mError);
					texFileOrPath = getAssetPath("") / mTextureName;
					fileExists = fs::exists(texFileOrPath);
					if (!fileExists) {
						mError = texFileOrPath.string() + " video does not exist in parent folder";
						CI_LOG_V(mError);
					}
				}
				if (fileExists) {
					try {
						mGlslVideoTexture = gl::GlslProg::create(gl::GlslProg::Format()
							.vertex(loadAsset("video_texture.vs.glsl"))
							.fragment(loadAsset("video_texture.fs.glsl")));
					}
					catch (gl::GlslProgCompileExc ex) {
						CI_LOG_E("<< GlslProg Compile Error >>\n" << ex.what());
					}
					catch (gl::GlslProgLinkExc ex) {
						CI_LOG_E("<< GlslProg Link Error >>\n" << ex.what());
					}
					catch (gl::GlslProgExc ex) {
						CI_LOG_E("<< GlslProg Error >> " << ex.what());
					}
					catch (AssetLoadExc ex) {
						CI_LOG_E("<< Asset Load Error >> " << ex.what());
					}
					mBatchPlaneVideo = gl::Batch::create(geom::Plane().normal(vec3(0, 0, 1)), mGlslVideoTexture);
					if (mBatchPlaneVideo) {
						mBatchPlaneVideo->replaceGlslProg(mGlslVideoTexture);
					}
					if (!mVideo.isStopped()) {
						mVideo.stop();
					}
					mIsVideoLoaded = mVideo.loadMovie(texFileOrPath);
					mVideoDuration = mVideo.getDuration();
					mVideoPos = mVideo.getPosition();
					//mVideo.play();
					mCam.setPerspective(60.0f, getWindowAspectRatio(), 0.01f, 10000.0f);
					mCam.lookAt(vec3(0, 0, 500), vec3(), vec3(0, 1, 0));
					mCam.setAspectRatio(getWindowAspectRatio());
					mCamUi.setCamera(&mCam);
					mCamUi.setMouseWheelMultiplier(-mCamUi.getMouseWheelMultiplier());
					mVDUniforms->setUniformValue(mVDUniforms->IMOUSEX, 0.0f);
					mVDUniforms->setUniformValue(mVDUniforms->IMOUSEY, 0.0f);
					mVDUniforms->setUniformValue(mVDUniforms->IMOUSEZ, 0.0f);
					//mInputTextureRef = gl::Texture::create(, gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
					mTypestr = "video";
					mCurrentFilename = mTextureName;
					mMode = 3;
				}
				else {
					// default to audio
					mCurrentFilename = mTextureName = "audio";
					mMode = 6;
				}
				//mInputTextureList.push_back(mInputTextureRef);
			}
			else {
				// default to audio
				mCurrentFilename = mTextureName = "audio";
				mMode = 6;
			}

		}

		break;
	}

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
	if (isAudio && mValid) {
		mInputTextureRef = mVDAnimation->getAudioTexture();
		mCurrentFilename = "audio";
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
void VDFboShader::loadNextTexture(int aCurrentIndex) {
	if (mCurrentImageSequenceIndex != aCurrentIndex) {
		mCurrentImageSequenceIndex = aCurrentIndex;
		mCurrentFilename = mTextureName + " (" + toString(mCurrentImageSequenceIndex) + ").jpg";
		//fs::path texFileOrPath = getAssetPath("") / mAssetsPath / currentFilename;
		fs::path texFileOrPath = getAssetPath("") / mTextureName / mCurrentFilename;

		bool fileExists = fs::exists(texFileOrPath);
		if (fileExists) {
			// start profiling
			auto start = Clock::now();

			mInputTextureRef = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown(mLoadTopDown).mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
			auto end = Clock::now();
			auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			int ms = msdur.count();
			mStatus = mCurrentFilename + " " + toString(ms) + "ms";
		}
	}
}
ci::gl::Texture2dRef VDFboShader::getFboTexture() {

	if (mValid) {
		if (mMode == 2) {
			// 20211115 OK for SOS a (n).jpg
			loadNextTexture((int)mVDUniforms->getUniformValue(mVDUniforms->IBARBEAT));
		}

		// removed 20211107 only if no texture
		/*if (mInputTextureIndex == 0) {
			mInputTextureRef = mVDAnimation->getAudioTexture();
		}
		else {
			*/
		if (mMode == 6) {
			mInputTextureRef = mVDAnimation->getAudioTexture();
		}
		gl::ScopedFramebuffer fbScp(mFbo);
		if (mVDUniforms->getUniformValue(mVDUniforms->ICLEAR)) {
			gl::clear(Color::black());
		}
		if (mMode == 3) {
			// video
			gl::pushMatrices();
			//gl::setMatricesWindow(mWidth, mHeight)

			gl::setMatrices(mCam);
			gl::ScopedViewport scopedViewport(getWindowSize());
			//gl::ScopedDepth scopedDepth(true);
			if (mIsVideoLoaded) {
				mVideo.update();
				mVideoPos = mVideo.getPosition();
				if (mVideo.isStopped() || mVideo.isPaused()) {
					mVideo.setPosition(0.0);
					mVideo.play();
				}
				vec2 videoSize = vec2(mVideo.getWidth(), mVideo.getHeight());
				mGlslVideoTexture->uniform("uVideoSize", videoSize);
				videoSize *= 0.5f;
				{
					gl::ScopedColor scopedColor(Colorf::white());
					gl::ScopedModelMatrix scopedModelMatrix;

					ciWMFVideoPlayer::ScopedVideoTextureBind scopedVideoTex(mVideo, 0);
					gl::translate(vec3(mVDUniforms->getUniformValue(mVDUniforms->IMOUSEX)*100.0f, mVDUniforms->getUniformValue(mVDUniforms->IMOUSEY)*100.0f, mVDUniforms->getUniformValue(mVDUniforms->IMOUSEZ)*100.0f));

					gl::scale(vec3(videoSize, 1.0f));
					mBatchPlaneVideo->draw();
				}
			}
			// restore matrices
			gl::popMatrices();
		}
		else {
			// not a video
			if (mMode == 8) {
				// imgseq
				for (size_t i{ 0 }; i < 4; i++)
				{
					mInputTextureList[i]->bind(253 + i);
				}
			}
			else {
				mInputTextureRef->bind(0);
			}

			if (mIsHydraTex) {
				mInputTextureRef->bind(253);
				for (size_t i{ 0 }; i < 4; i++)
				{
					mInputTextureList[i]->bind(254 + i);
				}
			}
			else {
				/* onezero ko for (size_t i{ 0 }; i < 4; i++)
				{
					mInputTextureList[i]->bind( i);
				}*/
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
						mInputTextureName = name.substr(0, texNameEndIndex + 8);
						mShader->uniform(name, (uint32_t)(253 + channelIndex));
						mInputTextureList[channelIndex]->bind(253 + channelIndex);
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
								// onezero check
								mShader->uniform(name, (uint32_t)(0));

							}
						}
					}
					/* TODO CHECK if needed 20111121, apparently not helping onezero ko
					for (size_t i{ 1 }; i < 14; i++)
					{
						//mShader->uniform(name, (uint32_t)(253 + i));
						mShader->uniform(name, (uint32_t)(i));
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
					if (name == "iDate") {
						mShader->uniform(name, vec4(mVDUniforms->getUniformValue(mVDUniforms->IDATEX), mVDUniforms->getUniformValue(mVDUniforms->IDATEY), mVDUniforms->getUniformValue(mVDUniforms->IDATEZ), mVDUniforms->getUniformValue(mVDUniforms->IDATEW)));
						//CI_LOG_E(mShader->getLabel() + ", getShader uniform name:" + uniform.getName() + ", IDATEX:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEX)) + ", IDATEY:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEY)) + ", IDATEZ:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEZ)) + ", IDATEW:" + toString(mVDUniforms->getUniformValue(mVDUniforms->IDATEW)));
					}
					else {
						mShader->uniform(name, mVDUniforms->getVec4UniformValueByName(name));
					}
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
		}
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
