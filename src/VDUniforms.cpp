#include "VDUniforms.h"

using namespace videodromm;
VDUniformsRef VDUniforms::UNIFORMS;
VDUniformsRef	VDUniforms::create() {
	if (UNIFORMS.use_count() == 0) {
		UNIFORMS = VDUniformsRef(new VDUniforms());
	}
	return UNIFORMS;
}
VDUniforms::VDUniforms() {
	// render windows
	mRenderWidth = 1280;
	mRenderHeight = 720;
	// textures
	for (size_t i{ 0 }; i < 30; i++)
	{
		createSampler2DUniform("iChannel" + toString(i), 400 + i, i);// TODO verify doesn't mess up type (uint!)
	}
	createSampler2DUniform("inputImage", 399, 0);// TODO verify doesn't mess up type (uint!)

	//mUniformsJson = getAssetPath("") / mAssetsPath / "uniforms.json";
	/*mUniformsJson = getAssetPath("") / "uniforms.json";
	if (fs::exists(mUniformsJson)) {
		loadUniforms(loadFile(mUniformsJson));
	}
	else {*/
	// global time in seconds
	// TODO 20200301 get rid of iTime createFloatUniform("iTime", ITIME, 0.0f); // 0
	createFloatUniform("TIME", ITIME, 0.0f); // 0
	// sliders
	// red
	createFloatUniform("iColorX", ICOLORX, 0.45f); // 1
	// green
	createFloatUniform("iColorY", ICOLORY, 0.0f); // 2
	// blue
	createFloatUniform("iColorZ", ICOLORZ, 1.0f); // 3
	// Alpha 
	createFloatUniform("iAlpha", IALPHA, 1.0f); // 4
	// red multiplier 
	createFloatUniform("iRedMultiplier", IFRX, 1.0f, 0.0f, 3.0f); // 5
	// green multiplier 
	createFloatUniform("iGreenMultiplier", IFGX, 1.0f, 0.0f, 3.0f); // 6
	// blue multiplier 
	createFloatUniform("iBlueMultiplier", IFBX, 1.0f, 0.0f, 3.0f); // 7
	// gstnsmk
	createFloatUniform("iSobel", ISOBEL, 0.02f, 0.02f, 1.0f); // 8
	// RotationSpeed
	createFloatUniform("iRotationSpeed", IROTATIONSPEED, 0.02f, -0.1f, 1.0f); // 9
	// vignette
	createFloatUniform("iVignette", IVIGNETTE, 0.0f); // 10
	// background red
	createFloatUniform("iBackgroundColorX", IBACKGROUNDCOLORX, 0.56f); // 11
	// background green
	createFloatUniform("iBackgroundColorY", IBACKGROUNDCOLORY, 0.0f); // 12
	// background blue
	createFloatUniform("iBackgroundColorZ", IBACKGROUNDCOLORZ, 1.0f); // 13
	createVec3Uniform("iBackgroundColor", IBACKGROUNDCOLOR, vec3(getUniformValue(IBACKGROUNDCOLORX), getUniformValue(IBACKGROUNDCOLORY), getUniformValue(IBACKGROUNDCOLORZ))); // 311

	// rotary
	// exposure
	createFloatUniform("iExposure", IEXPOSURE, 1.0f, 0.0f, 3.0f); // 14
	// Pixelate
	createFloatUniform("iPixelate", IPIXELATE, 1.0f, 0.01f); // 15
	// Trixels
	createFloatUniform("iTrixels", ITRIXELS, 0.0f); // 16
	// iChromatic
	createFloatUniform("iChromatic", ICHROMATIC, 0.0f, 0.000000001f); // 17
	// iCrossfade
	createFloatUniform("iCrossfade", IXFADE, 1.0f); // 18
	// tempo time
	createFloatUniform("iTempoTime", ITEMPOTIME, 0.1f); // 19
	// fps
	createFloatUniform("iFps", IFPS, 60.0f, 0.0f, 500.0f); // 20	
	// iBpm 
	createFloatUniform("iBpm", IBPM, 165.0f, 0.000000001f, 400.0f); // 21
	// Speed 
	createFloatUniform("iSpeed", ISPEED, 1.0f, 0.01f, 12.0f); // 22
	// slitscan / matrix (or other) Param1 
	createFloatUniform("iPixelX", IPIXELX, 0.01f, -1.5f, 1.5f); // 23
	// slitscan / matrix(or other) Param2 
	createFloatUniform("iPixelY", IPIXELY, 0.01f, -1.5f, 1.5f); // 24
	// delta time in seconds
	createFloatUniform("iDeltaTime", IDELTATIME, 60.0f / 160.0f); // 25
	// Steps
	createFloatUniform("iSteps", ISTEPS, 16.0f, 1.0f, 128.0f); // 26
	// ratio
	createFloatUniform("iRatio", IRATIO, 36.0f, 0.00000000001f, 40.0f); // 27
	// zoom
	createFloatUniform("iZoom", IZOOM, 1.0f, 0.95f, 1.1f); // 28
	// Max Volume
	createFloatUniform("volume0", IMAXVOLUME, 0.0f, 0.0f, 255.0f); // 29

	// contour
	createFloatUniform("iContour", ICONTOUR, 0.0f, 0.0f, 0.5f); // 30
	// weight mix fbo texture 0
	createFloatUniform("iWeight0", IWEIGHT0, 0.0f); // 31
	// weight texture 1
	createFloatUniform("iWeight1", IWEIGHT1, 1.0f); // 32
	// weight texture 2
	createFloatUniform("iWeight2", IWEIGHT2, 0.0f); // 33
	// weight texture 3
	createFloatUniform("iWeight3", IWEIGHT3, 0.0f); // 34
	// weight texture 4
	createFloatUniform("iWeight4", IWEIGHT4, 0.0f); // 35
	// weight texture 5
	createFloatUniform("iWeight5", IWEIGHT5, 0.0f); // 36
	// weight texture 6
	createFloatUniform("iWeight6", IWEIGHT6, 0.0f); // 37
	// weight texture 7
	createFloatUniform("iWeight7", IWEIGHT7, 0.0f); // 38
	// weight texture 8 
	createFloatUniform("iWeight8", IWEIGHT8, 0.0f); // 39
	// elapsed in bar 
	//createFloatUniform("iElapsed", IELAPSED, 0.0f); // 39
	// Audio multfactor 
	createFloatUniform("iAudioX", IAUDIOX, 1.0f, 0.01f, 30.0f); // 40

	// iMouseX  
	createFloatUniform("iMouseX", IMOUSEX, 0.27710f, 0.0f, 1280.0f); // 42
	// iMouseY  
	createFloatUniform("iMouseY", IMOUSEY, 0.5648f, 0.0f, 800.0f); // 43
	// iMouseZ  
	createFloatUniform("iMouseZ", IMOUSEZ, 0.0f, 0.0f, 1.0f); // 44
	// vignette amount
	createFloatUniform("iMouseW", IMOUSEW, 0.91f, 0.0f, 1.0f); // 45
	// vignette falloff
	createFloatUniform("iVFallOff", IVFALLOFF, 0.31f, 0.0f, 1.0f); // 46
	// hydra time
	//createFloatUniform("time", TIME, 0.0f); // 47

	// bad tv
	createFloatUniform("iBadTv", IBADTV, 0.0f, 0.0f, 5.0f); // 48
	// iTimeFactor
	createFloatUniform("iTimeFactor", ITIMEFACTOR, 1.0f); // 49
	// int
	// blend mode 
	createFloatUniform("iBlendmode", IBLENDMODE, 0); // 50
	// beat 
	createFloatUniform("iBeat", IBEAT, 0.0f, 0.0f, 300.0f); // 51
	// bar 
	createFloatUniform("iBar", IBAR, 0.0f, 0.0f, 3008.0f); // 52
	// bar 
	createFloatUniform("iBarBeat", IBARBEAT, 1.0f, 1.0f, 1200.0f); // 53		
	// fbo A
	createFloatUniform("iFboA", IFBOA, 0); // 54
	// fbo B
	createFloatUniform("iFboB", IFBOB, 1); // 55
	// iOutW
	createFloatUniform("iOutW", IOUTW, mRenderWidth); // 56
	// iOutH  
	createFloatUniform("iOutH", IOUTH, mRenderHeight); // 57
	// beats per bar 
	createFloatUniform("iBeatsPerBar", IBEATSPERBAR, 4); // 59
	// iPhase 
	createFloatUniform("iPhase", IPHASE, 1.0f); // 60
	// vignette amount
	createFloatUniform("iVAmount", IVAMOUNT, 0.91f, 0.0f, 1.0f); // 62
	//createVec3Uniform("iChannelResolution[0]", 63, vec3(mVDParams->getFboWidth(), mVDParams->getFboHeight(), 1.0));
	createFloatUniform("iDateX", IDATEX, 0.91f, 0.0f, 90000.0f); // 25 hours
	createFloatUniform("iDateY", IDATEY, 0.91f, 0.0f, 90000.0f); // 25 hours
	createFloatUniform("iDateZ", IDATEZ, 0.91f, 0.0f, 90000.0f); // 25 hours
	createFloatUniform("iDateW", IDATEW, 0.91f, 0.0f, 90000.0f); // 25 hours
	// vec3
	// iResolutionX (should be fbowidth?) 
	createFloatUniform("iResolutionX", IRESOLUTIONX, mRenderWidth, 320.01f, 4280.0f); // 121
	// iResolutionY (should be fboheight?)  
	createFloatUniform("iResolutionY", IRESOLUTIONY, mRenderHeight, 240.01f, 2160.0f); // 122
	createVec3Uniform("iResolution", IRESOLUTION, vec3(getUniformValue(IRESOLUTIONX), getUniformValue(IRESOLUTIONY), 1.0)); // 421


	createVec3Uniform("iColor", ICOLOR, vec3(0.45, 0.0, 1.0)); // 301


	createVec4Uniform("iDate", IDATE, vec4(2021.0f, 12.0f, 1.0f, 5.0f));// 363

	// boolean
	// invert
	// glitch
	createFloatUniform("iGlitch", IGLITCH, 0.0f); // 81

	// toggle
	createFloatUniform("iToggle", ITOGGLE, 0.0f); // 83
	// invert
	createFloatUniform("iInvert", IINVERT, 0.0f); // 86
	// greyscale 
	createFloatUniform("iGreyScale", IGREYSCALE, 0.0f); //87
	createFloatUniform("iClear", ICLEAR, 0.0f); // 88
	createFloatUniform("iDebug", IDEBUG, 0.0f); // 129
	createFloatUniform("iXorY", IXORY, 0.0f); // 130
	createFloatUniform("iFlipH", IFLIPH, 0.0f); // 131
	createFloatUniform("iFlipV", IFLIPV, 0.0f); // 132
	createFloatUniform("iFlipPostH", IFLIPPOSTH, 0.0f); // 133
	createFloatUniform("iFlipPostV", IFLIPPOSTV, 0.0f); // 134

	// 119 to 124 timefactor from midithor sos
	// floats for warps
	// srcArea 
	createFloatUniform("srcXLeft", SRCXLEFT, 0.0f, 0.0f, 4280.0f); // 160
	createFloatUniform("srcXRight", SRCXRIGHT, mRenderWidth, 320.01f, 4280.0f); // 161
	createFloatUniform("srcYLeft", SRCYLEFT, 0.0f, 0.0f, 1024.0f); // 162
	createFloatUniform("srcYRight", SRCYRIGHT, mRenderHeight, 0.0f, 1024.0f); // 163
	// iFreq0  
	createFloatUniform("iFreq0", IFREQ0, 0.0f, 0.01f, 256.0f); // 140	
	// iFreq1  
	createFloatUniform("iFreq1", IFREQ1, 0.0f, 0.01f, 256.0f); // 141
	// iFreq2  
	createFloatUniform("iFreq2", IFREQ2, 0.0f, 0.01f, 256.0f); // 142
	// iFreq3  
	createFloatUniform("iFreq3", IFREQ3, 0.0f, 0.01f, 256.0f); // 143

	// vec2
	createVec2Uniform("resolution", RESOLUTION, vec2(1280.0f, 720.0f)); // hydra 150
	createVec2Uniform("RENDERSIZE", RENDERSIZE, vec2(getUniformValue(IRESOLUTIONX), getUniformValue(IRESOLUTIONY))); // isf 151
		// vec4
	createVec4Uniform("iMouse", IMOUSE, vec4(0.27710f, 0.5648f, 0.0f, 0.0f)); // 342
	// vec4 kinect2
	createVec4Uniform("iSpineBase", 200, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("SpineMid", 201, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("Neck", 202, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("Head", 203, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("ShldrL", 204, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("ElbowL", 205, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("WristL", 206, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("HandL", 207, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("ShldrR", 208, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("ElbowR", 209, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("WristR", 210, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("HandR", 211, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("HipL", 212, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("KneeL", 213, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("AnkleL", 214, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("FootL", 215, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("HipR", 216, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("KneeR", 217, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("AnkleR", 218, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("FootR", 219, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("SpineShldr", 220, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("HandTipL", 221, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("ThumbL", 222, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("HandTipR", 223, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	createVec4Uniform("ThumbR", 224, vec4(320.0f, 240.0f, 0.0f, 0.0f));
	//}
}
void VDUniforms::loadUniforms(const ci::DataSourceRef& source) {

	JsonTree json(source);

	// try to load the specified json file
	if (json.hasChild("uniforms")) {
		JsonTree u(json.getChild("uniforms"));

		// iterate uniforms
		for (size_t i{ 0 }; i < u.getNumChildren(); i++) {
			JsonTree child(u.getChild(i));

			if (child.hasChild("uniform")) {
				JsonTree w(child.getChild("uniform"));
				// create uniform of the correct type
				int uniformType = (w.hasChild("type")) ? w.getValueForKey<int>("type") : 0;
				switch (uniformType) {
				case GL_FLOAT:
					// float 5126 GL_FLOAT 0x1406
					floatFromJson(child);
					break;
				case GL_SAMPLER_2D:
					// sampler2d 35678 GL_SAMPLER_2D 0x8B5E
					sampler2dFromJson(child);
					break;
				case GL_FLOAT_VEC2:
					// vec2 35664
					vec2FromJson(child);
					break;
				case GL_FLOAT_VEC3:
					// vec3 35665
					vec3FromJson(child);
					break;
				case GL_FLOAT_VEC4:
					// vec4 35666 GL_FLOAT_VEC4
					vec4FromJson(child);
					break;
				case GL_INT:
					// int 5124 GL_INT 0x1404
					floatFromJson(child);
					break;
				case GL_BOOL:
					// boolean 35670 GL_BOOL 0x8B56
					floatFromJson(child);
					break;
				}
			}
		}
	}
}

void VDUniforms::floatFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	float jValue, jMin, jMax;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 249;
		jValue = (u.hasChild("value")) ? u.getValueForKey<float>("value") : 0.01f;
		jMin = (u.hasChild("min")) ? u.getValueForKey<float>("min") : 0.0f;
		jMax = (u.hasChild("max")) ? u.getValueForKey<float>("max") : 1.0f;
		createFloatUniform(jName, jCtrlIndex, jValue, jMin, jMax);
	}
}
void VDUniforms::sampler2dFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	int jTextureIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 250;
		jTextureIndex = (u.hasChild("textureindex")) ? u.getValueForKey<int>("textureindex") : 0;;
		createSampler2DUniform(jName, jTextureIndex);
	}
}
void VDUniforms::vec2FromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 251;
		createVec2Uniform(jName, jCtrlIndex);
	}
}
void VDUniforms::vec3FromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 252;
		createVec3Uniform(jName, jCtrlIndex);
	}
}
void VDUniforms::vec4FromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 253;
		createVec4Uniform(jName, jCtrlIndex);
	}
}
/*
void VDUniforms::intFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex, jValue;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 254;
		jValue = (u.hasChild("value")) ? u.getValueForKey<int>("value") : 1;
		createIntUniform(jName, jCtrlIndex, jValue);
	}

}
void VDUniforms::boolFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	bool jValue;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 255;
		jValue = (u.hasChild("value")) ? u.getValueForKey<bool>("value") : false;
		createBoolUniform(jName, jCtrlIndex, jValue);
	}
}*/

bool VDUniforms::setUniformValue(unsigned int aIndex, float aValue) {
	bool rtn = false;

	// we can't change TIME at index 0
	if (aIndex > 0) {
		if (aIndex == IBPM) {
			if (aValue > 0.0f) {
				setUniformValue(IDELTATIME, 60 / aValue);
			}
		}
		if (shaderUniforms[aIndex].floatValue != aValue) {
			if ((aValue >= shaderUniforms[aIndex].minValue && aValue <= shaderUniforms[aIndex].maxValue) || shaderUniforms[aIndex].anim > 0) {
				shaderUniforms[aIndex].floatValue = aValue;
				rtn = true;
			}
			else {
				// TODO 20211011 bug max = 0.0
				if (shaderUniforms[aIndex].maxValue == 0.0f) {
					shaderUniforms[aIndex].maxValue == 1.0f;
				}
			}
		}
		// not all controls are from 0.0 to 1.0
		/* not working float lerpValue = lerp<float, float>(shaderUniforms[getUniformNameForIndex(aIndex)].minValue, shaderUniforms[getUniformNameForIndex(aIndex)].maxValue, aValue);
		if (shaderUniforms[getUniformNameForIndex(aIndex)].floatValue != lerpValue) {
			shaderUniforms[getUniformNameForIndex(aIndex)].floatValue = lerpValue;
			rtn = true;
		}*/
	}
	else {
		// no max 
		shaderUniforms[aIndex].floatValue = aValue;
	}
	return rtn;
}
int VDUniforms::stringToIndex(const std::string& key) {
	int rtn = -1;
	if (key == "iTime" || key == "uTime" || key == "time" || key == "TIME") {
		rtn = ITIME;
	}
	else if (key == "iColorX") {
		rtn = ICOLORX;
	} // 1
	// green
	else if (key == "iColorY") {
		rtn = ICOLORY;
	} // 2
	// blue
	else if (key == "iColorZ") {
		rtn = ICOLORZ;
	} // 3
	// Alpha 
	else if (key == "iAlpha") {
		rtn = IALPHA;
	} // 4
	// red multiplier 
	else if (key == "iRedMultiplier") {
		rtn = IFRX;
	} // 5
	// green multiplier 
	else if (key == "iGreenMultiplier") {
		rtn = IFGX;
	} // 6
	// blue multiplier 
	else if (key == "iBlueMultiplier") {
		rtn = IFBX;
	} // 7
	// gstnsmk
	else if (key == "iSobel") {
		rtn = ISOBEL;
	} // 8
	// RotationSpeed
	else if (key == "iRotationSpeed") {
		rtn = IROTATIONSPEED;
	} // 9
	// background 
	else if (key == "iBackgroundColor") {
		rtn = IBACKGROUNDCOLOR;
	} // 10
	else if (key == "iBackgroundColorX") {
		rtn = IBACKGROUNDCOLORX;
	} // 11
	else if (key == "iBackgroundColorY") {
		rtn = IBACKGROUNDCOLORY;
	} // 12
	else if (key == "iBackgroundColorZ") {
		rtn = IBACKGROUNDCOLORZ;
	} // 13

	// rotary
	// exposure
	else if (key == "iExposure") {
		rtn = IEXPOSURE;
	} // 14
	// Pixelate
	else if (key == "iPixelate") {
		rtn = IPIXELATE;
	} // 15
	// Trixels
	else if (key == "iTrixels") {
		rtn = ITRIXELS;
	} // 16
	// iChromatic
	else if (key == "iChromatic") {
		rtn = ICHROMATIC;
	} // 17
	// iCrossfade
	else if (key == "iCrossfade") {
		rtn = IXFADE;
	} // 18
	// tempo time
	else if (key == "iTempoTime") {
		rtn = ITEMPOTIME;
	} // 19
	// fps
	else if (key == "iFps") {
		rtn = IFPS;
	} // 20	
	// iBpm 
	else if (key == "iBpm") {
		rtn = IBPM;
	} // 21
	// Speed 
	else if (key == "speed") {
		rtn = ISPEED;
	} // 22
	// slitscan / matrix (or other) Param1 
	else if (key == "iPixelX") {
		rtn = IPIXELX;
	} // 23
	// slitscan / matrix(or other) Param2 
	else if (key == "iPixelY") {
		rtn = IPIXELY;
	} // 24
	// delta time in seconds
	else if (key == "iDeltaTime") {
		rtn = IDELTATIME;
	} // 25
	// Steps
	else if (key == "iSteps") {
		rtn = ISTEPS;
	} // 26
	// ratio
	else if (key == "iRatio") {
		rtn = IRATIO;
	} // 27
	// zoom
	else if (key == "iZoom") {
		rtn = IZOOM;
	} // 28
	// volume0 from hydra 
	else if (key == "volume0") {
		rtn = IMAXVOLUME;
	} // 29
	// contour
	else if (key == "iContour") {
		rtn = ICONTOUR;
	} // 30

	// weight mix fbo texture 0
	else if (key == "iWeight0") {
		rtn = IWEIGHT0;
	} // 31
	// weight texture 1
	else if (key == "iWeight1") {
		rtn = IWEIGHT1;
	} // 32
	// weight texture 2
	else if (key == "iWeight2") {
		rtn = IWEIGHT2;
	} // 33
	// weight texture 3
	else if (key == "iWeight3") {
		rtn = IWEIGHT3;
	} // 34
	// weight texture 4
	else if (key == "iWeight4") {
		rtn = IWEIGHT4;
	} // 35
	// weight texture 5
	else if (key == "iWeight5") {
		rtn = IWEIGHT5;
	} // 36
	// weight texture 6
	else if (key == "iWeight6") {
		rtn = IWEIGHT6;
	} // 37
	// weight texture 7
	else if (key == "iWeight7") {
		rtn = IWEIGHT7;
	} // 38
	// weight texture 8 
	else if (key == "iWeight8") {
		rtn = IWEIGHT8;
	} // 39
	// Audio multfactor 
	else if (key == "iAudioX") {
		rtn = IAUDIOX;
	} // 40
	// IMouse vec4
	else if (key == "iMouse") {
		rtn = IMOUSE;
	}// 342
	// iMouseX  
	else if (key == "iMouseX") {
		rtn = IMOUSEX;
	} // 42
	// iMouseY  
	else if (key == "iMouseY") {
		rtn = IMOUSEY;
	} // 43
	// iMouseZ  
	else if (key == "iMouseZ") {
		rtn = IMOUSEZ;
	} // 44
	// iMouseW
	else if (key == "iMouseW") {
		rtn = IMOUSEW;
	} // 45
	// vignette falloff
	else if (key == "iVFallOff") {
		rtn = IVFALLOFF;
	} // 46
	// bad tv
	else if (key == "iBadTv") {
		rtn = IBADTV;
	} // 48

	// iTimeFactor
	else if (key == "iTimeFactor") {
		rtn = ITIMEFACTOR;// 49
	}
	// int
	// blend mode 
	else if (key == "iBlendmode") {
		rtn = IBLENDMODE;
	} // 50
	// beat 
	else if (key == "iBeat") {
		rtn = IBEAT;
	} // 51
	// bar 
	else if (key == "iBar") {
		rtn = IBAR;
	} // 52
	// bar 
	else if (key == "iBarBeat") {
		rtn = IBARBEAT;
	} // 53		
	// fbo A
	else if (key == "iFboA") {
		rtn = IFBOA;
	} // 54
	// fbo B
	else if (key == "iFboB") {
		rtn = IFBOB;
	} // 55
	// iOutW
	else if (key == "iOutW") {
		rtn = IOUTW;
	} // 56
	// iOutH  
	else if (key == "iOutH") {
		rtn = IOUTH;
	} // 57
	// beats per bar 
	else if (key == "iBeatsPerBar") {
	rtn = IBEATSPERBAR;
	} // 59
	else if (key == "iVAmount") {
	rtn = IVAMOUNT;
	} // 62
	// vec3
	else if (key == "iResolution") {
		rtn = IRESOLUTION;
	} // 120
	// iResolutionX (should be fbowidth?) 
	else if (key == "iResolutionX") {
		rtn = IRESOLUTIONX;
	} // 121
	// iResolutionY (should be fboheight?)  
	else if (key == "iResolutionY") {
		rtn = IRESOLUTIONY;
	} // 122


	// IPHASE 
	else if (key == "iPhase") {
		rtn = IPHASE;
	} // 60
	else if (key == "iColor") {
		rtn = ICOLOR;
	} // 301

	// vec4
	
	else if (key == "iDate") {
		rtn = IDATE;
	}//71

	// boolean
	// invert
	// glitch
	else if (key == "iGlitch") {
		rtn = IGLITCH;
	} // 81
	// vignette 10
	else if (key == "iVignette") {
		rtn = IVIGNETTE;
	} // 10
	// toggle
	else if (key == "iToggle") {
		rtn = ITOGGLE;
	} // 83
	// invert
	else if (key == "iInvert") {
		rtn = IINVERT;
	} // 86
	// greyscale 
	else if (key == "iGreyScale") {
		rtn = IGREYSCALE;
	} //87

	else if (key == "iClear") {
		rtn = ICLEAR;
	} // 88
	else if (key == "iDebug") {
		rtn = IDEBUG;
	} // 129
	else if (key == "iXorY") {
		rtn = IXORY;
	} // 130
	else if (key == "iFlipH") {
		rtn = IFLIPH;
	} // 131
	else if (key == "iFlipV") {
		rtn = IFLIPV;
	} // 132
	else if (key == "iFlipPostH") {
		rtn = IFLIPPOSTH;
	} // 133
	else if (key == "iFlipPostV") {
		rtn = IFLIPPOSTV;
	} // 134

	// 119 to 124 timefactor from midithor sos
	// floats for warps
	// srcArea 
	else if (key == "srcXLeft") {
		rtn = SRCXLEFT;
	} // 160
	else if (key == "srcXRight") {
		rtn = SRCXRIGHT;
	} // 161
	else if (key == "srcYLeft") {
		rtn = SRCYLEFT;
	} // 162
	else if (key == "srcYRight") {
		rtn = SRCYRIGHT;
	} // 163
	// iFreq0  
	else if (key == "iFreq0") {
		rtn = IFREQ0;
	} // 140	
	// iFreq1  
	else if (key == "iFreq1") {
		rtn = IFREQ1;
	} // 141
	// iFreq2  
	else if (key == "iFreq2") {
		rtn = IFREQ2;
	} // 142
	// iFreq3  
	else if (key == "iFreq3") {
		rtn = IFREQ3;
	} // 143

	// vec2
	else if (key == "resolution") {
		rtn = RESOLUTION;
	} // hydra 150
	else if (key == "RENDERSIZE") {
		rtn = RENDERSIZE;
	} // isf 151
	else if (key == "ciModelViewProjection") {
		rtn = 498; // TODO
	}
	else if (key == "inputImage") {
		rtn = 499; // TODO
	}
	if (rtn == -1) {
		// not found
		createFloatUniform(key, 500); // TODO
	}
	return rtn;
}

bool VDUniforms::isExistingUniform(const std::string& aName) {
	return stringToIndex(aName) != -1;
};
