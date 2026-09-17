/*
	VDLog
	Handles logging to the filesystem and console.
*/
// rewriten as a singleton

#pragma once

#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/DataSource.h"
#include "cinder/Utilities.h"
// log
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{
	// stores the pointer to the VDLog instance
	typedef std::shared_ptr<class VDLog> VDLogRef;

	class VDLog {
	public:

		static VDLogRef	create();
		// file logging is opt-in at runtime (a VDUI button, see VDUI.cpp) rather than always-on
		// for the whole debug session - only meaningful in debug builds at all ("log to file only
		// in debug mode"); a no-op in release builds. Each time it's turned on, every log file
		// this run creates shares one per-session timestamp, so separate app launches never mix
		// their output into the same file the way the single daily-rotating file used to.
		static void		setFileLoggingEnabled(bool aEnabled);
		static bool		isFileLoggingEnabled();

	private:
		static VDLogRef LOGGER;
		VDLog();
		static std::string							sessionTimestamp();
		static std::shared_ptr<ci::log::LoggerFile>	sFileLoggerAll;
		static std::shared_ptr<ci::log::LoggerFile>	sFileLoggerWarnings;
		static std::shared_ptr<ci::log::LoggerFile>	sFileLoggerErrors;
	};


}
