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

/**
 * ============================================================================
 * DIAGRAMME DE FLUX ET ARCHITECTURE INTERNE DU SOLVEUR 2D
 * ============================================================================
 *
 * 1. PHASE D'INITIALISATION (MousePress / Chargement)
 *    -------------------------------------------------------------------------
 *    [SketchParams] (Primitives & Contraintes de haut niveau dans CAD_Operation)
 *          │
          ▼  (Parcours des primitives : Lignes, Cercles)
 *    [SolverInteractiveSession::Initialize()]
 *          ├─► Extrait les pointeurs des coordonnées (X, Y, Rayon)
 *          │   et les stocke dans le vecteur : `variablePointers` (std::vector<double*>)
 *          │
 *          ├─► Redimensionne le vecteur d'état numérique : `Vector_X` (Eigen::VectorXd)
 *          │   et copie les valeurs initiales depuis l'esquisse via `pullFromSketch()`
 *          │
 *          └─► Traduit les contraintes géométriques de l'esquisse (Horizontal, Coïncident, etc.)
 *              en objets de contraintes mathématiques (`IConstraint2D`) injectés
 *              dans le `Solver2D_Solver` via `addConstraint()`.
 *
 *
 * 2. PHASE INTERACTIVE EN TEMPS RÉEL (MouseMove / Glissement de poignée à 60 FPS)
 *    -------------------------------------------------------------------------
 *    [Utilisateur déplace une poignée à la souris]
 *          │
            ▼
*    [SolverInteractiveSession::UpdatePoint(int varIndex)]
 *          ├─► Reçoit uniquement le `varIndex` (l'indice de la variable manipulée).
 *          │
 *          ├─► Utilise ce `varIndex` pour aller piocher directement dans le
 *          │   vecteur interne des pointeurs :
 *          │   `variablePointers[varIndex]` (qui pointe vers la variable native
 *          │   modifiée en amont par la souris dans l'esquisse).
 *          │
 *          ├─► Récupère la nouvelle valeur à cette adresse mémoire et la reporte
 *          │   dans le vecteur d'état numérique `Vector_X` à la même position `varIndex`.
 *          │
 *          ▼
 *    [SolverInteractiveSession::Step()]
 *          ├─► Appelle `solver.solve(Vector_X)` (Optimisation de Levenberg-Marquardt) :
 *          │     ├─ Calcule les erreurs résiduelles f(X) pour chaque contrainte.
 *          │     ├─ Construit la matrice Jacobienne J par dérivations analytiques/numériques.
 *          │     └─ Résout le système amorti (JT * J + lambda * I) * deltaX = -JT * f(X)
 *          │
 *          ├─► Répercute les nouvelles coordonnées calculées de `Vector_X` vers
 *              les variables de l'esquisse via `pushToSketch()`.
 *          │
          └─► Appelle `sketch.recomputeGeometry3D()` pour mettre à jour la
              représentation graphique 3D (OpenCASCADE / VTK).
 * ============================================================================
 */






#pragma once

#include <External/Eigen/Dense>
#include <vector>
#include <memory>
#include "2DSolver_Constraints.h"
#include "2DSolver_Solver.h"
#include "CAD_PartOp.h"

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
    static bool Solve(SketchParams& sketch, bool enableDiagnostics=false);

    /**
     * @brief Lance les diagnostics globaux de l'esquisse.
     * @param sketch [Entrée/Sortie] Référence vers l'esquisse à analyser.
     * @return bool True si le diagnostic s'est exécuté, false sinon.
     */
    static bool Diagnostics(SketchParams& sketch);
};

// ============================================================================
// 2. STRUCTURE POUR LE SOLVE INCRÉMENTAL (Session Interactive / Drag)
// ============================================================================
struct SolverInteractiveSession {
    Solver2D_Solver solver;
    Eigen::VectorXd Vector_X;

    // vecteur plat (unidimensionnel) de pointeurs vers des double (std::vector<double*>),
    // chaque point géométrique 2D (qui possède une coordonnée $X$ et une coordonnée $Y$)
    // va occuper 2 entrées consécutives dans ce vecteur :
    // L'indice i pointe vers la coordonnée X.L'indice i + 1 pointe vers la coordonnée Y.
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
    void Initialize(SketchParams& sketch);


    /**
     * @brief Met à jour une coordonnée dans le vecteur d'état à partir de la variable liée.
     * @param sketch [Entrée] Référence vers l'esquisse.
     * @param varIndex [Entrée] Index de la variable à actualiser.
     * @return void
     */
    void UpdatePoint(int varIndex);

    /**
     * @brief Exécute un pas de résolution (à appeler au MouseMove).
     * @param sketch [Entrée/Sortie] Référence vers l'esquisse dont la géométrie 3D est recalculée.
     * @return bool True si le pas de résolution a réussi, false sinon.
     */
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
    static bool GetIndicesForHandle(SketchParams& sketch, int primitiveId, int handleType, int& outIndexX, int& outIndexY);
    static bool GetIndicesForHandle(SketchParams& sketch, uint64_t targetPointId, int& outIndexX, int& outIndexY);
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
    static std::string formatRef(const SketchParams& sketch, const PartSketchConstraint::RefGeometry& ref);
    static gp_Pnt2d* getPointPointerFromRef(SketchParams& sketch, const PartSketchConstraint::RefGeometry& ref);

public:
    Solver2D_Mapper() = default;


};


