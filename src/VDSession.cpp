//
//  VDsession.cpp
//

#include "VDSession.h"

using namespace videodromm;

VDSession::VDSession(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, VDMixRef aVDMix)
{
	CI_LOG_V("VDSession ctor");
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mVDUniforms = aVDUniforms;
	// Mix
	mVDMix = aVDMix;
	// Params
	mVDParams = VDParams::create();

	// Animation
	// TODO: needed? mVDAnimation->tapTempo();
	//createFboShaderTexture("default.fs", "0.jpg");
	//createFboShaderTexture("audio.fs", "audio");
	// allow log to file
	//mVDLog = VDLog::create();
	// fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing

	fboFmt.setColorTextureFormat(fmt);
	mWarpsFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), format.depthTexture());
	mPostFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), format.depthTexture());
	mGlslPost = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("passthrough.vs")).fragment(loadAsset("post.glsl")));
	mWarpTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	// adjust the content size of the warps

	// TODO 20200305 was 20200302 if (getFboRenderedTexture(0)) Warp::setSize(mWarpList, getFboRenderedTexture(0)->getSize());
	Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), mVDParams->getFboHeight())); //
	// initialize warps
	mSettings = getAssetPath("") / mVDSettings->mAssetsPath / "warps.xml";
	if (fs::exists(mSettings)) {
		// load warp settings from file if one exists
		mWarpList = Warp::readSettings(loadFile(mSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarpList.push_back(WarpPerspectiveBilinear::create());
	}
	loadFbos();
	//loadWarps();
	// init fbo format
	//fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
	//fmt.setBorderColor(Color::black());
	// uncomment this to enable 4x antialiasing
	//fboFmt.setSamples( 4 );
	/*fboFmt.setColorTextureFormat(fmt);
	mPosX = mPosY = 0.0f;
	mZoom = 1.0f;
	mSelectedWarp = 0;*/
	// Modes
	mModesList[0] = "Mixette";
	mModesList[1] = "Fbo0";
	mModesList[2] = "Fbo1";
	mModesList[3] = "Fbo2";
	mModesList[4] = "Fbo3";
	mModesList[5] = "Fbo4";
	mModesList[6] = "Fbo5";
	mModesList[7] = "Fbo6";
	mModesList[8] = "Fbo7";
	mModesList[9] = "Fbo8";
	mMode = 0;

	// Message router
	//mVDRouter = VDRouterBuilder::createVDRouter(mVDSettings, mVDAnimation)->addShader(0,6)->getInstance();
	//mVDRouterBuilder = VDRouterBuilder::createVDRouter(mVDSettings, mVDAnimation)->setWarpBFboIndex(0, 1);
	//Attente d'une récupération de données via socket io
	//mVDRouterBuilder->setWarpAFboIndex(1, 1)->setWarpBFboIndex(1,0);
	//Attente d'une autre autre récupération de données via socket io
	//mVDRouterBuilder->setWarpAFboIndex(0, 0);
	//mVDRouter = builder->getInstance();

	// reset no matter what, so we don't miss anything
	cmd = -1;
	mFreqWSSend = false;
	reset();

	mCurrentBlend = 0;
	// check to see if VDSession.xml file exists and restore if it does
	sessionPath = getAssetPath("") / mVDSettings->mAssetsPath / sessionFileName;
	if (fs::exists(sessionPath))
	{
		restore();
	}
	else
	{
		// Create json file if it doesn't already exist.
		std::ofstream oStream(sessionPath.string());
		oStream.close();
		save();
	}
}
void VDSession::loadFromJsonFile(const fs::path& jsonFile) {
	if (fs::exists(jsonFile)) {
		JsonTree json(loadFile(jsonFile));
		fboFromJson(json);
	}
}
void VDSession::loadFbos() {

	int f = 0;
	bool found = true;
	std::string shaderFileName;
	std::string textureFileName;
	while (found) {
		std::string jsonFileName = "fbo" + toString(f) + ".json";

		fs::path jsonFile = getAssetPath("") / mVDSettings->mAssetsPath / jsonFileName;
		if (fs::exists(jsonFile)) {
			// nouveau
				/*loadFromJsonFile(jsonFile)
						->createShader()
						->createUniforms()
						->compile()
						->createFboWhenSuccess()
						->addToFboList();*/
						// ancien
			JsonTree json(loadFile(jsonFile));
			fboFromJson(json);
			f++;
		}
		else {
			found = false;
		}
	} //while

}
int  VDSession::getMode() { 
	return mMode; 
};
void VDSession::setMode(int aMode) { 
	mMode = aMode; 
};
int VDSession::getModesCount() { 
	return mModesList.size() - 1; 
};
void VDSession::toggleUI() {
	mShowUI = !mShowUI; 
};
bool VDSession::showUI() { 
	return mShowUI; 
};
std::string VDSession::getModeName(unsigned int aMode) {
	if (aMode > mModesList.size() - 1) aMode = mModesList.size() - 1;
	return mModesList[aMode];
}

void VDSession::update(unsigned int aClassIndex) {

	// fps calculated in main app
	mVDSettings->sFps = toString(floor(mVDUniforms->getUniformValue(mVDUniforms->IFPS)));
	mVDAnimation->update();

	mVDMix->getMixetteTexture(0);
	renderWarpsToFbo();
	renderPostToFbo();
}
void VDSession::renderPostToFbo()
{
	{
		gl::ScopedFramebuffer fbScp(mPostFbo);
		// clear out the FBO with black
		gl::clear(Color::black());
		//gl::clear(ColorA(0.4f, 0.8f, 0.0f, 0.3f));

		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mPostFbo->getSize());

		// texture binding must be before ScopedGlslProg
		//mWarpsFbo->getColorTexture()
		mWarpTexture->bind(40);
		gl::ScopedGlslProg prog(mGlslPost);

		// not used yet mGlslPost->uniform("TIME", getUniformValue(mVDUniforms->ITIME) - mVDSettings->iStart);;
		mGlslPost->uniform("iResolution", vec3(mVDParams->getFboWidth(), mVDParams->getFboHeight(), 1.0));
		mGlslPost->uniform("iChannel0", 40); // texture 0
		mGlslPost->uniform("iSobel", mVDUniforms->getUniformValue(mVDUniforms->ISOBEL));
		mGlslPost->uniform("iExposure", mVDUniforms->getUniformValue(mVDUniforms->IEXPOSURE));
		mGlslPost->uniform("iTrixels", mVDUniforms->getUniformValue(mVDUniforms->ITRIXELS)); // trixels if > 0.
		mGlslPost->uniform("iZoom", mVDUniforms->getUniformValue(mVDUniforms->IZOOM));
		mGlslPost->uniform("iChromatic", mVDUniforms->getUniformValue(mVDUniforms->ICHROMATIC));
		mGlslPost->uniform("iFlipV", (int)getBoolUniformValueByIndex(mVDUniforms->IFLIPPOSTV));
		mGlslPost->uniform("iFlipH", (int)getBoolUniformValueByIndex(mVDUniforms->IFLIPPOSTH));
		mGlslPost->uniform("iInvert", (int)getBoolUniformValueByIndex(mVDUniforms->IINVERT));
		gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
	}
}
void VDSession::renderWarpsToFbo()
{
	{
		gl::ScopedFramebuffer fbScp(mWarpsFbo);
		// clear out the FBO with black
		//gl::clear(Color::black());
		gl::clear(ColorA(0.4f, 0.0f, 0.8f, 0.3f));

		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mWarpsFbo->getSize());
		// iterate over the warps and draw their content
		int i = 0;
		int a = 0;
		int s = 0;
		for (auto& warp : mWarpList) {
			a = warp->getAFboIndex();
			if (a < 0) a = 0; // TODO 20200228 a could be negative if warps3.xml > warps01.json
			i = math<int>::min(a, getFboShaderListSize() - 1);
			s = getFboShaderListSize(); // TMP
			//if (isFboValid(i)) {
				//ko  warp->draw(getFboRenderedTexture(0));
				//ko warp->draw(getFboTexture(0)); bind to 0 broken
			warp->draw(mVDMix->getRenderedMixetteTexture(0));
			//}

		}
		//gl::color(0.5, 0.0, 1.0, 0.4f);
		//gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth()/2, mVDParams->getFboHeight()/2));
		mWarpTexture = mWarpsFbo->getColorTexture();
	}
}
bool VDSession::save()
{
	/* 20201229
	saveFbos();*/
	saveWarps();
	// save uniforms settings
	//mVDAnimation->save();
	// save in sessionPath
	// TODO add shaders section
	JsonTree doc;

	JsonTree settings = JsonTree::makeArray("settings");
	settings.addChild(ci::JsonTree("bpm", mOriginalBpm));
	settings.addChild(ci::JsonTree("beatsperbar", mVDUniforms->getIntUniformValueByName("iBeatsPerBar")));
	//settings.addChild(ci::JsonTree("fadeindelay", mFadeInDelay));
	//settings.addChild(ci::JsonTree("fadeoutdelay", mFadeOutDelay));
	settings.addChild(ci::JsonTree("endframe", mVDAnimation->mEndFrame));
	doc.pushBack(settings);

	/*JsonTree assets = JsonTree::makeArray("assets");
	if (mWaveFileName.length() > 0) assets.addChild(ci::JsonTree("wavefile", mWaveFileName));
	assets.addChild(ci::JsonTree("waveplaybackdelay", mWavePlaybackDelay));
	if (mMovieFileName.length() > 0) assets.addChild(ci::JsonTree("moviefile", mMovieFileName));
	assets.addChild(ci::JsonTree("movieplaybackdelay", mMoviePlaybackDelay));
	if (mImageSequencePath.length() > 0) assets.addChild(ci::JsonTree("imagesequencepath", mImageSequencePath));
	if (mText.length() > 0) {
		assets.addChild(ci::JsonTree("text", mText));
		assets.addChild(ci::JsonTree("textplaybackdelay", mTextPlaybackDelay));
		assets.addChild(ci::JsonTree("textplaybackend", mTextPlaybackEnd));
	}
	doc.pushBack(assets);*/

	doc.write(writeFile(sessionPath), JsonTree::WriteOptions());

	return true;
}

