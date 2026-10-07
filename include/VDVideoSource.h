#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Filesystem.h"
#if defined( CINDER_MSW )
// WIN32_LEAN_AND_MEAN leaves OLE automation (V_VT...) out of windows.h; the player's
// Media Foundation headers (propvarutil.h) need it
#include <ole2.h>
#include "ciWMFVideoPlayer.h"
#endif
#include <memory>
#include <string>

namespace videodromm
{
	// A video in the shared texture pool (VDMix): its own player, independent of any fbo. Fbos
	// only reference its texture (a plain GL_TEXTURE_2D, stable for the source's lifetime), so
	// several fbos can show it, and it keeps playing when an fbo switches to another input.
	// Windows only (Media Foundation); create() returns nullptr elsewhere.
	typedef std::shared_ptr<class VDVideoSource> VDVideoSourceRef;

	class VDVideoSource {
	public:
		// aAudioDevice: output device name for the video's sound (VDAnimation's preferred output)
		static VDVideoSourceRef create(const std::string& aPath, const std::string& aAudioDevice, const ci::ivec2& aSize, bool aAutoPlay) {
#if defined( CINDER_MSW )
			VDVideoSourceRef source(new VDVideoSource());
			if (source->load(aPath, aAudioDevice, aSize, aAutoPlay)) return source;
#endif
			return nullptr;
		}
		// basename, the pool entry's name
		const std::string&		getName() const { return mName; }
		const std::string&		getPath() const { return mPath; }
		ci::gl::Texture2dRef	getTexture() const { return mBlitFbo ? mBlitFbo->getColorTexture() : nullptr; }

		// decodes and copies the current frame, every frame (VDMix::updateVideoSources)
		void update() {
#if defined( CINDER_MSW )
			mVideo.update();
			if (mReversed && mVideo.isPlaying()) {
				// native negative-rate playback isn't reliable on this backend: step backward by hand
				float fps = mVideo.getFrameRate();
				if (fps > 0.0f) {
					float newPos = mVideo.getPosition() - (1.0f / fps);
					if (newPos < 0.0f) newPos = mVideo.isLooping() ? (mVideo.getDuration() + newPos) : 0.0f;
					mVideo.setPosition(newPos);
				}
			}
			if (!mVideo.hasTexture() || !mBlitShader || !mBlitFbo) return;
			// the player's texture is GL_TEXTURE_RECTANGLE: blit it into a plain GL_TEXTURE_2D for
			// every shader (sampler2D, normalized UVs). The DX/GL interop object must be locked
			// around any GL access, or the GL side never sees the decoded frames (black texture)
			ci::gl::TextureRef rectTex = mVideo.getTexture();
			mVideo.lockSharedTexture();
			{
				ci::gl::ScopedFramebuffer scopedFbo(mBlitFbo);
				ci::gl::ScopedViewport scopedViewport(ci::ivec2(0), mBlitFbo->getSize());
				ci::gl::ScopedMatrices scopedMatrices;
				ci::gl::setMatricesWindow(mBlitFbo->getSize());
				rectTex->bind(0);
				ci::gl::ScopedGlslProg scopedShader(mBlitShader);
				mBlitShader->uniform("uSampler", 0);
				mBlitShader->uniform("uVideoSize", ci::vec2((float)rectTex->getWidth(), (float)rectTex->getHeight()));
				// Y span swapped: the decoded frame is top-down (loadTopDown(true) in ciWMFVideoPlayer)
				ci::gl::drawSolidRect(ci::Rectf(0, (float)mBlitFbo->getHeight(), (float)mBlitFbo->getWidth(), 0));
				rectTex->unbind(0);
			}
			mVideo.unlockSharedTexture();
#endif
		}

