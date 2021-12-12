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
			if (aIndex != 44) { // mVDUniforms->IFPS
				osc::Message msg("/params");
				msg.append(aIndex);
				msg.append(aValue);
				// Send the msg and also provide an error handler. If the message is important you could store it in the error callback to dispatch it again if there was a problem.
				mSender.send(msg, std::bind(&VDOscObserver::onSendError,
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
