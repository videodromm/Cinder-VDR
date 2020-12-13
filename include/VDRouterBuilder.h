
/*
	VDRouterBuilder

*/
// TODO 

#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Utilities.h"
#include "cinder/Timeline.h"

#include "Resources.h"
// Logger
#include "VDLog.h"
// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
//
#include "VDRouter.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	typedef std::shared_ptr<class VDRouter> VDRouterRef;
	typedef std::shared_ptr<class VDRouterBuilder> VDRouterBuilderRef;

	class VDRouterBuilder : public std::enable_shared_from_this<VDRouterBuilder> {
	public:
		static VDRouterBuilderRef createVDRouter(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation)
		{
			//VDRouterRef router = VDRouterBuilder::createVDRouter(aVDSettings, aVDAnimation)->setWarpBFboIndex(1, 1)->setWarpAFboIndex(1, 2)->getInstance();

			return VDRouterBuilderRef(new VDRouterBuilder(VDRouterRef(new VDRouter(aVDSettings, aVDAnimation))));
		}
		VDRouterBuilderRef changeFloatValue(unsigned int aControl, float aValue, bool forceSend = false, bool toggle = false, bool increase = false, bool decrease = false) {
			mVDRouter->changeFloatValue(aControl, aValue, forceSend, toggle, increase, decrease);
			return shared_from_this();
		}
		VDRouterBuilderRef setWarpAFboIndex(float v1, float v2) {
			mVDRouter->setWarpAFboIndex(1, 1);
			return shared_from_this();
		}
		VDRouterBuilderRef setWarpBFboIndex(float v1, float v2) {
			int a = mVDRouter.use_count();
			mVDRouter->setWarpBFboIndex(1, 1);
			//return VDRouterBuilderRef(this); //BAD !!!
			return shared_from_this();
		}
		VDRouterRef getInstance() const {
			return mVDRouter;
		}

	private:
		VDRouterBuilder(VDRouterRef r) : mVDRouter(r) { }
		VDRouterRef mVDRouter;
	};


}
