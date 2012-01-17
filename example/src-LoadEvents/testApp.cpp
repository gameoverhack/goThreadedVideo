#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

    ofSetLogLevel(OF_LOG_VERBOSE);
    
    video.setup();
    
    dir.allowExt("mov");
	numberOfMovies = dir.listDir("/Volumes/Artuad/vjMedia/dancing/dancing");
	count = 0;

}

//--------------------------------------------------------------
void testApp::update(){

    ofSetLogLevel(OF_LOG_VERBOSE);
    
    ofBackground(0,0,0);
	//if(ofGetFrameNum() % 50 == 0) loadNextVideo(); // load a video every 150 frames

	video.update();

}

//--------------------------------------------------------------
void testApp::loadNextVideo() {

    video.loadMovie(dir.getPath(count));
    video.setLoopState(OF_LOOP_PALINDROME); // Should work here now too
    
    // add listeners to the threaded video
    ofAddListener(video.success, this, &testApp::loaded); // this fires when the video is ACTUALLY loaded and ready
    ofAddListener(video.error, this, &testApp::error); // this fires if there's a problem loading
    
    if (count+1 < numberOfMovies) {
        count++;
    } else count = 0;

}

//--------------------------------------------------------------
void testApp::loaded(string & path){
    ofLogVerbose() << "Video is actually loaded:" << path;
    //video.setLoopState(OF_LOOP_PALINDROME); // used to only work here
    
    // remember to remove the listeners
    ofRemoveListener(video.success, this, &testApp::loaded);
    ofRemoveListener(video.error, this, &testApp::error);
}

//--------------------------------------------------------------
void testApp::error(goVideoError & err){
    ofLogVerbose() << "Error loading video with error code: " << err; // see enum goVideoError for codes
    
    // remember to remove the listeners
    ofRemoveListener(video.success, this, &testApp::loaded);
    ofRemoveListener(video.error, this, &testApp::error);
}

//--------------------------------------------------------------
void testApp::draw(){

    ofSetColor(255,255,255);
	video.draw(0,0);

	string msg = "goThreadedVideo lets you load movies without blocking the main thread.\n";
    msg += "Once they are loaded they are just like a normal movie...\n ie., the threading only happens during the load.\n\n";
    msg += "This app changes movies in a directory every 50 frames...\n";
    msg += "Hold down the SPACE key to try and load movies as fast as possible...\n";
    msg += "FPS: " + ofToString(ofGetFrameRate());

    ofSetColor(0,255,0);
    ofDrawBitmapString(msg, 20, ofGetHeight() - 7*14);

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
        case ' ':
        loadNextVideo(); // load video's as fast as possible
        break;
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){

}
