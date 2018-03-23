#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup() {
    //ofSetWindowShape(640, 480);
    
    ofSetWindowTitle("Video Data 2 ML");
    //ofDisableAlphaBlending();
    
    oscPort = 6448;
    //oscAddress = "/wek/inputs";
    oscHost = "localhost";
    
    osc.setup(oscHost, oscPort);
    sending = false;

    //ccv.setup("image-net-2012.sqlite3");
    //ccv.setupFace("face.sqlite3");
    
    /*
    if (!ccv.isLoaded())  {
        cout << "ccv not loaded!"<<endl;
        return;
    }
     */
    
    input_source = LOCAL_VIDEO;
    //input_source = WEBCAM;
    //input_source = REMOTE_VIDEO;
    //input_source = SYPHON_VIDEO;
    
    output_source = WEKINATOR;
    //output_source = FACE_TRACKER;
    //output_source = CLASSIFIER;
    
    load_local_video_files();
    
    setup_input_source();
    setup_output_source();
    
}

//--------------------------------------------------------------
void ofApp::next_local_video() {
    local_video_index = (local_video_index+1)%(localVideoFiles.size());
}

//--------------------------------------------------------------
void ofApp::previous_local_video() {
    local_video_index = abs((int)((local_video_index-1)%(localVideoFiles.size())));
}

//--------------------------------------------------------------
void ofApp::load_local_video_files() {
    localVideoFiles.open("movies");
    localVideoFiles.allowExt("mov");
    localVideoFiles.listDir();
    
    //go through and print out all the paths
    for(int i = 0; i < localVideoFiles.size(); i++){
        cout <<localVideoFiles.getPath(i) << endl;
    }
    local_video_index = 0;
}

//--------------------------------------------------------------
void ofApp::setup_input_source () {
    localVideo.stop();
    cam.close();
    webVideo.stop();
    
    switch(input_source)
    {
        case LOCAL_VIDEO: {
            string v = localVideoFiles.getPath(local_video_index);
            localVideo.load(v);
            localVideo.setLoopState(OF_LOOP_NORMAL);
            localVideo.play();
            break;
        }
            
        case WEBCAM:
            cam.initGrabber(320, 240);
            break;
            
        case REMOTE_VIDEO:
            load_webvideo();
            break;
        
            
        case SYPHON_VIDEO:
            syphonClient.setup();
            break;
    }
}

//--------------------------------------------------------------
void ofApp::setup_output_source () {
    switch(output_source)
    {
        case WEKINATOR:
            ccv.setup("image-net-2012.sqlite3");
            break;
        
        case FACE_TRACKER:
            ccv.setupFace("face.sqlite3");
            break;
            
        case CLASSIFIER:
            ccv.setup("image-net-2012.sqlite3");
            break;
    }
}

//--------------------------------------------------------------
void ofApp::update() {
    ofImage currentFrame;
    ofPixels pixels;
    
    switch(input_source)
    {
        case LOCAL_VIDEO:
            localVideo.update();
            localVideo.getTexture().readToPixels(pixels);
            break;
            
        case WEBCAM:
            cam.update();
            cam.getTexture().readToPixels(pixels);
            break;
            
        case REMOTE_VIDEO: {
            webVideo.update();
            webVideo.getTexture().readToPixels(pixels);
        
            if (ofGetFrameNum() % 30 == 0)
                checkSettings();
            break;
        }
        case SYPHON_VIDEO: {
            ofFbo fbo;
            fbo.allocate(syphonClient.getWidth(), syphonClient.getHeight(), GL_RGBA);
            fbo.begin();
            syphonClient.draw(0, 0);
            fbo.end();
            fbo.readToPixels(pixels);
            break;
        }
    }
    
    pixels.setImageType(OF_IMAGE_COLOR);
    currentFrame.setFromPixels(pixels);
    currentFrame.resize(RESIZE_WIDTH, RESIZE_HEIGHT);
    //cout << "number of pixels " <<currentFrame.getPixels().size() << endl;
    
    //            if(localVideo.isPlaying() && localVideo.isFrameNew())
    //                results_face = ccv.classifyFace(localVideo);
    //                results_classifier = ccv.classify(localVideo);
    
    if (sending) {
        computeFeatures(currentFrame);
        sendOsc();
    }
}

