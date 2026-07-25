#pragma once


#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>


// vtk3d_Pipeline.h (ou Vtk3d_ActorFactory.h)
namespace Vtk3d_Actors {

    void configurerWireframeActor(vtkActor* actor, vtkPolyData* polyData, const float color[3]);
void configurerSolideActor(vtkActor* actor, vtkPolyData* polyData, const float color[3]);
}










