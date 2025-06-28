#pragma once

#include <tiny_obj_loader.h>
#include <string>
#include <vector>

struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = nullptr, bool triangulate = true);
};