//--------------------------------------------------------------
void ofApp::checkSettings(){
    boost::filesystem::path filePath = ofToDataPath("video.txt");
    time_t t = boost::filesystem::last_write_time(filePath);
    if (t != lastSaveVideoFile) {
        lastSaveVideoFile = t;
        load_webvideo();
    }
}

//--------------------------------------------------------------
void ofApp::computeFeatures(ofImage& img) {
    switch(output_source)
    {
        case WEKINATOR:
            featureEncoding = ccv.encode(img, ccv.numLayers()-1);
            break;
            
        case FACE_TRACKER:
            results_face = ccv.classifyFace(img);
            break;
            
        case CLASSIFIER:
            results_classifier = ccv.classify(img);
            break;
    }
}

//@TODO - 1. FORMAT OSC MESSAGES TO BE SENT
//@TODO - 2. MAKE UI TO REFLECT THE AVAILABLE OUTPUT
//@TODO - 3. TEST EVERYTHING TOGETHER!
//--------------------------------------------------------------
void ofApp::sendOsc() {
    
    switch(output_source)
    {
        case WEKINATOR:
            oscAddress = OSC_ADDRESS_WEKINATOR;
            send_wekinator_OSC();
            break;
            
        case FACE_TRACKER:
            oscAddress = OSC_ADDRESS_FACE_TRACKER;
            send_face_tracking_OSC();
            break;
            
        case CLASSIFIER:
            oscAddress = OSC_ADDRESS_CLASSIFIER;
            send_classifier_OSC();
            break;
    }

}

//--------------------------------------------------------------
void ofApp::send_wekinator_OSC() {
    msg.clear();
    msg.setAddress(oscAddress);
    for (int i=0; i<featureEncoding.size(); i++)
        msg.addFloatArg(featureEncoding[i]);
    osc.sendMessage(msg);
}

//--------------------------------------------------------------
void ofApp::send_face_tracking_OSC() {
    
    for(int i = 0; i < results_face.size(); i++) {
        msg.clear();
        msg.setAddress(oscAddress);
        
//        results_face[i] = ofRectangle(results_face[i].getX()/localVideo.getWidth(),
//                                      results_face[i].getY()/localVideo.getHeight(),
//                                      results_face[i].getWidth()/localVideo.getWidth(),
//                                      results_face[i].getHeight()/localVideo.getHeight());

        results_face[i] = ofRectangle(results_face[i].getX()/RESIZE_WIDTH,
                                      results_face[i].getY()/RESIZE_HEIGHT,
                                      results_face[i].getWidth()/RESIZE_WIDTH,
                                      results_face[i].getHeight()/RESIZE_HEIGHT);
        
        msg.addIntArg(i);
        msg.addFloatArg(results_face[i].getX());
        msg.addFloatArg(results_face[i].getY());
        msg.addFloatArg(results_face[i].getWidth());
        msg.addFloatArg(results_face[i].getHeight());
        
        osc.sendMessage(msg);
    }

}

//--------------------------------------------------------------
void ofApp::send_classifier_OSC() {
    for(int i = 0; i < results_classifier.size(); i++) {
        msg.clear();
        msg.setAddress(oscAddress);
        
        msg.addFloatArg(results_classifier[i].confidence);
        msg.addStringArg(results_classifier[i].imageNetName);
    
        osc.sendMessage(msg);
    }
}


