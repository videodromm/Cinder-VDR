#pragma once
// GPU time of one render stage (2026-11-12, studies/mixette-fx.md step 0), shown in the Fps window.
// GL_TIME_ELAPSED queries in a ring of 3, read only once GL_QUERY_RESULT_AVAILABLE says so: never waits for the GPU
// (cinder's gl::QueryTimeSwapped reads GL_QUERY_RESULT directly, which blocks if the result isn't there yet).
// GL_TIME_ELAPSED queries can't nest: time stages one after the other, never one inside another.
// Needs a GL context: the queries are created on the first begin().
#include "cinder/gl/gl.h"

namespace videodromm {
	class VDGpuTimer {
	public:
		~VDGpuTimer() { if (mCreated) glDeleteQueries(RING, mQueries); }
		void begin() {
			if (!mCreated) { glGenQueries(RING, mQueries); mCreated = true; }
			poll();
			// all queries still in flight (GPU more than RING stages behind): skip this measure rather than wait
			if (mPending[mIndex]) return;
			glBeginQuery(GL_TIME_ELAPSED, mQueries[mIndex]);
			mActive = true;
		}
		void end() {
			if (!mActive) return;
			glEndQuery(GL_TIME_ELAPSED);
			mPending[mIndex] = true;
			mIndex = (mIndex + 1) % RING;
			mActive = false;
		}
		// smoothed milliseconds (0 until a first result), and the last raw value
		double ms() const { return mSmoothedMs; }
		double lastMs() const { return mLastMs; }
		// seconds since the last result: a stage that stopped running keeps its old value, the UI can grey it out
		double ageSeconds() const { return mLastResultAt < 0.0 ? 1e9 : ci::app::getElapsedSeconds() - mLastResultAt; }
	private:
		static const int RING = 3;
		void poll() {
			for (int i = 0; i < RING; i++) {
				if (!mPending[i]) continue;
				GLint available = 0;
				glGetQueryObjectiv(mQueries[i], GL_QUERY_RESULT_AVAILABLE, &available);
				if (!available) continue;
				GLuint64 ns = 0;
				glGetQueryObjectui64v(mQueries[i], GL_QUERY_RESULT, &ns);
				mPending[i] = false;
				mLastMs = (double)ns * 1e-6;
				mSmoothedMs = (mLastResultAt < 0.0) ? mLastMs : mSmoothedMs * 0.9 + mLastMs * 0.1;
				mLastResultAt = ci::app::getElapsedSeconds();
			}
		}
		GLuint	mQueries[RING] = { 0, 0, 0 };
		bool	mPending[RING] = { false, false, false };
		bool	mCreated = false;
		bool	mActive = false;
		int		mIndex = 0;
		double	mSmoothedMs = 0.0;
		double	mLastMs = 0.0;
		double	mLastResultAt = -1.0;
	};
}
