
 
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// Animation
#include "VDAnimation.h"
// Session Facade
#include "VDSessionFacade.h"
// Spout
#include "CiSpoutOut.h"
// Video
//#include "ciWMFVideoPlayer.h"
// Uniforms
#include "VDUniforms.h"
// Params
#include "VDParams.h"
// Mix
#include "VDMix.h"

using namespace ci;
using namespace ci::app;
using namespace videodromm;

class _TBOX_PREFIX_App : public App {
public:
	_TBOX_PREFIX_App();
	void cleanup() override;
	void update() override;
	void draw() override;
	void resize() override;
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
private:
	// Settings
	VDSettingsRef					mVDSettings;
	// Animation
	VDAnimationRef					mVDAnimation;
	// Session
	VDSessionFacadeRef				mVDSessionFacade;
	// Mix
	VDMixRef						mVDMix;
	// Uniform
	VDUniformRef					mVDUniform;
	// Params
	VDParamsRef						mVDParams;
	// video
	/*ciWMFVideoPlayer				mVideo;
	float							mVideoPos;
	float							mVideoDuration;
	bool							mIsVideoLoaded;*/

	bool							mFadeInDelay = true;
	void							toggleCursorVisibility(bool visible);
	SpoutOut 						mSpoutOut;
};


_TBOX_PREFIX_App::_TBOX_PREFIX_App() : mSpoutOut("VDR", app::getWindowSize())
{

	// Settings
	mVDSettings = VDSettings::create("VDR");
	// Uniform
	mVDUniforms = VDUniforms::create();
	// Params
	mVDParams = VDParams::create();
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDUniforms);
	// Mix
	mVDMix = VDMix::create(mVDSettings, mVDAnimation, mVDUniforms);
	// Session
	mVDSessionFacade = VDSessionFacade::createVDSession(mVDSettings, mVDAnimation, mVDUniforms, mVDMix)
		->setUniformValue(mVDUniforms->IBPM, 160.0f)
		->setUniformValue(mVDUniforms->IMOUSEX, 0.27710f)
		->setUniformValue(mVDUniforms->IMOUSEY, 0.5648f)
		->setMode(1)
		->setupWSClient()
		->wsConnect()
		//->setupOSCReceiver()
		//->addOSCObserver(mVDSettings->mOSCDestinationHost, mVDSettings->mOSCDestinationPort)
		->addUIObserver(mVDSettings, mVDUniforms)
		->toggleUI()
		->toggleValue(mVDUniforms->IFLIPV);

	// sos only mVDSessionFacade->setUniformValue(mVDSettings->IEXPOSURE, 1.93f);
	mFadeInDelay = true;
	
	/*fs::path texFileOrPath = getAssetPath("") / mVDSettings->mAssetsPath / "accueil.mp4";
	if (fs::exists(texFileOrPath)) {
		string ext = "";
		int dotIndex = texFileOrPath.filename().string().find_last_of(".");
		if (dotIndex != std::string::npos) ext = texFileOrPath.filename().string().substr(dotIndex + 1);
		if (ext == "mp4" || ext == "wmv" || ext == "avi" || ext == "mov") {
			if (!mVideo.isStopped()) {
				mVideo.stop();
			}

			mIsVideoLoaded = mVideo.loadMovie(texFileOrPath);

			mVideoDuration = mVideo.getDuration();
			mVideoPos = mVideo.getPosition();
			mVideo.play();

		}
	}*/
}

void _TBOX_PREFIX_App::toggleCursorVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}

void _TBOX_PREFIX_App::fileDrop(FileDropEvent event)
{
	mVDSessionFacade->fileDrop(event);
}

void _TBOX_PREFIX_App::mouseMove(MouseEvent event)
{
	if (!mVDSessionFacade->handleMouseMove(event)) {

	}
}

void _TBOX_PREFIX_App::mouseDown(MouseEvent event)
{

	if (!mVDSessionFacade->handleMouseDown(event)) {

	}
}

void _TBOX_PREFIX_App::mouseDrag(MouseEvent event)
{

	if (!mVDSessionFacade->handleMouseDrag(event)) {

	}
}

void _TBOX_PREFIX_App::mouseUp(MouseEvent event)
{

	if (!mVDSessionFacade->handleMouseUp(event)) {

	}
}

