#ifndef MODEL_H
#define MODEL_H
#include "chai3d.h"



class Model
{
    int height;
    int width;
    int length;

public:
    Model(chai3d::cMultiMesh * object, int height, int width, int length);
    chai3d::cMultiMesh * getObject();
    int getScale();
    chai3d::cMultiMesh * object;

};

#endif // MODEL_H
