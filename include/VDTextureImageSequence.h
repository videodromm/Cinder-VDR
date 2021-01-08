/*
	---- TextureImageSequence ------------------------------------------------
	implements VDTexture
*/
/*#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
// VDUniforms
#include "VDUniforms.h"
// Texture
#include "VDTexture.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
typedef std::shared_ptr<class TextureImageSequence>	TextureImageSequenceRef;

class TextureImageSequence
	: public VDTexture {
public:
	//
	static TextureImageSequenceRef	create(VDUniformsRef aVDUniforms) { return std::make_shared<TextureImageSequence>(aVDUniforms); }
	//!
	//bool					fromXml(const XmlTree& xml) override;
	//!
	//virtual	XmlTree			toJson() const override;
	//!
	virtual bool			loadFromFullPath(const std::string& aPath) override;
	TextureImageSequence(VDUniformsRef aVDUniforms);
	virtual ~TextureImageSequence(void);

	//! returns a shared pointer 
	TextureImageSequenceRef	getPtr() { return std::static_pointer_cast<TextureImageSequence>(shared_from_this()); }
	void							stopSequence();
	void							toggleLoadingFromDisk() override;
	bool							isLoadingFromDisk() override;
	//void							stopLoading();
	//int							getPlayheadPosition() override;
	void							setPlayheadPosition(int position) override;

	float							getSpeed() override;
	void							setSpeed(float speed) override;
	void							reverse() override;

	bool							isValid() { return mFramesLoaded > 0; };
	int								getMaxFrame() override;
	//ci::gl::Texture2dRef			getNextTexture() override;
protected:
	//! 
	virtual ci::gl::Texture2dRef	getTexture(unsigned int aIndex = 0) override;

private:
	// Uniforms
	VDUniformsRef				mVDUniforms;

	float						playheadFrameInc;
	void						loadNextImageFromDisk();
	void						updateSequence();
	bool						mIsSequence;
	std::string						mFolder;
	std::string						mPrefix;
	std::string						mExt;
	int							mNumberOfDigits;
	int							mNextIndexFrameToTry;
	int							mCurrentLoadedFrame;
	int							mFramesLoaded;
	//int							mPlayheadPosition;
	bool						mLoadingPaused;
	bool						mLoadingFilesComplete;
	float						mSpeed;
	std::vector<ci::gl::TextureRef>	mSequenceTextures;
};
}*/
