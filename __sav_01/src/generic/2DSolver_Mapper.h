#pragma once



#include <External/Eigen/Dense>

#include "2DSolver_Constraints.h"
#include "2DSolver_Solver.h"

// Tes structures d'esquisse
#include "CAD_Operation.h" // Contient SketchParams, SketchPrimitive, etc.


#define SOLVE_DBG_CORRECT_COINCIDENCE





class Solver2D_Mapper {
private:
    static std::string formatRef(const SketchParams& sketch, const GeometryReference& ref);
    static gp_Pnt2d* getPointPointerFromRef(SketchParams& sketch, const GeometryReference& ref);

public:
    Solver2D_Mapper() = default;

    static void SolveWithDiagnostics(SketchParams& sketch);
    static bool PrepareAndSolve(SketchParams& sketch);
};














