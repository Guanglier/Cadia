#pragma once




#include "tool_constraints/vtk3d_sketch_TSC_Horizontal.h"




class ConstraintTool_Distance : public ConstraintTool_Horizontal {
public:
    ConstraintTool_Distance(Tool_SetConstraints* parent) : ConstraintTool_Horizontal(parent) {}
    void popup_create() override ;
    bool OnSelectedElement ( PickResult_element li_Element ) override;
    void AppliqueContrainte () override;

    void OnInputValueChanged(std::string string_id, double value) override;

    double m_value = 10.0;

    float CalculeDistance ( );
};


