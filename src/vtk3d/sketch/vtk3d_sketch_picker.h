#pragma once

#include <gp_Pnt2d.hxx>
#include <vtkPolyData.h>



struct PickResult_element {
    enum class TargetType { None, Point, Primitive };
    TargetType type = TargetType::None;
    int Id = -1;             // ID du point ou ID de l'edge/primitive
    gp_Pnt2d Clicked_Point2D;
    gp_Pnt Clicked_Point3D;
};

struct PickResult {
    PickResult_element Element;
    vtkPolyData* sourcePolyData = nullptr;
};