void VDSession::restore()
{
	// save load settings
	//load();

	// check to see if json file exists
	if (!fs::exists(sessionPath)) {
		return;
	}

	try {
		JsonTree doc(loadFile(sessionPath));
		/*if (doc.hasChild("shaders")) {
			JsonTree shaders(doc.getChild("shaders"));
			if (shaders.hasChild("0")) createShaderFbo(shaders.getValueForKey<string>("0"));
			if (shaders.hasChild("1")) createShaderFbo(shaders.getValueForKey<string>("1"));
			if (shaders.hasChild("2")) createShaderFbo(shaders.getValueForKey<string>("2"));
			if (shaders.hasChild("3")) createShaderFbo(shaders.getValueForKey<string>("3"));
		}*/
		if (doc.hasChild("settings")) {
			JsonTree settings(doc.getChild("settings"));
			if (settings.hasChild("bpm")) {
				mOriginalBpm = settings.getValueForKey<float>("bpm", 166.0f);
				//CI_LOG_W("getBpm" + toString(mVDAnimation->getBpm()) + " mOriginalBpm " + toString(mOriginalBpm));
				mVDUniforms->setUniformValue(mVDUniforms->IBPM, mOriginalBpm);
				//CI_LOG_W("getBpm" + toString(mVDAnimation->getBpm()));
			};
			if (settings.hasChild("beatsperbar")) mVDUniforms->setUniformValue(mVDUniforms->IBEATSPERBAR, settings.getValueForKey<int>("beatsperbar"));
			if (mVDUniforms->getUniformValue(mVDUniforms->IBEATSPERBAR) < 1) mVDUniforms->setUniformValue(mVDUniforms->IBEATSPERBAR, 4);
			//if (settings.hasChild("fadeindelay")) mFadeInDelay = settings.getValueForKey<int>("fadeindelay");
			//if (settings.hasChild("fadeoutdelay")) mFadeOutDelay = settings.getValueForKey<int>("fadeoutdelay");
			if (settings.hasChild("endframe")) mVDAnimation->mEndFrame = settings.getValueForKey<int>("endframe");
			//CI_LOG_W("getBpm" + toString(mVDAnimation->getBpm()) + " mTargetFps " + toString(mTargetFps));
			mTargetFps = mVDUniforms->getUniformValue(mVDUniforms->IBPM) / 60.0f * mFpb;
			//CI_LOG_W("getBpm" + toString(mVDAnimation->getBpm()) + " mTargetFps " + toString(mTargetFps));
		}

		/*if (doc.hasChild("assets")) {
			JsonTree assets(doc.getChild("assets"));
			if (assets.hasChild("wavefile")) mWaveFileName = assets.getValueForKey<string>("wavefile");
			if (assets.hasChild("waveplaybackdelay")) mWavePlaybackDelay = assets.getValueForKey<int>("waveplaybackdelay");
			if (assets.hasChild("moviefile")) mMovieFileName = assets.getValueForKey<string>("moviefile");
			if (assets.hasChild("movieplaybackdelay")) mMoviePlaybackDelay = assets.getValueForKey<int>("movieplaybackdelay");
			if (assets.hasChild("imagesequencepath")) mImageSequencePath = assets.getValueForKey<string>("imagesequencepath");
			if (assets.hasChild("text")) mText = assets.getValueForKey<string>("text");
			if (assets.hasChild("textplaybackdelay")) mTextPlaybackDelay = assets.getValueForKey<int>("textplaybackdelay");
			if (assets.hasChild("textplaybackend")) mTextPlaybackEnd = assets.getValueForKey<int>("textplaybackend");
		}*/

	}
	catch (const JsonTree::ExcJsonParserError& exc) {
		CI_LOG_W(exc.what());
	}
}

