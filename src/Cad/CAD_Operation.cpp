
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

/*

void DiagnostiquerForme(const TopoDS_Shape& shape) {
    BRepCheck_Analyzer analyzer(shape);

    if (analyzer.IsValid()) {
        std::cout << "=> La forme est topologiquement VALIDE." << std::endl;
        return;
    }

    std::cerr << "=> La forme est INVALID ! Liste des erreurs :" << std::endl;

    // On explore la forme pour trouver le sous-élément qui pose problème (ex: le Wire)
    TopExp_Explorer exp(shape, TopAbs_WIRE);
    for (; exp.More(); exp.Next()) {
        const TopoDS_Wire& wire = TopoDS::Wire(exp.Current());
        Handle(BRepCheck_Result) res = analyzer.Result(wire);

        if (!res.IsNull() && !res->IsValid()) {
            // Il y a une erreur sur ce wire, on liste les status
            std::cerr << "   [Wire Error] Problème topologique détecté sur un wire." << std::endl;
            // Note : Tu peux pousser l'analyse en inspectant les arêtes (TopAbs_EDGE) si besoin
        }
    }
}
*/






TopoDS_Shape CoordinateSystem::evaluate(const CAD_Document& doc) const {
    std::cout << "CoordinateSystem::evaluate : Generation geometrique du repère local (Plans XY, XZ, YZ)" << std::endl;

    // 1. Récupérer l'axe spatial de ta structure (gp_Ax2)
    // Si tu n'as pas encore de gp_Ax2 dans CoordinateSystem, tu peux utiliser l'origine par défaut :
    gp_Ax2 repereLocal(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));

    gp_Pnt origine = repereLocal.Location();
    gp_Dir axeZ = repereLocal.Direction();
    gp_Dir axeX = repereLocal.XDirection();
    gp_Dir axeY = repereLocal.YDirection();

    // 2. Définir la demi-taille visuelle des plans (ex: 20mm pour faire un carré de 40mm)
    double size = 5.0;

    // Plan XY
    gp_Pln plnXY(origine, axeZ);
    TopoDS_Face faceXY = BRepBuilderAPI_MakeFace(plnXY, -size, size, -size, size);

    // Plan XZ (sa normale est l'axe Y)
    gp_Pln plnXZ(origine, axeY);
    TopoDS_Face faceXZ = BRepBuilderAPI_MakeFace(plnXZ, -size, size, -size, size);

    // Plan YZ (sa normale est l'axe X)
    gp_Pln plnYZ(origine, axeX);
    TopoDS_Face faceYZ = BRepBuilderAPI_MakeFace(plnYZ, -size, size, -size, size);

    // 3. Fusionner les 3 faces en un seul composé pour qu'OpenCASCADE le propage comme une seule forme
    TopoDS_Compound repereCompound;
    BRep_Builder builder;
    builder.MakeCompound(repereCompound);

    builder.Add(repereCompound, faceXY);
    builder.Add(repereCompound, faceXZ);
    builder.Add(repereCompound, faceYZ);

    // 🌟 L'AJOUT CRUCIAL : Forcer la triangulation OpenCASCADE sur le repère
    // Sans cela, le composé n'a pas de données de facettes (Poly_Triangulation) valides,
    // et ta fonction extraireFacesOccVersVtk ne peut pas associer les triangles aux IDs de faces.
    double deflection = 0.1; // Précision du maillage
    BRepMesh_IncrementalMesh mesh(repereCompound, deflection);
    mesh.Perform();



    return repereCompound;
}


std::string CadOperation::getTypeName() const {
    return std::visit([](const auto& params) -> std::string {
        using T = std::decay_t<decltype(params)>;

        if constexpr (std::is_same_v<T, CoordinateSystem>) return "OriginSystem";
        else if constexpr (std::is_same_v<T, SketchParams>)     return "Sketch";
        else if constexpr (std::is_same_v<T, ExtrudeParams>)    return "Extrusion";
        else if constexpr (std::is_same_v<T, BooleanParams>)    return "BooleanOp";
        else                                                    return "UnknownOp";
    }, m_params); // Utilisez ici le nom de votre variable membre variant (ex: m_params ou m_variant)
}

std::string EBooleanOpToString(EBooleanOp type) {
    switch (type) {
    case EBooleanOp::None:      return "None";
    case EBooleanOp::Union:     return "Union";
    case EBooleanOp::Substract: return "Substract";
    case EBooleanOp::Intersect: return "Intersect";
    default:                     return "Unknown";
    }
}



std::string CadOperation::getConstraintTypeString( const ConstraintType li_ConstType) const {
    // votre switch qui retourne un std::string ou const char*
    switch ( li_ConstType ){
        case ConstraintType::Horizontal:    return "Horizontal"; break;
        case ConstraintType::Vertical:      return "Vertical"; break;
        case ConstraintType::Parallel:      return "Parallel"; break;
        case ConstraintType::Perpendicular: return "Perpendicular"; break;
        case ConstraintType::Coincident:    return "Coincident"; break;
        case ConstraintType::Tangent:       return "Tangent"; break;
        case ConstraintType::Distance:      return "Distance"; break;
        case ConstraintType::Radius:        return "Radius"; break;
        default : return "??"; break;
    }
}
std::string CadOperation::getConstraintSubElementString( const ConstraintSubElement li_SubElmt) const {
    switch ( li_SubElmt ){
    case ConstraintSubElement::Whole:           return "Whole"; break;
        case ConstraintSubElement::StartPoint:  return "StartPoint"; break;
        case ConstraintSubElement::EndPoint:    return "EndPoint"; break;
        case ConstraintSubElement::CenterPoint: return "CenterPoint"; break;
        default : return "??"; break;
    }
}



