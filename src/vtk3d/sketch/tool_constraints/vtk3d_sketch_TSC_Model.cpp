
#include "vtk3d_sketch_TSC_Model.h"
#include "vtk3d_sketch_ToolSetConstraints.h"






QString ConstraintToolBase::PickResult_ElementType_to_string ( PickResult_element li_Element ){
    switch ( li_Element.type ){
    default:
    case PickResult_element::TargetType::None: return "None";
    case PickResult_element::TargetType::Point : return "Point";
    case PickResult_element::TargetType::Primitive : return "Primitive";
    }
}



