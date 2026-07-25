#pragma once

#include <External/Eigen/Dense>
#include <vector>
#include <memory>
#include "2DSolver_Constraints.h"
#include "2DSolver_Solver.h"
#include "CAD_Operation.h"

// ============================================================================
// 1. STRUCTURE POUR LE SOLVE "ONE-SHOT" (Global / Complet)
// ============================================================================
struct SolverOneShot {
    // Exécute la résolution globale en une seule passe (ex: PrepareAndSolve historique)
    static bool Solve(SketchParams& sketch, bool enableDiagnostics=false);

    // Lance les diagnostics globaux
    static bool Diagnostics(SketchParams& sketch);
};

// ============================================================================
// 2. STRUCTURE POUR LE SOLVE INCRÉMENTAL (Session Interactive / Drag)
// ============================================================================
struct SolverInteractiveSession {
    Solver2D_Solver solver;
    Eigen::VectorXd Vector_X;
    std::vector<double*> variablePointers;
    std::vector<int>    activeVarIndicesAll;
    bool isInitialized = false;

    int activeVarIndexX = -1;
    int activeVarIndexY = -1;

    // Initialise la session lourde (à appeler au MousePress)
    void Initialize(SketchParams& sketch);

    // Met à jour une coordonnée mais ne rédoud pas !
    void UpdatePoint(SketchParams& sketch, int varIndex, double newValue);

    // résout (à appeler au MouseMove)
    bool Step(SketchParams& sketch);

    double GetVarValue(int li_idx) { return *(variablePointers[li_idx]); }

    // Récupère les indices X et Y d'une poignée pour le pilotage direct
    static bool GetIndicesForHandle(SketchParams& sketch, int primitiveId, int handleType, int& outIndexX, int& outIndexY);
    static bool GetIndicesForEntireEdge(SketchParams& sketch, int primitiveId, std::vector<int>& outIndices);

private:
    void pullFromSketch();
    void pushToSketch();
};

// ============================================================================
// 3. MAPPER PRINCIPAL (Interface simplifiée)
// ============================================================================
class Solver2D_Mapper {
    friend struct SolverOneShot;
    friend struct SolverInteractiveSession;

private:
    static std::string formatRef(const SketchParams& sketch, const GeometryReference& ref);
    static gp_Pnt2d* getPointPointerFromRef(SketchParams& sketch, const GeometryReference& ref);

public:
    Solver2D_Mapper() = default;

    // Méthodes statiques de convenance ou délégation directe si besoin,
    // mais l'utilisation directe de SolverOneShot::Solve(...) et SolverInteractiveSession
    // rendra ton code d'appel ultra-explicite.
};