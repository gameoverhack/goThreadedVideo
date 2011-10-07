/*
 *  goThreadedVideo.cpp
 *
 *  Created by gameoverx [matthew gingold] on 25/01/10.
 *
 */

#include "goThreadedVideo.h"

// this static var allows us to block simultaneous
// calls to quicktime to open a movie - although
// it's meant to be multithreaded (on a mac anyways)
// tests have shown me that calls made within 200 milliseconds
// by seperate instances tend to cause random crashes

static bool loading = false;

//--------------------------------------------------------------
goThreadedVideo::goThreadedVideo() {
    isSetup = false;
}

//--------------------------------------------------------------
goThreadedVideo::~goThreadedVideo() {

    if (isSetup) {
        // close the videos and clean up RAM
        for (int i = 0; i < MAX_VIDEOS; i++) {
            delete video[i];
            video[i] = NULL;
        }
    }

}

void goThreadedVideo::setup() {

	verbose = false;

    for (int i = 0; i < MAX_VIDEOS; i++) {
        video[i] = new goVideoPlayer();		// set up the two video players (NB: this won't work with standard ofVideoPlayer!)
        name[i] = "";
        loaded[i] = true;		// for the first load we pretend that actually something is already loaded
        textured[i] = true;	// otherwise the whole toggling thing doesn;t work...
    }

	currentVideo = cueVideo = 0;		// used to keep track of toggling between 0 and 1 in the video[] player array

	videoRequests = 1;

	firstLoad = true;					// but we use this firstLoad variable to let us know that's what happening
	swapVideo = false;
	isSetup = true;
}

//--------------------------------------------------------------
bool goThreadedVideo::loadMovie(string _name) {

    if (!isSetup) setup();

	// make sure that...
	if(!isThreadRunning()				// ...the thread is not already running...
	   && loaded[cueVideo]				// ...that the cueVideo is loaded...
	   && loaded[currentVideo]			// ...that the currentVideo is loaded...
	   && textured[cueVideo]			// ...and that both
	   && textured[currentVideo]		// are textured...
	   && !loading                    // ...and that no other instance is also loading (this is optional, but recommended ;-)
	   ) {

		loading = true;					// flag all instances that we are loading (see above)

		videoRequests++;				// use a counter to switch between cue and current video[] player indexs

		if(videoRequests % MAX_VIDEOS != currentVideo || firstLoad) {	// ie., this toggles which of the video[] players is
			// the cue or current index (also handles a "firstload")

			cueVideo = videoRequests % MAX_VIDEOS;				// set the cueVideo index

			name[cueVideo] = _name;								// set the video to load name

			ofLog(OF_LOG_VERBOSE, "Loading: " + _name);

			video[cueVideo]->setUseTexture(false);				// make sure the texture is turned off or else the thread crashes: openGL is thread safe!

			loaded[cueVideo] = false;							// set flags for loaded (ie., quicktime loaded)
			textured[cueVideo] = false;							// and textured (ie., "forced texture" once the thread is stop)

			startThread(false, false);							// start the thread

			return true;										// NB: this does not let us know that the video is loaded; just that it has STARTED loading...

		} else {												// ELSE: the video has not been textured and/or updated yet (ie., we
			// have to wait one cycle between the thread terminating and the texture
			// being "forced" back on - otherwise openGL + thread = crash...

			ofLog(OF_LOG_VERBOSE, "Load BLOCKED by reload before draw/update");
			int err = GO_TV_UPDATE_BLOCKED;
			ofNotifyEvent(error, err);
			return false;
		}

	} else {													// ELSE: either we're already loading in this instance OR another instance

		ofLog(OF_LOG_VERBOSE, "Load BLOCKED by already thread loading...attempting to enqueue");
#ifdef USE_QUEUE
		pushQueue(_name);
		return true;
#else
		int err = GO_TV_LOAD_BLOCKED;
        ofNotifyEvent(error, err);
		return false;
#endif
	}
}

#ifdef USE_QUEUE

// flip-flop que -> if load is not available we can
// flip the request to the waiting goVideoPlayer instance...
// see note at top of goThreadedVideo.h
// thanks to briangibson for this suggestion:
// http://forum.openframeworks.cc/index.php/topic,1335.msg32416.html#msg32416

//--------------------------------------------------------------
void goThreadedVideo::pushQueue(string _name) {

    if(queue != this){
        ofLog(OF_LOG_VERBOSE, "...SUCCESS enqueing: " + _name);
        cueVideo = videoRequests % MAX_VIDEOS;
        name[cueVideo] = _name;
        queue = this;
    } else {
        ofLog(OF_LOG_VERBOSE, "...FAILED to enqueue: " + _name + "(discarding)");
        int err = GO_TV_ENQUE_BLOCKED;
        ofNotifyEvent(error, err);
    }

}

