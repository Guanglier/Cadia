
#include "CAD_Operation.h"
#include <gp_Pln.hxx>
#include <deque>

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
//           ┌──────────┴──────────┐
//        (Dans A)             (Hors de A)
//           │                     │
//           ▼                     ▼
//     B est un TROU         B est DISJOINT (ex: dans le creux d'un croissant)
//
//
//─────────────────────────────────────────────────────────────────────



#ifdef PROUT_SUPP

//─────────────────────────────────────────────────────────────────────
//  calcule l'air Formule de l'arpenteur pour savoir dans quel sens
// et quelle taille.
// ATTENTION : il faut tous les segments dans le meme sens !
//─────────────────────────────────────────────────────────────────────
double Contours_CalculeAire ( SketchContours& li_Contour ){
    double l_Aire = 0.0;

    //std::cout << "SketchParams::Contours_CalculeAire" << std::endl;
    if ( (li_Contour.elements.size() == 1) && (li_Contour.elements[0].type == PrimitiveType::Circle ) ){
        li_Contour.Aire = M_PI * li_Contour.elements[0].Radius * li_Contour.elements[0].Radius;
        return li_Contour.Aire;
    }

    //size_t n = li_Contour.elements.size();
    for (const auto& element : li_Contour.elements) {
        const gp_Pnt2d& p1 = element.StartCpy2D;
        const gp_Pnt2d& p2 = element.StopCpy2D; // Retourne au premier point à la fin
        l_Aire += (p2.X() - p1.X()) * (p2.Y() + p1.Y());  // Formule de l'arpenteur (Shoelace)
    }
    li_Contour.Aire = l_Aire / 2.0;
    //std::cout << "\t-> aire = " << li_Contour.Aire << std::endl;
    return li_Contour.Aire;
}



//─────────────────────────────────────────────────────────────────────
// Renvoie l'orientation du triplet (P, Q, R)
// 0 -> Colinéaires, 1 -> Sens horaire, 2 -> Sens anti-horaire
//─────────────────────────────────────────────────────────────────────
int GetOrientation(const gp_Pnt2d& P, const gp_Pnt2d& Q, const gp_Pnt2d& R) {
    double val = (Q.Y() - P.Y()) * (R.X() - Q.X()) - (Q.X() - P.X()) * (R.Y() - Q.Y());
    if (std::abs(val) < 1e-9) return 0; // Seuil de tolérance numérique
    return (val > 0) ? 1 : 2;
}

//─────────────────────────────────────────────────────────────────────
// Vérifie si le point R est sur le segment PQ (sachant qu'ils sont colinéaires)
//─────────────────────────────────────────────────────────────────────
bool IsOnSegment(const gp_Pnt2d& P, const gp_Pnt2d& Q, const gp_Pnt2d& R) {
    return R.X() <= std::max(P.X(), Q.X()) && R.X() >= std::min(P.X(), Q.X()) &&
           R.Y() <= std::max(P.Y(), Q.Y()) && R.Y() >= std::min(P.Y(), Q.Y());
}


