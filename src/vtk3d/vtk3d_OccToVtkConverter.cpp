

#include "vtk3d_OccToVtkConverter.h"



#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>            // Pour lire la géométrie de l'arête
#include <BRepMesh_IncrementalMesh.hxx>
#include <vtkCellArray.h>
#include <vtkCellData.h>

#include <GCPnts_TangentialDeflection.hxx>

void Vtk3d_Converter::extraireFacesOccVersVtk(const TopoDS_Shape& shape, vtkPoints* points, vtkCellArray* triangles, vtkIntArray* faceIdsArray) {
    if (shape.IsNull() || !points || !triangles) return;

    // 1. S'assurer que le maillage OCC existe
    BRepMesh_IncrementalMesh(shape, 0.1);

    // --- PHASE 1 : ESTIMATION ET ALLOCATION EXACTE ---
    vtkIdType totalNodes = 0;
    vtkIdType totalTriangles = 0;

    TopExp_Explorer faceExp(shape, TopAbs_FACE);
    for (; faceExp.More(); faceExp.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (!tri.IsNull()) {
            totalNodes += tri->NbNodes();
            totalTriangles += tri->NbTriangles();
        }
    }

    points->SetNumberOfPoints(totalNodes);
    triangles->AllocateExact(totalTriangles, totalTriangles * 3);

    // --- PHASE 2 : REMPLISSAGE DIRECT ---
    vtkIdType pointOffset = 0;
    faceExp.ReInit();

    //--- ajouter id pour le clic
    //auto faceIdsArray = vtkSmartPointer<vtkIntArray>::New();
    faceIdsArray->SetName("OpenCascadeFaceID");
    faceIdsArray->SetNumberOfComponents(1);
    int faceIndex = 0; // Un index incrémental pour identifier tes faces


    for (; faceExp.More(); faceExp.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;

        const gp_Trsf& trans = loc.Transformation();
        int nbNodes = tri->NbNodes();

        // Remplissage des points
        for (int i = 1; i <= nbNodes; ++i) {
            gp_Pnt p = tri->Node(i).Transformed(trans);
            points->SetPoint(pointOffset + i - 1, p.X(), p.Y(), p.Z());
        }

        // Remplissage des triangles
        int nbTriangles = tri->NbTriangles();
        vtkIdType pts[3];
        for (int i = 1; i <= nbTriangles; ++i) {
            Poly_Triangle t = tri->Triangle(i);
            Standard_Integer n1, n2, n3;
            t.Get(n1, n2, n3);

            if (face.Orientation() == TopAbs_REVERSED) {
                std::swap(n2, n3);
            }

            pts[0] = pointOffset + n1 - 1;
            pts[1] = pointOffset + n2 - 1;
            pts[2] = pointOffset + n3 - 1;
            triangles->InsertNextCell(3, pts);

            // 2. STOCKE L'ID ICI : On associe ce triangle à la face OpenCascade courante
            faceIdsArray->InsertNextValue(faceIndex);
        }

        pointOffset += nbNodes;
        faceIndex++; // On passe à la face OpenCascade suivante
    }
}




