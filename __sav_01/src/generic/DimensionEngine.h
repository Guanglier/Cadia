// =================================================================
// DANS UN NOUVEAU FICHIER (OU DANS VOTRE CORE) : DimensionEngine.h
// =================================================================
#pragma once
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax3.hxx>
#include <string>
#include <variant>

namespace DimensionEngine {

// --- 1. La cote de distance classique (Lignes, points) ---
enum class DimMode { PointToPoint, Horizontal, Vertical };

struct DistanceDescriptor {
    uint64_t targetPrimitiveId1 = 0;
    uint64_t targetPrimitiveId2 = 0; // Optionnel (ex: distance entre 2 points)
    gp_Pnt   pntStart;
    gp_Pnt   pntStop;
    DimMode  mode = DimMode::PointToPoint;
    double   offset = 0.0;
};

// --- 2. La cote de Diamètre / Rayon (Cercles, arcs) ---
enum class RadialMode { Radius, Diameter };

struct RadialDescriptor {
    uint64_t targetPrimitiveId = 0; // L'ID du cercle/arc
    gp_Pnt   center;               // Centre du cercle
    gp_Pnt   anchorPoint;          // Point touché sur la circonférence
    RadialMode mode = RadialMode::Diameter;
    double   pointerAngleRad = 0.0; // Angle de la flèche de cotation
    double   offset = 0.0;          // Distance du texte par rapport au cercle
    double   diameter = 0.0;
};

// --- 3. La cote d'Angle (Entre deux lignes) ---
struct AngleDescriptor {
    uint64_t targetLineId1 = 0;
    uint64_t targetLineId2 = 0;
    gp_Pnt   intersection;         // Centre fictif de l'arc d'angle
    gp_Pnt   line1End;             // Direction de la ligne 1
    gp_Pnt   line2End;             // Direction de la ligne 2
    double   radius = 15.0;        // Rayon de l'arc de la cote d'angle
};

// =================================================================
// LE VARIANT UNIVERSEL
// =================================================================
using DimensionVariant = std::variant<DistanceDescriptor, RadialDescriptor, AngleDescriptor>;

// Le conteneur final qui possède son ID unique pour le registre
struct Descriptor {
    uint64_t id = 0;
    std::string overrideText = "";
    DimensionVariant data; // Stocke l'un des trois descripteurs ci-dessus
};


inline bool IsMouseInPerpendiZone(const gp_Pnt& A, const gp_Pnt& B, const gp_Pnt& mousePos) {
    gp_Vec AB(A, B);
    gp_Vec AMouse(A, mousePos);
    double longAB2 = AB.SquareMagnitude();
    if (longAB2 < 1e-12) return false;

    double u = AMouse.Dot(AB) / longAB2;
    return (u >= (1.0 / 3.0) && u <= (2.0 / 3.0));
}



struct GeometryResult {
    // Pour les lignes droites (Rappels, cotes rectilignes)
    std::vector<std::pair<gp_Pnt, gp_Pnt>> lines;

    // Pour les arcs de cercle (Cotes d'angle, texte circulaire)
    struct ArcData { gp_Pnt center; gp_Pnt start; gp_Pnt stop; double radius; };
    std::vector<ArcData> arcs;

    // Pour positionner les flèches : {Position_Pointe, Vecteur_Direction, Inversée}
    struct ArrowData { gp_Pnt position; gp_Vec direction; bool bInverted; };
    std::vector<ArrowData> arrows;

