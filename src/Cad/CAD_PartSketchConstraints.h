
#pragma once
#include <cstdint>


#include "CAD_PartOpIdentifiable.h"
#include <variant>

namespace PartSketchConstraint{


//----------------------------------------------------------------------
//		
//----------------------------------------------------------------------



	//----------------------------------------------------------------------
	//		
	//----------------------------------------------------------------------
	enum class SubElement {
		Whole, StartPoint, EndPoint, CenterPoint
	};
	//std::ostream& operator<<(std::ostream& os, ConstraintSubElement sub);

	//----------------------------------------------------------------------
	//		
	//----------------------------------------------------------------------
	struct RefGeometry {
		uint64_t             			operationId = 0;
		uint64_t             			primitiveId = 0;
		SubElement 	subElement = SubElement::Whole;
	};




	struct VerticalConstraint {
		RefGeometry ref;
	};

	struct HorizontalConstraint {
		RefGeometry ref;
	};
	struct ParallelConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
	};
	struct DistanceConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
		double value = 0.0;
		bool isDriven = false;
	};
	struct PerpendicularConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
	};
	struct CoincidentConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
	};
	struct RadiusConstraint {
		RefGeometry ref1;
		double value = 0.0;
	};
	using ConstraintVariant = std::variant<
		VerticalConstraint,
		HorizontalConstraint,
		ParallelConstraint,
		PerpendicularConstraint,
		CoincidentConstraint,
		DistanceConstraint,
		RadiusConstraint
	>;
	
	struct SketchConstraint : public Identifiable {
		ConstraintVariant data;
	};

/*
	//----------------------------------------------------------------------
	//		
	//----------------------------------------------------------------------
	struct Constraint : public Identifiable {
		RefGeometry ref1;
		RefGeometry ref2;
		double            value = 0.0;
		bool              isDriven = false;

		bool isEquivalentTo(const Constraint& other) const ;
	};
	*/
}





