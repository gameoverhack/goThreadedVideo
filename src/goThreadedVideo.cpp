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

goThreadedVideo::goThreadedVideo() {

	verbose = false;

	video[0] = new goVideoPlayer();		// set up the two video players (NB: this won't work with standard ofVideoPlayer!)
	video[1] = new goVideoPlayer();

	currentVideo = cueVideo = 0;		// used to keep track of toggling between 0 and 1 in the video[] player array

	videoRequests = 1;

	loaded[0] = loaded[1] = true;		// for the first load we pretend that actually something is already loaded
	textured[0] = textured[0] = true;	// otherwise the whole toggling thing doesn;t work...

	firstLoad = true;					// but we use this firstLoad variable to let us know that's what happening
	swapVideo = false;
}

goThreadedVideo::~goThreadedVideo() {

	// close the videos and clean up RAM
    name[0] = name[1] = "";
    videoRequests = NULL;

	delete video[0];
	delete video[1];

	video[0] = video[1] = NULL;

}

bool goThreadedVideo::loadMovie(string _name) {

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

		if(videoRequests % 2 != currentVideo || firstLoad) {	// ie., this toggles which of the video[] players is
			// the cue or current index (also handles a "firstload")

			cueVideo = videoRequests % 2;						// set the cueVideo index

			name[cueVideo] = _name;								// set the video to load name

			cout << "Loading: " << _name << endl;

			video[cueVideo]->setUseTexture(false);				// make sure the texture is turned off or else the thread crashes: openGL is thread safe!

			loaded[cueVideo] = false;							// set flags for loaded (ie., quicktime loaded)
			textured[cueVideo] = false;							// and textured (ie., "forced texture" once the thread is stop)

			startThread(false, false);							// start the thread

			return true;										// NB: this does not let us know that the video is loaded; just that it has STARTED loading...

		} else {												// ELSE: the video has not been textured and/or updated yet (ie., we
			// have to wait one cycle between the thread terminating and the texture
			// being "forced" back on - otherwise openGL + thread = crash...

			if(verbose) cout << "Load BLOCKED by reload before draw/update" << endl;
			int c = 0;
			ofNotifyEvent(error, c);
			return false;
		}

	} else {													// ELSE: either we're already loading in this instance OR another instance

		if(verbose) cout << "Load BLOCKED by already thread loading" << endl;
		int c = 1;
        ofNotifyEvent(error, c);
		return false;
	}
}

void goThreadedVideo::threadedFunction() {

	// this is where we load the video in a thread
	// whilst the texture is turned off...

	stopThread();										// immediately stop the thread

	video[cueVideo]->loadMovie(name[cueVideo]);			// load the movie
	video[cueVideo]->play();							// and start playing it
	video[cueVideo]->setLoopState(OF_LOOP_NORMAL);
	loaded[cueVideo] = true;							// set flag that the video is loaded

}

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

}

void goThreadedVideo::psuedoUpdate()
{
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
}

void goThreadedVideo::psuedoDraw()
{
    if(loaded[currentVideo] && textured[currentVideo]) {

		if(swapVideo) {									// the video is loaded but we now toggle the cue and current video[] player index's

			// to get flickerless loading we draw one frame using the cue index
			// before swapping indexs between cue and current in the video[] player array

			//video[cueVideo]->draw(0, 0, width, height);
			//video[(videoRequests + 1) % 2]->close();

			currentVideo = cueVideo;
			name[currentVideo] = name[cueVideo];

			width = video[currentVideo]->width;
			height = video[currentVideo]->height;
			speed = video[currentVideo]->speed;

			swapVideo = false;
			loading = false;							// this is the end of a complete "load" - let's free all instances to load now!!

            ofNotifyEvent(loadDone, name[currentVideo]);

		}

	}
}

void goThreadedVideo::draw(float x, float y) {
	draw(x, y, width, height);
}

