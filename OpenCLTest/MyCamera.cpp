//
//  MyCamera.cpp
//  OpenCLTest
//
//  Created by Tomoyuki Maehara on 4/26/15.
//  Copyright (c) 2015 Tomoyuki Maehara. All rights reserved.
//

#include "MyCamera.h"


MyCamera::MyCamera(float length){
    this->targetX=0.0f;
    this->targetY=0.0f;
    this->targetZ=0.0f;
    this->x=0.0f;
    this->y=0.0f;
    this->z=0.0f;
    this->rotateX=0.0f;
    this->rotateY=-90.0f;
    this->rotateZ=0.0f;
    this->length=length;
    update();
}
void MyCamera::update(){
    float hpi = M_PI *0.5;
    float radiansY = float(M_PI*(rotateY)/180.0f);
    float radiansX = float(M_PI*(rotateX)/180.0f);
    x=targetX+sin(-radiansY-hpi)*length;
    float tmp=cos(-radiansY-hpi)*length;
    y=targetY+tmp*sin(-radiansX);
    z=targetZ+tmp*cos(-radiansX);
    
}