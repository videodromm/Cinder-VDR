#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/TextureFont.h"

#include <memory>
#include <string>
#include <vector>

namespace videodromm
{
	// Live-coding overlay: renders the code being typed in the WebApp editors (received as
	// websocket "codeview" events: text + cursor + compile-error lines + "lang") with syntax
	// colours into a transparent RGBA texture, meant to be sent out as its own Spout sender.
	// Two documents: the GLSL shader and the Strudel code. Each shows while its editor is open;
	// when both are, the texture is split (side by side or stacked).
	typedef std::shared_ptr<class VDCodeView> VDCodeViewRef;

	class VDCodeView {
	public:
		static VDCodeViewRef	create() { return std::shared_ptr<VDCodeView>(new VDCodeView()); }

		enum Lang { LANG_GLSL = 0, LANG_STRUDEL = 1, LANG_COUNT = 2 };
		// "glsl" (default, also for anything unknown) or "strudel"
		static Lang				langFromName(const std::string& aName) { return aName == "strudel" ? LANG_STRUDEL : LANG_GLSL; }
		// state, from the websocket ("line" is 1-based, "col" 0-based, error lines 1-based)
		void					setState(const std::string& aText, int aLine, int aCol, const std::vector<int>& aErrorLines, bool aActive, Lang aLang = LANG_GLSL);
		// at least one document is shown
		bool					isActive() const { return mDocs[LANG_GLSL].active || mDocs[LANG_STRUDEL].active; }
		bool					isActive(Lang aLang) const { return mDocs[aLang].active; }

		// re-renders only when the state, the cursor blink phase or a setting changed
		ci::gl::Texture2dRef	render(const ci::ivec2& aSize);
		// last rendered texture without re-rendering (UI preview), nullptr before the first render()
		ci::gl::Texture2dRef	getTexture() const {
			if (!mFbo) return nullptr;
			return (mPremultiplied || !mUnpremultiplyProg) ? mFbo->getColorTexture() : mStraightFbo->getColorTexture();
		}
		int						getLineCount(Lang aLang = LANG_GLSL) const { return (int)mDocs[aLang].lines.size(); }

		// settings (UI)
		float					getFontSize() const { return mFontSize; }
		void					setFontSize(float aSize);
		float					getBackgroundAlpha() const { return mBackgroundAlpha; }
		void					setBackgroundAlpha(float aAlpha) { mBackgroundAlpha = aAlpha; mDirty = true; }
		bool					getShadow() const { return mShadow; }
		void					setShadow(bool aShadow) { mShadow = aShadow; mDirty = true; }
		bool					getLineNumbers() const { return mLineNumbers; }
		void					setLineNumbers(bool aLineNumbers) { mLineNumbers = aLineNumbers; mDirty = true; }
		// Spout carries 8-bit RGBA either way; whether the receiver expects premultiplied or straight
		// alpha decides whether anti-aliased glyph edges get dark or bright fringes
		// how the cursor's line is marked (error lines are always tinted red instead)
		enum CurrentLineStyle { LINE_FILL = 0, LINE_BORDER = 1, LINE_CURSOR_ONLY = 2, LINE_NUMBER_FILL = 3 };
		int						getCurrentLineStyle() const { return mCurrentLineStyle; }
		void					setCurrentLineStyle(int aStyle) { mCurrentLineStyle = aStyle; mDirty = true; }
		// highlight colour, for every style except cursor-only
		ci::ColorA				getCurrentLineColor() const { return mCurrentLineColor; }
		void					setCurrentLineColor(const ci::ColorA& aColor) { mCurrentLineColor = aColor; mDirty = true; }
		bool					getPremultiplied() const { return mPremultiplied; }
		void					setPremultiplied(bool aPremultiplied) { mPremultiplied = aPremultiplied; mDirty = true; }
		// both documents shown: GLSL left / Strudel right, or GLSL top / Strudel bottom
		enum Layout { LAYOUT_SIDE_BY_SIDE = 0, LAYOUT_STACKED = 1 };
		int						getLayout() const { return mLayout; }
		void					setLayout(int aLayout) { mLayout = aLayout; mDirty = true; }

	private:
		VDCodeView() = default;

		enum class TokenKind { Plain, Keyword, Type, Function, Uniform, Number, Comment, Preprocessor, String };
		struct Token { size_t start; size_t length; TokenKind kind; };
		struct Doc {
			std::vector<std::string>		lines;
			std::vector<std::vector<Token>>	tokens;
			std::vector<int>				errorLines;
			int								cursorLine = 0;	// 0-based internally
			int								cursorCol = 0;
			bool							active = false;
			// scroll follows the cursor, using this view's own font metrics (the editor's differ)
			int								topLine = 0;
			int								leftCol = 0;
		};

		static void				tokenizeGlsl(Doc& aDoc);
		static void				tokenizeStrudel(Doc& aDoc);
		void					ensureFont();
		void					ensureFbos(const ci::ivec2& aSize);
		static void				updateScroll(Doc& aDoc, int aVisibleLines, int aVisibleCols);
		void					draw();
		void					drawDoc(Doc& aDoc, const ci::Rectf& aArea, std::vector<std::pair<ci::Font::Glyph, ci::vec2>>& aGlyphs, std::vector<ci::ColorA8u>& aColors);

		// state
		Doc									mDocs[LANG_COUNT];
		Lang								mFocus = LANG_GLSL;	// last document typed in: shows the cursor

		// settings
		float								mFontSize = 28.0f;
		float								mBackgroundAlpha = 0.0f;
		bool								mShadow = true;
		bool								mLineNumbers = true;
		bool								mPremultiplied = false;
		ci::ColorA							mCurrentLineColor = ci::ColorA(1.0f, 1.0f, 1.0f, 0.07f);
		int									mCurrentLineStyle = LINE_FILL;
		int									mLayout = LAYOUT_SIDE_BY_SIDE;

		// rendering
		bool								mDirty = true;
		bool								mCursorOn = true;
		ci::gl::TextureFontRef				mFont;
		float								mFontCreatedSize = 0.0f;
		float								mCharWidth = 0.0f;
		float								mLineHeight = 0.0f;
		ci::gl::FboRef						mFbo;			// premultiplied
		ci::gl::FboRef						mStraightFbo;	// unpremultiplied copy, when !mPremultiplied
		ci::gl::GlslProgRef					mUnpremultiplyProg;
	};
}
