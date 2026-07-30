/**
 * @file 2DSolver_Mapper.cpp
 * @brief Implémentation du mapping entre les primitives d'une esquisse et le solveur 2D.
 *
 * Ce fichier assure la passerelle entre l'objet esquisse (`SketchParams`), ses primitives
 * géométriques (lignes, cercles), ses contraintes de haut niveau, et le vecteur d'état
 * numérique manipulé par le solveur (`Solver2D_Solver`).
 * Il gère deux modes d'utilisation principaux :
 * 1. Le mode "OneShot" (résolution globale en une passe avec diagnostics complets).
 * 2. Le mode "InteractiveSession" (résolution incrémentale en temps réel lors du déplacement
 *    de poignées de géométrie par la souris à 60 FPS).
 */

#include "2DSolver_Mapper.h"
#include "Logger.h"


#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <type_traits>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <variant>
#include "Logger.h"

#define LOCAL_LOG_LEVEL ::LogLevel::Info


// --------------------------------------------------------------------
//      Afficher une référence
// --------------------------------------------------------------------
/**
 * @brief Formate une référence géométrique sous forme de chaîne de caractères lisible.
 * @param sketch [Entrée] Référence vers l'esquisse contenant les primitives.
 * @param ref [Entrée] Référence géométrique à formater.
 * @return std::string Représentation textuelle de la référence (ex: "Line #1 (Start)").
 */
std::string Solver2D_Mapper::formatRef(const SketchParams& sketch, const GeometryReference& ref) {
    if (ref.primitiveId == 0 && ref.subElement == ConstraintSubElement::Whole) {
        // Au cas où c'est une ref nulle / non assignée
        return "None";
    }

    std::string subStr = "";
    switch (ref.subElement) {
    case ConstraintSubElement::StartPoint: subStr = "Start"; break;
    case ConstraintSubElement::EndPoint:   subStr = "End"; break;
    case ConstraintSubElement::CenterPoint:     subStr = "Center"; break;
    case ConstraintSubElement::Whole:      subStr = "Whole"; break;
    default:                               subStr = "Unknown"; break;
    }

    // Récupération du type de primitive pour le contexte
    std::string typeStr = "Prim";
    if (const auto* prim = const_cast<SketchParams&>(sketch).GetPrimitiveMutable(ref.primitiveId)) {
        std::visit([&](auto& concrete) {
            using T = std::decay_t<decltype(concrete)>;
            if constexpr (std::is_same_v<T, SketchLine>)   typeStr = "Line";
            else if constexpr (std::is_same_v<T, SketchCircle>) typeStr = "Circle";
        }, *prim);
    }

    return typeStr + " #" + std::to_string(ref.primitiveId) + " (" + subStr + ")";
}


// --------------------------------------------------------------------
// Helper pour récupérer le pointeur sur le gp_Pnt2d ciblé par une GeometryReference
// --------------------------------------------------------------------
/**
 * @brief Récupère un pointeur vers le point 2D OpenCASCADE ciblé par une référence géométrique.
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse.
 * @param ref [Entrée] Référence géométrique pointant vers un sous-élément.
 * @return gp_Pnt2d* Pointeur vers le point 2D, ou nullptr en cas d'erreur.
 */
gp_Pnt2d* Solver2D_Mapper::getPointPointerFromRef(SketchParams& sketch, const GeometryReference& ref) {
    //if (ref.primitiveId == 0) return nullptr;

    SketchPrimitive* prim = sketch.GetPrimitiveMutable(ref.primitiveId);
    if (!prim){
        LOG_ERROR << "ERROR static gp_Pnt2d* getPointPointerFromRef : prim " << std::endl;
        return nullptr;
    }

    gp_Pnt2d* targetPnt = nullptr;
/*
    std::visit([&](auto& concretePrim) {
        using T = std::decay_t<decltype(concretePrim)>;

        if constexpr (std::is_same_v<T, SketchLine>) {
            if (ref.subElement == ConstraintSubElement::StartPoint) {
                targetPnt = &(concretePrim.start.p2d);
            } else if (ref.subElement == ConstraintSubElement::EndPoint) {
                targetPnt = &(concretePrim.stop.p2d);
            }else{
                LOG_ERROR << "ERROR DEFAULT dans if (ref.subElement == ConstraintSubElement ) " << std::endl;
            }
        }
        else if constexpr (std::is_same_v<T, SketchCircle>) {
            if (ref.subElement == ConstraintSubElement::CenterPoint || ref.subElement == ConstraintSubElement::Whole) {
                targetPnt = &(concretePrim.center.p2d);
            }
        }else{
            LOG_ERROR << "ERROR else if constexpr (std::is_same_v<T, SketchCircle>) VIDE " << std::endl;
        }
    }, *prim);
*/
    return targetPnt;
}


// --------------------------------------------------------------------
// SYNCHRONISATION : COPIE DE L'ESQUISSE VERS LE VECTEUR X
// --------------------------------------------------------------------
/**
 * @brief Copie les valeurs actuelles des variables de l'esquisse vers le vecteur d'état interne X.
 * @return void
 */