void goThreadedVideo::draw(int x, int y, int w, int h) {

	// make sure we are both loaded and textured
	if(loaded[currentVideo] && textured[currentVideo]) {

		if(swapVideo) {									// the video is loaded but we now toggle the cue and current video[] player index's

			// to get flickerless loading we draw one frame using the cue index
			// before swapping indexs between cue and current in the video[] player array

			video[cueVideo]->draw(x, y, w, h);
			video[(videoRequests + 1) % 2]->close();

			currentVideo = cueVideo;
			name[currentVideo] = name[cueVideo];

			width = video[currentVideo]->width;
			height = video[currentVideo]->height;
			speed = video[currentVideo]->speed;

			swapVideo = false;
			loading = false;							// this is the end of a complete "load" - let's free all instances to load now!!

            ofNotifyEvent(loadDone, name[currentVideo]);

		} else {

			// just draw the current videos' frame to screen
			video[currentVideo]->draw(x, y, w, h);

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


void goThreadedVideo::play() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->play();
	}
}
void goThreadedVideo::stop() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->stop();
	}
}
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
	//}
}
bool goThreadedVideo::isFrameNew() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->isFrameNew();
	} else return false;
}
unsigned char * goThreadedVideo::getPixels() {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getPixels();
	//} else return 0; // maybe should be -1 so we know something is loading??
}

float goThreadedVideo::getPosition() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getPosition();
	} else return 0; // maybe should be -1 so we know something is loading??
}

float goThreadedVideo::getSpeed() {
	//if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getSpeed();
	//} else return 0; // maybe should be -1 so we know something is loading??
}

float goThreadedVideo::getDuration() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getDuration();
	} else return 0; // maybe should be -1 so we know something is loading??
}

int goThreadedVideo::getCurrentFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getCurrentFrame();
	} else return 0; // maybe should be -1 so we know something is loading??
}

int goThreadedVideo::getTotalNumFrames() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getTotalNumFrames();
	} else return 0;
}

void goThreadedVideo::setPosition(float pct){
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setPosition(pct);
	}
}

void goThreadedVideo::setVolume(int volume) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setVolume(volume);
	}
}

void goThreadedVideo::setPan(float pan) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setPan(pan);
	}
}
void goThreadedVideo::setLoopState(int state) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setLoopState(state);
	}
}

void goThreadedVideo::setSpeed(float speed) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setSpeed(speed);
	}
}

void goThreadedVideo::setFrame(int frame) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setFrame(frame);
	}
}

bool goThreadedVideo::getIsMovieDone() {
	if(loaded[currentVideo] && textured[currentVideo] && !firstLoad) {
		return video[currentVideo]->getIsMovieDone();
	} else return false;
}

void goThreadedVideo::setUseTexture(bool bUse) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setUseTexture(bUse);
	}
}

void goThreadedVideo::setAnchorPercent(float xPct, float yPct) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setAnchorPercent(xPct, yPct);
	}
}

void goThreadedVideo::setAnchorPoint(int x, int y) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setAnchorPoint(x, y);
	}
}

void goThreadedVideo::resetAnchor() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->resetAnchor();
	}
}

void goThreadedVideo::setPaused(bool bPause) {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->setPaused(bPause);
	}
}

void goThreadedVideo::firstFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->firstFrame();
	}
}
void goThreadedVideo::nextFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->nextFrame();
	}
}

void goThreadedVideo::previousFrame() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		video[currentVideo]->previousFrame();
	}
}

float goThreadedVideo::getHeight() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getHeight();
	} else return 0; // maybe should be -1 so we know something is loading??
}

float goThreadedVideo::getWidth() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getWidth();
	} else return 0; // maybe should be -1 so we know something is loading??
}

ofTexture & goThreadedVideo::getTextureReference() {
    //if(loaded[currentVideo] && textured[currentVideo]) {
		return video[currentVideo]->getTextureReference();
	//} else return 0;
}

goVideoPlayer & goThreadedVideo::getVideoReference() {
    //if(loaded[currentVideo] && textured[currentVideo]) {
		return *video[currentVideo];
	//} else return 0;
}


bool goThreadedVideo::isPlaying() {
	if(loaded[currentVideo] && textured[currentVideo] && !firstLoad) {
		return video[currentVideo]->isPlaying();
	} else return false;
}

string goThreadedVideo::getCurrentlyPlaying() {
	if(loaded[currentVideo] && textured[currentVideo]) {
		return name[currentVideo];
	} return "";
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
