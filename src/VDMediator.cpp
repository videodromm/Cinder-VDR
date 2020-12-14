#include "VDMediator.h"

using namespace videodromm;

VDMediatorObservableRef VDMediatorObservable::createVDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms)
{
	return VDMediatorObservableRef(new VDMediatorObservable(aVDSettings, aVDAnimation, aVDUniforms));
}

VDMediatorObservable::VDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms) {
	CI_LOG_V("VDMediatorObservable constructor");
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mVDUniforms = aVDUniforms;
	mOSCReceiverPort = OSC_DEFAULT_PORT;
	mWSHost = WS_DEFAULT_HOST;
	mWSPort = WS_DEFAULT_PORT;
}
VDMediatorObservableRef VDMediatorObservable::addObserver(VDUniformObserverRef o) {
	mObservers.push_back(o);
	return shared_from_this();
}
VDMediatorObservableRef VDMediatorObservable::setupOSCReceiver() {
	// Osc Receiver
	mVDOscReceiver = VDOscReceiver::create(mVDSettings, mVDAnimation, mVDUniforms);
	mOSCReceiverPort = OSC_DEFAULT_PORT;
	fs::path jsonFile = getAssetPath("") / mOSCJsonFileName;
	loadOSCReceiverFromJsonFile(jsonFile);
	mVDOscReceiver->setupOSCReceiver(shared_from_this(), mOSCReceiverPort);
	saveOSCReceiverToJson();
	return shared_from_this();
}
VDMediatorObservableRef VDMediatorObservable::setupWSClient() {
	// WS Receiver
	mVDWebsocket = VDWebsocket::create();
	mWSHost = WS_DEFAULT_HOST;
	mWSPort = WS_DEFAULT_PORT;
	fs::path jsonFile = getAssetPath("") / mWSJsonFileName;
	loadWSFromJsonFile(jsonFile);
	mVDWebsocket->setupWSClient(shared_from_this(), mWSHost, mWSPort);
	saveWSToJson();
	return shared_from_this();
}
void VDMediatorObservable::loadOSCReceiverFromJsonFile(const fs::path& jsonFile) {
	if (fs::exists(jsonFile)) {
		JsonTree json(loadFile(jsonFile));
		if (json.hasChild("receiver")) {
			JsonTree u(json.getChild("receiver"));
			if (validateJson(u)) {
				// (u.hasChild("port")) ? u.getValueForKey<int>("port") : OSC_DEFAULT_PORT;
				mOSCReceiverPort = u.getValueForKey<int>("port");
			}
		}		
	}
}
void VDMediatorObservable::loadWSFromJsonFile(const fs::path& jsonFile) {
	if (fs::exists(jsonFile)) {
		JsonTree json(loadFile(jsonFile));
		if (json.hasChild("client")) {
			JsonTree u(json.getChild("client"));
			if (validateJson(u)) {
				// (u.hasChild("port")) ? u.getValueForKey<int>("port") : OSC_DEFAULT_PORT;
				mWSHost = u.getValueForKey<std::string>("host");
				mWSPort = u.getValueForKey<int>("port");
			}
		}		
	}
}
// => VDJsonManager
bool VDMediatorObservable::validateJson(const JsonTree& tree) {
	bool rtn = false;
	if (tree.hasChild("port")) {
		if (tree.getNodeType() == cinder::JsonTree::NodeType::NODE_VALUE) {
			int p = tree.getValueForKey<int>("port");
			if (p > 0 && p < 65536) {
				rtn = true;
			}
		}
	}
	if (!rtn) {
		CI_LOG_E("json not well formatted");
	}
	return rtn;
}
//! to json
JsonTree VDMediatorObservable::saveOSCReceiverToJson() const
{
	JsonTree json;
	JsonTree receiver = ci::JsonTree::makeArray("receiver");
	receiver.addChild(ci::JsonTree("port", mOSCReceiverPort));
	json.addChild(receiver);
	fs::path jsonFile = getAssetPath("") / mOSCJsonFileName;
	json.write(jsonFile);
	return json;
}
JsonTree VDMediatorObservable::saveWSToJson() const
{
	JsonTree json;
	JsonTree client = ci::JsonTree::makeArray("client");
	client.addChild(ci::JsonTree("host", mWSHost));
	client.addChild(ci::JsonTree("port", mWSPort));
	json.addChild(client);
	fs::path jsonFile = getAssetPath("") / mWSJsonFileName;
	json.write(jsonFile);
	return json;
}
int VDMediatorObservable::getWSClientPort() {
	return mWSPort;
}
void VDMediatorObservable::setWSClientPort(int aPort) {
	mWSPort = aPort;
}
void VDMediatorObservable::wsConnect() {
	mVDWebsocket->wsConnect();
}
int VDMediatorObservable::getOSCReceiverPort() {
	return mOSCReceiverPort;
};
void VDMediatorObservable::setOSCReceiverPort(int aReceiverPort) {
	mOSCReceiverPort = aReceiverPort;
};
void VDMediatorObservable::setOSCMsg(const std::string& aMsg) {
	mVDOscReceiver->setOSCMsg(aMsg);
};
std::string VDMediatorObservable::getOSCMsg() {
	return mVDOscReceiver->getOSCMsg();
}

void VDMediatorObservable::setWSMsg(const std::string& aMsg) {
	mVDWebsocket->setWSMsg(aMsg);
};
std::string VDMediatorObservable::getWSMsg() {
	return mVDWebsocket->getWSMsg();
}
void VDMediatorObservable::wsPing() {
	mVDWebsocket->wsPing();
};
VDMediatorObservableRef VDMediatorObservable::setupKeyboard() {
	// Keyboard
	mVDKeyboard = VDKeyboard::create(mVDSettings, mVDAnimation, mVDUniforms);
	mVDKeyboard->setupKeyboard(shared_from_this());
	return shared_from_this();
}
bool VDMediatorObservable::handleKeyDown(KeyEvent& event) {
	return mVDKeyboard->handleKeyDown(event);
}
bool VDMediatorObservable::handleKeyUp(KeyEvent& event) {
	return mVDKeyboard->handleKeyUp(event);
}
float VDMediatorObservable::getUniformValue(unsigned int aIndex) {
	return mVDAnimation->getUniformValue(aIndex);
}
std::string VDMediatorObservable::getUniformName(unsigned int aIndex) {
	return mVDAnimation->getUniformName(aIndex);
}
VDMediatorObservableRef VDMediatorObservable::setUniformValue(int aIndex, float aValue) {
	if (aIndex != mVDUniforms->IFPS) {
		for (auto observer : mObservers) {
			observer->setUniformValue(aIndex, aValue);
		}
	}
	return shared_from_this();
};

VDMediatorObservableRef VDMediatorObservable::updateShaderText(int aIndex, float aValue) {
	for (auto observer : mObservers) {
		observer->setUniformValue(aIndex, aValue);
	}
	return shared_from_this();
};