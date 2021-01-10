#pragma onceB

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
//#include "cinder/Xml.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"

#if defined( CINDER_MSW )
// spout
#include "CiSpoutIn.h"
#endif

#if defined( NDI_RECEIVER )
// ndi
#include "CinderNDIReceiver.h"
#endif

#if defined( CINDER_MAC )
// syphon
#include "cinderSyphon.h"
#endif

// Animation
#include "VDAnimation.h"

// audio
#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/audio/Source.h"
#include "cinder/audio/Target.h"
#include "cinder/audio/dsp/Converter.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/SampleRecorderNode.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/Rand.h"
// base64 for stream
#include "cinder/Base64.h"

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;
//using namespace std;


namespace videodromm
{
	/*
	** ---- Texture parent class ------------------------------------------------
	*/
	// stores the pointer to the VDTexture instance
	typedef std::shared_ptr<class VDTexture> 	VDTextureRef;
	typedef std::vector<VDTextureRef>			VDTextureList;
	//typedef map<string, ci::gl::TextureRef>		VDCachedTextures;

	// for profiling
	typedef std::chrono::high_resolution_clock Clock;

	class VDTexture : public std::enable_shared_from_this < VDTexture > {
	public:
		typedef enum { UNKNOWN, IMAGE, SEQUENCE, CAMERA, SHARED, AUDIO, STREAM } TextureType;
	public:
		VDTexture(TextureType aType = UNKNOWN);
		virtual ~VDTexture(void);

		virtual ci::gl::Texture2dRef	getTexture();
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename);
		//! returns a shared pointer to this input texture
		VDTextureRef					getPtr() { return shared_from_this(); }
		ci::ivec2						getSize();
		ci::Area						getBounds();
		GLuint							getId();
		//! returns the type
		TextureType						getType() { return mType; };
		std::string						getName();
		unsigned int					getTextureWidth();
		unsigned int					getTextureHeight();
		unsigned int					getOriginalWidth();
		unsigned int					getOriginalHeight();
		//!
		virtual bool					fromJson(const JsonTree& json);
		//!
		//virtual XmlTree					toJson() const;
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
		bool							getLockBounds();
		void							toggleLockBounds();
		// sequence and movie
		void							togglePlayPause();
		// sequence only
		virtual void					toggleLoadingFromDisk();
		virtual bool					isLoadingFromDisk();
		void							syncToBeat();
		void							reverse();
		float							getSpeed();
		void							setSpeed(float speed);
		//virtual int					getPlayheadPosition();
		int								getPosition() { return mPosition; };
		virtual void					setPlayheadPosition(int position);
		virtual int						getMaxFrame();
		//virtual ci::gl::Texture2dRef	getNextTexture();

	protected:
		std::string						mName;
		TextureType						mType;
		std::string						mPath;
		std::string						mAssetsPath;
		unsigned int 					mWidth;
		unsigned int					mHeight;
		unsigned int 					mAreaWidth;
		unsigned int					mAreaHeight;
		int								mPosition;
		std::string						mStatus;
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

		float							mSpeed;
	private:
	};
	/*
	** ---- TextureImage ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureImage>	TextureImageRef;

	class TextureImage
		: public VDTexture {
	public:
		//
		static TextureImageRef	create() { return std::make_shared<TextureImage>(); }
		//!
		bool					fromJson(const JsonTree& json) override;
		//!
		//virtual	XmlTree			toJson() const override;
		virtual bool			loadFromFullPath(const std::string& aPath) override;

	public:
		TextureImage();
		virtual ~TextureImage(void);

		//! returns a shared pointer
		TextureImageRef	getPtr() { return std::static_pointer_cast<TextureImage>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename) override;
	};

	/*
	** ---- TextureImageSequence ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureImageSequence>	TextureImageSequenceRef;

	class TextureImageSequence
		: public VDTexture {
	public:
		//
		static TextureImageSequenceRef	create(VDAnimationRef aVDAnimation) { return std::make_shared<TextureImageSequence>(aVDAnimation); }
		//!
		bool					fromJson(const JsonTree& json) override;
		//!
		//virtual	XmlTree			toJson() const override;
		//!
		virtual bool			loadFromFullPath(const std::string& aPath) override;
		TextureImageSequence(VDAnimationRef aVDAnimation);
		virtual ~TextureImageSequence(void);

		//! returns a shared pointer 
		TextureImageSequenceRef	getPtr() { return std::static_pointer_cast<TextureImageSequence>(shared_from_this()); }
		void							stopSequence();
		void							toggleLoadingFromDisk() override;
		bool							isLoadingFromDisk() override;
		//void							stopLoading();
		//int							getPlayheadPosition() override;
		void							setPlayheadPosition(int position) override;

		bool							isValid() { return mFramesLoaded > 0; };
		int								getMaxFrame() override;
		//ci::gl::Texture2dRef			getNextTexture() override;
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename) override;

	private:
		// Animation
		VDAnimationRef				mVDAnimation;

		float						playheadFrameInc;
		void						loadNextImageFromDisk();
		void						updateSequence();
		bool						mIsSequence;
		std::string					mFolder;
		std::string					mPrefix;
		std::string					mExt;
		int							mNumberOfDigits;
		int							mNextIndexFrameToTry;
		std::chrono::steady_clock::time_point	startGlobal;
		int							mCurrentLoadedFrame;
		int							mFramesLoaded;
		int							currentSecond;
		//int						mPlayheadPosition;
		bool						mLoadingPaused;
		bool						mLoadingFilesComplete;
		std::string					mLastCachedFilename;

		vector<ci::gl::TextureRef>	mSequenceTextures;
		map<string, ci::gl::TextureRef>	mCachedTextures;
	};

	/*
	** ---- TextureCamera ------------------------------------------------
	*/
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
	typedef std::shared_ptr<class TextureCamera>	TextureCameraRef;

	class TextureCamera
		: public VDTexture {
	public:
		//
		static TextureCameraRef create() { return std::make_shared<TextureCamera>(); }
		//!
		bool				fromJson(const JsonTree& json) override;
		//!
		//virtual	XmlTree	toJson() const override;

	public:
		TextureCamera();
		virtual ~TextureCamera(void);

		//! returns a shared pointer 
		TextureCameraRef	getPtr() { return std::static_pointer_cast<TextureCamera>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename) override;
	private:
		void printDevices();
		std::string				mFirstCameraDeviceName;
		CaptureRef				mCapture;
	};
