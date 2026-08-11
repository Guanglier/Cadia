

#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolSelect.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkTextProperty.h>
#include <gp_Vec.hxx>
#include <iostream>
#include <cmath>

//#define gererMouseReleaseSketch_DBG
constexpr int TOLERANCE_CLIC = 10;

// --- Placeholders pour développements futurs ---

void Tool_Dimensions::activate( const CadEvent::Sketch::Tool_SubMode& submode ) {
    std::cout<<"Tool_Dimensions -> activate" <<std::endl;
    CotationStrRef.ref_defined = false;

    m_points->Reset();
    m_lines->Reset();
    m_polyData->AllocateExact(0, 0); // Vide le polydata proprement

    m_points->Initialize(); // Libère la mémoire des points
    m_lines->Initialize();  // Libère la mémoire des lignes
    m_polyData->Initialize();
    m_polyData->SetPoints(m_points);
    m_polyData->SetLines(m_lines);

    if (m_Parent && m_Parent->GetView() && m_Parent->GetView()->getRenderer()) {
        m_Parent->GetView()->renderWindow()->Render();
    }
    m_bDescriptorDefined = false;
}

void Tool_Dimensions::desactivate() {
    // TODO: Implémenter la désactivation de l'outil
    std::cout<<"Tool_Dimensions -> desactivate" <<std::endl;
}

bool Tool_Dimensions::keyPressEvent(QKeyEvent* event) {
    return false;
}

bool Tool_Dimensions::gererWheelEvent(QWheelEvent* event) {
    return false;
}


void Tool_Dimensions::Cotation_Configure(int li_PrimId, int li_PrimSubElmt) {
    // 1. Récupération des SketchParams mutables car on va vouloir y ajouter la cote plus tard
    OperationParams& param = m_Parent->PartRefs.GetOperation()->getParamsMutable();
    auto* sketchParams = std::get_if<SketchParams>(&param);
    if (!sketchParams) return;

    for (const auto& primitive : sketchParams->getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                if (concretePrim.id == li_PrimId) {

                    DimensionEngine::DistanceDescriptor  DistDesc;
                    // On initialise notre descripteur temporaire de l'outil
                    //istDesc. = 0; // Sera assigné par le registre lors du clic final
                    DistDesc.pntStart = sketchParams->GetPointById( concretePrim.startPointId).cache_p3d;
                    DistDesc.pntStop = sketchParams->GetPointById( concretePrim.stopPointId).cache_p3d;
                    DistDesc.targetPrimitiveId1 = concretePrim.id;
                    DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
                    DistDesc.offset = 0.0;
                    m_currentDimensionDescriptor.data = DistDesc;
                    m_currentDimensionDescriptor.id = 0;
                    m_bDescriptorDefined = true; // Flag à mettre dans votre Tool_Dimensions.h
                }
            }
            if constexpr (std::is_same_v <T,SketchCircle>){
                if (concretePrim.id == li_PrimId) {
                    DimensionEngine::RadialDescriptor  DistDescRadial;
                    DistDescRadial.targetPrimitiveId = li_PrimId;
                    DistDescRadial.center = sketchParams->GetPointById( concretePrim.centerPointId).cache_p3d;
                    DistDescRadial.diameter = concretePrim.radius;
                    DistDescRadial.mode = DimensionEngine::RadialMode::Diameter;
                    m_currentDimensionDescriptor.data = DistDescRadial;
                    m_currentDimensionDescriptor.id = 0;
                    m_bDescriptorDefined = true;

                    //calculer un vecteur depuis le centre vers la souris et de longueur radius, pour avoir la dimension affichée.
                    // n'est possible que pendant le move.
                    //DistDescRadial.anchorPoint = concretePrim.center;
                    //gp_Vec VectCenterToMouse ( concretePrim.center, )
                }

            }
        }, primitive);
    }
    m_Parent->m_Cotation->Afficher();
}

