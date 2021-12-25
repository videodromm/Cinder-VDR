/*
	VDFboShader
	Handles the frame buffer objects to draw into, from shaders, images, video.
*/
// TODO avoid duplicate get(Float/Int/Bool)UniformValueBy(Name/Index) which exist in VDAnimation too
// TODO recycle using object pool

#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"
#include "cinder/ImageIo.h"

// textures
#include "VDTexture.h"
//! Uniforms
#include "VDUniforms.h"
//! Animation
#include "VDAnimation.h"
// Params
#include "VDParams.h"
// video
#include "ciWMFVideoPlayer.h"

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	// stores the pointer to the VDFboShader instance
	typedef std::shared_ptr<class VDFboShader> 		VDFboShaderRef;
	typedef std::vector<VDFboShaderRef>				VDFboShaderList;
	// for profiling
	typedef std::chrono::high_resolution_clock		Clock;

	class VDFboShader {
	public:
		VDFboShader(VDUniformsRef aVDUniforms, VDAnimationRef aVDAnimation, const JsonTree &json, unsigned int aFboIndex, const std::string& aAssetsPath);
		~VDFboShader(void);
		static VDFboShaderRef create(VDUniformsRef aVDUniforms, VDAnimationRef aVDAnimation, const JsonTree &json, unsigned int aFboIndex, const std::string& aAssetsPath) {
			return std::make_shared<VDFboShader>(aVDUniforms, aVDAnimation, json, aFboIndex, aAssetsPath);
		}

		ci::gl::Texture2dRef					getTexture(); //TODO 20200610; = 0
		ci::gl::Texture2dRef					getRenderedTexture();
		//ci::gl::Texture2dRef					getInputTexture();
		bool									isValid();
		std::string								getShaderName();
		std::vector<ci::gl::GlslProg::Uniform>	getUniforms();
		//new 
		bool									setFragmentShaderString(const std::string& aFragmentShaderString, const std::string& aName = "");
		float									getUniformValueByLocation(unsigned int aLocationIndex) {
			return mUniformValueByLocation[aLocationIndex]; 
		};
		void									setUniformValueByLocation(unsigned int aLocationIndex, float aValue) { 
			mUniformValueByLocation[aLocationIndex] = aValue;
		};
		//void									setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename);
		void									setInputTextureRef(ci::gl::Texture2dRef aTextureRef) { 
			mInputTextureRef = aTextureRef; 
		};
		void									setInputTextureRefByIndex(unsigned int aTexIndex, ci::gl::Texture2dRef aTextureRef) {
			mInputTextureList[aTexIndex] = aTextureRef;
		};
		void									setFboTextureMode(unsigned int aFboTextureMode) {
			mCurrentFilename = mTextureName = "audio";
			mMode = 6;
		}
		ci::gl::Texture2dRef					getFboInputTextureListItem(unsigned int aTexIndex) {
			return mInputTextureList[aTexIndex];
		};
		std::string								getTextureName() {
			if (mIsHydraTex) {
				return "fbotex" + mInputTextureIndex;
			}
			else {
				return mStatus;// 20211212 was mCurrentFilename;
			}
		};
		
		/*unsigned int							getInputTexturesCount() {
			return 1;// mInputTextureList.size();
		}*/
		std::string								getMsg() {
			return mMsg;
		};
		std::string								getError() {
			return mError;
		};
		bool									loadFragmentShaderFromFile(const string& aFileOrPath, bool isAudio = false);
		void									setInputTextureIndex(unsigned int aTexIndex = 0) { 
			mInputTextureIndex = aTexIndex; 
		};
		unsigned int							getInputTextureIndex() {
			return mInputTextureIndex;
		};
		bool									isHydraTex() {
			return mIsHydraTex;
		}
		ci::gl::Texture2dRef					getInputTexture() {
			return mInputTextureRef;
		};
		
	private:
		// Params
		VDParamsRef						mVDParams;
		// Animation
		const VDAnimationRef			mVDAnimation;
		// uniforms
		VDUniformsRef					mVDUniforms;
		//! Input textures
		//ci::gl::Texture2dRef			mTexture;

		//! Input textures
		std::string						mInputTextureName;
		std::string						mCurrentFilename;
		std::map<unsigned int, gl::TextureRef>		mInputTextureList;
		gl::TextureRef					mInputTextureRef;
		unsigned int					mInputTextureIndex;
		unsigned int					createInputTexture(const JsonTree &json);
		// 20211115
		int								mCurrentImageSequenceIndex = 0;
		// 20211107
		string							mTextureName = "";
		string							mStatus = "";
		string							mTypestr = "";
		string							mExt = "jpg";
		int								mMode = 0;
		int								dotIndex = std::string::npos;
		// 20211107
		bool							mIsHydraTex = false;
		// 20211121
		fs::path						texFileOrPath;
		bool							fileExists;
		//! shader
		gl::GlslProgRef					mShader;
		std::vector<ci::gl::GlslProg::Uniform> mUniforms;
		std::map<unsigned int, float>	mUniformValueByLocation;
		std::string						mShaderName = "";
		std::string						mName = "";
		std::string						mShaderFileName = "";
		fs::path						mFragFilePath = "";
		bool							loadFragmentStringFromFile();
		std::string						mShaderFragmentString;

		std::string						mFileNameWithExtension;
		bool							mActive;
		int								mMicroSeconds;
		// include shader lines
		std::string						shaderInclude;
		std::string						mError;
		bool							mValid = false;
		//! Fbo
		gl::FboRef						mFbo;

		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		bool							isReady;
		ci::gl::Texture2dRef			mRenderedTexture;
		ci::gl::Texture2dRef			getFboTexture();
		void							loadNextTexture(int aCurrentIndex);
		// messages
		static const int				mMsgLength = 150;
		std::string						mMsg;
		std::string						mAssetsPath = "";
		unsigned int					mFboIndex = 0;
		// video
		ciWMFVideoPlayer				mVideo;
		float							mVideoPos;
		float							mVideoDuration;
		bool							mIsVideoLoaded = false;
		ci::gl::BatchRef				mBatchPlaneVideo;
		ci::gl::GlslProgRef				mGlslVideoTexture;
	};
}