void SolverInteractiveSession::pullFromSketch() {
    for (size_t i = 0; i < variablePointers.size(); ++i) {
        if (variablePointers[i]) {
            Vector_X[i] = *(variablePointers[i]);
        }
    }
}

// --------------------------------------------------------------------
// SYNCHRONISATION : COPIE DU VECTEUR X VERS L'ESQUISSE
// --------------------------------------------------------------------
/**
 * @brief Copie les valeurs calculées du vecteur d'état X vers les variables de l'esquisse.
 * @return void
 */
void SolverInteractiveSession::pushToSketch() {
    for (size_t i = 0; i < variablePointers.size(); ++i) {
        if (variablePointers[i]) {
            *(variablePointers[i]) = Vector_X[i];
        }
    }
}

// --------------------------------------------------------------------
// Phase légère : Mise à jour d'une coordonnée et résolution instantanée (pour le MouseMove)
// --------------------------------------------------------------------

/**
 * @brief Met à jour une variable spécifique dans le vecteur d'état à partir de sa source liée.
 * @param varIndex [Entrée] Index de la variable à actualiser.
 * @return void
 */
void SolverInteractiveSession::UpdatePoint(int varIndex) {
    if (!isInitialized) return;

    // 1. Récupère la nouvelle valeur directement depuis la source pointée par variablePointers
    //    et la reporte dans le vecteur d'état numérique X.
    if (varIndex >= 0 && varIndex < Vector_X.size() && varIndex < variablePointers.size()) {
        Vector_X[varIndex] = *(variablePointers[varIndex]);
    } else {
        LOG_ERROR << "[Solver] Tentative de mise à jour d'une variable inexistante ! Index : "
                  << std::to_string(varIndex) << " (Max: " + std::to_string(Vector_X.size()) + ")";
    }
}

/**
 * @brief Initialise la session interactive lourde (map les variables, remplit X et ajoute les contraintes).
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse à résoudre interactivement.
 * @return void
 */
