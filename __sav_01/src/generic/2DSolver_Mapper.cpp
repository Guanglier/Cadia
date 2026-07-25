

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



// Helper pour récupérer le pointeur sur le gp_Pnt2d ciblé par une GeometryReference
gp_Pnt2d* Solver2D_Mapper::getPointPointerFromRef(SketchParams& sketch, const GeometryReference& ref) {
    //if (ref.primitiveId == 0) return nullptr;

    SketchPrimitive* prim = sketch.GetPrimitiveMutable(ref.primitiveId);
    if (!prim){
        std::cout<< "ERROR static gp_Pnt2d* getPointPointerFromRef : prim " << std::endl;
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
                std::cout<< "ERROR DEFAULT dans if (ref.subElement == ConstraintSubElement::??) " << std::endl;
            }
        }
        else if constexpr (std::is_same_v<T, SketchCircle>) {
            if (ref.subElement == ConstraintSubElement::CenterPoint || ref.subElement == ConstraintSubElement::Whole) {
                targetPnt = &(concretePrim.center.p2d);
            }
        }else{
            std::cout<< "ERROR else if constexpr (std::is_same_v<T, SketchCircle>) VIDE " << std::endl;
        }
    }, *prim);

    return targetPnt;
}



// --------------------------------------------------------------------
// FONCTION DE DIAGNOSTIC
// --------------------------------------------------------------------
void Solver2D_Mapper::SolveWithDiagnostics(SketchParams& sketch) {
    std::cout << "\n==================================================" << std::endl;
    std::cout << "         DIAGNOSTIC DU SOLVEUR 2D                 " << std::endl;
    std::cout << "==================================================" << std::endl;

    // --- 1. MAPPING ET LECTURE DES VARIABLES (IMPORT) ---
    std::vector<double*> variablePointers;
    std::unordered_map<void*, int> pointToXIndex;
    std::unordered_map<void*, int> circleToRIndex;

    std::cout << "\n--- 1. VARIABLES MAPPÉES (DANS VECTEUR X) ---" << std::endl;

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

                std::cout << " [Line ID " << concretePrim.id << "]\n"
                          << "   -> Start (X" << idxStart << ", Y" << idxStart+1 << ") = ("
                          << concretePrim.start.p2d.X() << ", " << concretePrim.start.p2d.Y() << ")\n"
                          << "   -> End   (X" << idxEnd << ", Y" << idxEnd+1 << ") = ("
                          << concretePrim.stop.p2d.X() << ", " << concretePrim.stop.p2d.Y() << ")\n";
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                int idxCenter = static_cast<int>(variablePointers.size());
                pointToXIndex[&concretePrim.center.p2d] = idxCenter;
                variablePointers.push_back(&(concretePrim.center.p2d.ChangeCoord().ChangeCoord(1)));
                variablePointers.push_back(&(concretePrim.center.p2d.ChangeCoord().ChangeCoord(2)));

                int idxR = static_cast<int>(variablePointers.size());
                circleToRIndex[&concretePrim] = idxR;
                variablePointers.push_back(&(concretePrim.radius));

                std::cout << " [Circle ID " << concretePrim.id << "]\n"
                          << "   -> Center (X" << idxCenter << ", Y" << idxCenter+1 << ") = ("
                          << concretePrim.center.p2d.X() << ", " << concretePrim.center.p2d.Y() << ")\n"
                          << "   -> Radius (R" << idxR << ") = " << concretePrim.radius << "\n";
            }
        }, prim);
    }

    int numVariables = static_cast<int>(variablePointers.size());
    std::cout << " Total variables (Cols de la Jacobienne) : " << numVariables << std::endl;

    // Construction du vecteur X actuel
    Eigen::VectorXd X(numVariables);
    for (int i = 0; i < numVariables; ++i) {
        X[i] = *(variablePointers[i]);
    }

    // --- 2. EVALUATION ET CALCUL DES ERREURS DES CONTRAINTES ---
    std::cout << "\n--- 2. EVALUATION DES CONTRAINTES ---" << std::endl;
    std::vector<std::unique_ptr<IConstraint2D>> constraints;

    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) {
            std::cout << " [Constraint ID " << c.id << "] -> IGNOREE (Pilotee/Driven)\n";
            continue;
        }

        std::string cTypeStr = "";
        switch (c.type) {
        case ConstraintType::Horizontal: {
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (prim && std::holds_alternative<SketchLine>(*prim)) {
                SketchLine& line = std::get<SketchLine>(*prim);

                // On récupère les indices des variables X et Y du Start ET du Stop
                int x1 = pointToXIndex[&line.start.p2d];
                int x2 = pointToXIndex[&line.stop.p2d];

                // Horizontal => Les Y doivent être égaux (Y_start == Y_stop)
                // Donc (Y_start - Y_stop) = 0
                constraints.push_back(std::make_unique<ConstraintHorizontal>(x1 + 1, x2 + 1));
            } else {
                std::cout << "ERREUR : Contrainte Horizontale demande une ligne !" << std::endl;
            }
            break;
        }
        case ConstraintType::Vertical: {
            cTypeStr = "Vertical";
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);

            // On vérifie que c'est bien une ligne
            if (prim && std::holds_alternative<SketchLine>(*prim)) {
                SketchLine& line = std::get<SketchLine>(*prim);

                // Vertical => Les X doivent être égaux (X_start == X_stop)
                // On récupère les index des coordonnées X (ceux qui sont à pointToXIndex)
                int x1 = pointToXIndex[&line.start.p2d];
                int x2 = pointToXIndex[&line.stop.p2d];

                constraints.push_back(std::make_unique<ConstraintVertical>(x1, x2));
            } else {
                std::cout << "ERROR : Vertical demande une référence sur une ligne !" << std::endl;
            }
            break;
        }
        case ConstraintType::Coincident: {
            cTypeStr = "Coincident (2 eq.)";
            gp_Pnt2d* p1 = getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2) {
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointToXIndex[p1], pointToXIndex[p2]));
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointToXIndex[p1] + 1, pointToXIndex[p2] + 1));
            }else{
                std::cout<<"ERROR static void SolveWithDiagnostics(SketchParams& sketch) : if (p1 && p2) ConstraintType::Coincident" << std::endl;
            }
            break;
        }
        case ConstraintType::Distance: {
            cTypeStr = "Distance (" + std::to_string(c.value) + ")";
            gp_Pnt2d* p1 = getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2) {
                constraints.push_back(std::make_unique<ConstraintDistancePointPoint>(
                    pointToXIndex[p1], pointToXIndex[p1] + 1,
                    pointToXIndex[p2], pointToXIndex[p2] + 1, c.value));
            }else{
                std::cout<<"ERROR static void SolveWithDiagnostics(SketchParams& sketch) : if (p1 && p2) ConstraintType::Distance" << std::endl;
            }
            break;
        }
        case ConstraintType::Radius: {
            cTypeStr = "Radius (" + std::to_string(c.value) + ")";
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (prim && std::holds_alternative<SketchCircle>(*prim)) {
                SketchCircle& circle = std::get<SketchCircle>(*prim);
                constraints.push_back(std::make_unique<ConstraintRadius>(circleToRIndex[&circle], c.value));
            }else{
                std::cout<<"ERROR static void SolveWithDiagnostics(SketchParams& sketch) : if (p1 && p2) ConstraintType::Radius" << std::endl;
            }
            break;
        }
        case ConstraintType::Perpendicular: {
            cTypeStr = "Perpendicular";

            // Récupération des deux lignes à partir des références de la contrainte
            SketchPrimitive* prim1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            SketchPrimitive* prim2 = sketch.GetPrimitiveMutable(c.ref2.primitiveId);

            if (prim1 && prim2 && std::holds_alternative<SketchLine>(*prim1) && std::holds_alternative<SketchLine>(*prim2)) {
                auto& line1 = std::get<SketchLine>(*prim1);
                auto& line2 = std::get<SketchLine>(*prim2);

                gp_Pnt2d* p1 = &line1.start.p2d;
                gp_Pnt2d* p2 = &line1.stop.p2d;
                gp_Pnt2d* p3 = &line2.start.p2d;
                gp_Pnt2d* p4 = &line2.stop.p2d;

                constraints.push_back(std::make_unique<ConstraintPerpendicular>(
                    pointToXIndex[p1], pointToXIndex[p1] + 1,
                    pointToXIndex[p2], pointToXIndex[p2] + 1,
                    pointToXIndex[p3], pointToXIndex[p3] + 1,
                    pointToXIndex[p4], pointToXIndex[p4] + 1
                    ));
            } else {
                std::cout << "ERROR static void SolveWithDiagnostics(SketchParams& sketch) : if (prim1 && prim2) ConstraintType::Perpendicular" << std::endl;
            }
            break;
        }

        case ConstraintType::Parallel: {
            cTypeStr = "Parallel";

            SketchPrimitive* prim1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            SketchPrimitive* prim2 = sketch.GetPrimitiveMutable(c.ref2.primitiveId);

            if (prim1 && prim2 && std::holds_alternative<SketchLine>(*prim1) && std::holds_alternative<SketchLine>(*prim2)) {
                auto& line1 = std::get<SketchLine>(*prim1);
                auto& line2 = std::get<SketchLine>(*prim2);

                gp_Pnt2d* p1 = &line1.start.p2d;
                gp_Pnt2d* p2 = &line1.stop.p2d;
                gp_Pnt2d* p3 = &line2.start.p2d;
                gp_Pnt2d* p4 = &line2.stop.p2d;

                constraints.push_back(std::make_unique<ConstraintParallel>(
                    pointToXIndex[p1], pointToXIndex[p1] + 1,
                    pointToXIndex[p2], pointToXIndex[p2] + 1,
                    pointToXIndex[p3], pointToXIndex[p3] + 1,
                    pointToXIndex[p4], pointToXIndex[p4] + 1
                    ));
            } else {
                std::cout << "ERROR static void SolveWithDiagnostics(SketchParams& sketch) : if (prim1 && prim2) ConstraintType::Parallel" << std::endl;
            }
            break;
        }
        default:
            cTypeStr = "Type " + std::to_string(static_cast<int>(c.type)) + " (Non géré)";
            std::cout<<"ERROR static void SolveWithDiagnostics(SketchParams& sketch) : default non gere" << std::endl;
            break;
        }

        std::cout << " [Constraint ID " << c.id << "] " << cTypeStr
                  << " | Ref1: " << formatRef(sketch, c.ref1)
                  << " | Ref2: " << formatRef(sketch, c.ref2) << "\n";
    }

    // Calcul du vecteur d'erreur de résidu C(X)
    int numEquations = 0;
    double totalSquaredError = 0.0;

    for (const auto& c : constraints) {
        double err = c->calcError(X);
        totalSquaredError += err * err;
        numEquations++;
    }

    std::cout << " Total équations générées (Lignes de la Jacobienne) : " << numEquations << std::endl;
    std::cout << " Erreur globale actuelle (Norme L2 ||C(X)||)     : " << std::sqrt(totalSquaredError) << std::endl;

    // --- 3. BILAN DU SYSTÈME (DOF) ---
    std::cout << "\n--- 3. BILAN DES DEGRÉS DE LIBERTÉ (DOF) ---" << std::endl;
    int dof = numVariables - numEquations;
    std::cout << " Degres de Liberte calcules (DOF = Vars - Eq) : " << dof;

    if (dof > 0) {
        std::cout << " -> Systemes sous-contraint\n";
    } else if (dof == 0) {
        std::cout << " -> Systeme isostatique (Totalement contraint)\n";
    } else {
        std::cout << " -> Systeme sur-contraint (Hyperstatique)\n";
    }
    std::cout << "==================================================\n" << std::endl;
}

