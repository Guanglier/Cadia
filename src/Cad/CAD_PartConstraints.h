
#pragma once
#include <cstdint>


#include "CAD_PartOpIdentifiable.h"



//----------------------------------------------------------------------
//		
//----------------------------------------------------------------------

enum class ConstraintType {
    Horizontal, Vertical, Parallel, Perpendicular,
    Coincident, Tangent, Distance, Radius
};

//----------------------------------------------------------------------
//		
//----------------------------------------------------------------------
enum class ConstraintSubElement {
    Whole, StartPoint, EndPoint, CenterPoint
};
//std::ostream& operator<<(std::ostream& os, ConstraintSubElement sub);

//----------------------------------------------------------------------
//		
//----------------------------------------------------------------------
struct GeometryReference {
    uint64_t             operationId = 0;
    uint64_t             primitiveId = 0;
    ConstraintSubElement subElement = ConstraintSubElement::Whole;
};



//----------------------------------------------------------------------
//		
//----------------------------------------------------------------------
struct SketchConstraint : public Identifiable {
    ConstraintType    type;
    GeometryReference ref1;
    GeometryReference ref2;
    double            value = 0.0;
    bool              isDriven = false;

    bool isEquivalentTo(const SketchConstraint& other) const ;
};