void _TBOX_PREFIX_App::keyDown(KeyEvent event)
{

	// warp editor did not handle the key, so handle it here
	if (!mVDSessionFacade->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_F12:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_f:
			// toggle full screen
			setFullScreen(!isFullScreen());
			break;

		case KeyEvent::KEY_l:
			mVDSessionFacade->createWarp();
			break;
		}
	}
}

void _TBOX_PREFIX_App::keyUp(KeyEvent event)
{

	// let your application perform its keyUp handling here
	if (!mVDSessionFacade->handleKeyUp(event)) {
		/*switch (event.getCode()) {
		default:
			CI_LOG_V("main keyup: " + toString(event.getCode()));
			break;
		}*/
	}
}
void _TBOX_PREFIX_App::cleanup()
{
	CI_LOG_V("cleanup and save");
	ui::Shutdown();
	mVDSessionFacade->saveWarps();
	mVDSettings->save();
	CI_LOG_V("quit");
}

void _TBOX_PREFIX_App::update()
{
	switch (mVDSessionFacade->getCmd()) {
	case 0:
		//createControlWindow();
		break;
	case 1:
		//deleteControlWindows();
		break;
	}
	mVDSessionFacade->setUniformValue(mVDUniform->IFPS, getAverageFps());
	mVDSessionFacade->update();
	/*mVideo.update();
	mVideoPos = mVideo.getPosition();
	if (mVideo.isStopped() || mVideo.isPaused()) {
		mVideo.setPosition(0.0);
		mVideo.play();
	}*/
}


void _TBOX_PREFIX_App::resize()
{
}
void _TBOX_PREFIX_App::draw()
{
	// clear the window and set the drawing color to black
	gl::clear();
	gl::color(Color::white());
	if (mFadeInDelay) {
		mVDSettings->iAlpha = 0.0f;
		if (getElapsedFrames() > 10.0) {// mVDSessionFacade->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	else {
		gl::setMatricesWindow(mVDParams->getFboWidth(), mVDParams->getFboHeight(), false);
		//gl::setMatricesWindow(mVDSessionFacade->getIntUniformValueByIndex(mVDSettings->IOUTW), mVDSessionFacade->getIntUniformValueByIndex(mVDSettings->IOUTH), true);
		// textures needs updating
		for (int t = 0; t < mVDSessionFacade->getInputTexturesCount(); t++) {
			mVDSessionFacade->getInputTexture(t);
		}
		int m = mVDSessionFacade->getMode();
		if (m < mVDSessionFacade->getFboShaderListSize()) {
			gl::draw(mVDSessionFacade->getFboShaderTexture(m));
			mSpoutOut.sendTexture(mVDSessionFacade->getFboShaderTexture(m));
		}
		else {
			gl::draw(mVDSessionFacade->buildRenderedMixetteTexture(0), Area(50, 50, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
			//gl::draw(mVDSessionFacade->getPostFboTexture(), Area(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));
			//gl::draw(mVDSession->getRenderedMixetteTexture(0), Area(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));
			// ok gl::draw(mVDSession->getWarpFboTexture(), Area(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));//getWindowBounds()
			//mSpoutOut.sendTexture(mVDSession->getRenderedMixetteTexture(0));
		}
		/*vec2 videoSize = vec2(mVideo.getWidth(), mVideo.getHeight());
		mGlslVideoTexture->uniform("uVideoSize", videoSize);
		videoSize *= 0.25f;
		videoSize *= 0.5f;
		ciWMFVideoPlayer::ScopedVideoTextureBind scopedVideoTex(mVideo, 0);
		gl::scale(vec3(videoSize, 1.0f));*/

		//gl::draw(mPostFbo->getColorTexture());
		//gl::draw(mVDSessionFacade->getFboRenderedTexture(0));
	}
	// Spout Send
	// KO mSpoutOut.sendViewport();
	// OK
	 mSpoutOut.sendTexture(mVDSessionFacade->buildRenderedMixetteTexture(0));

	getWindow()->setTitle(toString((int)getAverageFps()) + " fps");
}
void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(1280, 720);
}
CINDER_APP(_TBOX_PREFIX_App, RendererGl(RendererGl::Options().msaa(8)),  prepareSettings)
