#pragma once

#include <vtkActor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>                  // Pour manipuler l'objet Arête
#include <TopoDS_Shape.hxx>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkUnsignedLongLongArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>

#include "CAD_PartOp.h"


#include <variant>
template<class... T> struct overload : T... { using T::operator()...; };
template<class... T> overload(T...) -> overload<T...>;

// On déplace tes fonctions ici pour qu'elles soient globales et réutilisables
namespace Vtk3d_Converter {

    // Pour le mode 3D (Solides, Extrusions, Faces)
    void extraireFacesOccVersVtk(const TopoDS_Shape& shape,
                                 vtkPoints* points,
                                 vtkCellArray* triangles,
                                 vtkIntArray* faceIdsArray);

    // Pour les arêtes du solide 3D et les plans de référence
    void extraireAretesOccVersVtk(const TopoDS_Shape& shape,
                                  vtkPoints* points,
                                  vtkCellArray* lines,
                                  vtkIntArray* edgeIdsArray);

    // 🌟 LA NOUVEAUTÉ POUR TON MODE ESQUISSE : Convertir tes variants directement !
    void convertirSketchPrimitivesVersVtk(const std::vector<SketchPrimitive>& primitives,
                                          vtkPoints* vtkPoints,
                                          vtkCellArray* vtkLines,
                                          vtkUnsignedLongLongArray* customIds);

    vtkSmartPointer<vtkPolyData> creerWireframePolyData(const TopoDS_Shape& shape);
    vtkSmartPointer<vtkPolyData> creerSolidePolyData(const TopoDS_Shape& shape);


}

//void extraireFacesOccVersVtk(const TopoDS_Shape& shape, vtkPoints* points, vtkCellArray* triangles, vtkIntArray* faceIdsArray);
//void extraireAretesOccVersVtk(const TopoDS_Shape& shape, vtkPoints* edgePoints, vtkCellArray* edgeLines, vtkIntArray* edgeIdsArray);









