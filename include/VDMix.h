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
#include "cinder/Xml.h"
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
#include "VDFbo.h"
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
	/*struct VDMixFbo
	{
		ci::gl::FboRef					fbo;
		ci::gl::Texture2dRef			texture;
		string							name;
	};*/
	class VDMix {
	public:
		VDMix(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
		static VDMixRef					create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
		{
			return std::shared_ptr<VDMix>(new VDMix(aVDSettings, aVDAnimation, aVDUniforms));
		}
		// RTE in release mode 
		//ci::gl::Texture2dRef			getRenderedTexture(bool reDraw = true);
		// fbolist
		unsigned int					getFboListSize() { 
			return mFboList.size(); 
		};
		bool							isFboValid(unsigned int aFboIndex) {
			bool valid = false;
			if (mFboList.size() > 0) {
				valid = mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->isValid();
			}
			return valid;
			
		};
		/*bool							getFboBoolUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getBoolUniformValueByIndex(aCtrl);
		};

		void							toggleFboValue(unsigned int aCtrl, unsigned int aFboIndex) {
			mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->toggleValue(aCtrl);
		};
		int								getFboIntUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getIntUniformValueByIndex(aCtrl);
		};

		float							getFboFloatUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getUniformValue(aCtrl);
		};
		bool							setFboFloatUniformValueByIndex(unsigned int aCtrl, unsigned int aFboIndex, float aValue) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->setUniformValue(aCtrl, aValue);
		};*/
		bool									getGlobal(unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getGlobal();
		};
		void									toggleGlobal(unsigned int aFboIndex) {
			mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->toggleGlobal();
		};
		std::string							getFboStatus(unsigned int aFboIndex = 0) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getStatus();
		}
		void							updateShaderThumbFile(unsigned int aFboIndex) {
			mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->updateThumbFile();
		}
		std::string							getFboInputTextureName(unsigned int aFboIndex = 0) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getTextureName();
		}
		ci::gl::Texture2dRef							getFboInputTexture(unsigned int aFboIndex = 0) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getInputTexture();
		}
		int								getFboInputTextureWidth(unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getInputTexture() ? mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getInputTexture()->getWidth() : mVDParams->getFboWidth();
		};
		int								getFboInputTextureHeight(unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getInputTexture() ? mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getInputTexture()->getHeight() : mVDParams->getFboHeight();
		};
		std::string							getFboName(unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getName();
		};
		std::string							getFboShaderName(unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getShaderName();
		};
		std::string							getFboTextureName(unsigned int aFboIndex) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getTextureName();
		};
		void							saveFbos() {
			unsigned int i = 0;
			for (auto &fbo : mFboList) {
				JsonTree		json = fbo->toJson(true, i);
				i++;
			}
		};
		std::vector<ci::gl::GlslProg::Uniform>			getUniforms(unsigned int aFboIndex = 0) {
			return mFboList[math<int>::min(aFboIndex, mFboList.size() - 1)]->getUniforms();
		}
		int												loadFragmentShader(const std::string& aFilePath, unsigned int aFboShaderIndex) {
			int rtn = -1;
			mVDSettings->mMsg = "load " + aFilePath + " at index " + toString(aFboShaderIndex) + "\n" + mVDSettings->mMsg.substr(0, mVDSettings->mMsgLength);
			bool loaded = false;
			for (auto &fbo : mFboList) {
				rtn++;
				if (!loaded) {
					if (!fbo->isValid()) {
						fbo->loadFragmentStringFromFile(aFilePath);
						loaded = true;
						break;
					}
				}
			}
			if (!loaded) {
				rtn = math<int>::min(aFboShaderIndex, mFboList.size() - 1);
				loaded = mFboList[rtn]->loadFragmentStringFromFile(aFilePath);
			}
			mVDSettings->mMsg = "loaded " + toString(loaded) + " at index " + toString(rtn) + "\n" + mVDSettings->mMsg.substr(0, mVDSettings->mMsgLength);

			return rtn;
		}
		void											loadImageFile(const std::string& aFile, unsigned int aTextureIndex) {
			int rtn = math<int>::min(aTextureIndex, mFboList.size() - 1);
			fs::path texFileOrPath = aFile;
			if (fs::exists(texFileOrPath)) {

				std::string ext = "";
				int dotIndex = texFileOrPath.filename().string().find_last_of(".");
				if (dotIndex != std::string::npos)  ext = texFileOrPath.filename().string().substr(dotIndex + 1);
				if (ext == "jpg" || ext == "png") {
					if (rtn == -1 || mFboList.size() < 1) {
						// no fbos, create one
						JsonTree		json;
						JsonTree shader = ci::JsonTree::makeArray("shader");
						shader.addChild(ci::JsonTree("shadername", "inputImage.fs"));
						shader.pushBack(ci::JsonTree("shadertype", "fs"));						
						json.addChild(shader);
						JsonTree texture = ci::JsonTree::makeArray("texture");
						texture.addChild(ci::JsonTree("texturename", texFileOrPath.filename().string()));
						texture.pushBack(ci::JsonTree("texturetype", "image"));
						json.addChild(texture);
						
						createFboShaderTexture(json);
					}
					else {
						ci::gl::Texture2dRef mTexture = gl::Texture::create(loadImage(texFileOrPath), gl::Texture2d::Format().loadTopDown().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));
						mFboList[rtn]->setImageInputTexture(mTexture, texFileOrPath.filename().string());
					}					
				}
			}			
		}
		unsigned int									createFboShaderTexture(const JsonTree &json, unsigned int aFboIndex = 0) {
			unsigned int rtn = 0;
			VDFboRef fboRef = VDFbo::create(mVDSettings, mVDAnimation, mVDUniforms, json);
			if (aFboIndex == 0) {
				mFboList.push_back(fboRef);
				rtn = mFboList.size() - 1;
			}
			else {
				rtn = math<int>::min(aFboIndex, mFboList.size() - 1);
				mFboList[rtn] = fboRef;			
			}

			return rtn;
		}
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex) {
			if (mFboList.size() == 0) return mDefaultTexture;
			if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
			return mFboList[aFboIndex]->getRenderedTexture();

		}
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex) {
			if (mFboList.size() == 0) return mDefaultTexture;
			if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
			return mFboList[aFboIndex]->getTexture();

		}
		ci::gl::TextureRef				getMixetteTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getRenderedMixetteTexture(unsigned int aFboIndex) { return mMixetteTexture; };

	private:
		// Params
		VDParamsRef						mVDParams;
		// Animation
		VDAnimationRef					mVDAnimation;
		// Settings
		VDSettingsRef					mVDSettings;
		// Uniforms
		VDUniformsRef					mVDUniforms;

		//! Fbos
		// maintain a list of fbos specific to this mix
		VDFboList						mFboList;
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		gl::TextureRef					mDefaultTexture;
		//! mixette
		gl::FboRef						mMixetteFbo;
		gl::GlslProgRef					mGlslMixette;
		ci::gl::Texture2dRef			mMixetteTexture;
		std::string						mError;
	};
}
