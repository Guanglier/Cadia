

#include "Contours.h"

//#include "CAD_Operation.h" // Conserve tes inclusions d'origine
#include <gp_Pln.hxx>
#include <deque>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "Logger.h"




//#define DBG_COUNTOURS_ACTIVE

#ifdef DBG_COUNTOURS_ACTIVE
#define DBG_COUNTOURS_TYPE	LOG_DEBUG
#endif


//─────────────────────────────────────────────────────────────────────
//
// 1 -  Créer les contours fermés, avoir une liste pour ça.
// 2 -  gérer la validation des contours, si ils sont ouverts ou se chevauchent alors nok
// 3 -  utliser les algos pour identifier le contour extérieur et les contours inclus
// 4 -  calculer le sens des contours, par la méthode de l'aire signée -> extérieur
//      sens trigo (anti horaire) et intérieur sens horaire
//
//
//          [ Calculer Bounding Boxes ]
//                      │
//                      ▼
//          [ Les boîtes s'englobent ? ] ──(Non)──> Disjoints (Fin)
//                      │ (Oui)
//                      ▼
//    [ Y a-t-il des intersections de segments ? ] ──(Oui)──> Erreur / Fusion requise (Chevauchement)
//                      │ (Non, garantie qu'ils ne se touchent pas)
//                      ▼
//         [ Tester un seul point B[0] dans A ]
//                      │
//            ┌──────────┴──────────┐
//        (Dans A)             (Hors de A)
//            │                     │
//            ▼                     ▼
//        B est un TROU         B est DISJOINT (ex: dans le creux d'un croissant)
//
//─────────────────────────────────────────────────────────────────────

//─────────────────────────────────────────────────────────────────────
//  calcule l'air Formule de l'arpenteur pour savoir dans quel sens
// et quelle taille.
// ATTENTION : il faut tous les segments dans le meme sens !
//─────────────────────────────────────────────────────────────────────
double ContoursEngine_CalculeAire(Contour& li_Contour) {
    // Si l'aire a déjà été forcée ou inversée par l'orientation d'un cercle, on la préserve
    if (li_Contour.elements.size() == 1 && li_Contour.elements[0].type == ContoursPrimitiveType::Circle) {
        if (std::abs(li_Contour.Aire) > 1e-9) {
            return li_Contour.Aire;
        }
        li_Contour.Aire = M_PI * li_Contour.elements[0].Radius * li_Contour.elements[0].Radius;
        return li_Contour.Aire;
    }

    double l_Aire = 0.0;
    for (const auto& element : li_Contour.elements) {
        const gp_Pnt2d& p1 = element.StartCpy2D;
        const gp_Pnt2d& p2 = element.StopCpy2D;
        l_Aire += (p2.X() - p1.X()) * (p2.Y() + p1.Y());  // Formule de l'arpenteur (Shoelace)
    }
    li_Contour.Aire = l_Aire / 2.0;
    return li_Contour.Aire;
}

//─────────────────────────────────────────────────────────────────────
// Renvoie l'orientation du triplet (P, Q, R)
// 0 -> Colinéaires, 1 -> Sens horaire, 2 -> Sens anti-horaire
//─────────────────────────────────────────────────────────────────────
int ContoursEngine_GetOrientation(const gp_Pnt2d& P, const gp_Pnt2d& Q, const gp_Pnt2d& R) {
    double val = (Q.Y() - P.Y()) * (R.X() - Q.X()) - (Q.X() - P.X()) * (R.Y() - Q.Y());
    if (std::abs(val) < 1e-9) return 0; // Seuil de tolérance numérique
    return (val > 0) ? 1 : 2;
}

//─────────────────────────────────────────────────────────────────────
// Vérifie si le point R est sur le segment PQ (sachant qu'ils sont colinéaires)
//─────────────────────────────────────────────────────────────────────
bool ContoursEngine_IsOnSegment(const gp_Pnt2d& P, const gp_Pnt2d& Q, const gp_Pnt2d& R) {
    return R.X() <= std::max(P.X(), Q.X()) && R.X() >= std::min(P.X(), Q.X()) &&
           R.Y() <= std::max(P.Y(), Q.Y()) && R.Y() >= std::min(P.Y(), Q.Y());
}

