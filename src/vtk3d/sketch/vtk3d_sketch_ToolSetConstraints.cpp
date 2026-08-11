
#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolSetConstraints.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include "Logger.h"



//#define gererMouseReleaseSketch_DBG
constexpr int TOLERANCE_CLIC = 10;

void Tool_SetConstraints::ajusterEchelleElements ( double li_echelle ){

}

void Tool_SetConstraints::activate() {
    // TODO: Implémenter l'activation de l'outil
}

void Tool_SetConstraints::desactivate() {
    // TODO: Implémenter la désactivation de l'outil
}

bool Tool_SetConstraints::keyPressEvent(QKeyEvent* event) {
    return false;
}

bool Tool_SetConstraints::gererWheelEvent(QWheelEvent* event) {
    return false;
}

bool Tool_SetConstraints::gererMouseMove(QMouseEvent* event) {
    if ( true == m_b_MouseLIsPressed ){
        if ( true == b_IsSomethingSelected ){
            std::cout<<".";
        }
    }
    return false;
}

// --- Implémentations actives ---

bool Tool_SetConstraints::gererMouseRelease(QMouseEvent* event) {
    b_IsSomethingSelected = false;
    m_b_MouseLIsPressed = false;
    m_SelectedPrimitiveId = -1;


    return false;
}




bool Tool_SetConstraints::gererMousePress(QMouseEvent* event) {
    PickResult  l_PickerResult;

    if (event->button() == Qt::LeftButton) {

        l_PickerResult = m_Parent->PickerGetPickedElement(event->position().x(), event->position().y() );

        switch ( l_PickerResult.type  )
        {
            default:
                break;
            case PickResult::TargetType::Point:{

                break;
            };


            case PickResult::TargetType::Primitive:{


                break;
            };

            case PickResult::TargetType::None:{
                LOG_INFO << " Clic vide " << std::endl;

                break;
            };

        };// fin switch


        m_MouseclickStartPosition = event->position().toPoint();
        m_b_MouseLIsPressed = true;


        m_Parent->GetView()->renderWindow()->Render();
        return true;
    }


    return false;
}

bool Tool_SetConstraints::gererkeyPressEvent(QKeyEvent* event) {
    //event->accept();


    return false;
}


void Tool_SetConstraints::CADEvent_TraiterCommande(const CadCommandEvent& event){

}




void Tool_SetConstraints::popup_create(){


    // 1. On prépare la structure de données initiale (Helper)
    m_ToolHelper.title = "Assistant dimensions";
    m_ToolHelper.instructionText = "Veuillez cliquer sur deux points ou sur une ligne.";
    m_ToolHelper.isSelectionComplete = false;
    m_ToolHelper.showButtonCancel = true;
    m_ToolHelper.showButtonReset = false;
    m_ToolHelper.showButtonOk = true;
    m_ToolHelper.isButtonOkEnabled = false;

    // Ajout d'un champ double (ex: distance)
    DialogSketchHelper::ChampInputSelection champFirstRef;
    champFirstRef.id = "premiere_entite";
    champFirstRef.title = "Point ou ligne :";
    champFirstRef.IsOk = false;
    champFirstRef.b_IsFocus = true;      // Met le focus dessus
    champFirstRef.b_IsDisabled = false;  // Actif
    champFirstRef.b_IsValid = false;      // Valide au départ
    m_ToolHelper.champMultiple.push_back(champFirstRef);

    // Ajout d'un champ de sélection (ex: point cible)
    DialogSketchHelper::ChampInputSelection champSel;
    champSel.id = "sel_point";
    champSel.title = "Point d'arrivée :";
    champSel.IsOk = false;          // Pas encore sélectionné
    champSel.b_IsFocus = false;
    champSel.b_IsDisabled = true;
    champSel.b_IsValid = false;     // Invalide -> Affichera la bordure/croix rouge
    m_ToolHelper.champMultiple.push_back(champSel);



}