void VDSession::resetSomeParams() {
	// parameters not exposed in json file
	mFpb = 16;
	mVDUniforms->setUniformValue(mVDUniforms->IBPM, mOriginalBpm);
	mTargetFps = mOriginalBpm / 60.0f * mFpb;
}

void VDSession::reset()
{
	// parameters exposed in json file
	mOriginalBpm = 166;
	/* TODO 20200221
	mWaveFileName = "";
	mWavePlaybackDelay = 10;
	mMovieFileName = "";
	mImageSequencePath = "";
	mMoviePlaybackDelay = 10;
	mFadeInDelay = 5;
	mFadeOutDelay = 1;
	mText = "";
	mTextPlaybackDelay = 10;
	mTextPlaybackEnd = 2020000;*/
	mVDAnimation->mEndFrame = 20000000;

	resetSomeParams();
}

void VDSession::blendRenderEnable(bool render) {
	mVDAnimation->blendRenderEnable(render);
}

void VDSession::fileDrop(FileDropEvent event) {
	std::string ext = "";
	//string fileName = "";

	unsigned int index = (unsigned int)( (event.getX() - mVDParams->getUILargeW() ) / ( mVDParams->getUILargePreviewW() + mVDParams->getUIMargin() ) );
	int y = (int)(event.getY());
	//if (index < 2 || y < mVDSettings->uiYPosRow3 || y > mVDSettings->uiYPosRow3 + mVDSettings->uiPreviewH) index = 0;
	ci::fs::path mPath = event.getFile(event.getNumFiles() - 1);
	std::string absolutePath = mPath.string();
	// use the last of the dropped files
	int dotIndex = absolutePath.find_last_of(".");
	int slashIndex = absolutePath.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) {
		ext = absolutePath.substr(dotIndex + 1);
		//fileName = absolutePath.substr(slashIndex + 1, dotIndex - slashIndex - 1);
		if (ext == "json") {
			JsonTree json(loadFile(absolutePath));
			fboFromJson(json);
		}
		
		else if (ext == "glsl" || ext == "frag" || ext == "fs") {
			loadFragmentShader(absolutePath, index);

			// façade fbo()->shader()->uniform()->execute() 
		}
		else if (ext == "png" || ext == "jpg") {
			if (index < 1) index = 1;
			if (index > 3) index = 3;
			loadImageFile(absolutePath, index);
		}
		/*else if (ext == "wav" || ext == "mp3") {
			loadAudioFile(absolutePath);
		}
		else if (ext == "xml") {
		}
		else if (ext == "mov") {
			loadMovie(absolutePath, index);
		}
		else if (ext == "txt") {
		}
		else if (ext == "") {
			// try loading image sequence from dir
			if (!loadImageSequence(absolutePath, index)) {
				// try to load a folder of shaders
				loadShaderFolder(absolutePath);
			}
		}*/
	}
}