//─────────────────────────────────────────────────────────────────────
// Teste l'intersection entre le segment [A, B] et le segment [C, D]
//─────────────────────────────────────────────────────────────────────
bool ContoursEngine_SegmentsIntersect(const gp_Pnt2d& A, const gp_Pnt2d& B, const gp_Pnt2d& C, const gp_Pnt2d& D) {
    int o1 = ContoursEngine_GetOrientation(A, B, C);
    int o2 = ContoursEngine_GetOrientation(A, B, D);
    int o3 = ContoursEngine_GetOrientation(C, D, A);
    int o4 = ContoursEngine_GetOrientation(C, D, B);

    if (o1 != o2 && o3 != o4) return true;

    if (o1 == 0 && ContoursEngine_IsOnSegment(A, B, C)) return true;
    if (o2 == 0 && ContoursEngine_IsOnSegment(A, B, D)) return true;
    if (o3 == 0 && ContoursEngine_IsOnSegment(C, D, A)) return true;
    if (o4 == 0 && ContoursEngine_IsOnSegment(C, D, B)) return true;

    return false;
}

//─────────────────────────────────────────────────────────────────────
// Moteur d'intersection unifié (Narrow-Phase Linéaire vs Circulaire)
//─────────────────────────────────────────────────────────────────────
bool ContoursEngine_PrimitivesIntersect(const ContoursElement& el1, const ContoursElement& el2) {
    // CAS 1 : Ligne vs Ligne
    if (el1.type == ContoursPrimitiveType::Line && el2.type == ContoursPrimitiveType::Line) {
        return ContoursEngine_SegmentsIntersect(el1.StartCpy2D, el1.StopCpy2D, el2.StartCpy2D, el2.StopCpy2D);
    }

    // CAS 2 : Cercle vs Cercle
    if (el1.type == ContoursPrimitiveType::Circle && el2.type == ContoursPrimitiveType::Circle) {
        double dist = el1.CenterCpy2D.Distance(el2.CenterCpy2D);
        double rSum = el1.Radius + el2.Radius;
        double rDiff = std::abs(el1.Radius - el2.Radius);
        return (dist < rSum && dist > rDiff);
    }

    // CAS 3 : Ligne vs Cercle
    const ContoursElement& line = (el1.type == ContoursPrimitiveType::Line) ? el1 : el2;
    const ContoursElement& circle = (el1.type == ContoursPrimitiveType::Circle) ? el1 : el2;

    gp_Vec2d ab(line.StartCpy2D, line.StopCpy2D);
    gp_Vec2d ac(line.StartCpy2D, circle.CenterCpy2D);
    double abLen2 = ab.SquareMagnitude();
    if (abLen2 < 1e-9) return line.StartCpy2D.Distance(circle.CenterCpy2D) < circle.Radius;

    double t = ac.Dot(ab) / abLen2;
    t = std::max(0.0, std::min(1.0, t)); // Clamping sur le segment
    //gp_Pnt2d projection = line.StartCpy2D.XYZ() + ab.XYZ() * t;
    gp_Pnt2d projection = line.StartCpy2D.Translated(ab * t);

    double distToSegment = projection.Distance(circle.CenterCpy2D);

    return (distToSegment <= circle.Radius &&
            (line.StartCpy2D.Distance(circle.CenterCpy2D) > circle.Radius ||
             line.StopCpy2D.Distance(circle.CenterCpy2D) > circle.Radius));
}

enum class SensRotation {
    Horaire,
    AntiHoraire
};

//─────────────────────────────────────────────────────────────────────
// Inverse l'ordre et les flèches des segments de manière brute
//─────────────────────────────────────────────────────────────────────
void ContoursEngine_InverserChaine(Contour& contour) {
    if (contour.elements.size() == 1 && contour.elements[0].type == ContoursPrimitiveType::Circle) {
        contour.Aire = -contour.Aire; // L'inversion d'un cercle complet est purement topologique (son signe change)
        return;
    }

    std::reverse(contour.elements.begin(), contour.elements.end());
    for (auto& element : contour.elements) {
        std::swap(element.StartCpy2D, element.StopCpy2D);
    }
}