//--------------------------------------------------------------
void goThreadedVideo::popQueue() {

    if(queue != NULL){
        ofLog(OF_LOG_VERBOSE, "...ATTEMPTING deque...");
        if(queue->loadMovie(queue->name[queue->cueVideo])) {
            ofLog(OF_LOG_VERBOSE, "...SUCCESS deque");
            queue = NULL;
        } else {
            ofLog(OF_LOG_VERBOSE, "...FAILED to dequeue");
            int err = GO_TV_DEQUE_BLOCKED;
            ofNotifyEvent(error, err);
        }
    }

}
#endif

//--------------------------------------------------------------
void goThreadedVideo::threadedFunction() {

	// this is where we load the video in a thread
	// whilst the texture is turned off...

	stopThread();										// immediately stop the thread

	bool ok = video[cueVideo]->loadMovie(name[cueVideo], true);	// load the movie
	if (ok) {
		video[cueVideo]->play();							// and start playing it
		video[cueVideo]->setLoopState(OF_LOOP_NORMAL);
		loaded[cueVideo] = true;							// set flag that the video is loaded
	} else {
		int err = GO_TV_MOVIE_ERROR;
		ofNotifyEvent(error, err);
	}


}

//--------------------------------------------------------------
void goThreadedVideo::update() {

	// make sure the thread has finished and that we have a loaded 'cue' video that has NOT been textured
	if(!isThreadRunning() && loaded[cueVideo] && !textured[cueVideo]) {

		// setup the videos texture and force the
		// first frame of the texture to upload
		// (this required changes to ofVideoPlayer, see goVideoPlayer)

		video[cueVideo]->forceTextureUpload();
		video[cueVideo]->setUseTexture(true);

		// unless it's the very fist load, lets pause the current video
		if(!firstLoad) {
			video[currentVideo]->setPaused(true);
		} else firstLoad = false;

		// force an update of the cue'd videos frame
		video[cueVideo]->update();

		// update texture flags and set swapVideo flag to do a
		// "flickerless" change between cue and current video[] players
		textured[cueVideo] = true;
		swapVideo = true;

	}

	// if we have a loaded video and the thread is not running then update the current video player
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->update();
	}
#ifdef USE_QUE
	if(!loading) popQueue();
#endif
}

//--------------------------------------------------------------
void goThreadedVideo::draw(float x, float y) {
	draw(x, y, width, height);
}

//--------------------------------------------------------------
void goThreadedVideo::draw(int x, int y, int w, int h) {

	// make sure we are both loaded and textured
	if(loaded[currentVideo] && textured[currentVideo]) {

		if(swapVideo) {									// the video is loaded but we now toggle the cue and current video[] player index's

			// to get flickerless loading we draw one frame using the cue index
			// before swapping indexs between cue and current in the video[] player array

			video[cueVideo]->draw(x, y, w, h);
			video[(videoRequests + 1) % MAX_VIDEOS]->close();

			currentVideo = cueVideo;
			name[currentVideo] = name[cueVideo];

			width = video[currentVideo]->width;
			height = video[currentVideo]->height;
			speed = video[currentVideo]->speed;

			swapVideo = false;
			loading = false;							// this is the end of a complete "load" - let's free all instances to load now!!

            ofNotifyEvent(success, name[currentVideo]);

		} else {

			// just draw the current videos' frame to screen
			video[currentVideo]->draw(x, y, w, h);

		}

	}
}

//--------------------------------------------------------------
void goThreadedVideo::psuedoUpdate() {

    if(!isThreadRunning() && loaded[cueVideo] && !textured[cueVideo]) {

		// setup the videos texture and force the
		// first frame of the texture to upload
		// (this required changes to ofVideoPlayer, see goVideoPlayer)

		video[cueVideo]->forceTextureUpload();
		video[cueVideo]->setUseTexture(true);

		// unless it's the very fist load, lets pause the current video
		if(!firstLoad) {
			video[currentVideo]->setPaused(true);
		} else firstLoad = false;

		// force an update of the cue'd videos frame
		//video[cueVideo]->update();

		// update texture flags and set swapVideo flag to do a
		// "flickerless" change between cue and current video[] players
		textured[cueVideo] = true;
		swapVideo = true;

	}

#ifdef USE_QUE
	if(!loading) popQueue();
#endif

}

//--------------------------------------------------------------
void goThreadedVideo::psuedoDraw() {

    if(loaded[currentVideo] && textured[currentVideo]) {

		if(swapVideo) {									// the video is loaded but we now toggle the cue and current video[] player index's

			currentVideo = cueVideo;
			name[currentVideo] = name[cueVideo];

			width = video[currentVideo]->width;
			height = video[currentVideo]->height;
			speed = video[currentVideo]->speed;

			swapVideo = false;
			loading = false;							// this is the end of a complete "load" - let's free all instances to load now!!

            ofNotifyEvent(success, name[currentVideo]);

		}
	}
}

/*
 *
 * Wrappers around standard ofVideoPlayer calls are below...
 * ...I just did up most of these very quickly and haven't
 * tested them all - let me know how it goes :-)
 *
 */

//--------------------------------------------------------------
void goThreadedVideo::play() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->play();
	}
}

