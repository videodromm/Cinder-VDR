/*
	VDUIObserver
	Mediator UI Observer
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
// Settings
#include "VDSettings.h"
// Uniforms
#include "VDUniforms.h"
// Mediator
#include "VDMediator.h"

#include <memory>
#include <vector>

using namespace ci;
using namespace ci::app;

namespace videodromm {
	
	class VDUIObserver : public VDUniformObserver {
	public:
		static VDUniformObserverRef connect(VDSettingsRef aVDSettings, VDUniformsRef aVDUniforms) {
			VDUIObserver* o = new VDUIObserver(aVDSettings, aVDUniforms);
			
			VDUniformObserverRef obs(o);

			return obs;
		}
		VDUniformObserverRef setUniformValue(int aIndex, float aValue) {
			mVDUniforms->setUniformValue(aIndex, aValue);
			if (aIndex == 40) {
				int selected = int(mVDUniforms->getUniformValue(25));
				mVDUniforms->setUniformValue(int(mVDUniforms->getUniformValue(25)), aValue);
			}
			return shared_from_this();
		}
		
		
		bool mIsConnected = false;
		~VDUIObserver() {  };
	private:
		VDUIObserver(VDSettingsRef aVDSettings, VDUniformsRef aVDUniforms) {
			CI_LOG_V("VDUIObserver ctor");
			mVDSettings = aVDSettings;
			mVDUniforms = aVDUniforms;
		}
		// Settings
		VDSettingsRef	mVDSettings;
		// Uniforms
		VDUniformsRef	mVDUniforms;
	};
}
