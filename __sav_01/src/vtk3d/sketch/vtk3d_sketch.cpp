
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
#include "2DSolver_Mapper.h"


Vtk3d_Sketch::Vtk3d_Sketch(vtk3d_MainView* view, CadOperation* li_ptr_Operation)
    : AbstractViewMode(view),
    m_tool(Tool_Select(this))
{
    gp_Pnt origine(0.0, 0.0, 0.0);
    gp_Dir normale(0.0, 0.0, 1.0); // Plein axe Z
    gp_Dir axeX(1.0, 0.0, 0.0);    // Plein axe X

    gp_Ax3 planXY(origine, normale, axeX);

    m_sketchPlane = planXY;


    gp_Ax1 axeDeRotation(origine, axeX);
    double angleRadians = 45.0 * (M_PI / 180.0);
    m_sketchPlane = planXY.Rotated(axeDeRotation, angleRadians);


    if (nullptr == li_ptr_Operation) {
        m_Operation = new CadOperation("Esquisse test", SketchParams());
    } else {
        m_Operation = li_ptr_Operation;
    }


    auto* variantParams = &(m_Operation->getParamsMutable());
    if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ){
        sketchParams->m_sketchPlane = m_sketchPlane;
    }else{
        std::cout << "ERROR if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ) " << std::endl;
        std::cerr << "ERROR if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ) " << std::endl;
        assert(false && "CRITICAL: ERROR if ( auto* sketchParams = std::get_if<SketchParams>(variantParams) ).");
    }

    m_view = view;

    m_constraintManager = std::make_unique<SketchConstraintManager>(this);
    m_constraintManager->init();


    // ====================================================================
    // FIX INITIALISATION COORDONNÉES 3D
    // On force le calcul des coordonnées 3D du cache avant le premier rendu
    // ====================================================================
    if (nullptr != m_Operation) {
        m_Operation->setLocaleTopoChanged(true);
        if (auto* sketchParams = std::get_if<SketchParams>(&m_Operation->getParamsMutable())) {
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
        if (m_ActorSquareOfPrim) {
            m_view->getRenderer()->RemoveActor(m_ActorSquareOfPrim);
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



    gp_Pnt origine = m_sketchPlane.Location();
    gp_Dir normale = m_sketchPlane.Direction();
    gp_Dir vueHaut = m_sketchPlane.YDirection();

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
    auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
    interactor->SetInteractorStyle(imageStyle);

    //m_view->getRenderer()->ResetCamera(); //fait le reset de la caméra et donc perd l'angle ..

    if ( nullptr != m_Operation ){
        m_Operation->setLocaleTopoChanged(true);
        auto *SketchPara = std::get_if<SketchParams> (&m_Operation->getParamsMutable() );
        if ( nullptr != SketchPara ){
            SketchPara->recomputeGeometry3D();
        }

    }


    m_view->getRenderer()->ResetCameraClippingRange();
    interactor->GetRenderWindow()->Render();
}

void Vtk3d_Sketch::desactiver() {
    if (m_view ) {
        vtkCamera* camera = m_view->getRenderer()->GetActiveCamera();

        if (m_ActorSquareOfPrim) {
			m_ActorSquareOfPrim->SetVisibility(false);
		}
        if (m_constraintsDisplayActor) {
            m_constraintsDisplayActor->SetVisibility(false);
        }
        if ( nullptr != m_Operation ){
            auto *SketchPara = std::get_if<SketchParams> (&m_Operation->getParamsMutable() );
            if ( nullptr != SketchPara ){
                SketchPara->recomputeGeometry3D();
            }
            m_Operation->setLocaleTopoChanged(true);
        }
    }
}

void Vtk3d_Sketch::sketch_ActivateTool(SketchTool_mode li_tool) {
    if (m_mode != li_tool) {
        m_mode = li_tool;

        std::visit([](auto& activeTool) {
            activeTool.desactivate();
        }, m_tool);

        std::string l_string = "Outil : DEFAULT !! " ;

        switch(li_tool) {
            case SketchTool_mode::Tool_CircleDraw:
                m_tool = Tool_CircleDraw{this};
                l_string = "Outil : Tool_CircleDraw !! " ;
                break;
            case SketchTool_mode::Tool_Line:
                m_tool = Tool_LineDraw{this};
                l_string = "Outil : Tool_Line !! " ;
                break;
            case SketchTool_mode::Tool_RectCenterDraw:
                m_tool = Tool_RectCenterDraw{this};
                l_string = "Outil : Tool_RectCenterDraw !! " ;
                break;
            case SketchTool_mode::Tool_RectEdgesDraw:
                m_tool = Tool_RectEdgesDraw{this};
                l_string = "Outil : Tool_RectEdgesDraw !! " ;
                break;
            case SketchTool_mode::Tool_Select:
                m_tool = Tool_Select{this};
                l_string = "Outil : Tool_Select !! " ;
                break;
            case SketchTool_mode::Tool_Dimensions:
                m_tool = Tool_Dimensions{this};
                l_string = "Outil : Tool_Dimensions !! " ;
                break;
        }

        m_constraintManager->snapPointsVisited_Clean();




        CadResponseEvent resp;
        resp.documentId = 0; // id du doc
        resp.params = CadEvent::Sketch::RespStatus{
            l_string
        };
        CADEvent_RemonterEvent(resp);



        std::visit([](auto& activeTool) {
            activeTool.activate();
        }, m_tool);
    }
}





//-----------------------------------------------------------------------
//      Traiter la commande envoyée par l'IHM
//-----------------------------------------------------------------------
void Vtk3d_Sketch::CADEvent_TraiterCommande(const CadCommandEvent& event) {

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
        case CadEvent::Sketch::CadEvent_SketchConstraints::Constraint_Resolve:
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





    // 2. Transmet l'événement global à l'outil actif m_tool via std::visit
    // std::visit([&event](auto& activeTool) {
    //     activeTool.traiterCommande(event);
    // }, m_tool);
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