#pragma region events
bool VDSession::handleMouseMove(MouseEvent& event)
{
	bool handled = true;
	// 20180318 handled in VDUIMouse mVDAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	// pass this mouse event to the warp editor first
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleMouseDown(MouseEvent& event)
{
	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}
	// 20180318 handled in VDUIMouse mVDAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleMouseDrag(MouseEvent& event)
{

	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}
	// 20180318 handled in VDUIMouse mVDAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleMouseUp(MouseEvent& event)
{
	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}

	// 20180318 handled in VDUIMouse mVDAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	event.setHandled(handled);
	return event.isHandled();
}


bool VDSession::handleKeyDown(KeyEvent& event)
{
	bool handled = true;
	float newValue;
#if defined( CINDER_COCOA )
	bool isModDown = event.isMetaDown();
#else // windows
	bool isModDown = event.isControlDown();
#endif
	bool isShiftDown = event.isShiftDown();
	bool isAltDown = event.isAltDown();
	CI_LOG_V("session keydown: " + toString(event.getCode()) + " ctrl: " + toString(isModDown) + " shift: " + toString(isShiftDown) + " alt: " + toString(isAltDown));
	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarpList, event)) {

		switch (event.getCode()) {
		case KeyEvent::KEY_w:
			CI_LOG_V("oscConnect");
			if (isModDown) {
				//oscConnect();
			}
			else {
				// handled in main app
				//handled = false;
				// toggle warp edit mode
				Warp::enableEditMode(!Warp::isEditModeEnabled());
			}
			break;

		case KeyEvent::KEY_F1:
			mMode = 0;
			break;
		case KeyEvent::KEY_F2:
			mMode = 1;
			break;
		case KeyEvent::KEY_F3:
			mMode = 2;
			break;
		case KeyEvent::KEY_F4:
			mMode = 3;
			break;
		case KeyEvent::KEY_F5:
			mMode = 4;
			break;
		case KeyEvent::KEY_F6:
			mMode = 5;
			break;
		case KeyEvent::KEY_F7:
			mMode = 6;
			break;
		case KeyEvent::KEY_F8:
			mMode = 7;
			break;
		case KeyEvent::KEY_F9:
			mMode = 8;
			break;

		
		case KeyEvent::KEY_UP:
			// imgseq next
			//incrementSequencePosition();
			break;
		case KeyEvent::KEY_DOWN:
			// imgseq next
			//decrementSequencePosition();
			break;
		case KeyEvent::KEY_v:
			//if (isModDown) fboFlipV(0);// TODO other indexes mVDSettings->mFlipV = !mVDSettings->mFlipV; useless?
			break;
		case KeyEvent::KEY_h:
			if (isModDown) {
				//fboFlipH(0);// TODO other indexes mVDSettings->mFlipH = !mVDSettings->mFlipH; useless?
			}
			else {
				// ui visibility
				toggleUI();
			}
			break;
		case KeyEvent::KEY_d:
			/*if (isAltDown) {
				setSpeed(0, getSpeed(0) - 0.01f);
			}
			else {
				setSpeed(0, getSpeed(0) + 0.01f);
			}*/
			break;
		default:
			CI_LOG_V("session keydown: " + toString(event.getCode()));
			handled = false;
			break;
		}
	}
	CI_LOG_V((handled ? "session keydown handled " : "session keydown not handled "));
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleKeyUp(KeyEvent& event) {
	bool handled = true;

	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarpList, event)) {
		if (!mVDAnimation->handleKeyUp(event)) {
			// Animation did not handle the key, so handle it here
			switch (event.getCode()) {
			
			default:
				CI_LOG_V("session keyup: " + toString(event.getCode()));
				handled = false;
				break;
			}
		}
	}
	CI_LOG_V((handled ? "session keyup handled " : "session keyup not handled "));
	event.setHandled(handled);
	return event.isHandled();
}
#pragma endregion events
// fbos
#pragma region fbos

