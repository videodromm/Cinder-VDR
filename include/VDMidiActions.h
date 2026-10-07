#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/JsonTree.h"
#include "cinder/Log.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace videodromm
{
	// MIDI bindings shared with TSWebsocketServer: assets/actions/<channel><Device>.json, arrays of
	// {id, channel, value, action}. Ids are TSWebsocketServer's: "<type>_<channel 0-15>_<number>_<Device>",
	// type cc / on (note on) / of (note off) / pa (poly aftertouch) / nr (NRPN), Device = the
	// assets/hardware/*.json file stem whose "midiPortMatch" (else "name") is a case-insensitive
	// substring of the MIDI port name (TSWebsocketServer's deviceFileForPortName), else "unknown".
	// Cinder runs the uniform actions itself only when it isn't connected to TSWebsocketServer:
	// wsp_<uniform>_x (value 0..1 = MIDI value / 127), won_<uniform> (1), wof_<uniform> (0).
	class VDMidiActions {
	public:
		struct Binding {
			std::string	id;
			int			channel = 0;
			std::string	action;
		};

		void ensureLoaded() {
			if (mLoaded) return;
			mLoaded = true;
			loadHardware();
			loadActions();
		}
		// reloads both folders (e.g. after TSWebsocketServer or another PC changed them)
		void reload() { mLoaded = false; mActions.clear(); mMatchers.clear(); ensureLoaded(); }

		std::string deviceForPort(const std::string& aPortName) const {
			std::string port = lower(aPortName);
			for (const auto& m : mMatchers) {
				if (!m.needle.empty() && port.find(m.needle) != std::string::npos) return m.file;
			}
			return "unknown";
		}
		bool isKnownDevice(const std::string& aDevice) const {
			for (const auto& m : mMatchers) if (m.file == aDevice) return true;
			return false;
		}
		static std::string makeId(const std::string& aType, int aChannel, int aNumber, const std::string& aDevice) {
			return aType + "_" + std::to_string(aChannel) + "_" + std::to_string(aNumber) + "_" + aDevice;
		}
		std::string actionFor(const std::string& aId) const {
			auto it = mActions.find(aId);
			return it != mActions.end() ? it->second.action : "";
		}
		void set(const std::string& aId, int aChannel, const std::string& aAction) {
			mActions[aId] = { aId, aChannel, aAction };
		}
		bool remove(const std::string& aId, Binding& aRemoved) {
			auto it = mActions.find(aId);
			if (it == mActions.end()) return false;
			aRemoved = it->second;
			mActions.erase(it);
			return true;
		}
		// rewrites assets/actions/<channel><Device>.json from memory (every entry of that channel
		// and device, so entries loaded from the file are kept). Unknown devices aren't written,
		// like TSWebsocketServer's saveActionsToDisk.
		bool saveFile(int aChannel, const std::string& aDevice) {
			if (!isKnownDevice(aDevice)) {
				CI_LOG_W("midi actions: device \"" << aDevice << "\" has no assets/hardware layout (midiPortMatch), binding kept for this session only");
				return false;
			}
			try {
				ci::JsonTree doc = ci::JsonTree::makeArray();
				for (const auto& kv : mActions) {
					const Binding& b = kv.second;
					if (b.channel != aChannel || deviceOfId(b.id) != aDevice) continue;
					ci::JsonTree entry = ci::JsonTree::makeObject();
					entry.addChild(ci::JsonTree("id", b.id));
					entry.addChild(ci::JsonTree("channel", b.channel));
					entry.addChild(ci::JsonTree("value", 0));
					entry.addChild(ci::JsonTree("action", b.action));
					doc.pushBack(entry);
				}
				ci::fs::path dir = ci::app::getAssetPath("") / "actions";
				if (!ci::fs::exists(dir)) ci::fs::create_directories(dir);
				doc.write(ci::writeFile(dir / (std::to_string(aChannel) + aDevice + ".json")), ci::JsonTree::WriteOptions());
				return true;
			}
			catch (const std::exception& ex) {
				CI_LOG_E("midi actions save: " << ex.what());
				return false;
			}
		}
		// uniform index an action sets (wsp/won/wof/wpv-free), -1 otherwise
		static int uniformOfAction(const std::string& aAction, std::string* aCommand = nullptr) {
			std::size_t first = aAction.find('_');
			if (first == std::string::npos) return -1;
			std::string cmd = aAction.substr(0, first);
			if (aCommand) *aCommand = cmd;
			if (cmd != "wsp" && cmd != "won" && cmd != "wof") return -1;
			try { return std::stoi(aAction.substr(first + 1)); }
			catch (...) { return -1; }
		}
		static std::string deviceOfId(const std::string& aId) {
			// 4th "_"-separated segment
			std::size_t pos = 0;
			for (int i = 0; i < 3; i++) {
				pos = aId.find('_', pos);
				if (pos == std::string::npos) return "";
				pos++;
			}
			return aId.substr(pos);
		}
		// every binding whose action sets a uniform, ordered by id
		std::vector<Binding> uniformBindings() const {
			std::vector<Binding> out;
			for (const auto& kv : mActions) if (uniformOfAction(kv.second.action) >= 0) out.push_back(kv.second);
			return out;
		}
		// ids bound to a uniform, joined for a tooltip/label ("" when none)
		std::string idsForUniform(int aUniform) const {
			std::string out;
			for (const auto& kv : mActions) {
				if (uniformOfAction(kv.second.action) != aUniform) continue;
				if (!out.empty()) out += ", ";
				out += kv.first;
			}
			return out;
		}

	private:
		struct Matcher {
			std::string file;
			std::string needle;
		};
		bool						mLoaded = false;
		std::map<std::string, Binding>	mActions;
		std::vector<Matcher>		mMatchers;

		static std::string lower(std::string s) {
			for (auto& c : s) c = (char)std::tolower((unsigned char)c);
			return s;
		}
		void loadHardware() {
			ci::fs::path dir = ci::app::getAssetPath("") / "hardware";
			if (!ci::fs::is_directory(dir)) return;
			for (const auto& entry : ci::fs::directory_iterator(dir)) {
				if (entry.path().extension() != ".json") continue;
				Matcher m;
				m.file = entry.path().stem().string();
				try {
					ci::JsonTree json(ci::loadFile(entry.path()));
					if (json.hasChild("midiPortMatch")) m.needle = lower(json.getValueForKey<std::string>("midiPortMatch"));
					else if (json.hasChild("name")) m.needle = lower(json.getValueForKey<std::string>("name"));
				}
				catch (const std::exception& ex) {
					CI_LOG_W("midi actions: " << entry.path().filename().string() << ": " << ex.what());
				}
				mMatchers.push_back(m);
			}
		}
		void loadActions() {
			ci::fs::path dir = ci::app::getAssetPath("") / "actions";
			if (!ci::fs::is_directory(dir)) return;
			for (const auto& entry : ci::fs::directory_iterator(dir)) {
				if (entry.path().extension() != ".json") continue;
				try {
					ci::JsonTree json(ci::loadFile(entry.path()));
					for (const auto& item : json.getChildren()) {
						if (!item.hasChild("id") || !item.hasChild("action")) continue;
						std::string action = item.getValueForKey<std::string>("action");
						if (action.empty()) continue;
						Binding b;
						b.id = item.getValueForKey<std::string>("id");
						b.channel = item.hasChild("channel") ? item.getValueForKey<int>("channel") : 0;
						b.action = action;
						mActions[b.id] = b;
					}
				}
				catch (const std::exception& ex) {
					CI_LOG_W("midi actions: " << entry.path().filename().string() << ": " << ex.what());
				}
			}
		}
	};
}
