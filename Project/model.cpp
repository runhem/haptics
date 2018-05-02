#include "model.h"
#include <chai3d.h>

Model::Model(chai3d::cMultiMesh * object, int height, int width, int length)
{
    object = object;
    height = height;
    width = width;
    length = length;
}

chai3d::cMultiMesh * Model::getObject()
{
    return object;
}


int Model::getScale()
{
    return height*width*length;
}

