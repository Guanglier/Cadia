

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




// --------------------------------------------------------------------
//      Afficher une référence
// --------------------------------------------------------------------
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
gp_Pnt2d* Solver2D_Mapper::getPointPointerFromRef(SketchParams& sketch, const GeometryReference& ref) {
    //if (ref.primitiveId == 0) return nullptr;

    SketchPrimitive* prim = sketch.GetPrimitiveMutable(ref.primitiveId);
    if (!prim){
        LOG_ERROR << "ERROR static gp_Pnt2d* getPointPointerFromRef : prim " << std::endl;
        return nullptr;
    }

    gp_Pnt2d* targetPnt = nullptr;

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

    return targetPnt;
}


// --------------------------------------------------------------------
// SYNCHRONISATION : COPIE DE L'ESQUISSE VERS LE VECTEUR X
// --------------------------------------------------------------------
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
void SolverInteractiveSession::pushToSketch() {
    for (size_t i = 0; i < variablePointers.size(); ++i) {
        if (variablePointers[i]) {
            *(variablePointers[i]) = Vector_X[i];
        }
    }
}

void SolverInteractiveSession::Initialize(SketchParams& sketch) {
    // 1. Nettoyage / Réinitialisation des structures internes de la session courante
    variablePointers.clear();
    std::unordered_map<void*, int> pointToXIndex;
    std::unordered_map<void*, int> circleToRIndex;

    // --- Remplissage des variables ---
    for (const auto& primConst : sketch.getPrimitives()) {
        SketchPrimitive& prim = const_cast<SketchPrimitive&>(primConst);
        std::visit([&](auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
            if constexpr (std::is_same_v<T, SketchLine>) {
                pointToXIndex[&concretePrim.start.p2d] = static_cast<int>(variablePointers.size());
                variablePointers.push_back(&(concretePrim.start.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.start.p2d.ChangeCoord().ChangeCoord(2)));

                pointToXIndex[&concretePrim.stop.p2d] = static_cast<int>(variablePointers.size());
                variablePointers.push_back(&(concretePrim.stop.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.stop.p2d.ChangeCoord().ChangeCoord(2)));
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                pointToXIndex[&concretePrim.center.p2d] = static_cast<int>(variablePointers.size());
                variablePointers.push_back(&(concretePrim.center.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.center.p2d.ChangeCoord().ChangeCoord(2)));

                circleToRIndex[&concretePrim] = static_cast<int>(variablePointers.size());
                variablePointers.push_back(&(concretePrim.radius));
            }else{
                LOG_ERROR << " SolverInteractiveSession::Initialize : default ! " << std::endl;
            }
        }, prim);
    }

    int numVariables = static_cast<int>(variablePointers.size());
    this->variablePointers = variablePointers;
    Vector_X.resize(numVariables);

    // Remplir X avec les valeurs actuelles de l'esquisse
    pullFromSketch();

    solver.setNumVariables(numVariables);



    // Fonction utilitaire locale pour récupérer l'index de manière sécurisée
    auto getIndexOrError = [&](gp_Pnt2d* ptr, const std::string& constraintName) -> int {
        if (!ptr) {
            LOG_ERROR << "[Solver] " << constraintName << " : Pointeur de point nul." << std::endl;
            return -1;
        }
        auto it = pointToXIndex.find(ptr);
        if (it == pointToXIndex.end()) {
            LOG_ERROR << "[Solver] " << constraintName << " : Tentative d'utiliser un point qui n'existe plus ou introuvable dans la map !" << std::endl;
            return -1;
        }
        return it->second;
    };
    // Lambda utilitaire pour sécuriser la recherche d'index de rayon de cercle
    auto getRadiusIndexOrError = [&](SketchCircle* circlePtr, const std::string& constraintName) -> int {
        if (!circlePtr) {
            LOG_ERROR << "[Solver] " << constraintName << " : Pointeur de cercle nul." << std::endl;
            return -1;
        }
        auto it = circleToRIndex.find(circlePtr);
        if (it == circleToRIndex.end()) {
            LOG_ERROR << "[Solver] " << constraintName << " : Rayon de cercle introuvable dans la map !" << std::endl;
            return -1;
        }
        return it->second;
    };

    // --- Ajout des contraintes une seule fois ---
    solver.clearConstraints();
    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) continue;

        switch (c.type) {
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
        default: break;
        }
    }

    isInitialized = true;
}


// --------------------------------------------------------------------
// Phase légère : Mise à jour d'une coordonnée et résolution instantanée (pour le MouseMove)
// --------------------------------------------------------------------
void SolverInteractiveSession::UpdatePoint(SketchParams& sketch, int varIndex, double newValue) {
    if (!isInitialized) return;

    // 1. Modifier directement la variable pilotée par la souris dans le vecteur d'état X
    if (varIndex >= 0 && varIndex < Vector_X.size()) {
        Vector_X[varIndex] = newValue;
    }else{
        LOG_ERROR << "[Solver] Tentative de mise à jour d'une variable inexistante ! Index : "
                  + std::to_string(varIndex) + " (Max: " + std::to_string(Vector_X.size())  + ")";
    }
}


// --------------------------------------------------------------------
// Phase légère : Mise à jour d'une coordonnée et résolution instantanée (pour le MouseMove)
// --------------------------------------------------------------------
bool SolverInteractiveSession::Step(SketchParams& sketch) {
    if (!isInitialized) return false;

    // 2. Lancer le solveur incrémentalement sur l'état existant (peu d'itérations suffisent en live)
    //auto t1 = std::chrono::high_resolution_clock::now();
    bool success = solver.solve(Vector_X, 50, 1e-1); // 10 itérations max pour être fluide à 60 FPS
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
    int currentIndex = 0;
    for (const auto& p : sketch.getPrimitives()) {
        bool stop = false;
        std::visit([&](auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                int startX = currentIndex;
                int startY = currentIndex + 1;
                currentIndex += 2;

                int stopX = currentIndex;
                int stopY = currentIndex + 1;
                currentIndex += 2;

                stop = callback(concretePrim.id, std::vector<int>{startX, startY, stopX, stopY});
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                int centerX = currentIndex;
                int centerY = currentIndex + 1;
                currentIndex += 2;

                int radiusIdx = currentIndex;
                currentIndex += 1;

                stop = callback(concretePrim.id, std::vector<int>{centerX, centerY}); // + radiusIdx si besoin un jour
            }
        }, p);

        if (stop) return true;
    }
    return false;
}

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

bool SolverInteractiveSession::GetIndicesForEntireEdge(SketchParams& sketch, int primitiveId, std::vector<int>& outIndices) {
    outIndices.clear();
    SketchPrimitive* prim = sketch.GetPrimitiveMutable(primitiveId);
    if (!prim){
        LOG_ERROR << "[Solver] Impossible de récupérer les indices : la primitive ID "  + std::to_string(primitiveId) + " n'existe plus dans l'esquisse !";
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

    return found;
}




bool SolverOneShot::Diagnostics(SketchParams& sketch){
    return Solve( sketch, true);
}

// --------------------------------------------------------------------

// --------------------------------------------------------------------
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