//─────────────────────────────────────────────────────────────────────
// Teste l'intersection entre le segment [A, B] et le segment [C, D]
//
//	1 - Le concept clé : L'orientation de 3 points
//		Imaginez que vous marchez du point $P$ vers le point $Q$, puis que vous tournez vers le point $R$.
//		Trois situations sont possibles :
//		 - Vous continuez tout droit : les points sont colinéaires (orientation = 0).
//		 - Vous tournez à droite : sens horaire (orientation = 1).
//		 - Vous tournez à gauche : sens anti-horaire (orientation = 2).
//
//		Pour calculer cela mathématiquement en 2D, on utilise le produit vectoriel (le déterminant)
//		des vecteurs $\vec{PQ}$ et $\vec{QR}$.
//
//		$$\text{val} = (y_2 - y_1)(x_3 - x_2) - (x_2 - x_1)(y_3 - y_2)$$
//		 - Si $\text{val} == 0 \implies$ Colinéaires.
//		 - Si $\text{val} > 0 \implies$ Sens horaire.
//		 - Si $\text{val} < 0 \implies$ Sens anti-horaire.
//
//
//	2 - La règle d'intersection (Le cas général
//		Deux segments $[A, B]$ et $[C, D]$ se croisent si et seulement si les deux conditions suivantes
//		sont vraies en même temps :
//		 - $C$ et $D$ sont de part d'autre de la droite $(AB)$
//		      Autrement dit, le triplet $(A, B, C)$ et le triplet $(A, B, D)$
//			  doivent avoir des orientations différentes.
//		 - $A$ et $B$ sont de part d'autre de la droite $(CD)$
//		      Le triplet $(C, D, A)$ et le triplet $(C, D, B)$ doivent
//			  avoir des orientations différentes.
//	3. Les cas particuliers (Points alignés / colinéaires)
// 		En CAO, il arrive souvent que les segments se touchent juste par un sommet,
// 		ou soient couchés l'un sur l'autre.
// 		Si on se contente du cas général, on passe à côté.
//
// 		Si trois points sont colinéaires (orientation = 0),
// 		il faut vérifier si le troisième point est physiquement "écrasé"
// 		sur le segment formé par les deux autres.
// 		C'est le rôle de la fonction IsOnSegment(P, Q, R).
//─────────────────────────────────────────────────────────────────────
bool SegmentsIntersect(const gp_Pnt2d& A, const gp_Pnt2d& B, const gp_Pnt2d& C, const gp_Pnt2d& D) {
    // 1. On calcule les 4 orientations nécessaires
    int o1 = GetOrientation(A, B, C); // Position de C par rapport à [A,B]
    int o2 = GetOrientation(A, B, D); // Position de D par rapport à [A,B]
    int o3 = GetOrientation(C, D, A); // Position de A par rapport à [C,D]
    int o4 = GetOrientation(C, D, B); // Position de B par rapport à [C,D]

    // 2. CAS GÉNÉRAL
    // Si C et D sont opposés par rapport à AB (o1 != o2)
    // ET que A et B sont opposés par rapport à CD (o3 != o4), alors ça se croise forcément !
    if (o1 != o2 && o3 != o4) {
        return true;
    }

    // 3. CAS PARTICULIERS (Colinbackground)
    // Cas 1: A, B et C sont alignés, et C est sur le segment [A, B]
    if (o1 == 0 && IsOnSegment(A, B, C)) return true;

    // Cas 2: A, B et D sont alignés, et D est sur le segment [A, B]
    if (o2 == 0 && IsOnSegment(A, B, D)) return true;

    // Cas 3: C, D et A sont alignés, et A est sur le segment [C, D]
    if (o3 == 0 && IsOnSegment(C, D, A)) return true;

    // Cas 4: C, D et B sont alignés, et B est sur le segment [C, D]
    if (o4 == 0 && IsOnSegment(C, D, B)) return true;

    // Sinon, aucune intersection
    return false;
}




enum class SensRotation {
    Horaire,
    AntiHoraire
};


//─────────────────────────────────────────────────────────────────────
// Inverse l'ordre et les flèches des segments de manière brute
//─────────────────────────────────────────────────────────────────────
void InverserChaine(SketchContours& contour) {
    // 1. On retourne le vecteur pour inverser l'ordre des segments
    std::reverse(contour.elements.begin(), contour.elements.end());

    // 2. On retourne chaque segment sur lui-même (Inversion Départ <-> Fin)
    for (auto& element : contour.elements) {
        std::swap(element.StartCpy2D, element.StopCpy2D);
    }
}


//─────────────────────────────────────────────────────────────────────
// Fonction principale : Vérifie et force le sens de manière brute
//─────────────────────────────────────────────────────────────────────
void OrienteContour(SketchContours& contour, SensRotation sensDesire) {
    // 1. On RECALCULE l'aire ici, point par point, pour être 100% sûr de l'état actuel
    double aireActuelle = Contours_CalculeAire(contour);

    // Si le contour est plat (les points sont alignés), on ne peut pas l'orienter
    if (std::abs(aireActuelle) < 1e-9) return;

    bool estActuellementAntiHoraire = (aireActuelle > 0.0);
    bool veutAntiHoraire = (sensDesire == SensRotation::AntiHoraire);

    // 2. Si le sens géométrique mesuré ne colle pas avec ce qu'on veut, on inverse tout
    if (estActuellementAntiHoraire != veutAntiHoraire) {
        InverserChaine(contour);
        // 3. On met à jour la structure pour que le reste de ton soft soit au courant
        //contour.Aire = -contour.Aire;
        contour.Aire = Contours_CalculeAire(contour);
    }
}