//--------------------------------------------------------------
void goThreadedVideo::stop() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->stop();
	}
}

//--------------------------------------------------------------
void goThreadedVideo::close() {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		video[0]->close();
		video[1]->close();
		currentVideo = cueVideo = 0;		// used to keep track of toggling between 0 and 1 in the video[] player array
		videoRequests = 1;
		loaded[0] = loaded[1] = true;		// for the first load we pretend that actually something is already loaded
		textured[0] = textured[0] = true;	// otherwise the whole toggling thing doesn;t work...
		firstLoad = true;					// but we use this firstLoad variable to let us know that's what happening
		swapVideo = false;
		loading = false;
	//}
}

//--------------------------------------------------------------
bool goThreadedVideo::isFrameNew() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->isFrameNew();
	} else return false;
}

//--------------------------------------------------------------
unsigned char * goThreadedVideo::getPixels() {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getPixels();
	//} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
float goThreadedVideo::getPosition() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getPosition();
	} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
float goThreadedVideo::getSpeed() {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getSpeed();
	//} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
float goThreadedVideo::getDuration() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getDuration();
	} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
int goThreadedVideo::getCurrentFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getCurrentFrame();
	} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
int goThreadedVideo::getTotalNumFrames() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getTotalNumFrames();
	} else return 0;
}

//--------------------------------------------------------------
void goThreadedVideo::setPosition(float pct){
	//if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setPosition(pct);
	//}
}

//--------------------------------------------------------------
void goThreadedVideo::setVolume(int volume) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setVolume(volume);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::setPan(float pan) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setPan(pan);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::setLoopState(int state) {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		loopState = state;
		video[currentVideo]->setLoopState(state);
	//}
}

//--------------------------------------------------------------
int goThreadedVideo::getLoopState() {
	return loopState;
}

//--------------------------------------------------------
void goThreadedVideo::setPixelType(goPixelType _pixelType) {
    if (!isSetup) setup();
    video[0]->setPixelType(_pixelType);
    video[1]->setPixelType(_pixelType);
}

//--------------------------------------------------------
goPixelType goThreadedVideo::getPixelType() {
    return video[currentVideo]->getPixelType();;
}

//--------------------------------------------------------------
void goThreadedVideo::setSpeed(float speed) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setSpeed(speed);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::setFrame(int frame) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setFrame(frame);
	}
}

//--------------------------------------------------------------
bool goThreadedVideo::getIsMovieDone() {
	if(loaded[currentVideo] && textured[currentVideo] && !firstLoad) {
		return video[currentVideo]->getIsMovieDone();
	} else return false;
}

//--------------------------------------------------------------
void goThreadedVideo::setUseTexture(bool bUse) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setUseTexture(bUse);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::setAnchorPercent(float xPct, float yPct) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setAnchorPercent(xPct, yPct);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::setAnchorPoint(int x, int y) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setAnchorPoint(x, y);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::resetAnchor() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->resetAnchor();
	}
}

//--------------------------------------------------------------
void goThreadedVideo::setPaused(bool bPause) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setPaused(bPause);
	}
}

//--------------------------------------------------------------
void goThreadedVideo::togglePaused() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->togglePaused();
	}
}

//--------------------------------------------------------------
void goThreadedVideo::firstFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->firstFrame();
	}
}

//--------------------------------------------------------------
void goThreadedVideo::nextFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->nextFrame();
	}
}

//--------------------------------------------------------------
void goThreadedVideo::previousFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->previousFrame();
	}
}

//--------------------------------------------------------------
float goThreadedVideo::getHeight() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getHeight();
	} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
float goThreadedVideo::getWidth() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getWidth();
	} else return 0; // maybe should be -1 so we know something is loading??
}

//--------------------------------------------------------------
ofTexture & goThreadedVideo::getTextureReference() {
    //if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getTextureReference();
	//} else return 0
}

//--------------------------------------------------------------
goVideoPlayer & goThreadedVideo::getVideoReference() {
    //if(loaded[currentVideo] && textured[currentVideo]) {
		return *video[currentVideo];
	//} else return 0;
}

//--------------------------------------------------------------
bool goThreadedVideo::isPlaying() {
	if(loaded[currentVideo] && textured[currentVideo] && !firstLoad) {
		return video[currentVideo]->isPlaying();
	} else return false;
}

//--------------------------------------------------------------
string goThreadedVideo::getCurrentlyPlaying() {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		return name[currentVideo];
	//} return "";
}

//--------------------------------------------------------------
bool goThreadedVideo::isLoaded() {
	if (!loading) {
		return loaded[currentVideo] && textured[currentVideo];
	} else return false;
}

//--------------------------------------------------------------
bool goThreadedVideo::isLoading() {
	return loading;
}

/*goThreadedVideo::goThreadedVideo(const goThreadedVideo& other)
{
    //copy ctor
}

goThreadedVideo& goThreadedVideo::operator=(const goThreadedVideo& rhs)
{
    if (this == &rhs) return *this; // handle self assignment
    //assignment operator
    return *this;
}
*/