//─────────────────────────────────────────────────────────────────────
// Fonction principale : Vérifie et force le sens de manière brute
//─────────────────────────────────────────────────────────────────────
void ContoursEngine_OrienteContour(Contour& contour, SensRotation sensDesire) {
    double aireActuelle = ContoursEngine_CalculeAire(contour);

    if (std::abs(aireActuelle) < 1e-9) return;

    bool estActuellementAntiHoraire = (aireActuelle > 0.0);
    bool veutAntiHoraire = (sensDesire == SensRotation::AntiHoraire);

    if (estActuellementAntiHoraire != veutAntiHoraire) {
        ContoursEngine_InverserChaine(contour);
        contour.Aire = ContoursEngine_CalculeAire(contour);
    }
}

//─────────────────────────────────────────────────────────────────────
// Fonction pour mettre à jour la bounding box d'un contour
//─────────────────────────────────────────────────────────────────────
void ContoursEngine_ComputeBoundingBox(Contour& contour) {
    contour.BoundingBox2D.SetVoid();

    for (const auto& element : contour.elements) {
        if (element.type == ContoursPrimitiveType::Circle) {
            contour.BoundingBox2D.Add(gp_Pnt2d(element.CenterCpy2D.X() - element.Radius, element.CenterCpy2D.Y() - element.Radius));
            contour.BoundingBox2D.Add(gp_Pnt2d(element.CenterCpy2D.X() + element.Radius, element.CenterCpy2D.Y() + element.Radius));
        } else {
            contour.BoundingBox2D.Add(element.StartCpy2D);
            contour.BoundingBox2D.Add(element.StopCpy2D);
        }
    }
}

//─────────────────────────────────────────────────────────────────────
// Parcours le segment et rends uniforme les sens
//─────────────────────────────────────────────────────────────────────
void ContoursEngine_UniformiseSensSegments(Contour& li_Contour) {
    if (li_Contour.elements.size() < 2) {
        return;
    }

    const double tolerance = 1e-6;

    for (size_t i = 1; i < li_Contour.elements.size(); ++i) {
        const gp_Pnt2d& referencePoint = li_Contour.elements[i - 1].StopCpy2D;
        ContoursElement& currentElement = li_Contour.elements[i];

        if (currentElement.StartCpy2D.IsEqual(referencePoint, tolerance)) {
            continue;
        }
        else if (currentElement.StopCpy2D.IsEqual(referencePoint, tolerance)) {
            std::swap(currentElement.StartCpy2D, currentElement.StopCpy2D);
        }
        else {
            li_Contour.hasError = true;
            li_Contour.diagnosticMessage = "Discontinuité / Gap détecté avant l'élément d'ID " + std::to_string(currentElement.primitiveId);
#ifdef DBG_COUNTOURS_ACTIVE
            DBG_COUNTOURS_TYPE << "SketchParams::Contours_UniformiseSensSegments : ERREUR sur le segment " << currentElement.primitiveId << std::endl;
#endif
        }
    }

    if (li_Contour.isClosed && li_Contour.elements.size() >= 2) {
        const gp_Pnt2d& finalPoint = li_Contour.elements.back().StopCpy2D;
        const gp_Pnt2d& firstPoint = li_Contour.elements.front().StartCpy2D;

        if (!finalPoint.IsEqual(firstPoint, tolerance)) {
            li_Contour.elements.back().StopCpy2D = firstPoint; // Fermeture forcée pour micro-trous numériques
        }
    }
}

