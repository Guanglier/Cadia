#include "vtk3d_sketch.h"
#include "vtk3d_MainView.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <iostream>
#include <cmath>
#include "2DSolver_Mapper.h"

bool Vtk3d_Sketch::gererMousePress(QMouseEvent* event) {
    std::visit([event](auto& _tool) {
        _tool.gererMousePress(event);
    }, m_tool);
    return false;
}

bool Vtk3d_Sketch::gererMouseRelease(QMouseEvent* event) {
#ifdef gererMouseReleaseSketch_DBG
    std::cout << " Vtk3d_Sketch::gererMouseRelease " << std::flush;
#endif
    std::visit([event](auto& _tool) {
        _tool.gererMouseRelease(event);
    }, m_tool);
    return true;
}

bool Vtk3d_Sketch::gererMouseMove(QMouseEvent* event) {
    if (!m_view) return false;
    std::visit([event](auto& _tool) {
        _tool.gererMouseMove(event);
    }, m_tool);
    return true;
}

bool Vtk3d_Sketch::gererWheelEvent(QWheelEvent* event) {
    if (!m_view) return false;

    vtkRenderer* renderer = m_view->getRenderer();
    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) return false;

    Echelle_ajusterEchelleElements(camera);

    double mouseX = event->position().x();
    double mouseY = m_view->height() - event->position().y();

    renderer->SetDisplayPoint(mouseX, mouseY, 0.0);
    renderer->DisplayToWorld();
    double pOld[4];
    renderer->GetWorldPoint(pOld);
    double worldOld[3] = { pOld[0]/pOld[3], pOld[1]/pOld[3], pOld[2]/pOld[3] };

    double zoomFactor = (event->angleDelta().y() > 0) ? 0.85 : 1.15;

    if (camera->GetParallelProjection()) {
        camera->SetParallelScale(camera->GetParallelScale() * zoomFactor);
    } else {
        camera->Dolly(1.0 / zoomFactor);
    }

    camera->Modified();
    renderer->ResetCameraClippingRange();
    renderer->ComputeVisiblePropBounds();

    renderer->SetDisplayPoint(mouseX, mouseY, 0.0);
    renderer->DisplayToWorld();
    double pNew[4];
    renderer->GetWorldPoint(pNew);
    double worldNew[3] = { pNew[0]/pNew[3], pNew[1]/pNew[3], pNew[2]/pNew[3] };

    double shiftX = worldOld[0] - worldNew[0];
    double shiftY = worldOld[1] - worldNew[1];
    double shiftZ = worldOld[2] - worldNew[2];

    double pos[3], focal[3];
    camera->GetPosition(pos);
    camera->GetFocalPoint(focal);

    camera->SetPosition(pos[0] + shiftX, pos[1] + shiftY, pos[2] + shiftZ);
    camera->SetFocalPoint(focal[0] + shiftX, focal[1] + shiftY, focal[2] + shiftZ);

    renderer->ResetCameraClippingRange();
    m_view->renderWindow()->Render();

    return true;
}

void Vtk3d_Sketch::keyPressEvent(QKeyEvent* event) {
    std::visit([event](auto& activeTool) {
        activeTool.gererkeyPressEvent(event);
    }, m_tool);

    if (event->key() == Qt::Key_Delete) {
#ifdef DBG_keyPressEvent
        std::cout << "🎯 Touche [Suppr] détectée dans le Sketcher !" << std::endl;
#endif
        return;
    }

    if (event->key() == Qt::Key_S) sketch_ActivateTool(SketchTool_mode::Tool_Select);
    if (event->key() == Qt::Key_L) sketch_ActivateTool(SketchTool_mode::Tool_Line);
    if (event->key() == Qt::Key_C) sketch_ActivateTool(SketchTool_mode::Tool_CircleDraw);
    if (event->key() == Qt::Key_R) sketch_ActivateTool(SketchTool_mode::Tool_RectEdgesDraw);
    if (event->key() == Qt::Key_T) sketch_ActivateTool(SketchTool_mode::Tool_RectCenterDraw);
    if (event->key() == Qt::Key_D) sketch_ActivateTool(SketchTool_mode::Tool_Dimensions);


    if (event->key() == Qt::Key_M){
        if ( nullptr != m_Operation ){
            m_Operation->setLocaleTopoChanged(true);
            auto *SketchParam = std::get_if<SketchParams> (&m_Operation->getParamsMutable() );
            if ( nullptr != SketchParam ){
                //Solver2D_Mapper::Solve(*SketchParam);
                Solver2D_Mapper::SolveWithDiagnostics(*SketchParam);
                Solver2D_Mapper::PrepareAndSolve(*SketchParam);
                rafraichirAffichageEsquisse();
            }

        }
    }


}




