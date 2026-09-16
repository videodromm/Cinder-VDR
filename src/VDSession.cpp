//
//  VDsession.cpp
//

#include "jsoncpp/json.h"
#include "VDSession.h"

using namespace videodromm;

namespace {
	// cinder::log::Entry's destructor (implicitly noexcept, see Cinder/src/cinder/Log.cpp) writes
	// to every registered logger, including the always-on Windows Event Log logger, which runs
	// codecvt_utf8_utf16::from_bytes() on the text unconditionally and throws std::range_error if
	// it isn't valid UTF-8. That throw escapes the noexcept destructor straight into
	// std::terminate(), bypassing any try/catch around the CI_LOG_E(...) call that triggered it -
	// asio::error_code::message() returns OS error text in the system's ANSI codepage (e.g. on a
	// French Windows install, common socket errors come back with accented characters), which is
	// not valid UTF-8, so it must never be logged as-is. Strip anything non-ASCII before it can
	// reach CI_LOG_E.
	std::string sanitizeForLog(const std::string &in) {
		std::string out;
		out.reserve(in.size());
		for (unsigned char c : in) {
			out += (c < 0x80) ? static_cast<char>(c) : '?';
		}
		return out;
	}
}

VDSession::VDSession(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms, VDMixRef aVDMix)
{
	CI_LOG_V("VDSession ctor");
	{
		GLint maxTexUnits = 0, maxCombined = 0;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombined);
		CI_LOG_V("GL_MAX_TEXTURE_IMAGE_UNITS=" << maxTexUnits << " GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS=" << maxCombined);
	}
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mVDUniforms = aVDUniforms;
	// Mix
	mVDMix = aVDMix;
	// Params
	mVDParams = VDParams::create();

	// Animation
	// TODO: needed? mVDAnimation->tapTempo();
	// fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing

	fboFmt.setColorTextureFormat(fmt);
	mWarpsFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), format.depthTexture());
	mPostFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), format.depthTexture());
	mFxFbo = gl::Fbo::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), format.depthTexture());
	// 20210103 mGlslPost = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("passthrough.vs")).fragment(loadAsset("post.glsl")));
	mGlslPost = gl::GlslProg::create(gl::GlslProg::Format().vertex(mVDParams->getDefaultVertexString()).fragment(loadAsset("post.glsl")));
	mGlslFx = gl::GlslProg::create(gl::GlslProg::Format().vertex(mVDParams->getDefaultVertexString()).fragment(loadAsset("fx.glsl")));
	mWarpTexture = ci::gl::Texture::create(mVDParams->getFboWidth(), mVDParams->getFboHeight(), ci::gl::Texture::Format().loadTopDown());
	// adjust the content size of the warps

	// TODO 20200305 if (getFboRenderedTexture(0)) Warp::setSize(mWarpList, getFboRenderedTexture(0)->getSize());
	Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), mVDParams->getFboHeight())); //
	// initialize warps
	mSettings = getAssetPath("") / mVDMix->getAssetsPath() / "warps.xml";
	if (fs::exists(mSettings)) {
		// load warp settings from file if one exists
		mWarpList = Warp::readSettings(loadFile(mSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarpList.push_back(WarpPerspectiveBilinear::create());
	}
	loadFolder(mVDMix->getAssetsPath());

	// Modes
	mModesList[VDDisplayMode::FX] = "Fx";
	mModesList[VDDisplayMode::POST] = "Post";
	mModesList[VDDisplayMode::WARP] = "Warp";

	// reset no matter what, so we don't miss anything
	cmd = -1;
	mFreqWSSend = false;
	reset();

	mCurrentBlend = 0;

}
void VDSession::loadFromJsonFile(const fs::path& jsonFile) {
	if (fs::exists(jsonFile)) {
		JsonTree json(loadFile(jsonFile));
		fboFromJson(json);
	}
}
void VDSession::setupHttpClient() {

}
void VDSession::loadShaderFromHttp(const std::string& url, unsigned int aFboIndex) {
	httpsUrl = std::make_shared<http::Url>(mApiurl + url);
	makeRequest(httpsUrl, aFboIndex);
}
void VDSession::makeRequest(http::UrlRef url, unsigned int aFboIndex)
{
	auto request = std::make_shared<http::Request>(http::RequestMethod::GET, url);
	request->appendHeader(http::Connection(http::Connection::Type::CLOSE));
	request->appendHeader(http::Accept());

	auto onComplete = [&](asio::error_code ec, http::ResponseRef response) {
		// asio's io_context::poll() has no exception safety net around handler dispatch: any
		// exception escaping this lambda (bad JSON, formatting, logging, ...) reaches
		// std::terminate() and takes down the whole app, so it must never propagate out.
		try {
			//texture = ci::gl::Texture::create(loadImage(ci::DataSourceBuffer::create(response->getContent()),
			//	ImageSource::Options(), ".jpg"));
			app::console() << response->getHeaders() << std::endl;
			app::console() << "Content: " << std::endl;
			auto content = response->getContent();
			std::string jsonStr(static_cast<const char*>(content->getData()), content->getSize());
			::Json::Features features;
			features.allowComments_ = true;
			features.strictRoot_ = true;
			::Json::Reader reader(features);
			::Json::Value value;
			reader.parse(jsonStr, value, false);
			CI_LOG_I(value.toStyledString());
			int found = 0;
			auto types = value.getMemberNames();
			for (auto &typeName : types) {
				auto &typeObj = value[typeName];
				if (typeName == "title") {
					found++;
				}
				if (typeName == "content") {
					found++;
				}
			}
			if (found == 2) {
				auto &titleObj = value["title"];
				auto &contentObj = value["content"];
				// aFboIndex was previously ignored here, so loadShaderFromHttp always landed on fbo 0
				mVDMix->setFragmentShaderString(contentObj.asString(), titleObj.asString(), aFboIndex);
			}
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeRequest onComplete exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeRequest onComplete: unknown exception" << std::endl;
		}
	};
	auto onError = [](asio::error_code ec, const http::UrlRef &url, http::ResponseRef response) {
		try {
			// see sanitizeForLog's comment above: ec.message() can be non-UTF-8 localized OS text
			CI_LOG_E(sanitizeForLog(ec.message()) << " val: " << ec.value() << " Url: " << url->to_string());
			if (response) {
				app::console() << "Headers: " << std::endl;
				app::console() << response->getHeaders() << std::endl;
			}
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeRequest onError exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeRequest onError: unknown exception" << std::endl;
		}
	};

	// dispatch by protocol, not port: the folder/shader browser API runs on a non-standard
	// port (e.g. http://localhost:40088/), which port()==80/443 would silently miss
	if (url->protocol() == "https") {
		sslSession = std::make_shared<http::SslSession>(request, onComplete, onError);
		sslSession->start();
	}
	else {
		session = std::make_shared<http::Session>(request, onComplete, onError);
		session->start();
	}
}
namespace {
	// both /api/folders and /api/folders/{folder}/{extension} return a plain JSON array of
	// strings (folder names, and shader filenames with their extension, e.g. "BinarySerpents.glsl")
	void parseStringArray(const ::Json::Value& aValue, std::vector<std::string>& aOut) {
		aOut.clear();
		for (::Json::ArrayIndex i = 0; i < aValue.size(); i++) {
			if (aValue[i].isString()) {
				aOut.push_back(aValue[i].asString());
			}
		}
	}
}
void VDSession::listFolders() {
	httpsUrl = std::make_shared<http::Url>(mApiurl + "api/folders");
	makeFolderListRequest(httpsUrl);
}
void VDSession::listShaders(const std::string& aFolder, const std::string& aExtension) {
	httpsUrl = std::make_shared<http::Url>(mApiurl + "api/folders/" + aFolder + "/" + aExtension);
	makeShaderListRequest(httpsUrl);
}
void VDSession::loadShaderFromFolder(const std::string& aFolder, const std::string& aExtension, const std::string& aName) {
	// pick a slot that's not currently mixed in, so loading it doesn't cause a sudden change in the rendering
	unsigned int aFboIndex = mVDMix->findFirstZeroWeightFboIndex();
	httpsUrl = std::make_shared<http::Url>(mApiurl + "api/folders/" + aFolder + "/" + aExtension + "/" + aName);
	// aName is the listed filename (e.g. "BinarySerpents.glsl") - strip the extension for a cleaner display title
	std::string title = aName;
	std::size_t dotIndex = title.find_last_of('.');
	if (dotIndex != std::string::npos) title = title.substr(0, dotIndex);
	makeShaderContentRequest(httpsUrl, aFboIndex, title);
}
void VDSession::makeFolderListRequest(http::UrlRef url) {
	app::console() << "VDSession::makeFolderListRequest url: " << url->to_string() << std::endl;

	auto request = std::make_shared<http::Request>(http::RequestMethod::GET, url);
	request->appendHeader(http::Connection(http::Connection::Type::CLOSE));
	request->appendHeader(http::Accept());

	auto onComplete = [&](asio::error_code ec, http::ResponseRef response) {
		// see the comment in makeRequest's onComplete: never let an exception escape a
		// handler dispatched from asio::io_context::poll(), or the app terminates.
		try {
			auto content = response->getContent();
			std::string jsonStr(static_cast<const char*>(content->getData()), content->getSize());
			::Json::Features features;
			features.allowComments_ = true;
			features.strictRoot_ = false;
			::Json::Reader reader(features);
			::Json::Value value;
			reader.parse(jsonStr, value, false);
			parseStringArray(value, mFolderList);
			CI_LOG_I("listFolders: " << mFolderList.size() << " folders");
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeFolderListRequest onComplete exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeFolderListRequest onComplete: unknown exception" << std::endl;
		}
	};
	auto onError = [](asio::error_code ec, const http::UrlRef &url, http::ResponseRef response) {
		try {
			// see sanitizeForLog's comment above: ec.message() can be non-UTF-8 localized OS text
			CI_LOG_E(sanitizeForLog(ec.message()) << " val: " << ec.value() << " Url: " << url->to_string());
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeFolderListRequest onError exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeFolderListRequest onError: unknown exception" << std::endl;
		}
	};

	if (url->protocol() == "https") {
		sslSession = std::make_shared<http::SslSession>(request, onComplete, onError);
		sslSession->start();
	}
	else {
		session = std::make_shared<http::Session>(request, onComplete, onError);
		session->start();
	}
}
void VDSession::makeShaderListRequest(http::UrlRef url) {
	auto request = std::make_shared<http::Request>(http::RequestMethod::GET, url);
	request->appendHeader(http::Connection(http::Connection::Type::CLOSE));
	request->appendHeader(http::Accept());

	auto onComplete = [&](asio::error_code ec, http::ResponseRef response) {
		// see the comment in makeRequest's onComplete: never let an exception escape a
		// handler dispatched from asio::io_context::poll(), or the app terminates.
		try {
			auto content = response->getContent();
			std::string jsonStr(static_cast<const char*>(content->getData()), content->getSize());
			::Json::Features features;
			features.allowComments_ = true;
			features.strictRoot_ = false;
			::Json::Reader reader(features);
			::Json::Value value;
			reader.parse(jsonStr, value, false);
			parseStringArray(value, mShaderList);
			CI_LOG_I("listShaders: " << mShaderList.size() << " shaders");
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeShaderListRequest onComplete exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeShaderListRequest onComplete: unknown exception" << std::endl;
		}
	};
	auto onError = [](asio::error_code ec, const http::UrlRef &url, http::ResponseRef response) {
		try {
			// see sanitizeForLog's comment above: ec.message() can be non-UTF-8 localized OS text
			CI_LOG_E(sanitizeForLog(ec.message()) << " val: " << ec.value() << " Url: " << url->to_string());
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeShaderListRequest onError exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeShaderListRequest onError: unknown exception" << std::endl;
		}
	};

	if (url->protocol() == "https") {
		sslSession = std::make_shared<http::SslSession>(request, onComplete, onError);
		sslSession->start();
	}
	else {
		session = std::make_shared<http::Session>(request, onComplete, onError);
		session->start();
	}
}
void VDSession::makeShaderContentRequest(http::UrlRef url, unsigned int aFboIndex, const std::string& aName) {
	auto request = std::make_shared<http::Request>(http::RequestMethod::GET, url);
	request->appendHeader(http::Connection(http::Connection::Type::CLOSE));
	request->appendHeader(http::Accept());

	// unlike loadShaderFromHttp's endpoint (a {"title":...,"content":...} JSON wrapper), this one
	// returns the raw GLSL fragment shader source directly as the response body
	auto onComplete = [&, aFboIndex, aName](asio::error_code ec, http::ResponseRef response) {
		// see the comment in makeRequest's onComplete: never let an exception escape a
		// handler dispatched from asio::io_context::poll(), or the app terminates.
		try {
			auto content = response->getContent();
			std::string shaderSource(static_cast<const char*>(content->getData()), content->getSize());
			mVDMix->setFragmentShaderStringAtIndex(shaderSource, aName, aFboIndex);
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeShaderContentRequest onComplete exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeShaderContentRequest onComplete: unknown exception" << std::endl;
		}
	};
	auto onError = [](asio::error_code ec, const http::UrlRef &url, http::ResponseRef response) {
		try {
			// see sanitizeForLog's comment above: ec.message() can be non-UTF-8 localized OS text
			CI_LOG_E(sanitizeForLog(ec.message()) << " val: " << ec.value() << " Url: " << url->to_string());
		}
		catch (const std::exception &e) {
			app::console() << "VDSession::makeShaderContentRequest onError exception: " << e.what() << std::endl;
		}
		catch (...) {
			app::console() << "VDSession::makeShaderContentRequest onError: unknown exception" << std::endl;
		}
	};

	if (url->protocol() == "https") {
		sslSession = std::make_shared<http::SslSession>(request, onComplete, onError);
		sslSession->start();
	}
	else {
		session = std::make_shared<http::Session>(request, onComplete, onError);
		session->start();
	}
}

bool VDSession::loadFolder(const string& aFolder) {
	unsigned int f = 0;
	bool found = true;
	mVDSettings->setMsg(aFolder);
	if (aFolder != mVDMix->getAssetsPath()) {
		// find mix.json
		std::string mixFileName = "mix.json";
		fs::path mixFile = getAssetPath("") / aFolder / mixFileName;
		if (fs::exists(mixFile)) {
			mVDMix->clearFboShaderList();
			JsonTree mix(loadFile(mixFile));
			mVDMix->restore(mixFile);
		}
	}

	// find fbo...json
	while (found) {
		std::string jsonFileName = "fbo" + toString(f) + ".json";

		fs::path jsonFile = getAssetPath("") / aFolder / jsonFileName;
		if (fs::exists(jsonFile)) {
			//loadFromJsonFile(jsonFile)->createShader()->createUniforms()->compile()->createFboWhenSuccess()->addToFboList();
			JsonTree json(loadFile(jsonFile));
			fboFromJson(json, f, aFolder);
			f++;
		}
		else {
			found = false;
		}
	} //while

	return !found;
}
void VDSession::toggleUI() {
	mShowUI = !mShowUI;
};
bool VDSession::showUI() {
	return mShowUI;
};

std::string VDSession::getModeName(unsigned int aMode) {
	if (aMode > mModesList.size() - 1) aMode = mModesList.size() - 1;
	return mModesList[aMode];
}
unsigned int VDSession::getModesCount() {
	return mModesList.size();
};
void VDSession::update() {

	// fps calculated in main app
	mVDAnimation->update();

	mVDMix->getMixetteTexture(0);

	renderWarpsToFbo();
	renderPostToFbo();
	if (mVDUniforms->getUniformValue(mVDUniforms->IDISPLAYMODE) == VDDisplayMode::FX || getElapsedFrames() % 100 == 0) renderFxToFbo();
}
void VDSession::renderPostToFbo()
{
	{
		gl::ScopedFramebuffer fbScp(mPostFbo);
		// clear out the FBO with black
		gl::clear(Color::black());
		//gl::clear(ColorA(0.4f, 0.8f, 0.0f, 0.3f));

		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mPostFbo->getSize());

		// texture binding must be before ScopedGlslProg
		//mWarpsFbo->getColorTexture()
		mWarpTexture->bind(10);
		gl::ScopedGlslProg prog(mGlslPost);

		// not used yet mGlslPost->uniform("TIME", getUniformValue(mVDUniforms->ITIME) - mVDSettings->iStart);;
		mGlslPost->uniform("iResolution", vec3(mVDParams->getFboWidth(), mVDParams->getFboHeight(), 1.0));
		mGlslPost->uniform("iColor", vec3(mVDUniforms->getUniformValue(mVDUniforms->ICOLORX), mVDUniforms->getUniformValue(mVDUniforms->ICOLORY), mVDUniforms->getUniformValue(mVDUniforms->ICOLORZ)));
		mGlslPost->uniform("iChannel0", 10); // texture unit 10 (post)
		// tmp 20210102
		float iz = mVDUniforms->getUniformValue(mVDUniforms->IZOOM);
		mGlslPost->uniform("iTime", mVDUniforms->getUniformValue(mVDUniforms->ITIME));
		mGlslPost->uniform("iTempoTime", mVDUniforms->getUniformValue(mVDUniforms->ITEMPOTIME));
		mGlslPost->uniform("iRatio", mVDUniforms->getUniformValue(mVDUniforms->IRATIO));
		mGlslPost->uniform("iSobel", mVDUniforms->getUniformValue(mVDUniforms->ISOBEL));
		mGlslPost->uniform("iExposure", mVDUniforms->getUniformValue(mVDUniforms->IEXPOSURE));
		mGlslPost->uniform("iTrixels", mVDUniforms->getUniformValue(mVDUniforms->ITRIXELS)); // trixels if > 0.
		mGlslPost->uniform("iPixelate", mVDUniforms->getUniformValue(mVDUniforms->IPIXELATE)); // pixelate if < 1.
		mGlslPost->uniform("iZoom", mVDUniforms->getUniformValue(mVDUniforms->IZOOM));
		mGlslPost->uniform("iGlitch", mVDUniforms->getUniformValue(mVDUniforms->IGLITCH));
		mGlslPost->uniform("iChromatic", mVDUniforms->getUniformValue(mVDUniforms->ICHROMATIC));
		mGlslPost->uniform("iPar5", mVDUniforms->getUniformValue(mVDUniforms->IPAR5));
		mGlslPost->uniform("iFlipV", mVDUniforms->getUniformValue(mVDUniforms->IFLIPPOSTV));
		mGlslPost->uniform("iFlipH", mVDUniforms->getUniformValue(mVDUniforms->IFLIPPOSTH));
		mGlslPost->uniform("iInvert", mVDUniforms->getUniformValue(mVDUniforms->IINVERT));
		mGlslPost->uniform("iToggle", mVDUniforms->getUniformValue(mVDUniforms->ITOGGLE));
		mGlslPost->uniform("iGreyScale", mVDUniforms->getUniformValue(mVDUniforms->IGREYSCALE));
		mGlslPost->uniform("iVignette", mVDUniforms->getUniformValue(mVDUniforms->IVIGNETTE));
		mGlslPost->uniform("iAlpha", mVDUniforms->getUniformValue(mVDUniforms->IALPHA));
		mGlslPost->uniform("iRedMultiplier", mVDUniforms->getUniformValue(mVDUniforms->IFRX));
		mGlslPost->uniform("iGreenMultiplier", mVDUniforms->getUniformValue(mVDUniforms->IFGX));
		mGlslPost->uniform("iBlueMultiplier", mVDUniforms->getUniformValue(mVDUniforms->IFBX));
		mGlslPost->uniform("iRenderXYX", mVDUniforms->getUniformValue(mVDUniforms->IRENDERXYX));
		mGlslPost->uniform("iRenderXYY", mVDUniforms->getUniformValue(mVDUniforms->IRENDERXYY));
		mGlslPost->uniform("iMouse", mVDUniforms->getVec4UniformValueByName("iMouse"));
		gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
	}
}
void VDSession::renderFxToFbo()
{
	{
		gl::ScopedFramebuffer fbScp(mFxFbo);
		// clear out the FBO with black
		//gl::clear(Color::black());
		gl::clear(ColorA(0.4f, 0.8f, 0.0f, 0.3f));

		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mFxFbo->getSize());

		// texture binding must be before ScopedGlslProg
		//mWarpsFbo->getColorTexture()
		mWarpTexture->bind(11);
		gl::ScopedGlslProg prog(mGlslFx);

		mGlslFx->uniform("iResolution", vec3(mVDParams->getFboWidth(), mVDParams->getFboHeight(), 1.0));
		mGlslFx->uniform("iChannel0", 11); // texture unit 11 (fx)
		mGlslFx->uniform("iChannel1", 11); // texture audio (unused by fx.glsl; kept in range)
		mGlslFx->uniform("iTime", mVDUniforms->getUniformValue(mVDUniforms->ITIME));
		mGlslFx->uniform("iGreyScale", mVDUniforms->getUniformValue(mVDUniforms->IGREYSCALE));
		mGlslFx->uniform("iGlitch", mVDUniforms->getUniformValue(mVDUniforms->IGLITCH));
		mGlslFx->uniform("iFreq0", mVDUniforms->getUniformValue(mVDUniforms->IFREQ0));
		mGlslFx->uniform("iMouse", mVDUniforms->getVec4UniformValueByName("iMouse"));

		mGlslFx->uniform("iTempoTime", mVDUniforms->getUniformValue(mVDUniforms->ITEMPOTIME));
		mGlslFx->uniform("iRatio", mVDUniforms->getUniformValue(mVDUniforms->IRATIO));
		mGlslFx->uniform("iSobel", mVDUniforms->getUniformValue(mVDUniforms->ISOBEL));
		mGlslFx->uniform("iExposure", mVDUniforms->getUniformValue(mVDUniforms->IEXPOSURE));
		mGlslFx->uniform("iTrixels", mVDUniforms->getUniformValue(mVDUniforms->ITRIXELS)); // trixels if > 0.
		mGlslFx->uniform("iPixelate", mVDUniforms->getUniformValue(mVDUniforms->IPIXELATE)); // pixelate if < 1.
		mGlslFx->uniform("iZoom", mVDUniforms->getUniformValue(mVDUniforms->IZOOM));
		mGlslFx->uniform("iChromatic", mVDUniforms->getUniformValue(mVDUniforms->ICHROMATIC));
		mGlslFx->uniform("iFlipV", mVDUniforms->getUniformValue(mVDUniforms->IFLIPPOSTV));
		mGlslFx->uniform("iFlipH", mVDUniforms->getUniformValue(mVDUniforms->IFLIPPOSTH));
		mGlslFx->uniform("iInvert", mVDUniforms->getUniformValue(mVDUniforms->IINVERT));
		mGlslFx->uniform("iToggle", mVDUniforms->getUniformValue(mVDUniforms->ITOGGLE));
		mGlslFx->uniform("iVignette", mVDUniforms->getUniformValue(mVDUniforms->IVIGNETTE));
		mGlslFx->uniform("iRedMultiplier", mVDUniforms->getUniformValue(mVDUniforms->IFRX));
		mGlslFx->uniform("iGreenMultiplier", mVDUniforms->getUniformValue(mVDUniforms->IFGX));
		mGlslFx->uniform("iBlueMultiplier", mVDUniforms->getUniformValue(mVDUniforms->IFBX));
		mGlslFx->uniform("iRenderXYX", mVDUniforms->getUniformValue(mVDUniforms->IRENDERXYX));
		mGlslFx->uniform("iRenderXYY", mVDUniforms->getUniformValue(mVDUniforms->IRENDERXYY));

		gl::drawSolidRect(Rectf(0, 0, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
	}
}
void VDSession::renderWarpsToFbo()
{
	{
		gl::ScopedFramebuffer fbScp(mWarpsFbo);
		// clear out the FBO with black
		gl::clear(Color::black());
		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mWarpsFbo->getSize());
		// iterate over the warps and draw their content
		int i = 0;
		int a = 0;

		for (auto& warp : mWarpList) {
			a = warp->getAFboIndex();
			if (a < 0) a = 0; // TODO 20200228 a could be negative if warps3.xml > warps01.json
			i = math<int>::min(a, getFboShaderListSize() - 1);
			warp->draw(mVDMix->getRenderedMixetteTexture(0));
		}
		mWarpTexture = mWarpsFbo->getColorTexture();
	}
}


