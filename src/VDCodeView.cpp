#include "VDCodeView.h"

#include "cinder/Log.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>

using namespace ci;
using namespace videodromm;

namespace {
	const int TAB_WIDTH = 4;
	// keep this many lines/columns of context around the cursor when scrolling
	const int SCROLL_MARGIN_LINES = 3;
	const int SCROLL_MARGIN_COLS = 8;
	const double BLINK_PERIOD = 1.06;

	const std::unordered_set<std::string>& keywords() {
		static const std::unordered_set<std::string> s = {
			"if", "else", "for", "while", "do", "break", "continue", "return", "discard",
			"switch", "case", "default", "const", "uniform", "in", "out", "inout",
			"struct", "precision", "highp", "mediump", "lowp", "flat", "smooth", "true", "false" };
		return s;
	}
	const std::unordered_set<std::string>& types() {
		static const std::unordered_set<std::string> s = {
			"void", "bool", "int", "uint", "float",
			"vec2", "vec3", "vec4", "bvec2", "bvec3", "bvec4", "ivec2", "ivec3", "ivec4",
			"uvec2", "uvec3", "uvec4", "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4",
			"mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4",
			"sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler2DRect" };
		return s;
	}
	// uniforms/builtins from shadertoy.vd that don't follow the iXxx naming
	const std::unordered_set<std::string>& namedUniforms() {
		static const std::unordered_set<std::string> s = {
			"RENDERSIZE", "TIME", "spectrum", "inputImage", "fragColor", "fragCoord",
			"gl_FragCoord", "gl_FragColor" };
		return s;
	}

	// colours tuned brighter than the editor's one-dark theme, since this is shown over video
	ColorA8u colorFor(int kind) {
		switch (kind) {
		case 1: return ColorA8u(198, 120, 221, 255);	// Keyword
		case 2: return ColorA8u(229, 192, 123, 255);	// Type
		case 3: return ColorA8u(97, 175, 239, 255);		// Function
		case 4: return ColorA8u(240, 113, 120, 255);	// Uniform
		case 5: return ColorA8u(209, 154, 102, 255);	// Number
		case 6: return ColorA8u(140, 146, 158, 255);	// Comment
		case 7: return ColorA8u(86, 182, 194, 255);		// Preprocessor
		default: return ColorA8u(220, 223, 228, 255);	// Plain
		}
	}

	bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
	bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
	bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

	// TextureFont only has glyphs for ASCII; collapse every UTF-8 sequence to one '?' so columns
	// still line up with the editor's (UTF-16) column count for ordinary accented characters
	std::string toAsciiLine(const std::string& aLine, int aCursorCol, int* aVisualCursorCol) {
		std::string out;
		int sourceCol = 0;
		for (size_t i = 0; i < aLine.size(); ++i) {
			unsigned char c = static_cast<unsigned char>(aLine[i]);
			if ((c & 0xC0) == 0x80) continue;	// UTF-8 continuation byte
			if (aVisualCursorCol && sourceCol == aCursorCol) *aVisualCursorCol = (int)out.size();
			if (c == '\t') {
				out.append(TAB_WIDTH - (out.size() % TAB_WIDTH), ' ');
			}
			else if (c == '\r') {
				// CRLF from the browser on Windows
			}
			else {
				out.push_back((c >= 0x80 || c < 0x20) ? '?' : static_cast<char>(c));
			}
			++sourceCol;
		}
		if (aVisualCursorCol && sourceCol <= aCursorCol) *aVisualCursorCol = (int)out.size();
		return out;
	}

	const char* UNPREMULTIPLY_VS = R"(#version 150
uniform mat4 ciModelViewProjection;
in vec4 ciPosition;
in vec2 ciTexCoord0;
out vec2 vUv;
void main() { vUv = ciTexCoord0; gl_Position = ciModelViewProjection * ciPosition; }
)";
	const char* UNPREMULTIPLY_FS = R"(#version 150
uniform sampler2D uTex;
in vec2 vUv;
out vec4 oColor;
void main() {
	vec4 c = texture(uTex, vUv);
	oColor = c.a > 0.0 ? vec4(c.rgb / c.a, c.a) : vec4(0.0);
}
)";
}

void VDCodeView::setState(const std::string& aText, int aLine, int aCol, const std::vector<int>& aErrorLines, bool aActive) {
	mActive = aActive;
	mErrorLines = aErrorLines;
	mLines.clear();
	mCursorLine = std::max(0, aLine - 1);
	mCursorCol = 0;
	size_t start = 0;
	int lineIndex = 0;
	while (true) {
		size_t end = aText.find('\n', start);
		std::string raw = aText.substr(start, end == std::string::npos ? std::string::npos : end - start);
		mLines.push_back(toAsciiLine(raw, aCol, lineIndex == mCursorLine ? &mCursorCol : nullptr));
		if (end == std::string::npos) break;
		start = end + 1;
		++lineIndex;
	}
	mCursorLine = std::min(mCursorLine, (int)mLines.size() - 1);
	tokenize();
	// restart the blink on every update so the cursor is visible while typing
	mCursorOn = true;
	mDirty = true;
}

