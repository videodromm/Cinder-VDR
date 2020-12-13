/*
	VDRouter
	Handles routing of messages from the network and midi.
	Input: SocketIO, osc, midi
	Output: SocketIO
*/
// TODO separate SocketIO, osc, midi into different classes
// TODO implement lazy loading for SocketIO, osc, midi
// TODO choose output : OSC or SocketIO?
// TODO socketio https://github.com/kusharami/socket.io-client-cpp
#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/Json.h"

// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
//!  Uniforms
#include "VDUniforms.h"
// Osc
//#include "VDOscReceiver.h"


using namespace ci;
using namespace ci::app;

using namespace videodromm;


namespace videodromm
{
	// stores the pointer to the VDRouter instance
	typedef std::shared_ptr<class VDRouter> VDRouterRef;

	class VDRouter {
	public:
		VDRouter(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformRef aVDUniform);

		// messages
		void						updateParams(int iarg0, float farg1);

		void						changeFloatValue(unsigned int aControl, float aValue, bool forceSend = false, bool toggle = false, bool increase = false, bool decrease = false);

		void						sendJSON(std::string params);
		int							selectedWarp() { return mSelectedWarp; };
		int							selectedFboA() { return mSelectedFboA; };
		int							selectedFboB() { return mSelectedFboB; };
		bool						hasFBOAChanged() { if (mFBOAChanged) { mFBOAChanged = false; return true; } else { return false; } };
		bool						hasFBOBChanged() { if (mFBOBChanged) { mFBOBChanged = false; return true; } else { return false; } };
		void						setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) { 
			mSelectedFboA = aWarpFboIndex; 
		}
		void						setWarpBFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) { mSelectedFboB = aWarpFboIndex; }

	private:
		// Settings
		VDSettingsRef				mVDSettings;
		// Animation
		VDAnimationRef				mVDAnimation;
		VDUniformRef					mVDUniforms;
		// lights4events
		void						colorWrite();
		bool						mFBOAChanged;
		bool						mFBOBChanged;
	
		int							mSelectedWarp;
		int							mSelectedFboA;
		int							mSelectedFboB;

		static const int			MAX = 16;


	};
}

