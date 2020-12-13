/*
	VDMediator
	Mediator observable
*/
// TODO 

#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"

// Keyboard
#include "VDKeyboard.h"
// Osc
#include "VDOscReceiver.h"
#include <memory>
#include <vector>

#define OSC_DEFAULT_PORT 10001

using namespace ci;
using namespace ci::app;

namespace videodromm {
	class VDOscReceiver;
	typedef std::shared_ptr<VDOscReceiver> VDOscReceiverRef;

	class VDKeyboard;
	typedef std::shared_ptr<VDKeyboard> VDKeyboardRef;


	class VDUniformObserver;
	typedef std::shared_ptr<class VDUniformObserver> VDUniformObserverRef;

	class VDUniformObserver : public std::enable_shared_from_this<VDUniformObserver> {
	public:
		virtual VDUniformObserverRef setUniformValue(int aIndex, float aValue) = 0;
		/*VDUniformObserverRef getInstance() const {return std::shared_from_this();}*/			
	private:
	};
	class VDMediatorObservable;
	typedef std::shared_ptr<class VDMediatorObservable> VDMediatorObservableRef;
	class VDMediatorObservable : public std::enable_shared_from_this<VDMediatorObservable> {
	public:
		static VDMediatorObservableRef		createVDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation);
		VDMediatorObservableRef				addObserver(VDUniformObserverRef o);
		VDMediatorObservableRef				setupOSCReceiver();
		int									getOSCReceiverPort();
		void								setOSCReceiverPort(int aReceiverPort);
		void								setOSCMsg(const std::string& aMsg);
		std::string							getOSCMsg();
		VDMediatorObservableRef				setupKeyboard();
		float								getUniformValue(unsigned int aIndex);
		std::string							getUniformName(unsigned int aIndex);
		VDMediatorObservableRef				setUniformValue(int aIndex, float aValue);
		VDMediatorObservableRef				updateShaderText(int aIndex, float aValue);
		bool								handleKeyDown(KeyEvent& event);
		bool								handleKeyUp(KeyEvent& event);
	private:
		std::vector<VDUniformObserverRef>	mObservers;
		// Settings
		VDSettingsRef						mVDSettings;
		// Animation
		VDAnimationRef						mVDAnimation;
		// OSC
		VDOscReceiverRef					mVDOscReceiver;
		bool								validateJson(const JsonTree& tree);
		int									mOSCReceiverPort;
		const std::string					mOSCJsonFileName = "oscreceiver.json";
		void								loadOSCReceiverFromJsonFile(const fs::path& jsonFile);
		JsonTree							saveOSCReceiverToJson() const;
		// Keyboard
		VDKeyboardRef						mVDKeyboard;
		//VDMediatorObservable() {}
		VDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation);
	};

}
		/*void update(void (*callback) (VDUniformObserverRef, map<string, any::type>)) {
			for (VDUniformObserverRef observer : mObservers) {
				callback(observer, params);
			}
		};*/
/*VDMediatorObservableRef mVDMediator = VDMediatorObservable::create()->addObserver(VDSocketIOObserverBuilder->connect(ip, port))->addObserver(new UIDisplay());

mVDMediator->setUniformValue(a, b);
mVDMediator->update([](observer, { "a": 1, "b" : 2 }) -> {
	observer->setUniformValue(a, b);
});

VDProxy::connect(ip, port)

*/
