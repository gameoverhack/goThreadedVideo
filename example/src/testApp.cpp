#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

    ofSetLogLevel(OF_LOG_VERBOSE);
    dir.allowExt("mov");
	numberOfMovies = dir.listDir("<put your directory with videos here>");
	count = 0;

}

//--------------------------------------------------------------
void testApp::update(){

    ofBackground(0,0,0);
	if(ofGetFrameNum() % 50 == 0) loadNextVideo(); // load a video every 150 frames

	video.update();

}

void testApp::loadNextVideo() {

    video.loadMovie(dir.getPath(count));

    if (count+1 < numberOfMovies) {
        count++;
    } else count = 0;

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
