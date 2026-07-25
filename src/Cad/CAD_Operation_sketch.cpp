
#include "CAD_Operation.h"
#include "CAD_Document.h" // Indispensable ICI pour que le compilateur connaisse enfin les methodes de CAD_Document !
#include "Contours.h"

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
#include <BRepLib.hxx>
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

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <GC_MakeCircle.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <ElSLib.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>

#include <TopExp_Explorer.hxx>
#include <ShapeFix_Wire.hxx>
#include <Geom_Plane.hxx>

#include "Logger.h"


std::vector<ContoursElement> SketchParams::PrepareEnginePrimitives() const {
    std::vector<ContoursElement> enginePrimitives;

    // On parcourt les primitives graphiques stockées dans le Sketch (getPrimitives())
    for (const auto& primitive : getPrimitives()) {
        std::visit([&enginePrimitives](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                ContoursElement elmt;
                elmt.primitiveId = arg.id;
                elmt.StartCpy2D = arg.start.p2d;
                elmt.StopCpy2D = arg.stop.p2d;
                elmt.type = ContoursPrimitiveType::Line;
                enginePrimitives.push_back(elmt);
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                ContoursElement elmt;
                elmt.primitiveId = arg.id;
                elmt.CenterCpy2D = arg.center.p2d;
                elmt.Radius = arg.radius;
                elmt.type = ContoursPrimitiveType::Circle;
                enginePrimitives.push_back(elmt);
            }
            // Ajoute ici d'autres types (ex: SketchArc) si nécessaire

        }, primitive);
    }

    return enginePrimitives;
}


void PrintPoint(const std::string& label, const gp_Pnt& p) {
    LOG_DEBUG << label << " ("
              << p.X() << ", "
              << p.Y() << ", "
              << p.Z() << ")" << std::endl;
}

