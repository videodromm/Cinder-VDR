#include "VDCodeView.h"

#include "cinder/Log.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
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
	// Strudel code is JavaScript: its keywords (mini-notation lives in strings)
	const std::unordered_set<std::string>& jsKeywords() {
		static const std::unordered_set<std::string> s = {
			"const", "let", "var", "function", "return", "if", "else", "for", "while", "do", "break",
			"continue", "switch", "case", "default", "new", "await", "async", "of", "in", "typeof",
			"true", "false", "null", "undefined", "this" };
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
		case 8: return ColorA8u(152, 195, 121, 255);	// String
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

void VDCodeView::setState(const std::string& aText, int aLine, int aCol, const std::vector<int>& aErrorLines, bool aActive, Lang aLang) {
	Doc& doc = mDocs[aLang];
	doc.active = aActive;
	doc.errorLines = aErrorLines;
	doc.lines.clear();
	doc.cursorLine = std::max(0, aLine - 1);
	doc.cursorCol = 0;
	size_t start = 0;
	int lineIndex = 0;
	while (true) {
		size_t end = aText.find('\n', start);
		std::string raw = aText.substr(start, end == std::string::npos ? std::string::npos : end - start);
		doc.lines.push_back(toAsciiLine(raw, aCol, lineIndex == doc.cursorLine ? &doc.cursorCol : nullptr));
		if (end == std::string::npos) break;
		start = end + 1;
		++lineIndex;
	}
	doc.cursorLine = std::min(doc.cursorLine, (int)doc.lines.size() - 1);
	if (aLang == LANG_STRUDEL) tokenizeStrudel(doc);
	else tokenizeGlsl(doc);
	// the cursor shows in the document being typed in
	if (aActive) mFocus = aLang;
	// restart the blink on every update so the cursor is visible while typing
	mCursorOn = true;
	mDirty = true;
}

void VDCodeView::tokenizeGlsl(Doc& aDoc) {
	aDoc.tokens.assign(aDoc.lines.size(), {});
	bool inBlockComment = false;
	for (size_t l = 0; l < aDoc.lines.size(); ++l) {
		const std::string& s = aDoc.lines[l];
		std::vector<Token>& tokens = aDoc.tokens[l];
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

void VDCodeView::tokenizeStrudel(Doc& aDoc) {
	aDoc.tokens.assign(aDoc.lines.size(), {});
	bool inBlockComment = false;
	bool inTemplate = false;	// `...` strings can span lines
	for (size_t l = 0; l < aDoc.lines.size(); ++l) {
		const std::string& s = aDoc.lines[l];
		std::vector<Token>& tokens = aDoc.tokens[l];
		size_t i = 0;
		const size_t n = s.size();
		while (i < n) {
			if (inBlockComment || inTemplate) {
				const char* close = inBlockComment ? "*/" : "`";
				size_t end = s.find(close, i);
				size_t stop = (end == std::string::npos) ? n : end + std::strlen(close);
				tokens.push_back({ i, stop - i, inBlockComment ? TokenKind::Comment : TokenKind::String });
				if (end != std::string::npos) inBlockComment = inTemplate = false;
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
				tokens.push_back({ i, 2, TokenKind::Comment });
				i += 2;
				continue;
			}
			if (c == '`') {
				inTemplate = true;
				tokens.push_back({ i, 1, TokenKind::String });
				++i;
				continue;
			}
			if (c == '"' || c == '\'') {
				size_t j = i + 1;
				while (j < n && s[j] != c) j += (s[j] == '\\' && j + 1 < n) ? 2 : 1;
				size_t stop = std::min(n, j + 1);
				tokens.push_back({ i, stop - i, TokenKind::String });
				i = stop;
				continue;
			}
			if (isDigit(c) || (c == '.' && i + 1 < n && isDigit(s[i + 1]))) {
				size_t j = i + 1;
				while (j < n && (isIdentChar(s[j]) || s[j] == '.')) ++j;
				tokens.push_back({ i, j - i, TokenKind::Number });
				i = j;
				continue;
			}
			if (isIdentStart(c) || c == '$') {
				size_t j = i + 1;
				while (j < n && (isIdentChar(s[j]) || s[j] == '$')) ++j;
				const std::string word = s.substr(i, j - i);
				size_t next = s.find_first_not_of(' ', j);
				TokenKind kind = TokenKind::Plain;
				if (jsKeywords().count(word)) kind = TokenKind::Keyword;
				// note(...), .sound(...): pattern functions and methods
				else if (next != std::string::npos && s[next] == '(') kind = TokenKind::Function;
				tokens.push_back({ i, j - i, kind });
				i = j;
				continue;
			}
			size_t j = i + 1;
			while (j < n && !isIdentChar(s[j]) && s[j] != '/' && s[j] != '.' && s[j] != '"' && s[j] != '\'' && s[j] != '`' && s[j] != '$') ++j;
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

void VDCodeView::updateScroll(Doc& d, int aVisibleLines, int aVisibleCols) {
	int marginL = std::min(SCROLL_MARGIN_LINES, (aVisibleLines - 1) / 2);
	if (d.cursorLine < d.topLine + marginL) d.topLine = d.cursorLine - marginL;
	if (d.cursorLine > d.topLine + aVisibleLines - 1 - marginL) d.topLine = d.cursorLine - aVisibleLines + 1 + marginL;
	d.topLine = std::max(0, std::min(d.topLine, std::max(0, (int)d.lines.size() - aVisibleLines)));

	int marginC = std::min(SCROLL_MARGIN_COLS, (aVisibleCols - 1) / 2);
	if (d.cursorCol < d.leftCol + marginC) d.leftCol = d.cursorCol - marginC;
	if (d.cursorCol > d.leftCol + aVisibleCols - 1 - marginC) d.leftCol = d.cursorCol - aVisibleCols + 1 + marginC;
	d.leftCol = std::max(0, d.leftCol);
}

gl::Texture2dRef VDCodeView::render(const ivec2& aSize) {
	ensureFont();
	ensureFbos(aSize);
	bool cursorOn = std::fmod(app::getElapsedSeconds(), BLINK_PERIOD) < BLINK_PERIOD * 0.5;
	if (cursorOn != mCursorOn && isActive()) {
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

		if (isActive()) {
			// standard "over" for colour, but alpha accumulates as 1-(1-a)(1-b): together this
			// leaves a correctly premultiplied result on a transparent target
			gl::ScopedBlend scopedBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			gl::ScopedColor scopedColor;

			if (mBackgroundAlpha > 0.0f) {
				gl::color(ColorA(0, 0, 0, mBackgroundAlpha));
				gl::drawSolidRect(Rectf(vec2(0), vec2(size)));
			}

			// one area per shown document: GLSL first (left/top), Strudel second
			std::vector<Lang> shown;
			for (int l = 0; l < LANG_COUNT; ++l) if (mDocs[l].active && !mDocs[l].lines.empty()) shown.push_back((Lang)l);
			std::vector<std::pair<Font::Glyph, vec2>> glyphs;
			std::vector<ColorA8u> colors;
			const bool split = shown.size() > 1;
			for (size_t k = 0; k < shown.size(); ++k) {
				Rectf area(vec2(0), vec2(size));
				if (split && mLayout == LAYOUT_STACKED) {
					area.y1 = size.y * (float)k / shown.size();
					area.y2 = size.y * (float)(k + 1) / shown.size();
				}
				else if (split) {
					area.x1 = size.x * (float)k / shown.size();
					area.x2 = size.x * (float)(k + 1) / shown.size();
				}
				drawDoc(mDocs[shown[k]], area, glyphs, colors);
			}
			if (split) {
				// separator between the two documents
				gl::color(ColorA(1, 1, 1, 0.35f));
				if (mLayout == LAYOUT_STACKED) gl::drawSolidRect(Rectf(0.0f, size.y * 0.5f - 1.0f, (float)size.x, size.y * 0.5f + 1.0f));
				else gl::drawSolidRect(Rectf(size.x * 0.5f - 1.0f, 0.0f, size.x * 0.5f + 1.0f, (float)size.y));
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

// Line backgrounds, the cursor and the glyphs (collected, drawn in one call by draw()) of one
// document inside aArea.
void VDCodeView::drawDoc(Doc& d, const Rectf& aArea, std::vector<std::pair<Font::Glyph, vec2>>& glyphs, std::vector<ColorA8u>& colors) {
	const float pad = std::round(mFontSize * 0.75f);
	const int digits = std::max(3, (int)std::to_string(d.lines.size()).size());
	const float gutter = mLineNumbers ? (digits + 1) * mCharWidth : 0.0f;
	const float left = aArea.x1 + pad;
	const float right = aArea.x2 - pad;
	const int visibleLines = std::max(1, (int)((aArea.getHeight() - 2 * pad) / mLineHeight));
	const int visibleCols = std::max(1, (int)((aArea.getWidth() - 2 * pad - gutter) / mCharWidth));
	updateScroll(d, visibleLines, visibleCols);
	const bool focused = &d == &mDocs[mFocus];

	const float textX = left + gutter;
	const float ascentOffset = (mLineHeight - (mFont->getAscent() + mFont->getDescent())) * 0.5f + mFont->getAscent();
	const Font& font = mFont->getFont();
	auto addGlyph = [&](char ch, float x, float baseline, const ColorA8u& color) {
		if (ch == ' ') return;
		glyphs.push_back({ font.getGlyphChar(ch), vec2(x, baseline) });
		colors.push_back(color);
	};

	const int lastLine = std::min((int)d.lines.size(), d.topLine + visibleLines);
	for (int l = d.topLine; l < lastLine; ++l) {
		const float top = aArea.y1 + pad + (l - d.topLine) * mLineHeight;
		const float baseline = top + ascentOffset;
		const bool isError = std::find(d.errorLines.begin(), d.errorLines.end(), l + 1) != d.errorLines.end();
		if (isError) {
			gl::color(ColorA(0.9f, 0.15f, 0.15f, 0.30f));
			gl::drawSolidRect(Rectf(textX - mCharWidth * 0.5f, top, right, top + mLineHeight));
			gl::color(ColorA(1.0f, 0.25f, 0.25f, 0.95f));
			gl::drawSolidRect(Rectf(textX - mCharWidth * 0.5f, top, textX - mCharWidth * 0.5f + 3.0f, top + mLineHeight));
		}
		else if (l == d.cursorLine && mCurrentLineStyle != LINE_CURSOR_ONLY) {
			const Rectf lineRect(textX - mCharWidth * 0.5f, top, right, top + mLineHeight);
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
				gl::drawSolidRect(Rectf(left - mCharWidth * 0.25f, top, left + digits * mCharWidth + mCharWidth * 0.25f, top + mLineHeight));
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
				: (l == d.cursorLine ? ColorA8u(200, 204, 212, 255) : ColorA8u(110, 118, 130, 255));
			float x = left + (digits - (int)number.size()) * mCharWidth;
			for (char ch : number) { addGlyph(ch, x, baseline, numberColor); x += mCharWidth; }
		}
		const std::string& s = d.lines[l];
		for (const Token& token : d.tokens[l]) {
			const ColorA8u color = colorFor((int)token.kind);
			for (size_t k = token.start; k < token.start + token.length; ++k) {
				int col = (int)k - d.leftCol;
				if (col < 0) continue;
				if (col >= visibleCols) break;
				addGlyph(s[k], textX + col * mCharWidth, baseline, color);
			}
		}
	}

	const int cursorCol = d.cursorCol - d.leftCol;
	if (focused && mCursorOn && d.cursorLine >= d.topLine && d.cursorLine < lastLine && cursorCol >= 0 && cursorCol <= visibleCols) {
		const float top = aArea.y1 + pad + (d.cursorLine - d.topLine) * mLineHeight;
		const float x = textX + cursorCol * mCharWidth;
		gl::color(ColorA(1, 1, 1, 0.95f));
		gl::drawSolidRect(Rectf(x, top + 2.0f, x + std::max(2.0f, mCharWidth * 0.12f), top + mLineHeight - 2.0f));
	}
}