void VDCodeView::tokenize() {
	mTokens.assign(mLines.size(), {});
	bool inBlockComment = false;
	for (size_t l = 0; l < mLines.size(); ++l) {
		const std::string& s = mLines[l];
		std::vector<Token>& tokens = mTokens[l];
		size_t i = 0;
		const size_t n = s.size();
		if (!inBlockComment) {
			size_t first = s.find_first_not_of(' ');
			if (first != std::string::npos && s[first] == '#') {
				tokens.push_back({ first, n - first, TokenKind::Preprocessor });
				continue;
			}
		}
		while (i < n) {
			if (inBlockComment) {
				size_t end = s.find("*/", i);
				size_t stop = (end == std::string::npos) ? n : end + 2;
				tokens.push_back({ i, stop - i, TokenKind::Comment });
				inBlockComment = (end == std::string::npos);
				i = stop;
				continue;
			}
			char c = s[i];
			if (c == '/' && i + 1 < n && s[i + 1] == '/') {
				tokens.push_back({ i, n - i, TokenKind::Comment });
				break;
			}
			if (c == '/' && i + 1 < n && s[i + 1] == '*') {
				inBlockComment = true;
				size_t end = s.find("*/", i + 2);
				size_t stop = (end == std::string::npos) ? n : end + 2;
				tokens.push_back({ i, stop - i, TokenKind::Comment });
				inBlockComment = (end == std::string::npos);
				i = stop;
				continue;
			}
			if (isDigit(c) || (c == '.' && i + 1 < n && isDigit(s[i + 1]))) {
				size_t j = i + 1;
				while (j < n && (isIdentChar(s[j]) || s[j] == '.'
					|| ((s[j] == '+' || s[j] == '-') && (s[j - 1] == 'e' || s[j - 1] == 'E')))) ++j;
				tokens.push_back({ i, j - i, TokenKind::Number });
				i = j;
				continue;
			}
			if (isIdentStart(c)) {
				size_t j = i + 1;
				while (j < n && isIdentChar(s[j])) ++j;
				const std::string word = s.substr(i, j - i);
				size_t next = s.find_first_not_of(' ', j);
				TokenKind kind = TokenKind::Plain;
				if (keywords().count(word)) kind = TokenKind::Keyword;
				else if (types().count(word)) kind = TokenKind::Type;
				else if (namedUniforms().count(word)
					|| (word.size() > 1 && word[0] == 'i' && std::isupper(static_cast<unsigned char>(word[1])))) kind = TokenKind::Uniform;
				// builtins and the shader's own functions alike
				else if (next != std::string::npos && s[next] == '(') kind = TokenKind::Function;
				tokens.push_back({ i, j - i, kind });
				i = j;
				continue;
			}
			// punctuation/operators: one Plain token per run
			size_t j = i + 1;
			while (j < n && !isIdentChar(s[j]) && s[j] != '/' && s[j] != '.') ++j;
			tokens.push_back({ i, j - i, TokenKind::Plain });
			i = j;
		}
	}
}

void VDCodeView::setFontSize(float aSize) {
	mFontSize = math<float>::clamp(aSize, 8.0f, 160.0f);
	mDirty = true;
}

void VDCodeView::ensureFont() {
	if (mFont && mFontCreatedSize == mFontSize) return;
#if defined( CINDER_MSW )
	const char* fontName = "Consolas";
#else
	const char* fontName = "Menlo";
#endif
	try {
		mFont = gl::TextureFont::create(Font(fontName, mFontSize));
	}
	catch (const std::exception& e) {
		CI_LOG_W("VDCodeView: font " << fontName << " unavailable (" << e.what() << "), using the default font");
		mFont = gl::TextureFont::create(Font(Font::getDefault().getName(), mFontSize));
	}
	mFontCreatedSize = mFontSize;
	// monospace: one advance for every column
	mCharWidth = mFont->measureString("MMMMMMMMMM").x / 10.0f;
	mLineHeight = std::ceil((mFont->getAscent() + mFont->getDescent()) * 1.15f);
	mDirty = true;
}