//─────────────────────────────────────────────────────────────────────
// Fonction pour mettre à jour la bounding box d'un contour
//─────────────────────────────────────────────────────────────────────
void Contours_ComputeBoundingBox(SketchContours& contour) {
    contour.BoundingBox2D.SetVoid(); // On s'assure qu'elle est marquée comme vide au départ

    // 2. Parcourir tous les éléments (primitives) du contour
    for (const auto& element : contour.elements) {
        contour.BoundingBox2D.Add(element.StartCpy2D);
        contour.BoundingBox2D.Add(element.StopCpy2D);
    }
}

//─────────────────────────────────────────────────────────────────────
//  Parcours le segment et rends uniforme les sens, cad si un segment
//  va dans le mauvais sens il le remet dans le "bon" sens.
//  le "bon" sens est celui du premier segment.
//─────────────────────────────────────────────────────────────────────
#define SKETCH_COUNTOUR_UNIFORMISE
void Contours_UniformiseSensSegments ( SketchContours& li_Contour){
    // Si le contour est vide ou ne possède qu'un seul segment, rien à uniformiser
    if (li_Contour.elements.size() < 2) {
        return;
    }

    // Tolérance pour la comparaison géométrique des points (1 micromètre)
    const double tolerance = 1e-6;

    for (size_t i = 1; i < li_Contour.elements.size(); ++i) {
        // Le point où le segment précédent s'est arrêté (notre cible)
        const gp_Pnt2d& referencePoint = li_Contour.elements[i - 1].StopCpy2D;

        SketchContoursElement& currentElement = li_Contour.elements[i];

        // 1. Cas idéal : le départ du segment actuel colle avec la fin du précédent
        if (currentElement.StartCpy2D.IsEqual(referencePoint, tolerance)) {
            // Le segment est déjà dans le bon sens, rien à faire
            continue;
        }
        // 2. Cas inversé : la fin du segment actuel colle avec la fin du précédent
        else if (currentElement.StopCpy2D.IsEqual(referencePoint, tolerance)) {
            // Le segment est à l'envers, on l'inverse !
            std::swap(currentElement.StartCpy2D, currentElement.StopCpy2D);
        }
        // 3. Cas de discontinuité géométrique
        else {
            // Optionnel : Vous pouvez lever une alerte ou logger une erreur ici.
            // Cela signifie que le vector de primitives n'est pas trié dans le bon ordre de chaînage,
            // ou qu'il y a un espace vide (gap) entre les deux primitives.
#ifdef SKETCH_COUNTOUR_UNIFORMISE
            std::cout << "SketchParams::Contours_UniformiseSensSegments : ERREUR sur le segment" << std::endl;
#endif
        }
    }

    // Si le contour est marqué comme fermé, on peut faire une vérification ultime :
    // Est-ce que la fin du tout dernier élément boucle bien avec le début du tout premier ?
    if (li_Contour.isClosed && li_Contour.elements.size() >= 3) {
        const gp_Pnt2d& finalPoint = li_Contour.elements.back().StopCpy2D;
        const gp_Pnt2d& firstPoint = li_Contour.elements.front().StartCpy2D;

        if (!finalPoint.IsEqual(firstPoint, tolerance)) {
            // Le contour est censé être fermé mais ne boucle pas géométriquement.
            // On peut forcer la fermeture parfaite pour éviter les micro-trous numériques :
            li_Contour.elements.back().StopCpy2D = firstPoint;
        }
    }
}


