/*
	VDMidi
*/

#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"

// Uniforms
#include "VDUniforms.h"
// Mediator
#include "VDMediator.h"
// Midi
#include "MidiIn.h"
// bindings shared with TSWebsocketServer (assets/actions, assets/hardware)
#include "VDMidiActions.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	class VDMediatorObservable;
	typedef std::shared_ptr<VDMediatorObservable> VDMediatorObservableRef;

	class VDMidi;
	// stores the pointer to the VDMidi instance
	typedef std::shared_ptr<VDMidi> VDMidiRef;
	struct midiInput
	{
		std::string			portName;
		bool			isConnected;
	};
	struct midiOutput
	{
		std::string			portName;
		bool			isConnected;
	};
	class VDMidi {
	public:
		VDMidi(VDUniformsRef aVDUniforms);
		static VDMidiRef			create(VDUniformsRef aVDUniforms);
		void						setupMidi(VDMediatorObservableRef aVDMediator);
		void						setMidiMsg(const std::string& aMsg);
		std::string					getMidiMsg();
		// MIDI
		void						saveMidiPorts();
		int							getMidiInPortsCount() { return mMidiInputs.size(); };
		std::string					getMidiInPortName(unsigned int i) { return (i < mMidiInputs.size()) ? mMidiInputs[i].portName : "No midi in ports"; };
		bool						isMidiInConnected(unsigned int i) { return (i < mMidiInputs.size()) ? mMidiInputs[i].isConnected : false; };
		unsigned int				getMidiOutPortsCount() { return (unsigned int)mMidiOutputs.size(); };
		std::string					getMidiOutPortName(unsigned int i) { return (i < mMidiOutputs.size()) ? mMidiOutputs[i].portName : "No midi out ports"; };
		bool						isMidiOutConnected(unsigned int i) { return (i < mMidiOutputs.size()) ? mMidiOutputs[i].isConnected : false; };
		void						midiOutSendNoteOn(int i, int channel, int pitch, int velocity);

		void						openMidiInPort(unsigned int i);
		void						closeMidiInPort(int i);
		void						openMidiOutPort(int i);
		void						closeMidiOutPort(int i);
		void						setMediator(VDMediatorObservableRef aVDMediator) { mVDMediator = aVDMediator; }
		// midi learn, shared with TSWebsocketServer: while learn mode is on and a uniform is armed,
		// the next note / CC / poly aftertouch binds to it, as an action in assets/actions/*.json
		// (TSWebsocketServer's ids and action strings, see VDMidiActions.h). Learn mode stays on so
		// several controls can be learned one after another.
		// - standalone: from this app's own MIDI ports; Cinder runs the actions and writes the file
		// - connected to TSWebsocketServer: it handles the MIDI (own ports ignored here, no double
		//   triggers); learning uses its "midi" websocket events and sends the binding to it
		//   ("set_action" command), which saves the file
		void						setMidiLearnMode(bool aEnabled) { mMidiLearnMode = aEnabled; mActions.ensureLoaded(); }
		bool						isMidiLearnMode() { return mMidiLearnMode; }
		void						armMidiLearn(int aUniformIndex) { mMidiLearnTargetUniform = aUniformIndex; }
		int							getMidiLearnTarget() { return mMidiLearnTargetUniform; }
		// a "midi" event relayed by TSWebsocketServer (connected mode): learning only
		void						onRemoteMidi(const std::string& aId, int aChannel);
		// "cc_14_30_VSN1, ..." bound to this uniform, "" if none
		std::string					getMidiBindingLabel(int aUniform) { mActions.ensureLoaded(); return mActions.idsForUniform(aUniform); }
		std::vector<VDMidiActions::Binding>	getMidiUniformBindings() { mActions.ensureLoaded(); return mActions.uniformBindings(); }
		void						removeMidiBinding(const std::string& aId);
		void						reloadMidiBindings() { mActions.reload(); }
		std::string					getMidiLearnStatus() { return mMidiLearnStatus; }
		// legacy midilearn.json (CC number -> uniform, any device): still applied, no longer written
		int							getMidiLearnMappingsCount() { return (int)mMidiLearnMap.size(); }
		// aIndex-th learned mapping, ordered by CC number (std::map<int,int> is already sorted by
		// key) - lets the UI list what's bound without exposing the map type itself
		bool						getMidiLearnMappingAt(int aIndex, int& aCc, int& aUniform);
		void						removeMidiLearnMapping(int aCc);
		void						clearMidiLearnMap() { mMidiLearnMap.clear(); saveMidiLearnMap(); }
		~VDMidi(void);
	private:
		// Uniforms
		VDUniformsRef				mVDUniforms;
		//Mediator
		VDMediatorObservableRef		mVDMediator;

		// MIDI
		std::vector<midiInput>		mMidiInputs;
		// mMidiIn0 is used only for enumeration (midiSetup()'s listPorts()/getPortName() calls) -
		// actually opening a port for real (any index, including 0) goes through mMidiInPorts
		// below instead. This used to be 4 fixed named members (mMidiIn0..mMidiIn3, "couldn't make
		// a vector" per the comment that was here) capping simultaneous connections at 4 - worse,
		// openMidiInPort()/closeMidiInPort() only ever handled indices 0-2, so port 3 (the fourth
		// declared-but-dead member) and anything beyond it (a 5-6 port audio interface/controller
		// setup is common) silently could never be opened at all. midi::Input isn't safely
		// copyable (a raw, non-refcounted RtMidiIn* member) so a plain vector<Input> can't be
		// resized directly - vector<unique_ptr<Input>> sidesteps that (move-only, no copies) and
		// scales to however many ports actually exist, one real Input created lazily per port
		// index the first time it's opened.
		midi::Input					mMidiIn0;
		std::vector<std::unique_ptr<midi::Input>>	mMidiInPorts;
		void						midiSetup();
		void						midiListener(midi::Message msg);
		std::string					mMidiMsg;
		// midi output - same reasoning as mMidiInPorts above, same previous 3-index limitation
		midi::MidiOut				mMidiOut0;
		std::vector<std::unique_ptr<midi::MidiOut>>	mMidiOutPorts;
		std::vector<midiOutput>		mMidiOutputs;
		std::string					midiControlType;
		int							midiControl;
		int							midiPitch;
		int							midiVelocity;
		float						midiNormalizedValue;
		int							midiValue;
		int							midiChannel;
		// midimix solo mode
		bool						midiSticky;
		bool						midiStickyPrevValue;
		int							midiStickyPrevIndex;
		// toggles
		bool						midiWeights = true; // nano mix weights

		// midi learn - see the public methods above
		bool						mMidiLearnMode = false;
		int							mMidiLearnTargetUniform = -1;
		bool						mMidiLearnMapLoaded = false;
		std::map<int, int>			mMidiLearnMap; // MIDI CC number -> uniform index
		void						saveMidiLearnMap();
		void						loadMidiLearnMapIfNeeded();
		// shared bindings (see setMidiLearnMode)
		VDMidiActions				mActions;
		std::string					mMidiLearnStatus;
		bool						isConnectedToServer();
		// binds the armed uniform to a message id ("on" also binds the matching "of", momentary)
		void						learn(const std::string& aId, int aChannel);
		void						persistBinding(const VDMidiActions::Binding& aBinding);
		// runs a uniform action (wsp/won/wof) with a 0..1 MIDI value; false if not a uniform action
		bool						runAction(const std::string& aAction, float aValue);

	};
}

