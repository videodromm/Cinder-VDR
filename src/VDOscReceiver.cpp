#include "VDOscReceiver.h"

using namespace videodromm;

VDOscReceiverRef VDOscReceiver::create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
{
	return std::shared_ptr<VDOscReceiver>(new VDOscReceiver(aVDSettings, aVDAnimation, aVDUniforms));
}

VDOscReceiver::VDOscReceiver(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms) {
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mVDUniforms = aVDUniforms;
	CI_LOG_V("VDOscReceiver constructor");
	mOSCMsg = "";

}

void VDOscReceiver::setOSCMsg(const std::string& aMsg) {
	mOSCMsg = aMsg;
};
std::string VDOscReceiver::getOSCMsg() {
	return mOSCMsg;
}
void VDOscReceiver::setupOSCReceiver(VDMediatorObservableRef aVDMediator, int aOSCReceiverPort) {
	mVDMediator = aVDMediator;
	mOscReceiver = std::make_shared<osc::ReceiverUdp>(aOSCReceiverPort);

	mOscReceiver->setListener("/*",
		[&](const osc::Message& msg) {
			// touchosc
			bool found = false;
			std::string ctrl = "";
			int index = -1;
			int i = 0;
			float f = 1.0f;
			std::stringstream ss;
			vec2 vv = vec2(0.0f);
			std::string addr = msg.getAddress();
			// handle all msg without page integer first
			// midi cc in osc
			ss << addr;
			ctrl = "/cc";
			index = addr.find(ctrl); // 20210110 TODO check if npos or -1 to compare
			if (index != std::string::npos)
			{
				// from midi2osc (Cinder) or hydra standalone
				found = true;
				i = msg[0].int32();// TODO check was flt() from hydra standalone if ok can merge with midi;
				f = msg[1].flt();// was for hydra / 128;
				//mVDAnimation->setUniformValue(i, f);
				//mVDSession->setUniformValue(aCtrl, aValue);
				mVDMediator->setUniformValue(i, f);
				//ss << " midi from OSC " << i << " value " << f;
			}
			if (!found)
			{
				// accxyz
				ctrl = "accxyz";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
				}
			}
			if (!found)
			{
				// ableton link tempo from ofx abletonLinkToWebsocket
				ctrl = "/live/tempo";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					//mVDAnimation->setAutoBeatAnimation(false);
					mVDMediator->setUniformValue(mVDUniforms->IBPM, msg[0].flt());
				}
			}
			if (!found)
			{
				// int32 0 or 1 play from Transthor
				ctrl = "/play";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
				}
			}
			if (!found)
			{
				// float ticks from Transthor
				ctrl = "/ticks";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					f = msg[0].flt();
					mVDAnimation->useTimeWithTempo();
					mVDMediator->setUniformValue(mVDUniforms->ITIME, f);
					//stringstream ss;
					//ss << " " << f;
					//CI_LOG_I("OSC: " << ctrl << " addr: " << addr);

					//mVDAnimation->setUniformValue(mVDUniforms->IELAPSED, msg[0].flt());
				}
			}
			if (!found)
			{
				//	float tempo from Transthor
				//	SOS 160.0
				//	HumanET	150
				//	Hoover	135
				//	Massactiv whatelse 138.77
				//
				ctrl = "/tempo";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					mVDAnimation->useTimeWithTempo();
					f = msg[0].flt();
					mVDMediator->setUniformValue(mVDUniforms->IBPM, f);
					//ss << " " << f;
					//CI_LOG_I("tempo:" + toString(mVDAnimation->getBpm()));
				}
			}
			if (!found)
			{
				//32 floats from Speckthor
				ctrl = "/Spectrum";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					mVDMediator->setUniformValue(mVDUniforms->IMAXVOLUME, 0.0f);
					for (int a{0}; a < msg.getNumArgs(); a++) {
						// get the argument type 'f'
						if (msg.getArgType(i) == ArgType::FLOAT) {
							f = msg[a].flt() * 200.0f * mVDUniforms->getUniformValue(mVDUniforms->IAUDIOX);
							if (f > mVDUniforms->getUniformValue(mVDUniforms->IMAXVOLUME))
							{
								mVDMediator->setUniformValue(mVDUniforms->IMAXVOLUME, f);
							}
							mVDAnimation->iFreqs[a] = f;
							if (a == mVDAnimation->getFreqIndex(0)) mVDMediator->setUniformValue(mVDUniforms->IFREQ0, f);
							if (a == mVDAnimation->getFreqIndex(1)) mVDMediator->setUniformValue(mVDUniforms->IFREQ1, f);
							if (a == mVDAnimation->getFreqIndex(2)) mVDMediator->setUniformValue(mVDUniforms->IFREQ2, f);
							if (a == mVDAnimation->getFreqIndex(3)) mVDMediator->setUniformValue(mVDUniforms->IFREQ3, f);
						}
					}
				}
			}
			if (!found)
			{
				// from Midithor (i=0 on noteoff) Midi1 to Midi8
				ctrl = "/Midi";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					// get the argument type
					if (msg.getArgType(0) == ArgType::INTEGER_32) {
						int i = msg[0].int32();
						if (i > 80 && i < 109) {
							// 20210101 was bool mVDUniforms->setBoolUniformValueByIndex(i, !mVDUniforms->getBoolUniformValueByIndex(i));
							mVDMediator->setUniformValue(i, !mVDUniforms->getUniformValue(i));

						}
						// sos specific
						if (i == 119) { // B7 end
							mVDMediator->setUniformValue(mVDUniforms->ITIMEFACTOR, 0.02f);
						}
						if (i == 120) { // C8 slow
							mVDMediator->setUniformValue(mVDUniforms->ITIMEFACTOR, 0.1f);
						}
						if (i == 121) { // C#8
							mVDMediator->setUniformValue(mVDUniforms->ITIMEFACTOR, 0.18f);
						}
						if (i == 122) { // D8
							mVDMediator->setUniformValue(mVDUniforms->ITIMEFACTOR, 0.25f);
						}
						if (i == 123) { // D#8
							mVDMediator->setUniformValue(mVDUniforms->ITIMEFACTOR, 0.35f);
						}
						if (i == 124) { // E8 fast
							mVDMediator->setUniformValue(mVDUniforms->ITIMEFACTOR, 1.0f);
						}
					}
				}
			}
			
			if (!found)
			{
				// int32 1 to 4 beat from Transthor
				ctrl = "/beat";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{				
					found = true;
					mVDMediator->setUniformValue(mVDUniforms->IBEAT, msg[0].int32() - 1);
					mVDMediator->setUniformValue(
						mVDUniforms->IBARBEAT,
						mVDUniforms->getUniformValue(mVDUniforms->IBAR) * 4 + mVDUniforms->getUniformValue(mVDUniforms->IBEAT));
				}
			}
			if (!found)
			{
				// int32 bar from Transthor
				ctrl = "/bar";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					float previousBar = mVDUniforms->getUniformValue(mVDUniforms->IBAR); // 20210101 was int
					
					float newBar = (float)msg[0].int32();
					mVDSettings->mErrorMsg = "0bar: " + toString(msg[0].int32() - 1) + " - " + toString(newBar);
					mVDSettings->mMidiMsg = "1bar: " + toString(previousBar) + " - " + toString(mVDUniforms->getUniformValue(mVDUniforms->IBAR));
					// TODO test if useless:
					if (previousBar != newBar) {
						mVDSettings->iBarDuration = mVDUniforms->getUniformValue(mVDUniforms->ITIME) - mBarStart;
						mBarStart = mVDUniforms->getUniformValue(mVDUniforms->ITIME);
					}
					// TODO END
					mVDMediator->setUniformValue(mVDUniforms->IBAR, newBar);
					mVDMediator->setUniformValue(
						mVDUniforms->IBARBEAT,
						mVDUniforms->getUniformValue(mVDUniforms->IBAR) * 4 + mVDUniforms->getUniformValue(mVDUniforms->IBEAT));

				}
			}

			if (!found)
			{
				ctrl = "link";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					double d0 = msg[0].dbl(); // tempo
					mVDMediator->setUniformValue(mVDUniforms->IBPM, d0);
					double d1 = msg[1].dbl();
					int d2 = msg[2].int32();
					//! 20200526 mVDSocketio->changeIntValue(mVDUniforms->IBEAT, d2);
				}
			}
			if (!found)
			{
				// touchosc
				int page = 0;
				try {
					// CHECK exception if not integer
					page = std::stoi(addr.substr(1, 1)); // 1 to 4

					int lastSlashIndex = addr.find_last_of("/"); // 0 2
					// if not a page selection
					if (addr.length() > 2) {
						ctrl = "multifader1";
						index = addr.find(ctrl);
						if (index != std::string::npos)
						{
							found = true;
							f = msg[0].flt();
							i = std::stoi(addr.substr(lastSlashIndex + 1)) + 8;
							mVDMediator->setUniformValue(i, f);
						}

						if (!found)
						{
							ctrl = "multifader2";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(lastSlashIndex + 1)) + 32; // 24 + 8
								mVDMediator->setUniformValue(i, f);
							}
						}

						if (!found)
						{
							ctrl = "multifader";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(lastSlashIndex + 1)) + 56; // 48 + 8
								mVDMediator->setUniformValue(i, f);
							}
						}
						if (!found)
						{
							ctrl = "fader";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length()));
								mVDMediator->setUniformValue(i, f);// starts at 1: mVDUniforms->IFR G B
							}
						}
						if (!found)
						{
							ctrl = "rotary";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length())) + 10;
								mVDMediator->setUniformValue(i, f);
							}
						}
						if (!found)
						{
							// toggle
							ctrl = "multitoggle";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								// /2/multitoggle/5/3
							}
						}
						if (!found)
						{
							// toggle
							ctrl = "toggle";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length())) + 80;
								mVDAnimation->toggleValue(i);
							}
						}
						if (!found)
						{
							// push
							ctrl = "push";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length())) + 80;
								mVDAnimation->toggleValue(i);
							}
						}
						if (!found)
						{
							// xy
							ctrl = "xy1";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								vv = vec2(msg[0].flt(), msg[1].flt());
							}
						}
						if (!found)
						{
							// xy
							ctrl = "xy2";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								vv = vec2(msg[0].flt(), msg[1].flt());
							}
						}
						if (!found)
						{
							// xy
							ctrl = "xy";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								vv = vec2(msg[0].flt(), msg[1].flt());
							}
						}
					}
				}
				catch (const std::exception& e) {
					ss << addr << " not integer";
					mVDSettings->mErrorMsg += "\n" + ss.str();
					CI_LOG_E("not integer: " << addr);
				}
			}
			if (!found) {
				CI_LOG_E("not handled: " << msg.getNumArgs() << " addr: " << addr);
				//mVDSettings->mOSCMsg += "\nnot handled: " + addr;
				mVDSettings->mErrorMsg = "osc not handled: " + addr + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
			}
			if (addr != "/play") {
				ss << " f:" << f << " i:" << i;
				mOSCMsg = ss.str();
			}

		});

	try {
		// Bind the receiver to the endpoint. This function may throw.
		mOscReceiver->bind();
	}
	catch (const osc::Exception& ex) {
		CI_LOG_E("Error binding: " << ex.what() << " val: " << ex.value());
	}

	// UDP opens the socket and "listens" accepting any message from any endpoint. The listen
	// function takes an error handler for the underlying socket. Any errors that would
	// call this function are because of problems with the socket or with the remote message.
	mOscReceiver->listen(
		[](asio::error_code error, protocol::endpoint endpoint) -> bool {
			if (error) {
				CI_LOG_E("Error Listening: " << error.message() << " val: " << error.value() << " endpoint: " << endpoint);
				return false;
			}
			else
				
				return true;
		});
}



