#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    particleSize = 12.0f;
    timeStep = 0.005f;
    numParticles = 100000;
    
    // Width and Heigth of the windows
    width = ofGetWindowWidth();
    height = ofGetWindowHeight();
    
    string shadersFolder;
    if(ofGetGLProgrammableRenderer()){
    	shadersFolder="shaders_programmable";
    }else{
    	shadersFolder="shaders";
    }


    // Loading the Shaders
    //updateDuration.load("", shadersFolder+"/durationUpdate.frag");
    
    updateDuration.load("",shadersFolder+"/posUpdate2.frag");// shader for updating the texture that store the particles position on RG channels
    updatePos.load("",shadersFolder+"/posUpdate.frag");// shader for updating the texture that store the particles position on RG channels
    updateVel.load("",shadersFolder+"/velUpdate.frag");// shader for updating the texture that store the particles velocity on RG channels
    
    // Frag, Vert and Geo shaders for the rendering process of the spark image
    updateRender.setGeometryInputType(GL_POINTS);
	updateRender.setGeometryOutputType(GL_TRIANGLE_STRIP);
	updateRender.setGeometryOutputCount(6);
    updateRender.load(shadersFolder+"/render.vert",shadersFolder+"/render.frag",shadersFolder+"/render.geom");
    
    // Seting the textures where the information ( position and velocity ) will be
    textureRes = (int)sqrt((float)numParticles);
    numParticles = textureRes * textureRes;
    
    // 1. Making arrays of float pixels with position information
    float * pos = new float[numParticles*3];
    for (int x = 0; x < textureRes; x++){
        for (int y = 0; y < textureRes; y++){
            int i = textureRes * y + x;
            
            pos[i*3 + 0] = ofRandom( 0.4, 0.6); //x*offset;
            pos[i*3 + 1] = 1.0; //y*offset;
            pos[i*3 + 2] = 0.0;
        }
    }
    // Load this information in to the FBOÔøΩs texture
    posPingPong.allocate(textureRes, textureRes,GL_RGB32F);
    posPingPong.src->getTextureReference().loadData(pos, textureRes, textureRes, GL_RGB);
    posPingPong.dst->getTextureReference().loadData(pos, textureRes, textureRes, GL_RGB);
    delete [] pos;    // Delete the array
    
    
    // 2. Making arrays of float pixels with velocity information and the load it to a texture
    float * vel = new float[numParticles*3];
    for (int i = 0; i < numParticles; i++){
        vel[i*3 + 0] = ofRandom(-0.1,0.1);
        vel[i*3 + 1] = 0.0;
        vel[i*3 + 2] = 1.0;
    }
    // Load this information in to the FBOÔøΩs texture
    velPingPong.allocate(textureRes, textureRes,GL_RGB32F);
    velPingPong.src->getTextureReference().loadData(vel, textureRes, textureRes, GL_RGB);
    velPingPong.dst->getTextureReference().loadData(vel, textureRes, textureRes, GL_RGB);
    delete [] vel; // Delete the array
    
    // 3. Making arrays of float pixels with velocity information and the load it to a texture;
    float * duration = new float[numParticles * 3];
    for(int i = 0; i < numParticles; i++){
        duration[i * 3 + 0] = ofRandom( 0.0, 3.0); // 遅れ時間
        duration[i * 3 + 1] = ofRandom( 2.0, 5.0); // 持続時間
        duration[i * 3 + 2] = 0.0; // 現在の時間
    }
    
    // Load this information in to the FBO's texture
    durationPingPong.allocate(textureRes, textureRes,GL_RGB32F);
    durationPingPong.src->getTextureReference().loadData(duration, textureRes, textureRes, GL_RGB);
    durationPingPong.dst->getTextureReference().loadData(duration, textureRes, textureRes, GL_RGB);
    delete [] duration;
    
    
    // Loading and setings of the variables of the textures of the particles
    sparkImg.loadImage("smoke-texture.png");
    imgWidth = sparkImg.getWidth();
    imgHeight = sparkImg.getHeight();
    
    // Allocate the final 
    renderFBO.allocate(width, height, GL_RGB32F);
    renderFBO.begin();
    ofClear(0, 0, 0, 255);
    renderFBO.end();


    mesh.setMode(OF_PRIMITIVE_POINTS);
    for(int x = 0; x < textureRes; x++){
        for(int y = 0; y < textureRes; y++){
            mesh.addVertex(ofVec3f(x,y));
            mesh.addTexCoord(ofVec2f(x, y));
        }
    }

}

