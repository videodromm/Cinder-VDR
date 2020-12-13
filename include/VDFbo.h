/*
	VDFbo
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
// shader
#include "VDShader.h"
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
	// stores the pointer to the VDFbo instance
	typedef std::shared_ptr<class VDFbo> 			VDFboRef;
	typedef std::vector<VDFboRef>					VDFboList;
	// for profiling
	typedef std::chrono::high_resolution_clock		Clock;

	class VDFbo {
	public:
		VDFbo(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, const JsonTree &json);
		~VDFbo(void);
		static VDFboRef create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, const JsonTree &json) {
			return std::make_shared<VDFbo>(aVDSettings, aVDAnimation, aVDUniforms, json);
		}
		// fbo mix several fbos   fbo=shader+texture(s)
		// get live rendered texture
		ci::gl::Texture2dRef					getTexture(); //TODO 20200610; = 0
		ci::gl::Texture2dRef					getRenderedTexture() {
			//last frame rendered
			return mRenderedTexture;
		}
		bool									setFragmentString(const std::string& aFragmentShaderString, const std::string& aName = "");
		bool									loadFragmentStringFromFile(const std::string& aFileName);
		bool									isValid();

		std::string								getName();
		std::string								getShaderName();
		std::string								getTextureName();
		ci::gl::Texture2dRef					getInputTexture();
		std::string								getStatus();
		void									setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename);
		void									updateThumbFile();
		std::vector<ci::gl::GlslProg::Uniform>	getUniforms();
		ci::JsonTree							toJson(bool save, unsigned int aIndex) const;
		// uniforms
		bool									getGlobal();
		void									toggleGlobal();
		
		
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
		VDTextureList					mTextureList;
		unsigned int					mInputTextureIndex;
		unsigned int					createInputTexture(const JsonTree& json);

		//! shader
		gl::GlslProgRef					mShader;
		VDShaderRef						shaderToLoad;
		std::vector<ci::gl::GlslProg::Uniform> mUniforms;
		std::string						mShaderName = "";
		std::string						mName = "";
		std::string						mShaderFileName = "";

		std::string						mShaderFragmentString;
		std::string						mFileNameWithExtension;
		bool							mActive;
		int								mMicroSeconds;
		bool							mGlobal = true;

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