#endif
	/*
	** ---- TextureShared ------------------------------------------------
	*/
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
	typedef std::shared_ptr<class TextureShared>	TextureSharedRef;

	class TextureShared
		: public VDTexture {
	public:
		//
		static TextureSharedRef create() { return std::make_shared<TextureShared>(); }
		//!
		bool				fromJson(const JsonTree& json) override;
		//!
		//virtual	XmlTree	toJson() const override;

	public:
		TextureShared();
		virtual ~TextureShared(void);

		//! returns a shared pointer 
		TextureSharedRef	getPtr() { return std::static_pointer_cast<TextureShared>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename) override;
	private:
#if defined( CINDER_MSW )
		// -------- SPOUT -------------
		SpoutIn							mSpoutIn;
		//CinderNDIReceiver				mReceiver;
#endif
#if defined( CINDER_MAC )
		syphonClient					mClientSyphon;
#endif
		ci::gl::Texture2dRef			mTexture;
	};
#endif
	/*
	** ---- TextureAudio ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureAudio>	TextureAudioRef;

	class TextureAudio
		: public VDTexture {
	public:
		//
		static TextureAudioRef	create(VDAnimationRef aVDAnimation) { return std::make_shared<TextureAudio>(aVDAnimation); }
		//!
		bool					fromJson(const JsonTree& json) override;
		//!
		//virtual	XmlTree			toJson() const override;
		//!
		virtual bool			loadFromFullPath(const std::string& aPath) override;

	public:
		TextureAudio(VDAnimationRef aVDAnimation);
		virtual ~TextureAudio(void);

		//! returns a shared pointer 
		TextureAudioRef					getPtr() { return std::static_pointer_cast<TextureAudio>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename) override;
	private:
		// Animation
		VDAnimationRef					mVDAnimation;
		// Uniforms
		VDUniformsRef					mVDUniforms;
		// init
		bool							mLineInInitialized;
		bool							mWaveInitialized;
		//void							initializeLineIn();
		//audio::Context*					audioContext;
		// audio
		audio::InputDeviceNodeRef		mLineIn;
		audio::MonitorSpectralNodeRef	mMonitorLineInSpectralNode;
		audio::MonitorSpectralNodeRef	mMonitorWaveSpectralNode;
		audio::SamplePlayerNodeRef		mSamplePlayerNode;
		audio::SourceFileRef			mSourceFile;
		audio::MonitorSpectralNodeRef	mScopeLineInFmt;
		audio::BufferPlayerNodeRef		mBufferPlayerNode;

		std::vector<float>				mMagSpectrum;

		// number of frequency bands of our spectrum
		static const int				kBands = 16;

		// textures
		unsigned char					dTexture[256];// MUST be < mVDAnimation->mWindowSize
		ci::gl::Texture2dRef			mTexture;
		//WaveformPlot					mWaveformPlot;
	};

	/*
	** ---- TextureStream ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureStream>	TextureStreamRef;

	class TextureStream
		: public VDTexture {
	public:
		//
		static TextureStreamRef	create(VDAnimationRef aVDAnimation) { return std::make_shared<TextureStream>(aVDAnimation); }
		//!
		bool					fromJson(const JsonTree& json) override;
		//!
		//virtual	XmlTree			toJson() const override;
		//!
		virtual bool			loadFromFullPath(const std::string& aStream) override;

	public:
		TextureStream(VDAnimationRef aVDAnimation);
		virtual ~TextureStream(void);

		//! returns a shared pointer 
		TextureStreamRef				getPtr() { return std::static_pointer_cast<TextureStream>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		virtual ci::gl::Texture2dRef	getCachedTexture(const std::string& aFilename) override;
	private:
		// Animation
		VDAnimationRef					mVDAnimation;
		// textures
		ci::gl::Texture2dRef			mTexture;

	};
}


/*
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
		//virtual XmlTree					toJson() const;
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
}*/
