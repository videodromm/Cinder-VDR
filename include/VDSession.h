#pragma once
/*
	VDSession
	Top hierarchy class to use all the child classes
*/
// TODO implement factory or builder?

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
// json
#include "cinder/Json.h"

// Settings
#include "VDSettings.h"

// Params
#include "VDParams.h"
// Animation
#include "VDAnimation.h"
// Fbos
#include "VDFbo.h"
// Uniforms
#include "VDUniforms.h"

// Mix
#include "VDMix.h"
// Warping
#include "Warp.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;

namespace videodromm {

	typedef std::shared_ptr<class VDSession> VDSessionRef;
	// pattern State
	enum STATE {
		STATE_LOAD_FILE = 0,
		STATE_SHADER = 1,
		STATE_UNIFORMS = 2,
		STATE_FAILED_COMPILE = 3,
		STATE_SUCCESS_COMPILE = 4
	};
	
	class VDSession {
	public:
		STATE state;
		VDSession(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);

		bool							handleKeyDown(KeyEvent& event);
		bool							handleKeyUp(KeyEvent& event);
		void							update(unsigned int aClassIndex = 0);
		
		void							loadFromJsonFile(const fs::path& jsonFile);
		
		//! Mix
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getMixetteTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getRenderedMixetteTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getPostFboTexture();
		ci::gl::TextureRef				getWarpFboTexture();
		ci::gl::TextureRef				getRenderedWarpFboTexture();
		//!
		void							reset();
		void							resetSomeParams();
		void							resize();
		unsigned int					getWarpCount();
		std::string						getWarpName(unsigned int aWarpIndex);// or trycatch
		int								getWarpWidth(unsigned int aWarpIndex);
		int								getWarpHeight(unsigned int aWarpIndex);
		void							setWarpWidth(unsigned int aWarpIndex, int aWidth);
		void							setWarpHeight(unsigned int aWarpIndex, int aHeight);
		unsigned int					getWarpAFboIndex(unsigned int aWarpIndex);
		unsigned int					getWarpBFboIndex(unsigned int aWarpIndex);
		void							setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex);
		void							setWarpBFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex);

		void							createWarp();
		std::string						getFboShaderName(unsigned int aFboIndex);
		std::string						getFboTextureName(unsigned int aFboIndex);
		void							saveWarps();

		bool							handleMouseMove(MouseEvent& event);
		bool							handleMouseDown(MouseEvent& event);
		bool							handleMouseDrag(MouseEvent& event);
		bool							handleMouseUp(MouseEvent& event);
		bool							save();
		void							restore();

		void							setAnim(unsigned int aCtrl, unsigned int aAnim);
		// control values
		void							toggleValue(unsigned int aCtrl);
		float							getMinUniformValue(unsigned int aIndex);
		float							getMaxUniformValue(unsigned int aIndex);
		int								getSampler2DUniformValueByName(const std::string& aName);
		int								getIntUniformValueByName(const std::string& aName);
		int								getIntUniformValueByIndex(unsigned int aCtrl);
		bool							getBoolUniformValueByName(const std::string& aName);
		bool							getBoolUniformValueByIndex(unsigned int aCtrl);
		float							getUniformValueByName(const std::string& aCtrlName);

		// tempo
		void							tapTempo() { mVDAnimation->tapTempo(); };
		void							toggleUseTimeWithTempo() { mVDAnimation->toggleUseTimeWithTempo(); };
		void							useTimeWithTempo() { mVDAnimation->useTimeWithTempo(); };
		bool							getUseTimeWithTempo() { return mVDAnimation->getUseTimeWithTempo(); };
		//void							setTimeFactor(const int &aTimeFactor) { mVDAnimation->setTimeFactor(aTimeFactor); };
		// audio
		ci::gl::TextureRef				getAudioTexture() { return mVDAnimation->getAudioTexture(); };
		std::string						getAudioTextureName() { return mVDAnimation->getAudioTextureName(); };
		float*							getFreqs() { return mVDAnimation->iFreqs; };
		int								getFreqIndexSize() { return mVDAnimation->getFreqIndexSize(); };
		float							getFreq(unsigned int aFreqIndex) { return mVDAnimation->getUniformValue(mVDUniforms->IFREQ0 + aFreqIndex); };
		int								getFreqIndex(unsigned int aFreqIndex) { return mVDAnimation->getFreqIndex(aFreqIndex); };
		void							setFreqIndex(unsigned int aFreqIndex, unsigned int aFreq) { mVDAnimation->setFreqIndex(aFreqIndex, aFreq); };
		int								getFFTWindowSize() { return mVDAnimation->mFFTWindowSize; };
		bool							isAudioBuffered() { return mVDAnimation->isAudioBuffered(); };
		void							toggleAudioBuffered() { mVDAnimation->toggleAudioBuffered(); };
		bool							getUseLineIn() { return mVDAnimation->getUseLineIn(); };
		void							setUseLineIn(bool useLineIn) { mVDAnimation->setUseLineIn(useLineIn); };
		void							toggleUseLineIn() { mVDAnimation->toggleUseLineIn(); };
		int								loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex = 4) {
			return mVDMix->loadFragmentShader(aFilePath, aFboShaderIndex);
		};
		/*bool							getFreqWSSend() { return mFreqWSSend; };
		void							toggleFreqWSSend() { mFreqWSSend = !mFreqWSSend; };
		// uniforms
		//void							setMixCrossfade(unsigned int aWarpIndex, float aCrossfade) { mVDSettings->xFade = aCrossfade; mVDSettings->xFadeChanged = true; };
		//float							getMixCrossfade(unsigned int aWarpIndex) { return mVDSettings->xFade; };
		float							getCrossfade() {
			return mVDAnimation->getUniformValue(mVDUniforms->IXFADE);
		};
		void							setCrossfade(float aCrossfade) {
			mVDAnimation->setUniformValue(mVDUniforms->IXFADE, aCrossfade);
		};
		void							setFboAIndex(unsigned int aIndex, unsigned int aFboIndex);
		void							setFboBIndex(unsigned int aIndex, unsigned int aFboIndex);
		unsigned int					getFboAIndex(unsigned int aIndex) { return mVDAnimation->getIntUniformValueByName("iFboA"); };
		unsigned int					getFboBIndex(unsigned int aIndex) { return mVDAnimation->getIntUniformValueByName("iFboB"); };

		void							setFboFragmentShaderIndex(unsigned int aFboIndex, unsigned int aFboShaderIndex);
		unsigned int					getFboFragmentShaderIndex(unsigned int aFboIndex);
		bool							loadShaderFolder(const string& aFolder);

		unsigned int					getShadersCount() { return mShaderList.size(); };
		string							getShaderName(unsigned int aShaderIndex);
		ci::gl::TextureRef				getShaderThumb(unsigned int aShaderIndex);
		string							getFragmentString(unsigned int aShaderIndex) { return mShaderList[aShaderIndex]->getFragmentString(); };
		void							setFragmentShaderString(unsigned int aShaderIndex, const string& aFragmentShaderString, const string& aName = "");
		//string							getVertexShaderString(unsigned int aShaderIndex) { return mVDMix->getVertexShaderString(aShaderIndex); };
		string							getFragmentShaderString(unsigned int aShaderIndex);
		//string							getVertexShaderString(unsigned int aShaderIndex) { return mVDMix->getVertexShaderString(aShaderIndex); };
		void							setHydraFragmentShaderString(const string& aFragmentShaderString, const string& aName = "");
		string							getHydraFragmentShaderString();
		void							updateShaderThumbFile(unsigned int aShaderIndex);
		void							removeShader(unsigned int aShaderIndex);

		// file operations (filedrop, etc)
		//int								loadFileFromAbsolutePath(const string& aAbsolutePath, int aIndex = 0);

		unsigned int					getFboInputTextureIndex(unsigned int aFboIndex) ;

		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex = 0);
		ci::gl::TextureRef				getFboThumb(unsigned int aBlendIndex) { return mBlendFbos[aBlendIndex]->getColorTexture(); };
		unsigned int					createShaderFboFromString(const string& aFragmentShaderString, const string& aShaderFilename);*/
		int								getFboTextureWidth(unsigned int aFboIndex) {
			return mVDMix->getFboInputTextureWidth(aFboIndex);
		};
		int								getFboTextureHeight(unsigned int aFboIndex) {
			return mVDMix->getFboInputTextureHeight(aFboIndex);
		};
		// utils

		float							getTargetFps() { return mTargetFps; };
		void							blendRenderEnable(bool render);
		void							fileDrop(FileDropEvent event);
		void							setFboInputTexture(unsigned int aFboIndex, unsigned int aInputTextureIndex) {
			//mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->setInputTexture(mTextureList[aInputTextureIndex]->getTexture());
		}
		// utils
		int								getWindowsResolution() {
			mVDSettings->mDisplayCount = 0;
			for (auto display : Display::getDisplays())
			{
				//CI_LOG_V("VDUtils Window #" + toString(mVDSettings->mDisplayCount) + ": " + toString(display->getWidth()) + "x" + toString(display->getHeight()));
				mVDSettings->mDisplayCount++;
			}
			int w = Display::getMainDisplay()->getWidth();
			int h = Display::getMainDisplay()->getHeight();
			// Display sizes
			if (mVDSettings->mAutoLayout)
			{
				mVDSettings->mRenderX = 0;
				mVDSettings->mRenderY = 0;
				mVDSettings->mMainWindowWidth = w;
				mVDSettings->mMainWindowHeight = h;
				// in case only one screen, render from x = 0
				if (mVDSettings->mDisplayCount == 1) {
					mVDSettings->mRenderX = 0;
					mVDSettings->mRenderWidth = mVDSettings->mMainWindowWidth;
					mVDSettings->mRenderHeight = mVDSettings->mMainWindowHeight;
				}
				else {
					mVDSettings->mRenderX = mVDSettings->mMainWindowWidth;
					// TODO for MODE_MIX and triplehead(or doublehead), we might only want 1/3 of the screen centered	
					for (auto display : Display::getDisplays())
					{
						//CI_LOG_V("VDUtils Window #" + toString(mVDSettings->mDisplayCount) + ": " + toString(display->getWidth()) + "x" + toString(display->getHeight()));

						mVDSettings->mRenderWidth += display->getWidth();
						mVDSettings->mRenderHeight = display->getHeight();

					}
					mVDSettings->mRenderWidth -= mVDSettings->mMainWindowWidth;
				}


			}

			//CI_LOG_V("VDUtils mMainDisplayWidth:" + toString(mVDSettings->mMainWindowWidth) + " mMainDisplayHeight:" + toString(mVDSettings->mMainWindowHeight));
			//CI_LOG_V("VDUtils mRenderWidth:" + toString(mVDSettings->mRenderWidth) + " mRenderHeight:" + toString(mVDSettings->mRenderHeight));
			//CI_LOG_V("VDUtils mRenderX:" + toString(mVDSettings->mRenderX) + " mRenderY:" + toString(mVDSettings->mRenderY));
			//mVDSettings->mRenderResoXY = vec2(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);	
			//splitWarp(mVDParams->getFboWidth(), mVDParams->getFboHeight());	
			return w;

		};
		// fbos
		std::string						getFboName(unsigned int aFboIndex) {
			return mVDMix->getFboName(aFboIndex);
		};

		unsigned int					getFboListSize() { return mVDMix->getFboListSize(); };
		unsigned int 					createFboShaderTexture(const JsonTree& json, unsigned int aFboIndex = 0) {
			return mVDMix->createFboShaderTexture(json, aFboIndex);
		};
		unsigned int					fboFromJson(const JsonTree& json, unsigned int aFboIndex = 0);

		void							saveFbos() {
			mVDMix->saveFbos();
		};

		bool							isFboValid(unsigned int aFboIndex) {
			return mVDMix->isFboValid(aFboIndex);
		};

		// fbo 
		/*bool							getFboBoolUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex) {
			return mVDMix->getFboBoolUniformValueByIndex(aCtrl, aFboIndex);
		};

		void							toggleFboValue(unsigned int aCtrl, unsigned int aFboIndex) {
			mVDMix->toggleFboValue(aCtrl, aFboIndex);
		};
		int								getFboIntUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex) {
			return mVDMix->getFboIntUniformValueByIndex(aCtrl, aFboIndex);
		};

		float							getFboFloatUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex) {
			return mVDMix->getFboFloatUniformValueByIndex(aCtrl, aFboIndex);
		};
		bool							setFboFloatUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex, float aValue) {
			return mVDMix->setFboFloatUniformValueByIndex(aCtrl, aFboIndex, aValue);
		};*/
		bool									getGlobal(unsigned int aFboIndex) {
			return mVDMix->getGlobal(aFboIndex);
		};
		void									toggleGlobal(unsigned int aFboIndex) {
			mVDMix->toggleGlobal(aFboIndex);
		};
		unsigned int					getFboInputTexturesCount(unsigned int aFboIndex = 0) {
			return 1; //TODO support several textures
		}
		std::string							getFboStatus(unsigned int aFboIndex = 0) {
			return mVDMix->getFboStatus(aFboIndex);
		}
		void							updateShaderThumbFile(unsigned int aFboIndex) {
			mVDMix->updateShaderThumbFile(aFboIndex);
		}
		std::string							getFboInputTextureName(unsigned int aFboIndex = 0) {
			return mVDMix->getFboInputTextureName(aFboIndex);
		}
		ci::gl::Texture2dRef							getFboInputTexture(unsigned int aFboIndex = 0) {
			return mVDMix->getFboInputTexture(aFboIndex);
		}
		std::vector<ci::gl::GlslProg::Uniform>			getUniforms(unsigned int aFboIndex = 0) {
			return mVDMix->getUniforms(aFboIndex);
		}
		int								getUniformIndexForName(const std::string& aName) {
			return mVDAnimation->getUniformIndexForName(aName);
		};



		/*
		// blendmodes
		unsigned int					getFboBlendCount() { return mBlendFbos.size(); };
		void							useBlendmode(unsigned int aBlendIndex) { mVDSettings->iBlendmode = aBlendIndex; };
		*/
		bool							isAutoLayout() { return mVDSettings->mAutoLayout; };
		void							toggleAutoLayout() { mVDSettings->mAutoLayout = !mVDSettings->mAutoLayout; }
		// textures
		void							loadImageFile(const std::string& aFile, unsigned int aTextureIndex) {
			mVDMix->loadImageFile(aFile, aTextureIndex);
		};
		/*unsigned int					getInputTexturesCount() {
			return mTextureList.size();
		}
		string VDSession::getInputTextureName(unsigned int aTextureIndex) {
			return mTextureList[math<int>::min(aTextureIndex, mTextureList.size() - 1)]->getName();
		}*/

		//ci::gl::TextureRef				getInputTexture(unsigned int aTextureIndex);
		/*ci::gl::TextureRef				getCachedTexture(unsigned int aTextureIndex, const string& aFilename);
		//ci::gl::TextureRef				getNextInputTexture(unsigned int aTextureIndex);

		void							loadAudioFile(const string& aFile);
		void							loadMovie(const string& aFile, unsigned int aTextureIndex);
		bool							loadImageSequence(const string& aFolder, unsigned int aTextureIndex);
		//void							toggleSharedOutput(unsigned int aMixFboIndex = 0);
		//bool							isSharedOutputActive() { return mVDMix->isSharedOutputActive(); };
		//unsigned int					getSharedMixIndex() { return mVDMix->getSharedMixIndex(); };
		// move, rotate, zoom methods
		//void							setPosition(int x, int y);
		//void							setZoom(float aZoom);
		int								getInputTextureXLeft(unsigned int aTextureIndex);
		void							setInputTextureXLeft(unsigned int aTextureIndex, int aXLeft);
		int								getInputTextureYTop(unsigned int aTextureIndex);
		void							setInputTextureYTop(unsigned int aTextureIndex, int aYTop);
		int								getInputTextureXRight(unsigned int aTextureIndex);
		void							setInputTextureXRight(unsigned int aTextureIndex, int aXRight);
		int								getInputTextureYBottom(unsigned int aTextureIndex);
		void							setInputTextureYBottom(unsigned int aTextureIndex, int aYBottom);
		bool							isFlipVInputTexture(unsigned int aTextureIndex);
		bool							isFlipHInputTexture(unsigned int aTextureIndex);
		void							inputTextureFlipV(unsigned int aTextureIndex);
		void							inputTextureFlipH(unsigned int aTextureIndex);
		bool							getInputTextureLockBounds(unsigned int aTextureIndex);
		void							toggleInputTextureLockBounds(unsigned int aTextureIndex);
		unsigned int					getInputTextureOriginalWidth(unsigned int aTextureIndex);
		unsigned int					getInputTextureOriginalHeight(unsigned int aTextureIndex);
		void							togglePlayPause(unsigned int aTextureIndex);
		string							getStatus(unsigned int aTextureIndex);
		// movie
		bool							isMovie(unsigned int aTextureIndex);
		// sequence
		bool							isSequence(unsigned int aTextureIndex);
		bool							isLoadingFromDisk(unsigned int aTextureIndex);
		void							toggleLoadingFromDisk(unsigned int aTextureIndex);
		void							syncToBeat(unsigned int aTextureIndex);
		void							reverse(unsigned int aTextureIndex);*/
		/*float							getSpeed(unsigned int aTextureIndex) {
			return mTextureList[math<int>::min(aTextureIndex, mTextureList.size() - 1)]->getSpeed();
		};
		void							setSpeed(unsigned int aTextureIndex, float aSpeed) {
			//if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
			//mTextureList[aTextureIndex]->setSpeed(aSpeed);
			for (int i = 0; i < mTextureList.size() - 1; i++)
			{
				mTextureList[i]->setSpeed(aSpeed);
			}
		};
		void							incrementSequencePosition() {
			for (unsigned int i = 0; i < mTextureList.size() - 1; i++)
			{
				setPlayheadPosition(i, getPosition(i) + 1);
			}
		}
		void							decrementSequencePosition() {
			for (unsigned int i = 0; i < mTextureList.size() - 1; i++)
			{
				setPlayheadPosition(i, getPosition(i) - 1);
			}
		}
		int								getPosition(unsigned int aTextureIndex) {
			return mTextureList[math<int>::min(aTextureIndex, mTextureList.size() - 1)]->getPosition();
		};
		void							setPlayheadPosition(unsigned int aTextureIndex, int aPosition) {
			mTextureList[math<int>::min(aTextureIndex, mTextureList.size() - 1)]->setPlayheadPosition(aPosition);
		};
		int								getMaxFrame(unsigned int aTextureIndex) {
			return mTextureList[math<int>::min(aTextureIndex, mTextureList.size() - 1)]->getMaxFrame();
		};*/

		/*void							sendFragmentShader(unsigned int aShaderIndex);*/
		//! window management
		void							createWindow() { cmd = 0; };
		void							deleteWindow() { cmd = 1; };

		int								getCmd() { int rtn = cmd; cmd = -1; return rtn; };
		// utils
		/*float							formatFloat(float f) { return mVDUtils->formatFloat(f); };

		void							load();
		void							updateAudio() {mTextureList[0]->getTexture();}
		void							updateMixUniforms();
		void							updateBlendUniforms();
		// hydra
		string							getHydraUniformsString() { return mHydraUniformsValuesString; };
		ci::gl::TextureRef				getHydraTexture() { return mHydraFbo->getColorTexture(); };*/

		// modeint
		int								getMode();
		void							setMode(int aMode);
		int								getModesCount();
		void							toggleUI();
		bool							showUI();
		std::string						getModeName(unsigned int aMode);
		

	private:

		int								mMode = 1;
		VDParamsRef						mVDParams;
		// Settings
		VDSettingsRef					mVDSettings;
		// Settings
		VDAnimationRef					mVDAnimation;
		// Settings
		VDUniformsRef					mVDUniforms;
		
	
		// Message router
		//VDRouterRef					mVDRouter;
		// Log
		//VDLogRef						mVDLog;
		// Mix
		VDMixRef						mVDMix;
		const std::string				sessionFileName = "session.json";
		fs::path						sessionPath;
		// tempo
		float							mFpb;
		float							mOriginalBpm;
		float							mTargetFps;
		// audio
		bool							mFreqWSSend;
		// files and paths
		/*string							mWaveFileName;
		string							mMovieFileName;
		string							mImageSequencePath;
		// delay
		int								mWavePlaybackDelay;
		int								mMoviePlaybackDelay;
		unsigned int					mFadeInDelay;
		unsigned int					mFadeOutDelay;
		// font and text
		string							mText;
		int								mTextPlaybackDelay;
		int								mTextPlaybackEnd;
		//! Fbos
		// maintain a list of fbo for right only or left/right or more fbos specific to this mix
		//VDFboList						mFboList;
		fs::path						mFbosFilepath;
		// fbo
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		bool							mEnabledAlphaBlending;
		bool							mRenderTexture;
		//! Warps
		int								mSelectedWarp;
		//! Shaders
		string							mShaderLeft;
		string							mShaderRight;
		//! hydra
		string							mHydraUniformsValuesString;
		void							updateHydraUniforms();
		void							renderHydra();
		ci::gl::FboRef					mHydraFbo;
		gl::GlslProgRef					mGlslHydra;
		gl::GlslProgRef					mGlslRender;
		//! textures
		int								mWidth;
		int								mHeight;
		float							mPosX;
		float							mPosY;
		float							mZoom;
		void							updateStream(string * aStringPtr);*/
		//! window management
		int								cmd = -1;
		bool							mShowUI = false;
		// mix