void VDSession::resetSomeParams() {
	// parameters not exposed in json file

}

void VDSession::reset()
{
	// parameters exposed in json file
	resetSomeParams();
}

void VDSession::blendRenderEnable(bool render) {
	mVDAnimation->blendRenderEnable(render);
}

void VDSession::fileDrop(FileDropEvent event) {
	std::string ext = "";
	//string fileName = "";

	// drop position (event.getX/Y) comes straight from glfwGetCursorPos - logical points, same units as
	// the VDUIParams layout constants below (unlike ImGui's own io.MousePos, this is never run through
	// toPixels()/uiScale) - so this must match VDUIFbos.cpp's xPos formula in logical units, not device pixels
	int index = (int)((event.getX() - (mVDParams->getUIMargin() + mVDParams->getUIXPosCol1())) / (mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()));
	if (index < 0) index = (int)mVDMix->getFboShaderListSize(); // dropped left of the fbo row - treat as "empty area"
	//int y = (int)(event.getY());
	//if (index < 2 || y < mVDSettings->uiYPosRow3 || y > mVDSettings->uiYPosRow3 + mVDSettings->uiPreviewH) index = 0;
	ci::fs::path mPath = event.getFile(event.getNumFiles() - 1);
	std::string absolutePath = mPath.string();
	// use the last of the dropped files
	int dotIndex = absolutePath.find_last_of(".");
	int slashIndex = absolutePath.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) {
		ext = absolutePath.substr(dotIndex + 1);
		// files commonly arrive with an uppercase extension (phones, cameras, exports) - every
		// comparison below is against a lowercase literal, so without this an "IMG.JPG"/"Video.MP4"
		// would silently match nothing at all
		for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
		//fileName = absolutePath.substr(slashIndex + 1, dotIndex - slashIndex - 1);
		if (ext == "json") {
			JsonTree json(loadFile(absolutePath));
			fboFromJson(json);
		}

		else if (ext == "glsl" || ext == "frag" || ext == "fs") {
			loadFragmentShader(absolutePath, index);
		}
		else if (ext == "png" || ext == "jpg" || ext == "mp4") {
			// don't dispatch by the (stale, purely positional) index above - VDUIFbos.cpp hit-tests
			// this against each fbo's actual current ImGui window rect once per frame, and falls
			// back to loading it standalone into the shared texture pool if it doesn't land on any of them
			mPendingTextureDrop.active = true;
			mPendingTextureDrop.path = absolutePath;
			mPendingTextureDrop.pos = ci::app::toPixels(event.getPos());
		}
		else if (ext == "wav" || ext == "mp3") {
			loadAudioFile(absolutePath);
		}
		/*
		else if (ext == "mov") {
			loadMovie(absolutePath, index);
		}

		else if (ext == "") {
			// 20211108 never called?
			// try loading image sequence from dir
			//if (!loadImageSequence(absolutePath, index)) {
				// try to load a folder of shaders
				//loadShaderFolder(absolutePath);
			//}
		}*/
	}
	else {
		// folder was dropped
		std::string folderName = absolutePath.substr(slashIndex + 1);
		loadFolder(folderName);
		// try loading image sequence from dir
		// if (!loadImageSequence(absolutePath, index)) {
			// try to load a folder of shaders
			//loadShaderFolder(absolutePath);
		//}
	}
}