void SolverInteractiveSession::Initialize(SketchParams& sketch) {
    // 1. Nettoyage / Réinitialisation des structures internes de la session courante
    variablePointers.clear();
    std::unordered_map<uint64_t, int> pointIdToXIndex;
    std::unordered_map<uint64_t, int> circleToRIndex;

    LOG_INFO << "SolverInteractiveSession::Initialize -> entree " << std::endl;

    // --- Étape A : Mapper tous les points de la liste globale de l'esquisse ---
    for (const auto& pt : sketch.getPoints()) {
        uint64_t ptId = pt.id;
        int idxX = static_cast<int>(variablePointers.size());

        pointIdToXIndex[ptId] = idxX;

        // On ajoute l'adresse du X et du Y du gp_Pnt2d
        // (En supposant l'accès aux coordonnées modifiables de OpenCASCADE)
        variablePointers.push_back(&(const_cast<gp_Pnt2d&>(pt.p2d).ChangeCoord().ChangeCoord(1))); // X
        variablePointers.push_back(&(const_cast<gp_Pnt2d&>(pt.p2d).ChangeCoord().ChangeCoord(2))); // Y
    }

    // --- Étape B : Mapper les rayons des cercles ---
    for (const auto& primConst : sketch.getPrimitives()) {
        // Utilisation de std::visit pour identifier le type de primitive
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
            if constexpr (std::is_same_v<T, SketchCircle>) {
                int idxR = static_cast<int>(variablePointers.size());
                circleToRIndex[concretePrim.id] = idxR;
                // Pointeur vers le rayon du cercle
                variablePointers.push_back(&(const_cast<double&>(concretePrim.radius)));
            }
        }, primConst);
    }

    int numVariables = static_cast<int>(variablePointers.size());
    Vector_X.resize(numVariables);
    pullFromSketch();
    solver.setNumVariables(numVariables);

    // --- Ajout des contraintes une seule fois ---
    solver.clearConstraints();


    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) continue;

        switch (c.type) {

            case ConstraintType::Horizontal: {
                SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
                if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                    auto& line = std::get<SketchLine>(*p1);

                    // On récupère les indices X et Y via les IDs de points de la ligne
                    int idxStart = pointIdToXIndex[line.startPointId];
                    int idxStop  = pointIdToXIndex[line.stopPointId];

                    // Pour une contrainte horizontale, on s'assure que Y_start == Y_stop
                    // (donc l'index Y correspond à l'index X + 1)
                    solver.addConstraint(std::make_unique<ConstraintHorizontal>(
                        idxStart + 1, // Y1
                        idxStop + 1   // Y2
                        ));
                }
                break;
            }
            /*
        case ConstraintType::Horizontal: {
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                SketchLine& line = std::get<SketchLine>(*p1);

                int idxStart = getIndexOrError(&line.start.p2d, "Horizontal (start)");
                int idxStop  = getIndexOrError(&line.stop.p2d, "Horizontal (stop)");

                if (idxStart != -1 && idxStop != -1) {
                    solver.addConstraint(std::make_unique<ConstraintHorizontal>(
                        idxStart + 1,
                        idxStop + 1
                        ));
                }
            } else {
                LOG_ERROR << "[Solver] Horizontal : Primitive ID " << c.ref1.primitiveId << " introuvable ou ce n'est pas une ligne !" << std::endl;
            }
            break;
        }
        case ConstraintType::Vertical: {
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                SketchLine& line = std::get<SketchLine>(*p1);

                int idxStart = getIndexOrError(&line.start.p2d, "Vertical (start)");
                int idxStop  = getIndexOrError(&line.stop.p2d, "Vertical (stop)");

                if (idxStart != -1 && idxStop != -1) {
                    solver.addConstraint(std::make_unique<ConstraintVertical>(
                        idxStart,
                        idxStop
                        ));
                }
            } else {
                LOG_ERROR << "[Solver] Vertical : Primitive ID " << c.ref1.primitiveId << " introuvable ou ce n'est pas une ligne !" << std::endl;
            }
            break;
        }
        case ConstraintType::Coincident: {
            gp_Pnt2d* p1 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref2);

            int idx1 = getIndexOrError(p1, "Coincident (ref1)");
            int idx2 = getIndexOrError(p2, "Coincident (ref2)");

            if (idx1 != -1 && idx2 != -1) {
                solver.addConstraint(std::make_unique<ConstraintCoincident1D>(idx1, idx2));
                solver.addConstraint(std::make_unique<ConstraintCoincident1D>(idx1 + 1, idx2 + 1));
            }
            break;
        }
        case ConstraintType::Distance: {
            gp_Pnt2d* p1 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref2);

            int idx1 = getIndexOrError(p1, "Distance (ref1)");
            int idx2 = getIndexOrError(p2, "Distance (ref2)");

            if (idx1 != -1 && idx2 != -1) {
                solver.addConstraint(std::make_unique<ConstraintDistancePointPoint>(
                    idx1, idx1 + 1,
                    idx2, idx2 + 1,
                    c.value
                    ));
            }
            break;
        }
        case ConstraintType::Radius: {
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (prim && std::holds_alternative<SketchCircle>(*prim)) {
                SketchCircle& circle = std::get<SketchCircle>(*prim);

                int rIdx = getRadiusIndexOrError(&circle, "Radius");
                if (rIdx != -1) {
                    solver.addConstraint(std::make_unique<ConstraintRadius>(rIdx, c.value));
                }
            } else {
                LOG_ERROR << "[Solver] Radius : Primitive ID " << c.ref1.primitiveId << " introuvable ou ce n'est pas un cercle !" << std::endl;
            }
            break;
        }
        case ConstraintType::Perpendicular: {
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            SketchPrimitive* p2 = sketch.GetPrimitiveMutable(c.ref2.primitiveId);

            if (p1 && p2 && std::holds_alternative<SketchLine>(*p1) && std::holds_alternative<SketchLine>(*p2)) {
                auto& line1 = std::get<SketchLine>(*p1);
                auto& line2 = std::get<SketchLine>(*p2);

                int l1_startX = getIndexOrError(&line1.start.p2d, "Perpendicular (line1.start.X)");
                int l1_startY = getIndexOrError(&line1.start.p2d, "Perpendicular (line1.start.Y)"); // même index de base, +1 pour Y
                int l1_stopX  = getIndexOrError(&line1.stop.p2d,  "Perpendicular (line1.stop.X)");
                int l1_stopY  = getIndexOrError(&line1.stop.p2d,  "Perpendicular (line1.stop.Y)");

                int l2_startX = getIndexOrError(&line2.start.p2d, "Perpendicular (line2.start.X)");
                int l2_startY = getIndexOrError(&line2.start.p2d, "Perpendicular (line2.start.Y)");
                int l2_stopX  = getIndexOrError(&line2.stop.p2d,  "Perpendicular (line2.stop.X)");
                int l2_stopY  = getIndexOrError(&line2.stop.p2d,  "Perpendicular (line2.stop.Y)");

                if (l1_startX != -1 && l1_stopX != -1 && l2_startX != -1 && l2_stopX != -1) {
                    solver.addConstraint(std::make_unique<ConstraintPerpendicular>(
                        l1_startX, l1_startY + 1,
                        l1_stopX,  l1_stopY + 1,
                        l2_startX, l2_startY + 1,
                        l2_stopX,  l2_stopY + 1
                        ));
                }
            } else {
                LOG_ERROR << "[Solver] Perpendicular : Une ou plusieurs primitives sont introuvables ou ne sont pas des lignes !" << std::endl;
            }
            break;

        }
        */
        default:
            LOG_WARN << "SolverInteractiveSession::Initialize -> default dans switch (c.type) " << std::endl;
            break;
        }
    }

    isInitialized = true;
}




// --------------------------------------------------------------------
// Phase légère : Mise à jour d'une coordonnée et résolution instantanée (pour le MouseMove)
// --------------------------------------------------------------------
/**
 * @brief Exécute une étape de résolution incrémentale et met à jour la géométrie 3D de l'esquisse.
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse.
 * @return bool True si la résolution a réussi, false sinon.
 */