#pragma endregion fbos
// shaders

// mix
#pragma region mix


unsigned int VDSession::fboFromJson(const JsonTree& json, unsigned int aFboIndex) {
	unsigned int rtn = 0;

	rtn = createFboShaderTexture(json, aFboIndex);
	return rtn;
}

#pragma endregion mix
ci::gl::TextureRef VDSession::getFboRenderedTexture(unsigned int aFboIndex) {
	return mVDMix->getFboRenderedTexture(aFboIndex);
}
ci::gl::TextureRef VDSession::getFboTexture(unsigned int aFboIndex) {
	return mVDMix->getFboTexture(aFboIndex);
}
/*void VDSession::setFragmentShaderString(unsigned int aFboShaderIndex, const std::string& aFragmentShaderString, const std::string& aName) {
	//mFboShaderList[aFboShaderIndex]->setFragmentShaderString(aFboShaderIndex, aFragmentShaderString, aName);
	mVDMix->setFragmentShaderString
}*/
ci::gl::TextureRef VDSession::getFboShaderTexture(unsigned int aFboShaderIndex) {
	//return mFboShaderList[aFboShaderIndex]->getRenderedTexture(); //20201229 or getTexture?
	return mVDMix->getFboRenderedTexture(aFboShaderIndex); //20201229 or getTexture?
};