//─────────────────────────────────────────────────────────────────────
//  Extrait les primitives, crée les contours
//─────────────────────────────────────────────────────────────────────
//#define SKETCH_COUNTOUR_CREE_DBG
void SketchParams::Contours_IdentifieContours()  {

    SketchContours  l_ContoursDebut;
    bool            ThereCouldBeMoreContours = true;
    int             l_IntContourId = 0;

#ifdef SKETCH_COUNTOUR_CREE_DBG
    std::cout << "SketchParams::Contours_Cree" << std::endl;
    std::cout << "\tRemplissage temp: " ;
#endif

    m_ContoursList.clear();

    //--- 1. Remplissage de la liste de début (Inchangé) -------------
    for (const auto& primitive : getPrimitives()) {
        std::visit([&l_ContoursDebut, this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, SketchLine>) {
                SketchContoursElement elmt;
                elmt.primitiveId = arg.id;
                elmt.StartCpy2D = arg.start.p2d;
                elmt.StopCpy2D = arg.stop.p2d;
                elmt.type = PrimitiveType::Line;
                l_ContoursDebut.elements.push_back(elmt);
#ifdef SKETCH_COUNTOUR_CREE_DBG
                std::cout << "L";
#endif
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
#ifdef SKETCH_COUNTOUR_CREE_DBG
                std::cout << "C";
#endif
                SketchContoursElement elmt;
                elmt.primitiveId = arg.id;
                elmt.type = PrimitiveType::Circle;
                elmt.CenterCpy2D = arg.center.p2d;
                elmt.Radius = arg.radius;

                // Pour le cercle, pas besoin de Start/Stop pour le chaînage,
                // mais on peut y mettre le Center par précaution ou laisser vide.

                // On génère son contour autonome TOUT DE SUITE
                SketchContours cercleContour;
                cercleContour.id        = arg.id;
                cercleContour.isClosed  = true;
                cercleContour.isInternal = false; // Sera qualifié par la suite
                cercleContour.elements.push_back(elmt);

                // On pré-calcule ses données à partir du buffer ultra-rapide
                cercleContour.Aire = M_PI * elmt.Radius * elmt.Radius;

                cercleContour.BoundingBox2D.SetVoid();
                cercleContour.BoundingBox2D.Add(gp_Pnt2d(elmt.CenterCpy2D.X() - elmt.Radius, elmt.CenterCpy2D.Y() - elmt.Radius));
                cercleContour.BoundingBox2D.Add(gp_Pnt2d(elmt.CenterCpy2D.X() + elmt.Radius, elmt.CenterCpy2D.Y() + elmt.Radius));

                // On l'ajoute directement à la liste finale globale des contours !
                m_ContoursList.emplace_back(std::move(cercleContour));

            }
            else if constexpr (std::is_same_v<T, SketchArc>) {
                // ...
            }
        }, primitive);
    }



#ifdef SKETCH_COUNTOUR_CREE_DBG
    std::cout << std::endl;