////--------------------------------------------------------------
//void ofApp::sendOsc() {
//
//    switch(input_source)
//    {
//        case LOCAL_VIDEO: {
//            ofTexture t(localVideo.getTexture());
//            ofPixels pixels;
//            t.readToPixels(pixels);
//            ofImage img;
//            img.setFromPixels(pixels);
//            img.resize(300, 300);
//            //img.draw(20, 20, 120, 120);
//            featureEncoding = ccv.encode(img, ccv.numLayers()-1);
//            break;
//        }
//
//        case WEBCAM:
//            featureEncoding = ccv.encode(cam, ccv.numLayers()-1);
//            break;
//
//        case REMOTE_VIDEO: {
//            //    JERA
//            //    ofxAvFoundationHLSPlayer cannot be sent directly to ccv.encode.
//            //    you need to transcode it to an ofImage first to make it work.
//            //    however, trancoding from ofxAvFoundationHLSPlayer to ofImage seems bugged due to
//            //    lack of support on ofxAvFoundationHLSPlayer image type (currently undentified).
//            //    the following code is a little hacking to make it work, that involves:
//            //    ofxAvFoundationHLSPlayer > ofTexture > ofPixels > ofImage
//            ofTexture webTexture(webVideo.getTexture());
//            ofPixels pixels;
//            webTexture.readToPixels(pixels);
//            ofImage img;
//            img.setFromPixels(pixels);
//            img.resize(300, 300);
//            //img.draw(20, 20, 120, 120);
//            featureEncoding = ccv.encode(img, ccv.numLayers()-1);
//            break;
//        }
//
//        case SYPHON_VIDEO: {
//            if (!syphonClient.isSetup())
//                return;
//
//            ofFbo fbo;
//            fbo.allocate(syphonClient.getWidth(), syphonClient.getHeight(), GL_RGBA);
//            fbo.begin();
//            syphonClient.draw(0, 0);
//            fbo.end();
//            ofPixels pixels;
//            fbo.readToPixels(pixels);
//            ofImage sFrame;
//            sFrame.setFromPixels(pixels);
//            sFrame.resize(300, 300);
//
//            featureEncoding = ccv.encode(sFrame, ccv.numLayers()-1);
//
//            break;
//        }
//
//    }
//
//    msg.clear();
//    msg.setAddress(oscAddress);
//    for (int i=0; i<featureEncoding.size(); i++) {
//        msg.addFloatArg(featureEncoding[i]);
//    }
//    osc.sendMessage(msg);
//
//}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    
    switch (key) {
    case '1':
        input_source = LOCAL_VIDEO;
        setup_input_source();
        break;
    case '2':
        input_source = WEBCAM;
        setup_input_source();
        break;
    case '3':
        input_source = REMOTE_VIDEO;
        setup_input_source();
        break;
    case '4':
        input_source = SYPHON_VIDEO;
        setup_input_source();
        break;
            
            
    case 'a':
        sending = false;
        output_source = WEKINATOR;
        setup_output_source();
        break;
    case 's':
        sending = false;
        output_source = FACE_TRACKER;
        setup_output_source();
        break;
    case 'd':
        sending = false;
        output_source = CLASSIFIER;
        setup_output_source();
        break;
    case ' ':
        sending = !sending;
        break;
    
    case '=':
    case '+':
        if (input_source == LOCAL_VIDEO) {
            previous_local_video();
            setup_input_source();
        }
        break;
    
    case '-':
        if (input_source == LOCAL_VIDEO) {
            next_local_video();
            setup_input_source();
        }
            
        if (input_source == REMOTE_VIDEO)
            ofSystem("open "+ofToDataPath("video.txt"));
        break;
    
    }
    
    
}


