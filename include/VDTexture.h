#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"

// Params
#include "VDParams.h"
// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"

// base64 for stream
#include "cinder/Base64.h"

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	/*
	** ---- Texture parent class ------------------------------------------------
	*/
	// stores the pointer to the VDTexture instance
	typedef std::shared_ptr<class VDTexture> 	VDTextureRef;
	typedef std::vector<VDTextureRef>			VDTextureList;
	// for profiling
	typedef std::chrono::high_resolution_clock Clock;

	class VDTexture : public std::enable_shared_from_this < VDTexture > {
	public:
		enum class TextureType { UNKNOWN, IMAGE, SEQUENCE, CAMERA, SHARED, AUDIO, STREAM } ;
	public:
		VDTexture(TextureType aType = TextureType::UNKNOWN);
		VDTexture(VDParamsRef aVDParams, const JsonTree& json);
		static VDTextureRef	create(VDParamsRef aVDParams, const JsonTree& json);
		
		virtual ~VDTexture(void);
		void							setImageInputTexture(ci::gl::Texture2dRef aTextureRef, const std::string& aTextureFilename) {
			mType = TextureType::IMAGE;
			mTexture = aTextureRef;
			mName = aTextureFilename;
			
		};
		virtual ci::gl::Texture2dRef	getTexture(unsigned int aIndex = 0);
		//! returns a shared pointer to this input texture
		VDTextureRef					getPtr() { return shared_from_this(); }
		ci::ivec2						getSize();
		ci::Area						getBounds();
		GLuint							getId();
		//! returns the type
		//TextureType						getType() { return mType; };
		int								getMode() { return (int)mType; };
		std::string						getName();
		unsigned int					getTextureWidth();
		unsigned int					getTextureHeight();
		unsigned int					getOriginalWidth();
		unsigned int					getOriginalHeight();
		//!
		//virtual bool					fromXml(const ci::XmlTree &xml);
		//!
		//virtual XmlTree					toXml() const;
		//! read a xml file and pass back a vector of VDTextures
		//static VDTextureList			readSettings(VDAnimationRef aVDAnimation, const ci::DataSourceRef &source);
		//! write a xml file
		//static void						writeSettings(const VDTextureList &vdtexturelist, const ci::DataTargetRef &target);
		virtual bool					loadFromFullPath(const std::string& aPath);
		std::string						getStatus() { return mStatus; };
		//! area to display
		void							lockBounds(bool lock, unsigned int aWidth, unsigned int aHeight);
		void							setXLeft(int aXleft);
		void							setYTop(int aYTop);
		void							setXRight(int aXRight);
		void							setYBottom(int aYBottom);
		int								getXLeft() { return mXLeft; };
		int								getYTop() { return mYTop; };
		int								getXRight() { return mXRight; };
		int								getYBottom() { return mYBottom; };
		bool							isFlipH() { return mFlipH; };
		bool							isFlipV() { return mFlipV; };
		void							flipV();
		void							flipH();
		bool							getLockBounds();
		void							toggleLockBounds();
		// sequence and movie
		void							togglePlayPause();
		// sequence only
		virtual void					toggleLoadingFromDisk();
		virtual bool					isLoadingFromDisk();
		void							syncToBeat();
		virtual void					reverse();
		virtual float					getSpeed();
		virtual void					setSpeed(float speed);
		//virtual int					getPlayheadPosition();
		int								getPosition() { return mPosition; };
		virtual void					setPlayheadPosition(int position);
		virtual int						getMaxFrame();
		//virtual ci::gl::Texture2dRef	getNextTexture();
	protected:
		// Params
		VDParamsRef						mVDParams;
		std::string						mName;
		bool							mFlipV;
		bool							mFlipH;
		std::string						mPath;
		std::string						mFolder;
		unsigned int 					mWidth;
		unsigned int					mHeight;
		unsigned int 					mAreaWidth;
		unsigned int					mAreaHeight;
		int								mPosition;
		//! Texture
		ci::gl::Texture2dRef			mTexture;
		//! Surface
		Surface							mInputSurface;
		Surface							mProcessedSurface;
		int								mXLeft, mYTop, mXRight, mYBottom, mOriginalWidth, mOriginalHeight;
		bool							mBoundsLocked;
		bool							mSyncToBeat;
		bool							mPlaying;
		//! Fbo for audio only for now
		gl::FboRef						mFbo;
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		ci::gl::Texture2dRef			mRenderedTexture;
		// from rewrite
		map<string, ci::gl::TextureRef>	mCachedTextures;
		string							mLastCachedFilename = "";
		string							mCurrentSeqFilename = "";
		string							mStatus = "";
		TextureType						mType = TextureType::UNKNOWN;
		string							mTypestr = "";
		string							mExt = "jpg";
		int								mMode = 0;
		std::string						mAssetsPath = "";
	private:
	};
}
