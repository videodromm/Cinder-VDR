/*
	VDMix
	Handles mixing shaders.
	Maintains a Fbo list
	Outputs severals Fbos depending on the context: mix 2 shaders or several with weights
*/

#pragma once

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/JsonTree.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"

// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Uniforms
#include "VDUniforms.h"
// Fbos
#include "VDFboShader.h"
// Params
#include "VDParams.h"

// Syphon is optional; disabled by default for compatibility with modern macOS toolchains.
#if defined( CINDER_MAC ) && defined( VD_ENABLE_SYPHON )
#include "cinderSyphon.h"
#endif

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;

using namespace videodromm;

namespace videodromm
{
	// stores the pointer to the VDMix instance
	typedef std::shared_ptr<class VDMix> 	VDMixRef;

	class VDMix {
	public:
		VDMix(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
		static VDMixRef					create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
		{
			return std::shared_ptr<VDMix>(new VDMix(aVDSettings, aVDAnimation, aVDUniforms));
		}

		// fbolist
		unsigned int					getFboShaderListSize() {
			return (unsigned int)mFboShaderList.size();
		};
		bool							isFboValid(unsigned int aFboIndex) {
			bool valid = false;
			if (mFboShaderList.size() > 0) {
				valid = mFboShaderList[getValidFboIndex(aFboIndex)]->isValid();
			}
			return valid;

		};
		std::string						getFboMsg(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getFboMsg();
		};
		std::string						getFboError(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getFboError();
		};
		std::string						getFboStatus(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getFboStatus();
		};
		bool							isValidInputTexture(unsigned int aTexIndex = 0) {
			return mFboShaderList[getValidFboIndex(mSelectedFbo)]->isValidInputTexture(aTexIndex);
		};
		unsigned int					getFboMs(unsigned int aTexIndex = 0) {
			return mFboShaderList[getValidFboIndex(mSelectedFbo)]->getFboMs(aTexIndex);
		};
		unsigned int					getFboMsTotal(unsigned int aFboIndex = 0) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getFboMsTotal();
		};

		std::string						getAssetsPath() {
			return mAssetsPath;
		};
		unsigned int					findAvailableIndex(unsigned int aFboShaderIndex, const JsonTree &json);
		bool							setFragmentShaderString(const string& aFragmentShaderString, const std::string& aName = "", unsigned int aFboShaderIndex = 0);
		// sets the fragment shader on exactly aFboShaderIndex, bypassing findAvailableIndex's
		// hydra round-robin (which only triggers for setFragmentShaderString(..., 0)) - needed so
		// callers that already picked a specific slot (e.g. the folder/shader browser UI) land there
		bool							setFragmentShaderStringAtIndex(const string& aFragmentShaderString, const std::string& aName, unsigned int aFboShaderIndex);
		// first fbo shader slot (0..7) whose iWeight uniform is 0.0f, i.e. not currently mixed in -
		// used to load a new shader without a sudden change in the rendering
		unsigned int					findFirstZeroWeightFboIndex();

