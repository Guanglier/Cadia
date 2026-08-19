#pragma once




#include "tool_constraints/vtk3d_sketch_TSC_Horizontal.h"




class ConstraintTool_Vertical : public ConstraintTool_Horizontal {
public:
    ConstraintTool_Vertical(Tool_SetConstraints* parent) : ConstraintTool_Horizontal(parent) {}
    void popup_create() override ;
    void AppliqueContrainte () override;
};