std::vector<ci::gl::GlslProg::Uniform> VDSession::getFboShaderUniforms(unsigned int aFboShaderIndex) {
	//return mFboShaderList[aFboShaderIndex]->getUniforms();
	return mVDMix->getFboShaderUniforms(aFboShaderIndex);
}

int VDSession::getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex) {
	//return mFboShaderList[aFboShaderIndex]->getUniformValueByLocation(aLocationIndex);
	return mVDMix->getUniformValueByLocation(aFboShaderIndex, aLocationIndex);
};
void VDSession::setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue) {
	//mFboShaderList[aFboShaderIndex]->setUniformValueByLocation(aLocationIndex, aValue);
	mVDMix->setUniformValueByLocation(aFboShaderIndex, aLocationIndex, aValue);
};
ci::gl::TextureRef VDSession::getMixetteTexture(unsigned int aFboIndex) {
	return mVDMix->getMixetteTexture(aFboIndex);
}
ci::gl::TextureRef VDSession::getRenderedMixetteTexture(unsigned int aFboIndex) {
	return mVDMix->getRenderedMixetteTexture(aFboIndex);
}
ci::gl::TextureRef VDSession::getPostFboTexture() {
	return mPostFbo->getColorTexture();
};
ci::gl::TextureRef VDSession::getWarpFboTexture() {
	return mWarpsFbo->getColorTexture();
};
ci::gl::TextureRef VDSession::getRenderedWarpFboTexture() {
	return mWarpTexture;
};
void VDSession::resize() {
	//mRenderFbo = gl::Fbo::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, fboFmt);
	// tell the fbos our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarpList);
	Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
}
unsigned int VDSession::getWarpCount() { return mWarpList.size(); };
std::string	 VDSession::getWarpName(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getName(); };// or trycatch
int VDSession::getWarpWidth(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getWidth(); };
int VDSession::getWarpHeight(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getHeight(); };
void VDSession::setWarpWidth(unsigned int aWarpIndex, int aWidth) {
	mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->setWidth(aWidth);
	mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->resize();
};
void VDSession::setWarpHeight(unsigned int aWarpIndex, int aHeight) {
	Warp::handleResize(mWarpList);
	Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), aHeight));
	mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->setHeight(aHeight);

};
unsigned int VDSession::getWarpAFboIndex(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getAFboIndex(); };
unsigned int VDSession::getWarpBFboIndex(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getBFboIndex(); };
void VDSession::setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) {
	if (aWarpIndex < mWarpList.size() && aWarpFboIndex < mVDMix->getFboShaderListSize()) {
		mWarpList[aWarpIndex]->setAFboIndex(aWarpFboIndex);
		/* 20201229
updateWarpName(aWarpIndex);*/
	}
}
void VDSession::setWarpBFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) {
	if (aWarpIndex < mWarpList.size() && aWarpFboIndex < mVDMix->getFboShaderListSize()) {
		mWarpList[aWarpIndex]->setBFboIndex(aWarpFboIndex);
		/* 20201229
updateWarpName(aWarpIndex);*/
	}
}

