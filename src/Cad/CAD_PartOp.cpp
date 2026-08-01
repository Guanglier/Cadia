
#include "CAD_PartOp.h"
#include "CAD_Part.h"

// OpenCASCADE requis pour construire la geometrie
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <type_traits>
#include <TopoDS_Compound.hxx>






SketchPoint& SketchParams::GetPointById ( uint64_t li_id){
    SketchPoint* pt = m_points.findMutable(li_id);
    if (pt != nullptr) {
        return *pt; // On déréférence pour renvoyer une référence SketchPoint&
    }
    // 2. Gestion d'erreur si l'ID n'existe pas (par exemple, lancer une exception)
    throw std::runtime_error("Point ID non trouvé dans le registre !");
}
void SketchParams::removePrimitive(uint64_t idASupprimer) {
    m_primitiveRegistry.remove(idASupprimer);
    auto& constraints = m_constraintRegistry.getItemsMutable();
    constraints.erase(
        std::remove_if(constraints.begin(), constraints.end(), [idASupprimer](const PartSketchConstraint& c) {
            return c.ref1.primitiveId == idASupprimer || c.ref2.primitiveId == idASupprimer;
        }),
        constraints.end()
        );
}


uint64_t SketchParams::addPoint (const gp_Pnt2d& li_Pnt2D) {
    uint64_t lid;
    if ( false == PointExists(li_Pnt2D,  lid)){
        SketchPoint l_point(li_Pnt2D);
        l_point.Update3D( m_sketchPlane);
        return m_points.add(l_point);
    }else{
        return lid;
    }
}
uint64_t SketchParams::addLine ( gp_Pnt2d li_PntStart2d, gp_Pnt2d li_PntStop2d ){
    uint64_t u64_IdStart = addPoint ( li_PntStart2d );
    uint64_t u64_IdStop = addPoint ( li_PntStop2d );
    SketchLine  line( u64_IdStart, u64_IdStop );
    line.b_IsRef = false;
    return addPrimitive ( line);
}
uint64_t SketchParams::addCircle ( gp_Pnt2d li_PntCenter2d, double radius){
    uint64_t u64_IdCenter = addPoint ( li_PntCenter2d );
    SketchCircle  circle( u64_IdCenter, radius );
    return addPrimitive(circle);
}
bool SketchParams::PointExists ( const gp_Pnt2d& li_Pnt2D, uint64_t &lo_PointId){
    const auto& items = m_points.getItems();

    double tolerance = 1e-6;
    auto it = std::find_if(items.begin(), items.end(), [&](const SketchPoint& point) {
        return li_Pnt2D.IsEqual( point.p2d, tolerance);
    });
    if (it != items.end()) {
        lo_PointId = it->id; // Récupération de l'ID associé
        return true;         // Point trouvé
    }
    return false; // Point non trouvé
}



TopoDS_Shape CoordinateSystem::evaluate(const CAD_Part& part) const {
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


std::string CadPartOp::getTypeName() const {
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



std::string CadPartOp::getConstraintTypeString( const ConstraintType li_ConstType) const {
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
std::string CadPartOp::getConstraintSubElementString( const ConstraintSubElement li_SubElmt) const {
    switch ( li_SubElmt ){
    case ConstraintSubElement::Whole:           return "Whole"; break;
        case ConstraintSubElement::StartPoint:  return "StartPoint"; break;
        case ConstraintSubElement::EndPoint:    return "EndPoint"; break;
        case ConstraintSubElement::CenterPoint: return "CenterPoint"; break;
        default : return "??"; break;
    }
}



