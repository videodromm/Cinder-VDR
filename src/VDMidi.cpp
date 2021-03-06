#include "VDMidi.h"

using namespace videodromm;

VDMidiRef VDMidi::create(VDUniformsRef aVDUniforms)
{
	return std::shared_ptr<VDMidi>(new VDMidi(aVDUniforms));
}

VDMidi::VDMidi( VDUniformsRef aVDUniforms) {
	
	mVDUniforms = aVDUniforms;
	CI_LOG_V("VDMidi constructor");
	mMidiMsg = "";
	// midi
	//if (mVDSettings->mMIDIOpenAllInputPorts) midiSetup();

}
VDMidi::~VDMidi(void) {
	mMidiIn0.closePort();
	mMidiIn1.closePort();
	mMidiIn2.closePort();
	mMidiOut0.closePort();
	mMidiOut1.closePort();
	mMidiOut2.closePort();
}
void VDMidi::setMidiMsg(const std::string& aMsg) {
	mMidiMsg = aMsg;
};
std::string VDMidi::getMidiMsg() {
	return mMidiMsg;
}
void VDMidi::setupMidi(VDMediatorObservableRef aVDMediator) {
	mVDMediator = aVDMediator;	
	midiSetup();
}

void VDMidi::saveMidiPorts() {
	JsonTree		json;
	JsonTree midiin = ci::JsonTree::makeArray("midiin");
	for (int j = 0; j < mMidiInputs.size(); j++)
	{
		midiin.addChild(ci::JsonTree("midiinname", mMidiInputs[j].portName));
	}
	json.addChild(midiin);
	JsonTree midiout = ci::JsonTree::makeArray("midiout");
	for (int j = 0; j < mMidiOutputs.size(); j++)
	{
		midiout.addChild(ci::JsonTree("midioutname", mMidiOutputs[j].portName));
	}
	json.addChild(midiout);
	string jsonFileName = "midi.json";
	fs::path jsonFile = getAssetPath("") / jsonFileName;
	json.write(jsonFile);
}
void VDMidi::midiSetup() {
	std::stringstream ss;
	ss << "setupMidi ";
	CI_LOG_V("midiSetup: " + ss.str());
	if (mMidiIn0.getNumPorts() > 0)
	{
		mMidiIn0.listPorts();
		for (int i = 0; i < mMidiIn0.getNumPorts(); i++)
		{
			bool alreadyListed = false;
			for (int j = 0; j < mMidiInputs.size(); j++)
			{
				if (mMidiInputs[j].portName == mMidiIn0.getPortName(i)) alreadyListed = true;
			}
			if (!alreadyListed) {
				midiInput mIn;
				mIn.portName = mMidiIn0.getPortName(i);
				mMidiInputs.push_back(mIn);
				/*if (mVDSettings->mMIDIOpenAllInputPorts) {
					openMidiInPort(i);
					mMidiInputs[i].isConnected = true;
					ss << "Opening MIDI in port " << i << " " << mMidiInputs[i].portName;
				}
				else {*/
					mMidiInputs[i].isConnected = false;
					ss << "Available MIDI in port " << i << " " << mMidiIn0.getPortName(i);
				//}
			}
		}
	}
	else {
		ss << "no midi in ports found";
	}

	// midi out
	//mMidiOut0.getPortList();
	if (mMidiOut0.getNumPorts() > 0) {
		for (int i = 0; i < mMidiOut0.getNumPorts(); i++)
		{
			bool alreadyListed = false;
			for (int j = 0; j < mMidiOutputs.size(); j++)
			{
				if (mMidiOutputs[j].portName == mMidiOut0.getPortName(i)) alreadyListed = true;
			}
			if (!alreadyListed) {
				midiOutput mOut;
				mOut.portName = mMidiOut0.getPortName(i);
				mMidiOutputs.push_back(mOut);

				mMidiOutputs[i].isConnected = false;
				ss << "Available MIDI output port " << i << " " << mMidiOut0.getPortName(i);

			}
		}
	}
	else {
		ss << "no midi out Ports found";
	}
	saveMidiPorts();
	midiControlType = "none";
	midiControl = midiPitch = midiVelocity = midiValue = midiChannel = 0;
	midiNormalizedValue = 0.0f;
	ss << std::endl;
	mMidiMsg = ss.str() + "\n";
	CI_LOG_V(ss.str());
}