//--------------------------------------------------------------
void ofApp::draw() {
    
    //ofPushStyle();
    
    switch(input_source)
    {
        case LOCAL_VIDEO: {
            localVideo.draw(0, 0, ofGetWidth(), ofGetHeight());
     
// DRAWING FACE DATA
//            ofPushStyle();
//            ofNoFill();
//            ofSetLineWidth(2);
//            if (results_face.size() >0)
//
//            for(int i = 0; i < results_face.size(); i++) {
//                //cout << i << " - x: " << results[i].getPosition().x << " y: " << results[i].getPosition().y << endl;
//
//                results_face[i] = ofRectangle(results_face[i].getX()/localVideo.getWidth()*ofGetWidth(),
//                                              results_face[i].getY()/localVideo.getHeight()*ofGetHeight(),
//                                              results_face[i].getWidth()/localVideo.getWidth()*ofGetWidth(),
//                                              results_face[i].getHeight()/localVideo.getHeight()*ofGetHeight());
//                ofDrawRectangle(results_face[i]);
//                ofDrawBitmapStringHighlight(ofToString(i), results_face[i].getPosition());
//            }
//            ofPopStyle();
        
//DRAWING CLASISFIER DATA
//            ofPushStyle();
//            ofTranslate(5, 5);
//            for(int i = 0; i < results_classifier.size(); i++) {
//                ofSetColor(ofColor::white);
//                ofFill();
//                ofDrawRectangle(0, 0, 100, 10);
//                ofSetColor(ofColor::black);
//                ofDrawRectangle(1, 1, (100-2) * results_classifier[i].confidence, 10-2);
//                ofSetColor(ofColor::white);
//                ofDrawBitmapStringHighlight(results_classifier[i].imageNetName, 105, 10);
//                ofTranslate(0, 15);
//            }
//            ofPopStyle();
            
            
//            ofDrawBitmapStringHighlight("Input: LOCAL_VIDEO", 10, ofGetHeight()-60,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
//            ofDrawBitmapStringHighlight("File: " + localVideoFiles.getPath(local_video_index) + "\nPress - to change current video", 10, ofGetHeight()-30, ofColor(ofColor::black, 90), ofColor::yellow);
            
            stringstream in_msg;
            in_msg << "Current Input: LOCAL_VIDEO" <<endl<<endl;
            in_msg << "Available inputs..." << endl;
            in_msg << "   1: local" << endl;
            in_msg << "   2: webcam" << endl;
            in_msg << "   3: remote" << endl;
            in_msg << "   4: syphon" <<endl << endl;
            in_msg << "File: " + localVideoFiles.getPath(local_video_index) + "\nPress - to change current video";
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, ofGetHeight()-150, ofColor(ofColor::black, 90), ofColor::yellow);
            
            break;
        }
        
        case WEBCAM: {
            cam.draw(0, 0, ofGetWidth(), ofGetHeight());
//            ofDrawBitmapStringHighlight("Input: WEBCAM", 10, ofGetHeight()-30,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
            stringstream in_msg;
            in_msg << "Current Input: WEBCAM" <<endl<<endl;
            in_msg << "Available inputs..." << endl;
            in_msg << "   1: local" << endl;
            in_msg << "   2: webcam" << endl;
            in_msg << "   3: remote" << endl;
            in_msg << "   4: syphon";
            //in_msg << "File: " + localVideoFiles.getPath(local_video_index) + "\nPress - to change current video";
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, ofGetHeight()-150, ofColor(ofColor::black, 90), ofColor::yellow);
            break;
        }
            
        case REMOTE_VIDEO: {
            webVideo.draw(0, 0, ofGetWidth(), ofGetHeight());
//            ofDrawBitmapStringHighlight("Input: REMOTE_VIDEO", 10, ofGetHeight()-60,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
//             ofDrawBitmapStringHighlight("Press - to change the remote link", 10, ofGetHeight()-30, ofColor(ofColor::black, 90), ofColor::yellow);

            stringstream in_msg;
            in_msg << "Current Input: REMOTE_VIDEO" <<endl<<endl;
            in_msg << "Available inputs..." << endl;
            in_msg << "   1: local" << endl;
            in_msg << "   2: webcam" << endl;
            in_msg << "   3: remote" << endl;
            in_msg << "   4: syphon" <<endl << endl;
            in_msg << "Press - to change current video";
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, ofGetHeight()-150, ofColor(ofColor::black, 90), ofColor::yellow);
            break;
        }
            
        case SYPHON_VIDEO: {
            if (!syphonClient.isSetup())
                return;
            
            syphonClient.draw(0, 0, ofGetWidth(), ofGetHeight());
//            ofDrawBitmapStringHighlight("Input: SYPHON_VIDEO", 10, ofGetHeight()-30,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
            
            stringstream in_msg;
            in_msg << "Current Input: SYPHON_VIDEO" <<endl<<endl;
            in_msg << "Available inputs..." << endl;
            in_msg << "   1: local" << endl;
            in_msg << "   2: webcam" << endl;
            in_msg << "   3: remote" << endl;
            in_msg << "   4: syphon";
            //in_msg << "File: " + localVideoFiles.getPath(local_video_index) + "\nPress - to change current video";
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, ofGetHeight()-150, ofColor(ofColor::black, 90), ofColor::yellow);
            
            break;
        }
    }
    
    if (!ccv.isLoaded()) {
        ofDrawBitmapStringHighlight("Network file not found! Check your data folder to make sure it exists.", 10, 20, ofColor(ofColor::black, 90), ofColor::yellow);
        return;
    }
    
    //ofSetColor(255);
    //ofPushStyle();
    //ofSetRectMode(OF_RECTMODE_CENTER);
    
    stringstream output_status;
    
    if (sending)
        output_status << "Sending "+ofToString(msg.getNumArgs())+" values" << endl
                      << "to "+oscHost+" port "+ofToString(oscPort)+", address \""+oscAddress+"\"";
    else
        output_status << "Press space to start sending outputs";
    
