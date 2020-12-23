/*
	VDMediator
	Mediator observable
*/
// TODO 

#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"

// Uniforms
#include "VDUniforms.h"
// FboShader
#include "VDFboShader.h"
// Keyboard
#include "VDKeyboard.h"
// Osc
#include "VDOscReceiver.h"
// Websocket client
#include "VDWebsocket.h"
#include <memory>
#include <vector>

#define OSC_DEFAULT_PORT 10001
#define WS_DEFAULT_HOST "127.0.0.1"
#define WS_DEFAULT_PORT 8088

using namespace ci;
using namespace ci::app;

namespace videodromm {
	class VDOscReceiver;
	typedef std::shared_ptr<VDOscReceiver> VDOscReceiverRef;

	class VDWebsocket;
	typedef std::shared_ptr<VDWebsocket> VDWebsocketRef;

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
		static VDMediatorObservableRef		createVDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
		VDMediatorObservableRef				addObserver(VDUniformObserverRef o);
		VDMediatorObservableRef				setupOSCReceiver();
		int									getOSCReceiverPort();
		void								setOSCReceiverPort(int aReceiverPort);
		void								setOSCMsg(const std::string& aMsg);
		std::string							getOSCMsg();

		VDMediatorObservableRef				setupWSClient();
		int									getWSClientPort();
		void								setWSClientPort(int aPort);
		void								setWSMsg(const std::string& aMsg);
		std::string							getWSMsg();
		void								wsConnect();
		void								wsPing();
		void								update();

		VDMediatorObservableRef				setupKeyboard();
		float								getUniformValue(unsigned int aIndex);
		std::string							getUniformName(unsigned int aIndex);
		VDMediatorObservableRef				setUniformValue(int aIndex, float aValue);
		VDMediatorObservableRef				updateShaderText(int aIndex, float aValue);
		bool								handleKeyDown(KeyEvent& event);
		bool								handleKeyUp(KeyEvent& event);
		ci::gl::TextureRef					getFboShaderTexture(unsigned int aFboShaderIndex);
		std::string							getFboShaderName(unsigned int aFboShaderIndex);
		unsigned int						getFboShadersCount();
		std::vector<ci::gl::GlslProg::Uniform>	getFboShaderUniforms(unsigned int aFboShaderIndex);
		int									getUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex);
		void								setUniformValueByLocation(unsigned int aFboShaderIndex, unsigned int aLocationIndex, float aValue);

	private:
		std::vector<VDUniformObserverRef>	mObservers;
		// Settings
		VDSettingsRef						mVDSettings;
		// Animation
		VDAnimationRef						mVDAnimation;
		// Uniforms
		VDUniformsRef						mVDUniforms;
		// OSC
		VDOscReceiverRef					mVDOscReceiver;
		// Websockets
		VDWebsocketRef						mVDWebsocket;
		// FboShaders
		VDFboShaderRef						fboShaderHydra0;
		VDFboShaderRef						fboShaderHydra1;
		VDFboShaderRef						fboShader;
		// maintain a list of fbos 
		VDFboShaderList						mFboShaderList;
		//! OSC
		bool								validateJson(const JsonTree& tree);
		int									mOSCReceiverPort;
		const std::string					mOSCJsonFileName = "oscreceiver.json";
		void								loadOSCReceiverFromJsonFile(const fs::path& jsonFile);
		JsonTree							saveOSCReceiverToJson() const;
		//! WS
		std::string							mWSHost;
		int									mWSPort;
		const std::string					mWSJsonFileName = "wsclient.json";
		void								loadWSFromJsonFile(const fs::path& jsonFile);
		JsonTree							saveWSToJson() const;
		//std::string							mShaderLeft;
		//std::string							mShaderRight;
		void								setFragmentShaderString(unsigned int aFboShaderIndex, const std::string& aFragmentShaderString, const std::string& aName);
		// Keyboard
		VDKeyboardRef						mVDKeyboard;
		//VDMediatorObservable() {}
		VDMediatorObservable(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDUniformsRef aVDUniforms);
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