//─────────────────────────────────────────────────────────────────────
// Évalue le Sketch pour générer la face plane finale prête à l'extrusion
//─────────────────────────────────────────────────────────────────────
TopoDS_Shape SketchParams::evaluate(const CAD_Document& doc) const {
    LOG_DEBUG << "SketchParams::evaluate -> Début" << std::endl;

    std::vector<ContoursElement> inputPrimitives = PrepareEnginePrimitives();
    ContoursTopologyResult topologyResult = ContoursEngine::Process(inputPrimitives);
    ContoursEngine::Contours_DisplayContours(topologyResult);

    if (topologyResult.hasErrors || topologyResult.validContours.empty()) {
        LOG_ERROR << "\t❌ Erreur : Topologie du Sketch invalide ou vide." << std::endl;
        return TopoDS_Shape();
    }

    TopoDS_Wire outerWire3D;
    bool outerWireCreated = false;
    std::vector<TopoDS_Wire> holeWires3D;

    // 4. Parcourir les contours validés
    for (const auto& contour : topologyResult.validContours)
    {
        if (contour.hasError || !contour.isClosed) {
            continue;
        }

        // UTILISATION DE BRepBuilderAPI_MakeWire (au lieu de BRep_Builder raw)
        BRepBuilderAPI_MakeWire wireBuilder;

        for (const auto& element : contour.elements)
        {
            TopoDS_Edge edge;

            switch (element.type) {
            case ContoursPrimitiveType::Line: {
                gp_Pnt p3dStart = ElSLib::Value(element.StartCpy2D.X(), element.StartCpy2D.Y(), m_sketchPlane);
                gp_Pnt p3dStop  = ElSLib::Value(element.StopCpy2D.X(), element.StopCpy2D.Y(), m_sketchPlane);
                edge = BRepBuilderAPI_MakeEdge(p3dStart, p3dStop);

                PrintPoint( "start:", p3dStart);
                PrintPoint( "Stop:", p3dStop);
                LOG_DEBUG <<std::endl;
                break;
            }
            case ContoursPrimitiveType::Circle: {
                gp_Pnt p3dCenter = ElSLib::Value(element.CenterCpy2D.X(), element.CenterCpy2D.Y(), m_sketchPlane);
                gp_Ax2 axe(p3dCenter, m_sketchPlane.Direction());
                Handle(Geom_Circle) geomCircle = GC_MakeCircle(axe, element.Radius).Value();
                edge = BRepBuilderAPI_MakeEdge(geomCircle);
                break;
            }
            case ContoursPrimitiveType::Arc: {
                gp_Pnt p3dCenter = ElSLib::Value(element.CenterCpy2D.X(), element.CenterCpy2D.Y(), m_sketchPlane);
                gp_Pnt p3dStart  = ElSLib::Value(element.StartCpy2D.X(), element.StartCpy2D.Y(), m_sketchPlane);
                gp_Pnt p3dStop   = ElSLib::Value(element.StopCpy2D.X(), element.StopCpy2D.Y(), m_sketchPlane);

                gp_Ax2 axe(p3dCenter, m_sketchPlane.Direction());
                gp_Circ circSupport(axe, element.Radius);
                GC_MakeArcOfCircle arcMaker(circSupport, p3dStart, p3dStop, Standard_True);

                if (arcMaker.IsDone()) {
                    edge = BRepBuilderAPI_MakeEdge(arcMaker.Value());
                }
                break;
            }
            }

            if (!edge.IsNull()) {
                wireBuilder.Add(edge); // MakeWire gère la continuité et l'orientation !
            }
        }


        TopoDS_Wire currentWire3D;

        // 1. Diagnostics et traitement MakeWire
        if (wireBuilder.IsDone()) {
            LOG_DEBUG << "\t✅ BRepBuilderAPI_MakeWire OK" << std::endl;
            currentWire3D = wireBuilder.Wire();
        } else {
            // Premier diagnostic : Affichage de la raison exacte d'échec de MakeWire
            LOG_ERROR << "\t⚠️ Échec BRepBuilderAPI_MakeWire. Raison : ";
            switch (wireBuilder.Error()) {
            case BRepBuilderAPI_WireDone:
                LOG_ERROR << "OK" << std::endl; break;
            case BRepBuilderAPI_EmptyWire:
                LOG_ERROR << "Wire Vide" << std::endl; break;
            case BRepBuilderAPI_DisconnectedWire:
                LOG_ERROR << "Wire Disjoint (Écart entre sommets supérieur à la tolérance BRep)" << std::endl; break;
            case BRepBuilderAPI_NonManifoldWire:
                LOG_ERROR << "Wire Non-Manifold (Auto-intersection / Bicyclette / Branloche)" << std::endl; break;
            default:
                LOG_ERROR << "Erreur Inconnue (" << wireBuilder.Error() << ")" << std::endl; break;
            }

            // 2. Fallback avec ShapeFix_Wire pour recoudre les micro-écarts du solveur
            std::cout << "\t  ↳ Tentative de réparation via ShapeFix_Wire..." << std::endl;

            BRep_Builder rawBuilder;
            TopoDS_Wire rawWire;
            rawBuilder.MakeWire(rawWire);

            for (const auto& element : contour.elements) {
                gp_Pnt p3dStart = ElSLib::Value(element.StartCpy2D.X(), element.StartCpy2D.Y(), m_sketchPlane);
                gp_Pnt p3dStop  = ElSLib::Value(element.StopCpy2D.X(), element.StopCpy2D.Y(), m_sketchPlane);
                TopoDS_Edge e = BRepBuilderAPI_MakeEdge(p3dStart, p3dStop);
                if (!e.IsNull()) rawBuilder.Add(rawWire, e);
            }

            TopoDS_Face supportFace = BRepBuilderAPI_MakeFace(m_sketchPlane);
            ShapeFix_Wire fixWire;
            fixWire.Init(rawWire, supportFace, 1e-3); // 1 micron de tolérance
            fixWire.FixConnected();
            fixWire.FixClosed();

            currentWire3D = fixWire.Wire();
        }

        // 3. Second diagnostic : Vérification finale de la fermeture du Wire
        LOG_DEBUG << "\t[Diagnostic Contour " << (contour.isInternal ? "Intérieur" : "Extérieur")
                  << "] " << (currentWire3D.Closed() ? "✅ Fermé" : "❌ Ouvert") << std::endl;

        if (!contour.isInternal) {
            outerWire3D = currentWire3D;
            outerWireCreated = true;
        } else {
            holeWires3D.push_back(currentWire3D);
        }
    }

    if (!outerWireCreated) {
        LOG_ERROR << "\t❌ Erreur : Pas de contour extérieur principal." << std::endl;
        return TopoDS_Shape();
    }

    // 5. CONSTRUCTION DE LA FACE PLANE
    LOG_DEBUG << "\tGénération de la Face plane avec évidements..." << std::endl;

    BRepBuilderAPI_MakeFace faceMaker(m_sketchPlane, outerWire3D);
    for (const auto& holeWire : holeWires3D) {
        TopoDS_Wire reversedHole = holeWire;
        reversedHole.Orientation(TopAbs_REVERSED);
        faceMaker.Add(reversedHole);
    }

    if (!faceMaker.IsDone()) {
        LOG_ERROR << "\t❌ Erreur : BRepBuilderAPI_MakeFace a échoué." << std::endl;
        return outerWire3D;
    }

    TopoDS_Face faceFinale = faceMaker.Face();

    // 6. APPLIQUER BRepLib::BuildCurves3d POUR CORRIGER LA TOPOLOGIE
    // S'assure que les tolérances topologiques et les géométries 3D sont valides pour BRepOffsetAPI
    BRepLib::BuildCurves3d(faceFinale);

    LOG_INFO << "\tFace créée avec succès." << std::endl;
    return faceFinale;
}


/**
 * @brief Ajoute une contrainte au registre en évitant les doublons.
 * @param constraint [Entrée] La contrainte à ajouter.
 * @return uint64_t L'identifiant (ID) de la contrainte (existante ou nouvellement créée).
 */
uint64_t addConstraint(SketchConstraint constraint) {
    // Vérification des doublons via getItems()
    for (const auto& existingConstraint : m_constraintRegistry.getItems()) {
        if (existingConstraint.isEquivalentTo(constraint)) {
            // Un doublon existe déjà, on retourne son ID existant sans l'ajouter en double
            return existingConstraint.id;
        }
    }

    // Sinon, ajout normal via le registre
    return m_constraintRegistry.add(std::move(constraint));
}


