/*
	VDOscSender
	Mediator Osc Sender
*/
// TODO 

#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Mediator
#include "VDMediator.h"

#include <memory>
#include <vector>
#include "cinder/osc/Osc.h"

using namespace ci;
using namespace ci::app;
using namespace asio;
using namespace asio::ip;
using namespace ci::osc;

namespace videodromm {
	
	class VDOscObserver : public VDUniformObserver {
	public:
		static VDUniformObserverRef connect(const std::string& host, unsigned int port) {
			VDOscObserver* o = new VDOscObserver(host, port);
			o->bind();

			VDUniformObserverRef obs(o);

			return obs;
		}
		VDUniformObserverRef setUniformValue(int aIndex, float aValue) {
			/*if (aIndex != 44) { // mVDUniforms->IFPS
				//osc::Message msg("/params");
				osc::Message msg("/composition/video/effects/colorize/bypassed");
				msg.append(aIndex);
				msg.append(aValue);
				// Send the msg and also provide an error handler. If the message is important you could store it in the error callback to dispatch it again if there was a problem.
				mSender.send(msg, std::bind(&VDOscObserver::onSendError,
					this, std::placeholders::_1));
			}*/
			std::string oscMsg("");
			std::string oscMsg2("");
			bool sendIndex = true; 
			bool sendValueOnly = false;
				// /composition/crossfader/phase  /composition/crossfader/sideb /composition/tempocontroller/tempo
				// /composition/layers/2/clips/4/transport/position
			switch (aIndex) {
			case 11:
				// IROTATIONSPEED
				oscMsg = "/composition/dashboard/link1";
				sendIndex = false;
				break;
			case 18:
				// IMOUSEX setUniformValue(mVDUniforms->IMOUSEX, 0.27710f)
				oscMsg = "/composition/crossfader/phase";
				sendIndex = false;
				sendValueOnly = true;
				break;
			case 44:
				// IFPS nothing
				break;
			case 88:
				// IGREYSCALE
				oscMsg = "/composition/video/effects/colorize/bypassed";
				sendIndex = false;
				break;
			case 251:
				// ICOLUMN1 Welcome				
				oscMsg = "/composition/layers/1/clips/1/connect";
				sendIndex = false;
				break;
			case 252:
				// ICOLUMN2 het
				oscMsg = "/composition/layers/2/clips/2/connect";
				sendIndex = false;
				break;
			case 253:
				// ICOLUMN3 mtrlvr
				oscMsg = "/composition/layers/1/clips/3/connect";
				sendIndex = false;
				break;
			case 254:
				// ICOLUMN4 Evolution 
				sendIndex = false;
				oscMsg = "/composition/layers/2/clips/4/connect";
				oscMsg2 = "/composition/layers/3/clips/4/connect";
				//oscMsg = "/composition/columns/4/connect";
				break;
			case 255:
				// ICOLUMN5 SOS
				oscMsg = "/composition/layers/1/clips/5/connect";
				sendIndex = false;
				break;
			case 256:
				// ICOLUMN6 zf
				oscMsg = "/composition/layers/2/clips/6/connect";
				sendIndex = false;
				break;
			case 257:
				// ICOLUMN7 logo rotation
				oscMsg = "/composition/layers/1/clips/7/connect";
				sendIndex = false;
				break;
			case 258:
				// ICOLUMN8 fnm
				oscMsg = "/composition/layers/2/clips/8/connect";
				sendIndex = false;
				break;			

			default:
				oscMsg = "/params";
			}
			if (oscMsg.length() > 1) {
				osc::Message msg(oscMsg);
				if (sendIndex) {
					msg.append(aIndex);
				}
				msg.append(aValue);
				mSender.send(msg, std::bind(&VDOscObserver::onSendError,
					this, std::placeholders::_1));
			}
			if (oscMsg2.length() > 1) {
				osc::Message msg2(oscMsg2);
				msg2.append(aValue);
				mSender.send(msg2, std::bind(&VDOscObserver::onSendError,
					this, std::placeholders::_1));
			}
			return shared_from_this();
		}
		VDOscObserver* bind() {
			try {
				// Bind the sender to the endpoint. This function may throw. The exception will contain asio::error_code information.
				mSender.bind();
				mIsConnected = true;
			}
			catch (const osc::Exception& ex) {
				CI_LOG_E("Error binding: " << ex.what() << " val: " << ex.value());
			}
			return this;
		}
		void VDOscObserver::onSendError(asio::error_code error)
		{
			if (error) {
				CI_LOG_E("Error sending: " << error.message() << " val: " << error.value());
				// If you determine that this error is fatal, make sure to flip mIsConnected. It's possible that the error isn't fatal.
				mIsConnected = false;
				try {
					// Close the socket on exit. This function could throw. The exception will contain asio::error_code information.
					mSender.close();
				}
				catch (const osc::Exception& ex) {
					CI_LOG_EXCEPTION("Cleaning up socket: val -" << ex.value(), ex);
				}
			}
		}
		bool mIsConnected = false;
		~VDOscObserver() { mSender.close(); };
	private:
		VDOscObserver(const std::string& host, unsigned int port) : mSender(10002, host, port) {
		}
		osc::SenderUdp	mSender;
	};
}
