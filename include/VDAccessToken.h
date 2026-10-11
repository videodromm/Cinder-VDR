#pragma once
// TSWebsocketServer's shared access token (its .env ACCESS_TOKEN, security review 2026-10-11).
// Empty when not set: the server then requires nothing. Read once, from the ACCESS_TOKEN
// environment variable, else from TSWebsocketServer/.env next to the assets folder (same
// machine, the repos layout). Sent as "token=" on the websocket URL (VDWebsocket), on REST URLs
// (VDSession) and to the WebApp in the Web panel (VDUIHtmlPage).
#if defined( _WIN32 ) && !defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/Filesystem.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>

namespace videodromm {
	namespace access {
		inline std::string trimValue(std::string v) {
			while (!v.empty() && (v.back() == '\r' || v.back() == '\n' || v.back() == ' ' || v.back() == '\t')) v.pop_back();
			size_t start = v.find_first_not_of(" \t");
			v = start == std::string::npos ? "" : v.substr(start);
			if (v.size() >= 2 && (v.front() == '"' || v.front() == '\'') && v.back() == v.front()) v = v.substr(1, v.size() - 2);
			return v;
		}
		inline std::string loadToken() {
			if (const char* env = std::getenv("ACCESS_TOKEN")) {
				std::string v = trimValue(env);
				if (!v.empty()) return v;
			}
			try {
				const ci::fs::path assets = ci::app::getAssetPath("");
				if (assets.empty()) return "";
				const ci::fs::path envFile = assets.parent_path() / "TSWebsocketServer" / ".env";
				std::ifstream in(envFile.string());
				std::string line;
				while (std::getline(in, line)) {
					if (line.rfind("ACCESS_TOKEN=", 0) == 0) return trimValue(line.substr(13));
				}
			}
			catch (...) {
				// no token: the server requires none unless it's set there too
			}
			return "";
		}
		/** The token, "" if none. */
		inline const std::string& token() {
			static const std::string value = loadToken();
			return value;
		}
		/** Percent-encoded for a URL query. */
		inline std::string encodedToken() {
			std::string out;
			static const char* hex = "0123456789ABCDEF";
			for (unsigned char c : token()) {
				if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
				else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
			}
			return out;
		}
		/** aUrl with "token=" added ("?" or "&"), unchanged when there's no token. */
		inline std::string withToken(const std::string& aUrl) {
			if (token().empty()) return aUrl;
			const size_t hash = aUrl.find('#');
			std::string base = hash == std::string::npos ? aUrl : aUrl.substr(0, hash);
			const std::string fragment = hash == std::string::npos ? "" : aUrl.substr(hash);
			base += (base.find('?') == std::string::npos ? "?" : "&") + std::string("token=") + encodedToken();
			return base + fragment;
		}
	}
}