void VDCodeView::ensureFbos(const ivec2& aSize) {
	ivec2 size(std::max(aSize.x, 16), std::max(aSize.y, 16));
	if (mFbo && mFbo->getSize() == size) return;
	gl::Fbo::Format format;
	format.colorTexture(gl::Texture::Format().internalFormat(GL_RGBA8));
	format.disableDepth();
	mFbo = gl::Fbo::create(size.x, size.y, format);
	mStraightFbo = gl::Fbo::create(size.x, size.y, format);
	if (!mUnpremultiplyProg) {
		try {
			mUnpremultiplyProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(UNPREMULTIPLY_VS).fragment(UNPREMULTIPLY_FS));
		}
		catch (const std::exception& e) {
			CI_LOG_E("VDCodeView: unpremultiply shader failed, sending premultiplied alpha: " << e.what());
		}
	}
	mDirty = true;
}

void VDCodeView::updateScroll(int aVisibleLines, int aVisibleCols) {
	int marginL = std::min(SCROLL_MARGIN_LINES, (aVisibleLines - 1) / 2);
	if (mCursorLine < mTopLine + marginL) mTopLine = mCursorLine - marginL;
	if (mCursorLine > mTopLine + aVisibleLines - 1 - marginL) mTopLine = mCursorLine - aVisibleLines + 1 + marginL;
	mTopLine = std::max(0, std::min(mTopLine, std::max(0, (int)mLines.size() - aVisibleLines)));

	int marginC = std::min(SCROLL_MARGIN_COLS, (aVisibleCols - 1) / 2);
	if (mCursorCol < mLeftCol + marginC) mLeftCol = mCursorCol - marginC;
	if (mCursorCol > mLeftCol + aVisibleCols - 1 - marginC) mLeftCol = mCursorCol - aVisibleCols + 1 + marginC;
	mLeftCol = std::max(0, mLeftCol);
}

gl::Texture2dRef VDCodeView::render(const ivec2& aSize) {
	ensureFont();
	ensureFbos(aSize);
	bool cursorOn = std::fmod(app::getElapsedSeconds(), BLINK_PERIOD) < BLINK_PERIOD * 0.5;
	if (cursorOn != mCursorOn && mActive) {
		mCursorOn = cursorOn;
		mDirty = true;
	}
	if (mDirty) {
		draw();
		mDirty = false;
	}
	return (mPremultiplied || !mUnpremultiplyProg) ? mFbo->getColorTexture() : mStraightFbo->getColorTexture();
}