    // Pour le texte
    gp_Pnt textPosition;
    double measuredValue = 0.0;
    gp_Vec textDirection; // Pour orienter le texte
};





inline void PrepareFleche3D(
    //vtkSmartPointer<vtkCellArray> lines,
    //vtkSmartPointer<vtkPoints> points,
    GeometryResult& res,
    const gp_Ax3& sketchPlane,
    const gp_Pnt& pntExtremite,    // Pnt3DStart ou Pnt3DStop de la ligne de cote
    const gp_Vec& dirLigneCote,    // Vecteur directeur normalisé de la ligne de cote (de A vers B)
    double longueur,               // ex: 4.0 mm
    double angleDeg,               // ex: 15.0 degrés
    bool bInverser)                // true pour la flèche de départ, false pour celle de fin
{

    // 1. Déterminer le sens de la flèche sur la ligne
    gp_Vec dirFleche = bInverser ? dirLigneCote : -dirLigneCote;

    if (dirFleche.SquareMagnitude() > 1e-12) {
        dirFleche.Normalize();
    } else {
        return; // Évite les calculs si le vecteur est nul
    }

    // 2. Trouver la perpendiculaire à la ligne de cote RESTANT dans le plan d'esquisse
    gp_Vec perpDansPlan = dirLigneCote.Crossed(gp_Vec(sketchPlane.Direction()));
    perpDansPlan.Normalize();

    // 3. Conversion de l'angle en radians
    double angleRad = angleDeg * (M_PI / 180.0);

    // 4. Calcul des deux directions des barbes de la flèche (Trigonométrie)
    // On combine la direction arrière et la perpendiculaire pour ouvrir la flèche
    gp_Vec dirBarbe1 = (dirFleche * std::cos(angleRad)) + (perpDansPlan * std::sin(angleRad));
    gp_Vec dirBarbe2 = (dirFleche * std::cos(angleRad)) - (perpDansPlan * std::sin(angleRad));

    // 5. Calcul des deux points d'extrémité des barbes en 3D
    gp_Pnt pBarbe1 = pntExtremite.Translated(dirBarbe1 * longueur);
    gp_Pnt pBarbe2 = pntExtremite.Translated(dirBarbe2 * longueur);

    res.lines.push_back( { pntExtremite, pBarbe1 } );
    res.lines.push_back( { pntExtremite, pBarbe2 } );

}





inline GeometryResult ComputeGeometry(const gp_Ax3& localPlane, const Descriptor& globalDesc) {
    GeometryResult res;

    // std::visit va aiguiller automatiquement vers la bonne formule
    std::visit([&](const auto& desc)
    {
        using T = std::decay_t<decltype(desc)>;

        if constexpr (std::is_same_v<T, DistanceDescriptor>) {
            gp_Pnt A = desc.pntStart;
            gp_Pnt B = desc.pntStop;
            gp_Pnt LineCotationStart = A;
            gp_Pnt LineCotationStop = B;
            gp_Vec directionXLocal(localPlane.XDirection());
            gp_Vec directionYLocal(localPlane.YDirection());
            gp_Vec AB(A, B);

            switch (desc.mode) {
                case DimMode::PointToPoint: {
                    if (AB.SquareMagnitude() > 1e-12) {
                        gp_Vec vecPerp = AB.Crossed(gp_Vec(localPlane.Direction())).Normalized();
                        LineCotationStart = A.Translated(vecPerp * desc.offset);
                        LineCotationStop  = B.Translated(vecPerp * desc.offset);
                        res.measuredValue = A.Distance(B);
                    }
                    break;
                }
                case DimMode::Horizontal: {
                    double longueurProjetee = AB.Dot(directionXLocal);
                    LineCotationStart = A.Translated(directionYLocal * desc.offset);
                    LineCotationStop  = A.Translated(directionXLocal * longueurProjetee).Translated(directionYLocal * desc.offset);
                    res.measuredValue = std::abs(longueurProjetee);

                    break;
                }
                case DimMode::Vertical: {
                    double longueurProjetee = AB.Dot(directionYLocal);
                    LineCotationStart = A.Translated(directionXLocal * desc.offset);
                    LineCotationStop  = A.Translated(directionYLocal * longueurProjetee).Translated(directionXLocal * desc.offset);
                    res.measuredValue = std::abs(longueurProjetee);
                    break;
                }
            }
            res.lines.push_back( { LineCotationStart, LineCotationStop } );     // ligne de cotation
            res.lines.push_back( { LineCotationStart, A } );                    // entre la ligne de cotation et l'objet coté
            res.lines.push_back( { LineCotationStop, B } );

            //--- préparation du texte -------------
            gp_Vec dirCote(LineCotationStart, LineCotationStop);
            if (dirCote.SquareMagnitude() > 1e-12) {
                res.textDirection = dirCote.Normalized();
            } else {
                res.textDirection = localPlane.XDirection();
            }
            gp_Vec vecCote(LineCotationStart, LineCotationStop);
            res.textPosition = LineCotationStart.Translated(vecCote / 2.0);

            //-- préparation des flèches ----
            double FlechesLongueur = res.measuredValue/3;
            if ( FlechesLongueur > 2.0){
                FlechesLongueur = 2.0;
            }
            PrepareFleche3D ( res, localPlane, LineCotationStart, vecCote, FlechesLongueur, 15.0, true);
            PrepareFleche3D ( res, localPlane, LineCotationStop, vecCote, FlechesLongueur, 15.0, false);

        }
        else if constexpr (std::is_same_v<T, RadialDescriptor>) {

            gp_Vec dirCote(desc.center, desc.anchorPoint);
            if (dirCote.SquareMagnitude() > 1e-12) {
                dirCote = dirCote.Normalized();
            } else {
                dirCote = localPlane.XDirection();
            }
            gp_Pnt IntersectCercleLigneCote = desc.center.Translated (dirCote * desc.diameter);
            gp_Pnt IntersectCercleLigneCoteAutreExtremite = desc.center.Translated (-dirCote * desc.diameter);


            if ( RadialMode::Radius == desc.mode ){
                res.lines.push_back( { desc.center, desc.anchorPoint } );
                PrepareFleche3D ( res, localPlane, desc.center, dirCote, 1.2, 15.0, true);
                PrepareFleche3D ( res, localPlane, IntersectCercleLigneCote, dirCote, 1.2, 15.0, false);

                res.measuredValue = desc.center.Distance(IntersectCercleLigneCoteAutreExtremite);

                //--- préparation du texte -------------
                res.textDirection = dirCote;
                gp_Vec vecCote(desc.center, IntersectCercleLigneCote);
                res.textPosition = desc.center.Translated(vecCote / 2.0);
            }else{
                res.lines.push_back( { IntersectCercleLigneCoteAutreExtremite, desc.anchorPoint } );
                res.measuredValue = IntersectCercleLigneCote.Distance(IntersectCercleLigneCoteAutreExtremite);

                PrepareFleche3D ( res, localPlane, IntersectCercleLigneCoteAutreExtremite, dirCote, 1.2, 15.0, true);
                PrepareFleche3D ( res, localPlane, IntersectCercleLigneCote, dirCote, 1.2, 15.0, false);

                res.textDirection = dirCote;
                gp_Vec vecCote(desc.center, IntersectCercleLigneCote);
                res.textPosition = desc.center.Translated(vecCote / 2.0);
            }


        }
        else if constexpr (std::is_same_v<T, AngleDescriptor>) {
            // Mathématiques pour l'angle :
            // Calcul de l'angle entre les deux vecteurs via OpenCASCADE (gp_Vec::Angle)
            // Générer un arc de cercle dans res.arcs pour matérialiser l'angle
        }
    }, globalDesc.data);

    return res;
}





}; // fin namespace
