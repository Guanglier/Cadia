
#include "vtk3d_actors.h"



void Vtk3d_Actors::configurerWireframeActor(vtkActor* actor, vtkPolyData* polyData, const float color[3]) {
	if (!actor || !polyData) return;

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
	if (!mapper) {
		mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		actor->SetMapper(mapper);

		// 🎨 Style CAO standard unifié
		actor->GetProperty()->SetLineWidth(2.0);
		actor->GetProperty()->LightingOff();
		actor->GetProperty()->RenderLinesAsTubesOn();

		// Gestion globale des décalages anti-artefacts
		mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1.0, -1.0);
		mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1.0, -1.0);
	}

	mapper->SetInputData(polyData);
	actor->GetProperty()->SetColor(color[0], color[1], color[2]);
}


void Vtk3d_Actors::configurerSolideActor(vtkActor* actor, vtkPolyData* polyData, const float color[3]) {
    if (!actor || !polyData) return;

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (!mapper) {
        mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        actor->SetMapper(mapper);

        // 🎨 Charte graphique de rendu des volumes 3D
        actor->GetProperty()->SetRepresentationToSurface();
        actor->GetProperty()->SetAmbient(0.4);
        actor->GetProperty()->SetDiffuse(0.7);
        actor->GetProperty()->SetSpecular(0.01);
        actor->GetProperty()->EdgeVisibilityOff();
    }

    mapper->SetInputData(polyData);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
}








