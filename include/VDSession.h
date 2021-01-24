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
		VDSession(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, VDMixRef aVDMix);

		bool							handleKeyDown(KeyEvent& event);
		bool							handleKeyUp(KeyEvent& event);
		void							update(unsigned int aClassIndex = 0);

		void							loadFromJsonFile(const fs::path& jsonFile);
		unsigned int					fboFromJson(const JsonTree& json, unsigned int aFboIndex = 0) {
			return mVDMix->createFboShaderTexture(json, aFboIndex);
		};

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
		//std::string							getFboShaderName(unsigned int aFboShaderIndex);
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
		float							getDefaultUniformValue(unsigned int aIndex);
		int								getSampler2DUniformValueByName(const std::string& aName);
		float							getUniformValueByName(const std::string& aCtrlName);

		// tempo
		void							tapTempo() { mVDAnimation->tapTempo(); };
		void							toggleUseTimeWithTempo() { mVDAnimation->toggleUseTimeWithTempo(); };
		void							useTimeWithTempo() { mVDAnimation->useTimeWithTempo(); };
		bool							getUseTimeWithTempo() { return mVDAnimation->getUseTimeWithTempo(); };
		//void							setTimeFactor(const int &aTimeFactor) { mVDAnimation->setTimeFactor(aTimeFactor); };
		// audio
		ci::gl::TextureRef				getAudioTexture() {

			//mVDMix->setFboAudioInputTexture(0); // TODO 20210101 remove and use update()
			return mVDAnimation->getAudioTexture();
		};
		/*void							setFboAudioInputTexture(unsigned int aFboIndex = 0) {
			mVDMix->setFboAudioInputTexture(aFboIndex); // TODO 20210101 warning not updating
		}*/
		void							setFboInputTexture(unsigned int aFboIndex = 0, unsigned int aTexIndex = 0) {
			mVDMix->setFboInputTexture(aFboIndex, aTexIndex);
		}
		std::string						getAudioTextureName() { return mVDAnimation->getAudioTextureName(); };
		float*							getFreqs() { return mVDAnimation->iFreqs; };
		int								getFreqIndexSize() { return mVDAnimation->getFreqIndexSize(); };
		float							getFreq(unsigned int aFreqIndex) { return mVDUniforms->getUniformValue(mVDUniforms->IFREQ0 + aFreqIndex); };
		int								getFreqIndex(unsigned int aFreqIndex) { return mVDAnimation->getFreqIndex(aFreqIndex); };
		void							setFreqIndex(unsigned int aFreqIndex, unsigned int aFreq) { mVDAnimation->setFreqIndex(aFreqIndex, aFreq); };
		int								getFFTWindowSize() { return mVDAnimation->mFFTWindowSize; };
		bool							getUseAudio() { return mVDAnimation->getUseAudio(); };
		bool							isAudioBuffered() { return mVDAnimation->isAudioBuffered(); };
		void							toggleAudioBuffered() { mVDAnimation->toggleAudioBuffered(); };

		bool							getUseWaveMonitor() { return mVDAnimation->getUseWaveMonitor(); };
		void							toggleUseWaveMonitor() { mVDAnimation->toggleUseWaveMonitor(); };
		
		bool							getUseRandom() { return mVDAnimation->getUseRandom(); };
		void							toggleUseRandom() { mVDAnimation->toggleUseRandom(); };

		bool							getUseLineIn() { return mVDAnimation->getUseLineIn(); };
		void							setUseLineIn(bool useLineIn) { mVDAnimation->setUseLineIn(useLineIn); };
		void							toggleUseLineIn() { mVDAnimation->toggleUseLineIn(); };

		int								loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex = 0) {
			return mVDMix->loadFragmentShader(aFilePath, aFboShaderIndex);
		};
		
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
		/* 20201229
		std::string						getFboName(unsigned int aFboIndex) {
			return mVDMix->getFboName(aFboIndex);
		};
		void							saveFbos() {
			mVDMix->saveFbos();
		};*/
		unsigned int					getFboShaderListSize() { return mVDMix->getFboShaderListSize(); };
		ci::gl::TextureRef					getFboShaderTexture(unsigned int aFboShaderIndex);

		std::vector<ci::gl::GlslProg::Uniform>	getFboShaderUniforms(unsigned int aFboShaderIndex);
		float									getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex);
		void								setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue);
		unsigned int 					createFboShaderTexture(const JsonTree& json, unsigned int aFboIndex = 0) {
			return mVDMix->createFboShaderTexture(json, aFboIndex);
		};
		bool							isFboValid(unsigned int aFboIndex) {
			return mVDMix->isFboValid(aFboIndex);
		};
		std::string								getMsg(unsigned int aFboIndex) {
			return mVDMix->getMsg(aFboIndex);
		};
		std::string								getError(unsigned int aFboIndex) {
			return mVDMix->getError(aFboIndex);
		};
		
		std::vector<ci::gl::GlslProg::Uniform>			getUniforms(unsigned int aFboIndex = 0) {
			return mVDMix->getUniforms(aFboIndex);
		}
		int								getUniformIndexForName(const std::string& aName) {
			return mVDAnimation->getUniformIndexForName(aName);
		};
		// Fbo
		std::string						getFboInputTextureName(unsigned int aFboIndex = 0) {
			return mVDMix->getFboTextureName(aFboIndex);
		}
		ci::gl::Texture2dRef			getFboInputTexture(unsigned int aFboIndex = 0) {
			return mVDMix->getFboInputTexture(aFboIndex);
		}
		ci::gl::Texture2dRef			getFboInputTextureListItem(unsigned int aFboIndex, unsigned int aTexIndex) {
			return mVDMix->getFboInputTextureListItem(aFboIndex, aTexIndex);
		}
		unsigned int			getFboInputTextureIndex(unsigned int aFboIndex = 0) {
			return mVDMix->getFboInputTextureIndex(aFboIndex);
		}
		ci::gl::Texture2dRef			getInputTexture(unsigned int aTexIndex) {
			return mVDMix->getInputTexture(aTexIndex);
		}
		unsigned int					getInputTexturesCount() {
			return mVDMix->getInputTexturesCount();
		}
		std::string				getInputTextureName(unsigned int aTexIndex) {
			return mVDMix->getInputTextureName(aTexIndex);
		}
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
		bool							loadImageSequence(const string& aFolder, unsigned int aTextureIndex);
		void							loadAudioFile(const string& aFile);

		
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
		void							toggleUI();
		bool							showUI();
		std::string						getModeName(unsigned int aMode);


	private:
		VDParamsRef						mVDParams;
		// Settings
		VDSettingsRef					mVDSettings;
		// Settings
		VDAnimationRef					mVDAnimation;
		// Settings
		VDUniformsRef					mVDUniforms;

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
		//! window management
		int								cmd = -1;
		bool							mShowUI = false;
	
		//! Modes
		std::map<int, std::string>		mModesList;
		// blendmodes fbos
		std::map<int, ci::gl::FboRef>	mBlendFbos;
		int								mCurrentBlend;
		
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
		
		void							loadFbos();

		bool							odd = false;

	};

}
