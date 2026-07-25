/**
 * @file 2DSolver_Mapper.h
 * @brief Définition des structures de pilotage et de mapping du solveur 2D (`SolverOneShot`, `SolverInteractiveSession`, `Solver2D_Mapper`).
 *
 * Ce fichier en-tête expose les interfaces permettant de lier l'esquisse CAO
 * (primitives et contraintes) aux algorithmes de résolution mathématique. Il sépare
 * la résolution globale (one-shot) de la session interactive temps réel (glissement de poignée).
 *
 * Utilisation :
 * - Pour une résolution globale ponctuelle : `SolverOneShot::Solve(sketch, true);`
 * - Pour une session interactive (ex: au clic de souris) :
     *   1. `session.Initialize(sketch);`
     *   2. En boucle (`MouseMove`) : `session.UpdatePoint(sketch, idx); session.Step(sketch);`
 */

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
    /**
     * @brief Exécute la résolution globale en une seule passe.
     * @param sketch [Entrée/Sortie] Référence vers l'esquisse CAO à résoudre.
     * @param enableDiagnostics [Entrée] Active l'affichage des diagnostics et du bilan DOF (défaut : false).
     * @return bool True si la résolution a réussi, false sinon.
     */
    // Exécute la résolution globale en une seule passe (ex: PrepareAndSolve historique)
    static bool Solve(SketchParams& sketch, bool enableDiagnostics=false);

    /**
     * @brief Lance les diagnostics globaux de l'esquisse.
     * @param sketch [Entrée/Sortie] Référence vers l'esquisse à analyser.
     * @return bool True si le diagnostic s'est exécuté, false sinon.
     */
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

    /**
     * @brief Initialise la session lourde (à appeler au MousePress).
     * @param sketch [Entrée] Référence vers l'esquisse à configurer pour la session.
     * @return void
     */
    // Initialise la session lourde (à appeler au MousePress)
    void Initialize(SketchParams& sketch);

    // Met à jour une coordonnée mais ne rédoud pas !
    //void UpdatePoint(SketchParams& sketch, int varIndex, double newValue);
    /**
     * @brief Met à jour une coordonnée dans le vecteur d'état à partir de la variable liée.
     * @param sketch [Entrée] Référence vers l'esquisse.
     * @param varIndex [Entrée] Index de la variable à actualiser.
     * @return void
     */
    void UpdatePoint(SketchParams& sketch, int varIndex);

    /**
     * @brief Exécute un pas de résolution (à appeler au MouseMove).
     * @param sketch [Entrée/Sortie] Référence vers l'esquisse dont la géométrie 3D est recalculée.
     * @return bool True si le pas de résolution a réussi, false sinon.
     */
    // résout (à appeler au MouseMove)
    bool Step(SketchParams& sketch);

    /**
     * @brief Récupère la valeur courante d'une variable du vecteur d'état.
     * @param li_idx [Entrée] Index de la variable.
     * @return double Valeur de la variable pointée.
     */
    double GetVarValue(int li_idx) { return *(variablePointers[li_idx]); }

    /**
     * @brief Récupère les indices X et Y d'une poignée pour le pilotage direct.
     * @param sketch [Entrée] Référence vers l'esquisse.
     * @param primitiveId [Entrée] Identifiant de la primitive.
     * @param handleType [Entrée] Type de poignée.
     * @param outIndexX [Sortie] Index X récupéré.
     * @param outIndexY [Sortie] Index Y récupéré.
     * @return bool True si trouvé, false sinon.
     */
    // Récupère les indices X et Y d'une poignée pour le pilotage direct
    static bool GetIndicesForHandle(SketchParams& sketch, int primitiveId, int handleType, int& outIndexX, int& outIndexY);
    /**
     * @brief Récupère l'ensemble des indices d'une arête complète.
     * @param sketch [Entrée] Référence vers l'esquisse.
     * @param primitiveId [Entrée] Identifiant de la primitive.
     * @param outIndices [Sortie] Vecteur des indices de variables récupérés.
     * @return bool True si trouvé, false sinon.
     */
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


