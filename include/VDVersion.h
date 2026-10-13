#pragma once
// Which code is running, for the log files (VDLog) and the websocket "hello" sent to
// TSWebsocketServer (VDSessionFacade::update, listed by its GET /api/version/details). Each block
// registers itself from one of its own .cpp files (Cinder-VDR: VDLog.cpp, Cinder-VDRUI: VDUI.cpp)
// with VD_REGISTER_VERSION: the build date/time of that file, and the git commit of the block's
// working copy, read from its .git when asked (so it's the source on disk, which may be newer
// than the build if the files were synced without a rebuild: compare the build time).
// Header only, so no project file needs a new .cpp.
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// before the first Windows header (cinder/app/App.h), see VDLog.h
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/Filesystem.h"
#include "cinder/Json.h"
#include "cinder/app/App.h"
#include "VDMachine.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <string>
#if defined( CINDER_MAC )
#include <stdlib.h>
#endif

namespace videodromm
{
	namespace version
	{
		struct Component {
			std::string sourceFile; // __FILE__ of the registering .cpp: <block>/src/<file>.cpp
			std::string built;      // __DATE__ " " __TIME__
		};
		inline std::map<std::string, Component>& components() {
			static std::map<std::string, Component> sComponents;
			return sComponents;
		}
		inline bool registerComponent(const std::string& aName, const char* aSourceFile, const char* aBuilt) {
			components()[aName] = { aSourceFile, aBuilt };
			return true;
		}

		// "abc1234", read from <repoDir>/.git without running git; "" when not a git working copy
		inline std::string gitHead(const ci::fs::path& aRepoDir) {
			auto readLine = [](const ci::fs::path& aPath) {
				std::ifstream in(aPath.string());
				std::string line;
				std::getline(in, line);
				while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
				return line;
			};
			try {
				const ci::fs::path gitDir = aRepoDir / ".git";
				if (!ci::fs::is_directory(gitDir)) return "";
				const std::string head = readLine(gitDir / "HEAD");
				if (head.rfind("ref: ", 0) != 0) return head.substr(0, 7);
				const std::string ref = head.substr(5);
				if (ci::fs::exists(gitDir / ref)) return readLine(gitDir / ref).substr(0, 7);
				std::ifstream packed((gitDir / "packed-refs").string());
				std::string line;
				while (std::getline(packed, line)) {
					if (!line.empty() && line.back() == '\r') line.pop_back();
					if (line.size() > ref.size() && line.compare(line.size() - ref.size(), ref.size(), ref) == 0) return line.substr(0, 7);
				}
			}
			catch (...) {}
			return "";
		}

		// the block's folder: from __FILE__ (an absolute path with MSBuild and Xcode), else
		// <repos>/Cinder/blocks/<name>, found from the assets folder (only once the app runs)
		inline ci::fs::path blockFolder(const std::string& aName, const Component& aComponent) {
			try {
				ci::fs::path fromSource = ci::fs::path(aComponent.sourceFile).parent_path().parent_path();
				if (fromSource.is_absolute() && ci::fs::is_directory(fromSource / ".git")) return fromSource;
				ci::fs::path assets = ci::app::getAssetPath("");
				if (!assets.empty()) return assets / ".." / "Cinder" / "blocks" / aName;
			}
			catch (...) {}
			return ci::fs::path();
		}

		// "built Oct 13 2026 10:12:00, source abc1234"
		inline std::string describe(const std::string& aName) {
			auto it = components().find(aName);
			if (it == components().end()) return "";
			std::string text = "built " + it->second.built;
			const std::string sha = gitHead(blockFolder(aName, it->second));
			if (!sha.empty()) text += ", source " + sha;
			return text;
		}

		// the executable's name: "VDRUIProject", "VDSpoutInNdiOut"...
		inline std::string appName() {
#if defined( CINDER_MSW )
			char path[MAX_PATH] = { 0 };
			if (::GetModuleFileNameA(nullptr, path, MAX_PATH)) return ci::fs::path(path).stem().string();
#elif defined( CINDER_MAC )
			if (const char* name = getprogname()) return name;
#endif
			return "cinder";
		}

		// the machine id in lower case (as TSWebsocketServer writes it), and its first 8 characters for file names
		inline std::string machineId() {
			std::string id = machine::getId();
			std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return (char)std::tolower(c); });
			return id;
		}
		inline std::string machineShort() {
			std::string id;
			for (char c : machineId()) if (std::isalnum((unsigned char)c) && id.size() < 8) id += c;
			return id.empty() ? "unknown" : id;
		}

		// one line for the top of the log files
		inline std::string summary() {
			std::string text = appName() + " on " + machine::getName() + " (machine " + machineId() + ")";
			for (const auto& c : components()) text += ", " + c.first + " " + describe(c.first);
			return text;
		}

		// the websocket hello (TSWebsocketServer/server/ws.ts)
		inline ci::Json hello() {
			ci::Json versions = ci::Json::object();
			for (const auto& c : components()) versions[c.first] = describe(c.first);
			return {
				{ "event", "hello" }, { "app", "cinder" }, { "exe", appName() }, { "from", "cinder" },
				{ "machine", machineId() }, { "machineName", machine::getName() }, { "versions", versions },
			};
		}
	}
}

// in one .cpp of each block, at namespace scope
#define VD_REGISTER_VERSION(aName) \
	static const bool sVDVersionRegistered = ::videodromm::version::registerComponent(aName, __FILE__, __DATE__ " " __TIME__)