		bool isPlaying() {
#if defined( CINDER_MSW )
			return mVideo.isPlaying();
#else
			return false;
#endif
		}
		void play() {
#if defined( CINDER_MSW )
			// a non-looping video pauses on its last frame: play it again from the start
			if (mVideo.getDuration() > 0.0f && mVideo.getPosition() >= mVideo.getDuration() - 0.05f) mVideo.setPosition(0.0f);
			mVideo.play();
#endif
		}
		void pause() {
#if defined( CINDER_MSW )
			if (mVideo.isPlaying()) mVideo.pause();
#endif
		}
		void togglePlayPause() { if (isPlaying()) pause(); else play(); }
		bool isLooping() const { return mLoop; }
		void toggleLoop() {
			mLoop = !mLoop;
#if defined( CINDER_MSW )
			mVideo.setLoop(mLoop);
#endif
		}
		bool isReversed() const { return mReversed; }
		void toggleReverse() { mReversed = !mReversed; }
		float getSpeed() {
#if defined( CINDER_MSW )
			return mVideo.getSpeed();
#else
			return 1.0f;
#endif
		}
		void setSpeed(float aSpeed) {
#if defined( CINDER_MSW )
			// many codecs reject a non-1.0 rate unless thinning (dropping delta frames) is enabled
			if (!mVideo.setSpeed(aSpeed, false) && !mVideo.setSpeed(aSpeed, true) && !mSpeedWarningLogged) {
				mSpeedWarningLogged = true;
				CI_LOG_W("video " << mName << ": setSpeed(" << aSpeed << ") rejected by the codec, with and without thinning");
			}
#endif
		}
		// volume level (texture/fbo pane slider); VDMix applies level x the highest weight of the
		// fbos showing it, 0 while scrubbing
		float getVolumeLevel() const { return mVolumeLevel; }
		void setVolumeLevel(float aLevel) { mVolumeLevel = ci::math<float>::clamp(aLevel, 0.0f, 1.0f); }
		void setScrubbing(bool aScrubbing) { mScrubbing = aScrubbing; }
		bool isScrubbing() const { return mScrubbing; }
		void setOutputVolume(float aVolume) {
#if defined( CINDER_MSW )
			// only on change: every frame otherwise
			if (aVolume != mOutputVolume) {
				mOutputVolume = aVolume;
				mVideo.setVolume(aVolume);
			}
#endif
		}
		int getPosition() {
#if defined( CINDER_MSW )
			float fps = mVideo.getFrameRate();
			return fps > 0.0f ? (int)(mVideo.getPosition() * fps + 0.5f) : 0;
#else
			return 0;
#endif
		}
		void setPlayheadPosition(int aFrame) {
#if defined( CINDER_MSW )
			float fps = mVideo.getFrameRate();
			if (fps > 0.0f) mVideo.setPosition(aFrame / fps);
#endif
		}
		int getMaxFrame() {
#if defined( CINDER_MSW )
			float fps = mVideo.getFrameRate();
			return fps > 0.0f ? (int)(mVideo.getDuration() * fps + 0.5f) : 0;
#else
			return 0;
#endif
		}

	private:
		VDVideoSource() = default;
#if defined( CINDER_MSW )
		bool load(const std::string& aPath, const std::string& aAudioDevice, const ci::ivec2& aSize, bool aAutoPlay) {
			mPath = aPath;
			mName = ci::fs::path(aPath).filename().string();
			if (!mVideo.loadMovie(aPath, aAudioDevice)) {
				CI_LOG_E("video " << aPath << " could not be loaded");
				return false;
			}
			mVideo.setLoop(mLoop);
			// WMF only presents a frame once the session has started: start, then pause right
			// away when not autoplaying, so the texture shows the first frame instead of black
			mVideo.play();
			if (!aAutoPlay) mVideo.pause();
			try {
				mBlitShader = ci::gl::GlslProg::create(ci::gl::GlslProg::Format()
					.vertex(ci::app::loadAsset("video_texture.vs.glsl"))
					.fragment(ci::app::loadAsset("video_texture.fs.glsl")));
			}
			catch (const std::exception& ex) {
				CI_LOG_E("video blit shader: " << ex.what());
			}
			mBlitFbo = ci::gl::Fbo::create(aSize.x, aSize.y, ci::gl::Fbo::Format());
			return true;
		}
		ciWMFVideoPlayer		mVideo;
#endif
		std::string				mName;
		std::string				mPath;
		ci::gl::FboRef			mBlitFbo;
		ci::gl::GlslProgRef		mBlitShader;
		bool					mLoop = false;
		bool					mReversed = false;
		bool					mScrubbing = false;
		float					mVolumeLevel = 1.0f;
		float					mOutputVolume = -1.0f;
		bool					mSpeedWarningLogged = false;
	};
}