//─────────────────────────────────────────────────────────────────────
//  Extrait les primitives, crée les contours
//─────────────────────────────────────────────────────────────────────
//#define SKETCH_COUNTOUR_CREE_DBG
std::vector<Contour> ContoursEngine::Contours_IdentifieContours(const std::vector<ContoursElement>& inputPrimitives) {
    Contour l_ContoursDebut;
    bool ThereCouldBeMoreContours = true;
    std::vector<Contour> computedContoursList; // Liste locale au lieu de m_ContoursList

    //--- 1. Remplissage et isolation immédiate des cercles et lignes ---
    for (const auto& primitive : inputPrimitives) {
        if (primitive.type == ContoursPrimitiveType::Line) {
            l_ContoursDebut.elements.push_back(primitive);
        }
        else if (primitive.type == ContoursPrimitiveType::Circle) {
            Contour cercleContour;
            cercleContour.isClosed = true;
            cercleContour.isInternal = false;
            cercleContour.diagnosticMessage = "OK";
            cercleContour.elements.push_back(primitive);

            ContoursEngine_ComputeBoundingBox(cercleContour);
            ContoursEngine_CalculeAire(cercleContour);

            computedContoursList.emplace_back(std::move(cercleContour));
        }
    }

    //----- 2. Extraction des chaînes linéaires -------------------------
    do {
        if (l_ContoursDebut.elements.empty()) {
            break;
        }

        std::deque<ContoursElement> currentChain;
        currentChain.push_back(l_ContoursDebut.elements.back());
        l_ContoursDebut.elements.pop_back();

        gp_Pnt2d chainStart2D = currentChain.front().StartCpy2D;
        gp_Pnt2d chainEnd2D   = currentChain.back().StopCpy2D;
        bool elementAdded = true;

        while (elementAdded) {
            elementAdded = false;

            for (size_t i = 0; i < l_ContoursDebut.elements.size(); ++i) {
                auto candidate = l_ContoursDebut.elements[i];

                if (chainEnd2D.IsEqual(candidate.StartCpy2D, 1E-4)) {
                    currentChain.push_back(candidate);
                    chainEnd2D = candidate.StopCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
                else if (chainEnd2D.IsEqual(candidate.StopCpy2D, 1E-4)) {
                    std::swap(candidate.StartCpy2D, candidate.StopCpy2D);
                    currentChain.push_back(candidate);
                    chainEnd2D = candidate.StopCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
                else if (chainStart2D.IsEqual(candidate.StopCpy2D, 1E-4)) {
                    currentChain.push_front(candidate);
                    chainStart2D = candidate.StartCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
                else if (chainStart2D.IsEqual(candidate.StartCpy2D, 1E-4)) {
                    std::swap(candidate.StartCpy2D, candidate.StopCpy2D);
                    currentChain.push_front(candidate);
                    chainStart2D = candidate.StartCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
            }
        }

        // --- 3. Validation, Ordonnancement et Sauvegarde ---
        if (!currentChain.empty()) {
            Contour l_CurrentContour;
            l_CurrentContour.diagnosticMessage = "OK";

            for (const auto& el : currentChain) {
                l_CurrentContour.elements.push_back(el);
            }

            l_CurrentContour.isClosed = chainStart2D.IsEqual(chainEnd2D, 1E-4);
            if (!l_CurrentContour.isClosed) {
                l_CurrentContour.hasError = true;
                l_CurrentContour.diagnosticMessage = "Contour ouvert (Non connecté de bout en bout)";
            }

            ContoursEngine_UniformiseSensSegments(l_CurrentContour);
            ContoursEngine_CalculeAire(l_CurrentContour);
            ContoursEngine_ComputeBoundingBox(l_CurrentContour);

            computedContoursList.emplace_back(std::move(l_CurrentContour));
        }

        if (l_ContoursDebut.elements.empty()) {
            ThereCouldBeMoreContours = false;
        }

    } while (ThereCouldBeMoreContours);

    return computedContoursList;
}




//─────────────────────────────────────────────────────────────────────
//      Affichage des contours.
//─────────────────────────────────────────────────────────────────────
void ContoursEngine::Contours_DisplayContours(const ContoursTopologyResult &ContourResultInput) {
#ifdef DBG_COUNTOURS_ACTIVE
    DBG_COUNTOURS_TYPE << "SketchParams::Contours_DisplayContours" << std::endl;
#endif
    for (auto& current : ContourResultInput.validContours) {
        Contours_DisplayContour(current);
    }
}
void ContoursEngine::Contours_DisplayContour(const Contour& current) {
#ifdef DBG_COUNTOURS_ACTIVE
    DBG_COUNTOURS_TYPE << "SketchParams::Contours_DisplayContour" << std::endl;
    DBG_COUNTOURS_TYPE << "\t\tAire : " << current.Aire << std::endl;
    DBG_COUNTOURS_TYPE << "\t\tisClosed : " << (current.isClosed ? "yes" : "no") << std::endl;
    DBG_COUNTOURS_TYPE << "\t\tisInternal : " << (current.isInternal ? "yes" : "no") << std::endl;
    DBG_COUNTOURS_TYPE << "\t\thasError : " << (current.hasError ? "YES" : "no") << std::endl;
    DBG_COUNTOURS_TYPE << "\t\tDiagnostic : " << current.diagnosticMessage << std::endl;
#endif

    // Extraction compatible const pour la Bnd_Box2d d'OpenCASCADE
    double xmin = 0.0, ymin = 0.0, xmax = 0.0, ymax = 0.0;
    if (!current.BoundingBox2D.IsVoid()) {
        current.BoundingBox2D.Get(xmin, ymin, xmax, ymax);
    }
#ifdef DBG_COUNTOURS_ACTIVE
    DBG_COUNTOURS_TYPE << "\t\tisBounding : (" << xmin << "," << ymin << ")->(" << xmax << "," << ymax << ")" << std::endl;
    DBG_COUNTOURS_TYPE << "\t\tPrimitives : ";
#endif

    for (size_t j = 0; j < current.elements.size(); j++) {
        const auto& el = current.elements[j];
        switch (el.type) {
        case ContoursPrimitiveType::Line:
#ifdef DBG_COUNTOURS_ACTIVE
            DBG_COUNTOURS_TYPE << "Line [ID: " << el.primitiveId << "] (" << el.StartCpy2D.X() << "," << el.StartCpy2D.Y() << ") to (" << el.StopCpy2D.X() << "," << el.StopCpy2D.Y() << ") - ";
#endif
            break;
        case ContoursPrimitiveType::Circle:
#ifdef DBG_COUNTOURS_ACTIVE
            DBG_COUNTOURS_TYPE << "Circle [ID: " << el.primitiveId << "] Center(" << el.CenterCpy2D.X() << "," << el.CenterCpy2D.Y() << ") Radius " << el.Radius;
#endif
                break;
        default:
#ifdef DBG_COUNTOURS_ACTIVE
            DBG_COUNTOURS_TYPE << " ERROR DEFAULT 857" << std::endl;
#endif
            break;
        }

        if ((j > 0) && (0 == (j % 5))) {
#ifdef DBG_COUNTOURS_ACTIVE
            DBG_COUNTOURS_TYPE << std::endl << "\t\t";
#endif
        }
    }
#ifdef DBG_COUNTOURS_ACTIVE
    DBG_COUNTOURS_TYPE << std::endl;
#endif
}






ContoursTopologyResult ContoursEngine::Process(const std::vector<ContoursElement>& inputPrimitives) {
    ContoursTopologyResult l_Result;

    // 1. Identification et chaînage des contours à partir des primitives d'entrée
    std::vector<Contour> localContours = Contours_IdentifieContours(inputPrimitives);

    if (localContours.empty()) {
        l_Result.hasErrors = true;
        l_Result.errorMessage = "Aucun contour géométrique détecté.";
        return l_Result;
    }

    // 2. Tri décroissant basé sur la valeur absolue de la surface
    std::sort(localContours.begin(), localContours.end(),
              [](const Contour& a, const Contour& b) {
                  return std::abs(a.Aire) > std::abs(b.Aire);
              });

    // Le maître-hôte externe (le plus grand en surface)
    Contour& outerContour = localContours.front();
    outerContour.isInternal = false;

    // 3. Vérifier et qualifier les autres contours (les trous)
    // 3. Vérifier et qualifier les autres contours (les trous)
    for (size_t i = 1; i < localContours.size(); ++i) {
        Contour& current = localContours[i];

        // Étape A : Broad-phase (Bounding Box) avec le contour externe
        if (outerContour.BoundingBox2D.IsOut(current.BoundingBox2D)) {
            current.isInternal = false;
            current.hasError = true;
            current.diagnosticMessage = "Contour orphelin situé hors des limites du profil principal";
            l_Result.hasWarnings = true;
            l_Result.warningMessage = "Un ou plusieurs contours sont en dehors du profil principal.";
            continue;
        }

        // Étape B : Narrow-phase unifiée (Collisions/Intersections avec le bord externe)
        bool primitivesCrossOuter = false;
        for (const auto& edgeOuter : outerContour.elements) {
            for (const auto& edgeInner : current.elements) {
                if (ContoursEngine_PrimitivesIntersect(edgeOuter, edgeInner)) {
                    primitivesCrossOuter = true;
                    break;
                }
            }
            if (primitivesCrossOuter) break;
        }

        if (primitivesCrossOuter) {
            current.hasError = true;
            current.diagnosticMessage = "Auto-intersection / Collision détectée avec le bord externe";
            l_Result.hasErrors = true;
            l_Result.errorMessage = "Intersection détectée entre le contour externe et un contour interne.";
            continue;
        }

        // === ÉTAPE B2 : NOUVEAU - Vérification des intersections entre contours internes (Trous) ===
        bool primitivesCrossHoles = false;
        std::string intersectWithContourName = "";

        for (size_t j = 1; j < i; ++j) {
            const Contour& otherHole = localContours[j];

            // Si l'autre trou est déjà marqué en erreur, inutile de se comparer à lui
            if (otherHole.hasError) continue;

            // Broad-phase rapide : si leurs Bounding Boxes ne se touchent pas, ils ne s'intersectent pas !
            if (current.BoundingBox2D.IsOut(otherHole.BoundingBox2D)) {
                continue;
            }

            // Narrow-phase : On check si une de nos primitives croise une primitive de l'autre trou
            for (const auto& edgeCurrent : current.elements) {
                for (const auto& edgeOther : otherHole.elements) {
                    if (ContoursEngine_PrimitivesIntersect(edgeCurrent, edgeOther)) {
                        primitivesCrossHoles = true;
                        break;
                    }
                }
                if (primitivesCrossHoles) break;
            }

            if (primitivesCrossHoles) {
                break; // On a trouvé une collision, on arrête de chercher pour ce contour
            }
        }

        if (primitivesCrossHoles) {
            current.hasError = true;
            current.diagnosticMessage = "Collision / Chevauchement detecte(e) avec un autre contour interne";
            l_Result.hasWarnings = true;
            // On NE met PAS l_Result.hasErrors à true ici pour ne pas bloquer l'extrusion globale.
            // On se contente de marquer ce contour précis en erreur.
            continue;
        }

        // =========================================================================================
        // Étape C : Tout est vert, c'est une poche intérieure valide
        current.isInternal = true;
    }

    // 4. Forçage topologique de l'orientation
    for (auto& contour : localContours) {
        if (!contour.isInternal) {
            ContoursEngine_OrienteContour(contour, SensRotation::AntiHoraire);
        } else {
            ContoursEngine_OrienteContour(contour, SensRotation::Horaire);
        }

        // Si un sous-contour individuel est marqué en erreur (ex: ouvert ou intersectant)
        if (contour.hasError) {
            // Au lieu de lever l_Result.hasErrors, on lève un flag de Warning global
            // On s'assure que l_Result.hasErrors RESTE à false pour ne pas bloquer l'extrusion !
            l_Result.hasWarnings = true;
            if (l_Result.warningMessage.empty()) {
                l_Result.warningMessage = "Certains contours invalides ont été ignorés pour générer le solide.";
            }
        }
    }

    // 5. Transfert des résultats dans le bon vecteur de la structure
    l_Result.validContours = std::move(localContours);

    return l_Result;
}










