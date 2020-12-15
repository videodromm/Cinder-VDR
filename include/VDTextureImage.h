/*
	---- TextureImage ------------------------------------------------
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
	typedef std::shared_ptr<class TextureImage>	TextureImageRef;

	class TextureImage
		: public VDTexture {
	public:
		//
		static TextureImageRef	create() { return std::make_shared<TextureImage>(); }
		//!
		//bool					fromXml(const XmlTree &xml) override;
		//!
		//virtual	XmlTree			toXml() const override;
		virtual bool			loadFromFullPath(const std::string& aPath) override;

	public:
		TextureImage();
		virtual ~TextureImage(void);

		//! returns a shared pointer
		TextureImageRef	getPtr() { return std::static_pointer_cast<TextureImage>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture(unsigned int aIndex = 0) override;
	};
}
