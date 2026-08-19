

#include "vtk3d_sketch_TSC_Model.h"
#include "vtk3d_sketch_ToolSetConstraints.h"
#include "tool_constraints/vtk3d_sketch_TSC_Horizontal.h"
#include "tool_constraints/vtk3d_sketch_TSC_Vertical.h"

//---------------------------------------------------------
//
//---------------------------------------------------------
void ConstraintTool_Vertical::popup_create()  {

    ConstraintTool_Horizontal::popup_create();
    m_ToolHelper.title = "Contrainte Verticale";
}
//---------------------------------------------------------
//
//---------------------------------------------------------
void ConstraintTool_Vertical::AppliqueContrainte (){
    std::string  l_string = "";

    if (!data.sketchParams) return;
    PartSketchConstraint::SketchConstraint ConstHori;
    ConstHori.data = PartSketchConstraint::VerticalConstraint{{
            data.u64_SketchId,
            (uint64_t) data.SelectedElementsList[0].Id,
            PartSketchConstraint::TargetType::Primitive,
            PartSketchConstraint::SubElement::Whole
        }
    };
    data.sketchParams->addConstraint(ConstHori, l_string);
}