void VDMidi::openMidiInPort(unsigned int i) {
	CI_LOG_V("openMidiInPort: " + toString(i));
	std::stringstream ss;
	if (i < mMidiIn0.getNumPorts()) {
		if (i == 0) {
			mMidiIn0.openPort(i);
			mMidiIn0.midiSignal.connect(std::bind(&VDMidi::midiListener, this, std::placeholders::_1));
		}
		if (i == 1) {
			mMidiIn1.openPort(i);
			mMidiIn1.midiSignal.connect(std::bind(&VDMidi::midiListener, this, std::placeholders::_1));
		}
		if (i == 2) {
			mMidiIn2.openPort(i);
			mMidiIn2.midiSignal.connect(std::bind(&VDMidi::midiListener, this, std::placeholders::_1));
		}
	}
	mMidiInputs[i].isConnected = true;
	ss << "Opening MIDI in port " << i << " " << mMidiInputs[i].portName << std::endl;

	mMidiMsg = ss.str() + "\n" ;

	CI_LOG_V(ss.str());
}
void VDMidi::closeMidiInPort(int i) {

	if (i == 0)
	{
		mMidiIn0.closePort();
	}
	if (i == 1)
	{
		mMidiIn1.closePort();
	}
	if (i == 2)
	{
		mMidiIn2.closePort();
	}
	mMidiInputs[i].isConnected = false;

}
void VDMidi::midiOutSendNoteOn(int i, int channel, int pitch, int velocity) {

	if (i == 0)
	{
		if (mMidiOutputs[i].isConnected) mMidiOut0.sendNoteOn(channel, pitch, velocity);
	}
	if (i == 1)
	{
		if (mMidiOutputs[i].isConnected) mMidiOut1.sendNoteOn(channel, pitch, velocity);
	}
	if (i == 2)
	{
		if (mMidiOutputs[i].isConnected) mMidiOut2.sendNoteOn(channel, pitch, velocity);
	}

}
void VDMidi::openMidiOutPort(int i) {

	std::stringstream ss;
	ss << "Port " << i;
	if (i < mMidiOutputs.size()) {
		if (i == 0) {
			if (mMidiOut0.openPort(i)) {
				mMidiOutputs[i].isConnected = true;
				ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
				mMidiOut0.sendNoteOn(1, 40, 64);
			}
			else {
				ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
			}
		}
		if (i == 1) {
			if (mMidiOut1.openPort(i)) {
				mMidiOutputs[i].isConnected = true;
				ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
				mMidiOut1.sendNoteOn(1, 40, 64);
			}
			else {
				ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
			}
		}
		if (i == 2) {
			if (mMidiOut2.openPort(i)) {
				mMidiOutputs[i].isConnected = true;
				ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
				mMidiOut2.sendNoteOn(1, 40, 64);
			}
			else {
				ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
			}
		}
	}
	ss << std::endl;
	mMidiMsg = ss.str() + "\n" ;
	CI_LOG_V(ss.str());
}
void VDMidi::closeMidiOutPort(int i) {

	if (i == 0)
	{
		mMidiOut0.closePort();
	}
	if (i == 1)
	{
		mMidiOut1.closePort();
	}
	if (i == 2)
	{
		mMidiOut2.closePort();
	}
	mMidiOutputs[i].isConnected = false;

}

