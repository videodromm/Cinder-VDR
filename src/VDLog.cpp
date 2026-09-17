#include "VDLog.h"
#include <ctime>

using namespace videodromm;

VDLogRef VDLog::LOGGER;
std::shared_ptr<ci::log::LoggerFile> VDLog::sFileLoggerAll;
std::shared_ptr<ci::log::LoggerFile> VDLog::sFileLoggerWarnings;
std::shared_ptr<ci::log::LoggerFile> VDLog::sFileLoggerErrors;

VDLogRef	VDLog::create() {
	if (LOGGER.use_count() == 0) {
		LOGGER = VDLogRef(new VDLog());
	}
	return LOGGER;
}
VDLog::VDLog()
{
	auto sysLogger = log::makeLogger<log::LoggerSystem>();
	sysLogger->setLevel(log::LEVEL_WARNING);

	// file logging used to be created here unconditionally in debug builds - now it's opt-in at
	// runtime via setFileLoggingEnabled() (see VDUI.cpp's "Log to file" button), so a debug
	// session doesn't silently accumulate a log file unless explicitly turned on.
}

std::string VDLog::sessionTimestamp() {
	// one timestamp per process, computed once and reused for every log file this run creates,
	// so separate launches never mix their output into the same file - the previous setup
	// (LoggerFileRotating, one file per calendar day) could do exactly that across several runs
	// on the same day.
	static std::string sTimestamp;
	if (sTimestamp.empty()) {
		std::time_t t = std::time(nullptr);
		std::tm tmVal;
#if defined( CINDER_MSW )
		localtime_s(&tmVal, &t);
#else
		localtime_r(&t, &tmVal);
#endif
		char buf[32];
		std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmVal);
		sTimestamp = buf;
	}
	return sTimestamp;
}

bool VDLog::isFileLoggingEnabled() {
	return sFileLoggerAll != nullptr;
}

void VDLog::setFileLoggingEnabled(bool aEnabled) {
#if defined( _DEBUG )
	if (aEnabled == isFileLoggingEnabled()) return;
	if (aEnabled) {
		// a "logs" folder at the repos root, next to the shared "assets" folder every project
		// under it already resolves via getAssetPath() - reusing that same folder-walking search
		// instead of a hardcoded path keeps this working regardless of which project/subfolder
		// the running exe actually lives in.
		fs::path logsFolder = getAssetPath("") / ".." / "logs";
		if (!fs::exists(logsFolder)) {
			fs::create_directories(logsFolder);
		}
		std::string ts = sessionTimestamp();
		sFileLoggerAll = log::makeLogger<log::LoggerFile>(logsFolder / ("vd." + ts + ".log"), false);
		sFileLoggerWarnings = log::makeLogger<log::LoggerFile>(logsFolder / ("vd." + ts + ".warnings.log"), false);
		sFileLoggerWarnings->setLevel(log::LEVEL_WARNING);
		sFileLoggerErrors = log::makeLogger<log::LoggerFile>(logsFolder / ("vd." + ts + ".errors.log"), false);
		sFileLoggerErrors->setLevel(log::LEVEL_ERROR);
	}
	else {
		if (sFileLoggerAll) { log::manager()->removeLogger(sFileLoggerAll); sFileLoggerAll = nullptr; }
		if (sFileLoggerWarnings) { log::manager()->removeLogger(sFileLoggerWarnings); sFileLoggerWarnings = nullptr; }
		if (sFileLoggerErrors) { log::manager()->removeLogger(sFileLoggerErrors); sFileLoggerErrors = nullptr; }
	}
#endif // _DEBUG
}
