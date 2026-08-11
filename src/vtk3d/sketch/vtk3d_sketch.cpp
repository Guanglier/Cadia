
#include "vtk3d_sketch.h"
#include "vtk3d_MainView.h"
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkPointData.h>
#include "2DSolver_Mapper.h"
#include "Logger.h"

#undef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL LogLevel::Debug

Vtk3d_Sketch::Vtk3d_Sketch(vtk3d_MainView* view, CadPartOp* li_ptr_Operation)
    : AbstractViewMode(view),
    m_tool(Tool_Select(this))
{
    gp_Pnt origine(0.0, 0.0, 0.0);
    gp_Dir normale(0.0, 0.0, 1.0); // Plein axe Z
    gp_Dir axeX(1.0, 0.0, 0.0);    // Plein axe X

    gp_Ax3 planXY(origine, normale, axeX);




    gp_Ax1 axeDeRotation(origine, axeX);
    double angleRadians = 45.0 * (M_PI / 180.0);
    PartRefs.SetSketchPlane( planXY.Rotated(axeDeRotation, angleRadians) );


    if (nullptr == li_ptr_Operation) {
        PartRefs.SetOperation( new CadPartOp("Esquisse test", SketchParams() ) );
    } else {
        PartRefs.SetOperation( li_ptr_Operation );
    }


    auto* variantParams = &(PartRefs.GetOperation()->getParamsMutable());
    if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ){
        sketchParams->m_sketchPlane = PartRefs.GetSketchPlane();
    }else{
        std::cout << "ERROR if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ) " << std::endl;
        std::cerr << "ERROR if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ) " << std::endl;
        assert(false && "CRITICAL: ERROR if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ).");
    }

    m_view = view;

    m_SnapperManager = std::make_unique<SketchSnapperManager>(this);
    m_SnapperManager->init();


    // ====================================================================
    // FIX INITIALISATION COORDONNÉES 3D
    // On force le calcul des coordonnées 3D du cache avant le premier rendu
    // ====================================================================
    if (nullptr != PartRefs.GetOperation()) {
        PartRefs.GetOperation()->setLocaleTopoChanged(true);
        if (auto* sketchParams = std::get_if<SketchParams>(&PartRefs.GetOperation()->getParamsMutable())) {
            sketchParams->recomputeGeometry3D(); // Remplira les cache_p3d !
        }
    }
    // Afficher ce qui est contenu
    rafraichirAffichageEsquisse();
	
	if (!view) {
		throw std::invalid_argument("view == null dans Vtk3d_Sketch::Vtk3d_Sketch");
	}
	
    m_Cotation = new vtk3d_Sketch_Render_Cotations(m_view->getRenderer());
    m_Cotation2 = new vtk3d_Sketch_Render_Cotations(m_view->getRenderer());
}

Vtk3d_Sketch::~Vtk3d_Sketch() {
    if (m_view && m_view->getRenderer()) {
        if (m_ActorSketchDisplay) {
            m_view->getRenderer()->RemoveActor(m_ActorSketchDisplay);
        }
        if (m_ActorPointsOfPrim) {
            m_view->getRenderer()->RemoveActor(m_ActorPointsOfPrim);
        }
        if (m_constraintsDisplayActor) {
            m_view->getRenderer()->RemoveActor(m_constraintsDisplayActor);
        }
        m_view->renderWindow()->Render();
    }
}







void Vtk3d_Sketch::activer() {
    if (!m_view || !m_view->renderWindow()) return;


    m_view->setCategoryVisibility(SelectionType::Face, false);
    m_view->setCategoryVisibility(SelectionType::Sketch, false);
    m_view->setCategoryVisibility(SelectionType::Axis, false);

    vtkRenderer* renderer = m_view->getRenderer();
    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) return;



    gp_Pnt origine = PartRefs.GetSketchPlane().Location();
    gp_Dir normale = PartRefs.GetSketchPlane().Direction();
    gp_Dir vueHaut = PartRefs.GetSketchPlane().YDirection();

    //GetDistance renvoie la distance idéale après le resetcamera
    // ensuite on repositionne la camera et on la remet à la bonne distance
    renderer->ResetCamera();
    double distanceCamera = camera->GetDistance();
    if (distanceCamera < 1.0) {
        distanceCamera = 500.0;
    }
    camera->SetFocalPoint(origine.X(), origine.Y(), origine.Z());
    camera->SetPosition(
        origine.X() + normale.X() * distanceCamera,
        origine.Y() + normale.Y() * distanceCamera,
        origine.Z() + normale.Z() * distanceCamera
        );
    camera->SetViewUp(vueHaut.X(), vueHaut.Y(), vueHaut.Z());
    camera->ParallelProjectionOn();

    vtkRenderWindowInteractor* interactor = m_view->renderWindow()->GetInteractor();
    if (!interactor) return;

    // Figer la rotation : passage sur le style Image (uniquement Pan & Zoom)
    // origine et fonctionnait, mais en recherche de desactiver le zoom

    //auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
    //interactor->SetInteractorStyle(imageStyle);
    // note on change l'interactor car il traitait le zoom et le pan
    // le zoom empêchait de faire le soom depuis le point de la souris
    // désormais le zoom et le pan seront gérés dans les classes outils
    auto noOpStyle = vtkSmartPointer<vtkInteractorStyle>::New();
    interactor->SetInteractorStyle(noOpStyle);


    //m_view->getRenderer()->ResetCamera(); //fait le reset de la caméra et donc perd l'angle ..

    if ( nullptr != PartRefs.GetOperation() ){
        PartRefs.GetOperation()->setLocaleTopoChanged(true);
        auto *SketchPara = std::get_if<SketchParams> (&PartRefs.GetOperation()->getParamsMutable() );
        if ( nullptr != SketchPara ){
            SketchPara->recomputeGeometry3D();
        }

    }


    m_view->getRenderer()->ResetCameraClippingRange();
    interactor->GetRenderWindow()->Render();

    Signaler_ActivationModeEsquisse ();
}

