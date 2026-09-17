#pragma once
/*
	VDSession
	Top hierarchy class to use all the child classes
*/
// TODO implement factory or builder?

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
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
// json
#include "cinder/JsonTree.h"
// http
#include "cinder/http/http.hpp"

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
		void							update();

		void							loadFromJsonFile(const fs::path& jsonFile);
		void							setupHttpClient();
		void							loadShaderFromHttp(const std::string& url, unsigned int aFboIndex);
		// folder/shader browser: GET {apiUrl}api/folders, GET {apiUrl}api/folders/{folder}/{extension}
		// and GET {apiUrl}api/folders/{folder}/{extension}/{name} (the last one returns {title, content}
		// like loadShaderFromHttp's endpoint, and lands on the first fbo shader slot whose iWeight is 0)
		void							listFolders();
		void							listShaders(const std::string& aFolder, const std::string& aExtension);
		void							loadShaderFromFolder(const std::string& aFolder, const std::string& aExtension, const std::string& aName);
		std::vector<std::string>		getFolderList() { return mFolderList; };
		std::vector<std::string>		getShaderList() { return mShaderList; };
		unsigned int					fboFromJson(const JsonTree& json, unsigned int aFboIndex = 0, const std::string& aFolder = "") {
			return mVDMix->createFboShaderTexture(json, aFboIndex, aFolder);
		};

		//! Mix
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getPostFboTexture();
		ci::gl::TextureRef				getFxFboTexture();
		ci::gl::TextureRef				getWarpFboTexture();
		ci::gl::TextureRef				getRenderedWarpFboTexture();
		ci::gl::TextureRef				getMixetteTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getRenderedMixetteTexture(unsigned int aFboIndex);
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
		void							removeWarp(unsigned int aWarpIndex);
		// one warp's own small rendered preview (after warping, not the raw fbo) - see
		// VDUIWarps.cpp
		ci::gl::TextureRef				getWarpPreviewTexture(unsigned int aWarpIndex);
		std::string						getFboShaderName(unsigned int aFboIndex);
		//std::string							getFboShaderName(unsigned int aFboShaderIndex);
		std::string						getFboTextureName(unsigned int aFboIndex);
		void							saveWarps();

		bool							handleMouseMove(MouseEvent& event);
		bool							handleMouseDown(MouseEvent& event);
		bool							handleMouseDrag(MouseEvent& event);
		bool							handleMouseUp(MouseEvent& event);

		void							setAnim(unsigned int aCtrl, unsigned int aAnim);
		unsigned int					getAnim(unsigned int aCtrl);
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
		/*v
		void							setFboInputTexture(unsigned int aFboIndex = 0, unsigned int aTexIndex = 0) {
			mVDMix->setFboInputTexture(aFboIndex, aTexIndex);
		}*/
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
		void							blendRenderEnable(bool render);
		void							fileDrop(FileDropEvent event);
		// drag-and-drop: consumed by VDUIFbos.cpp, which is the only place that knows each fbo's
		// actual current ImGui window rect (these can be dragged/resized independently, so a
		// static position formula can't reliably target one). Called once per fbo, per frame, with
		// that window's current rect (in the same device-pixel space as the stored drop position -
		// see fileDrop()'s use of ci::app::toPixels()); returns true once some fbo claims it.
		bool							consumePendingTextureDropIfInRect(unsigned int aFboIndex, ci::vec2 aRectMin, ci::vec2 aRectMax) {
			if (!mPendingTextureDrop.active) return false;
			if (mPendingTextureDrop.pos.x < aRectMin.x || mPendingTextureDrop.pos.x > aRectMax.x ||
				mPendingTextureDrop.pos.y < aRectMin.y || mPendingTextureDrop.pos.y > aRectMax.y) return false;
			bool loaded = mVDMix->loadTextureIntoFboActiveSlot(aFboIndex, mPendingTextureDrop.path);
			mPendingTextureDrop.active = false;
			return loaded;
		}
		// called once per frame, after every fbo has had a chance to claim the drop above - if
		// still pending (dropped somewhere that isn't any fbo's window), loads it standalone into
		// the shared texture pool instead, pickable afterward by any fbo
		void							flushPendingTextureDrop() {
			if (!mPendingTextureDrop.active) return;
			mVDMix->addStandaloneTexture(mPendingTextureDrop.path);
			mPendingTextureDrop.active = false;
		}
		// called once per fbo, per frame, from VDUIFbos.cpp, so every fbo's own active texture
		// (json-loaded or drag-and-dropped directly onto it) stays available in the shared pool for
		// every other fbo to pick too - see VDMix::registerLoadedTexture()
		void							registerFboActiveTextureInGlobalPool(unsigned int aFboIndex) {
			mVDMix->registerLoadedTexture(mVDMix->getInputTextureName(aFboIndex, 0), mVDMix->getFboInputTextureListItem(aFboIndex, 0));
		}
		int								getInputTextureMode(unsigned int aFboIndex) { return mVDMix->getInputTextureMode(aFboIndex); }
		unsigned int					getLoadedTextureCount() { return mVDMix->getLoadedTextureCount(); }
		ci::gl::Texture2dRef			getLoadedTexture(unsigned int aIndex) { return mVDMix->getLoadedTexture(aIndex); }
		std::string						getLoadedTextureName(unsigned int aIndex) { return mVDMix->getLoadedTextureName(aIndex); }

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
		void									setApiUrl(const std::string& apiUrl) {
			mApiurl = apiUrl;
		};
		std::string								getApiUrl() { return mApiurl; };
		void									setPreferredAudioInputDevice(const std::string& aDevice) {
			mVDAnimation->setPreferredAudioInputDevice(aDevice);
		}
		void									setPreferredAudioOutputDevice(const std::string& aDevice) {
			mVDAnimation->setPreferredAudioOutputDevice(aDevice);
		}
		std::string								getPreferredAudioInputDevice() { return mVDAnimation->getPreferredAudioInputDevice(); };
		std::string								getPreferredAudioOutputDevice() { return mVDAnimation->getPreferredAudioOutputDevice(); };
		bool									refreshAudioDevices() { return mVDAnimation->refreshAudioDevices(); };
		std::vector<std::string>				getAudioInputDeviceNames() { return mVDAnimation->getAudioInputDeviceNames(); };
		std::vector<std::string>				getAudioOutputDeviceNames() { return mVDAnimation->getAudioOutputDeviceNames(); };
		unsigned int							getFboShaderListSize() { return mVDMix->getFboShaderListSize(); };
		ci::gl::TextureRef						getFboShaderTexture(unsigned int aFboShaderIndex);

		std::vector<ci::gl::GlslProg::Uniform>	getFboShaderUniforms(unsigned int aFboShaderIndex);
		float									getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex);
		void									setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue);
		unsigned int 							createFboShaderTexture(const JsonTree& json, unsigned int aFboIndex = 0) {
			return mVDMix->createFboShaderTexture(json, aFboIndex);
		};
		bool									isFboValid(unsigned int aFboIndex) {
			return mVDMix->isFboValid(aFboIndex);
		};
		std::string								getFboMsg(unsigned int aFboIndex) {
			return mVDMix->getFboMsg(aFboIndex);
		};
		std::string								getFboError(unsigned int aFboIndex) {
			return mVDMix->getFboError(aFboIndex);
		};
		std::string								getFboStatus(unsigned int aFboIndex) {
			return mVDMix->getFboStatus(aFboIndex);
		};
		
		unsigned int							getFboMs( unsigned int aTexIndex = 0) {
			return mVDMix->getFboMs(aTexIndex);
		};
		bool									isValidInputTexture( unsigned int aTexIndex = 0) {
			return mVDMix->isValidInputTexture(aTexIndex);
		};
		unsigned int								getFboMsTotal(unsigned int aFboIndex) {
			return mVDMix->getFboMsTotal(aFboIndex);
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
		ci::gl::Texture2dRef			getFboInputTexture(unsigned int aTexIndex = 0) {
			return mVDMix->getFboInputTexture(aTexIndex);
		}
		void							setFboTextureAudioMode(unsigned int aFboIndex) {
			return mVDMix->setFboTextureAudioMode(aFboIndex);
		};
		void							saveThumbnail(unsigned int aFboIndex) {
			mVDMix->saveThumbnail(aFboIndex);
		}
		void							setSelectedFbo(unsigned int aFboIndex) {
			mVDMix->setSelectedFbo(aFboIndex);
		}
		unsigned int					getSelectedFbo() {
			return mVDMix->getSelectedFbo();
		};
		ci::gl::Texture2dRef			getFboInputTextureListItem(unsigned int aFboIndex, unsigned int aTexIndex) {
			return mVDMix->getFboInputTextureListItem(aFboIndex, aTexIndex);
		}
		void							setFboInputTexture(unsigned int aFboIndex, ci::gl::Texture2dRef aTextureRef, const std::string& aName = "") {
			mVDMix->setFboInputTexture(aFboIndex, aTextureRef, aName);
		}
		void							setFboInputTexture(unsigned int aFboIndex, unsigned int aTexIndex) {
			mVDMix->setFboInputTexture(aFboIndex, aTexIndex);
		}
		unsigned int					getFboInputTextureIndex(unsigned int aFboIndex) {
			return mVDMix->getFboInputTextureIndex(aFboIndex);
		}
		unsigned int					getInputTexturesCount(unsigned int aFboIndex = 0) {
			return mVDMix->getInputTexturesCount(aFboIndex);
		}
		std::string						getInputTextureName(unsigned int aFboIndex, unsigned int aTexIndex = 0) {
			return mVDMix->getInputTextureName(aFboIndex, aTexIndex);
		}
		// playback controls (sequence/movie)
		bool							isSequence(unsigned int aFboIndex) { return mVDMix->isSequence(aFboIndex); }
		bool							isMovie(unsigned int aFboIndex) { return mVDMix->isMovie(aFboIndex); }
		void							togglePlayPause(unsigned int aFboIndex) { mVDMix->togglePlayPause(aFboIndex); }
		void							syncToBeat(unsigned int aFboIndex) { mVDMix->syncToBeat(aFboIndex); }
		void							reverse(unsigned int aFboIndex) { mVDMix->reverse(aFboIndex); }
		bool							isLoadingFromDisk(unsigned int aFboIndex) { return mVDMix->isLoadingFromDisk(aFboIndex); }
		void							toggleLoadingFromDisk(unsigned int aFboIndex) { mVDMix->toggleLoadingFromDisk(aFboIndex); }
		float							getSpeed(unsigned int aFboIndex) { return mVDMix->getSpeed(aFboIndex); }
		void							setSpeed(unsigned int aFboIndex, float aSpeed) { mVDMix->setSpeed(aFboIndex, aSpeed); }
		int								getPosition(unsigned int aFboIndex) { return mVDMix->getPosition(aFboIndex); }
		void							setPlayheadPosition(unsigned int aFboIndex, int aPosition) { mVDMix->setPlayheadPosition(aFboIndex, aPosition); }
		int								getMaxFrame(unsigned int aFboIndex) { return mVDMix->getMaxFrame(aFboIndex); }
		void							setVideoVolume(unsigned int aFboIndex, float aVolume) { mVDMix->setVideoVolume(aFboIndex, aVolume); }
		float							getVideoVolume(unsigned int aFboIndex) { return mVDMix->getVideoVolume(aFboIndex); }
		/*
		// blendmodes
		unsigned int					getFboBlendCount() { return mBlendFbos.size(); };
		void							useBlendmode(unsigned int aBlendIndex) { mVDSettings->iBlendmode = aBlendIndex; };
		*/
		bool							isAutoLayout() { return mVDSettings->mAutoLayout; };
		void							toggleAutoLayout() { mVDSettings->mAutoLayout = !mVDSettings->mAutoLayout; }
		// textures
		void							loadImageFile(const std::string& aFile, unsigned int aFboIndex = 0) {
			mVDMix->loadImageFile(aFile, aFboIndex);
		};
		void							loadVideoFile(const std::string& aFile, unsigned int aFboIndex = 0) {
			mVDMix->loadVideoFile(aFile, aFboIndex);
		};		
		//bool							loadImageSequence(const string& aFolder, unsigned int aTextureIndex);
		bool							loadFolder(const string& aFolder);
		void							loadAudioFile(const string& aFile);



		/*void							sendFragmentShader(unsigned int aShaderIndex);*/
		
		//! window management
		void							createWindow() { cmd = 0; };
		void							deleteWindow() { cmd = 1; };

		int								getCmd() { int rtn = cmd; cmd = -1; return rtn; };
		void							toggleUI();
		bool							showUI();
		std::string						getModeName(unsigned int aMode);
		unsigned int					getModesCount();
		void							resetAnim() {
			mVDAnimation->resetAnim();
		}
		
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
		// apiurl
		std::string						mApiurl = "http://localhost:40088/";
		// drag-and-drop: a .jpg/.png/.mp4 drop waiting for VDUIFbos.cpp to hit-test against each
		// fbo's actual current window rect - see consumePendingTextureDropIfInRect()/flushPendingTextureDrop()
		struct PendingTextureDrop {
			bool		active = false;
			std::string	path;
			ci::vec2	pos;
		};
		PendingTextureDrop				mPendingTextureDrop;

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
		gl::FboRef						mFxFbo;
		//! shaders
		gl::GlslProgRef					mGlslPost;
		gl::GlslProgRef					mGlslFx;
		void							renderPostToFbo();
		void							renderFxToFbo();
		void							renderWarpsToFbo();
		ci::gl::TextureRef				resolveWarpInputTexture(const WarpRef& aWarp);
		void							drawWarpWithInput(const WarpRef& aWarp, const ci::gl::TextureRef& aInputTex);
		ci::gl::Texture2dRef			mWarpTexture;
		// warps

		WarpList						mWarpList;
		// one small preview fbo per warp, lazily created - see getWarpPreviewTexture()
		std::vector<ci::gl::FboRef>	mWarpPreviewFbos;
		fs::path						mSettings;

		bool							odd = false;
		// http
		void									makeRequest(http::UrlRef url, unsigned int aFboIndex);
		void									makeFolderListRequest(http::UrlRef url);
		void									makeShaderListRequest(http::UrlRef url);
		void									makeShaderContentRequest(http::UrlRef url, unsigned int aFboIndex, const std::string& aName);

		std::shared_ptr<ci::http::Session>		session;
		std::shared_ptr<ci::http::SslSession>	sslSession;
		ci::gl::TextureRef texture;
		http::UrlRef							httpUrl, httpsUrl;
		//bool useHttp = false;
		std::vector<std::string>				mFolderList;
		std::vector<std::string>				mShaderList;
	};

}
