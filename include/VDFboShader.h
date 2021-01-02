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
//!  Uniforms
#include "VDUniforms.h"
// Params
#include "VDParams.h"

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
		VDFboShader(VDUniformsRef aVDUniforms, const JsonTree &json);
		~VDFboShader(void);
		static VDFboShaderRef create(VDUniformsRef aVDUniforms, const JsonTree &json) {
			return std::make_shared<VDFboShader>(aVDUniforms, json);
		}

		ci::gl::Texture2dRef					getTexture(); //TODO 20200610; = 0
		ci::gl::Texture2dRef					getRenderedTexture();
		ci::gl::Texture2dRef					getInputTexture();
		bool									isValid();
		std::string								getShaderName();
		void									setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename);
		std::vector<ci::gl::GlslProg::Uniform>	getUniforms();
		//new 
		bool									setFragmentShaderString(const std::string& aFragmentShaderString, const std::string& aName = "");
		int										getUniformValueByLocation(unsigned int aLocationIndex) {
			return mUniformValueByLocation[aLocationIndex]; 
		};
		void									setUniformValueByLocation(unsigned int aLocationIndex, float aValue) { 
			mUniformValueByLocation[aLocationIndex] = aValue;
		};
		std::string								getTextureName() {
			return mTextureList[0]->getName();
		};
		std::string								getMsg() {
			return mMsg;
		};
		std::string								getError() {
			return mError;
		};
		bool									loadFragmentStringFromFile(const string& aFileOrPath);
	private:
		// Params
		VDParamsRef						mVDParams;
		// Animation
		//const VDAnimationRef			mVDAnimation;
		// uniforms
		VDUniformsRef					mVDUniforms;
		//! Input textures
		//ci::gl::Texture2dRef			mTexture;

		//! Input textures
		std::string						mTextureName;
		VDTextureList					mTextureList;
		unsigned int					mInputTextureIndex;
		unsigned int					createInputTexture(const JsonTree &json);
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
		std::string						mDefaultVertexString;
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
		// messages
		static const int				mMsgLength = 150;
		std::string						mMsg;
		std::string						mAssetsPath = "";
	};
}