void VDMidi::midiListener(midi::Message msg) {
	std::stringstream ss;
	ss << "MIDI port: " << mMidiIn0.getPortName(msg.port) << "\n";
	midiChannel = msg.channel;
	switch (msg.status)
	{
	case MIDI_CONTROL_CHANGE:
		midiControlType = "/cc";
		midiControl = msg.control;
		midiValue = msg.value;
		midiNormalizedValue = lmap<float>(midiValue, 0.0, 127.0, 0.0, 1.0);
		ss << " cc Chn:" << midiChannel << " CC:" << midiControl << " Val:" << midiValue << " NVal:" << midiNormalizedValue;
		CI_LOG_V("Midi: " + ss.str());
		if (midiWeights) {
			if (midiControl > 0 && midiControl < 9) {
				midiControl += 30;
			}
		}
		
		//if (midiControl > 20 && midiControl < 49) {
			/*if (midiControl > 20 && midiControl < 29) {
				mSelectedWarp = midiControl - 21;
			}
			if (midiControl > 40 && midiControl < 49) {
				mSelectedFboB = midiControl - 41;
				mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOB, mSelectedFboB);
			}
			*/
			//if (midiControl > 30 && midiControl < 39) {
			mVDMediator->setUniformValue(midiControl, midiNormalizedValue);
			//mSelectedFboA = midiControl - 31;
			//mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOA, mSelectedFboA);
		//}

		//}
		//else {
			//updateParams(midiControl, midiNormalizedValue);
		//}
		//mWebSockets->write("{\"params\" :[{" + controlType);
		break;
	case MIDI_NOTE_ON:
		/*
		TODO nano notes instead of cc
		if (midiControl > 20 && midiControl < 29) {
				mSelectedWarp = midiControl - 21;
			}
			if (midiControl > 30 && midiControl < 39) {
				mSelectedFboA = midiControl - 31;
				mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOA, mSelectedFboA);
			}
			if (midiControl > 40 && midiControl < 49) {
				mSelectedFboB = midiControl - 41;
				mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOB, mSelectedFboB);
			}
		*/
		midiControlType = "/on";
		//midiVelocity = msg.velocity;
		//midiNormalizedValue = lmap<float>(midiVelocity, 0.0, 127.0, 0.0, 1.0);
		//// quick hack!
		//mVDMediator->setUniformValue(14, 1.0f + midiNormalizedValue);
		midiPitch = msg.pitch;
		// midimix solo mode
		/*if (midiPitch == 27) midiSticky = true;
		if (midiSticky) {
			midiStickyPrevIndex = midiPitch;
			midiStickyPrevValue = mVDAnimation->getBoolUniformValueByIndex(midiPitch + 80);
		}*/
		//mVDMediator->setUniformValue(midiPitch + 80, true);

		// This does mVDSession->setFboFragmentShaderIndex(0, midiPitch);
		if (midiPitch == 8) {
			midiWeights = true;
		}
		
		/*if (midiPitch < 7) {
			mVDMediator->setMode(midiPitch);			
			//mSelectedFboA = midiPitch;
			//mFBOAChanged = true;
		}
		if (midiPitch > 8 && midiPitch < 17) {
			//mSelectedFboB = midiPitch - 8;
			//mFBOBChanged = true;
		}*/
		if (midiPitch > 17 && midiPitch < 24) {
			mVDMediator->setUniformValue(midiPitch + 80 - 17, true);
		}
		ss << " noteon Chn: " << midiChannel << " Pitch: " << midiPitch;
		CI_LOG_V("Midi: " + ss.str());
		break;
	case MIDI_NOTE_OFF:
		midiPitch = msg.pitch;
		if (midiPitch == 8) {
			midiWeights = false;
		}
		
		if (midiPitch < 7) {
			mVDMediator->setMode(7);

			//mSelectedFboA = midiPitch;
			//mFBOAChanged = true;
		}
		if (midiPitch > 17 && midiPitch < 24) {
			mVDMediator->setUniformValue(midiPitch + 80 - 17, false);
		}
		// midimix solo mode
		/*if (midiPitch == 27) {
			midiStickyPrevIndex = 0;
			midiSticky = false;
		}
		if (!midiSticky) {*/
		//mVDMediator->setUniformValue(midiPitch + 80, false);
	/*}
	else {
		if (midiPitch == midiStickyPrevIndex) {
			mVDMediator->setUniformValue(midiPitch + 80, !midiStickyPrevValue);
		}
	}*/
		ss << " noteoff Chn: " << midiChannel << " Pitch: " << midiPitch;
		CI_LOG_V("Midi: " + ss.str());
		midiControlType = "/off";
		/*midiPitch = msg.pitch;
		midiVelocity = msg.velocity;
		midiNormalizedValue = lmap<float>(midiVelocity, 0.0, 127.0, 0.0, 1.0);*/
		break;
	case MIDI_PITCH_BEND:
		midiControlType = "/pb";
		midiControl = msg.control;
		midiValue = msg.value;
		midiNormalizedValue = lmap<float>(midiValue, 0.0, 127.0, 0.0, 1.0);
		ss << " pb Chn: " << midiChannel << " CC: " << midiControl << " Val: " << midiValue << " NVal: " << midiNormalizedValue;
		mVDMediator->setUniformValue(mVDUniforms->IMOUSEX, midiValue);
		break;
	default:
		ss << " unknown status: " << msg.status;
		CI_LOG_V("Midi: " + ss.str());
		break;
	}
	//ss << "MIDI Chn: " << midiChannel << " type: " << midiControlType << " CC: " << midiControl << " Pitch: " << midiPitch << " Vel: " << midiVelocity << " Val: " << midiValue << " NVal: " << midiNormalizedValue << std::endl;
	//CI_LOG_V("Midi: " + ss.str());
	ss << std::endl;
	mMidiMsg = ss.str();
}