void VDSession::createWarp() {
	auto warp = WarpBilinear::create();
	warp->setName("New");
	warp->setAFboIndex(0);
	warp->setBFboIndex(0);
	warp->setAShaderIndex(0);
	warp->setBShaderIndex(0);
	warp->setAShaderFilename("inputImage.fs");
	warp->setBShaderFilename("inputImage.fs");
	warp->setATextureFilename("audio");
	warp->setBTextureFilename("audio");
	mWarpList.push_back(WarpBilinear::create());
}
std::string VDSession::getFboShaderName(unsigned int aFboIndex) {
	return mVDMix->getFboShaderName(aFboIndex);
}
std::string VDSession::getFboTextureName(unsigned int aFboIndex) {
	return mVDMix->getFboTextureName(aFboIndex);
}
void VDSession::saveWarps() {
	/*int i = 0;
	for (auto &warp : mWarpList) {
		//
		warp->setAShaderFilename(getFboShaderName(warp->getAFboIndex()));
		warp->setATextureFilename(getFboTextureName(warp->getAFboIndex()));
		JsonTree		json;
		string jsonFileName = "warp" + toString(i) + ".json";
		fs::path jsonFile = getAssetPath("") / mVDSettings->mAssetsPath / jsonFileName;
		// write file
		json.pushBack(warp->toJson());
		json.write(jsonFile);
		i++;
	}
	// save warp settings*/
	Warp::writeSettings(mWarpList, writeFile(mSettings));
}
void VDSession::setAnim(unsigned int aCtrl, unsigned int aAnim) {
	mVDAnimation->setAnim(aCtrl, aAnim);
}
// control values
void VDSession::toggleValue(unsigned int aCtrl) {
	//! 20200526 mVDSocketio->toggleValue(aCtrl);
};

float VDSession::getMinUniformValue(unsigned int aIndex) {
	return mVDUniforms->getMinUniformValue(aIndex);
};
float VDSession::getMaxUniformValue(unsigned int aIndex) {
	return mVDUniforms->getMaxUniformValue(aIndex);
};
int VDSession::getSampler2DUniformValueByName(const std::string& aName) {
	return mVDUniforms->getSampler2DUniformValueByName(aName);
};
int VDSession::getIntUniformValueByName(const std::string& aName) {
	return mVDUniforms->getIntUniformValueByName(aName);
};
int VDSession::getIntUniformValueByIndex(unsigned int aCtrl) {
	return mVDUniforms->getIntUniformValueByIndex(aCtrl);
};
bool VDSession::getBoolUniformValueByName(const std::string& aName) {
	return mVDUniforms->getBoolUniformValueByName(aName);
};
bool VDSession::getBoolUniformValueByIndex(unsigned int aCtrl) {
	return mVDUniforms->getBoolUniformValueByIndex(aCtrl);
}

float VDSession::getUniformValueByName(const std::string& aCtrlName) {
	return mVDUniforms->getUniformValueByName(aCtrlName);
};