/*bool VDSession::loadImageSequence(const string& aFolder, unsigned int aTextureIndex) {
	return mVDMix->loadImageSequence(aFolder, aTextureIndex);
}*/
void VDSession::loadAudioFile(const string& aFile) {
	//mTextureList[0]->loadFromFullPath(aFile);
}
#pragma region events
bool VDSession::handleMouseMove(MouseEvent& event)
{
	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleMouseDown(MouseEvent& event)
{
	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarpList, event)) {
		// let your application perform its mouseMove handling here event.getPos()
		handled = mVDMix->handleMouseDown(event);
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleMouseDrag(MouseEvent& event)
{

	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
		mVDMix->handleMouseDrag(event);
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleMouseUp(MouseEvent& event)
{
	bool handled = true;
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarpList, event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}

	event.setHandled(handled);
	return event.isHandled();
}


bool VDSession::handleKeyDown(KeyEvent& event)
{
	bool handled = true;
	float newValue;
#if defined( CINDER_COCOA )
	bool isModDown = event.isMetaDown();
#else // windows
	bool isModDown = event.isControlDown();
#endif
	bool isShiftDown = event.isShiftDown();
	bool isAltDown = event.isAltDown();
	CI_LOG_V("session keydown: " + toString(event.getCode()) + " ctrl: " + toString(isModDown) + " shift: " + toString(isShiftDown) + " alt: " + toString(isAltDown));
	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarpList, event)) {

		switch (event.getCode()) {
		case KeyEvent::KEY_s:
			// Spout UI
			mVDMix->selectSenderPanel();
			break;
		case KeyEvent::KEY_w:
			CI_LOG_V("oscConnect");
			if (isModDown) {
				//oscConnect();
			}
			else {
				// handled in main app
				//handled = false;
				// toggle warp edit mode
				Warp::enableEditMode(!Warp::isEditModeEnabled());
			}
			break;
		case KeyEvent::KEY_UP:
			// imgseq next
			//incrementSequencePosition();
			break;
		case KeyEvent::KEY_DOWN:
			// imgseq next
			//decrementSequencePosition();
			break;
		case KeyEvent::KEY_v:
			//if (isModDown) fboFlipV(0);// TODO other indexes mVDSettings->mFlipV = !mVDSettings->mFlipV; useless?
			break;
		case KeyEvent::KEY_h:
			if (isModDown) {
				//fboFlipH(0);// TODO other indexes mVDSettings->mFlipH = !mVDSettings->mFlipH; useless?
			}
			else {
				// ui visibility
				toggleUI();
			}
			break;
		case KeyEvent::KEY_d:
			/*if (isAltDown) {
				setSpeed(0, getSpeed(0) - 0.01f);
			}
			else {
				setSpeed(0, getSpeed(0) + 0.01f);
			}*/
			break;
		default:
			CI_LOG_V("session keydown: " + toString(event.getCode()));
			handled = false;
			break;
		}
	}
	CI_LOG_V((handled ? "session keydown handled " : "session keydown not handled "));
	event.setHandled(handled);
	return event.isHandled();
}

