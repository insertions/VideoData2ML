#include "ofApp.h"


#define LOCAL_VIDEO  0
#define WEBCAM       1
#define REMOTE_VIDEO 2

void ofApp::setup() {
    //ofSetWindowShape(640, 480);
    
    oscPort = 6448;
    oscAddress = "/wek/inputs";
    oscHost = "localhost";
    
    osc.setup(oscHost, oscPort);
    sending = false;
    
    ccv.setup("image-net-2012.sqlite3");
    if (!ccv.isLoaded()) return;
    
    video_source = LOCAL_VIDEO;
    //video_source = WEBCAM;
    //video_source = REMOTE_VIDEO;
    
    switch(video_source)
    {
        case LOCAL_VIDEO:
            localVideo.load("movies/fingers.mov");
            localVideo.setLoopState(OF_LOOP_NORMAL);
            localVideo.play();
            break;
        
        case WEBCAM:
            cam.initGrabber(320, 240);
            break;
        
        case REMOTE_VIDEO:
            string youtube_url="https://www.youtube.com/watch?v=wNc5Po2eSc0";
            string m3u8_url = from_youtube_url_to_m3u8_url(youtube_url);
            load_webvideo(m3u8_url);
            break;
    }
}

void ofApp::update() {
    switch(video_source)
    {
        case LOCAL_VIDEO:
            localVideo.update();
            break;
        case WEBCAM:
            cam.update();
            break;
        case REMOTE_VIDEO:
            webVideo.update();
            break;
    }
}

void ofApp::sendOsc() {
    
    switch(video_source)
    {
        case LOCAL_VIDEO:
            featureEncoding = ccv.encode(localVideo, ccv.numLayers()-1);
            break;
        case WEBCAM:
            featureEncoding = ccv.encode(cam, ccv.numLayers()-1);
            break;
        case REMOTE_VIDEO:
            //    JERA
            //    ofxAvFoundationHLSPlayer cannot be sent directly to ccv.encode.
            //    you need to transcode it to an ofImage first to make it work.
            //    however, trancoding from ofxAvFoundationHLSPlayer to ofImage seems bugged due to
            //    lack of support on ofxAvFoundationHLSPlayer image type (currently undentified).
            //    the following code is a little hacking to make it work, that involves:
            //    ofxAvFoundationHLSPlayer > ofTexture > ofPixels > ofImage
            ofTexture webTexture(webVideo.getTexture());
            ofPixels pixels;
            webTexture.readToPixels(pixels);
            ofImage webFrame;
            webFrame.setFromPixels(pixels);
            featureEncoding = ccv.encode(webFrame, ccv.numLayers()-1);
            break;
    }
    
    msg.clear();
    msg.setAddress(oscAddress);
    for (int i=0; i<featureEncoding.size(); i++) {
        msg.addFloatArg(featureEncoding[i]);
    }
    osc.sendMessage(msg);
    
}

void ofApp::keyPressed(int key) {
    if (key==' ') {
        sending = !sending;
    }
}

void ofApp::draw() {
    
    ofSetColor(255);

    switch(video_source)
    {
        case LOCAL_VIDEO:
            localVideo.draw(0, 0, ofGetWidth(), ofGetHeight());
            break;
        case WEBCAM:
            cam.draw(0, 0, ofGetWidth(), ofGetHeight());
            break;
        case REMOTE_VIDEO:
            webVideo.draw(0, 0, ofGetWidth(), ofGetHeight());
            break;
    }
    
    if (!ccv.isLoaded()) {
        ofDrawBitmapString("Network file not found! Check your data folder to make sure it exists.", 10, 20);
        return;
    }
    
    ofSetColor(255);
    
    if (sending) {
        ofDrawBitmapString("sending "+ofToString(msg.getNumArgs())+" values", 10, 70);
        ofDrawBitmapString("to "+oscHost+" port "+ofToString(oscPort)+", address \""+oscAddress+"\"", 10, 20);
        sendOsc();
    }
    else
        ofDrawBitmapString("Press space to start sending inputs", 10, 20);
    
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 10, 50);
}

//--------------------------------------------------------------
string ofApp::from_youtube_url_to_m3u8_url (string youtube_url) {
    
    int quality = 95;
    string m3u8_url = "";
    
    //if quality 95 is too high...
    while (m3u8_url.empty()) {
        stringstream cmd;
        
        cmd << "/usr/local/bin/youtube-dl -f " << quality <<" -g "  << youtube_url;
        m3u8_url = terminal_cmd(cmd.str().c_str());
        m3u8_url.pop_back();
        
        if (!m3u8_url.empty())
            //tries to find the last occurrence of https (the valid one)
            m3u8_url = m3u8_url.substr(m3u8_url.rfind("https://"), m3u8_url.size());
        
        quality--;
    }
    
    return m3u8_url;
}


//--------------------------------------------------------------
//code retrieved from:
//http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
//--------------------------------------------------------------
string ofApp::terminal_cmd (const char* cmd) {
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

//--------------------------------------------------------------
void ofApp::load_webvideo(string url_in_m3u8) {
    webVideo.stop();
    webVideo = *(new ofxAvFoundationHLSPlayer());
    webVideo.load(url_in_m3u8);
    cout << "video loaded! " << endl;
}

