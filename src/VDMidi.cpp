#include "VDMidi.h"

using namespace videodromm;

VDMidiRef VDMidi::create(VDUniformsRef aVDUniforms)
{
	return std::shared_ptr<VDMidi>(new VDMidi(aVDUniforms));
}

VDMidi::VDMidi(VDUniformsRef aVDUniforms) {

	mVDUniforms = aVDUniforms;
	CI_LOG_V("VDMidi constructor");
	mMidiMsg = "";
	// midi
	//if (mVDSettings->mMIDIOpenAllInputPorts) midiSetup();

}
VDMidi::~VDMidi(void) {
	mMidiIn0.closePort();
	for (auto& port : mMidiInPorts) { if (port) port->closePort(); }
	mMidiOut0.closePort();
	for (auto& port : mMidiOutPorts) { if (port) port->closePort(); }
}
void VDMidi::setMidiMsg(const std::string& aMsg) {
	mMidiMsg = aMsg;
};
std::string VDMidi::getMidiMsg() {
	return mMidiMsg;
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
void VDMidi::setupMidi(VDMediatorObservableRef aVDMediator) {
	mVDMediator = aVDMediator;
	loadMidiLearnMapIfNeeded();
	midiSetup();
}
void VDMidi::saveMidiLearnMap() {
	JsonTree json;
	JsonTree mappings = ci::JsonTree::makeArray("mappings");
	for (auto& kv : mMidiLearnMap) {
		JsonTree entry = ci::JsonTree::makeObject();
		entry.addChild(ci::JsonTree("cc", kv.first));
		entry.addChild(ci::JsonTree("uniform", kv.second));
		mappings.pushBack(entry);
	}
	json.addChild(mappings);
	fs::path jsonFile = getAssetPath("") / "midilearn.json";
	json.write(jsonFile);
}
void VDMidi::loadMidiLearnMapIfNeeded() {
	if (mMidiLearnMapLoaded) return;
	mMidiLearnMapLoaded = true;
	fs::path jsonFile = getAssetPath("") / "midilearn.json";
	if (!fs::exists(jsonFile)) return;
	try {
		JsonTree json(loadFile(jsonFile));
		if (json.hasChild("mappings")) {
			for (auto& entry : json.getChild("mappings").getChildren()) {
				if (entry.hasChild("cc") && entry.hasChild("uniform")) {
					mMidiLearnMap[entry.getValueForKey<int>("cc")] = entry.getValueForKey<int>("uniform");
				}
			}
		}
	}
	catch (const std::exception& ex) {
		CI_LOG_E("loadMidiLearnMapIfNeeded error: " << ex.what());
	}
}
bool VDMidi::getMidiLearnMappingAt(int aIndex, int& aCc, int& aUniform) {
	if (aIndex < 0 || (size_t)aIndex >= mMidiLearnMap.size()) return false;
	int i = 0;
	for (auto& kv : mMidiLearnMap) {
		if (i == aIndex) {
			aCc = kv.first;
			aUniform = kv.second;
			return true;
		}
		i++;
	}
	return false;
}
void VDMidi::removeMidiLearnMapping(int aCc) {
	mMidiLearnMap.erase(aCc);
	saveMidiLearnMap();
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
				std::size_t nameIndex = mMidiInputs[i].portName.find(mVDMediator->getPreferredMidiInputDevice());
				if (nameIndex != std::string::npos) {
					// openMidiInPort() already sets isConnected based on whether the open actually
					// succeeded - don't stomp that with an unconditional true right after
					openMidiInPort(i);
					ss << "Opening MIDI in port " << i << " " << mMidiInputs[i].portName;
				}
				else {
					mMidiInputs[i].isConnected = false;
					ss << "Available MIDI in port " << i << " " << mMidiIn0.getPortName(i);
				}
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
	// most of RtMidi's own openPort()/midiInStart() failures (device already claimed by another
	// application, a driver error) don't throw - see RtMidiErrorLogCallback - so hadOpenError()
	// (set by that callback) is the only reliable way to know the open actually worked; this used
	// to mark isConnected = true unconditionally, which claimed success even when the underlying
	// Windows MM port never opened and would therefore never deliver a single MIDI message
	bool opened = false;
	if (i < mMidiIn0.getNumPorts()) {
		// a real Input, opened directly at index i, created the first time this specific port is
		// connected - see mMidiInPorts' declaration for why this replaced 3 fixed named members
		// (mMidiIn0..2, capped at 3 simultaneous ports and silently unable to open port 3+ at all)
		if (mMidiInPorts.size() <= i) mMidiInPorts.resize(i + 1);
		if (!mMidiInPorts[i]) mMidiInPorts[i] = std::make_unique<midi::Input>();
		mMidiInPorts[i]->openPort(i);
		opened = !mMidiInPorts[i]->hadOpenError();
		if (opened) mMidiInPorts[i]->midiSignal.connect(std::bind(&VDMidi::midiListener, this, std::placeholders::_1));
	}
	mMidiInputs[i].isConnected = opened;
	ss << (opened ? "Opened" : "Failed to open") << " MIDI in port " << i << " " << mMidiInputs[i].portName << std::endl;

	mMidiMsg = ss.str() + "\n";

	CI_LOG_V(ss.str());
}
void VDMidi::closeMidiInPort(int i) {

	if (i >= 0 && (size_t)i < mMidiInPorts.size() && mMidiInPorts[i]) {
		mMidiInPorts[i]->closePort();
	}
	if (i >= 0 && (size_t)i < mMidiInputs.size()) mMidiInputs[i].isConnected = false;

}
void VDMidi::midiOutSendNoteOn(int i, int channel, int pitch, int velocity) {

	if (i >= 0 && (size_t)i < mMidiOutputs.size() && mMidiOutputs[i].isConnected
		&& (size_t)i < mMidiOutPorts.size() && mMidiOutPorts[i]) {
		mMidiOutPorts[i]->sendNoteOn(channel, pitch, velocity);
	}

}
void VDMidi::openMidiOutPort(int i) {

	std::stringstream ss;
	ss << "Port " << i;
	if (i >= 0 && (size_t)i < mMidiOutputs.size()) {
		// same reasoning as openMidiInPort() above - a real MidiOut per index, created on demand,
		// instead of 3 fixed named members that silently couldn't open port 3+ at all
		if (mMidiOutPorts.size() <= (size_t)i) mMidiOutPorts.resize(i + 1);
		if (!mMidiOutPorts[i]) mMidiOutPorts[i] = std::make_unique<midi::MidiOut>();
		if (mMidiOutPorts[i]->openPort(i)) {
			mMidiOutputs[i].isConnected = true;
			ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
			mMidiOutPorts[i]->sendNoteOn(1, 40, 64);
		}
		else {
			mMidiOutputs[i].isConnected = false;
			ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
		}
	}
	ss << std::endl;
	mMidiMsg = ss.str() + "\n";
	CI_LOG_V(ss.str());
}
void VDMidi::closeMidiOutPort(int i) {

	if (i >= 0 && (size_t)i < mMidiOutPorts.size() && mMidiOutPorts[i]) {
		mMidiOutPorts[i]->closePort();
	}
	if (i >= 0 && (size_t)i < mMidiOutputs.size()) mMidiOutputs[i].isConnected = false;

}

void VDMidi::midiListener(midi::Message msg) {
	// unconditional, not just the MIDI_CONTROL_CHANGE case below - previously a Note On/Off (or
	// any other status) produced no log line at all, which could look identical to "no message
	// is ever received" even when messages were arriving and being processed correctly
	CI_LOG_V("midiListener: port=" << msg.port << " status=" << (int)msg.status << " channel=" << msg.channel);
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
		ss << " U:" << mVDUniforms->getUniformName(midiControl) << " cc Chn:" << midiChannel << " CC:" << midiControl << " Val:" << midiValue << " NVal:" << midiNormalizedValue;
		CI_LOG_V("Midi: " + ss.str());
		if (mMidiLearnMode && mMidiLearnTargetUniform >= 0) {
			// bind this CC to the armed uniform and stop - don't also apply it as a live value
			// change on the same message, and don't fall through to the nanoKONTROL2-specific
			// remap/blendmode hacks below, which only make sense for the implicit convention
			mMidiLearnMap[midiControl] = mMidiLearnTargetUniform;
			saveMidiLearnMap();
			CI_LOG_I("Midi learn: bound CC " << midiControl << " to uniform " << mMidiLearnTargetUniform << " (" << mVDUniforms->getUniformName(mMidiLearnTargetUniform) << ")");
			mMidiLearnTargetUniform = -1;
			break;
		}
		{
			// an explicitly learned mapping always wins and is applied directly, bypassing the
			// nanoKONTROL2-specific remap/blendmode special-casing below (that only exists for
			// the older, implicit "CC number == uniform index" convention)
			auto learned = mMidiLearnMap.find(midiControl);
			if (learned != mMidiLearnMap.end()) {
				mVDMediator->setUniformValue(learned->second, midiNormalizedValue);
				break;
			}
		}
		if (midiWeights) {
			if (midiControl > 0 && midiControl < 9) {
				midiControl += 30;
			}
		}
		if (midiNormalizedValue > 0.1) {
			if (midiControl == 60) {
				// set (reset blendmode)
				mVDMediator->setUniformValue(mVDUniforms->IBLENDMODE, 0.0f);
			}
			if (midiControl == 61) {
				// right arrow
				if (mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE) == 0) {
					mVDMediator->setUniformValue(mVDUniforms->IBLENDMODE, mVDUniforms->getMaxUniformValue(mVDUniforms->IBLENDMODE));
				}
				else {
					mVDMediator->setUniformValue(mVDUniforms->IBLENDMODE, math<float>::max(mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE) - 1.0f, 0.0f));
				}
			}
			if (midiControl == 62) {
				// left arrow
				if (mVDUniforms->getMaxUniformValue(mVDUniforms->IBLENDMODE) == mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE)) {
					mVDMediator->setUniformValue(mVDUniforms->IBLENDMODE, 0.0f);
				}
				else {
					mVDMediator->setUniformValue(mVDUniforms->IBLENDMODE, math<float>::max(mVDUniforms->getUniformValue(mVDUniforms->IBLENDMODE) + 1.0f, 0.0f));
				}

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
		if (midiPitch < 7) {
			mVDUniforms->setUniformValue(mVDUniforms->IWEIGHT0 + midiPitch, 1.0);
			//mVDMediator->setMode(midiPitch);			
			//mSelectedFboA = midiPitch;
			//mFBOAChanged = true;
		}
		/*
		if (midiPitch > 8 && midiPitch < 17) {
			//mSelectedFboB = midiPitch - 8;
			//mFBOBChanged = true;
		}*/
		// R row top 0 to 7
		// R row middle 8 to 15
		if (midiPitch > 7 && midiPitch < 16) {
			int uniformIndex = midiPitch + 243;
			// ICOLUMN1 = 201
			if (mVDUniforms->getUniformValue(uniformIndex) == 0.0f) {
				mVDMediator->setUniformValue(uniformIndex, 1.0f);
			}
			else {
				ss << " ICOLUMN1: " << mVDUniforms->getUniformValue(uniformIndex);
				CI_LOG_E("Midi: " + ss.str());
			}
		}
		// R row bottom 16 to 23
		if (midiPitch > 15 && midiPitch < 24) {
			mVDMediator->setUniformValue(midiPitch + 65, true);
		}
		// 7 for midiWeights
		if (midiPitch == 7) {
			midiWeights = false;
		}
		ss << " noteon Chn: " << midiChannel << " Pitch: " << midiPitch;
		CI_LOG_V("Midi: " + ss.str());
		break;
	case MIDI_NOTE_OFF:
		midiPitch = msg.pitch;
		// R row 16 to 22
		if (midiPitch > 15 && midiPitch < 24) {
			mVDMediator->setUniformValue(midiPitch + 65, false);
		}
		// 7 for midiWeights
		if (midiPitch == 7) {
			midiWeights = true;
		}
		if (midiPitch < 7) {
			//mVDMediator->setMode(7);
			mVDUniforms->setUniformValue(mVDUniforms->IWEIGHT0 + midiPitch, 0.0);
			//mSelectedFboA = midiPitch;
			//mFBOAChanged = true;
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


