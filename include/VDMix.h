/*
	VDMix
	Handles mixing shaders.
	Maintains a Fbo list
	Outputs severals Fbos depending on the context: mix 2 shaders or several with weights
*/

#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Json.h"
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

// Syphon
#if defined( CINDER_MAC )
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
		std::string						getMsg(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getMsg();
		};
		std::string						getError(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getError();
		};
		std::string						getAssetsPath() {
			return mAssetsPath;
		};
		unsigned int					findAvailableIndex(unsigned int aFboShaderIndex, const JsonTree &json);
		bool							setFragmentShaderString(const string& aFragmentShaderString, const std::string& aName = "");

		int								loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex);
		std::vector<ci::gl::GlslProg::Uniform>			getUniforms(unsigned int aFboIndex = 0) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getUniforms();
		}
		/*void							setFboAudioInputTexture(unsigned int aFboIndex = 0) {
			if (mFboShaderList.size() > 0) {
				mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->setInputTextureIndex(0);
				//mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->setImageInputTexture(mVDAnimation->getAudioTexture(), "Audio");
			}
		}*/
		void							setSelectedFbo(unsigned int aFboIndex = 0) {
			mSelectedFbo = getValidFboIndex(aFboIndex);
		}
		unsigned int					getSelectedFbo() {
			return mSelectedFbo;
		};
		void							setFboInputTexture(unsigned int aFboIndex = 0, unsigned int aTexIndex = 0) {
			if (mFboShaderList.size() > 0) {
				
				//unsigned int texIndex = getValidTexIndex(aTexIndex);
				mFboShaderList[getValidFboIndex(aFboIndex)]->setInputTextureIndex(aTexIndex);
				// TODO 20211227 check if useless now mFboShaderList[getValidFboIndex(aFboIndex)]->setInputTextureRef(mTextureList[texIndex]->getTexture());
			}
		}
		ci::gl::Texture2dRef			getFboInputTexture(unsigned int aFboIndex = 0, unsigned int aTexIndex = 0) {
			// Before 20111115 return mTextureList[mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureIndex()]->getTexture();
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTexture(aTexIndex);
		}
		void							setFboTextureMode(unsigned int aFboIndex, unsigned int aFboTextureMode) {
			mFboShaderList[getValidFboIndex(aFboIndex)]->setFboTextureMode(aFboTextureMode);
		}
		ci::gl::Texture2dRef			getFboInputTextureListItem(unsigned int aFboIndex, unsigned int aTexIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getFboInputTextureListItem(aTexIndex);
		}
		unsigned int					getFboInputTextureIndex(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureIndex();
		}
		ci::gl::Texture2dRef			getInputTexture(unsigned int aTexIndex) {
			return mFboShaderList[getValidFboIndex(mSelectedFbo)]->getInputTexture(aTexIndex);
		}
		// TODO 20211227
		unsigned int					getInputTexturesCount(unsigned int aFboIndex = 0) {
			//return (unsigned int)mTextureList.size();
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTexturesCount();
		}
		std::string						getInputTextureName(unsigned int aFboIndex) {
			// Before 20111115 return mTextureList[aTexIndex]->getName();
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getTextureName();
		}
		int								getFboInputTextureWidth(unsigned int aFboIndex) {
			//return mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->getInputTexture() ? mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->getInputTexture()->getWidth() : mVDParams->getFboWidth();
			//return mVDParams->getFboWidth();
			//return mTextureList[mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureIndex()]->getTextureWidth();
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureWidth();
		};
		int								getFboInputTextureHeight(unsigned int aFboIndex) {
			//return mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->getInputTexture() ? mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->getInputTexture()->getHeight() : mVDParams->getFboHeight();
			//return mVDParams->getFboHeight();
			//return mTextureList[mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureIndex()]->getTextureHeight();
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getInputTextureHeight();
		};

		std::string						getFboShaderName(unsigned int aFboIndex) {
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getShaderName();
		};
		std::string						getFboTextureName(unsigned int aFboIndex) {
			//return mTextureList[getValidTexIndex(aFboIndex)]->getName();
			return mFboShaderList[getValidFboIndex(aFboIndex)]->getTextureName();
			//mFboShaderList[math<int>::min(aFboIndex, mFboShaderList.size() - 1)]->getTextureName();
		};
		bool							loadImageSequence(const string& aFolder, unsigned int aTextureIndex);
		void							loadImageFile(const std::string& aFile, unsigned int aFboIndex = 0);

		std::vector<ci::gl::GlslProg::Uniform>	getFboShaderUniforms(unsigned int aFboShaderIndex);
		float							getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex);
		void							setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue);

		unsigned int					createFboShaderTexture(const JsonTree &json, unsigned int aFboIndex = 0, const std::string& aFolder = "");
		void							clearFboShaderList() {
			mFboShaderList.clear();
		}
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex) {
			if (mFboShaderList.size() == 0) return mDefaultTexture;// mTextureList[0]->getTexture();
			if (aFboIndex > mFboShaderList.size() - 1) aFboIndex = 0;

			if (mFboShaderList[aFboIndex]->isHydraTex()) {
				// fbo as inputTexture
				//mFboShaderList[aFboIndex]->setInputTextureRef(mFboShaderList[1]->getTexture());
				for (unsigned int i = 0; i < 4; i++)
				{
					mFboShaderList[aFboIndex]->setInputTextureRefByIndex(i, mFboShaderList[getValidFboIndex(aFboIndex)]->getTexture());
				}
			}
			else {
				// 20211227 useless? mFboShaderList[aFboIndex]->setInputTextureRef(mFboShaderList[aFboIndex]->getTexture());
			}

			return mFboShaderList[aFboIndex]->getRenderedTexture();

		}
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex) {
			if (mFboShaderList.size() == 0) return mDefaultTexture;// mTextureList[0]->getTexture();
			if (aFboIndex > mFboShaderList.size() - 1) aFboIndex = 0;
			return mFboShaderList[aFboIndex]->getTexture();

		}
		bool handleMouseDown(MouseEvent event)
		{
			for (unsigned int i = 0; i < mFboShaderList.size() - 1; i++)
			{
				mFboShaderList[i]->handleMouseDown(event);
			}
			return true;
		}
		bool handleMouseDrag(MouseEvent event)
		{
			for (unsigned int i = 0; i < mFboShaderList.size() - 1; i++)
			{
				mFboShaderList[i]->handleMouseDrag(event);
			}
			return true;
		}
		ci::gl::TextureRef				getMixetteTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getRenderedMixetteTexture(unsigned int aFboIndex) { return mMixetteTexture; };
		//unsigned int					fboFromJson(const JsonTree& json, unsigned int aFboIndex = 0);
		//void							selectSenderPanel();
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
		// textures
		//VDTextureList					mTextureList;
		bool							save();
		void							loadFbos();
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		//gl::TextureRef					mDefaultTexture;
		//! mixette
		gl::FboRef						mMixetteFbo;
		gl::GlslProgRef					mGlslMixette;
		ci::gl::Texture2dRef			mMixetteTexture;
		std::string						mError;
		const unsigned int				MAXSHADERS = 7;
		std::string						mAssetsPath = "";
		//const string					mixFileName = "mix.json";
		fs::path						mixPath;
		unsigned int					mCurrentSecond = 0;
		unsigned int					mCurrentIndex = 0;
		unsigned int					getValidFboIndex(unsigned int aFboIndex);
		//unsigned int					getValidTexIndex(unsigned int aTexIndex);
		//TextureSharedRef				ts;
		ci::gl::Texture2dRef			mDefaultTexture; //in case no fbos
	};
}
