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
	// Live-coding overlay: renders the shader text being typed in the WebApp editor (received as
	// websocket "codeview" events: text + cursor + compile-error lines) with GLSL syntax colours
	// into a transparent RGBA texture, meant to be sent out as its own Spout sender.
	typedef std::shared_ptr<class VDCodeView> VDCodeViewRef;

	class VDCodeView {
	public:
		static VDCodeViewRef	create() { return std::shared_ptr<VDCodeView>(new VDCodeView()); }

		// state, from the websocket ("line" is 1-based, "col" 0-based, error lines 1-based)
		void					setState(const std::string& aText, int aLine, int aCol, const std::vector<int>& aErrorLines, bool aActive);
		bool					isActive() const { return mActive; }

		// re-renders only when the state, the cursor blink phase or a setting changed
		ci::gl::Texture2dRef	render(const ci::ivec2& aSize);
		// last rendered texture without re-rendering (UI preview), nullptr before the first render()
		ci::gl::Texture2dRef	getTexture() const {
			if (!mFbo) return nullptr;
			return (mPremultiplied || !mUnpremultiplyProg) ? mFbo->getColorTexture() : mStraightFbo->getColorTexture();
		}
		int						getLineCount() const { return (int)mLines.size(); }

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

	private:
		VDCodeView() = default;

		enum class TokenKind { Plain, Keyword, Type, Function, Uniform, Number, Comment, Preprocessor };
		struct Token { size_t start; size_t length; TokenKind kind; };

		void					tokenize();
		void					ensureFont();
		void					ensureFbos(const ci::ivec2& aSize);
		void					updateScroll(int aVisibleLines, int aVisibleCols);
		void					draw();

		// state
		std::vector<std::string>			mLines;
		std::vector<std::vector<Token>>		mTokens;
		std::vector<int>					mErrorLines;
		int									mCursorLine = 0;	// 0-based internally
		int									mCursorCol = 0;
		bool								mActive = false;
		// scroll follows the cursor, using this view's own font metrics (the editor's differ)
		int									mTopLine = 0;
		int									mLeftCol = 0;

		// settings
		float								mFontSize = 28.0f;
		float								mBackgroundAlpha = 0.0f;
		bool								mShadow = true;
		bool								mLineNumbers = true;
		bool								mPremultiplied = false;
		ci::ColorA							mCurrentLineColor = ci::ColorA(1.0f, 1.0f, 1.0f, 0.07f);
		int									mCurrentLineStyle = LINE_FILL;

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