void Vtk3d_Sketch::desactiver() {
    if (m_view ) {
        //vtkCamera* camera = m_view->getRenderer()->GetActiveCamera();

        if (m_ActorPointsOfPrim) {
            m_ActorPointsOfPrim->SetVisibility(false);
		}
        if (m_constraintsDisplayActor) {
            m_constraintsDisplayActor->SetVisibility(false);
        }
        if ( nullptr != PartRefs.GetOperation() ){
            auto *SketchPara = std::get_if<SketchParams> (&PartRefs.GetOperation()->getParamsMutable() );
            if ( nullptr != SketchPara ){
                SketchPara->recomputeGeometry3D();
            }
            PartRefs.GetOperation()->setLocaleTopoChanged(true);
        }
    }
}




inline std::string Vtk3d_Sketch::ToolMode_To_String (CadEvent::Sketch::ToolMode li_internalMode, CadEvent::Sketch::Tool_SubMode li_submode) {
    switch(li_internalMode) {
        case CadEvent::Sketch::ToolMode::Draw_Circle:         return std::string ( "Outil : Tool_CircleDraw !! ") ;        break;
        case CadEvent::Sketch::ToolMode::Draw_line:           return std::string ( "Outil : Tool_Line !! ") ;            break;
        case CadEvent::Sketch::ToolMode::Draw_Rectangle:      return std::string ( "Outil : Draw_Rectangle !! ") ;   break;
        case CadEvent::Sketch::ToolMode::Select:              return std::string ( "Outil : Tool_Select !! " );            break;
        case CadEvent::Sketch::ToolMode::SetConstraints:      return std::string ( "Outil : Tool_SetConstraints !! ") ;   break;
        case CadEvent::Sketch::ToolMode::Dimensions:          return std::string ( "Outil : Tool_Dimensions !! ") ;       break;
        default:                                              return std::string ( "ERREUR  sketch_ActivateTool") ;        break;
    }
}




void Vtk3d_Sketch::sketch_ActivateTool(CadEvent::Sketch::ToolMode li_tool, CadEvent::Sketch::Tool_SubMode li_submode) {
    bool    ToolSelected = false;

    // if (m_ToolMode != li_tool) {
        m_ToolMode = li_tool;

        std::visit([](auto& activeTool) {
            activeTool.desactivate();
        }, m_tool);

        std::string l_string = "Outil : DEFAULT !! " ;

        switch(m_ToolMode) {
            case CadEvent::Sketch::ToolMode::Draw_Circle:
                m_tool = Tool_CircleDraw{this};
                ToolSelected = true;
                break;
            case CadEvent::Sketch::ToolMode::Draw_line:
                m_tool = Tool_LineDraw{this};
                ToolSelected = true;
                break;
            case CadEvent::Sketch::ToolMode::Draw_Rectangle:
                m_tool = Tool_Rectangle{this};
                ToolSelected = true;
                break;
            case CadEvent::Sketch::ToolMode::Select:
                m_tool = Tool_Select{this};
                ToolSelected = true;
                break;
            case CadEvent::Sketch::ToolMode::Dimensions:
                m_tool = Tool_Dimensions{this};
                ToolSelected = true;
                break;
            case CadEvent::Sketch::ToolMode::SetConstraints:
                m_tool = Tool_SetConstraints{this};
                ToolSelected = true;
                break;
            default:
                ToolSelected = false;
                break;
        }

        m_SnapperManager->snapPointsVisited_Clean();

        l_string = ToolMode_To_String ( li_tool , li_submode);
        CadResponseEvent resp;
        resp.PartId = 0; // id du doc
        resp.params = CadEvent::Sketch::RespStatus{
            l_string
        };
        CADEvent_RemonterEvent(resp);

        if ( true == ToolSelected ){
            CadResponseEvent    resp;
            resp.params = CadEvent::Sketch::RespChangedTool{ li_tool , li_submode };
            CADEvent_RemonterEvent (resp);
        }

        std::visit([li_submode](auto& activeTool) {
            activeTool.activate( li_submode );
        }, m_tool);
    //}
}