void Vtk3d_Converter::extraireAretesOccVersVtk(const TopoDS_Shape& shape, vtkPoints* edgePoints, vtkCellArray* edgeLines, vtkIntArray* edgeIdsArray) {
    if (shape.IsNull() || !edgePoints || !edgeLines || !edgeIdsArray) return;

    // 1. Configuration initiale du tableau d'IDs
    edgeIdsArray->SetName("OpenCascadeEdgeID");
    edgeIdsArray->SetNumberOfComponents(1);

    vtkIdType edgePointOffset = 0;
    int edgeIndex = 0; // Index incrémental pour identifier tes arêtes OpenCASCADE

    TopExp_Explorer edgeExp(shape, TopAbs_EDGE);
    for (; edgeExp.More(); edgeExp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
        if (BRep_Tool::Degenerated(edge)) continue;

        BRepAdaptor_Curve curve(edge);
        GCPnts_TangentialDeflection discretizer(curve, 0.1, 0.1);
        int nbPts = discretizer.NbPoints();
        if (nbPts < 2) continue;

        // Insertion des points de discrétisation
        for (int i = 1; i <= nbPts; ++i) {
            gp_Pnt p = discretizer.Value(i);
            edgePoints->InsertNextPoint(p.X(), p.Y(), p.Z());
        }

        // Création de la ligne poly-segments dans VTK (1 cellule créée)
        edgeLines->InsertNextCell(nbPts);
        for (int i = 0; i < nbPts; ++i) {
            edgeLines->InsertCellPoint(edgePointOffset + i);
        }

        // 2. STOCKE L'ID ICI : On associe cette cellule (la polyligne complète) à l'index courant
        edgeIdsArray->InsertNextValue(edgeIndex);

        edgePointOffset += nbPts;
        edgeIndex++; // On passe à l'arête OpenCASCADE suivante
    }
}


void Vtk3d_Converter::convertirSketchPrimitivesVersVtk(const std::vector<SketchPrimitive>& primitives,
                                                       vtkPoints* vtkPoints,
                                                       vtkCellArray* vtkLines,
                                                       vtkUnsignedLongLongArray* customIds)
{
    auto visitor = overload {
        [&](const SketchLine& line) {
            vtkIdType p1 = vtkPoints->InsertNextPoint(line.start.cache_p3d.X(), line.start.cache_p3d.Y(), line.start.cache_p3d.Z());
            vtkIdType p2 = vtkPoints->InsertNextPoint(line.stop.cache_p3d.X(), line.stop.cache_p3d.Y(), line.stop.cache_p3d.Z());

            vtkLines->InsertNextCell(2);
            vtkLines->InsertCellPoint(p1);
            vtkLines->InsertCellPoint(p2);

            customIds->InsertNextValue(line.id); // Lien direct avec ID unique !
        },
        [&](const SketchCircle& circle) {
            // Discrétiser le cercle en petits segments de lignes et leur attribuer circle.id
        },
        [&](const SketchArc& arc) {
            // Idem pour l'arc
        }
    };

    for (const auto& prim : primitives) {
        std::visit(visitor, prim);
    }
}


vtkSmartPointer<vtkPolyData> Vtk3d_Converter::creerWireframePolyData(const TopoDS_Shape& shape) {
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();

    auto edgeIdsArray = vtkSmartPointer<vtkIntArray>::New();
    edgeIdsArray->SetName("OpenCascadeEdgeID");
    edgeIdsArray->SetNumberOfComponents(1);

    // Extraction brute géométrie + IDs
    Vtk3d_Converter::extraireAretesOccVersVtk(shape, points, lines, edgeIdsArray);

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(lines);
    polyData->GetCellData()->AddArray(edgeIdsArray);
    polyData->GetCellData()->SetPedigreeIds(edgeIdsArray);

    return polyData;
}

// vtk3d_OccToVtkConverter.cpp
vtkSmartPointer<vtkPolyData> Vtk3d_Converter::creerSolidePolyData(const TopoDS_Shape& shape) {
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto triangles = vtkSmartPointer<vtkCellArray>::New();

    // 1. Initialisation du tableau d'IDs de faces OpenCASCADE
    auto faceIdsArray = vtkSmartPointer<vtkIntArray>::New();
    faceIdsArray->SetName("OpenCascadeFaceID");
    faceIdsArray->SetNumberOfComponents(1);

    // 2. Extraction algorithmique via ta fonction existante
    Vtk3d_Converter::extraireFacesOccVersVtk(shape, points, triangles, faceIdsArray);

    // 3. Assemblage du PolyData
    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetPolys(triangles);

    // 4. Marquage des métadonnées de sélection (Pedigree)
    polyData->GetCellData()->AddArray(faceIdsArray);
    polyData->GetCellData()->SetPedigreeIds(faceIdsArray);

    return polyData;
}



