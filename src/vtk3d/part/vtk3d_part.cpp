#include "vtk3d_part.h"
#include "vtk3d_MainView.h" // Impératif pour que les modes connaissent les méthodes de la classe mère

#include <QMouseEvent>
#include <vtkAssemblyPath.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleImage.h>
#include <vtkProp3DCollection.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkPolyData.h>
#include <vtkMapper.h>

Vtk3d_Part::Vtk3d_Part(vtk3d_MainView* view) : AbstractViewMode(view) {

}

void Vtk3d_Part::activer() {
    if (!m_view) return;
    if (!m_view->renderWindow()) return;

    vtkRenderWindowInteractor* interactor = m_view->renderWindow()->GetInteractor();
    if (!interactor) return;

    vtkCamera* camera = m_view->getRenderer()->GetActiveCamera();
    if (!camera) return;


    m_view->setCategoryVisibility(SelectionType::Face, true);
    m_view->setCategoryVisibility(SelectionType::Sketch, false);
    m_view->setCategoryVisibility(SelectionType::Axis, true);

    //camera->ParallelProjectionOff();
    camera->ParallelProjectionOn();

    // 2. 🔓 REDONNER LA LIBERTÉ DE ROTATION (Style CAO 3D)
    // On remplace le style "Image" par le style "Trackball Camera" standard de VTK
    auto trackballStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(trackballStyle);

    // 3. 📐 OPTIONNEL : REPLACER EN VUE ISOMÉTRIQUE STANDARD
    // Comme cela, l'utilisateur voit immédiatement qu'il est revenu en mode 3D
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(1.0, -1.0, 1.0);
    camera->SetViewUp(0.0, 0.0, 1.0); // Ton axe Z vers le haut

    // 4. RECALCULER LES PLANS DE COUPE ET RAFRAÎCHIR
    m_view->getRenderer()->ResetCamera();
    m_view->getRenderer()->ResetCameraClippingRange();
    m_view->renderWindow()->Render();

    Signaler_ActivationModePart ();
}
void Vtk3d_Part::desactiver(){

}


bool Vtk3d_Part::gererMousePress(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_MouseclickStartPosition = event->position().toPoint(); // Enregistre la position de départ en pixels logiques Qt
        return true;
    }
    return false;   // On laisse toujours le comportement natif (indispensable pour la navigation VTK)
}
bool Vtk3d_Part::gererMouseMove(QMouseEvent* event) {
    return false;   // On laisse toujours le comportement natif (indispensable pour la navigation VTK)
}
bool Vtk3d_Part::gererWheelEvent(QWheelEvent* event) {
    return false;
}



//#define gererMouseRelease_DBG
bool Vtk3d_Part::gererMouseRelease(QMouseEvent* event) {
#ifdef gererMouseRelease_DBG
    std::cout << " Vtk3d_Part::gererMouseRelease ";
#endif

    if (event->button() == Qt::LeftButton) {
        QPoint clickEndPosition = event->position().toPoint();

        // Calcule la distance de déplacement en pixels
        int deltaX = std::abs(clickEndPosition.x() - m_MouseclickStartPosition.x());
        int deltaY = std::abs(clickEndPosition.y() - m_MouseclickStartPosition.y());

        // Tolérance de 3 pixels pour accepter le clic
        if (deltaX <= 3 && deltaY <= 3) {
#ifdef gererMouseRelease_DBG
            std::cout << "\t Delta<seuil ok " << std::endl;
#endif
            double dpr = m_view->devicePixelRatioF();
            int x = static_cast<int>(event->position().x() * dpr);
            int y = static_cast<int>(event->position().y() * dpr);
            int vtkY = (m_view->height() * dpr) - y;

            auto picker = vtkSmartPointer<vtkCellPicker>::New();
            picker->SetTolerance(0.003);

            if (picker->Pick(x, vtkY, 0, m_view->getRenderer())) {
#ifdef gererMouseRelease_DBG
                std::cout << "\t picker viable" << std::endl;
#endif
                vtkActor* pickedActor = nullptr;
                if (picker->GetPath()) {
                    // Traverse l'arborescence des vtkAssembly pour récupérer le vtkActor feuille cliqué
                    vtkProp* leafProp = picker->GetPath()->GetLastNode()->GetViewProp();
                    pickedActor = vtkActor::SafeDownCast(leafProp);
                }

                vtkIdType cellId = picker->GetCellId();

                // 🎯 Appel de ton analyseur géométrique (qui renvoie le PartId dans operationId)
                SelectionResult selection = m_view->analyserClic(pickedActor, cellId);

                // Si l'acteur n'appartient à aucun Part enregistré, on ne fait rien
                if (selection.operationId == 0) {
                    m_view->m_Chighlighter->masquerSurbrillance();
                    m_view->renderWindow()->Render();
                    return true;
                }

                switch (selection.type) {
                case SelectionType::Face: {
                    std::cout << "[Part ID: " << selection.operationId << "] Face CAO detectee (CellID: " << selection.internalVtkId << ")" << std::endl;
                    vtkPolyData* polyData = vtkPolyData::SafeDownCast(pickedActor->GetMapper()->GetInput());
                    m_view->m_Chighlighter->mettreEnSurbrillanceFaceParId(polyData, selection.internalVtkId);
                    break;
                }
                case SelectionType::Edge: {
                    std::cout << "[Part ID: " << selection.operationId << "] Arete detectee (CellID VTK: " << selection.internalVtkId << ")" << std::endl;
                    vtkPolyData* polyDataEdge = vtkPolyData::SafeDownCast(pickedActor->GetMapper()->GetInput());
                    m_view->m_Chighlighter->mettreEnSurbrillanceEdgeParId(polyDataEdge, selection.internalVtkId);
                    break;
                }
                case SelectionType::Axis: {
                    std::cout << "[Part ID: " << selection.operationId << "] Axe de repere detecte (CellID: " << selection.internalVtkId << ")" << std::endl;
                    double facteurEchelle = m_view->m_dernierParallelScale;
                    vtkPolyData* polyDataEdge = vtkPolyData::SafeDownCast(pickedActor->GetMapper()->GetInput());
                    m_view->m_Chighlighter->mettreEnSurbrillanceAxeParId(polyDataEdge, selection.internalVtkId, facteurEchelle);
                    break;
                }
                case SelectionType::Sketch: {
                    std::cout << "[Part ID: " << selection.operationId << "] Esquisse detectee (ID CAO: " << selection.internalVtkId << ")" << std::endl;
                    vtkPolyData* polyDataSketch = vtkPolyData::SafeDownCast(pickedActor->GetMapper()->GetInput());
                    m_view->m_Chighlighter->mettreEnSurbrillanceEdgeParId(polyDataSketch, selection.internalVtkId);
                    break;
                }
                default:
                    break;
                }
            } else {
                // Clic dans le vide sans bouger : effacement de la surbrillance
                m_view->m_Chighlighter->masquerSurbrillance();
#ifdef gererMouseRelease_DBG
                std::cout << "\tEffacement" << std::endl;
#endif

            }

            m_view->renderWindow()->Render();
            return true; // Court-circuit
        }
    }
    return false;
}







