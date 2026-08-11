

#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolSelect.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include "2DSolver_Mapper.h"
#include "chrono.h"
#include "Logger.h"

#include <iostream>
#include <cmath>

//#define gererMouseReleaseSketch_DBG
constexpr int TOLERANCE_CLIC = 10;

#undef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL LogLevel::Debug

void Tool_Select::activate( const CadEvent::Sketch::Tool_SubMode& submode ) {
    // TODO: Implémenter l'activation de l'outil
}

void Tool_Select::desactivate() {
    // TODO: Implémenter la désactivation de l'outil
}

bool Tool_Select::keyPressEvent(QKeyEvent* event) {
    return false;
}

bool Tool_Select::gererWheelEvent(QWheelEvent* event) {
    return false;
}


bool Tool_Select::gererMouseMove(QMouseEvent* event) {
    if (!m_b_MouseLIsPressed || !DynamicDrag.m_isDragging) {
        return false;
    }
    //ScopedTimer timer("rafraichirAffichageEsquisseInteractif");

    if (true == PrimitiveIsSelected) {
        m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
        PrimitiveIsSelected = false;
    }

    // 1. Calculer la nouvelle position de la souris sur le plan d'esquisse
    gp_Pnt2d mousePoint2D;
    gp_Pnt mousePoint3D;
    if (!m_Parent->calculerIntersectionSourisSurPlan(event->position().x(), event->position().y(), mousePoint2D, mousePoint3D)) {
        return false;
    }

    auto* sketchParams = m_Parent->PartRefs.GetParams();
    if (!sketchParams){
        LOG_ERROR << "Tool_Select::gererMouseMove : nullptr == sketchParams" << std::endl;
        return false;
    }

    switch( DynamicDrag.m_mode )
    {
        case DragMode::CercleCentre:
        case DragMode::PointUnique:{

            if (nullptr != DynamicDrag.PtrSelectedPoint ){
                DynamicDrag.PtrSelectedPoint->p2d.SetX( mousePoint2D.X() );
                DynamicDrag.PtrSelectedPoint->p2d.SetY( mousePoint2D.Y() );
                DynamicDrag.PtrSelectedPoint->Update3D( m_Parent->PartRefs.GetSketchPlane());

                //uint64_t ptId = DynamicDrag.PtrSelectedPoint->id;

                m_Parent->m_SolverSession.UpdatePoint( DynamicDrag.PointDrag.IndexX );
                m_Parent->m_SolverSession.UpdatePoint( DynamicDrag.PointDrag.IndexY );

            }

            m_Parent->m_SolverSession.Step(*sketchParams);
            m_Parent->rafraichirPoignees(sketchParams);
            m_Parent->rafraichirAffichageEsquisseInteractif();
            //m_Parent->rafraichirAffichageEsquisse();      // trop lent remplacé par les deux du dessus
            }
            break;


        case DragMode::LigneComplete:{

            SketchPrimitive* prim = sketchParams->GetPrimitiveMutable( DynamicDrag.m_activePrimitiveId );
            if ( nullptr == prim ){
                LOG_ERROR << "Tool_Select::gererMouseMove : nullptr == prim" << std::endl;
                return false;
            }

            std::visit ([&](auto& curr_prim)
            {
                using T = std::decay_t<decltype(curr_prim)>;

                if constexpr ( std::is_same_v<T,SketchLine>){
                    gp_Pnt2d PntStart  = mousePoint2D.Translated(DynamicDrag.PrimToMoseVects.line.start );
                    gp_Pnt2d PntStop  = mousePoint2D.Translated(DynamicDrag.PrimToMoseVects.line.stop );
                    SketchPoint&  sp_start = sketchParams->GetPointById(curr_prim.startPointId);
                    SketchPoint&  sp_stop = sketchParams->GetPointById(curr_prim.stopPointId);
                    sp_start.setPoint ( PntStart, &m_Parent->PartRefs.GetSketchPlane() );
                    sp_stop.setPoint (PntStop, &m_Parent->PartRefs.GetSketchPlane() );

                    // On applique le delta sur tous les indices de la primitive entière
                    for (int idx : m_Parent->m_SolverSession.activeVarIndicesAll) {
                        m_Parent->m_SolverSession.UpdatePoint( idx );
                    }

                }else if constexpr ( std::is_same_v<T,SketchCircle>){
                    gp_Pnt2d PntCenter = mousePoint2D.Translated(DynamicDrag.PrimToMoseVects.circle.center);
                    //curr_prim.center.setPoint(PntCenter, &m_Parent->PartRefs.GetSketchPlane());
                    SketchPoint&  sp_center = sketchParams->GetPointById(curr_prim.centerPointId);
                    sp_center.setPoint(PntCenter, &m_Parent->PartRefs.GetSketchPlane() );

                    // On applique le delta sur tous les indices de la primitive entière
                    for (int idx : m_Parent->m_SolverSession.activeVarIndicesAll) {
                        m_Parent->m_SolverSession.UpdatePoint( idx );
                    }

                    //std::cout<<" -> " << PntCenter.X() << " " << PntCenter.Y() << " " << std::endl;
                }else{
                    LOG_ERROR << " Tool_Select::gererMouseMove : if constexpr ( std::is_same_v<T,SketchCircle> DEFAULT case" << std::endl;
                }

            }, *prim);

            // On met à jour la position de référence pour le prochain tick de souris
            DynamicDrag.m_lastMousePos2D = mousePoint2D;

            // On lance le solveur pour propoger/maintenir les contraintes si nécessaire, puis on rafraîchit
            m_Parent->m_SolverSession.Step(*sketchParams);

            m_Parent->rafraichirPoignees(sketchParams);
            m_Parent->rafraichirAffichageEsquisseInteractif();
            //m_Parent->rafraichirAffichageEsquisse();      // trop lent remplacé par les deux du dessus
            return true;
            }
            break;


        default:
            LOG_ERROR << "Tool_Select::gererMouseMove default !" << std::endl;
            break;

    }; // fin swtich
    return true;
}





