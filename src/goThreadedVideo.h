/*
 *  goThreadedVideo.h
 *
 *  Created by gameoverx [matthew gingold] on 25/01/10.
 *
 * This class provides a simple interface to threaded loading of video clips
 * whilst maintaing the ability to use textures to draw the video to screen
 * Basically this is done by toggling between two instances of a modified
 * ofVideoPlayer class, in which it is possible to temporarily turn off
 * texture uploading during the load process, but once loaded, force the
 * texture back on again...neat, huh?
 */

#ifndef _GO_THREADED_VIDEO
#define _GO_THREADED_VIDEO

// USE_QUEUE = flip-flop que -> if load is not available we can
// flip the request to the waiting goVideoPlayer instance...
// NB: this is not a que in the true sense of the word as
// successive calls to loadMovie will simply lead to previous
// requests being overwritten...in some instances it might be
// not to allow this to happen in which case comment out this:
//#define USE_QUEUE

// not implemented fully DON"T CHANGE -> working toward full enque system
#define MAX_VIDEOS 2

#include "ofMain.h"
#include "string.h"

#if OF_VERSION < 7
#include "ofxThread.h"
#endif

#include "goVideoPlayer.h"

enum goThreadedVideoError {
    GO_TV_UPDATE_BLOCKED,
    GO_TV_LOAD_BLOCKED,
    GO_TV_ENQUE_BLOCKED,
    GO_TV_DEQUE_BLOCKED,
    GO_TV_MOVIE_ERROR
};

#if OF_VERSION < 7
class goThreadedVideo : public ofxThread {
#else
class goThreadedVideo : public ofThread {
#endif
public:

	goThreadedVideo();
	virtual ~goThreadedVideo();
    //goThreadedVideo(const goThreadedVideo& other);
    //goThreadedVideo& operator=(const goThreadedVideo& other);

	bool				loadMovie(string _name);

    void                setup();
	void				update(); // same as idlemovie
	void 				play();
	void 				stop();
	void				close();
	int 				width, height;
	float  				speed;

	bool 				isFrameNew();
	bool				isPlaying(); // added gameover
	bool				isLoaded();
	bool				isLoading();
	unsigned char * 	getPixels();
	float 				getPosition();
	int                 getVolume();
	float 				getSpeed();
	float 				getDuration();
	bool				getIsMovieDone();
	ofTexture &         getTextureReference();
    goVideoPlayer &     getVideoReference();

    goPixelType         getPixelType();
    void                setPixelType(goPixelType _pixelType);

	void 				setPosition(float pct);
	void				setPan(float pan); // added gameover
	void				setVolume(int volume);
	void 				setLoopState(int state);
	int					getLoopState();
	void   				setSpeed(float speed);
	void				setFrame(int frame, bool noPause = false);

	void 				setUseTexture(bool bUse);
	void                draw();
    void 				draw(float x, float y);
	void				draw(int x, int y, int w, int h);

	void				setAnchorPercent(float xPct, float yPct);
	void				setAnchorPoint(int x, int y);
	void				resetAnchor();

	void 				setPaused(bool bPause);
	void				togglePaused();

	int					getCurrentFrame();
	int					getTotalNumFrames();

	void				firstFrame();
	void				nextFrame();
	void				previousFrame();

	float 				getHeight();
	float 				getWidth();

	string				getCurrentlyPlaying(); // returns name of current mivie playing

	//bool				verbose; //deprecated use ofSetLogLevel(OF_LOG_VERBOSE)

    ofEvent<string>     success;
    ofEvent<int>        error;

    void psuedoUpdate();
    void psuedoDraw();

protected:



private:

    goVideoPlayer	*	video[MAX_VIDEOS];

	string				name[MAX_VIDEOS];

	int					currentVideo, cueVideo, videoRequests, loopState;

	bool				loaded[MAX_VIDEOS], textured[MAX_VIDEOS], swapVideo, firstLoad;

	void                threadedFunction();

	bool                isSetup;

#ifdef USE_QUEUE
    void                pushQueue(string _name);
    void                popQueue();

    goThreadedVideo*    queue;

#endif
	//goThreadedVideo*    queue[MAX_VIDEOS_IN_QUE];
	//int                 top;

};

#endif
