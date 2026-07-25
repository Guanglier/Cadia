

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

// --- Placeholders pour développements futurs ---

void Tool_Select::activate() {
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

    auto* sketchParams = std::get_if<SketchParams>(&m_Parent->m_Operation->getParamsMutable());
    if (!sketchParams){
        LOG_ERROR << "Tool_Select::gererMouseMove : nullptr == sketchParams" << std::endl;
        return false;
    }
    SketchPrimitive* prim = sketchParams->GetPrimitiveMutable( DynamicDrag.m_activePrimitiveId );
    if ( nullptr == prim ){
        LOG_ERROR << "Tool_Select::gererMouseMove : nullptr == prim" << std::endl;
        return false;
    }

    std::visit ([&](auto& curr_prim)
    {
        using T = std::decay_t<decltype(curr_prim)>;
        switch( DynamicDrag.m_mode )
        {
            case DragMode::CercleCentre:
            case DragMode::PointUnique:{
                if constexpr ( std::is_same_v<T,SketchLine>){
                    if (DynamicDrag.m_activeHandleType == 1) {
                        curr_prim.start.setPoint ( mousePoint2D, &m_Parent->m_sketchPlane );
                    } else if (DynamicDrag.m_activeHandleType == 2) {
                        curr_prim.stop.setPoint ( mousePoint2D, &m_Parent->m_sketchPlane );
                    }
                }else if constexpr ( std::is_same_v<T,SketchCircle>){
                    if (DynamicDrag.m_activeHandleType == 3) {
                        curr_prim.center.setPoint(mousePoint2D, &m_Parent->m_sketchPlane);
                    }else{
                        LOG_ERROR << " Tool_Select::gererMouseMove : if constexpr ( std::is_same_v<T,SketchCircle> DEFAULT case" << std::endl;
                    }
                }
                // On met à jour le solveur avec les nouvelles coordonnées X et Y de la souris
                 m_Parent->m_SolverSession.UpdatePoint(*sketchParams, m_Parent->m_SolverSession.activeVarIndexX );
                 m_Parent->m_SolverSession.UpdatePoint(*sketchParams, m_Parent->m_SolverSession.activeVarIndexY );

                m_Parent->m_SolverSession.Step(*sketchParams);
                //m_Parent->rafraichirPoignees(sketchParams);
                //m_Parent->rafraichirAffichageEsquisseInteractif();
                m_Parent->rafraichirAffichageEsquisse();      // trop lent remplacé par les deux du dessus
                break;
            }

            case DragMode::LigneComplete:{
                double deltaX = mousePoint2D.X() - DynamicDrag.m_lastMousePos2D.X();
                double deltaY = mousePoint2D.Y() - DynamicDrag.m_lastMousePos2D.Y();


                if constexpr ( std::is_same_v<T,SketchLine>){
                    gp_Pnt2d PntStart  = mousePoint2D.Translated(DynamicDrag.PrimToMoseVects.line.start );
                    gp_Pnt2d PntStop  = mousePoint2D.Translated(DynamicDrag.PrimToMoseVects.line.stop );
                    curr_prim.start.setPoint ( PntStart, &m_Parent->m_sketchPlane );
                    curr_prim.stop.setPoint (PntStop, &m_Parent->m_sketchPlane );

                    // On applique le delta sur tous les indices de la primitive entière
                    for (int idx : m_Parent->m_SolverSession.activeVarIndicesAll) {
                        m_Parent->m_SolverSession.UpdatePoint(*sketchParams, idx );
                    }

                }else if constexpr ( std::is_same_v<T,SketchCircle>){
                    gp_Pnt2d PntCenter = mousePoint2D.Translated(DynamicDrag.PrimToMoseVects.circle.center);
                    curr_prim.center.setPoint(PntCenter, &m_Parent->m_sketchPlane);

                    // On applique le delta sur tous les indices de la primitive entière
                    for (int idx : m_Parent->m_SolverSession.activeVarIndicesAll) {
                        m_Parent->m_SolverSession.UpdatePoint(*sketchParams, idx );
                    }

                    //std::cout<<" -> " << PntCenter.X() << " " << PntCenter.Y() << " " << std::endl;
                }else{
                    LOG_ERROR << " Tool_Select::gererMouseMove : if constexpr ( std::is_same_v<T,SketchCircle> DEFAULT case" << std::endl;
                }

                // On met à jour la position de référence pour le prochain tick de souris
                DynamicDrag.m_lastMousePos2D = mousePoint2D;

                // On lance le solveur pour propoger/maintenir les contraintes si nécessaire, puis on rafraîchit
                m_Parent->m_SolverSession.Step(*sketchParams);

                //m_Parent->rafraichirPoignees(sketchParams);
                //m_Parent->rafraichirAffichageEsquisseInteractif();
                m_Parent->rafraichirAffichageEsquisse();      // trop lent remplacé par les deux du dessus
                break;
            }

            default:
                LOG_ERROR << "Tool_Select::gererMouseMove default !" << std::endl;
                break;


        }; // fin swtich


    }, *prim);

    return true;
}