// --- Implémentations actives ---

bool Tool_Select::gererMouseRelease(QMouseEvent* event) {
    if (DynamicDrag.m_isDragging) {
        DynamicDrag.m_isDragging = false;
        //DynamicDrag.m_activePointIndex = -1;

        // Résolution finale de clôture (OneShot propre pour éliminer toute dérive)
        auto* sketchParams = m_Parent->PartRefs.GetParams();
        if (sketchParams) {
            SolverOneShot::Solve(*sketchParams, false);
        }

        m_Parent->SolveEsquisse();
    }

    //b_IsSomethingSelected = false;
    m_b_MouseLIsPressed = false;

    return false;
}

void Tool_Select::ajusterEchelleElements( double li_echelle){
    //double facteurEchelle = li_echelle * 0.05;
    //if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    //m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}

bool Tool_Select::gererMousePress(QMouseEvent* event) {
    PickResult  l_PickerResult;

    if (event->button() == Qt::LeftButton) {

        m_b_MouseLIsPressed = true;

        // Au tout début de gererMousePress, avant de tester les picks :
        DynamicDrag.m_isDragging = false;
        DynamicDrag.m_mode = DragMode::None;
        DynamicDrag.m_activePrimitiveId = -1;

        // On remet aussi à zéro les variables du solveur de la session interactive
        m_Parent->m_SolverSession.activeVarIndexX = -1;
        m_Parent->m_SolverSession.activeVarIndexY = -1;
        m_Parent->m_SolverSession.activeVarIndicesAll.clear();

        l_PickerResult = m_Parent->PickerGetPickedElement(event->position().x(), event->position().y() );

        switch ( l_PickerResult.type  ){
        default:
            break;


        case PickResult::TargetType::Point:{
            DynamicDrag.m_activePrimitiveId = l_PickerResult.id;
            LOG_INFO << "Tool_Select::gererMousePress(QMouseEvent* event) -> point trouve  Id=" << DynamicDrag.m_activePrimitiveId << std::endl;
            auto* sketchParams = m_Parent->PartRefs.GetParams();
            if (!sketchParams) {
                LOG_ERROR << "Tool_Select::gererMousePress(QMouseEvent* event) -> if ( sketchParams ) " << std::endl;
                return false;
            }

            SketchPoint& sp = sketchParams->GetPointById(DynamicDrag.m_activePrimitiveId);
            DynamicDrag.PtrSelectedPoint = &sp;
            DynamicDrag.m_mode = DragMode::PointUnique;
            PrimitiveIsSelected = true;
            if( sp.b_Locked == false ){
                DynamicDrag.m_isDragging = true;
                LOG_INFO << "Tool_Select::gererMousePress(QMouseEvent* event) -> point NOT locked " << std::endl;
            }else{
                LOG_INFO << "Tool_Select::gererMousePress(QMouseEvent* event) -> point LOCKED Id=" << DynamicDrag.m_activePrimitiveId << std::endl;
            }

            m_Parent->m_SolverSession.Initialize(*sketchParams);
            m_Parent->Signaler_Selection( " Sélection de point" );
            SolverInteractiveSession::GetIndicesForHandle(*sketchParams, (uint64_t) DynamicDrag.m_activePrimitiveId, DynamicDrag.PointDrag.IndexX, DynamicDrag.PointDrag.IndexY );
            //subtool_Changesubtool ();
            //DialogSketchHelper::Helper  helpdlg = subtool_GetPopupDef();
            break;
        }


        case PickResult::TargetType::Primitive:{
            PrimitiveIsSelected = true;
            SelectedPrimitiveId = l_PickerResult.id;
            DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d = l_PickerResult.Clicked_Point2D;
            if (nullptr != l_PickerResult.sourcePolyData ){
                m_Parent->GetView()->m_Chighlighter->mettreEnSurbrillanceEdgeParId(l_PickerResult.sourcePolyData, SelectedPrimitiveId);
            }

            // Activer le drag complet de la ligne
            DynamicDrag.m_isDragging = true;
            DynamicDrag.m_mode = DragMode::LigneComplete; // Ligne entière
            DynamicDrag.m_activePrimitiveId = SelectedPrimitiveId;
            DynamicDrag.m_lastMousePos2D = l_PickerResult.Clicked_Point2D;

            //auto* sketchParams = std::get_if<SketchParams>(&m_Parent->PartRefs.GetOperation()->getParamsMutable());
            auto* sketchParams = m_Parent->PartRefs.GetParams();
            if ( nullptr == sketchParams) {
                LOG_ERROR << " Tool_Select::gererMousePress: sketchParams = nullptr " << std::endl;
                return false;
            }

            m_Parent->m_SolverSession.Initialize(*sketchParams);
            // Récupérer tous les indices de la ligne d'un coup
            SolverInteractiveSession::GetIndicesForEntireEdge(*sketchParams, DynamicDrag.m_activePrimitiveId, m_Parent->m_SolverSession.activeVarIndicesAll);

            // Remonter l'événement à l'IHM
            std::string l_string = "[Sketch Mode] Primitive sélectionnée ! ID unique CAO " + std::to_string(SelectedPrimitiveId);
            CadResponseEvent resp;
            resp.PartId = 0;
            resp.params = CadEvent::Sketch::RespStatus{ l_string };
            m_Parent->CADEvent_RemonterEvent(resp);


            SketchPrimitive *Primm = sketchParams->GetPrimitiveMutable(DynamicDrag.m_activePrimitiveId);
            if ( nullptr == Primm ){
                LOG_ERROR << " Tool_Select::gererMousePress: Primm = nullptr " << std::endl;
                return false;
            }


            std::visit ([&](auto& ConcretePrim) {
                using T = std::decay_t<decltype(ConcretePrim)>;
                if constexpr( std::is_same_v<T,SketchLine>){
                    if( ConcretePrim.b_Locked == false ){
                        DynamicDrag.m_isDragging = true;
                        DynamicDrag.PrimToMoseVects.line.start = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , sketchParams->GetPointById( ConcretePrim.startPointId).p2d );
                        DynamicDrag.PrimToMoseVects.line.stop = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , sketchParams->GetPointById( ConcretePrim.stopPointId).p2d );
                    }else{
                        DynamicDrag.m_isDragging = false;
                        LOG_INFO << "Tool_Select::gererMousePress(QMouseEvent* event) -> ligne LOCKED Id=" << DynamicDrag.m_activePrimitiveId << std::endl;
                    }

                }else if constexpr( std::is_same_v<T,SketchCircle>){
                    DynamicDrag.PrimToMoseVects.circle.center = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , sketchParams->GetPointById( ConcretePrim.centerPointId).p2d );
                }else if constexpr( std::is_same_v<T,SketchArc>){
                    LOG_ERROR << "456 " << std::endl;
                }else{
                    LOG_ERROR << "457 " << std::endl;
                }
            }, *Primm);

            m_Parent->Signaler_Selection( " Sélection de ligne" );

            break;
        }

        case PickResult::TargetType::None:{
            LOG_INFO << " Clic vide " << std::endl;
            DynamicDrag.PtrSelectedPoint = nullptr;
            if (true == PrimitiveIsSelected ) {
                m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
                PrimitiveIsSelected = false;
            }
            m_Parent->Signaler_Selection( "Dé-Sélection" );
            break;
        }

        };// fin switch
        m_Parent->GetView()->renderWindow()->Render();
        return true;
    }
    return false;

}




