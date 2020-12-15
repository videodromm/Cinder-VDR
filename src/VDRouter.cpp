#include "VDRouter.h"

using namespace videodromm;

VDRouter::VDRouter(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms) {
	
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mVDUniforms = aVDUniforms;
	//mVDOsc = VDOsc::create(mVDSettings, mVDAnimation);
	CI_LOG_V("VDRouter constructor");
	mFBOAChanged = false;
	mFBOBChanged = false;
	mSelectedWarp = 0;
	mSelectedFboA = 1;
	mSelectedFboB = 2;
}
void VDRouter::changeFloatValue(unsigned int aControl, float aValue, bool forceSend, bool toggle, bool increase, bool decrease) {
	/*if (aControl == 31) {
		CI_LOG_V("old value " + toString(mVDAnimation->getUniformValue(aControl)) + " newvalue " + toString(aValue));
	}*/
	float newValue;
	if (increase) {
		// increase
		newValue = mVDAnimation->getUniformValue(aControl) + 0.1f;
		if (newValue > 1.0f) newValue = 1.0f;
		aValue = newValue;
	}
	else {
		// decrease
		if (decrease) {
			newValue = mVDAnimation->getUniformValue(aControl) - 0.1f;
			if (newValue < 0.0f) newValue = 0.0f;
			aValue = newValue;
		}
		else {
			// toggle
			if (toggle) {
				newValue = mVDAnimation->getUniformValue(aControl);
				if (newValue > 0.0f) { newValue = 0.0f; }
				else { newValue = 1.0f; } // Check for max instead?
				aValue = newValue;
			}
		}
	}

	// check if changed
	if ((mVDAnimation->setUniformValue(aControl, aValue) && aControl != mVDUniforms->IFPS) || forceSend) {
		std::stringstream sParams;
		// update color vec3
		if (aControl > 0 && aControl < 4) {
			mVDAnimation->setVec3UniformValueByIndex(mVDUniforms->ICOLOR, vec3(
				mVDAnimation->getUniformValue(mVDUniforms->IFR),
				mVDAnimation->getUniformValue(mVDUniforms->IFG),
				mVDAnimation->getUniformValue(mVDUniforms->IFB)
			));

		}
		// update mouse vec4
		if (aControl > 41 && aControl < 45) {
			mVDAnimation->setVec4UniformValueByIndex(mVDUniforms->IMOUSE, vec4(
				mVDAnimation->getUniformValue(mVDUniforms->IMOUSEX),
				mVDAnimation->getUniformValue(mVDUniforms->IMOUSEY),
				mVDAnimation->getUniformValue(mVDUniforms->IMOUSEZ),
				1.0));

		}

		// update iResolution vec3
		if (aControl == 121 || aControl == 122) {
			mVDAnimation->setVec3UniformValueByIndex(mVDUniforms->IRESOLUTION, vec3(mVDAnimation->getUniformValue(mVDUniforms->IRESOLUTIONX), mVDAnimation->getUniformValue(mVDUniforms->IRESOLUTIONY), 1.0));
		}
		sParams << "{\"params\" :[{\"name\" : " << aControl << ",\"value\" : " << mVDAnimation->getUniformValue(aControl) << "}]}";
		std::string strParams = sParams.str();

		sendJSON(strParams);
	}
}

void VDRouter::sendJSON(const std::string& params) {
	/*if (mOscSender) {
	Observer Observable
	mVDOsc->send(params);
	mVDSocketio->send(params);
	}*/
}

void VDRouter::updateParams(int iarg0, float farg1) {
	if (farg1 > 0.1) {
		//avoid to send twice
		if (iarg0 == 58) {
			// track left		
			mVDSettings->iTrack--;
			if (mVDSettings->iTrack < 0) mVDSettings->iTrack = 0;
		}
		if (iarg0 == 59) {
			// track right
			mVDSettings->iTrack++;
			if (mVDSettings->iTrack > 7) mVDSettings->iTrack = 7;
		}
		if (iarg0 == 60) {
			// set (reset blendmode)
			mVDSettings->iBlendmode = 0;
		}
		if (iarg0 == 61) {
			// right arrow
			mVDSettings->iBlendmode--;
			if (mVDSettings->iBlendmode < 0) mVDSettings->iBlendmode = mVDAnimation->getBlendModesCount() - 1;
		}
		if (iarg0 == 62) {
			// left arrow
			mVDSettings->iBlendmode++;
			if (mVDSettings->iBlendmode > mVDAnimation->getBlendModesCount() - 1) mVDSettings->iBlendmode = 0;
		}
	}
	if (iarg0 > 0 && iarg0 < 9) {
		// sliders 
		//! 20200526 mVDSocketio->changeFloatValue(iarg0, farg1);
		
		mVDAnimation->setUniformValue(iarg0, farg1);
	}
	if (iarg0 > 10 && iarg0 < 19) {
		// rotary 
		//! 20200526 
		//mVDSocketio->changeFloatValue(iarg0, farg1);
		// audio multfactor
		//! 20200526 if (iarg0 == 13) mVDSocketio->changeFloatValue(iarg0, (farg1 + 0.01) * 10);
		// exposure
		//! 20200526 if (iarg0 == 14) mVDSocketio->changeFloatValue(iarg0, (farg1 + 0.01) * mVDAnimation->getMaxUniformValue(14));
		// xfade
		if (iarg0 == mVDUniforms->IXFADE) {//18
			//! 20200526 mVDSocketio->changeFloatValue(iarg0, farg1);
			//mVDSettings->xFade = farg1;
			//mVDSettings->xFadeChanged = true;
		}
	}
	// buttons
	if (iarg0 > 20 && iarg0 < 29) {
		// top row
		//! 20200526 mVDSocketio->changeFloatValue(iarg0, farg1);
	}
	if (iarg0 > 30 && iarg0 < 39)
	{
		// middle row
		//! 20200526 mVDSocketio->changeFloatValue(iarg0, farg1);
		//mVDAnimation->setIntUniformValueByIndex(mVDUniforms->IFBOA, iarg0 - 31);
	}
	if (iarg0 > 40 && iarg0 < 49) {
		// low row 
		//! 20200526 mVDSocketio->changeFloatValue(iarg0, farg1);
		//mVDAnimation->setIntUniformValueByIndex(mVDUniforms->IFBOB, iarg0 - 41);
	}
	//if (iarg0 > 0 && iarg0 < 49) {
		// float values 
		//mVDSocketio->wsWrite("{\"params\" :[{ \"name\":" + toString(iarg0) + ",\"value\":" + toString(mVDAnimation->getUniformValue(iarg0)) + "}]}");
	//}
}


void VDRouter::colorWrite()
{
#if defined( CINDER_MSW )
	// lights4events
	char col[97];
	int r = (int)(mVDAnimation->getUniformValue(1) * 255);
	int g = (int)(mVDAnimation->getUniformValue(2) * 255);
	int b = (int)(mVDAnimation->getUniformValue(3) * 255);
	int a = (int)(mVDAnimation->getUniformValue(4) * 255);
	//sprintf(col, "#%02X%02X%02X", r, g, b);
	sprintf(col, "{\"type\":\"action\", \"parameters\":{\"name\":\"FC\",\"parameters\":{\"color\":\"#%02X%02X%02X%02X\",\"fading\":\"NONE\"}}}", a, r, g, b);
	//! 20200526 mVDSocketio->wsWrite(col);
#endif
}
