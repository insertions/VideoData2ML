#pragma once

#include "ofxCcv.h"
#include "ofxOsc.h"
#include "ofxSyphon.h"
#include "video/ofQuickTimePlayer.h"
#include "ofUtils.h"
#include "ofxAvFoundationHLSPlayer.h"

#define LOCAL_VIDEO  0
#define WEBCAM       1
#define REMOTE_VIDEO 2
#define SYPHON_VIDEO 3

#define DEFAULT_VIDEO "http://devimages.apple.com.edgekey.net/samplecode/avfoundationMedia/AVFoundationQueuePlayer_HLS2/master.m3u8"
#define QUALITY_RANGE 5
#define YOUTUBE_DL_PARAMETER_FOR_LIVE_VIDEO "-g"
#define YOUTUBE_DL_PARAMETER_FOR_NON_LIVE_VIDEO "--get-url"
#define INITIAL_LIVE_VIDEO_QUALITY 95
#define INITIAL_NON_LIVE_VIDEO_QUALITY 18


class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    
    void sendOsc();
    void keyPressed(int key);
    
    void next_local_video();
    void previous_local_video();
    void load_local_video_files();
    void setup_input_source ();
    
    void load_webvideo ();
    void load_video_from_url(string);
    void checkSettings();
    string read_url_from_file();
    string translate_video_url(string, int, string);
    string translate_live_video_url(string);
    string translate_non_live_video_url(string);
    
    
    //string from_youtube_url_to_m3u8_url (string);
    string terminal_cmd (const char*);
    
    ofxOscSender osc;
    ofxOscMessage msg;
    ofxSyphonClient syphonClient;
    //ofxSyphonServerDirectory dir;
    
    ofDirectory localVideoFiles;
    
    ofxCcv ccv;
    
    ofVideoGrabber cam;
    ofVideoPlayer localVideo;
    ofxAvFoundationHLSPlayer webVideo;
    
    vector<float> classifierEncoding;
    vector<float> featureEncoding;
    
    string oscHost, oscAddress;
    int oscPort;
    int video_source;
    
    int local_video_index;
    
    bool sending;
    
    time_t lastSaveVideoFile;
};