//    if (sending) {
//        ofDrawBitmapStringHighlight("sending "+ofToString(msg.getNumArgs())+" values", 10, 70, ofColor(ofColor::black, 90), ofColor::yellow);
//        ofDrawBitmapStringHighlight("to "+oscHost+" port "+ofToString(oscPort)+", address \""+oscAddress+"\"", 10, 50, ofColor(ofColor::black, 90), ofColor::yellow);
//    }
//
//    else
//        ofDrawBitmapStringHighlight("Press space to start sending outputs", 10, 50, ofColor(ofColor::black, 90), ofColor::yellow);
//
    switch(output_source)
    {
        case WEKINATOR: {
//            ofDrawBitmapStringHighlight("Output: WEKINATOR", 10, 20,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
            stringstream in_msg;
            in_msg << "Current Output: WEKINATOR" <<endl<<endl;
            in_msg << "Available Outputs..." << endl;
            in_msg << "   a: wekinator" << endl;
            in_msg << "   s: face tracking" << endl;
            in_msg << "   d: classifier" << endl << endl;
            in_msg << output_status.str();
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, 20, ofColor(ofColor::black, 90), ofColor::yellow);
            
            break;
        }
        case FACE_TRACKER: {
//            ofDrawBitmapStringHighlight("Output: FACE_TRACKER", 10, 20,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
            stringstream in_msg;
            in_msg << "Current Output: FACE_TRACKER" <<endl<<endl;
            in_msg << "Available Outputs..." << endl;
            in_msg << "   a: wekinator" << endl;
            in_msg << "   s: face tracking" << endl;
            in_msg << "   d: classifier" << endl << endl;
            in_msg << output_status.str();
            
            ofPushStyle();
            ofNoFill();
            ofSetLineWidth(2);
            for(int i = 0; i < results_face.size(); i++) {
                results_face[i] = ofRectangle(results_face[i].getX()*ofGetWidth(),
                                              results_face[i].getY()*ofGetHeight(),
                                              results_face[i].getWidth()*ofGetWidth(),
                                              results_face[i].getHeight()*ofGetHeight());
                ofDrawRectangle(results_face[i]);
                ofDrawBitmapStringHighlight(ofToString(i), results_face[i].getPosition());
            }
            ofPopStyle();
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, 20, ofColor(ofColor::black, 90), ofColor::yellow);
            break;
        }
        case CLASSIFIER: {
//            ofDrawBitmapStringHighlight("Output: CLASSIFIER", 10, 20,
//                                        ofColor(ofColor::black, 90), ofColor::yellow);
            stringstream in_msg;
            in_msg << "Current Output: CLASSIFIER" <<endl<<endl;
            in_msg << "Available Outputs..." << endl;
            in_msg << "   a: wekinator" << endl;
            in_msg << "   s: face tracking" << endl;
            in_msg << "   d: classifier" << endl << endl;
            in_msg << output_status.str();
            
            ofPushStyle();
            ofTranslate(5, 5);
            for(int i = 0; i < results_classifier.size(); i++) {
                ofSetColor(ofColor::white);
                ofFill();
                ofDrawRectangle(0, 0, 100, 10);
                ofSetColor(ofColor::black);
                ofDrawRectangle(1, 1, (100-2) * results_classifier[i].confidence, 10-2);
                ofSetColor(ofColor::white);
                ofDrawBitmapStringHighlight(results_classifier[i].imageNetName, 105, 10);
                ofTranslate(0, 15);
            }
            ofPopStyle();
            
            ofDrawBitmapStringHighlight(in_msg.str(), 10, 20, ofColor(ofColor::black, 90), ofColor::yellow);
            break;
        }
    }
    
    
    ofDrawBitmapStringHighlight("FPS: " + ofToString(ofGetFrameRate()), ofGetWidth()-125, 20, ofColor(ofColor::black, 90), ofColor::yellow);
    
