
#include "CAD_Operation.h"
#include "CAD_Document.h" // Indispensable ICI pour que le compilateur connaisse enfin les methodes de CAD_Document !

// OpenCASCADE requis pour construire la geometrie
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepTools.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <GC_MakeCircle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <type_traits>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopoDS_Wire.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx> // Nécessaire pour GeomAbs_Plane

#include "Logger.h"



TopoDS_Shape ExtrudeParams::evaluate(const CAD_Document& doc) const {
    std::cout << "ExtrudeParams::evaluate\n";

    const CadOperation* opEsquisse = doc.trouverOperation(this->SketchId);
    if (!opEsquisse) {
        LOG_ERROR << "\tExtrudeParams::evaluate -> Erreur : Esquisse parente ID "
                  << this->SketchId << " introuvable." << std::endl;
        return TopoDS_Shape();
    }

    // 1. Récupération de la forme générée par l'esquisse
    TopoDS_Shape topoEsquisse = opEsquisse->getLocalTopo();
    if (topoEsquisse.IsNull()) {
        LOG_ERROR << "\tExtrudeParams::evaluate -> Erreur : La geometrie de l'esquisse parente est vide." << std::endl;
        return TopoDS_Shape();
    }

    // NOUVEAU : On sait maintenant que cette forme EST la face plane évidée !
    if (topoEsquisse.ShapeType() != TopAbs_FACE) {
        LOG_ERROR << "\tExtrudeParams::evaluate -> Erreur : L'esquisse n'a pas genere une Face." << std::endl;
        return TopoDS_Shape();
    }
    TopoDS_Face maFace = TopoDS::Face(topoEsquisse); // Cast sécurisé

    // --- CONFIGURATION DE L'EXTRUSION ASYMÉTRIQUE ---
    double startOffset = this->start;
    double endOffset = this->end;

    // Détermination de la direction d'extrusion
    gp_Vec vecDirectionUnit;
    if (this->vecteurExtrusion.Magnitude() > 0.001) {
        vecDirectionUnit = this->vecteurExtrusion.Normalized();
    } else {
        // Extraction de la normale de notre face plane
        BRepAdaptor_Surface adaptateur(maFace);
        if (adaptateur.GetType() == GeomAbs_Plane) {
            gp_Pln planForme = adaptateur.Plane();
            vecDirectionUnit = gp_Vec(planForme.Position().Direction()); // Normale exacte
        } else {
            vecDirectionUnit = gp_Vec(0, 0, 1); // Fallback par défaut
        }
    }

    // 4. Translation de la face de départ (startOffset)
    TopoDS_Face faceDeplacee = maFace;
    if (std::abs(startOffset) > 1e-6) {
        gp_Trsf translation;
        translation.SetTranslation(vecDirectionUnit * startOffset);

        BRepBuilderAPI_Transform transformateur(maFace, translation);
        if (!transformateur.IsDone()) {
            LOG_ERROR << "\t-> Echec de la translation de la face d'origine." << std::endl;
            return TopoDS_Shape();
        }
        faceDeplacee = TopoDS::Face(transformateur.Shape());
    }

    // 5. Calcul du vecteur d'extrusion total
    double distanceTotale = endOffset - startOffset;
    gp_Vec vecFinal = vecDirectionUnit * distanceTotale;

    if (std::abs(distanceTotale) < 1e-4) {
        LOG_ERROR << "\t-> Erreur : La distance d'extrusion totale est nulle." << std::endl;
        return TopoDS_Shape();
    }

    // 6. Appel au moteur de Prism d'OpenCASCADE
    BRepPrimAPI_MakePrism prismBuilder(faceDeplacee, vecFinal);

    if (prismBuilder.IsDone()) {
        LOG_INFO << "\t-> Solide 3D asymetrique genere avec succes !" << std::endl;
        return prismBuilder.Shape(); // Retourne le solide percé
    }

    LOG_ERROR << "\t-> Echec de l'extrusion OpenCASCADE." << std::endl;
    return TopoDS_Shape();
}