void Vtk3d_Sketch::SolveEsquisse() {
    if (!PartRefs.GetOperation()) return;

    auto* sketchParams = std::get_if<SketchParams>(&PartRefs.GetOperation()->getParamsMutable());
    if (!sketchParams) return;

    // 1. Lancer ton solveur (ex: ta fonction de résolution avec Jacobi/ExprTk)
    SolverOneShot::Solve(*sketchParams);

    // 2. Recalculer les positions 3D des caches si nécessaire
    sketchParams->recomputeGeometry3D();

    // 3. Rafraîchir l'affichage graphique
    rafraichirAffichageEsquisse();
}


// y mettre event->position().x() dans screenX etc
PickResult Vtk3d_Sketch::PickerGetPickedElement(int screenX, int screenY){

    PickResult  l_result;
    l_result.id = -1;
    l_result.type = PickResult::TargetType::None;



    double dpr = GetView()->devicePixelRatioF();
    int x = static_cast<int>(screenX * dpr);
    int y = static_cast<int>(screenY * dpr);
    int vtkY = (GetView()->height() * dpr) - y;

    auto cellPicker = vtkSmartPointer<vtkCellPicker>::New();
    double pixelsTarget = 4.0;
    int* winSize = GetView()->getRenderer()->GetSize();
    if (winSize[0] > 0 && winSize[1] > 0) {
        double minWinSize = (winSize[0] < winSize[1]) ? winSize[0] : winSize[1];
        cellPicker->SetTolerance(pixelsTarget / minWinSize);
    }

    bool wasTubeOn = m_ActorSketchDisplay->GetProperty()->GetRenderLinesAsTubes();
    if (wasTubeOn) {
        m_ActorSketchDisplay->GetProperty()->RenderLinesAsTubesOff();
    }

    vtkIdType cellId = -1;
    vtkActor* pickedActor = nullptr;

    // --- PRIORITÉ 1 : Tester les POINTS (les carrés) ---
    cellPicker->PickFromListOn();
    cellPicker->AddPickList(m_ActorPointsOfPrim);

    if (cellPicker->Pick(x, vtkY, 0, GetView()->getRenderer())) {
        pickedActor = cellPicker->GetActor();
        cellId = cellPicker->GetCellId();
    }


    gp_Pnt2d startPoint2D;
    gp_Pnt startPoint3D;
    if (!calculerIntersectionSourisSurPlan(screenX, screenY, startPoint2D, startPoint3D)){
        LOG_ERROR << " Vtk3d_Sketch::PickerGetPickedElement: calculerIntersectionSourisSurPlan pas d intersection " << std::endl;
        return l_result;
    }
    l_result.Clicked_Point2D = startPoint2D;
    l_result.Clicked_Point3D = startPoint3D;

    // --- PRIORITÉ 2 : Si on n'a pas touché de poignée, tester L'ESQUISSE GLOBALE (les lignes) ---
    if (!pickedActor) {
        cellPicker->PickFromListOff();
        cellPicker->AddPickList(m_ActorSketchDisplay);
        if (cellPicker->Pick(x, vtkY, 0, GetView()->getRenderer())) {
            pickedActor = cellPicker->GetActor();
            cellId = cellPicker->GetCellId();
        }
    }



    // CAS 1 : On a cliqué sur un point
    if (pickedActor && pickedActor == m_ActorPointsOfPrim) {
        vtkIdType VtkPointId = cellPicker->GetPointId();
        if (VtkPointId == -1) {
            LOG_ERROR << "Vtk3d_Sketch::PickerGetPickedElement -> VtkPointId == -1" << std::endl;
            return l_result;
        }
        auto mapper = vtkPolyDataMapper::SafeDownCast(m_ActorPointsOfPrim->GetMapper());
        auto polyData = mapper ? vtkPolyData::SafeDownCast(mapper->GetInput()) : nullptr;
        if (!polyData) {
            LOG_ERROR << "Vtk3d_Sketch::PickerGetPickedElement -> polyData est nullptr" << std::endl;
            return l_result;
        }
        auto edgeIdArray = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("OpenCascadeEdgeID"));

        int edgeId = edgeIdArray->GetValue(VtkPointId);
        l_result.id = edgeId;
        l_result.type = PickResult::TargetType::Point;
        LOG_INFO << " Vtk3d_Sketch::PickerGetPickedElement: Clic point id=" << l_result.id << std::endl;
    }


    // CAS 2 : On a cliqué DIRECTEMENT SUR LA LIGNE
    else if (pickedActor && pickedActor == m_ActorSketchDisplay) {
        vtkPolyData* polyData = vtkPolyData::SafeDownCast(m_ActorSketchDisplay->GetMapper()->GetInput());

        if (polyData && cellId != -1) {
            auto* edgeIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeEdgeID"));
            l_result.sourcePolyData = polyData;

            if (!edgeIdsArray || cellId >= edgeIdsArray->GetNumberOfValues()) {
                LOG_ERROR << " Vtk3d_Sketch::PickerGetPickedElement:  cellId invalide ou edgeIdsArray null " << std::endl;
                return l_result; // Ou return; selon la fonction
            }
            l_result.id = edgeIdsArray->GetValue(cellId);
            l_result.type = PickResult::TargetType::Primitive;
            LOG_INFO << " Vtk3d_Sketch::PickerGetPickedElement: Clic ligne id=" << l_result.id << std::endl;
        }
    }
    else {
        LOG_INFO << " Vtk3d_Sketch::PickerGetPickedElement: Clic vide " << std::endl;
        l_result.type = PickResult::TargetType::None;
    }
    return l_result;

}