bool Tool_Dimensions::Cotation_MouseMove (gp_Pnt& PtnMouse3D){
    gp_Ax3 sketchPlane = m_Parent->PartRefs.GetSketchPlane();

    std::visit([&](auto& desc) {
        using ActualType = std::decay_t<decltype(desc)>;

        //- si on a une ligne
        if constexpr (std::is_same_v<ActualType, DimensionEngine::DistanceDescriptor>) {

            // 1. On récupère un pointeur direct sur la distance stockée dans le variant globale de l'outil
            auto* pDescDistance = std::get_if<DimensionEngine::DistanceDescriptor>(&m_currentDimensionDescriptor.data);
            if (!pDescDistance) return; // Sécurité si le variant ne contient pas une distance


            // 1. Détermination du mode à la volée (Horizontal, Vertical, ou PointToPoint) via le namespace
            bool isInPerpZone = DimensionEngine::IsMouseInPerpendiZone(
                pDescDistance->pntStart,
                pDescDistance->pntStop,
                PtnMouse3D
                );

            if (isInPerpZone) {
                pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
                gp_Vec AB(pDescDistance->pntStart, pDescDistance->pntStop);
                gp_Vec vecPerp = AB.Crossed(gp_Vec(sketchPlane.Direction())).Normalized();
                gp_Vec ASouris(pDescDistance->pntStart, PtnMouse3D);
                pDescDistance->offset = ASouris.Dot(vecPerp); // Calcul du décalage (offset)
            } else {
                gp_Vec ASouris(pDescDistance->pntStart, PtnMouse3D);
                gp_Vec BSouris(pDescDistance->pntStop, PtnMouse3D);
                gp_Vec directionXLocal(sketchPlane.XDirection());
                gp_Vec directionYLocal(sketchPlane.YDirection());

                double distASouris_X = ASouris.Dot(directionXLocal);
                double distBSouris_X = BSouris.Dot(directionXLocal);
                double distASouris_Y = ASouris.Dot(directionYLocal);
                double distBSouris_Y = BSouris.Dot(directionYLocal);

                if (distASouris_X * distBSouris_X < 0.0) {
                    pDescDistance->mode = DimensionEngine::DimMode::Horizontal;
                    pDescDistance->offset = ASouris.Dot(directionYLocal);
                } else if (distASouris_Y * distBSouris_Y < 0.0) {
                    pDescDistance->mode = DimensionEngine::DimMode::Vertical;
                    pDescDistance->offset = ASouris.Dot(directionXLocal);
                } else {
                    pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
                    gp_Vec AB(pDescDistance->pntStart, pDescDistance->pntStop);
                    gp_Vec vecPerp = AB.Crossed(gp_Vec(sketchPlane.Direction())).Normalized();
                    pDescDistance->offset = ASouris.Dot(vecPerp);
                }
            }

            m_currentDimensionDescriptor.data = *pDescDistance;

            // 2. Calcul des points de rendu 3D par le moteur (sans VTK)
            DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry(sketchPlane, m_currentDimensionDescriptor);

            // 3. Envoi du résultat brut à VTK pour l'affichage physique de l'aperçu
            m_Parent->m_Cotation->DessinerCotationDepuisResultat(sketchPlane, geoResult);

        // si on a un cercle
        }else if constexpr (std::is_same_v<ActualType, DimensionEngine::RadialDescriptor>) {
            auto* pDescRadial= std::get_if<DimensionEngine::RadialDescriptor>(&m_currentDimensionDescriptor.data);
            if (!pDescRadial) return; // Sécurité si le variant ne contient pas une distance
            pDescRadial->anchorPoint = PtnMouse3D;
            pDescRadial->offset = 0.0;
            pDescRadial->mode = DimensionEngine::RadialMode::Diameter;
            pDescRadial->pointerAngleRad = 0.0;

            DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry(sketchPlane, m_currentDimensionDescriptor);
            m_Parent->m_Cotation->DessinerCotationDepuisResultat(sketchPlane, geoResult);
        }

    }, m_currentDimensionDescriptor.data);

    return false;
}