//    ofDrawBitmapStringHighlight("INPUTS",(ofGetWidth()/2),(ofGetHeight()/2)-80, ofColor(ofColor::black, 90), ofColor::yellow);
//    ofDrawBitmapStringHighlight("1 for local video",(ofGetWidth()/2)-75,(ofGetHeight()/2)-50, ofColor(ofColor::black, 90), ofColor::yellow);
//    ofDrawBitmapStringHighlight("2 for webcam",(ofGetWidth()/2)-55,(ofGetHeight()/2)-20, ofColor(ofColor::black, 90), ofColor::yellow);
//    ofDrawBitmapStringHighlight("3 for remote video",(ofGetWidth()/2)-80,(ofGetHeight()/2)+10, ofColor(ofColor::black, 90), ofColor::yellow);
//    ofDrawBitmapStringHighlight("4 for syphon video",(ofGetWidth()/2)-80,(ofGetHeight()/2)+40, ofColor(ofColor::black, 90), ofColor::yellow);
    
//    if (sending)
//        sendOsc();
    
    //ofPopStyle();
}

/*
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
*/

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

/*
//--------------------------------------------------------------
void ofApp::load_webvideo(string url_in_m3u8) {
    webVideo.stop();
    webVideo = *(new ofxAvFoundationHLSPlayer());
    webVideo.load(url_in_m3u8);
    cout << "video loaded! " << endl;
}
*/
 
 //--------------------------------------------------------------
 void ofApp::load_webvideo() {
     webVideo.stop();
     
     string video_url = read_url_from_file();
     
     string translated_url = "";
     
     //try to load url as a live video
     translated_url = translate_live_video_url(video_url);
     
     //if no results were found for live videos...
     if (translated_url.empty())
     // ...try non-live videos
     translated_url = translate_non_live_video_url(video_url);
     
     //if no results were found again, use default video...
     if (translated_url.empty())
     translated_url = DEFAULT_VIDEO;
     
     load_video_from_url(translated_url);
 }


//--------------------------------------------------------------
void ofApp::load_video_from_url(string var_video) {
    webVideo.load(var_video);
}


//--------------------------------------------------------------
string ofApp::read_url_from_file() {
    ofBuffer buffer = ofBufferFromFile("video.txt");
    
    string video_url = "";
    
    if(buffer.size()) {
        
        for (ofBuffer::Line it = buffer.getLines().begin(), end = buffer.getLines().end(); it != end; ++it) {
            
            video_url = *it;
            
            if(video_url.empty() == false) {
                cout <<  "video link: " << video_url << endl;
                break;
            }
        }
        
    }
    
    //updating the last time saved
    boost::filesystem::path filePath = ofToDataPath("video.txt");
    lastSaveVideoFile = boost::filesystem::last_write_time(filePath);
    
    return video_url;
    
}

//--------------------------------------------------------------
string ofApp::translate_live_video_url(string url){
    return translate_video_url(url,
                               INITIAL_LIVE_VIDEO_QUALITY,
                               YOUTUBE_DL_PARAMETER_FOR_LIVE_VIDEO);
}

//--------------------------------------------------------------
string ofApp::translate_non_live_video_url(string url){
    return translate_video_url(url,
                               INITIAL_NON_LIVE_VIDEO_QUALITY,
                               YOUTUBE_DL_PARAMETER_FOR_NON_LIVE_VIDEO);
}

//--------------------------------------------------------------
string ofApp::translate_video_url(string url, int quality, string parameter){
    
    //int quality = INITIAL_VIDEO_QUALITY;
    string translated_url = "";
    int quality_limit = quality - QUALITY_RANGE;
    
    //if quality 95 is too high...
    while (translated_url.empty() && quality >= quality_limit) {
        stringstream cmd;
        //formats the command as a string
        //cmd << "/usr/local/bin/youtube-dl -f " << quality <<" -g "  << url;
        cmd << "/usr/local/bin/youtube-dl -f " << quality << " " << parameter << " " << url;
        
        //tries to execute the command
        translated_url = terminal_cmd(cmd.str().c_str());
        translated_url.pop_back();
        
        //if the video is not empty
        if (!translated_url.empty())
            //tries to find the last occurrence of https (the valid one)
            translated_url = translated_url.substr(translated_url.rfind("https://"), translated_url.size());
        
        quality--;
    }
    
    if (translated_url.empty())
        return "";
    else
        return translated_url;
}