bool SolverInteractiveSession::Step(SketchParams& sketch) {
    if (!isInitialized) return false;

    // 2. Lancer le solveur incrémentalement sur l'état existant (peu d'itérations suffisent en live)
    //auto t1 = std::chrono::high_resolution_clock::now();
    bool success = solver.solve(Vector_X, 50, 1e-4); // 10 itérations max pour être fluide à 60 FPS
    //auto t2 = std::chrono::high_resolution_clock::now();

    // 3. Réinjecter les valeurs calculées dans l'esquisse
    pushToSketch();

    // 4. Mettre à jour la géométrie 3D
    //auto t3 = std::chrono::high_resolution_clock::now();
    sketch.recomputeGeometry3D();
    //auto t4 = std::chrono::high_resolution_clock::now();

    //auto solveTime = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    //auto recomputeGeometry3DTime = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
    //std::cout << "  Solve: " << solveTime/1000 << " recomputeGeometry3DTime: " << recomputeGeometry3DTime  << "us ";

    return success;
}



// Fonction utilitaire interne pour éviter de dupliquer la boucle d'indexation
template<typename F>
bool ForEachPrimitiveIndex(SketchParams& sketch, F&& callback) {
    // On reconstruit dynamiquement la map des points (ID -> Index dans variablePointers)
    // exactement de la même manière que dans Initialize()
    std::unordered_map<uint64_t, int> pointIdToXIndex;
    int currentVarIndex = 0;

    for (const auto& pt : sketch.getPoints()) {
        pointIdToXIndex[pt.id] = currentVarIndex;
        currentVarIndex += 2; // X et Y pour chaque point
    }

    // On parcourt les primitives et on extrait les exacts indices de leurs points
    for (const auto& p : sketch.getPrimitives()) {
        bool stop = false;
        std::visit([&](auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                if (pointIdToXIndex.count(concretePrim.startPointId) && pointIdToXIndex.count(concretePrim.stopPointId)) {
                    int startX = pointIdToXIndex[concretePrim.startPointId];
                    int startY = startX + 1;
                    int stopX = pointIdToXIndex[concretePrim.stopPointId];
                    int stopY = stopX + 1;

                    stop = callback(concretePrim.id, std::vector<int>{startX, startY, stopX, stopY});
                }
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                if (pointIdToXIndex.count(concretePrim.centerPointId)) {
                    int centerX = pointIdToXIndex[concretePrim.centerPointId];
                    int centerY = centerX + 1;

                    stop = callback(concretePrim.id, std::vector<int>{centerX, centerY});
                }
            }
        }, p);

        if (stop) return true;
    }
    return false;
}

/**
 * @brief Récupère les indices X et Y d'une poignée de primitive pour le pilotage direct.
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse.
 * @param primitiveId [Entrée] Identifiant de la primitive cible.
 * @param handleType [Entrée] Type de poignée recherchée (1: début, 2: fin, 3: centre).
 * @param outIndexX [Sortie] Référence pour stocker l'index X trouvé.
 * @param outIndexY [Sortie] Référence pour stocker l'index Y trouvé.
 * @return bool True si les indices ont été trouvés, false sinon.
 */
bool SolverInteractiveSession::GetIndicesForHandle(SketchParams& sketch, int primitiveId, int handleType, int& outIndexX, int& outIndexY) {
    SketchPrimitive* prim = sketch.GetPrimitiveMutable(primitiveId);
    if (!prim) return false;

    bool found = false;
    ForEachPrimitiveIndex(sketch, [&](int id, const std::vector<int>& indices) {
        if (id == primitiveId) {
            if (indices.size() >= 4) { // C'est une ligne
                if (handleType == 1) { // Point de départ
                    outIndexX = indices[0];
                    outIndexY = indices[1];
                    found = true;
                    return true;
                } else if (handleType == 2) { // Point d'arrivée
                    outIndexX = indices[2];
                    outIndexY = indices[3];
                    found = true;
                    return true;
                }
            } else if (indices.size() >= 2) { // C'est un cercle
                if (handleType == 3) { // Centre
                    outIndexX = indices[0];
                    outIndexY = indices[1];
                    found = true;
                    return true;
                }
            }
        }
        return false;
    });

    return found;
}
bool SolverInteractiveSession::GetIndicesForHandle(SketchParams& sketch, uint64_t targetPointId, int& outIndexX, int& outIndexY) {
    // On reconstruit la map des points (ID -> Index) exactement comme dans ForEachPrimitiveIndex
    std::unordered_map<uint64_t, int> pointIdToXIndex;
    int currentVarIndex = 0;

    for (const auto& pt : sketch.getPoints()) {
        pointIdToXIndex[pt.id] = currentVarIndex;
        currentVarIndex += 2;
    }

    // On cherche directement si le point recherché existe dans la map
    auto it = pointIdToXIndex.find(targetPointId);
    if (it != pointIdToXIndex.end()) {
        outIndexX = it->second;
        outIndexY = it->second + 1;
        return true;
    }

    return false;
}

/**
 * @brief Récupère l'ensemble des indices associés à une arête/primitive complète.
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse.
 * @param primitiveId [Entrée] Identifiant de la primitive cible.
 * @param outIndices [Sortie] Vecteur contenant la liste de tous les indices de variables de la primitive.
 * @return bool True si la primitive existe et les indices ont été récupérés, false sinon.
 */