//-----------------------------------------------------------------------
//      Traiter la commande envoyée par l'IHM
//-----------------------------------------------------------------------
void Vtk3d_Sketch::CADEvent_TraiterCommande(const CadCommandEvent& event) {
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdActivateTool>(&event.params)  ) {
        sketch_ActivateTool (  cmd->toolMode , cmd->sub_mode );
    }

/*
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdConstraints>(&event.params)  ) {
        switch ( cmd->cmd ){
            case CadEvent::Sketch::Constraints::Constraint_Resolve:
                if ( nullptr != PartRefs.GetOperation() ){
                    PartRefs.GetOperation()->setLocaleTopoChanged(true);
                    auto *SketchParam = std::get_if<SketchParams> (&PartRefs.GetOperation()->getParamsMutable() );
                    if ( nullptr != SketchParam ){

                        //Solver2D_Mapper::PrepareAndSolve(*SketchParam);
                        SolveEsquisse ();
                        rafraichirAffichageEsquisse();
                    }

                }
                break;

            case CadEvent::Sketch::Constraints::Set_Vertical:
                LOG_ERROR << "Vtk3d_Sketch::CADEvent_TraiterCommande : vertical !" << std::endl;
                if ( nullptr == PartRefs.GetOperation() ){
                    LOG_ERROR << "Vtk3d_Sketch::CADEvent_TraiterCommande :  nullptr == m_Operation " << std::endl;
                }
                std::visit([event](auto& activeTool) {
                    activeTool.CADEvent_TraiterCommande(event);
                }, m_tool);
                LOG_ERROR << "fdf" << std::endl;
                break;

            default:
                LOG_ERROR << "Vtk3d_Sketch::CADEvent_TraiterCommande : default !" << std::endl;
                break;
        }
        return;
    }
*/

}

//-----------------------------------------------------------------------
//      envoyer le retour vers l'ihm QT
//-----------------------------------------------------------------------
void Vtk3d_Sketch::CADEvent_RemonterEvent(const CadResponseEvent& event){
    // 2. Transmet à la vue principale si branchée
    if (m_view) {
        m_view->CADEvent_RemonterEvent(event);
    }
}


void Vtk3d_Sketch::Signaler_ActivationModePart (){
    CADEvent_RemonterEvent(CadResponseEvent{
        0,
        CadEvent::Part::RespGeneralSignal{
            CadEvent::Part::GeneralMessage::Activated
        }
    });
}
void Vtk3d_Sketch::Signaler_ActivationModeEsquisse (){
    CADEvent_RemonterEvent(CadResponseEvent{
        0,
        CadEvent::Sketch::RespGeneralSignal{
            CadEvent::Sketch::GeneralMessage::SketchActivated
        }
    });
}
void Vtk3d_Sketch::Signaler_ChangementEsquisseIHM (){
    CADEvent_RemonterEvent(CadResponseEvent{
        0,
        CadEvent::Sketch::RespGeneralSignal{
            CadEvent::Sketch::GeneralMessage::SketchChanged
        }
    });
}


void Vtk3d_Sketch::Signaler_Selection ( std::string li_string ){
    CADEvent_RemonterEvent(CadResponseEvent{
        0,
        CadEvent::Sketch::RespSelection{
            li_string
        }
    });
}







