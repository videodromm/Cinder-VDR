/*
	VDSessionFacade

*/
// TODO 

#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
// Logger
#ifdef _DEBUG
#include "VDLog.h"
#endif 

// Settings
#include "VDSettings.h"
//
#include "VDSession.h"
#include "VDAnimation.h"
#include "VDMediator.h"
#include "VDOscObserver.h"
#include "VDUniforms.h"
#include "VDUIObserver.h"
// VDRouterBuilder
//#include "VDRouterBuilder.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	typedef std::shared_ptr<class VDSession> VDSessionRef;
	typedef std::shared_ptr<class VDSessionFacade> VDSessionFacadeRef;

	class VDSessionFacade : public std::enable_shared_from_this<VDSessionFacade> {
	public:
		static VDSessionFacadeRef createVDSession(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, VDMixRef aVDMix);

		VDSessionFacadeRef		setUniformValue(unsigned int aCtrl, float aValue);
		VDSessionFacadeRef		addUIObserver(VDSettingsRef aVDSettings, VDUniformsRef aVDUniforms);
		VDSessionFacadeRef		getWindowsResolution();
		VDSessionFacadeRef		setupSession();
		VDSessionFacadeRef		setupOSCReceiver();
		VDSessionFacadeRef		setupMidi();
		VDSessionFacadeRef		setupWSClient();
		VDSessionFacadeRef		setupHttpClient();
		VDSessionFacadeRef		loadShaderFromHttp(const std::string& url, unsigned int aFboIndex);
		VDSessionFacadeRef		listFolders();
		VDSessionFacadeRef		listShaders(const std::string& aFolder, const std::string& aExtension);
		VDSessionFacadeRef		loadShaderFromFolder(const std::string& aFolder, const std::string& aExtension, const std::string& aName);
		std::vector<std::string> getFolderList() { return mVDSession->getFolderList(); };
		std::vector<std::string> getShaderList() { return mVDSession->getShaderList(); };
		VDSessionFacadeRef		setupKeyboard();
		VDSessionFacadeRef		addOSCObserver(const std::string& host, unsigned int port);
		VDSessionFacadeRef		addSocketIOObserver(const std::string& host, unsigned int port);
		VDSessionFacadeRef		setAnim(unsigned int aCtrl, unsigned int aAnim);
		VDSessionFacadeRef		toggleValue(unsigned int aCtrl);
		VDSessionFacadeRef		tapTempo();
		VDSessionFacadeRef		toggleUseTimeWithTempo();
		VDSessionFacadeRef		useTimeWithTempo();
		VDSessionFacadeRef		toggleUseLineIn();
		VDSessionFacadeRef		loadFromJsonFile(const fs::path& jsonFile);
		VDSessionFacadeRef		update();
		// begin terminal operations
		unsigned int			getAnim(unsigned int aCtrl);
		bool					getUseTimeWithTempo();
		// OSC
		bool					isOscSenderConnected();
		bool					isOscReceiverConnected();
		int						getOSCReceiverPort();
		void					setOSCReceiverPort(int aReceiverPort);
		void					setOSCMsg(const std::string& aMsg);
		std::string				getOSCMsg();
		// midi
		void					midiOutSendNoteOn(int i, int channel, int pitch, int velocity) { mVDMediator->midiOutSendNoteOn(i, channel, pitch, velocity); };
		int						getMidiInPortsCount() { return mVDMediator->getMidiInPortsCount(); };
		string					getMidiInPortName(int i) { return mVDMediator->getMidiInPortName(i); };
		bool					isMidiInConnected(int i) { return mVDMediator->isMidiInConnected(i); };
		void					openMidiInPort(int i) { mVDMediator->openMidiInPort(i); };
		void					closeMidiInPort(int i) { mVDMediator->closeMidiInPort(i); };
		int						getMidiOutPortsCount() { return mVDMediator->getMidiOutPortsCount(); };
		string					getMidiOutPortName(int i) { return mVDMediator->getMidiOutPortName(i); };
		bool					isMidiOutConnected(int i) { return mVDMediator->isMidiOutConnected(i); };
		void					openMidiOutPort(int i) { mVDMediator->openMidiOutPort(i); };
		void					closeMidiOutPort(int i) { mVDMediator->closeMidiOutPort(i); };
		std::string				getMidiMsg() { return mVDMediator->getMidiMsg(); }
		bool					isMidiSetup();
		// midi learn
		void					setMidiLearnMode(bool aEnabled) { mVDMediator->setMidiLearnMode(aEnabled); }
		bool					isMidiLearnMode() { return mVDMediator->isMidiLearnMode(); }
		void					armMidiLearn(int aUniformIndex) { mVDMediator->armMidiLearn(aUniformIndex); }
		int						getMidiLearnTarget() { return mVDMediator->getMidiLearnTarget(); }
		int						getMidiLearnMappingsCount() { return mVDMediator->getMidiLearnMappingsCount(); }
		bool					getMidiLearnMappingAt(int aIndex, int& aCc, int& aUniform) { return mVDMediator->getMidiLearnMappingAt(aIndex, aCc, aUniform); }
		void					removeMidiLearnMapping(int aCc) { mVDMediator->removeMidiLearnMapping(aCc); }
		void					clearMidiLearnMap() { mVDMediator->clearMidiLearnMap(); }
		// shared MIDI bindings (assets/actions, see VDMidiActions.h)
		std::string				getMidiBindingLabel(int aUniform) { return mVDMediator->getMidiBindingLabel(aUniform); }
		std::vector<VDMidiActions::Binding>	getMidiUniformBindings() { return mVDMediator->getMidiUniformBindings(); }
		void					removeMidiBinding(const std::string& aId) { mVDMediator->removeMidiBinding(aId); }
		void					reloadMidiBindings() { mVDMediator->reloadMidiBindings(); }
		std::string				getMidiLearnStatus() { return mVDMediator->getMidiLearnStatus(); }
		// websockets
		bool					isWSClientConnected();
		int						getWSClientPort();
		VDSessionFacadeRef		wsConnect();
		void					wsPing();
		void					setWSClientPort(int aPort);
		void					setWSMsg(const std::string& aMsg);
		std::string				getWSMsg();
		// live code view: the WebApp editor's text rendered into a transparent texture (for Spout)
		VDCodeViewRef			getCodeView();
		ci::gl::Texture2dRef	buildCodeViewTexture(const ci::ivec2& aSize);

		ci::gl::TextureRef		buildFboTexture(unsigned int aIndex);
		ci::gl::TextureRef		buildFboRenderedTexture(unsigned int aFboIndex);
		ci::gl::TextureRef		buildRenderedMixetteTexture(unsigned int aIndex);
		ci::gl::TextureRef		getFboShaderTexture(unsigned int aIndex);
		std::string				getFboShaderName(unsigned int aIndex);
		unsigned int			getFboShaderListSize(); 
		bool					isFboValid(unsigned int aFboIndex) {
			return mVDSession->isFboValid(aFboIndex);
		};
		std::string				getFboMsg(unsigned int aFboIndex) {
			return mVDSession->getFboMsg(aFboIndex);
		};
		std::string				getFboError(unsigned int aFboIndex) {
			return mVDSession->getFboError(aFboIndex);
		};	
		std::string				getFboStatus(unsigned int aFboIndex) {
			return mVDSession->getFboStatus(aFboIndex);
		};
		unsigned int			getFboMs(unsigned int aTexIndex = 0) {
			return mVDSession->getFboMs(aTexIndex);
		};
		bool					isValidInputTexture(unsigned int aTexIndex) {
			return mVDSession->isValidInputTexture(aTexIndex);
		};
		unsigned int			getFboMsTotal(unsigned int aFboIndex) {
			return mVDSession->getFboMsTotal(aFboIndex);
		};
		
		std::vector<ci::gl::GlslProg::Uniform> getFboShaderUniforms(unsigned int aFboShaderIndex);
		//float					getFboShaderUniformValue();
		float					getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex);
		void					setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue);
		ci::gl::TextureRef		buildPostFboTexture();
		ci::gl::TextureRef		buildFxFboTexture();
		ci::gl::TextureRef		buildWarpFboTexture();
		ci::gl::TextureRef		buildRenderedWarpFboTexture();
		unsigned int			getWarpAFboIndex(unsigned int aWarpIndex);
		void					setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) { mVDSession->setWarpAFboIndex(aWarpIndex, aWarpFboIndex); }
		unsigned int			getWarpBFboIndex(unsigned int aWarpIndex);
		float					getMinUniformValue(unsigned int aIndex);
		float					getMaxUniformValue(unsigned int aIndex);
		float					getDefaultUniformValue(unsigned int aIndex);
		int						getFboTextureWidth(unsigned int aFboIndex);
		int						getFboTextureHeight(unsigned int aFboIndex);
		unsigned int			getWarpCount();
		void					createWarp();
		void					removeWarp(unsigned int aWarpIndex) { mVDSession->removeWarp(aWarpIndex); }
		ci::gl::TextureRef		getWarpPreviewTexture(unsigned int aWarpIndex) { return mVDSession->getWarpPreviewTexture(aWarpIndex); }
		void					drawWarpsToCurrentTarget(const ci::gl::TextureRef& aComposite) { mVDSession->drawWarpsToCurrentTarget(aComposite); }
		void					saveWarps() {
			mVDSession->saveWarps();
		};
		std::string				getWarpName(unsigned int aWarpIndex);// or trycatch
		int						getWarpWidth(unsigned int aWarpIndex);
		int						getWarpHeight(unsigned int aWarpIndex);
		std::string				getFboInputTextureName(unsigned int aFboIndex = 0);
		ci::gl::Texture2dRef	getFboInputTexture(unsigned int aTexIndex = 0);
		void					setFboTextureAudioMode(unsigned int aFboIndex);
		void					saveThumbnail(unsigned int aFboIndex = 0);
		void					setSelectedFbo(unsigned int aFboIndex = 0);
		unsigned int			getSelectedFbo();
		void					setWebAppUrl(const std::string& aUrl);
		std::string				getWebAppUrl();
		//unsigned int			getFboInputTextureIndex(unsigned int aFboIndex = 0);

		std::string				getFboName(unsigned int aFboIndex);
		std::string				getTrackName() { return mTrackName; };
		// audio
		ci::gl::TextureRef		getAudioTexture() { return mVDSession->getAudioTexture(); };
		bool					getUseAudio() { return mVDSession->getUseAudio(); };
		bool					getUseLineIn() { return mVDSession->getUseLineIn(); };
		void					setUseLineIn(bool useLineIn = true) { mVDSession->setUseLineIn(useLineIn); };
		bool					refreshAudioDevices() { return mVDSession->refreshAudioDevices(); };
		std::vector<std::string> getAudioInputDeviceNames() { return mVDSession->getAudioInputDeviceNames(); };
		std::vector<std::string> getAudioOutputDeviceNames() { return mVDSession->getAudioOutputDeviceNames(); };
		std::string				getPreferredAudioInputDevice() { return mVDSession->getPreferredAudioInputDevice(); };
		// per-PC default devices (assets/audio.json, keyed by machine id - see VDMachine.h)
		std::string				getDefaultAudioInputDevice() { return mVDSession->getDefaultAudioInputDevice(); }
		std::string				getDefaultAudioOutputDevice() { return mVDSession->getDefaultAudioOutputDevice(); }
		void					setDefaultAudioInputDevice(const std::string& aName) { mVDSession->setDefaultAudioInputDevice(aName); }
		void					setDefaultAudioOutputDevice(const std::string& aName) { mVDSession->setDefaultAudioOutputDevice(aName); }
		std::string				getMachineName() { return mVDSession->getMachineName(); }
		std::string				getMachineId() { return mVDSession->getMachineId(); }
		std::string				getPreferredAudioOutputDevice() { return mVDSession->getPreferredAudioOutputDevice(); };
		// selects the preferred device by name and persists it to sessionPath immediately, so it's
		// still selected on the next launch even if the app isn't closed cleanly.
		void					selectAudioInputDevice(const std::string& aName);
		void					selectAudioOutputDevice(const std::string& aName);

		bool					isAudioBuffered() { return mVDSession->isAudioBuffered(); };
		void					toggleAudioBuffered() { mVDSession->toggleAudioBuffered(); };
		bool					getUseWaveMonitor() { return mVDSession->getUseWaveMonitor(); };
		void					toggleUseWaveMonitor() { mVDSession->toggleUseWaveMonitor(); };
		bool					getUseRandom() { return mVDSession->getUseRandom(); };
		void					toggleUseRandom() { mVDSession->toggleUseRandom(); };
		void					setFboInputTexture(unsigned int aFboIndex, unsigned int aTexIndex) {
			mVDSession->setFboInputTexture(aFboIndex, aTexIndex);
		}
		unsigned int			getFboInputTextureIndex(unsigned int aFboIndex) {
			return mVDSession->getFboInputTextureIndex(aFboIndex);
		}
		unsigned int			getInputTexturesCount(unsigned int aFboIndex = 0) {
			return mVDSession->getInputTexturesCount(aFboIndex);
		}
		std::string				getInputTextureName(unsigned int aFboIndex, unsigned int aTexIndex = 0) {
			return mVDSession->getInputTextureName(aFboIndex, aTexIndex);
		}
		// playback controls (sequence/movie)
		bool					isSequence(unsigned int aFboIndex) { return mVDSession->isSequence(aFboIndex); }
		bool					isMovie(unsigned int aFboIndex) { return mVDSession->isMovie(aFboIndex); }
		// a video in the texture pool, by its pool name (nullptr if that entry isn't a video)
		VDVideoSourceRef		getVideoSource(const std::string& aName) { return mVDSession->getVideoSource(aName); }
		// starting it pauses every other playing video/audio file
		void					togglePlayPauseSource(const std::string& aName) { mVDSession->togglePlayPauseSource(aName); }
		void					setScrubbing(unsigned int aFboIndex, bool aScrubbing) { mVDSession->setScrubbing(aFboIndex, aScrubbing); }
		float					getVolumeLevel(unsigned int aFboIndex) { return mVDSession->getVolumeLevel(aFboIndex); }
		void					setVolumeLevel(unsigned int aFboIndex, float aLevel) { mVDSession->setVolumeLevel(aFboIndex, aLevel); }
		void					togglePlayPause(unsigned int aFboIndex) { mVDSession->togglePlayPause(aFboIndex); }
		bool					isAudioFile(unsigned int aFboIndex) { return mVDSession->isAudioFile(aFboIndex); }
		float					getAudioBpm(unsigned int aFboIndex) { return mVDSession->getAudioBpm(aFboIndex); }
		double					getAudioFilePosition() { return mVDSession->getAudioFilePosition(); }
		double					getAudioFileDuration() { return mVDSession->getAudioFileDuration(); }
		void					seekAudioFile(double aSeconds) { mVDSession->seekAudioFile(aSeconds); }
		bool					isAudioFileClock() { return mVDSession->isAudioFileClock(); }
		bool					isPlaying(unsigned int aFboIndex) { return mVDSession->isPlaying(aFboIndex); }
		bool					isLooping(unsigned int aFboIndex) { return mVDSession->isLooping(aFboIndex); }
		void					toggleLoop(unsigned int aFboIndex) { mVDSession->toggleLoop(aFboIndex); }
		void					syncToBeat(unsigned int aFboIndex) { mVDSession->syncToBeat(aFboIndex); }
		void					reverse(unsigned int aFboIndex) { mVDSession->reverse(aFboIndex); }
		bool					isLoadingFromDisk(unsigned int aFboIndex) { return mVDSession->isLoadingFromDisk(aFboIndex); }
		void					toggleLoadingFromDisk(unsigned int aFboIndex) { mVDSession->toggleLoadingFromDisk(aFboIndex); }
		float					getSpeed(unsigned int aFboIndex) { return mVDSession->getSpeed(aFboIndex); }
		void					setSpeed(unsigned int aFboIndex, float aSpeed) { mVDSession->setSpeed(aFboIndex, aSpeed); }
		int						getPosition(unsigned int aFboIndex) { return mVDSession->getPosition(aFboIndex); }
		void					setPlayheadPosition(unsigned int aFboIndex, int aPosition) { mVDSession->setPlayheadPosition(aFboIndex, aPosition); }
		int						getMaxFrame(unsigned int aFboIndex) { return mVDSession->getMaxFrame(aFboIndex); }
		void					setVideoVolume(unsigned int aFboIndex, float aVolume) { mVDSession->setVideoVolume(aFboIndex, aVolume); }
		float					getVideoVolume(unsigned int aFboIndex) { return mVDSession->getVideoVolume(aFboIndex); }
		// drag-and-drop, consumed by VDUIFbos.cpp - see VDSession.h for the full explanation
		bool					hasPendingTextureDrop() const { return mVDSession->hasPendingTextureDrop(); }
		void					setExternalDrag(bool aActive, const ci::vec2& aPos) { mVDSession->setExternalDrag(aActive, aPos); }
		bool					isExternalDragActive() const { return mVDSession->isExternalDragActive(); }
		ci::vec2				getExternalDragPos() const { return mVDSession->getExternalDragPos(); }
		void					registerPoolTexture(const std::string& aName, ci::gl::Texture2dRef aTexture) { mVDSession->registerPoolTexture(aName, aTexture); }
		void					unregisterPoolTexture(const std::string& aName) { mVDSession->unregisterPoolTexture(aName); }
		std::string				getAudioTextureName() { return mVDSession->getAudioTextureName(); }
		std::string				getAudioSourceLabel() { return mVDSession->getAudioSourceLabel(); }
		ci::vec2				getPendingTextureDropPos() const { return mVDSession->getPendingTextureDropPos(); }
		bool					consumePendingTextureDrop(unsigned int aFboIndex) { return mVDSession->consumePendingTextureDrop(aFboIndex); }
		void					flushPendingTextureDrop() { mVDSession->flushPendingTextureDrop(); }
		void					registerFboActiveTextureInGlobalPool(unsigned int aFboIndex) { mVDSession->registerFboActiveTextureInGlobalPool(aFboIndex); }
		int						getInputTextureMode(unsigned int aFboIndex) { return mVDSession->getInputTextureMode(aFboIndex); }
		unsigned int			getLoadedTextureCount() { return mVDSession->getLoadedTextureCount(); }
		ci::gl::Texture2dRef	getLoadedTexture(unsigned int aIndex) { return mVDSession->getLoadedTexture(aIndex); }
		std::string				getLoadedTextureName(unsigned int aIndex) { return mVDSession->getLoadedTextureName(aIndex); }
		int						getFFTWindowSize();
		float*					getFreqs();
		float					getFreq(unsigned int aFreqIndex) { return mVDSession->getFreq(aFreqIndex); };
		int						getFreqIndex(unsigned int aFreqIndex) { return mVDSession->getFreqIndex(aFreqIndex); };
		void					setFreqIndex(unsigned int aFreqIndex, unsigned int aFreq) { mVDSession->setFreqIndex(aFreqIndex, aFreq); };

		bool					showUI();
		VDSessionFacadeRef		toggleUI();
		int						getErrorCode() { return mVDMediator->getErrorCode(); }

		std::vector<ci::gl::GlslProg::Uniform> getUniforms(unsigned int aFboIndex = 0);
		ci::gl::Texture2dRef	buildFboInputTexture(unsigned int aFboIndex = 0);
		ci::gl::Texture2dRef	getFboInputTextureListItem(unsigned int aFboIndex = 0, unsigned int aTexIndex = 0);
		void					setFboInputTexture(unsigned int aFboIndex, ci::gl::Texture2dRef aTextureRef, const std::string& aName = "");
		void					resetAnim() { mVDSession->resetAnim(); }
		std::string				getModeName(unsigned int aMode);
		unsigned int			getModesCount();
		int						getUniformIndexForName(const std::string& aName);
		float					getUniformValue(unsigned int aCtrl);
		std::string				getUniformName(unsigned int aIndex);
		// end terminal operations 
		// begin events
		bool					handleMouseMove(MouseEvent event);
		bool					handleMouseDown(MouseEvent event);
		bool					handleMouseDrag(MouseEvent event);
		bool					handleMouseUp(MouseEvent event);
		void					fileDrop(FileDropEvent event);
		bool					handleKeyDown(KeyEvent& event);
		bool					handleKeyUp(KeyEvent& event);

		// end events
		VDSessionRef			getInstance() const;
		bool					loadFolder(const string& aFolder) {
			mTrackName = aFolder;
			return mVDSession->loadFolder(aFolder);
		}

	private:
		VDSessionFacade(VDSessionRef session, VDMediatorObservableRef mediator) : mVDSession(session), mVDMediator(mediator), mOscSenderConnected(false), mIsMidiSetup(false), mOscReceiverConnected(false) { }
		VDSessionRef						mVDSession;
		VDMediatorObservableRef				mVDMediator;
		bool								mOscSenderConnected = false;
		bool								mOscReceiverConnected = false;

		bool								mIsMidiSetup = false;
		std::string							mTrackName = "";
		// session
		fs::path							sessionPath;
		const std::string					sessionFileName = "session.json";
		void								save();
		void								restore();
	};


}