bool SolverInteractiveSession::GetIndicesForEntireEdge(SketchParams& sketch, int primitiveId, std::vector<int>& outIndices) {
    outIndices.clear();
    SketchPrimitive* prim = sketch.GetPrimitiveMutable(primitiveId);
    if (!prim){
        LOG_ERROR << "[Solver] : SolverInteractiveSession::GetIndicesForEntireEdge: Impossible de récupérer les indices : la primitive ID "  + std::to_string(primitiveId) + " n'existe plus dans l'esquisse !" << std::endl;
        return false;
    }

    bool found = false;
    ForEachPrimitiveIndex(sketch, [&](int id, const std::vector<int>& indices) {
        if (id == primitiveId) {
            outIndices = indices;
            found = true;
            return true;
        }
        return false;
    });
    if ( false == found ){
        LOG_ERROR << "[Solver] : SolverInteractiveSession::GetIndicesForEntireEdge : not found ID "  << std::to_string(primitiveId) << std::endl ;
    }

    return found;
}



/**
 * @brief Exécute les diagnostics complets du solveur en mode OneShot.
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse à diagnostiquer.
 * @return bool True si l'exécution s'est déroulée correctement, false sinon.
 */
bool SolverOneShot::Diagnostics(SketchParams& sketch){
    return Solve( sketch, true);
}

// --------------------------------------------------------------------

// --------------------------------------------------------------------
/**
 * @brief Résout l'ensemble des contraintes d'une esquisse en une seule passe globale (OneShot).
 * @param sketch [Entrée/Sortie] Référence vers l'esquisse à résoudre.
 * @param enableDiagnostics [Entrée] Active ou non les logs de diagnostic (défaut : false).
 * @return bool True si la résolution globale a convergé, false en cas d'échec.
 */
