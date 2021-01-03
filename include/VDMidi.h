/*
	VDMidiB
	
*/

#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/Json.h"

// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Uniforms
#include "VDUniforms.h"
// Mediator
#include "VDMediator.h"
// Midi
#include "MidiIn.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	class VDMediatorObservable;
	typedef std::shared_ptr<VDMediatorObservable> VDMediatorObservableRef;

	class VDMidi;
	// stores the pointer to the VDMidi instance
	typedef std::shared_ptr<VDMidi> VDMidiRef;
	struct midiInput
	{
		string			portName;
		bool			isConnected;
	};
	struct midiOutput
	{
		string			portName;
		bool			isConnected;
	};
	class VDMidi {
	public:
		VDMidi(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
		static VDMidiRef	create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);

		void								setupMidi(VDMediatorObservableRef aVDMediator);
		void								setMidiMsg(const std::string& aMsg);
		std::string							getMidiMsg();
		// MIDI
		void						midiSetup();
		void						saveMidiPorts();
		int							getMidiInPortsCount() { return mMidiInputs.size(); };
		string						getMidiInPortName(unsigned int i) { return (i < mMidiInputs.size()) ? mMidiInputs[i].portName : "No midi in ports"; };
		bool						isMidiInConnected(unsigned int i) { return (i < mMidiInputs.size()) ? mMidiInputs[i].isConnected : false; };
		int							getMidiOutPortsCount() { return mMidiOutputs.size(); };
		string						getMidiOutPortName(unsigned int i) { return (i < mMidiOutputs.size()) ? mMidiOutputs[i].portName : "No midi out ports"; };
		bool						isMidiOutConnected(unsigned int i) { return (i < mMidiOutputs.size()) ? mMidiOutputs[i].isConnected : false; };
		void						midiOutSendNoteOn(int i, int channel, int pitch, int velocity);

		void						openMidiInPort(int i);
		void						closeMidiInPort(int i);
		void						openMidiOutPort(int i);
		void						closeMidiOutPort(int i);
		~VDMidi(void);
	private:
		// Settings
		VDSettingsRef						mVDSettings;
		// Animation
		VDAnimationRef						mVDAnimation;
		// Uniforms
		VDUniformsRef						mVDUniforms;
		//Mediator
		VDMediatorObservableRef				mVDMediator;

		std::string							mMidiMsg;
		// MIDI
		vector<midiInput>			mMidiInputs;
		// midi inputs: couldn't make a vector
		midi::Input					mMidiIn0;
		midi::Input					mMidiIn1;
		midi::Input					mMidiIn2;
		midi::Input					mMidiIn3;
		void						midiListener(midi::Message msg);
		// midi output
		midi::MidiOut				mMidiOut0;
		midi::MidiOut				mMidiOut1;
		midi::MidiOut				mMidiOut2;
		midi::MidiOut				mMidiOut3;
		vector<midiOutput>			mMidiOutputs;
		string						midiControlType;
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
	};
}