		int								loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex);
		std::vector<ci::gl::GlslProg::Uniform>			getUniforms(unsigned int aFboIndex = 0) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getUniforms();
		}
		
		void							setSelectedFbo(unsigned int aFboIndex = 0) {
			mSelectedFbo = getValidFboIndex(aFboIndex);
		}
		unsigned int					getSelectedFbo() {
			return mSelectedFbo;
		};
		void							setFboInputTexture(unsigned int aFboIndex, ci::gl::Texture2dRef aTextureRef, const std::string& aName = "") {
			if (mFboShaderList.size() > 0) {
				mFboShaderList[getValidFboIndex(aFboIndex)]->setInputTextureRefByIndex(0, aTextureRef, aName);
			}
		}
		// selects which already-loaded input texture slot is the active one for this fbo
		void							setFboInputTexture(unsigned int aFboIndex, unsigned int aTexIndex) {
			mFboShaderList[getValidFboIndex(aFboIndex)]->setInputTextureIndex(aTexIndex);
		}
		unsigned int					getFboInputTextureIndex(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureIndex();
		}
		int								getInputTextureMode(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureMode();
		}

		ci::gl::Texture2dRef			getFboInputTexture(unsigned int aTexIndex = 0) {
			return mFboShaderList[getValidFboIndex(mSelectedFbo)]->getInputTexture(aTexIndex);
		}
		std::string						getInputTextureName(unsigned int aFboIndex, unsigned int aTexIndex = 0) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getTextureName(aTexIndex);
		}
		std::string						getFboTextureName(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getTextureName(); // useless or duplic
		};

		int								getFboInputTextureWidth(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureWidth();
		};
		int								getFboInputTextureHeight(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureHeight();
		};

		void							setFboTextureAudioMode(unsigned int aFboIndex) {
			mFboShaderList[getValidFboIndex(aFboIndex)]->setFboTextureAudioMode();
		}
		void							saveThumbnail(unsigned int aFboIndex) {
			mFboShaderList[getValidFboIndex(aFboIndex)]->saveThumbnail();
		}
		ci::gl::Texture2dRef			getFboInputTextureListItem(unsigned int aFboIndex, unsigned int aTexIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getFboInputTextureListItem(aTexIndex);
		}
		unsigned int					getInputTexturesCount(unsigned int aFboIndex = 0) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTexturesCount();
		}
		// playback controls (sequence/movie), forwarded straight to the fbo - see VDFboShader.h
		bool							isSequence(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->isSequence(); }
		bool							isMovie(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->isMovie(); }
		void							togglePlayPause(unsigned int aFboIndex) { mFboShaderList[getValidFboIndex(aFboIndex)]->togglePlayPause(); }
		void							syncToBeat(unsigned int aFboIndex) { mFboShaderList[getValidFboIndex(aFboIndex)]->syncToBeat(); }
		void							reverse(unsigned int aFboIndex) { mFboShaderList[getValidFboIndex(aFboIndex)]->reverse(); }
		bool							isLoadingFromDisk(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->isLoadingFromDisk(); }
		void							toggleLoadingFromDisk(unsigned int aFboIndex) { mFboShaderList[getValidFboIndex(aFboIndex)]->toggleLoadingFromDisk(); }
		float							getSpeed(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->getSpeed(); }
		void							setSpeed(unsigned int aFboIndex, float aSpeed) { mFboShaderList[getValidFboIndex(aFboIndex)]->setSpeed(aSpeed); }
		int								getPosition(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->getPosition(); }
		void							setPlayheadPosition(unsigned int aFboIndex, int aPosition) { mFboShaderList[getValidFboIndex(aFboIndex)]->setPlayheadPosition(aPosition); }
		int								getMaxFrame(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->getMaxFrame(); }
		void							setVideoVolume(unsigned int aFboIndex, float aVolume) { mFboShaderList[getValidFboIndex(aFboIndex)]->setVideoVolume(aVolume); }
		float							getVideoVolume(unsigned int aFboIndex) { return mFboShaderList[getValidFboIndex(aFboIndex)]->getVideoVolume(); }

		std::string						getFboShaderName(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getShaderName();
		};
		void							loadImageFile(const std::string& aFile, unsigned int aFboIndex = 0);
		void							loadVideoFile(const std::string& aFile, unsigned int aFboIndex = 0);
		// drag-and-drop onto an existing fbo's own window - dispatches by extension to
		// loadImageFile/loadVideoFile at that fbo's active slot (0)
		bool							loadTextureIntoFboActiveSlot(unsigned int aFboIndex, const std::string& aFile);
		// shared, deduplicated-by-name pool of every loaded texture (any fbo's own, or dropped
		// standalone) - lets any fbo pick any of them, not just the ones it loaded itself
		void							registerLoadedTexture(const std::string& aName, ci::gl::Texture2dRef aTexture);
		unsigned int					getLoadedTextureCount() { return (unsigned int)mLoadedTextures.size(); }
		ci::gl::Texture2dRef			getLoadedTexture(unsigned int aIndex) { return mLoadedTextures[aIndex].texture; }
		std::string						getLoadedTextureName(unsigned int aIndex) { return mLoadedTextures[aIndex].name; }
		// drag-and-drop that didn't land on any specific fbo window - loads the image standalone
		// and registers it in the pool above, unattached to any fbo until manually assigned via
		// setFboInputTexture(); returns false if the file isn't an image (video needs its own
		// ciWMFVideoPlayer, tied to a specific fbo - no standalone equivalent)
		bool							addStandaloneTexture(const std::string& aFile);

		std::vector<ci::gl::GlslProg::Uniform>	getFboShaderUniforms(unsigned int aFboShaderIndex);
		float							getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex);
		void							setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue);

		unsigned int					createFboShaderTexture(const JsonTree &json, unsigned int aFboIndex = 0, const std::string& aFolder = "");
		ci::gl::TextureRef				getMixetteTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getRenderedMixetteTexture(unsigned int aFboIndex) { return mMixetteTexture; };
		void							clearFboShaderList() {
			mFboShaderList.clear();
		}
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex) {
			if (mFboShaderList.size() == 0) return mDefaultTexture;
			/* 20220101 hydra check

			if (aFboIndex > mFboShaderList.size() - 1) aFboIndex = 0;

			if (mFboShaderList[aFboIndex]->isHydraTex()) {
				// fbo as inputTexture
				for (unsigned int i = 0; i < 4; i++)
				{
					mFboShaderList[aFboIndex]->setInputTextureRefByIndex(i, mFboShaderList[getValidFboIndex(aFboIndex)]->getTexture());
				}
			}
			else {
				// 20211227 useless? mFboShaderList[aFboIndex]->setInputTextureRef(mFboShaderList[aFboIndex]->getTexture());
			}
			return mFboShaderList[aFboIndex]->getRenderedTexture();
			*/
			unsigned int idx = getValidFboIndex(aFboIndex);
			// this is called every frame per fbo just for its own small UI preview thumbnail
			// (VDUIFbos.cpp's buildFboRenderedTexture()), regardless of whether that fbo actually
			// contributes anything to the final mix - getTexture() always fully re-renders, which
			// is wasted work once a fbo's weight is 0 (invisible either way). One render is enough
			// to have valid content ready for whenever the weight comes back up; every frame after
			// that while weight stays at 0 just reuses the same cached texture instead of paying
			// for another full shader pass. getMixetteTexture() already has its own, separate
			// weight>0 gate for the actual composited output - this covers the other render path.
			if (mVDUniforms->getUniformValue(mVDUniforms->IWEIGHT0 + idx) <= 0.01f && mFboShaderList[idx]->hasRenderedOnce()) {
				return mFboShaderList[idx]->getRenderedTexture();
			}
			return mFboShaderList[idx]->getTexture();

		}
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex) {
			if (mFboShaderList.size() == 0) return mDefaultTexture;
			if (aFboIndex > mFboShaderList.size() - 1) aFboIndex = 0;
			return mFboShaderList[aFboIndex]->getTexture();

		}

		
		 
			
		
		
		bool handleMouseDown(MouseEvent event)
		{
			bool handled = false;
			if (mFboShaderList.size() > 0) {
				for (unsigned int i = 0; i < mFboShaderList.size() - 1; i++)
				{
					if (mFboShaderList[i]->handleMouseDown(event)) handled = true;// event.getPos()
				}
			}
			event.setHandled(handled);
			return event.isHandled();			
		}
		bool handleMouseDrag(MouseEvent event)
		{
			if (mFboShaderList.size() == 0) return false;
			for (unsigned int i = 0; i < mFboShaderList.size() - 1; i++)
			{
				mFboShaderList[i]->handleMouseDrag(event);
			}
			return true;
		}
		
		void							selectSenderPanel() {
			if (mFboShaderList.size() == 0) return;
			mFboShaderList[0]->selectSenderPanel();
			/* CI_LOG_E("selectSenderPanel " << (unsigned int)mFboShaderList.size());
			for (size_t i{ 0 }; i < mFboShaderList.size() - 1; i++)
			{
				if (mFboShaderList[i]->getInputTextureMode() == VDTextureMode::SHARED) {
					mFboShaderList[i]->selectSenderPanel();
				}
				
			} */
		};
		void							restore(const fs::path& aFilePath);
	private:
		// Params
		VDParamsRef						mVDParams;
		// Animation
		VDAnimationRef					mVDAnimation;
		// Settings
		VDSettingsRef					mVDSettings;
		// Uniforms
		VDUniformsRef					mVDUniforms;

		//! FboShaders
		VDFboShaderRef					mMixFboShader;
		VDFboShaderRef					mFboShader;
		// maintain a list of fbos specific to this mix
		VDFboShaderList					mFboShaderList;
		unsigned int					mSelectedFbo = 0;
		// shared, deduplicated-by-name pool of every loaded texture - see registerLoadedTexture()
		std::vector<VDTextureStruct>	mLoadedTextures;
		// textures
		bool							save();
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		std::string						mError;
		const unsigned int				MAXSHADERS = 8;
		std::string						mAssetsPath = "";
		fs::path						mixPath;
		unsigned int					mCurrentSecond = 0;
		unsigned int					mCurrentIndex = 0;
		unsigned int					getValidFboIndex(unsigned int aFboIndex);
		ci::gl::Texture2dRef			mDefaultTexture; //in case no fbos
		gl::FboRef						mMixetteFbo;
		gl::GlslProgRef					mGlslMixette;
		ci::gl::Texture2dRef			mMixetteTexture;
	};
}