//-----------------------------------------------------------------------
//      Traiter la commande envoyée par l'IHM
//-----------------------------------------------------------------------
void Vtk3d_Part::CADEvent_TraiterCommande(const CadCommandEvent& event) {

    /*
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdActivateTool>(&event.params)  ) {

        bool    ToolSelected = true;
        // Transmet à l'outil actif m_tool via std::visit
        //this->distribuerAmiOutil(cmd->value);
        switch ( cmd->toolMode ){
        case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_line:
            sketch_ActivateTool (SketchTool_mode::Tool_Line);
            break;
        case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_Circle:
            sketch_ActivateTool (SketchTool_mode::Tool_CircleDraw);
            break;
        case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_RectEdges:
            sketch_ActivateTool (SketchTool_mode::Tool_RectEdgesDraw);
            break;
        case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_RectCenter:
            sketch_ActivateTool (SketchTool_mode::Tool_RectCenterDraw);
            break;
        case CadEvent::Sketch::CadEvent_SketchToolMode::Select:
            sketch_ActivateTool (SketchTool_mode::Tool_Select);
            break;
        default:
            ToolSelected = false;
            break;
        }
        if ( true == ToolSelected ){
            CadResponseEvent    resp;
            resp.params = CadEvent::Sketch::RespChangedTool{ cmd->toolMode };
            CADEvent_RemonterEvent (resp);
        }
    }


    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdConstraints>(&event.params)  ) {
        switch ( cmd->cmd ){
        case CadEvent::Sketch::CadEvent_PartSketchConstraints::Constraint_Resolve:
            if ( nullptr != m_Operation ){
                m_Operation->setLocaleTopoChanged(true);
                auto *SketchParam = std::get_if<SketchParams> (&m_Operation->getParamsMutable() );
                if ( nullptr != SketchParam ){
                    //Solver2D_Mapper::Solve(*SketchParam);
                    //Solver2D_Mapper::SolveWithDiagnostics(*SketchParam);
                    Solver2D_Mapper::PrepareAndSolve(*SketchParam);
                    rafraichirAffichageEsquisse();
                }

            }
            break;
        default:
            break;
        }
    }
    */
}

//-----------------------------------------------------------------------
//      envoyer le retour vers l'ihm QT
//-----------------------------------------------------------------------
void Vtk3d_Part::CADEvent_RemonterEvent(const CadResponseEvent& event){
    // 2. Transmet à la vue principale si branchée
    if (m_view) {
        m_view->CADEvent_RemonterEvent(event);
    }
}




void Vtk3d_Part::Signaler_ActivationModePart (){
    CADEvent_RemonterEvent(CadResponseEvent{
        0,
        CadEvent::Part::RespGeneralSignal{
            CadEvent::Part::GeneralMessage::Activated
        }
    });
}



