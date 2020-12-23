#include "VDSessionFacade.h"

using namespace videodromm;

VDSessionFacadeRef VDSessionFacade::createVDSession(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
{
	VDMediatorObservableRef mediator =
		VDMediatorObservable::createVDMediatorObservable(aVDSettings, aVDAnimation, aVDUniforms);
	// OK ->addObserver(VDSocketIOObserver::connect(aVDSettings->mSocketIOHost, aVDSettings->mSocketIOPort))
	// OK ->addObserver(VDOscObserver::connect(aVDSettings->mOSCDestinationHost, aVDSettings->mOSCDestinationPort));
	// OK ->addObserver(VDUIObserver::connect(aVDSettings, aVDAnimation));// ->addObserver(new UIDisplay());	
	return VDSessionFacadeRef(new VDSessionFacade(VDSessionRef(new VDSession(aVDSettings, aVDAnimation, aVDUniforms)), mediator));
}
VDSessionFacadeRef VDSessionFacade::setUniformValue(unsigned int aCtrl, float aValue) {
	mVDMediator->setUniformValue(aCtrl, aValue);
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::addUIObserver(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation) {
	mVDMediator->addObserver(VDUIObserver::connect(aVDSettings, aVDAnimation));
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::setupOSCReceiver() {
	if (!mOscReceiverConnected) {
		mOscReceiverConnected = true;
		mVDMediator->setupOSCReceiver();
	}
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::setupWSClient() {
	if (!mWSClientConnected ) {
		mWSClientConnected = true;
		mVDMediator->setupWSClient();
	}
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::setupKeyboard() {
	mVDMediator->setupKeyboard();
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::addOSCObserver(const std::string& host, unsigned int port) {
	if (!mOscSenderConnected) {
		mOscSenderConnected = true;
		mVDMediator->addObserver(VDOscObserver::connect(host, port));
	}
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::addSocketIOObserver(const std::string& host, unsigned int port) {
	//mVDMediator->addObserver(VDSocketIOObserver::connect(host, port));
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::setAnim(unsigned int aCtrl, unsigned int aAnim) {
	mVDSession->setAnim(aCtrl, aAnim);
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::toggleValue(unsigned int aCtrl) {
	mVDSession->toggleValue(aCtrl);
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::tapTempo() {
	mVDSession->tapTempo();
	return shared_from_this();
};
VDSessionFacadeRef VDSessionFacade::toggleUseTimeWithTempo() {
	mVDSession->toggleUseTimeWithTempo();
	return shared_from_this();
};
VDSessionFacadeRef VDSessionFacade::useTimeWithTempo() {
	mVDSession->useTimeWithTempo();
	return shared_from_this();
};
VDSessionFacadeRef VDSessionFacade::loadFromJsonFile(const fs::path& jsonFile) {
	mVDSession->loadFromJsonFile(jsonFile);
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::setMode(int aMode) {
	mVDSession->setMode(aMode);
	return shared_from_this();
}
VDSessionFacadeRef VDSessionFacade::update() {
	mVDSession->update();
	mVDMediator->update();
	return shared_from_this();
}
// begin terminal operations
bool VDSessionFacade::getUseTimeWithTempo() {
	return mVDSession->getUseTimeWithTempo();
};
// osc
bool VDSessionFacade::isOscSenderConnected() {
	return mOscSenderConnected;
};
bool VDSessionFacade::isOscReceiverConnected() {
	return mOscReceiverConnected;
};
int VDSessionFacade::getOSCReceiverPort() {
	return mVDMediator->getOSCReceiverPort();
};
void VDSessionFacade::setOSCReceiverPort(int aReceiverPort) {
	mVDMediator->setOSCReceiverPort(aReceiverPort);
};
void VDSessionFacade::setOSCMsg(const std::string& aMsg) {
	if (mOscReceiverConnected) {
		mVDMediator->setOSCMsg(aMsg);
	}
};
std::string VDSessionFacade::getOSCMsg() {
	return (mOscReceiverConnected) ? mVDMediator->getOSCMsg() : "";
};

// Websockets begin
bool					VDSessionFacade::isWSClientConnected() {
	return mWSClientConnected;
};
int						VDSessionFacade::getWSClientPort() {
	return mVDMediator->getWSClientPort();
};
void VDSessionFacade::wsPing() {
	mVDMediator->wsPing();
}
void VDSessionFacade::setWSClientPort(int aPort) {
	mVDMediator->setWSClientPort(aPort);
};
VDSessionFacadeRef VDSessionFacade::wsConnect() {
	mVDMediator->wsConnect();
	return shared_from_this();
}
void					VDSessionFacade::setWSMsg(const std::string& aMsg) {
	if (mWSClientConnected) {
		mVDMediator->setWSMsg(aMsg);
	}
};

std::string VDSessionFacade::getWSMsg() {
	return (mWSClientConnected) ? mVDMediator->getWSMsg() : "";
};
// Websockets end

ci::gl::TextureRef VDSessionFacade::buildRenderedMixetteTexture(unsigned int aIndex) {
	return mVDSession->getRenderedMixetteTexture(aIndex);
}
ci::gl::TextureRef VDSessionFacade::buildFboTexture(unsigned int aIndex) {
	return mVDSession->getFboTexture(aIndex);;
}
ci::gl::TextureRef VDSessionFacade::getFboShaderTexture(unsigned int aIndex) {
	return mVDMediator->getFboShaderTexture(aIndex);
}
std::string VDSessionFacade::getFboShaderName(unsigned int aIndex) {
	return mVDMediator->getFboShaderName(aIndex);
}
unsigned int VDSessionFacade::getFboShadersCount() {
	return mVDMediator->getFboShadersCount();
}
std::vector<ci::gl::GlslProg::Uniform> VDSessionFacade::getFboShaderUniforms(unsigned int aFboShaderIndex) {
	return mVDMediator->getFboShaderUniforms(aFboShaderIndex);
}

int VDSessionFacade::getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex) {
	return mVDMediator->getUniformValueByLocation(aFboShaderIndex, aLocationIndex);
};
void VDSessionFacade::setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue) {
	mVDMediator->setUniformValueByLocation(aFboShaderIndex, aLocationIndex, aValue);
};

ci::gl::TextureRef VDSessionFacade::buildFboRenderedTexture(unsigned int aFboIndex) {
	return mVDSession->getFboRenderedTexture(aFboIndex);
}
ci::gl::TextureRef VDSessionFacade::buildPostFboTexture() {
	return mVDSession->getPostFboTexture();
}
ci::gl::TextureRef VDSessionFacade::buildWarpFboTexture() {
	return mVDSession->getWarpFboTexture();
}
ci::gl::TextureRef VDSessionFacade::buildRenderedWarpFboTexture() {
	return mVDSession->getRenderedWarpFboTexture();
}
unsigned int VDSessionFacade::getWarpAFboIndex(unsigned int aWarpIndex) {
	return mVDSession->getWarpAFboIndex(aWarpIndex);
}
unsigned int VDSessionFacade::getWarpBFboIndex(unsigned int aWarpIndex) {
	return mVDSession->getWarpBFboIndex(aWarpIndex);
}
float VDSessionFacade::getMinUniformValue(unsigned int aIndex) {
	return mVDSession->getMinUniformValue(aIndex);
}
float VDSessionFacade::getMaxUniformValue(unsigned int aIndex) {
	return mVDSession->getMinUniformValue(aIndex);
}
int VDSessionFacade::getFboTextureWidth(unsigned int aFboIndex) {
	return mVDSession->getFboTextureWidth(aFboIndex);
};
int VDSessionFacade::getFboTextureHeight(unsigned int aFboIndex) {
	return mVDSession->getFboTextureHeight(aFboIndex);
}
void VDSessionFacade::createWarp() {
	mVDSession->createWarp();
}
unsigned int VDSessionFacade::getWarpCount() {
	return mVDSession->getWarpCount();
};
std::string VDSessionFacade::getWarpName(unsigned int aWarpIndex) {
	return mVDSession->getWarpName(aWarpIndex);
}// or trycatch
int VDSessionFacade::getWarpWidth(unsigned int aWarpIndex) {
	return mVDSession->getWarpWidth(aWarpIndex);
}
int VDSessionFacade::getWarpHeight(unsigned int aWarpIndex) {
	return mVDSession->getWarpHeight(aWarpIndex);
}
unsigned int VDSessionFacade::getFboListSize() {
	return mVDSession->getFboListSize();
}
std::string VDSessionFacade::getFboInputTextureName(unsigned int aFboIndex) {
	return mVDSession->getFboInputTextureName(aFboIndex);
}
ci::gl::Texture2dRef VDSessionFacade::getFboInputTexture(unsigned int aFboIndex) {
	return mVDSession->getFboInputTexture(aFboIndex);
}
std::string VDSessionFacade::getFboName(unsigned int aFboIndex) {
	return mVDSession->getFboName(aFboIndex);
}
int VDSessionFacade::getFFTWindowSize() {
	return mVDSession->getFFTWindowSize();
}
float* VDSessionFacade::getFreqs() {
	return mVDSession->getFreqs();
}
std::vector<ci::gl::GlslProg::Uniform> VDSessionFacade::getUniforms(unsigned int aFboIndex) {
	return mVDSession->getUniforms(aFboIndex);
}
ci::gl::Texture2dRef VDSessionFacade::buildFboInputTexture(unsigned int aFboIndex) {
	return mVDSession->getFboInputTexture(aFboIndex);
}
int VDSessionFacade::getMode() {
	return mVDSession->getMode();
}
bool VDSessionFacade::showUI() {
	return mVDSession->showUI();
};
VDSessionFacadeRef VDSessionFacade::toggleUI() {
	mVDSession->toggleUI();
	return shared_from_this();
};
std::string VDSessionFacade::getModeName(unsigned int aMode) {
	return mVDSession->getModeName(aMode);
}
int VDSessionFacade::getUniformIndexForName(const std::string& aName) {
	return mVDSession->getUniformIndexForName(aName);
};
float VDSessionFacade::getUniformValue(unsigned int aCtrl) {
	return mVDMediator->getUniformValue(aCtrl);
};

std::string VDSessionFacade::getUniformName(unsigned int aIndex) {
	return mVDMediator->getUniformName(aIndex);
}
void VDSessionFacade::setIntUniformValueByIndex(unsigned int aCtrl, int aValue) {
	mVDMediator->setUniformValue(aCtrl, aValue);
}
void VDSessionFacade::setBoolUniformValueByIndex(unsigned int aCtrl, float aValue) {
	mVDMediator->setUniformValue(aCtrl, aValue);
}
// end terminal operations 
// begin events
bool VDSessionFacade::handleMouseMove(MouseEvent event)
{
	return mVDSession->handleMouseMove(event);
}
bool VDSessionFacade::handleMouseDown(MouseEvent event)
{
	return mVDSession->handleMouseDown(event);
}
bool VDSessionFacade::handleMouseDrag(MouseEvent event)
{
	return mVDSession->handleMouseDrag(event);
}
bool VDSessionFacade::handleMouseUp(MouseEvent event)
{
	return mVDSession->handleMouseUp(event);
}
void VDSessionFacade::fileDrop(FileDropEvent event)
{
	mVDSession->fileDrop(event);
}

bool VDSessionFacade::handleKeyDown(KeyEvent& event) {
	bool handled = true;
	if (!mVDSession->handleKeyDown(event)) {
		if (!mVDMediator->handleKeyDown(event)) {
			handled = false;
		}
	}
	event.setHandled(handled);
	return event.isHandled();
}
bool VDSessionFacade::handleKeyUp(KeyEvent& event) {
	bool handled = true;
	if (!mVDSession->handleKeyUp(event)) {
		if (!mVDMediator->handleKeyUp(event)) {
			handled = false;
		}
	}
	event.setHandled(handled);
	return event.isHandled();
}

// end events
VDSessionRef VDSessionFacade::getInstance() const {
	return mVDSession;
}