#endif

    //----- 2. Extraction des contours (Version Validée & Corrigée) -------------------------
    do {
        if (l_ContoursDebut.elements.empty()) {
            break;
        }

#ifdef SKETCH_COUNTOUR_CREE_DBG
        std::cout << "\tContour " << l_IntContourId << std::endl;
#endif

        // Utilisation d'un deque pour collecter bilatéralement les segments non ordonnés
        std::deque<SketchContoursElement> currentChain;

        // On prend le premier élément restant pour démarrer la chaîne
        currentChain.push_back(l_ContoursDebut.elements.back());
        l_ContoursDebut.elements.pop_back();

        gp_Pnt2d chainStart2D = currentChain.front().StartCpy2D;
        gp_Pnt2d chainEnd2D   = currentChain.back().StopCpy2D;

        bool elementAdded = true;

        while (elementAdded) {
            elementAdded = false;

            for (size_t i = 0; i < l_ContoursDebut.elements.size(); ++i) {
                auto candidate = l_ContoursDebut.elements[i];

                // --- CAS 1 : Connexion à l'extrémité FIN de la chaîne ---
                if (chainEnd2D.IsEqual(candidate.StartCpy2D, 1E-4)) {
                    currentChain.push_back(candidate);
                    chainEnd2D = candidate.StopCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
                else if (chainEnd2D.IsEqual(candidate.StopCpy2D, 1E-4)) {
                    // On le connecte à la fin, mais il est inversé : on redresse immédiatement ses flags internes
                    std::swap(candidate.StartCpy2D, candidate.StopCpy2D);
                    currentChain.push_back(candidate);
                    chainEnd2D = candidate.StopCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
                // --- CAS 2 : Connexion à l'extrémité DÉBUT de la chaîne ---
                else if (chainStart2D.IsEqual(candidate.StopCpy2D, 1E-4)) {
                    currentChain.push_front(candidate);
                    chainStart2D = candidate.StartCpy2D;
                    l_ContoursDebut.elements.erase(l_ContoursDebut.elements.begin() + i);
                    elementAdded = true;
                    break;
                }
                else if (chainStart2D.IsEqual(candidate.StartCpy2D, 1E-4)) {
                    // On le connecte au début, mais il est inversé : on redresse immédiatement ses flags internes
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
        if (currentChain.size() >= 3) {
            SketchContours l_CurrentContour;
            l_CurrentContour.id = l_IntContourId;

            // Transférer du deque au vecteur final
            for (const auto& el : currentChain) {
                l_CurrentContour.elements.push_back(el);
            }

            // Re-calcul propre de la fermeture
            l_CurrentContour.isClosed = chainStart2D.IsEqual(chainEnd2D, 1E-4);

#ifdef SKETCH_COUNTOUR_CREE_DBG
            std::cout << "\tcount: " << l_CurrentContour.elements.size() << std::endl;
            std::cout << "\tisClosed: " << (l_CurrentContour.isClosed ? "true" : "false") << std::endl;
#endif

            //std::cout<<"\tContours_DisplayContours AVANT Contours_UniformiseSensSegments "<<std::endl;
            //Contours_DisplayContour (l_CurrentContour);

            // Appel aux fonctions de traitement
            Contours_UniformiseSensSegments (l_CurrentContour);
            Contours_CalculeAire (l_CurrentContour);
            Contours_ComputeBoundingBox (l_CurrentContour);

            //std::cout<<"\tContours_DisplayContours APRES Contours_UniformiseSensSegments "<<std::endl;
            //Contours_DisplayContour (l_CurrentContour);

            m_ContoursList.emplace_back(std::move(l_CurrentContour));
        }
        else {
#ifdef SKETCH_COUNTOUR_CREE_DBG
            std::cout << "\tINVALIDE (nb<3)" << std::endl;
#endif
        }

        if (l_ContoursDebut.elements.size() < 3) {
            ThereCouldBeMoreContours = false;
        }

#ifdef SKETCH_COUNTOUR_CREE_DBG
        l_IntContourId++;
#endif

    } while (ThereCouldBeMoreContours);

#ifdef SKETCH_COUNTOUR_CREE_DBG
    std::cout << "\tFIN" << std::endl;
#endif
}






//─────────────────────────────────────────────────────────────────────
//  Process final des contours. à la fin on sait quel est le contour
//      extérieur et quels sont les coutonrs internes d'évidement.
//
//		- Trier la liste par ordre décroissant d'aire (le premier élément devient le contour principal).
//		- Filtrer via la Broad-Phase (les Bounding Boxes) pour écarter immédiatement les contours distants.
//		- Valider l'inclusion et l'absence d'intersection (Narrow-Phase).
//─────────────────────────────────────────────────────────────────────
void SketchParams::Contours_ProcessAndValidate() {

    std::cout<<"SketchParams::Contours_ProcessAndValidate -> debut!"<<std::endl;

    Contours_IdentifieContours ();


    if (m_ContoursList.empty()){
        std::cout<<"\tERREUR : contour vide !"<<std::endl;
        return;
    }

    // 1. Trouver le plus grand contour en triant par l'aire de manière décroissante
    std::sort(m_ContoursList.begin(), m_ContoursList.end(),
              [](const SketchContours& a, const SketchContours& b) {
                  return std::abs(a.Aire) > std::abs(b.Aire);
              });

    // Le premier de la liste est désormais notre contour extérieur (Hôte)
    SketchContours& outerContour = m_ContoursList.front();
    outerContour.isInternal = false; // C'est l'Outer

    std::cout<<"\tContour 0 ->Contour externe"<<std::endl;

    // 2. Vérifier et qualifier les autres contours (les trous potentiels)
    for (size_t i = 1; i < m_ContoursList.size(); ++i) {
        SketchContours& current = m_ContoursList[i];

        // Étape A : Broad-phase avec IsOut
        // Si la boîte du petit n'est PAS contenue ou chevauchée par le grand, problème.
        if (outerContour.BoundingBox2D.IsOut(current.BoundingBox2D)) {
            // Erreur CAO : Un contour se trouve en dehors du contour principal
            current.isInternal = false;
            std::cout<<"\tContour "<< i <<" ->Contour en dehors"<<std::endl;
            continue;
        }

        // Étape B : Narrow-phase (Vérifier s'ils se croisent)
        // On réutilise la fonction d'intersection de segments développée plus tôt
        bool segmentsCross = false;
        for (const auto& edgeOuter : outerContour.elements) {
            for (const auto& edgeInner : current.elements) {
                if (SegmentsIntersect(edgeOuter.StartCpy2D, edgeOuter.StopCpy2D,
                                      edgeInner.StartCpy2D, edgeInner.StopCpy2D)) {
                    segmentsCross = true;

                    break;
                }
            }
            if (segmentsCross) break;
        }

        if (segmentsCross) {
            // Erreur CAO critique : Le trou coupe le bord extérieur (Intersection interdite)
            // Gérer l'erreur ou lever une alerte utilisateur ici
            std::cout<<"\tContour "<< i <<" ->Croisement de segments !"<<std::endl;
            continue;
        }

        // Étape C : Si aucune intersection et BoundingBox OK -> C'est un évidement valide
        current.isInternal = true; // C'est un Hole (poche intérieure)
        std::cout<<"\tContour "<< i <<" ->Contour totalement interne"<<std::endl;
    }

    std::cout << "\tOrientation des contours..." << std::endl;
    for (auto& contour : m_ContoursList) {
        if (!contour.isInternal) {
            // Un contour externe (Outer) DOIT tourner en Anti-Horaire (Trigo) -> Aire > 0
            OrienteContour(contour, SensRotation::AntiHoraire);
        } else {
            // Un contour interne (Hole) DOIT tourner en Horaire -> Aire < 0
            OrienteContour(contour, SensRotation::Horaire);
        }
    }

    Contours_DisplayContours ();
}


void SketchParams::Contours_DisplayContours() {
    std::cout<<"SketchParams::Contours_DisplayContours"<<std::endl;
    for (size_t i = 0; i < m_ContoursList.size(); ++i)
    {
        SketchContours& current = m_ContoursList[i];
        Contours_DisplayContour ( current );
    }
}


void SketchParams::Contours_DisplayContour(SketchContours& current) {
    std::cout<<"SketchParams::Contours_DisplayContour"<<std::endl;

    //SketchContours& current = m_ContoursList[i];
    std::cout<<"\tContour id"<< " elmt.id=" << current.id << std::endl;

    std::cout<<"\t\tAire : "<<current.Aire<<std::endl;
    std::cout<<"\t\tisClosed : "<< (current.isClosed==true?"yes":"no")<<std::endl;
    std::cout<<"\t\tisInternal : "<<(current.isInternal==true?"yes":"no")<<std::endl;
    std::cout<<"\t\tisBounding : ("<<current.BoundingBox2D.GetXMin()<<","<<current.BoundingBox2D.GetYMin() << ")->(";
    std::cout<<current.BoundingBox2D.GetXMax()<<","<<current.BoundingBox2D.GetYMax() << ")" << std::endl;
    std::cout<<"\t\tPoints : ";

    for ( size_t j=0 ; j<current.elements.size() ; j++){
        switch (  current.elements[j].type){
        case PrimitiveType::Line:
            std::cout<< "Line ("<< current.elements[j].StartCpy2D.X() << "," << current.elements[j].StartCpy2D.Y()<<") to ";
            std::cout<< "("<< current.elements[j].StopCpy2D.X() << "," << current.elements[j].StopCpy2D.Y()<<") - ";
            break;
        case PrimitiveType::Circle:
            std::cout<< "Circle ("<< current.elements[j].CenterCpy2D.X() << "," << current.elements[j].CenterCpy2D.Y()<<") radius " << current.elements[j].Radius;
            break;
        default:
            std::cout<< " ERROR DEFAULT 857" << std::endl;
            break;
        }


        if ((j>0) &&  (0==(j % 5)) ){
            std::cout << std::endl << "\t\t";
        }
    }
    std::cout<< std::endl;

}

#endif