bool VDSession::handleKeyUp(KeyEvent& event) {
	bool handled = true;

	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarpList, event)) {
		if (!mVDAnimation->handleKeyUp(event)) {
			// Animation did not handle the key, so handle it here
			switch (event.getCode()) {

			default:
				CI_LOG_V("session keyup: " + toString(event.getCode()));
				handled = false;
				break;
			}
		}
	}
	CI_LOG_V((handled ? "session keyup handled " : "session keyup not handled "));
	event.setHandled(handled);
	return event.isHandled();
}
#pragma endregion events
// fbos
#pragma region fbos

#pragma endregion fbos
// shaders

ci::gl::TextureRef VDSession::getFboRenderedTexture(unsigned int aFboIndex) {
	return mVDMix->getFboRenderedTexture(aFboIndex);
}
ci::gl::TextureRef VDSession::getFboTexture(unsigned int aFboIndex) {
	return mVDMix->getFboTexture(aFboIndex);
}

ci::gl::TextureRef VDSession::getFboShaderTexture(unsigned int aFboShaderIndex) {
	return mVDMix->getFboRenderedTexture(aFboShaderIndex); //20201229 or getTexture?
};

std::vector<ci::gl::GlslProg::Uniform> VDSession::getFboShaderUniforms(unsigned int aFboShaderIndex) {
	return mVDMix->getFboShaderUniforms(aFboShaderIndex);
}

