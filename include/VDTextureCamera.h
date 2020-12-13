/*
	---- TextureCamera ------------------------------------------------
	implements VDTexture
*/
#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
// Animation
#include "VDAnimation.h"
// Texture
#include "VDTexture.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	typedef std::shared_ptr<class TextureCamera>	TextureCameraRef;

	class TextureCamera
		: public VDTexture {
	public:
		//
		static TextureCameraRef create() { return std::make_shared<TextureCamera>(); }
		//!
		//bool				fromXml(const XmlTree& xml) override;
		//!
		//virtual	XmlTree	toXml() const override;

	public:
		TextureCamera();
		virtual ~TextureCamera(void);

		//! returns a shared pointer 
		//TextureCameraRef	getPtr() { return std::static_pointer_cast<TextureCamera>(std::shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture(unsigned int aIndex = 0) override;
	private:
		void printDevices();
		std::string					mFirstCameraDeviceName;
		CaptureRef				mCapture;
	};
}