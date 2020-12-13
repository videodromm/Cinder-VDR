/*
	VDUIObserver
	Mediator UI Observer
*/
// TODO 

#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Mediator
#include "VDMediator.h"

#include <memory>
#include <vector>

using namespace ci;
using namespace ci::app;

namespace videodromm {
	
	class VDUIObserver : public VDUniformObserver {
	public:
		static VDUniformObserverRef connect(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation) {
			VDUIObserver* o = new VDUIObserver(aVDSettings, aVDAnimation);
			
			VDUniformObserverRef obs(o);

			return obs;
		}
		VDUniformObserverRef setUniformValue(int aIndex, float aValue) {
			mVDAnimation->setUniformValue(aIndex, aValue);
			return shared_from_this();
		}
		
		
		bool mIsConnected = false;
		~VDUIObserver() {  };
	private:
		VDUIObserver(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation) {
			CI_LOG_V("VDUIObserver ctor");
			mVDSettings = aVDSettings;
			mVDAnimation = aVDAnimation;
		}
		// Settings
		VDSettingsRef	mVDSettings;
		// Animation
		VDAnimationRef	mVDAnimation;
	};
}
