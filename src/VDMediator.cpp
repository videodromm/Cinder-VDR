#include "VDMediator.h"

using namespace videodromm;

VDMediatorObservableRef VDMediatorObservable::createVDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation)
{
	return VDMediatorObservableRef(new VDMediatorObservable(aVDSettings, aVDAnimation));
}

VDMediatorObservable::VDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation) {
	CI_LOG_V("VDMediatorObservable constructor");
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mOSCReceiverPort = OSC_DEFAULT_PORT;
}
VDMediatorObservableRef VDMediatorObservable::addObserver(VDUniformObserverRef o) {
	mObservers.push_back(o);
	return shared_from_this();
}
VDMediatorObservableRef VDMediatorObservable::setupOSCReceiver() {
	// Osc Receiver
	mVDOscReceiver = VDOscReceiver::create(mVDSettings, mVDAnimation);
	mOSCReceiverPort = OSC_DEFAULT_PORT;
	fs::path jsonFile = getAssetPath("") / mOSCJsonFileName;
	loadOSCReceiverFromJsonFile(jsonFile);
	mVDOscReceiver->setupOSCReceiver(shared_from_this(), mOSCReceiverPort);
	saveOSCReceiverToJson();
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
VDMediatorObservableRef VDMediatorObservable::setupKeyboard() {
	// Keyboard
	mVDKeyboard = VDKeyboard::create(mVDSettings, mVDAnimation);
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