void VDCodeView::draw() {
	const ivec2 size = mFbo->getSize();
	{
		gl::ScopedFramebuffer scopedFbo(mFbo);
		gl::ScopedViewport scopedViewport(ivec2(0), size);
		gl::ScopedMatrices scopedMatrices;
		gl::setMatricesWindow(size);
		gl::clear(ColorA(0, 0, 0, 0));

		if (mActive && !mLines.empty()) {
			// standard "over" for colour, but alpha accumulates as 1-(1-a)(1-b): together this
			// leaves a correctly premultiplied result on a transparent target
			gl::ScopedBlend scopedBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			gl::ScopedColor scopedColor;

			if (mBackgroundAlpha > 0.0f) {
				gl::color(ColorA(0, 0, 0, mBackgroundAlpha));
				gl::drawSolidRect(Rectf(vec2(0), vec2(size)));
			}

			const float pad = std::round(mFontSize * 0.75f);
			const int digits = std::max(3, (int)std::to_string(mLines.size()).size());
			const float gutter = mLineNumbers ? (digits + 1) * mCharWidth : 0.0f;
			const int visibleLines = std::max(1, (int)((size.y - 2 * pad) / mLineHeight));
			const int visibleCols = std::max(1, (int)((size.x - 2 * pad - gutter) / mCharWidth));
			updateScroll(visibleLines, visibleCols);

			const float textX = pad + gutter;
			const float ascentOffset = (mLineHeight - (mFont->getAscent() + mFont->getDescent())) * 0.5f + mFont->getAscent();
			const Font& font = mFont->getFont();
			std::vector<std::pair<Font::Glyph, vec2>> glyphs;
			std::vector<ColorA8u> colors;
			auto addGlyph = [&](char ch, float x, float baseline, const ColorA8u& color) {
				if (ch == ' ') return;
				glyphs.push_back({ font.getGlyphChar(ch), vec2(x, baseline) });
				colors.push_back(color);
			};

			const int lastLine = std::min((int)mLines.size(), mTopLine + visibleLines);
			for (int l = mTopLine; l < lastLine; ++l) {
				const float top = pad + (l - mTopLine) * mLineHeight;
				const float baseline = top + ascentOffset;
				const bool isError = std::find(mErrorLines.begin(), mErrorLines.end(), l + 1) != mErrorLines.end();
				if (isError) {
					gl::color(ColorA(0.9f, 0.15f, 0.15f, 0.30f));
					gl::drawSolidRect(Rectf(textX - mCharWidth * 0.5f, top, (float)size.x - pad, top + mLineHeight));
					gl::color(ColorA(1.0f, 0.25f, 0.25f, 0.95f));
					gl::drawSolidRect(Rectf(textX - mCharWidth * 0.5f, top, textX - mCharWidth * 0.5f + 3.0f, top + mLineHeight));
				}
				else if (l == mCursorLine && mCurrentLineStyle != LINE_CURSOR_ONLY) {
					const Rectf lineRect(textX - mCharWidth * 0.5f, top, (float)size.x - pad, top + mLineHeight);
					// a faint fill colour (default alpha 0.07) would be invisible as a thin outline
					// or a small gutter box: those two styles use at least half opacity
					ColorA strongColor = mCurrentLineColor;
					strongColor.a = std::max(strongColor.a, 0.5f);
					if (mCurrentLineStyle == LINE_BORDER) {
						gl::color(strongColor);
						gl::drawStrokedRect(lineRect, std::max(1.0f, std::round(mFontSize / 20.0f)));
					}
					else if (mCurrentLineStyle == LINE_NUMBER_FILL && mLineNumbers) {
						gl::color(strongColor);
						gl::drawSolidRect(Rectf(pad - mCharWidth * 0.25f, top, pad + digits * mCharWidth + mCharWidth * 0.25f, top + mLineHeight));
					}
					else {
						// LINE_FILL, or LINE_NUMBER_FILL with line numbers hidden
						gl::color(mCurrentLineColor);
						gl::drawSolidRect(lineRect);
					}
				}
				if (mLineNumbers) {
					const std::string number = std::to_string(l + 1);
					const ColorA8u numberColor = isError ? ColorA8u(255, 110, 110, 255)
						: (l == mCursorLine ? ColorA8u(200, 204, 212, 255) : ColorA8u(110, 118, 130, 255));
					float x = pad + (digits - (int)number.size()) * mCharWidth;
					for (char ch : number) { addGlyph(ch, x, baseline, numberColor); x += mCharWidth; }
				}
				const std::string& s = mLines[l];
				for (const Token& token : mTokens[l]) {
					const ColorA8u color = colorFor((int)token.kind);
					for (size_t k = token.start; k < token.start + token.length; ++k) {
						int col = (int)k - mLeftCol;
						if (col < 0) continue;
						if (col >= visibleCols) break;
						addGlyph(s[k], textX + col * mCharWidth, baseline, color);
					}
				}
			}

			if (mShadow && !glyphs.empty()) {
				// a soft offset copy keeps the code readable over bright footage
				const float offset = std::max(1.0f, std::round(mFontSize / 14.0f));
				std::vector<std::pair<Font::Glyph, vec2>> shadow(glyphs);
				for (auto& g : shadow) g.second += vec2(offset);
				mFont->drawGlyphs(shadow, vec2(0), gl::TextureFont::DrawOptions(), std::vector<ColorA8u>(shadow.size(), ColorA8u(0, 0, 0, 220)));
			}
			if (!glyphs.empty()) {
				mFont->drawGlyphs(glyphs, vec2(0), gl::TextureFont::DrawOptions(), colors);
			}

			const int cursorCol = mCursorCol - mLeftCol;
			if (mCursorOn && mCursorLine >= mTopLine && mCursorLine < lastLine && cursorCol >= 0 && cursorCol <= visibleCols) {
				const float top = pad + (mCursorLine - mTopLine) * mLineHeight;
				const float x = textX + cursorCol * mCharWidth;
				gl::color(ColorA(1, 1, 1, 0.95f));
				gl::drawSolidRect(Rectf(x, top + 2.0f, x + std::max(2.0f, mCharWidth * 0.12f), top + mLineHeight - 2.0f));
			}
		}
	}

	if (!mPremultiplied && mUnpremultiplyProg) {
		gl::ScopedFramebuffer scopedFbo(mStraightFbo);
		gl::ScopedViewport scopedViewport(ivec2(0), size);
		gl::ScopedMatrices scopedMatrices;
		gl::setMatricesWindow(size);
		gl::clear(ColorA(0, 0, 0, 0));
		gl::ScopedBlend scopedBlend(false);
		gl::ScopedGlslProg scopedProg(mUnpremultiplyProg);
		gl::ScopedTextureBind scopedTex(mFbo->getColorTexture(), 0);
		mUnpremultiplyProg->uniform("uTex", 0);
		gl::drawSolidRect(Rectf(vec2(0), vec2(size)));
	}
}