bool SolverOneShot::Solve(SketchParams& sketch, bool enableDiagnostics) {
    if (enableDiagnostics) {
        LOG_DEBUG << "\n==================================================" << std::endl;
        LOG_DEBUG << "         DIAGNOSTIC DU SOLVEUR 2D (OneShot)       " << std::endl;
        LOG_DEBUG << "==================================================" << std::endl;
    } else {
        LOG_DEBUG << "Solver2D_Mapper::PrepareAndSolve(SketchParams& sketch)" << std::endl;
    }

    std::vector<double*> variablePointers;
    std::unordered_map<uint64_t, int> pointIdToXIndex;
    std::unordered_map<uint64_t, int> circleToRIndex;

    // --- 1. MAPPING DES VARIABLES DE L'ESQUISSE ---
    if (enableDiagnostics) LOG_DEBUG << "\n--- 1. VARIABLES MAPPÉES (DANS VECTEUR X) ---" << std::endl;

    // A. Mapping de tous les points globaux de l'esquisse
    for (const auto& pt : sketch.getPoints()) {
        uint64_t ptId = pt.id;
        int idxX = static_cast<int>(variablePointers.size());

        pointIdToXIndex[ptId] = idxX;

        // Pointeurs vers X et Y du SketchPoint (via const_cast pour accéder aux modificateurs OCC)
        variablePointers.push_back(&(const_cast<gp_Pnt2d&>(pt.p2d).ChangeCoord().ChangeCoord(1))); // X
        variablePointers.push_back(&(const_cast<gp_Pnt2d&>(pt.p2d).ChangeCoord().ChangeCoord(2))); // Y

        if (enableDiagnostics) {
            LOG_DEBUG << " [Point ID " << ptId << "]\n"
                      << "    -> Coords (X" << idxX << ", Y" << idxX+1 << ") = ("
                      << pt.p2d.X() << ", " << pt.p2d.Y() << ")\n";
        }
    }

    // B. Mapping des rayons des cercles et autres paramètres spécifiques
    for (const auto& primConst : sketch.getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
            if constexpr (std::is_same_v<T, SketchCircle>) {
                int idxR = static_cast<int>(variablePointers.size());
                circleToRIndex[concretePrim.id] = idxR;
                variablePointers.push_back(&(const_cast<double&>(concretePrim.radius)));

                if (enableDiagnostics) {
                    LOG_DEBUG << " [Circle ID " << concretePrim.id << "]\n"
                              << "    -> Radius (R" << idxR << ") = " << concretePrim.radius << "\n";
                }
            }
        }, primConst);
    }

    int numVariables = static_cast<int>(variablePointers.size());
    if (enableDiagnostics) LOG_INFO << " Total variables (Cols de la Jacobienne) : " << numVariables << std::endl;
    if (numVariables == 0) return true;

    // Construction du vecteur d'état initial X
    Eigen::VectorXd X(numVariables);
    for (int i = 0; i < numVariables; ++i) {
        X[i] = *(variablePointers[i]);
    }

    // --- 2. TRADUCTION DES CONTRAINTES ---
    if (enableDiagnostics) LOG_DEBUG << "\n--- 2. EVALUATION DES CONTRAINTES ---" << std::endl;
    std::vector<std::unique_ptr<IConstraint2D>> constraints;

    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) {
            if (enableDiagnostics) LOG_DEBUG << " [Constraint ID " << c.id << "] -> IGNOREE (Pilotee/Driven)\n";
            continue;
        }

        std::string cTypeStr = "";
        switch (c.type) {
        case ConstraintType::Horizontal: {
            cTypeStr = "Horizontal";
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                auto& line = std::get<SketchLine>(*p1);
                constraints.push_back(std::make_unique<ConstraintHorizontal>(
                    pointIdToXIndex[line.startPointId] + 1,
                    pointIdToXIndex[line.stopPointId] + 1
                    ));
            } else {
                LOG_ERROR << "ERROR: Horizontal attend une ligne valide." << std::endl;
            }
            break;
        }
        case ConstraintType::Vertical: {
            cTypeStr = "Vertical";
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                auto& line = std::get<SketchLine>(*p1);
                constraints.push_back(std::make_unique<ConstraintVertical>(
                    pointIdToXIndex[line.startPointId],
                    pointIdToXIndex[line.stopPointId]
                    ));
            } else {
                LOG_ERROR << "ERROR: Vertical attend une ligne valide." << std::endl;
            }
            break;
        }
            /*
        case ConstraintType::Coincident: {
            cTypeStr = "Coincident (2 eq.)";
            gp_Pnt2d* p1 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref2);
            // Récupération des IDs associés via les pointeurs ou adaptation de la logique de référence
            // (Si getPointPointerFromRef retourne un pointeur, on peut retrouver l'index, ou utiliser les helpers d'ID)
            // Alternative propre basée sur les IDs si vos helpers le supportent :
            uint64_t id1 = sketch.findPointIdFromRef(c.ref1); // Assurez-vous d'avoir une méthode équivalente ou utilisez la map inverse
            uint64_t id2 = sketch.findPointIdFromRef(c.ref2);
            if (pointIdToXIndex.count(id1) && pointIdToXIndex.count(id2)) {
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointIdToXIndex[id1], pointIdToXIndex[id2]));
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointIdToXIndex[id1] + 1, pointIdToXIndex[id2] + 1));
            }
            break;

        }
        */
        case ConstraintType::Radius: {
            cTypeStr = "Radius (" + std::to_string(c.value) + ")";
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (prim && std::holds_alternative<SketchCircle>(*prim)) {
                auto& circle = std::get<SketchCircle>(*prim);
                constraints.push_back(std::make_unique<ConstraintRadius>(circleToRIndex[circle.id], c.value));
            }
            break;
        }
        default:
            cTypeStr = "Type non géré";
            LOG_ERROR << "\tERROR : Type de contrainte non géré" << std::endl;
            break;
        }

        if (enableDiagnostics) {
            LOG_DEBUG << " [Constraint ID " << c.id << "] " << cTypeStr << "\n";
        }
    }

    // Affichage optionnel des résidus et DOF avant résolution
    if (enableDiagnostics) {
        int numEquations = 0;
        double totalSquaredError = 0.0;
        for (const auto& c : constraints) {
            double err = c->calcError(X);
            totalSquaredError += err * err;
            numEquations++;
        }
        LOG_INFO << " Total équations générées : " << numEquations << std::endl;
        LOG_INFO << " Erreur globale initiale (L2) : " << std::sqrt(totalSquaredError) << std::endl;

        int dof = numVariables - numEquations;
        LOG_INFO << "\n--- 3. BILAN DES DEGRÉS DE LIBERTÉ (DOF) ---" << std::endl;
        LOG_INFO << " Degrés de Liberté : " << dof;
        if (dof > 0) LOG_INFO << " -> Sous-contraint\n";
        else if (dof == 0) LOG_INFO << " -> Isostatique\n";
        else LOG_INFO << " -> Sur-contraint\n";
        LOG_DEBUG << "==================================================\n" << std::endl;
    }

    // --- 3. RESOLUTION ---
    Solver2D_Solver solver;
    solver.setNumVariables(numVariables);
    for (auto& c : constraints) {
        solver.addConstraint(std::move(c));
    }

    bool success = solver.solve(X);
    if (!success) {
        LOG_WARN << "\t[2DSolver_Mapper] Echec de convergence !" << std::endl;
        return false;
    }

    // Réinjection des valeurs calculées dans l'esquisse
    for (int i = 0; i < numVariables; ++i) {
        *(variablePointers[i]) = X[i];
    }

    LOG_INFO << "\tFIN fonction SolverOneShot::Solve" << std::endl;
    sketch.recomputeGeometry3D();

    return true;
}

