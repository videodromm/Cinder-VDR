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
// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Uniforms
#include "VDUniforms.h"
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
	//typedef std::vector<VDFboRef>					VDFboList;
	// for profiling
	typedef std::chrono::high_resolution_clock		Clock;

	class VDFboShader {
	public:
		VDFboShader(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
		~VDFboShader(void);
		static VDFboShaderRef create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms) {
			return std::make_shared<VDFboShader>(aVDSettings, aVDAnimation, aVDUniforms);
		}

		ci::gl::Texture2dRef					getFboShaderTexture(); //TODO 20200610; = 0
		bool									isValid();
		std::string								getName();
		std::string								getShaderName();
		void									setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename);
		std::vector<ci::gl::GlslProg::Uniform>	getUniforms();
		//new 
		bool setFragmentShaderString(unsigned int aShaderIndex, const std::string& aFragmentShaderString, const std::string& aName = "");
	private:
		// Params
		VDParamsRef						mVDParams;
		// Settings
		VDSettingsRef					mVDSettings;
		// Animation
		const VDAnimationRef			mVDAnimation;
		// uniforms
		VDUniformsRef					mVDUniforms;
		//! Input textures
		ci::gl::Texture2dRef			mTexture;

		//VDTextureList					mTextureList;
		//unsigned int					mInputTextureIndex;
		//! shader
		gl::GlslProgRef					mShader;
		std::vector<ci::gl::GlslProg::Uniform> mUniforms;
		std::string						mShaderName = "";
		std::string						mName = "";
		std::string						mShaderFileName = "";

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
	};
}