// --- Implémentations actives ---

bool Tool_Select::gererMouseRelease(QMouseEvent* event) {
    if (DynamicDrag.m_isDragging) {
        DynamicDrag.m_isDragging = false;
        DynamicDrag.m_activePointIndex = -1;

        // Résolution finale de clôture (OneShot propre pour éliminer toute dérive)
        auto* sketchParams = std::get_if<SketchParams>(&m_Parent->m_Operation->getParamsMutable());
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
    if (event->button() == Qt::LeftButton) {
        m_MouseclickStartPosition = event->position().toPoint();
        m_b_MouseLIsPressed = true;


        // Au tout début de gererMousePress, avant de tester les picks :
        DynamicDrag.m_isDragging = false;
        DynamicDrag.m_mode = DragMode::None;
        DynamicDrag.m_activePointIndex = -1;
        DynamicDrag.m_activePrimitiveId = -1;
        DynamicDrag.m_activeHandleType = -1;

        // On remet aussi à zéro les variables du solveur de la session interactive
        m_Parent->m_SolverSession.activeVarIndexX = -1;
        m_Parent->m_SolverSession.activeVarIndexY = -1;
        m_Parent->m_SolverSession.activeVarIndicesAll.clear();


        double dpr = m_Parent->GetView()->devicePixelRatioF();
        int x = static_cast<int>(event->position().x() * dpr);
        int y = static_cast<int>(event->position().y() * dpr);
        int vtkY = (m_Parent->GetView()->height() * dpr) - y;

        auto cellPicker = vtkSmartPointer<vtkCellPicker>::New();
        double pixelsTarget = 4.0;
        int* winSize = m_Parent->GetView()->getRenderer()->GetSize();
        if (winSize[0] > 0 && winSize[1] > 0) {
            double minWinSize = (winSize[0] < winSize[1]) ? winSize[0] : winSize[1];
            cellPicker->SetTolerance(pixelsTarget / minWinSize);
        }

        bool wasTubeOn = m_Parent->m_ActorSketchDisplay->GetProperty()->GetRenderLinesAsTubes();
        if (wasTubeOn) {
            m_Parent->m_ActorSketchDisplay->GetProperty()->RenderLinesAsTubesOff();
        }

        vtkIdType cellId = -1;
        vtkActor* pickedActor = nullptr;

        // --- PRIORITÉ 1 : Tester les POIGNÉES (les carrés) ---
        cellPicker->PickFromListOn();
        cellPicker->AddPickList(m_Parent->m_ActorSquareOfPrim);

        if (cellPicker->Pick(x, vtkY, 0, m_Parent->GetView()->getRenderer())) {
            pickedActor = cellPicker->GetActor();
            cellId = cellPicker->GetCellId();
        }

        // --- PRIORITÉ 2 : Si on n'a pas touché de poignée, tester L'ESQUISSE GLOBALE (les lignes) ---
        if (!pickedActor) {
            cellPicker->PickFromListOff();
            cellPicker->AddPickList(m_Parent->m_ActorSketchDisplay);
            if (cellPicker->Pick(x, vtkY, 0, m_Parent->GetView()->getRenderer())) {
                pickedActor = cellPicker->GetActor();
                cellId = cellPicker->GetCellId();
            }
        }


        // CAS 1 : On a cliqué sur une POIGNÉE (déplacement d'un point unique)
        if (pickedActor && pickedActor == m_Parent->m_ActorSquareOfPrim) {
            vtkIdType originalPointId = cellPicker->GetPointId();

            if (originalPointId != -1) {
                auto mapper = vtkPolyDataMapper::SafeDownCast(m_Parent->m_ActorSquareOfPrim->GetMapper());
                auto polyData = vtkPolyData::SafeDownCast(mapper->GetInput());

                if (polyData) {
                    auto edgeIdArray = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("OpenCascadeEdgeID"));
                    auto typeArray   = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("ArrayTypeHandle"));

                    if (edgeIdArray && typeArray) {
                        int edgeId = edgeIdArray->GetValue(originalPointId);
                        int handleType = typeArray->GetValue(originalPointId);

                        //b_IsSomethingSelected = true;
                        PrimitiveIsSelected = true;
                        DynamicDrag.m_isDragging = true;
                        DynamicDrag.m_mode = DragMode::PointUnique;
                        DynamicDrag.m_activePointIndex = originalPointId;
                        DynamicDrag.m_activePrimitiveId = edgeId;


                        //voir rafraichirPoignees donne le départ etc
                        // 1 = Point de départ de la ligne (StartPoint)
                        // 2 = Point d'arrivée de la ligne (EndPoint)
                        // 3 = Centre du cercle (CercleCentre)
                        DynamicDrag.m_activeHandleType = handleType;

                        auto* sketchParams = std::get_if<SketchParams>(&m_Parent->m_Operation->getParamsMutable());
                        if (sketchParams) {


                            SketchPrimitive *Primm = sketchParams->GetPrimitiveMutable(DynamicDrag.m_activePrimitiveId);
                            if ( Primm ){
                                std::visit ([&](auto& ConcretePrim) {
                                    using T = std::decay_t<decltype(ConcretePrim)>;
                                    if constexpr( std::is_same_v<T,SketchLine>){
                                        //DynamicDrag.Positions.start_when_clicked_2d = ConcretePrim.start.p2d;
                                        //DynamicDrag.Positions.stop_when_clicked_2d = ConcretePrim.stop.p2d;
                                        //DynamicDrag.PrimToMoseVects.line.start = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , ConcretePrim.start.p2d);
                                        //DynamicDrag.PrimToMoseVects.line.stop = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , ConcretePrim.stop.p2d );
                                    }else if constexpr( std::is_same_v<T,SketchCircle>){
                                        //DynamicDrag.m_mode = DragMode::CercleCentre;
                                        //DynamicDrag.Positions.center_when_clicked_2d = ConcretePrim.center.p2d;
                                        //DynamicDrag.PrimToMoseVects.circle.center = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , ConcretePrim.center.p2d);
                                    }else if constexpr( std::is_same_v<T,SketchArc>){
                                        LOG_ERROR << "Tool_Select::gererMousePress(QMouseEvent* event) -> std::visit ([&](auto& ConcretePrim)  non code " << std::endl;
                                    }else{
                                        LOG_ERROR << "Tool_Select::gererMousePress(QMouseEvent* event) -> std::visit ([&](auto& ConcretePrim)  non traité " << std::endl;
                                    }
                                }, *Primm);


                                m_Parent->m_SolverSession.Initialize(*sketchParams);
                                SolverInteractiveSession::GetIndicesForHandle(
                                    *sketchParams, DynamicDrag.m_activePrimitiveId, DynamicDrag.m_activeHandleType,
                                    m_Parent->m_SolverSession.activeVarIndexX,
                                    m_Parent->m_SolverSession.activeVarIndexY
                                    );
                            }
                        }

                    }
                }
            }
        }
        // CAS 2 : On a cliqué DIRECTEMENT SUR LA LIGNE (sélection + déplacement de toute la ligne)
        else if (pickedActor && pickedActor == m_Parent->m_ActorSketchDisplay) {
            vtkPolyData* polyData = vtkPolyData::SafeDownCast(m_Parent->m_ActorSketchDisplay->GetMapper()->GetInput());

            if (polyData && cellId != -1) {
                auto* edgeIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeEdgeID"));

                if (edgeIdsArray && cellId < edgeIdsArray->GetNumberOfValues())
                {
                    int primitiveId = edgeIdsArray->GetValue(cellId);

                    gp_Pnt2d startPoint2D;
                    gp_Pnt startPoint3D;
                    if (m_Parent->calculerIntersectionSourisSurPlan(event->position().x(), event->position().y(), startPoint2D, startPoint3D))
                    {
                        PrimitiveIsSelected = true;
                        SelectedPrimitiveId = primitiveId;

                        DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d = startPoint2D;

                        // Mettre en surbrillance l'edge (ton code d'origine)
                        m_Parent->GetView()->m_Chighlighter->mettreEnSurbrillanceEdgeParId(polyData, primitiveId);

                        // Activer le drag complet de la ligne
                        DynamicDrag.m_isDragging = true;
                        DynamicDrag.m_mode = DragMode::LigneComplete; // Ligne entière
                        DynamicDrag.m_activePrimitiveId = primitiveId;

                        DynamicDrag.m_lastMousePos2D = startPoint2D;

                        auto* sketchParams = std::get_if<SketchParams>(&m_Parent->m_Operation->getParamsMutable());
                        if (sketchParams) {
                            m_Parent->m_SolverSession.Initialize(*sketchParams);
                            // Récupérer tous les indices de la ligne d'un coup
                            SolverInteractiveSession::GetIndicesForEntireEdge(*sketchParams, DynamicDrag.m_activePrimitiveId, m_Parent->m_SolverSession.activeVarIndicesAll);

                            // Remonter l'événement à l'IHM
                            std::string l_string = "[Sketch Mode] Primitive sélectionnée ! ID unique CAO " + std::to_string(primitiveId);
                            CadResponseEvent resp;
                            resp.documentId = 0;
                            resp.params = CadEvent::Sketch::RespStatus{ l_string };
                            m_Parent->CADEvent_RemonterEvent(resp);


                            SketchPrimitive *Primm = sketchParams->GetPrimitiveMutable(DynamicDrag.m_activePrimitiveId);
                            if ( Primm ){
                                std::visit ([&](auto& ConcretePrim) {
                                    using T = std::decay_t<decltype(ConcretePrim)>;
                                    if constexpr( std::is_same_v<T,SketchLine>){
                                        DynamicDrag.PrimToMoseVects.line.start = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , ConcretePrim.start.p2d);
                                        DynamicDrag.PrimToMoseVects.line.stop = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , ConcretePrim.stop.p2d );
                                    }else if constexpr( std::is_same_v<T,SketchCircle>){
                                        DynamicDrag.PrimToMoseVects.circle.center = gp_Vec2d( DynamicDrag.PrimToMoseVects.mouse_when_clicked_2d , ConcretePrim.center.p2d);
                                    }else if constexpr( std::is_same_v<T,SketchArc>){
                                        LOG_ERROR << "456 " << std::endl;
                                    }else{
                                        LOG_ERROR << "457 " << std::endl;
                                    }
                                }, *Primm);

                            }

                        }

                    }else{
                        LOG_ERROR << " Tool_Select::gererMousePress: calculerIntersectionSourisSurPlan pas d intersection " << std::endl;
                    }
                }
            }
        }
        else {
            LOG_ERROR << " Clic vide " << std::endl;
            // Clic dans le vide : Désélection
            if (true == PrimitiveIsSelected ) {
                m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
                PrimitiveIsSelected = false;
            }
        }

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

            auto* sketchParams = std::get_if<SketchParams>(&m_Parent->m_Operation->getParamsMutable());
            if (!sketchParams) return false;

            sketchParams->removePrimitive(SelectedPrimitiveId);
            sketchParams->evaluate(*(m_Parent->GetView()->GetCurrentDoc()));

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

}




