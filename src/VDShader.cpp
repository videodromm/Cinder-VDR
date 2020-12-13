#include "VDShader.h"

using namespace videodromm;
/*

	test exists at assets path or full path
	get isf string
	compile
	run
	create thumb using fbo
	save isf in assets session subfolder
was VDShader::VDShader(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, const string& aFileOrPath, const string& aFragmentShaderString) {
*/
VDShader::VDShader(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, const std::string& aFileOrPath, const std::string& aShaderFragmentString, gl::TextureRef aTexture) {
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	// Params
	mVDParams = VDParams::create();
	mFragmentShaderString = aShaderFragmentString;
	mTexture = aTexture;
	mValid = false;
	mError = "";
	bool fileExists = true;
	// shadertoy include
	shaderInclude = loadString(loadAsset("shadertoy.vd"));
	mFragFilePath = getAssetPath("") / mVDSettings->mAssetsPath / aFileOrPath;
	// priority to string 
	if (mFragmentShaderString.length() > 0) {
		mFileNameWithExtension = mFragFilePath.filename().string();
		mValid = setFragmentString(mFragmentShaderString, mFileNameWithExtension);
	}
	else {
		if (aFileOrPath.length() > 0) {
			if (fs::exists(aFileOrPath)) {
				// it's a full path
				mFragFilePath = aFileOrPath;
			}
			else {
				// try in assets folder			
				if (!fs::exists(mFragFilePath)) {
					mFragFilePath = getAssetPath("") / aFileOrPath;
					if (!fs::exists(mFragFilePath)) {
						fileExists = false;
						mError = "shader file does not exist in assets root or current subfolder:" + aFileOrPath;
					}
				}

			}
		}
		else {
			mError = "shader file empty";
		}
		if (fileExists) {
			// file exists
			if (loadFragmentStringFromFile()) {
				fboFmt.setColorTextureFormat(fmt);
				mRenderedTexture = ci::gl::Texture::create(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight(), ci::gl::Texture::Format().loadTopDown());
				mThumbFbo = gl::Fbo::create(mVDParams->getPreviewWidth(), mVDParams->getPreviewHeight(), fboFmt);
				getThumbTexture();
			}
			else {
				mError = "could not compile:" + aFileOrPath;
			}

		}
	}

	mVDSettings->mErrorMsg = mError + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
	CI_LOG_V("VDShaders constructor " + mError);

#pragma region shaderStrings
	mDefaultVextexShaderString = "#version 150\n"
		"uniform mat4 ciModelViewProjection;\n"
		"in vec4 ciPosition;\n"
		"in vec4 ciColor;\n"
		"in vec2 ciTexCoord0;\n"
		"out vec4 vertColor;\n"
		"out vec2 vertTexCoord0;\n"
		"void main()\n"
		"{\n"
		"vertColor = ciColor;\n"
		"vertTexCoord0 = ciTexCoord0;\n"
		"gl_Position = ciModelViewProjection * ciPosition;\n"
		"}\n";
	mDefaultFragmentShaderString = "out vec4 fragColor;\n"
		"uniform sampler2D iChannel0;\n"
		"uniform sampler2D iChannel1;\n"
		"uniform sampler2D iChannel2;\n"
		"uniform sampler2D iChannel3;\n"
		"uniform sampler2D iChannel4;\n"
		"uniform sampler2D iChannel5;\n"
		"uniform sampler2D iChannel6;\n"
		"uniform sampler2D iChannel7;\n"
		"uniform sampler2D iChannel8;\n"
		"uniform sampler2D iChannel9;\n"
		"uniform vec3 iResolution;\n"
		"void main(void)\n"
		"{\n"
		"vec2 uv = gl_FragCoord.xy / iResolution.xy;\n"
		"vec3 t0 = texture(iChannel0, uv).rgb;\n"
		"vec3 t1 = 0.9*texture(iChannel1, uv).rgb;\n"
		"vec3 t2 = 0.8*texture(iChannel2, uv).rgb;\n"
		"vec3 t3 = 0.7*texture(iChannel3, uv).rgb;\n"
		"vec3 t4 = 0.6*texture(iChannel4, uv).rgb;\n"
		"vec3 t5 = 0.5*texture(iChannel5, uv).rgb;\n"
		"vec3 t6 = 0.4*texture(iChannel6, uv).rgb;\n"
		"vec3 t7 = 0.3*texture(iChannel7, uv).rgb;\n"
		"vec3 t8 = 0.2*texture(iChannel8, uv).rgb;\n"
		"vec3 t9 = 0.1*texture(iChannel9, uv).rgb;\n"
		"fragColor = vec4(t0 + t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8 + t9, 1.0);\n"
		"}\n";
	mHydraFragmentShaderString = "precision mediump float;\n"
		"varying vec2 uv;\n"
		"void main() {\n"
		"vec2 st = gl_FragCoord.xy/RENDERSIZE.xy;\n"
		"gl_FragColor = vec4(st.x,st.y,sin(TIME), 1.0);\n"
		"}\n";
	mMixetteFragmentShaderString = "uniform vec3      	iResolution;\n"
		"uniform sampler2D 	iChannel0;\n"
		"uniform sampler2D 	iChannel1;\n"
		"uniform sampler2D   iChannel2;\n"
		"uniform sampler2D   iChannel3;\n"
		"uniform sampler2D   iChannel4;\n"
		"uniform sampler2D   iChannel5;\n"
		"uniform sampler2D   iChannel6;\n"
		"uniform sampler2D   iChannel7;\n"
		"uniform float       iWeight0;\n"
		"uniform float       iWeight1;\n"
		"uniform float       iWeight2;\n"
		"uniform float       iWeight3;\n"
		"uniform float       iWeight4;\n"
		"uniform float       iWeight5;\n"
		"uniform float       iWeight6;\n"
		"uniform float       iWeight7;\n"
		"vec2  fragCoord = gl_FragCoord.xy;\n"
		"void main() {\n"
		"vec2 uv = gl_FragCoord.xy / iResolution.xy;\n"
		"vec3 c = texture(iChannel0, uv).xyz * iWeight0 + texture(iChannel1, uv).xyz * iWeight1 + texture(iChannel2, uv).xyz * iWeight2 + texture(iChannel3, uv).xyz * iWeight3 +\n"
		"texture(iChannel4, uv).xyz * iWeight4 + texture(iChannel4, uv).xyz * iWeight5 + texture(iChannel4, uv).xyz * iWeight6 + texture(iChannel4, uv).xyz * iWeight7;\n"
		"gl_FragColor = vec4(c.r, c.g, c.b, 1.0);\n"
		"}";
	//mPostFragmentShaderString;uv.y = 1.0 - uv.y;
	mPostFragmentShaderString = "uniform vec3 iResolution;\n"
		"uniform float TIME;\n"
		"uniform sampler2D 	iChannel0;\n"
		"uniform sampler2D 	iChannel1;\n"
		"uniform float		iExposure;\n"
		"uniform float		iSobel;\n"
		"uniform float		iChromatic;\n"
		"vec2  fragCoord = gl_FragCoord.xy; // keep the 2 spaces between vec2 and fragCoord\n"
		"float intensity(in vec4 c){return sqrt((c.x*c.x)+(c.y*c.y)+(c.z*c.z));}\n"
		"vec4 sobel(float stepx, float stepy, vec2 center) {\n"
		"float tleft = intensity(texture(iChannel0, center + vec2(-stepx, stepy)));\n"
		"float left = intensity(texture(iChannel0, center + vec2(-stepx, 0)));\n"
		"float bleft = intensity(texture(iChannel0, center + vec2(-stepx, -stepy)));\n"
		"float top = intensity(texture(iChannel0, center + vec2(0, stepy)));\n"
		"float bottom = intensity(texture(iChannel0, center + vec2(0, -stepy)));\n"
		"float tright = intensity(texture(iChannel0, center + vec2(stepx, stepy)));\n"
		"float right = intensity(texture(iChannel0, center + vec2(stepx, 0)));\n"
		"float bright = intensity(texture(iChannel0, center + vec2(stepx, -stepy)));\n"
		"float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;\n"
		"float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;\n"
		"return vec4(sqrt((x*x) + (y*y)));\n"
		"}\n"
		"vec4 chromatic(vec2 uv) {\n"
		"vec2 offset = vec2(iChromatic / 36., .0);\n"
		"return vec4(texture(iChannel0, uv + offset.xy).r, texture(iChannel0, uv).g, texture(iChannel0, uv + offset.yx).b, 1.0);\n"
		"}\n"
		"void main () {\n"
		"vec2 uv = gl_FragCoord.xy/iResolution.xy;\n"
		"uv.y = 1.0 - uv.y;\n"
		"vec4 t0 = texture(iChannel0, uv);\n"
		"vec4 c = vec4(0.0);\n"
		"if (iSobel > 0.03) { t0 = sobel(iSobel * 3.0 / iResolution.x, iSobel * 3.0 / iResolution.y, uv); }\n"
		"if (iChromatic > 0.0) { t0 = chromatic(uv) * t0; }\n"
		"c = t0;\n"
		"c *= iExposure;\n"
		"gl_FragColor = c; \n"
		"}\n";
	mMixFragmentShaderString = "#version 150\n"
		"uniform vec3 iResolution;\n"
		"uniform sampler2D iChannel0;\n"
		"uniform sampler2D iChannel1;\n"
		"uniform vec4      iMouse;\n"
		"uniform float     TIME;\n"
		"uniform vec3      iBackgroundColor;\n"
		"uniform vec3      iColor;\n"
		"uniform int       iSteps;\n"
		"uniform int       iFade;\n"
		"uniform int       iToggle;\n"
		"uniform int       iBlendmode;\n"
		"uniform int       iGreyScale;\n"
		"uniform int       iGlitch;\n"
		"uniform int       iTransition;\n"
		"uniform int       iRepeat;\n"
		"uniform int       iVignette;\n"
		"uniform int       iInvert;\n"
		"uniform float     iRatio;\n"

		"uniform float     iSobel;\n"

		"uniform float     iZoom;\n"
		"uniform float     iRotationSpeed;\n"
		"uniform float     iCrossfade;\n"
		"uniform float     iPixelate;\n"
		"uniform float     iAlpha;\n"
		"uniform float     iExposure;\n"
		"uniform float     iDeltaTime;\n"
		"uniform float     iTempoTime;\n"
		"uniform float     iChromatic;\n"
		"uniform float     iTrixels; // trixels if > 0.\n"
		"uniform float     iSeed;\n"
		"uniform float     iRedMultiplier;\n"
		"uniform float     iGreenMultiplier;\n"
		"uniform float     iBlueMultiplier;\n"
		"uniform float     iPixelX; // slitscan (or other) \n"
		"uniform float     iPixelY;\n"
		"uniform float     iBadTv; // badtv if > 0.01\n"
		"uniform float     iContour; // contour size if > 0.01\n"
		"uniform bool	   iFlipH;\n"
		"uniform bool	   iFlipV;\n"
		"uniform bool	   iXorY;\n"
		"uniform vec2	   iRenderXY;\n"
		"vec4 trixels( vec2 inUV, sampler2D tex )\n"
		"{ vec4 rtn;\n"
		"float height = iResolution.x/(1.01 - iTrixels)/90.0;\n"
		"float halfHeight = height*0.5;\n"
		"float halfBase = height/sqrt(3.0);\n"
		"float base = halfBase*2.0;\n"
		"float screenX = gl_FragCoord.x;\n"
		"float screenY = gl_FragCoord.y; \n"
		"float upSlope = height / halfBase;\n"
		"float downSlope = -height / halfBase;\n"
		"float oddRow = mod(floor(screenY / height), 2.0);\n"
		"screenX -= halfBase*oddRow;\n"
		"float oddCollumn = mod(floor(screenX / halfBase), 2.0);\n"
		"float localX = mod(screenX, halfBase);\n"
		"float localY = mod(screenY, height);\n"
		"if (oddCollumn == 0.0)\n"
		"{\n"
		"if (localY >= localX*upSlope)\n"
		"{\n"
		"screenX -= halfBase;\n"
		"}\n"
		"}\n"
		"else\n"
		"{\n"
		"	if (localY <= height + localX*downSlope)\n"
		"	{\n"
		"		screenX -= halfBase;\n"
		"	}\n"
		"}\n"
		"float startX = floor(screenX / halfBase)*halfBase;\n"
		"float startY = floor(screenY / height)*height;\n"
		"vec4 blend = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"for (float x = 0.0; x < 3.0; x += 1.0)\n"
		"{\n"
		"	for (float y = 0.0; y < 3.0; y += 1.0)\n"
		"	{\n"
		"		vec2 screenPos = vec2(startX + x*halfBase, startY + y*halfHeight);\n"
		"		vec2 uv1 = screenPos / iResolution.xy;\n"
		"		blend += texture(tex, uv1);\n"
		"	}\n"
		"}\n"
		"rtn = (blend / 9.0);\n"
		"return rtn;\n"
		"}\n"
		"float glitchHash(float x)\n"
		"{\n"
		"	return fract(sin(x * 11.1753) * 192652.37862);\n"
		"}\n"
		"float glitchNse(float x)\n"
		"{\n"
		"	float fl = floor(x);\n"
		"	return mix(glitchHash(fl), glitchHash(fl + 1.0), smoothstep(0.0, 1.0, fract(x)));\n"
		"}\n"
		"vec3 greyScale(vec3 colored)\n"
		"{\n"
		"	return vec3((colored.r + colored.g + colored.b) / 3.0);\n"
		"}\n"

		"float intensity(in vec4 c){return sqrt((c.x*c.x)+(c.y*c.y)+(c.z*c.z));}\n"
		"vec4 sobel0(float stepx, float stepy, vec2 center) {\n"
		"	float tleft = intensity(texture(iChannel0,center + vec2(-stepx,stepy)));\n"
		"	float left = intensity(texture(iChannel0,center + vec2(-stepx,0)));\n"
		"	float bleft = intensity(texture(iChannel0,center + vec2(-stepx,-stepy)));\n"
		"	float top = intensity(texture(iChannel0,center + vec2(0,stepy)));\n"
		"	float bottom = intensity(texture(iChannel0,center + vec2(0,-stepy)));\n"
		"	float tright = intensity(texture(iChannel0,center + vec2(stepx,stepy)));\n"
		"	float right = intensity(texture(iChannel0,center + vec2(stepx,0)));\n"
		"	float bright = intensity(texture(iChannel0,center + vec2(stepx,-stepy)));\n"
		"	float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;\n"
		"	float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;"
		"	return vec4(sqrt((x*x) + (y*y)));\n"
		"}\n"
		"vec4 sobel1(float stepx, float stepy, vec2 center) {\n"
		"	float tleft = intensity(texture(iChannel1,center + vec2(-stepx,stepy)));\n"
		"	float left = intensity(texture(iChannel1,center + vec2(-stepx,0)));\n"
		"	float bleft = intensity(texture(iChannel1,center + vec2(-stepx,-stepy)));\n"
		"	float top = intensity(texture(iChannel1,center + vec2(0,stepy)));\n"
		"	float bottom = intensity(texture(iChannel1,center + vec2(0,-stepy)));\n"
		"	float tright = intensity(texture(iChannel1,center + vec2(stepx,stepy)));\n"
		"	float right = intensity(texture(iChannel1,center + vec2(stepx,0)));\n"
		"	float bright = intensity(texture(iChannel1,center + vec2(stepx,-stepy)));\n"
		"	float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;\n"
		"	float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;"
		"	return vec4(sqrt((x*x) + (y*y)));\n"
		"}\n"

		"// left main lines begin\n"
		"vec3 shaderLeft(vec2 uv)\n"
		"{\n"
		"	vec4 left = texture(iChannel0, uv);\n"
		"	// chromatic aberration\n"
		"	if (iChromatic > 0.0)\n"
		"	{\n"
		"		vec2 offset = vec2(iChromatic / 50., .0);\n"
		"		left.r = texture(iChannel0, uv + offset.xy).r;\n"
		"		left.g = texture(iChannel0, uv).g;\n"
		"		left.b = texture(iChannel0, uv + offset.yx).b;\n"
		"	}\n"
		"	// sobel\n"
		"	if (iSobel > 0.03) { left = sobel0(iSobel * 3.0 /iResolution.x, iSobel * 3.0 /iResolution.y, uv); }\n"

		"	// Trixels\n"
		"	if (iTrixels > 0.0)\n"
		"	{\n"
		"		left = trixels(uv, iChannel0);\n"
		"	}\n"
		"	return vec3(left.r, left.g, left.b);\n"
		"}\n"
		"// left main lines end\n"
		"// right main lines begin\n"
		"vec3 shaderRight(vec2 uv)\n"
		"{ \n"
		"	vec4 right = texture(iChannel1, uv);\n"
		"	// chromatic aberation\n"
		"	if (iChromatic > 0.0)\n"
		"	{\n"
		"		vec2 offset = vec2(iChromatic / 50., .0);\n"
		"		right.r = texture(iChannel1, uv + offset.xy).r;\n"
		"		right.g = texture(iChannel1, uv).g;\n"
		"		right.b = texture(iChannel1, uv + offset.yx).b;\n"
		"	}\n"
		"	// sobel\n"
		"	if (iSobel > 0.03) { right = sobel1(iSobel * 3.0 /iResolution.x, iSobel * 3.0 /iResolution.y, uv); }\n"

		"	// Trixels\n"
		"	if (iTrixels > 0.0)\n"
		"	{\n"
		"		right = trixels(uv, iChannel1);\n"
		"	}\n"
		"	return vec3(right.r, right.g, right.b);\n"
		"}\n"
		"// Blend functions begin\n"
		"vec3 multiply(vec3 s, vec3 d)\n"
		"{\n"
		"	return s*d;\n"
		"}\n"
		"vec3 colorBurn(vec3 s, vec3 d)\n"
		"{\n"
		"	return 1.0 - (1.0 - d) / s;\n"
		"}\n"
		"vec3 linearBurn(vec3 s, vec3 d)\n"
		"{\n"
		"	return s + d - 1.0;\n"
		"}\n"
		"vec3 darkerColor(vec3 s, vec3 d)\n"
		"{\n"
		"	return (s.x + s.y + s.z < d.x + d.y + d.z) ? s : d;\n"
		"}\n"
		"vec3 lighten(vec3 s, vec3 d)\n"
		"{\n"
		"	return max(s, d);\n"
		"}\n"
		"vec3 darken(vec3 s, vec3 d)\n"
		"{\n"
		"	return min(s, d);\n"
		"}\n"
		"vec3 screen(vec3 s, vec3 d)\n"
		"{\n"
		"	return s + d - s * d;\n"
		"}\n"

		"vec3 colorDodge(vec3 s, vec3 d)\n"
		"{\n"
		"	return d / (1.0 - s);\n"
		"}\n"

		"vec3 linearDodge(vec3 s, vec3 d)\n"
		"{\n"
		"	return s + d;\n"
		"}\n"

		"vec3 lighterColor(vec3 s, vec3 d)\n"
		"{\n"
		"	return (s.x + s.y + s.z > d.x + d.y + d.z) ? s : d;\n"
		"}\n"

		"float overlay(float s, float d)\n"
		"{\n"
		"	return (d < 0.5) ? 2.0 * s * d : 1.0 - 2.0 * (1.0 - s) * (1.0 - d);\n"
		"}\n"

		"vec3 overlay(vec3 s, vec3 d)\n"
		"{\n"
		"	vec3 c;\n"
		"	c.x = overlay(s.x, d.x);\n"
		"	c.y = overlay(s.y, d.y);\n"
		"	c.z = overlay(s.z, d.z);\n"
		"	return c;\n"
		"}\n"

		"float softLight(float s, float d)\n"
		"{\n"
		"	return (s < 0.5) ? d - (1.0 - 2.0 * s) * d * (1.0 - d)\n"
		"		: (d < 0.25) ? d + (2.0 * s - 1.0) * d * ((16.0 * d - 12.0) * d + 3.0)\n"
		"		: d + (2.0 * s - 1.0) * (sqrt(d) - d);\n"
		"}\n"

		"vec3 softLight(vec3 s, vec3 d)\n"
		"{\n"
		"	vec3 c;\n"
		"	c.x = softLight(s.x, d.x);\n"
		"	c.y = softLight(s.y, d.y);\n"
		"	c.z = softLight(s.z, d.z);\n"
		"	return c;\n"
		"}\n"

		"float hardLight(float s, float d)\n"
		"{\n"
		"	return (s < 0.5) ? 2.0 * s * d : 1.0 - 2.0 * (1.0 - s) * (1.0 - d);\n"
		"}\n"

		"vec3 hardLight(vec3 s, vec3 d)\n"
		"{\n"
		"	vec3 c;\n"
		"	c.x = hardLight(s.x, d.x);\n"
		"	c.y = hardLight(s.y, d.y);\n"
		"	c.z = hardLight(s.z, d.z);\n"
		"	return c;\n"
		"}\n"

		"float vividLight(float s, float d)\n"
		"{\n"
		"	return (s < 0.5) ? 1.0 - (1.0 - d) / (2.0 * s) : d / (2.0 * (1.0 - s));\n"
		"}\n"

		"vec3 vividLight(vec3 s, vec3 d)\n"
		"{\n"
		"	vec3 c;\n"
		"	c.x = vividLight(s.x, d.x);\n"
		"	c.y = vividLight(s.y, d.y);\n"
		"	c.z = vividLight(s.z, d.z);\n"
		"	return c;\n"
		"}\n"

		"vec3 linearLight(vec3 s, vec3 d)\n"
		"{\n"
		"	return 2.0 * s + d - 1.0;\n"
		"}\n"

		"float pinLight(float s, float d)\n"
		"{\n"
		"	return (2.0 * s - 1.0 > d) ? 2.0 * s - 1.0 : (s < 0.5 * d) ? 2.0 * s : d;\n"
		"}\n"

		"vec3 pinLight(vec3 s, vec3 d)\n"
		"{\n"
		"	vec3 c;\n"
		"	c.x = pinLight(s.x, d.x);\n"
		"	c.y = pinLight(s.y, d.y);\n"
		"	c.z = pinLight(s.z, d.z);\n"
		"	return c;\n"
		"}\n"

		"vec3 hardMix(vec3 s, vec3 d)\n"
		"{\n"
		"	return floor(s + d);\n"
		"}\n"

		"vec3 difference(vec3 s, vec3 d)\n"
		"{\n"
		"	return abs(d - s);\n"
		"}\n"

		"vec3 exclusion(vec3 s, vec3 d)\n"
		"{\n"
		"	return s + d - 2.0 * s * d;\n"
		"}\n"

		"vec3 subtract(vec3 s, vec3 d)\n"
		"{\n"
		"	return s - d;\n"
		"}\n"

		"vec3 divide(vec3 s, vec3 d)\n"
		"{\n"
		"	return s / d;\n"
		"}\n"

		"// rgb<-->hsv functions by Sam Hocevar\n"
		"// http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl\n"
		"vec3 rgb2hsv(vec3 c)\n"
		"{\n"
		"	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n"
		"	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\n"
		"	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\n"

		"	float d = q.x - min(q.w, q.y);\n"
		"	float e = 1.0e-10;\n"
		"	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\n"
		"}\n"

		"vec3 hsv2rgb(vec3 c)\n"
		"{\n"
		"	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n"
		"	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n"
		"	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n"
		"}\n"

		"vec3 hue(vec3 s, vec3 d)\n"
		"{\n"
		"	d = rgb2hsv(d);\n"
		"	d.x = rgb2hsv(s).x;\n"
		"	return hsv2rgb(d);\n"
		"}\n"

		"vec3 color(vec3 s, vec3 d)\n"
		"{\n"
		"	s = rgb2hsv(s);\n"
		"	s.z = rgb2hsv(d).z;\n"
		"	return hsv2rgb(s);\n"
		"}\n"

		"vec3 saturation(vec3 s, vec3 d)\n"
		"{\n"
		"	d = rgb2hsv(d);\n"
		"	d.y = rgb2hsv(s).y;\n"
		"	return hsv2rgb(d);\n"
		"}\n"

		"vec3 luminosity(vec3 s, vec3 d)\n"
		"{\n"
		"	float dLum = dot(d, vec3(0.3, 0.59, 0.11));\n"
		"	float sLum = dot(s, vec3(0.3, 0.59, 0.11));\n"
		"	float lum = sLum - dLum;\n"
		"	vec3 c = d + lum;\n"
		"	float minC = min(min(c.x, c.y), c.z);\n"
		"	float maxC = max(max(c.x, c.y), c.z);\n"
		"	if (minC < 0.0) return sLum + ((c - sLum) * sLum) / (sLum - minC);\n"
		"	else if (maxC > 1.0) return sLum + ((c - sLum) * (1.0 - sLum)) / (maxC - sLum);\n"
		"	else return c;\n"
		"}\n"
		"// Blend functions end\n"

		"vec3 mainFunction(vec2 uv)\n"
		"{\n"
		"	vec3 c = vec3(0.0);\n"
		"	switch (iBlendmode)\n"
		"	{\n"
		"	case 0:\n"
		"		c = mix(shaderLeft(uv), shaderRight(uv), iCrossfade);\n"
		"		break;\n"
		"	case 1:\n"
		"		c = multiply(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 2:\n"
		"		c = colorBurn(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 3:\n"
		"		c = linearBurn(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 4:\n"
		"		c = darkerColor(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 5:\n"
		"		c = lighten(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 6:\n"
		"		c = screen(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 7:\n"
		"		c = colorDodge(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 8:\n"
		"		c = linearDodge(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 9:\n"
		"		c = lighterColor(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 10:\n"
		"		c = overlay(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 11:\n"
		"		c = softLight(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 12:\n"
		"		c = hardLight(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 13:\n"
		"		c = vividLight(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 14:\n"
		"		c = linearLight(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 15:\n"
		"		c = pinLight(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 16:\n"
		"		c = hardMix(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 17:\n"
		"		c = difference(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 18:\n"
		"		c = exclusion(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 19:\n"
		"		c = subtract(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 20:\n"
		"		c = divide(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 21:\n"
		"		c = hue(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 22:\n"
		"		c = color(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 23:\n"
		"		c = saturation(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 24:\n"
		"		c = luminosity(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 25:\n"
		"		c = darken(shaderLeft(uv), shaderRight(uv));\n"
		"		break;\n"
		"	case 26:\n"
		"		c = shaderLeft(uv);\n"
		"		break;\n"
		"	default: // in any other case.\n"
		"		c = shaderRight(uv);\n"
		"		break;\n"
		"	}\n"
		"	return c;\n"
		"}\n"
		"float BadTVResoRand(in float a, in float b) { return fract((cos(dot(vec2(a, b), vec2(12.9898, 78.233))) * 43758.5453)); }\n"
		"out vec4 oColor;\n"
		"void main(void)\n"
		"{\n"
		"vec2 uv = gl_FragCoord.xy / iResolution.xy;\n"
		"// flip horizontally\n"
		"if (iFlipH)\n"
		"{\n"
		"	uv.x = 1.0 - uv.x;\n"
		"}\n"
		"// flip vertically\n"
		"if (iFlipV)\n"
		"{\n"
		"	uv.y = 1.0 - uv.y;\n"
		"}\n"
		"// zoom centered might invert flipH and V!\n"
		"float xZ = (uv.x - 0.5)*iZoom*2.0;\n"
		"float yZ = (uv.y - 0.5)*iZoom*2.0;\n"
		"vec2 cZ = vec2(xZ, yZ);\n"

		"// slitscan\n"
		"if (iRatio < 20.0)\n"
		"{\n"
		"	float x = gl_FragCoord.x;\n"
		"	float y = gl_FragCoord.y;\n"
		"	float x2 = x;\n"
		"	float y2 = y;\n"
		"	if (iXorY)\n"
		"	{\n"
		"		float z1 = floor((x / iPixelX) + 0.5);     //((x/20.0) + 0.5)\n"
		"		x2 = x + (sin(z1 + (TIME * 2.0)) * iRatio);\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		float z2 = floor((y / iPixelY) + 0.5);     //((x/20.0) + 0.5)\n"
		"		y2 = y + (sin(z2 + (TIME * 2.0)) * iRatio);\n"
		"	}\n"
		"\n"
		"	vec2 uv2 = vec2(x2 / iResolution.x, y2 / iResolution.y);\n"
		"	uv = texture(iChannel1, uv2).rg;\n"
		"}\n"
		"// glitch\n"
		"if (iGlitch == 1)\n"
		"{\n"
		"	// glitch the point around\n"
		"	float s = iTempoTime * iRatio;//50.0;\n"
		"	float te = iTempoTime * 9.0 / 16.0;//0.25 + (iTempoTime + 0.25) / 2.0 * 128.0 / 60.0;\n"
		"	vec2 shk = (vec2(glitchNse(s), glitchNse(s + 11.0)) * 2.0 - 1.0) * exp(-5.0 * fract(te * 4.0)) * 0.1;\n"
		"	uv += shk;\n"
		"}\n"
		"// pixelate\n"
		"if (iPixelate < 1.0)\n"
		"{\n"
		"	vec2 divs = vec2(iResolution.x * iPixelate / iResolution.y*60.0, iPixelate*60.0);\n"
		"	uv = floor(uv * divs) / divs;\n"
		"}\n"
		"vec3 col;\n"
		"if (iCrossfade > 0.99)\n"
		"{\n"
		"	col = shaderRight(uv - cZ);\n"
		"}\n"
		"else\n"
		"{\n"
		"	if (iCrossfade < 0.01)\n"
		"	{\n"
		"		col = shaderLeft(uv - cZ);\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		col = mainFunction(uv - cZ);\n"
		"	}\n"
		"}\n"
		"if (iToggle == 1)\n"
		"{\n"
		"	col.rgb = col.gbr;\n"
		"}\n"
		"col *= iExposure;\n"
		"if (iInvert == 1) col = 1. - col;\n"
		"// badtv\n"
		"if (iBadTv > 0.01)\n"
		"{\n"
		"	float c = 1.;\n"
		"	if (iXorY)\n"
		"	{\n"
		"		c += iBadTv * sin(TIME * 2. + uv.y * 100. * iPixelX);\n"
		"		c += iBadTv * sin(TIME * 1. + uv.y * 80.);\n"
		"		c += iBadTv * sin(TIME * 5. + uv.y * 900. * iPixelY);\n"
		"		c += 1. * cos(TIME + uv.x);\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		c += iBadTv * sin(TIME * 2. + uv.x * 100. * iPixelX);\n"
		"		c += iBadTv * sin(TIME * 1. + uv.x * 80.);\n"
		"		c += iBadTv * sin(TIME * 5. + uv.x * 900. * iPixelY);\n"
		"		c += 1. * cos(TIME + uv.y);\n"
		"	}\n"

		"	//vignetting\n"
		"	c *= sin(uv.x*3.15);\n"
		"	c *= sin(uv.y*3.);\n"
		"	c *= .9;\n"

		"	uv += TIME;\n"

		"	float r = BadTVResoRand(uv.x, uv.y);\n"
		"	float g = BadTVResoRand(uv.x * 9., uv.y * 9.);\n"
		"	float b = BadTVResoRand(uv.x * 3., uv.y * 3.);\n"

		"	col.x *= r*c*.35;\n"
		"	col.y *= b*c*.95;\n"
		"	col.z *= g*c*.35;\n"
		"}\n"

		"// grey scale mode\n"
		"if (iGreyScale == 1)\n"
		"{\n"
		"	col = greyScale(col);\n"
		"}\n"
		"col.r *= iRedMultiplier;\n"
		"col.g *= iGreenMultiplier;\n"
		"col.b *= iBlueMultiplier;\n"

		"// contour \n"
		"if (iContour> 0.01) {\n"
		"	if (uv.y > 1.0 - iContour)\n"
		"		col = iBackgroundColor;\n"
		"	if (uv.y < iContour)\n"
		"		col = iBackgroundColor;\n"
		"	if (uv.x > 1.0 - iContour)\n"
		"		col = iBackgroundColor;\n"
		"	if (uv.x < iContour)\n"
		"		col = iBackgroundColor;\n"
		"}\n"
		"oColor = iAlpha * vec4(col, 1.0);\n"
		"}\n";
#pragma endregion shaderStrings

}
std::string VDShader::getDefaultVextexShaderString() { return mDefaultVextexShaderString; };
std::string VDShader::getDefaultFragmentShaderString() { return mDefaultFragmentShaderString; };
std::string VDShader::getMixFragmentShaderString() { return mMixFragmentShaderString; };
std::string VDShader::getHydraFragmentShaderString() { return mHydraFragmentShaderString; };
std::string VDShader::getMixetteFragmentShaderString() { return mMixetteFragmentShaderString; };
std::string VDShader::getPostFragmentShaderString() { return mPostFragmentShaderString; };
bool VDShader::loadFragmentStringFromFile() {
	mValid = false;
	// load fragment shader
	mFileNameWithExtension = mFragFilePath.filename().string();
	CI_LOG_V("loadFragmentStringFromFile, loading " + mFileNameWithExtension);
	//mName = mFragFile.filename().string();
	// get filename without extension
	//int dotIndex = fileName.find_last_of(".");
	//
	//if (dotIndex != std::string::npos) {
	//	mName = fileName.substr(0, dotIndex);
	//}
	//else {
	//	mName = fileName;
	//}

	mFragmentShaderString = loadString(loadFile(mFragFilePath));
	mValid = setFragmentString(mFragmentShaderString, mFileNameWithExtension);

	CI_LOG_V(mFragFilePath.string() + " loaded and compiled");
	return mValid;
}// aName = fullpath
bool VDShader::setFragmentString(const std::string& aFragmentShaderString, const std::string& aName) {

	std::string mOriginalFragmentString = aFragmentShaderString;
	std::string mOutputFragmentString = aFragmentShaderString;
	std::string mISFString = aFragmentShaderString;
	std::string mOFISFString = "";
	mName = aName;
	//string fileName = "";
	std::string mCurrentUniformsString = "// active uniforms start\n";
	std::string mProcessedShaderString = "";
	ext = "";
	mError = "";

	// we would like a name without extension
	if (mName.length() == 0) {
		mName = toString((int)getElapsedSeconds()); // + ".frag" 
	}
	else {
		int dotIndex = mName.find_last_of(".");
		int slashIndex = mName.find_last_of("\\");

		if (dotIndex != std::string::npos && dotIndex > slashIndex) {
			ext = mName.substr(dotIndex + 1);
			mName = mName.substr(slashIndex + 1, dotIndex - slashIndex - 1);
		}

	}
	
	std::string mNotFoundUniformsString = "/* " + mName + "\n";
	// filename to save
	mValid = false;
	// load fragment shader
	CI_LOG_V("setFragmentString, loading" + mName);
	try
	{
		if (ext == "fs")
		{
			std::size_t foundUniform = mOriginalFragmentString.find("uniform ");
			if (foundUniform == std::string::npos) {
				CI_LOG_V("loadFragmentStringFromFile, no mUniforms found, we add from shadertoy.vd");
				mOutputFragmentString = "/* " + mName + " */\n" + shaderInclude + mOriginalFragmentString;
			}
			else {
				mOutputFragmentString = "/* " + mName + " */\n" + mOriginalFragmentString;
			}
			// try to compile a first time to get active uniforms
			mShader = gl::GlslProg::create(getDefaultVextexShaderString(), mOutputFragmentString);
			// update only if success
			mFragmentShaderString = mOutputFragmentString;
			mValid = true;
			mVDSettings->mMsg = mName + " compiled(fs)\n" + mVDSettings->mMsg.substr(0, mVDSettings->mMsgLength);

		}
		else {
			// from Bonzomatic
			std::regex pattern{ "fGlobalTime" };
			std::string replacement{ "TIME" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "uniform" };
			replacement = { "//" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "v2Resolution" };
			replacement = { "RENDERSIZE" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "#version 410 core" };
			replacement = { "// from Bonzomatic" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "layout" };// ( don't work
			replacement = { "// from Bonzomatic 1" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "out_color" };
			replacement = { "fragColor" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			// save in assetspath
			/*fileName = aName + ".fs";
			fs::path fsFile = getAssetPath("") / mVDSettings->mAssetsPath / fileName;

			ofstream mFS(fsFile.string(), std::ofstream::binary);
			mFS << mOriginalFragmentString;
			mFS.close();
			CI_LOG_V("ISF file saved:" + fsFile.string());*/
			// from Hydra
			pattern = { "time" };
			replacement = { "TIME" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "uniform vec2 resolution;" };
			replacement = { "uniform vec3 iResolution ;" }; //keep the space
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "resolution" };
			replacement = { "iResolution" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "varying vec2 uv;" };
			replacement = { "// from Hydra" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			/*uniform float time;
			uniform vec2 resolution;
			varying vec2 uv;*/

			//CI_LOG_V("before regex " + mOriginalFragmentString);
					// shadertoy: 
			// change void mainImage( out vec4 fragColor, in vec2 fragCoord ) to void main(void)
			pattern = { "mainImage" };
			replacement = { "main" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "out vec4 fragColor," };
			replacement = { "void" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "in vec2 fragCoord" };
			replacement = { "" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { " vec2 fragCoord" };
			replacement = { "" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			// html glslEditor:
			// change vec2 u_resolution to vec3 iResolution
			pattern = { "2 u_r" };
			replacement = { "3 iR" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "u_r" };
			replacement = { "iR" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "u_tex" };
			replacement = { "iChannel" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "2 u_mouse" };
			replacement = { "4 iMouse" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "u_m" };
			replacement = { "iM" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "u_time" };
			replacement = { "TIME" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "u_" };
			replacement = { "i" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "gl_TexCoord[0].st" };
			replacement = { "gl_FragCoord.xy/iResolution.xy" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "iAudio0" };
			replacement = { "iChannel0" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			// 20190727 TODO CHECK
			/*pattern = { "iFreq0" };
			replacement = { "iChannel0.x" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "iFreq1" };
			replacement = { "iChannel0.y" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "iFreq2" };
			replacement = { "iChannel0.x" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement); */
			pattern = { "iRenderXY.x" };
			replacement = { "0.0" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			pattern = { "iRenderXY.y" };
			replacement = { "0.0" };
			mOriginalFragmentString = std::regex_replace(mOriginalFragmentString, pattern, replacement);
			// ISF file format
			mISFString = mOriginalFragmentString;
			std::regex ISFPattern{ "iResolution" };
			std::string ISFReplacement{ "RENDERSIZE" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);

			ISFPattern = { "texture2D" };
			//ISFReplacement = { "IMG_THIS_PIXEL" };
			ISFReplacement = { "IMG_NORM_PIXEL" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);
			ISFPattern = { "texture" };
			//ISFReplacement = { "IMG_THIS_PIXEL" }; // CHECK
			ISFReplacement = { "IMG_NORM_PIXEL" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);

			/* 20200312
			ISFPattern = { "iChannel0" };
			ISFReplacement = { "inputImage" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);
			*/

			mOFISFString = mISFString;

			ISFPattern = { "out vec4 fragColor," };
			ISFReplacement = { "void" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);
			ISFPattern = { "in vec2 fragCoord" };
			ISFReplacement = { "" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);

			// 20200228
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);
			ISFPattern = { "gl_FragColor" };
			ISFReplacement = { "fragColor" };
			mISFString = std::regex_replace(mISFString, ISFPattern, ISFReplacement);

			// check if uniforms were declared in the file
			std::size_t foundUniform = mOriginalFragmentString.find("uniform ");
			if (foundUniform == std::string::npos) {
				CI_LOG_V("loadFragmentStringFromFile, no uniforms found, we add from shadertoy.vd");
				//aFragmentShaderString = "/* " + aName + " */\n" + shaderInclude + mOriginalFragmentString;
				mOutputFragmentString = "/* " + mName + " */\n" + shaderInclude + mISFString;
			}
			else {
				//aFragmentShaderString = "/* " + aName + " */\n" + mOriginalFragmentString;
				mOutputFragmentString = "/* " + mName + " */\n" + mISFString;
			}

			// try to compile a first time to get active uniforms
			mShader = gl::GlslProg::create(getDefaultVextexShaderString(), mOutputFragmentString);
			// update only if success
			mFragmentShaderString = mOutputFragmentString;
			mVDSettings->mMsg = mName + " compiled(shader)\n" + mVDSettings->mMsg.substr(0, mVDSettings->mMsgLength);

			// name of the shader
			//mName = aName;
			mValid = true;
			std::string mISFUniforms = ",\n"
				"		{\n"
				"			\"NAME\": \"iColor\", \n"
				"			\"TYPE\" : \"color\", \n"
				"			\"DEFAULT\" : [\n"
				"				0.9, \n"
				"				0.6, \n"
				"				0.0, \n"
				"				1.0\n"
				"			]\n"
				"		}\n";
			auto &uniforms = mShader->getActiveUniforms();
			std::string uniformName;
			for (const auto &uniform : uniforms) {
				uniformName = uniform.getName();
				CI_LOG_V(mName + ", uniform name:" + uniformName);
				// if uniform is handled
				if (mVDAnimation->isExistingUniform(uniformName)) {
					int uniformType = mVDAnimation->getUniformTypeByName(uniformName);
					switch (uniformType)
					{
					case GL_FLOAT:
						// float  5126 GL_FLOAT 0x1406
						mShader->uniform(uniformName, mVDAnimation->getUniformValueByName(uniformName));
						mCurrentUniformsString += "uniform float " + uniformName + "; // " + toString(mVDAnimation->getUniformValueByName(uniformName)) + "\n";
						if (uniformName != "TIME") {
							mISFUniforms += ",\n"
								"		{\n"
								"			\"NAME\": \"" + uniformName + "\", \n"
								"			\"TYPE\" : \"float\", \n"
								"			\"MIN\" : " + toString(mVDAnimation->getMinUniformValueByName(uniformName)) + ",\n"
								"			\"MAX\" : " + toString(mVDAnimation->getMaxUniformValueByName(uniformName)) + ",\n"
								"			\"DEFAULT\" : " + toString(mVDAnimation->getUniformValueByName(uniformName)) + "\n"
								"		}\n";
						}
						break;
					case GL_SAMPLER_2D:
						// sampler2d 35678 GL_SAMPLER_2D 0x8B5E
						mShader->uniform(uniformName, mVDAnimation->getSampler2DUniformValueByName(uniformName));
						mCurrentUniformsString += "uniform sampler2D " + uniformName + "; // " + toString(mVDAnimation->getSampler2DUniformValueByName(uniformName)) + "\n";
						break;
					case GL_FLOAT_VEC2:
						// vec2 35664 IRESOLUTION IRESOLUTIONX IRESOLUTIONY
						mShader->uniform(uniformName, mVDAnimation->getVec2UniformValueByName(uniformName));
						//mShader->uniform(uniformName + "x", mVDAnimation->getUniformValueByName(uniformName + "x"),
						//	mVDAnimation->getUniformValueByName(uniformName + "y")));
						mCurrentUniformsString += "uniform vec2 " + uniformName + "; // " + toString(mVDAnimation->getVec2UniformValueByName(uniformName)) + "\n";
						break;
					case GL_FLOAT_VEC3:
						// vec3 35665
						mShader->uniform(uniformName, mVDAnimation->getVec3UniformValueByName(uniformName));
						mCurrentUniformsString += "uniform vec3 " + uniformName + "; // " + toString(mVDAnimation->getVec3UniformValueByName(uniformName)) + "\n";
						break;
					case GL_FLOAT_VEC4:
						// vec4 35666 GL_FLOAT_VEC4
						mShader->uniform(uniformName, mVDAnimation->getVec4UniformValueByName(uniformName));
						mCurrentUniformsString += "uniform vec4 " + uniformName + "; // " + toString(mVDAnimation->getVec4UniformValueByName(uniformName)) + "\n";
						break;
					case GL_INT:
						// int 5124 GL_INT 0x1404
						mShader->uniform(uniformName, mVDAnimation->getIntUniformValueByName(uniformName));
						mCurrentUniformsString += "uniform int " + uniformName + "; // " + toString(mVDAnimation->getIntUniformValueByName(uniformName)) + "\n";
						break;
					case GL_BOOL:
						// boolean 35670 GL_BOOL 0x8B56
						mShader->uniform(uniformName, mVDAnimation->getBoolUniformValueByName(uniformName));
						mCurrentUniformsString += "uniform bool " + uniformName + "; // " + toString(mVDAnimation->getBoolUniformValueByName(uniformName)) + "\n";
						break;
					default:
						break;
					}
				}
				else {
					if (uniformName != "ciModelViewProjection") {
						mNotFoundUniformsString += "not found " + uniformName + "\n";
					}
				}
			}

			// save ISF
			std::string mISFHeader = "/*{\n"
				"	\"CREDIT\" : \"" + mName + " by \",\n"
				"	\"CATEGORIES\" : [\n"
				"		\"ci\"\n"
				"	],\n"
				"	\"DESCRIPTION\": \"\",\n"
				"	\"INPUTS\": [\n"
				"		{\n"
				"			\"NAME\": \"inputImage\",\n"
				"			\"TYPE\" : \"image\"\n"
				"		},\n"
				/*"		{\n"
				"			\"NAME\": \"iZoom\",\n"
				"			\"TYPE\" : \"float\",\n"
				"			\"MIN\" : 0.0,\n"
				"			\"MAX\" : 1.0,\n"
				"			\"DEFAULT\" : 1.0\n"
				"		},\n"*/
				"		{\n"
				"			\"NAME\": \"iSteps\",\n"
				"			\"TYPE\" : \"float\",\n"
				"			\"MIN\" : 2.0,\n"
				"			\"MAX\" : 75.0,\n"
				"			\"DEFAULT\" : 19.0\n"
				"		},\n"
				"		{\n"
				"			\"NAME\" :\"iMouse\",\n"
				"			\"TYPE\" : \"point2D\",\n"
				"			\"DEFAULT\" : [0.0, 0.0],\n"
				"			\"MAX\" : [640.0, 480.0],\n"
				"			\"MIN\" : [0.0, 0.0]\n"
				"		}\n";


			std::string mISFFooter = "	],\n"
				"}\n"
				"*/\n";


			mISFString = mISFHeader + mISFUniforms + mISFFooter + mISFString;
			mOFISFString = mISFHeader + mOFISFString;

			// ifs for openFrameworks ISFGif project
			//fileName = aName + ".fs";
			mFileNameWithExtension = mName + ".fs";
			mFragFilePath = getAssetPath("") / mVDSettings->mAssetsPath / mFileNameWithExtension;
			/*fs::path OFIsfFile = getAssetPath("") / "glsl" / "osf" / mFileNameWithExtension;
			ofstream mOFISF(OFIsfFile.string(), std::ofstream::binary);
			mOFISF << mOFISFString;
			mOFISF.close();
			CI_LOG_V("OF ISF file saved:" + OFIsfFile.string());*/

			// ifs for HeavyM, save in assetspath if not exists already
			//fileName = aName + ".fs";
			//fs::path isfFile = getAssetPath("") / "glsl" / "isf" / fileName;
			//fs::path isfFile = getAssetPath("") / mVDSettings->mAssetsPath / mFileNameWithExtension;
			/*if (fs::exists(isfFile)) {
				isfFile = getAssetPath("") / "glsl" / "isf" / fileName;
				ofstream mISF(isfFile.string(), std::ofstream::binary);
				mISF << mISFString;
				mISF.close();
				CI_LOG_V("ISF file saved:" + isfFile.string());
			}
			else {*/
			std::ofstream mISF(mFragFilePath.string(), std::ofstream::binary);
			mISF << mISFString;
			mISF.close();
			CI_LOG_V("ISF file saved:" + mFragFilePath.string());
			//}
		}
	}
	catch (gl::GlslProgCompileExc &exc)
	{
		mError = mName + std::string(exc.what());
		CI_LOG_V("setFragmentString, unable to compile live fragment shader:" + mError + " frag:" + mOutputFragmentString);
	}
	catch (const std::exception &e)
	{
		mError = mName + std::string(e.what());
		CI_LOG_V("setFragmentString, error on live fragment shader:" + mError + " frag:" + mOutputFragmentString);
	}
	if (mError.length() > 0) mVDSettings->mShaderMsg = mError + "\n" + mVDSettings->mShaderMsg.substr(0, mVDSettings->mMsgLength);
	return mValid;
}
ci::gl::Texture2dRef VDShader::getFboTexture() {

	if (mValid) {

		gl::ScopedFramebuffer fbScp(mThumbFbo);
		gl::clear(Color::black());

		if (mTexture) mTexture->bind(254);
		std::string name;

		mUniforms = mShader->getActiveUniforms();
		for (const auto &uniform : mUniforms) {
			name = uniform.getName();
			if (mVDAnimation->isExistingUniform(name)) {
				int uniformType = mVDAnimation->getUniformTypeByName(name);
				switch (uniformType)
				{
				case 0: // float
					mShader->uniform(name, mVDAnimation->getUniformValueByName(name));
					if (name == "TIME") {
						// globally
						mShader->uniform(name, mVDAnimation->getUniformValueByName("TIME"));
					}
					break;
				case 1: // sampler2D
					mShader->uniform(name, 254);
					break;
				case 2: // vec2
					if (name == "RENDERSIZE") {
						mShader->uniform(name, vec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
					}
					else {
						mShader->uniform(name, mVDAnimation->getVec2UniformValueByName(name));
					}
					break;
				case 3: // vec3
					mShader->uniform(name, mVDAnimation->getVec3UniformValueByName(name));
					break;
				case 4: // vec4
					mShader->uniform(name, mVDAnimation->getVec4UniformValueByName(name));
					break;
				case 5: // int
					mShader->uniform(name, mVDAnimation->getIntUniformValueByName(name));
					break;
				case 6: // bool
					mShader->uniform(name, mVDAnimation->getBoolUniformValueByName(name));
					break;
				default:
					break;
				}
			}
			else {
				if (name != "ciModelViewProjection") {//type 35676
					//mVDSettings->mNewMsg = true;
					mError = "uniform not found " + name;
					mVDSettings->mErrorMsg = mError + "\n" + mVDSettings->mErrorMsg.substr(0, mVDSettings->mMsgLength);
					CI_LOG_E(mError);
				}
			}
		}
		mShader->uniform("RENDERSIZE", vec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
		mShader->uniform("TIME", (float)getElapsedSeconds());// mVDAnimation->getUniformValue(0));

		gl::ScopedGlslProg glslScope(mShader);
		// TODO: test gl::ScopedViewport sVp(0, 0, mFbo->getWidth(), mFbo->getHeight());	

		gl::drawSolidRect(Rectf(0, 0, mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
		mRenderedTexture = mThumbFbo->getColorTexture();

		std::string filename = mName + ".jpg";
		fs::path fr = getAssetPath("") / "thumbs" / filename;

		//if (!fs::exists(fr)) {
		CI_LOG_V(fr.string() + " does not exist, creating");
		Surface s8(mRenderedTexture->createSource());
		writeImage(writeFile(fr), s8);
		//}

	}
	return mRenderedTexture;
}
ci::gl::Texture2dRef VDShader::getThumbTexture() {
	if (mValid) {
		getFboTexture();
	}
	return mRenderedTexture;
}

#pragma warning(pop) // _CRT_SECURE_NO_WARNINGS