bool Tool_Select::gererkeyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        std::cout << "Tool_Select::gererkeyPressEven -> Touche [Suppr]" << std::endl;

        if (PrimitiveIsSelected) {
#ifdef DBG_keyPressEvent
            std::cout << "Suppression id: " << SelectedPrimitiveId << std::endl;
#endif
            PrimitiveIsSelected = false;

            auto* sketchParams = m_Parent->PartRefs.GetParams();
            if (!sketchParams) return false;

            sketchParams->removePrimitive(SelectedPrimitiveId);
            sketchParams->evaluate(*(m_Parent->GetView()->GetCurrentPart()));

            m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
            m_Parent->rafraichirAffichageEsquisse();
            m_Parent->GetView()->renderWindow()->Render();
            m_Parent->Signaler_ChangementEsquisseIHM ();

            m_Parent->m_SolverSession.Initialize(*sketchParams); // On reconstruit la map complète avec les 28 variables
            DynamicDrag.m_isDragging = false;                    // On coupe tout drag en cours
            PrimitiveIsSelected = false;                         // On désélectionne
            m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
        }
        event->accept();
        return true;
    }
    return false;
}


void Tool_Select::CADEvent_TraiterCommande(const CadCommandEvent& event){

    /*
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdConstraints>(&event.params)  ) {
        switch ( cmd->cmd ){
        case CadEvent::Sketch::Constraints::Constraint_Resolve:

            break;

        case CadEvent::Sketch::Constraints::Set_Vertical:
            LOG_ERROR << "Tool_Select::CADEvent_TraiterCommande : vertical !" << std::endl;
            if ( true == PrimitiveIsSelected ){
                auto* sketchParams = m_Parent->PartRefs.GetParams();
                if (!sketchParams) return;

                PartSketchConstraint::SketchConstraint vert1;
                vert1.data = PartSketchConstraint::VerticalConstraint{
                    {
                        m_Parent->PartRefs.GetSketchId(),           // operationId (TEST BUG ATTENTION)
                        (uint64_t) SelectedPrimitiveId,             // primitiveId
                        PartSketchConstraint::SubElement::Whole
                    }
                };
                sketchParams->addConstraint(vert1);
            }else{
                LOG_ERROR << "Tool_Select::CADEvent_TraiterCommande : default  !" << std::endl;
            }
            break;

        default:
            LOG_ERROR << "Tool_Select::CADEvent_TraiterCommande : default !" << std::endl;
            break;
        }
        return;
    }
    */
}




