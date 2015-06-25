//
//  MyCamera.h
//  OpenCLTest
//
//  Created by Tomoyuki Maehara on 4/26/15.
//  Copyright (c) 2015 Tomoyuki Maehara. All rights reserved.
//

#ifndef __OpenCLTest__MyCamera__
#define __OpenCLTest__MyCamera__

#include <math.h>
#include <stdio.h>

#endif /* defined(__OpenCLTest__MyCamera__) */


class MyCamera {
public:
    float targetX;
    float targetY;
    float targetZ;
    float x;
    float y;
    float z;
    float rotateX;
    float rotateY;
    float rotateZ;
    float length;
    
    MyCamera(float length);
    void update();
};