float VDSession::getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex) {
	return mVDMix->getUniformValueByLocation(aFboShaderIndex, aLocationIndex);
};
void VDSession::setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue) {
	//mFboShaderList[aFboShaderIndex]->setUniformValueByLocation(aLocationIndex, aValue);
	mVDMix->setUniformValueByLocation(aFboShaderIndex, aLocationIndex, aValue);
};
ci::gl::TextureRef VDSession::getPostFboTexture() {
	return mPostFbo->getColorTexture();
};
ci::gl::TextureRef VDSession::getFxFboTexture() {
	return mFxFbo->getColorTexture();
};
ci::gl::TextureRef VDSession::getWarpFboTexture() {
	return mWarpsFbo->getColorTexture();
};
ci::gl::TextureRef VDSession::getRenderedWarpFboTexture() {
	return mWarpTexture;
};
ci::gl::TextureRef VDSession::getMixetteTexture(unsigned int aFboIndex) {
	return mVDMix->getMixetteTexture(aFboIndex);
};
ci::gl::TextureRef VDSession::getRenderedMixetteTexture(unsigned int aFboIndex) {
	return mVDMix->getRenderedMixetteTexture(aFboIndex);
};
void VDSession::resize() {
	// tell the fbos our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarpList);
	Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), mVDParams->getFboHeight()));
}
unsigned int VDSession::getWarpCount() { return mWarpList.size(); };
std::string	 VDSession::getWarpName(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getName(); };// or trycatch
int VDSession::getWarpWidth(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getWidth(); };
int VDSession::getWarpHeight(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getHeight(); };
void VDSession::setWarpWidth(unsigned int aWarpIndex, int aWidth) {
	mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->setWidth(aWidth);
	mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->resize();
};
void VDSession::setWarpHeight(unsigned int aWarpIndex, int aHeight) {
	Warp::handleResize(mWarpList);
	Warp::setSize(mWarpList, ivec2(mVDParams->getFboWidth(), aHeight));
	mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->setHeight(aHeight);

};
unsigned int VDSession::getWarpAFboIndex(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getAFboIndex(); };
unsigned int VDSession::getWarpBFboIndex(unsigned int aWarpIndex) { return mWarpList[math<int>::min(aWarpIndex, mWarpList.size() - 1)]->getBFboIndex(); };
void VDSession::setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) {
	if (aWarpIndex < mWarpList.size() && aWarpFboIndex < mVDMix->getFboShaderListSize()) {
		mWarpList[aWarpIndex]->setAFboIndex(aWarpFboIndex);
	}
}
void VDSession::setWarpBFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) {
	if (aWarpIndex < mWarpList.size() && aWarpFboIndex < mVDMix->getFboShaderListSize()) {
		mWarpList[aWarpIndex]->setBFboIndex(aWarpFboIndex);
	}
}

