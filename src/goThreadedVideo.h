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

#include "ofxThread.h"
#include "ofEvents.h"
#include "string.h"

#include "goVideoPlayer.h"


class goThreadedVideo : public ofxThread {

public:

	goThreadedVideo();
	virtual ~goThreadedVideo();
    //goThreadedVideo(const goThreadedVideo& other);
    //goThreadedVideo& operator=(const goThreadedVideo& other);

	bool				loadMovie(string _name);

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
	float 				getSpeed();
	float 				getDuration();
	bool				getIsMovieDone();
	ofTexture &         getTextureReference();
    goVideoPlayer &     getVideoReference();

	void 				setPosition(float pct);
	void				setPan(float pan); // added gameover
	void				setVolume(int volume);
	void 				setLoopState(int state);
	int					getLoopState();
	void   				setSpeed(float speed);
	void				setFrame(int frame);

	void 				setUseTexture(bool bUse);
	void				draw(int x, int y, int w, int h);
	void 				draw(float x, float y);

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

	bool				verbose;

    ofEvent<string>     loadDone;
    ofEvent<int>        error;

    void psuedoUpdate();
    void psuedoDraw();

protected:



private:

    goVideoPlayer	*	video[2];

	string				name[2];

	int					currentVideo, cueVideo, videoRequests, loopState;

	bool				loaded[2], textured[2], swapVideo, firstLoad;

	void                threadedFunction();
};

#endif
