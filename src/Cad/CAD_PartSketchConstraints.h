
#pragma once
#include <cstdint>


#include "CAD_PartOpIdentifiable.h"
#include <variant>
#include <cmath>

namespace PartSketchConstraint{


//----------------------------------------------------------------------
//		
//----------------------------------------------------------------------

    #define TOL_CHECK_DOUBLE 1E-7




/*

//----------------------------------------------------------------------
    //
    //----------------------------------------------------------------------
    enum class TargetType {
        Primitive, PrimitiveMiddlePoint, Point
    };

    //----------------------------------------------------------------------
    //
    //----------------------------------------------------------------------
    struct RefGeometry {
        uint64_t operationId = 0;
        struct {
            uint64_t Id = 0;
            TargetType type = TargetType::Primitive;

            bool operator==(const struct& other) const {
                return Id == other.Id && type == other.type;
            }
        } target;

        bool operator==(const RefGeometry& other) const {
            return operationId == other.operationId &&
                   target == other.target;
        }
    };
*/


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

        bool operator==(const RefGeometry& other) const {
            return
                operationId == other.operationId &&
                primitiveId == other.primitiveId &&
                subElement == other.subElement;
        }
	};




	struct VerticalConstraint {
        RefGeometry ref;
        bool operator==(const VerticalConstraint& other) const {
            return ref == other.ref;
        }
	};

	struct HorizontalConstraint {
		RefGeometry ref;
        bool operator==(const HorizontalConstraint& other) const {
            return ref == other.ref;
        }
	};
	struct ParallelConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
        bool operator==(const ParallelConstraint& other) const {
            return ref1 == other.ref1 &&
                   ref2 == other.ref2;
        }
	};
	struct DistanceConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
		double value = 0.0;
		bool isDriven = false;
        bool operator==(const DistanceConstraint& other) const {
            return (ref1 == other.ref1) &&
                   (ref2 == other.ref2) &&
                   (std::abs( value - other.value)<TOL_CHECK_DOUBLE) &&
                   (isDriven == other.isDriven);
        }
	};
	struct PerpendicularConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
        bool operator==(const PerpendicularConstraint& other) const {
            return ref1 == other.ref1 &&
                   ref2 == other.ref2;
        }
	};
	struct CoincidentConstraint {
		RefGeometry ref1;
		RefGeometry ref2;
        bool operator==(const CoincidentConstraint& other) const {
            return ref1 == other.ref1 &&
                   ref2 == other.ref2;
        }
	};
	struct RadiusConstraint {
		RefGeometry ref1;
		double value = 0.0;
        bool operator==(const RadiusConstraint& other) const {
            return (ref1 == other.ref1) &&
                   (std::abs(value - other.value) < TOL_CHECK_DOUBLE);
        }
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

        // Opérateur d'égalité pour la contrainte
        bool operator==(const SketchConstraint& other) const {
            // Compare l'identifiant de base (si Identifiable le gère)
            // et compare directement les variantes
            return this->id == other.id && data == other.data;
        }

        bool operator!=(const SketchConstraint& other) const {
            return !(*this == other);
        }
	};

}