void VDSession::createWarp() {
	auto warp = WarpBilinear::create();
	warp->setName("New");
	warp->setAFboIndex(0);
	warp->setBFboIndex(0);
	warp->setAShaderIndex(0);
	warp->setBShaderIndex(0);
	warp->setAShaderFilename("inputImage.fs");
	warp->setBShaderFilename("inputImage.fs");
	warp->setATextureFilename("audio");
	warp->setBTextureFilename("audio");
	mWarpList.push_back(WarpBilinear::create());
}
std::string VDSession::getFboShaderName(unsigned int aFboIndex) {
	return mVDMix->getFboShaderName(aFboIndex);
}
std::string VDSession::getFboTextureName(unsigned int aFboIndex) {
	return mVDMix->getFboTextureName(aFboIndex);
}
void VDSession::saveWarps() {
	/*int i = 0;
	for (auto &warp : mWarpList) {
		//
		warp->setAShaderFilename(getFboShaderName(warp->getAFboIndex()));
		warp->setATextureFilename(getFboTextureName(warp->getAFboIndex()));
		JsonTree		json;
		string jsonFileName = "warp" + toString(i) + ".json";
		fs::path jsonFile = getAssetPath("") / mVDSettings->mAssetsPath / jsonFileName;
		// write file
		json.pushBack(warp->toJson());
		json.write(jsonFile);
		i++;
	}
	// save warp settings*/
	Warp::writeSettings(mWarpList, writeFile(mSettings));
}
void VDSession::setAnim(unsigned int aCtrl, unsigned int aAnim) {
	mVDAnimation->setAnim(aCtrl, aAnim);
}
unsigned int VDSession::getAnim(unsigned int aCtrl) {
	return mVDAnimation->getAnim(aCtrl);
}

// control values
void VDSession::toggleValue(unsigned int aCtrl) {
	float val = mVDUniforms->getUniformValue(aCtrl);
	if (val > 0.0f) {
		mVDUniforms->setUniformValue(aCtrl, 0.0f);
	}
	else {
		mVDUniforms->setUniformValue(aCtrl, 1.0f);
	}
};

float VDSession::getMinUniformValue(unsigned int aIndex) {
	return mVDUniforms->getMinUniformValue(aIndex);
};
float VDSession::getMaxUniformValue(unsigned int aIndex) {
	return mVDUniforms->getMaxUniformValue(aIndex);
};
float VDSession::getDefaultUniformValue(unsigned int aIndex) {
	return mVDUniforms->getDefaultUniformValue(aIndex);
}
int VDSession::getSampler2DUniformValueByName(const std::string& aName) {
	return mVDUniforms->getSampler2DUniformValueByName(aName);
};
float VDSession::getUniformValueByName(const std::string& aCtrlName) {
	return mVDUniforms->getUniformValueByName(aCtrlName);
};
