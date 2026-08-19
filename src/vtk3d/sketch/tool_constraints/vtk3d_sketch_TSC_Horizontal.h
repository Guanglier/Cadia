#pragma once




#include "tool_constraints/vtk3d_sketch_TSC_Model.h"





class ConstraintTool_Horizontal : public ConstraintToolBase {
public:
    ConstraintTool_Horizontal(Tool_SetConstraints* parent) : ConstraintToolBase(parent) {}

    virtual void popup_create() override ;
    void OnBtnClicked ( CadEvent::Sketch::CmdPopupToolBtnClicked li_btn ) override;
    bool OnSelectedElement ( PickResult_element li_Element ) override;              //   retourne true si la sélection est correcte
    void Resetall () override;
    void AppliqueContrainte () override;
};