bool Tool_Dimensions::gererMouseMove(QMouseEvent* event) {
    gp_Pnt2d currentPoint2D;

    //DimensionEngine::DistanceDescriptor   DescDistance;


    if (m_bDescriptorDefined) {
        gp_Pnt PtnMouse3D;
        if (!m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, PtnMouse3D)) {
            return false;
        }
        Cotation_MouseMove (PtnMouse3D);
        m_Parent->GetView()->renderWindow()->Render();
    }
    return false;
}




// --- Implémentations actives ---

bool Tool_Dimensions::gererMouseRelease(QMouseEvent* event) {
    b_IsSomethingSelected = false;
    m_b_MouseLIsPressed = false;
    m_SelectedPrimitiveId = -1;

    return false;
}

void Tool_Dimensions::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    //if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    //m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}





bool Tool_Dimensions::gererMousePress(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_MouseclickStartPosition = event->position().toPoint();
        m_b_MouseLIsPressed = true;


        double dpr = m_Parent->GetView()->devicePixelRatioF();
        int x = static_cast<int>(event->position().x() * dpr);
        int y = static_cast<int>(event->position().y() * dpr);
        int vtkY = (m_Parent->GetView()->height() * dpr) - y;

        // 1. Initialiser directement le CellPicker
        auto cellPicker = vtkSmartPointer<vtkCellPicker>::New();

        // 2. Calculer la tolérance dynamique (ex: un rayon de 15-20 pixels autour du clic)
        double pixelsTarget = 4.0;
        int* winSize = m_Parent->GetView()->getRenderer()->GetSize();
        if (winSize[0] > 0 && winSize[1] > 0) {
            double minWinSize = (winSize[0] < winSize[1]) ? winSize[0] : winSize[1];
            cellPicker->SetTolerance(pixelsTarget / minWinSize);
        }

        // 🧪 ASTUCE : Désactiver le mode "Tube" AVANT le Pick pour que les lignes fines profitent de la tolérance
        bool wasTubeOn = m_Parent->m_ActorSketchDisplay->GetProperty()->GetRenderLinesAsTubes();
        if (wasTubeOn) {
            m_Parent->m_ActorSketchDisplay->GetProperty()->RenderLinesAsTubesOff();
        }


        // 3. On lance le PICK UNIQUE avec le CellPicker tolérant
        vtkIdType cellId = -1;
        vtkActor* pickedActor = nullptr;

        // on va d'abord activer la liste de pick pour traiter les m_ActorPointsOfPrim seulement, et si rien
        // n'est trouvé alors on désactive la liste et on pick tout ce qu'on trouve. le pb était que
        // le pick prenait les lignes et non le m_ActorPointsOfPrim
        cellPicker->PickFromListOn();
        cellPicker->AddPickList(m_Parent->m_ActorPointsOfPrim); // On restreint UNIQUEMENT aux carrés

        if (cellPicker->Pick(x, vtkY, 0, m_Parent->GetView()->getRenderer())) {
            pickedActor = cellPicker->GetActor();
            cellId = cellPicker->GetCellId();
        }

        // 3b. Si aucun carré n'a été touché, on réessaie sur TOUTE la scène (pour trouver la ligne)
        if (!pickedActor) {
            //cellPicker->PickFromListOff(); // On réactive la recherche globale
            cellPicker->InitializePickList();
            if (m_Parent->m_ActorSketchDisplay) {
                cellPicker->AddPickList(m_Parent->m_ActorSketchDisplay);
            }
            cellPicker->PickFromListOn();
            if (cellPicker->Pick(x, vtkY, 0, m_Parent->GetView()->getRenderer())) {
                pickedActor = cellPicker->GetActor();
                cellId = cellPicker->GetCellId();
            }
        }
        // Rétablissement immédiat du rendu graphique
        if (wasTubeOn) {
            //           m_Parent->m_ActorSketchDisplay->GetProperty()->RenderLinesAsTubesOn();
        }

#ifdef gererMouseReleaseSketch_DBG
        std::cout << "\n[DEBUG CELL-PICKER] Objet touché : "
                  << (pickedActor ? pickedActor->GetClassName() : "Aucun Acteur")
                  << " | CellID: " << cellId << std::endl;
#endif

        if (pickedActor && pickedActor == m_Parent->m_ActorPointsOfPrim) {
            // ATTENTION : C'est ici que réside l'astuce !
            // Au lieu de demander le PointId de l'acteur, on demande le PointId du GLYPHE d'origine.
            // VTK remonte automatiquement l'arbre du pipeline.
            vtkIdType originalPointId = cellPicker->GetPointId();

            if (originalPointId != -1) {
                // 3. On récupère les données associées au point d'origine
                // (Le mapper ou l'actor fournit le polydata final qui contient la structure du Glyph)
                auto mapper = vtkPolyDataMapper::SafeDownCast(m_Parent->m_ActorPointsOfPrim->GetMapper());
                auto polyData = vtkPolyData::SafeDownCast(mapper->GetInput());

                if (polyData) {
                    auto edgeIdArray = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("OpenCascadeEdgeID"));
                    //auto typeArray   = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("ArrayTypeHandle"));

                    //if (edgeIdArray && typeArray) {
                    if (edgeIdArray ) {
                        int edgeId = edgeIdArray->GetValue(originalPointId);
                        //int handleType = typeArray->GetValue(originalPointId);

                        // Reçu 5/5 ! Tu sais sur quelle ligne et quelle extrémité tu as cliqué.
                        //std::cout << "Primitive ID: " << edgeId << ", Type: " << handleType << std::endl;
                        std::cout << "Primitive ID: " << edgeId  << std::endl;
                        b_IsSomethingSelected = true;

                        //m_Parent->GetView()->m_Chighlighter->mettreEnSurbrillanceEdgeParId(polyData, cellId);
                    }

                }
            }
        }

        // 4. Vérification : Est-ce qu'on a touché notre esquisse active ?
        if (pickedActor && pickedActor == m_Parent->m_ActorSketchDisplay) {
            vtkPolyData* polyData = vtkPolyData::SafeDownCast(m_Parent->m_ActorSketchDisplay->GetMapper()->GetInput());

            if (polyData && cellId != -1) {
                auto* edgeIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeEdgeID"));

                if (edgeIdsArray && cellId < edgeIdsArray->GetNumberOfValues()) {
                    int primitiveId = edgeIdsArray->GetValue(cellId);

                    std::cout << "🎯 [Sketch Mode] Primitive sélectionnée ! ID unique CAO : " << primitiveId << std::endl;
                    m_Parent->GetView()->m_Chighlighter->mettreEnSurbrillanceEdgeParId(polyData, primitiveId);
                    b_IsSomethingSelected = true;

                    PrimitiveIsSelected = true;
                    SelectedPrimitiveId = primitiveId;
                    Cotation_Configure ( primitiveId, 0);
                }
            }
        } else {
            CotationStrRef.ref_defined = false;
            // Clic dans le vide ou sur un autre objet : On nettoie la sélection
            if ( false == b_IsSomethingSelected ){
#ifdef gererMouseReleaseSketch_DBG
                std::cout << "\tClic hors esquisse : Deselection" << std::endl;
#endif
                m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
                PrimitiveIsSelected = false;
            }
        }

        m_Parent->GetView()->renderWindow()->Render();
        return true;
    }


    return false;
}

bool Tool_Dimensions::gererkeyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        //std::cout << "Tool_Dimensions::gererkeyPressEven -> Touche [Suppr]" << std::endl;

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
        }
        event->accept();
        return true;
    }
    return false;
}


void Tool_Dimensions::CADEvent_TraiterCommande(const CadCommandEvent& event){

}