/*
		std::string						mFbosPath;

		//! mix shader
		gl::GlslProgRef					mMixShader;
		string							mError;

		//! Fbos
		map<int, VDMixFbo>				mMixFbos;
		*/

		//! Modes
		std::map<int, std::string>				mModesList;
		// blendmodes fbos
		std::map<int, ci::gl::FboRef>		mBlendFbos;
		int								mCurrentBlend;
		/*gl::GlslProgRef					mGlslMix, mGlslBlend, mGlslFeedback, mGlslMixette;
		// render
		void							renderMix();
		void							renderBlend();
		// warping
		gl::FboRef						mRenderFbo;
		// warp rendered texture
		ci::gl::Texture2dRef			mRenderedTexture, mMixetteTexture;
		*/
		//! fbos
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		gl::FboRef						mWarpsFbo;
		gl::FboRef						mPostFbo;
		//! shaders
		gl::GlslProgRef					mGlslPost;
		void							renderPostToFbo();
		void							renderWarpsToFbo();
		ci::gl::Texture2dRef			mWarpTexture;
		// warps

		WarpList						mWarpList;
		fs::path						mSettings;
		void							updateWarpName(unsigned int aWarpIndex) {
			if (aWarpIndex < mWarpList.size()) {
				mWarpList[aWarpIndex]->setName(mVDMix->getFboName(mWarpList[aWarpIndex]->getAFboIndex()));
			}
		}
		void							loadFbos();

		bool							odd = false;

	};

}
