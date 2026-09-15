/*
	VDKeyboard
	
*/

#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"

// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Uniforms
#include "VDUniforms.h"
// Mediator
#include "VDMediator.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	class VDMediatorObservable;
	typedef std::shared_ptr<VDMediatorObservable> VDMediatorObservableRef;

	class VDKeyboard;
	// stores the pointer to the VDKeyboard instance
	typedef std::shared_ptr<VDKeyboard> VDKeyboardRef;
	class VDKeyboard {
	public:
		VDKeyboard(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
		static VDKeyboardRef	create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);

		void								setupKeyboard(VDMediatorObservableRef aVDMediator);
		bool								handleKeyDown(KeyEvent& event);
		bool								handleKeyUp(KeyEvent& event);
	private:
		// Settings
		VDSettingsRef						mVDSettings;
		// Animation
		VDAnimationRef						mVDAnimation;
		// Uniforms
		VDUniformsRef						mVDUniforms;
		//Mediator
		VDMediatorObservableRef				mVDMediator;

	};
}

