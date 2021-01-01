/*
	VDAnimation
	Handles all uniform variables for shaders: index, name, type, bounds and their animation.
*/
// TODO put audio in a separate class
// TODO put timing in separate class?
// TODO implement lazy loading for audio

#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
//!  audio
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
//!  json
//#include "cinder/Json.h"
//!  Settings
#include "VDSettings.h"
//!  Uniforms
#include "VDUniforms.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	// stores the pointer to the VDAnimation instance
	typedef std::shared_ptr<class VDAnimation> VDAnimationRef;

	class VDAnimation {
	public:
		VDAnimation(VDSettingsRef aVDSettings, VDUniformsRef aVDUniforms);

		static VDAnimationRef			create(VDSettingsRef aVDSettings, VDUniformsRef aVDUniforms)
		{
			return std::shared_ptr<VDAnimation>(new VDAnimation(aVDSettings, aVDUniforms));
		}
		void							update();
		void							save();
		std::string							getAssetsPath() {
			return mVDSettings->mAssetsPath;
		}

		const int						mBlendModes = 28;
		void							blendRenderEnable(bool render) { mBlendRender = render; };
		// tap tempo
		bool							mUseTimeWithTempo;
		void							toggleUseTimeWithTempo() { mUseTimeWithTempo = !mUseTimeWithTempo; };
		void							useTimeWithTempo() { mUseTimeWithTempo = true; };
		bool							getUseTimeWithTempo() { return mUseTimeWithTempo; };
		float							iTempoTimeBeatPerBar;
		
		void							tapTempo();
		int								getEndFrame() { return mEndFrame; };
		void							setEndFrame(int frame) { mEndFrame = frame; };

		// animation
		int								currentScene;
		//int							getBadTV(int frame);
		// keyboard
		bool							handleKeyDown(KeyEvent& event);
		bool							handleKeyUp(KeyEvent& event);
		// audio
		ci::gl::TextureRef				getAudioTexture();
		std::string						getAudioTextureName() { return mAudioName; };
		//float							maxVolume;
		bool							mLineInInitialized;
		bool							mWaveInitialized;
		audio::InputDeviceNodeRef		mLineIn;
		audio::MonitorSpectralNodeRef	mMonitorLineInSpectralNode;
		audio::MonitorSpectralNodeRef	mMonitorWaveSpectralNode;
		audio::SamplePlayerNodeRef		mSamplePlayerNode;
		audio::SourceFileRef			mSourceFile;
		audio::MonitorSpectralNodeRef	mScopeLineInFmt;
		audio::BufferPlayerNodeRef		mBufferPlayerNode;

		std::vector<float>				mMagSpectrum;

		// number of frequency bands of our spectrum
		static const int				mFFTWindowSize = 32;
		float							iFreqs[mFFTWindowSize];
		int								mPosition;
		std::string						mAudioName;
		void							preventLineInCrash(); // at next launch
		void							saveLineIn();
		bool							getUseAudio() {
			return mVDSettings->mUseAudio;
		};
		bool							getUseLineIn() {
			return mVDSettings->mUseLineIn;
		};
		void							setUseLineIn(bool useLineIn) {
			mVDSettings->mUseLineIn = useLineIn;
		};
		void							toggleUseLineIn() { mVDSettings->mUseLineIn = !mVDSettings->mUseLineIn; };

		// audio
		bool							isAudioBuffered() { return mAudioBuffered; };
		void							toggleAudioBuffered() { mAudioBuffered = !mAudioBuffered; };

		// shaders
		int								getUniformTypeByName(const std::string& aName) {
			return mVDUniforms->getUniformTypeByName(aName);
		}
		bool							isExistingUniform(const std::string& aName) { 
			return mVDUniforms->isExistingUniform(aName); 
		};
		
		/*
		string							getUniformNameForIndex(int aIndex) {
			return shaderUniforms[aIndex].name; //controlIndexes[aIndex];
		};*/
		int								getUniformIndexForName(const std::string& aName) {
			return mVDUniforms->getUniformIndexForName(aName);
			//return shaderUniforms[stringToIndex(aName)].index;
		};
		bool							toggleValue(unsigned int aIndex);

		void							setAnim(unsigned int aCtrl, unsigned int aAnim) {
			mVDUniforms->setAnim(aCtrl, aAnim);
		}
		/*bool							setUniformValue(unsigned int aIndex, float aValue) {
			if (aIndex == mVDUniforms->IBPM) {
				if (aValue > 0.0f) {
					mVDUniforms->setUniformValue(mVDUniforms->IDELTATIME, 60 / aValue);
				}
			}
			return mVDUniforms->setUniformValue(aIndex, aValue);
		}*/
/*
		bool							setBoolUniformValueByIndex(unsigned int aIndex, bool aValue) {
			return mVDUniforms->setBoolUniformValueByIndex(aIndex, aValue);
		}
		void							setIntUniformValueByName(const std::string& aName, int aValue) {
			mVDUniforms->setIntUniformValueByName(aName, aValue);
		};
		void							setIntUniformValueByIndex(unsigned int aIndex, int aValue) {
			mVDUniforms->setIntUniformValueByIndex(aIndex, aValue);
		}*/
		/* 20201229
		void							setFloatUniformValueByName(const std::string& aName, float aValue) {
			mVDUniforms->setFloatUniformValueByName(aName, aValue);
		}
		void setVec2UniformValueByName(const std::string& aName, vec2 aValue) {
			mVDUniforms->setVec2UniformValueByName(aName, aValue);
		}
		void setVec2UniformValueByIndex(unsigned int aIndex, vec2 aValue) {
			mVDUniforms->setVec2UniformValueByIndex(aIndex, aValue);
		}
		void setVec3UniformValueByName(const std::string& aName, vec3 aValue) {
			mVDUniforms->setVec3UniformValueByName(aName, aValue);
		}
		void setVec3UniformValueByIndex(unsigned int aIndex, vec3 aValue) {
			mVDUniforms->setVec3UniformValueByIndex(aIndex, aValue);
		}
		void setVec4UniformValueByName(const std::string& aName, vec4 aValue) {
			mVDUniforms->setVec4UniformValueByName(aName, aValue);
		}
		void setVec4UniformValueByIndex(unsigned int aIndex, vec4 aValue) {
			mVDUniforms->setVec4UniformValueByIndex(aIndex, aValue);
		}
		bool							getBoolUniformValueByIndex(unsigned int aIndex) {
			return mVDUniforms->getBoolUniformValueByIndex(aIndex);
		}
		float							getMinUniformValue(unsigned int aIndex) {
			return mVDUniforms->getMinUniformValue(aIndex);
		}
		float							getMaxUniformValue(unsigned int aIndex) {
			return mVDUniforms->getMaxUniformValue(aIndex);
		}
		float							getMinUniformValueByName(const std::string& aName) {
			return mVDUniforms->getMinUniformValueByName(aName);
		}
		float							getMaxUniformValueByName(const std::string& aName) {
			return mVDUniforms->getMaxUniformValueByName(aName);
		}
		bool							getBoolUniformValueByName(const std::string& aName) {
			return mVDUniforms->getBoolUniformValueByName(aName);
		}
		float							getUniformValue(unsigned int aIndex) {
			return mVDUniforms->getUniformValue(aIndex);
		}
		std::string						getUniformName(unsigned int aIndex) {
			return mVDUniforms->getUniformName(aIndex);
		}
		int								getUniformAnim(unsigned int aIndex) {
			return mVDUniforms->getUniformAnim(aIndex);
		}
		float							getFloatUniformDefaultValueByIndex(unsigned int aIndex) {
			return mVDUniforms->getFloatUniformDefaultValueByIndex(aIndex);
		}
		int								getIntUniformValueByIndex(unsigned int aIndex) {
			return mVDUniforms->getIntUniformValueByIndex(aIndex);
		}
		int								getSampler2DUniformValueByName(const std::string& aName) {
			return mVDUniforms->getSampler2DUniformValueByName(aName);
		}
		float							getUniformValueByName(const std::string& aName) {
				return mVDUniforms->getUniformValueByName(aName);		
		}
		
		vec2							getVec2UniformValueByName(const std::string& aName) {
			return mVDUniforms->getVec2UniformValueByName(aName);
		}
		vec3							getVec3UniformValueByName(const std::string& aName) {
			return mVDUniforms->getVec3UniformValueByName(aName);
		}
		vec4							getVec4UniformValueByName(const std::string& aName) {
			return mVDUniforms->getVec4UniformValueByName(aName);
		}
		int								getIntUniformValueByName(const std::string& aName) {
			return mVDUniforms->getIntUniformValueByName(aName);
		};

		// mix fbo
		bool							isFlipH() { return getBoolUniformValueByIndex(mVDUniforms->IFLIPH); };
		bool							isFlipV() { return getBoolUniformValueByIndex(mVDUniforms->IFLIPV); };
		void							flipH() { setBoolUniformValueByIndex(mVDUniforms->IFLIPH, !getBoolUniformValueByIndex(mVDUniforms->IFLIPH)); };
		void							flipV() { setBoolUniformValueByIndex(mVDUniforms->IFLIPV, !getBoolUniformValueByIndex(mVDUniforms->IFLIPV)); };
		*/

		unsigned int					getBlendModesCount() { return mBlendModes; };
		bool							renderBlend() { return mBlendRender; };

		// timed animation
		int								mEndFrame;
		int								getFreqIndexSize() { return freqIndexes.size(); };
		int								getFreqIndex(unsigned int aFreqIndex) { return freqIndexes[aFreqIndex]; };
		void							setFreqIndex(unsigned int aFreqIndex, unsigned int aFreq) { freqIndexes[aFreqIndex] = aFreq; };
		//float							getFreq(unsigned int aFreqIndex) { return iFreqs[freqIndexes[aFreqIndex]]; };
		// public for hydra
		//void							createFloatUniform(const string& aName, int aCtrlIndex, float aValue = 0.01f, float aMin = 0.0f, float aMax = 1.0f);
		//void							createSampler2DUniform(const string& aName, int aCtrlIndex, int aTextureIndex = 0);
	private:
		// Settings
		VDSettingsRef					mVDSettings;
		VDUniformsRef					mVDUniforms;
		std::map<int, int>				freqIndexes;
		bool							mAudioBuffered;
		ci::gl::TextureRef				mAudioTexture;
		gl::Texture2d::Format			mAudioFormat;
		unsigned char					dTexture[256];// MUST be < mWindowSize

		// shaders
		//! write a uniforms json file
		void							saveUniforms();
		ci::JsonTree					uniformToJson(int i);

		// time
		ci::Timer						mTimer;
		std::deque <double>				buffer;
		void							calculateTempo();
		int								counter;
		double							averageTime;
		double							currentTime;
		double							startTime;
		float							previousTime;
		float							previousTimeBeatPerBar;
		JsonTree						mData;
		void							loadAnimation();
		void							saveAnimation();
		float							mLastBar = 0.0f; // 20210101 was int
		std::unordered_map<int, float>	mBadTV;
		bool							mBlendRender;
		
	};
}