// --------------------------------------------------------------------
// RESOLUTION DES CONTRAINTES (Inchangé)
// --------------------------------------------------------------------
bool Solver2D_Mapper::PrepareAndSolve(SketchParams& sketch) {
    std::vector<double*> variablePointers;
    std::unordered_map<void*, int> pointToXIndex;
    std::unordered_map<void*, int> circleToRIndex;

    LOG_DEBUG << "Solver2D_Mapper::PrepareAndSolve(SketchParams& sketch)" << std::endl;

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
            }
        }, prim);
    }

    int numVariables = static_cast<int>(variablePointers.size());
    if (numVariables == 0) return true;

    Eigen::VectorXd X(numVariables);
    for (int i = 0; i < numVariables; ++i) {
        X[i] = *(variablePointers[i]);
    }

    std::vector<std::unique_ptr<IConstraint2D>> constraints;

    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) continue;

        switch (c.type) {
        case ConstraintType::Horizontal: {
            // 1. On va chercher la primitive directement
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);

            // 2. On vérifie que c'est bien une ligne
            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                SketchLine& line = std::get<SketchLine>(*p1);

                // 3. On extrait les points manuellement, sans appeler getPointPointerFromRef
                gp_Pnt2d* ptStart = &line.start.p2d;
                gp_Pnt2d* ptStop  = &line.stop.p2d;

                // 4. On ajoute la contrainte au solveur
                constraints.push_back(std::make_unique<ConstraintHorizontal>(
                    pointToXIndex[ptStart] + 1, // +1 pour l'indice Y
                    pointToXIndex[ptStop] + 1   // +1 pour l'indice Y
                    ));
            } else {
                std::cout << "ERROR: Horizontal attend une ligne valide." << std::endl;
            }
            break;
        }
        case ConstraintType::Vertical: {
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);

            if (p1 && std::holds_alternative<SketchLine>(*p1)) {
                SketchLine& line = std::get<SketchLine>(*p1);

                // On récupère les pointeurs des deux extrémités
                gp_Pnt2d* ptStart = &line.start.p2d;
                gp_Pnt2d* ptStop  = &line.stop.p2d;

                // Vertical => Les X doivent être égaux (X_start == X_stop)
                // On utilise donc directement les indices des X (sans +1)
                constraints.push_back(std::make_unique<ConstraintVertical>(
                    pointToXIndex[ptStart],
                    pointToXIndex[ptStop]
                    ));
            } else {
                std::cout << "ERROR: Vertical attend une ligne valide." << std::endl;
            }
            break;
        }
        case ConstraintType::Coincident: {
            gp_Pnt2d* p1 = getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2) {
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointToXIndex[p1], pointToXIndex[p2]));
                constraints.push_back(std::make_unique<ConstraintCoincident1D>(pointToXIndex[p1] + 1, pointToXIndex[p2] + 1));
            }
            break;
        }
        case ConstraintType::Distance: {
            gp_Pnt2d* p1 = getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = getPointPointerFromRef(sketch, c.ref2);
            if (p1 && p2) {
                constraints.push_back(std::make_unique<ConstraintDistancePointPoint>(
                    pointToXIndex[p1], pointToXIndex[p1] + 1,
                    pointToXIndex[p2], pointToXIndex[p2] + 1, c.value));
            }
            break;
        }
        case ConstraintType::Radius: {
            SketchPrimitive* prim = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            if (prim && std::holds_alternative<SketchCircle>(*prim)) {
                SketchCircle& circle = std::get<SketchCircle>(*prim);
                constraints.push_back(std::make_unique<ConstraintRadius>(circleToRIndex[&circle], c.value));
            }
            break;
        }
        case ConstraintType::Perpendicular: {
            SketchPrimitive* p1 = sketch.GetPrimitiveMutable(c.ref1.primitiveId);
            SketchPrimitive* p2 = sketch.GetPrimitiveMutable(c.ref2.primitiveId);

            if (p1 && p2 && std::holds_alternative<SketchLine>(*p1) && std::holds_alternative<SketchLine>(*p2)) {
                auto& line1 = std::get<SketchLine>(*p1);
                auto& line2 = std::get<SketchLine>(*p2);

                gp_Pnt2d* p1_start = &line1.start.p2d;
                gp_Pnt2d* p1_stop  = &line1.stop.p2d;
                gp_Pnt2d* p2_start = &line2.start.p2d;
                gp_Pnt2d* p2_stop  = &line2.stop.p2d;

                constraints.push_back(std::make_unique<ConstraintPerpendicular>(
                    pointToXIndex[p1_start],     pointToXIndex[p1_start] + 1,
                    pointToXIndex[p1_stop],      pointToXIndex[p1_stop] + 1,
                    pointToXIndex[p2_start],     pointToXIndex[p2_start] + 1,
                    pointToXIndex[p2_stop],      pointToXIndex[p2_stop] + 1
                    ));
            }
            break;
        }
        default:
            LOG_ERROR << "\tERROR : static void SolveWithDiagnostics(SketchParams& sketch) "  << std::endl;
            break;
        }
    }

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

    for (int i = 0; i < numVariables; ++i) {
        *(variablePointers[i]) = X[i];
    }




    // --------------------------------------------------------------------
    // POST-TRAITEMENT : ÉGALISATION STRICTE DES COÏNCIDENCES 2D
    // --------------------------------------------------------------------
    // On parcourt les contraintes pour forcer les points coïncidents
    // à avoir STRICTEMENT les mêmes bits en mémoire double (IEEE-754).
    LOG_DEBUG << "\tPost fix des contraintes de coincidence"<<std::endl;
    for (const auto& c : sketch.getConstraints()) {
        if (c.isDriven) continue;

        if (c.type == ConstraintType::Coincident) {
            gp_Pnt2d* p1 = getPointPointerFromRef(sketch, c.ref1);
            gp_Pnt2d* p2 = getPointPointerFromRef(sketch, c.ref2);

            if (p1 && p2 && p1 != p2) {
                // Mesure de l'écart résiduel du solveur

#ifdef SOLVE_DBG_CORRECT_COINCIDENCE
                double residualDist = p1->Distance(*p2);
#endif
                // Moyennage et réaffectation
                double avgX = (p1->X() + p2->X()) * 0.5;
                double avgY = (p1->Y() + p2->Y()) * 0.5;

                p1->SetCoord(avgX, avgY);
                p2->SetCoord(avgX, avgY);
#ifdef SOLVE_DBG_CORRECT_COINCIDENCE
                // Affichage diagnostic (si un écart existait)
                if (residualDist > 0.0) {
                    LOG_INFO << "\t[Coincidence Post-Fix] Ecart de "
                              << residualDist * 1000.0 << " µm recousu à (X: "
                              << avgX << ", Y: " << avgY << ")" << std::endl;
                }
#endif
            }
        }
    }


    LOG_INFO << "\tFIN fonction Solver2D_Mapper::PrepareAndSolve " << std::endl;

    sketch.recomputeGeometry3D();

    return true;
}










