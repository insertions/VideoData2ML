#pragma once

#include "ofxCcv.h"
#include "ofxOsc.h"
#include "video/ofQuickTimePlayer.h"
#include "ofUtils.h"
#include "ofxAvFoundationHLSPlayer.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    
    void sendOsc();
    void keyPressed(int key);
    
    void load_webvideo (string);
    string from_youtube_url_to_m3u8_url (string);
    string terminal_cmd (const char*);
    
    ofxOscSender osc;
    ofxOscMessage msg;
    
    ofxCcv ccv;
    
    ofVideoGrabber cam;
    ofVideoPlayer localVideo;
    ofxAvFoundationHLSPlayer webVideo;
    
    vector<float> classifierEncoding;
    vector<float> featureEncoding;
    
    string oscHost, oscAddress;
    int oscPort;
    int video_source;
    
    bool sending;
};
