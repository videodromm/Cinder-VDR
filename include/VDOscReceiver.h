/*
	VDOscReceiver
	
*/

#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/Json.h"

// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Mediator
#include "VDMediator.h"
// Session
//#include "VDSessionFacade.h"
// OSC
#include "cinder/osc/Osc.h"

using namespace ci;
using namespace ci::app;
using namespace ci::osc;
using namespace asio;
using namespace asio::ip;
#define USE_UDP 1

#if USE_UDP
using Receiver = osc::ReceiverUdp;
using protocol = asio::ip::udp;
#else
using Receiver = osc::ReceiverTcp;
using protocol = asio::ip::tcp;
#endif
namespace videodromm
{
	class VDMediatorObservable;
	typedef std::shared_ptr<VDMediatorObservable> VDMediatorObservableRef;

	class VDOscReceiver;
	// stores the pointer to the VDOscReceiver instance
	typedef std::shared_ptr<VDOscReceiver> VDOscReceiverRef;
	class VDOscReceiver {
	public:
		VDOscReceiver(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation);
		static VDOscReceiverRef	create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation);

		void								setupOSCReceiver(VDMediatorObservableRef aVDMediator, int aOSCReceiverPort);
		void								setOSCMsg(const std::string& aMsg);
		std::string							getOSCMsg();
	private:
		// Settings
		VDSettingsRef						mVDSettings;
		// Animation
		VDAnimationRef						mVDAnimation;
		//Mediator
		VDMediatorObservableRef				mVDMediator;
		float								mBarStart = 0.0f;
		// osc
		std::shared_ptr<osc::ReceiverUdp>	mOscReceiver;

		std::string							mOSCMsg;

	};
}