//--------------------------------------------------------------
void ofApp::update(){
    // In each cycle itÔøΩs going to update the velocity first and the the position
    // Each one one with a different shader and FBO.
    // Because on GPU you canÔøΩt write over the texture that you are reading we are
    // using to pair of ofFbo attached together on what we call pingPongBuffer 
    // Learn more about Ping-Pong at:
    //
    // http://www.comp.nus.edu/~ashwinna/docs/PingPong_FBO.pdf
    // http://www.seas.upenn.edu/~cis565/fbo.htm#setupgl4
    
    // Durations PingPong
    //
    //
    
    
    durationPingPong.dst->begin();
    ofClear(0);
    updateDuration.begin();
    updateDuration.setUniformTexture("durationData", durationPingPong.src->getTextureReference(), 0);
    updateDuration.setUniform1f("timestep", (float)timeStep);
    
    durationPingPong.src->draw(0, 0);
    
    updateDuration.end();
    durationPingPong.dst->end();
    
    durationPingPong.swap();
    
    
    
    // Velocities PingPong
    //
    // It calculates the next frame and see if itÔøΩs there any collition
    // then updates the velocity with that information
    //
    velPingPong.dst->begin();
    ofClear(0);
    updateVel.begin();
    
    updateVel.setUniformTexture("backbuffer", velPingPong.src->getTextureReference(), 0);   // passing the previus velocity information
    updateVel.setUniformTexture("posData", posPingPong.src->getTextureReference(), 1);  // passing the position information
    updateVel.setUniformTexture("durationData", durationPingPong.src->getTextureReference(), 2);
    
    updateVel.setUniform1i("resolution", (int)textureRes); 
    updateVel.setUniform2f("screen", (float)width, (float)height);
    updateVel.setUniform1f("timestep", (float)timeStep);
    cout << (float)timeStep << endl;

    // draw the source velocity texture to be updated
    velPingPong.src->draw(0, 0);
    
    updateVel.end();
    velPingPong.dst->end();
    
    velPingPong.swap();
    
    
    // Positions PingPong
    //
    // With the velocity calculated updates the position
    //
    posPingPong.dst->begin();
    ofClear(0);
    updatePos.begin();
    updatePos.setUniformTexture("prevPosData", posPingPong.src->getTextureReference(), 0); // Previus position
    updatePos.setUniformTexture("velData", velPingPong.src->getTextureReference(), 1);  // Velocity
    

    updatePos.setUniform1f("timestep",(float) timeStep );
    
    // draw the source position texture to be updated
    posPingPong.src->draw(0, 0);
    
    updatePos.end();
    posPingPong.dst->end();
    
    posPingPong.swap();
    

    // Rendering
    //
    // 1.   Sending this vertex to the Vertex Shader. 
    //      Each one itÔøΩs draw exactly on the position that match where itÔøΩs stored on both vel and pos textures
    //      So on the Vertex Shader (thatÔøΩs is first at the pipeline) can search for it information and move it
    //      to it right position.
    // 2.   Once itÔøΩs in the right place the Geometry Shader make 6 more vertex in order to make a billboard 
    // 3.   that then on the Fragment Shader itÔøΩs going to be filled with the pixels of sparkImg texture
    //
    renderFBO.begin();
    ofClear(0,0,0,0);
    updateRender.begin();
    updateRender.setUniformTexture("posTex", posPingPong.dst->getTextureReference(), 0);
    updateRender.setUniformTexture("sparkTex", sparkImg.getTextureReference() , 1);
    updateRender.setUniform1i("resolution", (float)textureRes); 
    updateRender.setUniform2f("screen", (float)width, (float)height);
    updateRender.setUniform1f("size", (float)particleSize); 
    updateRender.setUniform1f("imgWidth", (float)sparkImg.getWidth());
    updateRender.setUniform1f("imgHeight", (float)sparkImg.getHeight());
    
    ofPushStyle();
    ofEnableBlendMode( OF_BLENDMODE_ADD );
    ofSetColor(255);

    mesh.draw();
    
    ofDisableBlendMode();
    glEnd();
    
    updateRender.end();
    renderFBO.end();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(255,0,0);
    renderFBO.draw(0,0);
    ofDisableBlendMode();
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(200,200,200);
    renderFBO.draw(0,0);
    ofDisableBlendMode();
     
    
    /**
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(0, 200, 0);
    velPingPong.dst->draw(0, 0);
    ofDisableAlphaBlending(); */
    
    
    ofSetColor(255);
    ofDrawBitmapString("Fps: " + ofToString( ofGetFrameRate()), 15,15);
    
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