/*
bool SolverOneShot::Solve(SketchParams& sketch, bool enableDiagnostics) {
    if (enableDiagnostics) {
        LOG_DEBUG << "\n==================================================" << std::endl;
        LOG_DEBUG << "         DIAGNOSTIC DU SOLVEUR 2D (OneShot)       " << std::endl;
        LOG_DEBUG << "==================================================" << std::endl;
    } else {
        LOG_DEBUG << "Solver2D_Mapper::PrepareAndSolve(SketchParams& sketch)" << std::endl;
    }

    std::vector<double*> variablePointers;
    std::unordered_map<void*, int> pointToXIndex;
    std::unordered_map<void*, int> circleToRIndex;

    // --- 1. MAPPING DES VARIABLES DE L'ESQUISSE ---
    if (enableDiagnostics) LOG_DEBUG << "\n--- 1. VARIABLES MAPPÉES (DANS VECTEUR X) ---" << std::endl;

    for (const auto& primConst : sketch.getPrimitives()) {
        SketchPrimitive& prim = const_cast<SketchPrimitive&>(primConst);

        std::visit([&](auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                int idxStart = static_cast<int>(variablePointers.size());
                pointToXIndex[&concretePrim.start.p2d] = idxStart;
                variablePointers.push_back(&(concretePrim.start.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.start.p2d.ChangeCoord().ChangeCoord(2)));

                int idxEnd = static_cast<int>(variablePointers.size());
                pointToXIndex[&concretePrim.stop.p2d] = idxEnd;
                variablePointers.push_back(&(concretePrim.stop.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.stop.p2d.ChangeCoord().ChangeCoord(2)));

                if (enableDiagnostics) {
                    LOG_DEBUG << " [Line ID " << concretePrim.id << "]\n"
                              << "   -> Start (X" << idxStart << ", Y" << idxStart+1 << ") = ("
                              << concretePrim.start.p2d.X() << ", " << concretePrim.start.p2d.Y() << ")\n"
                              << "   -> End   (X" << idxEnd << ", Y" << idxEnd+1 << ") = ("
                              << concretePrim.stop.p2d.X() << ", " << concretePrim.stop.p2d.Y() << ")\n";
                }
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                int idxCenter = static_cast<int>(variablePointers.size());
                pointToXIndex[&concretePrim.center.p2d] = idxCenter;
                variablePointers.push_back(&(concretePrim.center.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.center.p2d.ChangeCoord().ChangeCoord(2)));

                int idxR = static_cast<int>(variablePointers.size());
                circleToRIndex[&concretePrim] = idxR;
                variablePointers.push_back(&(concretePrim.radius));

                if (enableDiagnostics) {
                    LOG_DEBUG << " [Circle ID " << concretePrim.id << "]\n"
                              << "   -> Center (X" << idxCenter << ", Y" << idxCenter+1 << ") = ("
                              << concretePrim.center.p2d.X() << ", " << concretePrim.center.p2d.Y() << ")\n"
                              << "   -> Radius (R" << idxR << ") = " << concretePrim.radius << "\n";
                }
            }else{
                LOG_ERROR << " SolverOneShot::Solve : default case " << std::endl;
            }
        }, prim);
    }

    int numVariables = static_cast<int>(variablePointers.size());
    if (enableDiagnostics) LOG_INFO << " Total variables (Cols de la Jacobienne) : " << numVariables << std::endl;
    if (numVariables == 0) return true;

    // Construction du vecteur d'état initial X
    Eigen::VectorXd X(numVariables);
    for (int i = 0; i < numVariables; ++i) {
        X[i] = *(variablePointers[i]);
    }

    // --- 2. TRADUCTION DES CONTRAINTES ---
    if (enableDiagnostics) LOG_DEBUG << "\n--- 2. EVALUATION DES CONTRAINTES ---" << std::endl;
    std::vector<std::unique_ptr<IConstraint2D>> constraints;

    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) {
            if (enableDiagnostics) LOG_DEBUG << " [Constraint ID " << c.id << "] -> IGNOREE (Pilotee/Driven)\n";
            continue;
        }

        std::string cTypeStr = "";
        switch (c.type) {
        case ConstraintType::Horizontal: {
            cTypeStr = "Horizontal";
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                SketchLine& line = std::get<SketchLine>(*p1);
                constraints.push_back(std::make_unique<ConstraintHorizontal>(
                    pointToXIndex[&line.start.p2d] + 1,
                    pointToXIndex[&line.stop.p2d] + 1
                    ));
            } else {
                LOG_ERROR << "ERROR: Horizontal attend une ligne valide." << std::endl;
            }
            break;
        }
        case ConstraintType::Vertical: {
            cTypeStr = "Vertical";
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                SketchLine& line = std::get<SketchLine>(*p1);
                constraints.push_back(std::make_unique<ConstraintVertical>(
                    pointToXIndex[&line.start.p2d],
                    pointToXIndex[&line.stop.p2d]
                    ));
            } else {
                LOG_ERROR << "ERROR: Vertical attend une ligne valide." << std::endl;
            }
            break;
        }
        case ConstraintType::Coincident: {
            cTypeStr = "Coincident (2 eq.)";
            gp_Pnt2d* p1 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2) {
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointToXIndex[p1], pointToXIndex[p2]));
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointToXIndex[p1] + 1, pointToXIndex[p2] + 1));
            }
            break;
        }
        case ConstraintType::Distance: {
            cTypeStr = "Distance (" + std::to_string(c.value) + ")";
            gp_Pnt2d* p1 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2) {
                constraints.push_back(std::make_unique<ConstraintDistancePointPoint>(
                    pointToXIndex[p1], pointToXIndex[p1] + 1,
                    pointToXIndex[p2], pointToXIndex[p2] + 1, c.value));
            }
            break;
        }
        case ConstraintType::Radius: {
            cTypeStr = "Radius (" + std::to_string(c.value) + ")";
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (prim && std::holds_alternative<SketchCircle>(*prim)) {
                constraints.push_back(std::make_unique<ConstraintRadius>(circleToRIndex[&std::get<SketchCircle>(*prim)], c.value));
            }
            break;
        }
        case ConstraintType::Perpendicular: {
            cTypeStr = "Perpendicular";
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            SketchPrimitive* p2 = sketch.GetPrimitiveMutable(c.ref2.primitiveId);
            if (p1 && p2 && std::holds_alternative<SketchLine>(*p1) && std::holds_alternative<SketchLine>(*p2)) {
                auto& l1 = std::get<SketchLine>(*p1);
                auto& l2 = std::get<SketchLine>(*p2);
                constraints.push_back(std::make_unique<ConstraintPerpendicular>(
                    pointToXIndex[&l1.start.p2d], pointToXIndex[&l1.start.p2d] + 1,
                    pointToXIndex[&l1.stop.p2d],  pointToXIndex[&l1.stop.p2d] + 1,
                    pointToXIndex[&l2.start.p2d], pointToXIndex[&l2.start.p2d] + 1,
                    pointToXIndex[&l2.stop.p2d],  pointToXIndex[&l2.stop.p2d] + 1
                    ));
            }
            break;
        }
        case ConstraintType::Parallel: {
            cTypeStr = "Parallel";
            SketchPrimitive* prim1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            SketchPrimitive* prim2 = sketch.GetPrimitiveMutable(c.ref2.primitiveId);
            if (prim1 && prim2 && std::holds_alternative<SketchLine>(*prim1) && std::holds_alternative<SketchLine>(*prim2)) {
                auto& l1 = std::get<SketchLine>(*prim1);
                auto& l2 = std::get<SketchLine>(*prim2);
                constraints.push_back(std::make_unique<ConstraintParallel>(
                    pointToXIndex[&l1.start.p2d], pointToXIndex[&l1.start.p2d] + 1,
                    pointToXIndex[&l1.stop.p2d],  pointToXIndex[&l1.stop.p2d] + 1,
                    pointToXIndex[&l2.start.p2d], pointToXIndex[&l2.start.p2d] + 1,
                    pointToXIndex[&l2.stop.p2d],  pointToXIndex[&l2.stop.p2d] + 1
                    ));
            }
            break;
        }
        default:
            cTypeStr = "Type non géré";
            LOG_ERROR << "\tERROR : Type de contrainte non géré" << std::endl;
            break;
        }

        if (enableDiagnostics) {
            LOG_DEBUG << " [Constraint ID " << c.id << "] " << cTypeStr
                      << " | Ref1: " << Solver2D_Mapper::formatRef(sketch, c.ref1)
                      << " | Ref2: " << Solver2D_Mapper::formatRef(sketch, c.ref2) << "\n";
        }
    }

    // Affichage optionnel des résidus et DOF avant résolution (si diagnostic demandé)
    if (enableDiagnostics) {
        int numEquations = 0;
        double totalSquaredError = 0.0;
        for (const auto& c : constraints) {
            double err = c->calcError(X);
            totalSquaredError += err * err;
            numEquations++;
        }
        LOG_INFO << " Total équations générées : " << numEquations << std::endl;
        LOG_INFO << " Erreur globale initiale (L2) : " << std::sqrt(totalSquaredError) << std::endl;

        int dof = numVariables - numEquations;
        LOG_INFO << "\n--- 3. BILAN DES DEGRÉS DE LIBERTÉ (DOF) ---" << std::endl;
        LOG_INFO << " Degrés de Liberté : " << dof;
        if (dof > 0) LOG_INFO << " -> Sous-contraint\n";
        else if (dof == 0) LOG_INFO << " -> Isostatique\n";
        else LOG_INFO << " -> Sur-contraint\n";
        LOG_DEBUG << "==================================================\n" << std::endl;
    }

    // --- 3. RESOLUTION ---
    Solver2D_Solver solver;
    solver.setNumVariables(numVariables);
    for (auto& c : constraints) {
        solver.addConstraint(std::move(c));
    }

    bool success = solver.solve(X);
    if (!success) {
        LOG_WARN << "\t[2DSolver_Mapper] Echec de convergence !" << std::endl;
        return false;
    }

    // Réinjection des valeurs calculées dans l'esquisse
    for (int i = 0; i < numVariables; ++i) {
        *(variablePointers[i]) = X[i];
    }

    // --- 4. POST-TRAITEMENT : ÉGALISATION STRICTE DES COÏNCIDENCES 2D ---
    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) continue;
        if (c.type == ConstraintType::Coincident) {
            gp_Pnt2d* p1 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = Solver2D_Mapper::getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2 && p1 != p2) {
                double avgX = (p1->X() + p2->X()) * 0.5;
                double avgY = (p1->Y() + p2->Y()) * 0.5;
                p1->SetCoord(avgX, avgY);
                p2->SetCoord(avgX, avgY);
            }
        }
    }

    LOG_INFO << "\tFIN fonction SolverOneShot::Solve" << std::endl;
    sketch.recomputeGeometry3D();

    return true;
}
*/


