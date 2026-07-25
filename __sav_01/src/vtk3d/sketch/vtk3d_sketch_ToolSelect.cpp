

#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolSelect.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>

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
    if ( true == m_b_MouseLIsPressed ){
        if ( true == b_IsSomethingSelected ){
            std::cout<<".";
        }
    }
    return false;
}

// --- Implémentations actives ---

bool Tool_Select::gererMouseRelease(QMouseEvent* event) {
    b_IsSomethingSelected = false;
    m_b_MouseLIsPressed = false;
    m_SelectedPrimitiveId = -1;





    return false;
}

void Tool_Select::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    //if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    //m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}


bool Tool_Select::gererMousePress(QMouseEvent* event) {
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

        // on va d'abord activer la liste de pick pour traiter les m_ActorSquareOfPrim seulement, et si rien
        // n'est trouvé alors on désactive la liste et on pick tout ce qu'on trouve. le pb était que
        // le pick prenait les lignes et non le m_ActorSquareOfPrim
        cellPicker->PickFromListOn();
        cellPicker->AddPickList(m_Parent->m_ActorSquareOfPrim); // On restreint UNIQUEMENT aux carrés

        if (cellPicker->Pick(x, vtkY, 0, m_Parent->GetView()->getRenderer())) {
            pickedActor = cellPicker->GetActor();
            cellId = cellPicker->GetCellId();
        }

        // 3b. Si aucun carré n'a été touché, on réessaie sur TOUTE la scène (pour trouver la ligne)
        if (!pickedActor) {
            cellPicker->PickFromListOff(); // On réactive la recherche globale
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

        if (pickedActor && pickedActor == m_Parent->m_ActorSquareOfPrim) {
            // ATTENTION : C'est ici que réside l'astuce !
            // Au lieu de demander le PointId de l'acteur, on demande le PointId du GLYPHE d'origine.
            // VTK remonte automatiquement l'arbre du pipeline.
            vtkIdType originalPointId = cellPicker->GetPointId();

            if (originalPointId != -1) {
                // 3. On récupère les données associées au point d'origine
                // (Le mapper ou l'actor fournit le polydata final qui contient la structure du Glyph)
                auto mapper = vtkPolyDataMapper::SafeDownCast(m_Parent->m_ActorSquareOfPrim->GetMapper());
                auto polyData = vtkPolyData::SafeDownCast(mapper->GetInput());

                if (polyData) {
                    auto edgeIdArray = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("OpenCascadeEdgeID"));
                    auto typeArray   = vtkIntArray::SafeDownCast(polyData->GetPointData()->GetArray("ArrayTypeHandle"));

                    if (edgeIdArray && typeArray) {
                        int edgeId = edgeIdArray->GetValue(originalPointId);
                        int handleType = typeArray->GetValue(originalPointId);

                        // Reçu 5/5 ! Tu sais sur quelle ligne et quelle extrémité tu as cliqué.
                        std::string l_string = "Primitive ID: " + std::to_string(edgeId) + ", Type: " + std::to_string(handleType);
                        //std::cout << "Primitive ID: " << edgeId << ", Type: " << handleType << std::endl;
                        std::cout << l_string << std::endl;
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

                    //envoyer message vers l'ihm
                    std::string l_string = "[Sketch Mode] Primitive sélectionnée ! ID unique CAO " + std::to_string(primitiveId) ;
                    CadResponseEvent resp;
                    resp.documentId = 0; // id du doc
                    resp.params = CadEvent::Sketch::RespStatus{
                        l_string
                    };
                    m_Parent->CADEvent_RemonterEvent(resp);


                }
            }
        } else {
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
        }
        event->accept();
        return true;
    }
    return false;
}


void Tool_Select::CADEvent_TraiterCommande(const CadCommandEvent& event){

}




