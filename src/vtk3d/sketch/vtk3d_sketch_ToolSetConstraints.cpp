
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

void Tool_SetConstraints::activate( const CadEvent::Sketch::Tool_SubMode& submode ) {

    if ( auto* constraintMode = std::get_if<CadEvent::Sketch::ConstraintSubMode>(&submode) ) {
        m_mode = *constraintMode; // On déréférence le pointeur pour assigner la valeur
    } else {
        LOG_ERROR << "Tool_Rectangle::activate getif submode invalide " << std::endl;
        m_mode = CadEvent::Sketch::ConstraintSubMode::Horizontal;
    }
    popup_create();
    popup_sendpopup ();
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
    //if ( true == m_b_MouseLIsPressed ){

    //}
    return false;
}

// --- Implémentations actives ---

bool Tool_SetConstraints::gererMouseRelease(QMouseEvent* event) {
    data.first_element.PrimitiveIsSelected = false;
    //m_b_MouseLIsPressed = false;
    data.first_element.m_SelectedPrimitiveId = -1;


    return false;
}




bool Tool_SetConstraints::gererMousePress(QMouseEvent* event) {
    PickResult  l_PickerResult;

    if (event->button() != Qt::LeftButton) return false;
    if (data.select_state >= 2) return false;

    l_PickerResult = m_Parent->PickerGetPickedElement(event->position().x(), event->position().y() );

    switch ( l_PickerResult.type  )
    {
        default:
            break;
        case PickResult::TargetType::Point:{
            break;
        };


        case PickResult::TargetType::Primitive:{
            if (nullptr != l_PickerResult.sourcePolyData ){
                m_Parent->GetView()->m_Chighlighter->mettreEnSurbrillanceEdgeParId(l_PickerResult.sourcePolyData, l_PickerResult.id);
            }
            auto* sketchParams = m_Parent->PartRefs.GetParams();
            if ( nullptr == sketchParams) {
                LOG_ERROR << " Tool_Select::gererMousePress: sketchParams = nullptr " << std::endl;
                return false;
            }

            // Remonter l'événement à l'IHM
            std::string l_string = "[Sketch Mode] Primitive sélectionnée ! ID unique CAO " + std::to_string(l_PickerResult.id);
            CadResponseEvent resp;
            resp.PartId = 0;
            resp.params = CadEvent::Sketch::RespStatus{ l_string };
            m_Parent->CADEvent_RemonterEvent(resp);


            SketchPrimitive *Primm = sketchParams->GetPrimitiveMutable( l_PickerResult.id );
            if ( nullptr == Primm ){
                LOG_ERROR << " Tool_Select::gererMousePress: Primm = nullptr " << std::endl;
                return false;
            }
            std::visit ([&](auto& ConcretePrim) {
                using T = std::decay_t<decltype(ConcretePrim)>;
                if constexpr( std::is_same_v<T,SketchLine>)
                {
                    m_Parent->Signaler_Selection( " Sélection de ligne" );

                    if( ConcretePrim.b_Locked == true ){
                        LOG_INFO << "Tool_Select::gererMousePress(QMouseEvent* event) -> ligne LOCKED Id=" << l_PickerResult.id << std::endl;
                        return;
                    }
                    popup_StateMachine(l_PickerResult.id, "Ligne");
                }else if constexpr( std::is_same_v<T,SketchCircle>){

                }else if constexpr( std::is_same_v<T,SketchArc>){
                    LOG_ERROR << "456 " << std::endl;
                }else{
                    LOG_ERROR << "457 " << std::endl;
                }
            }, *Primm);


            break;
        };

        case PickResult::TargetType::None:{
            LOG_INFO << " Clic vide " << std::endl;
            m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
            m_Parent->Signaler_Selection( "Dé-Sélection" );
            break;
        };

    };// fin switch


    //m_MouseclickStartPosition = event->position().toPoint();
    //m_b_MouseLIsPressed = true;


    m_Parent->GetView()->renderWindow()->Render();
    return true;

}

bool Tool_SetConstraints::gererkeyPressEvent(QKeyEvent* event) {
    //event->accept();

    return false;
}


void Tool_SetConstraints::CADEvent_TraiterCommande(const CadCommandEvent& event){
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdPopupTool>(&event.params)  ) {
        switch ( cmd->btn ){
        case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Cancel:
            break;
        case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_OK:
            break;
        case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Reset:
            resetSelection();
            break;
        }
    }

    //PartSketchConstraint::SketchConstraint
}

void Tool_SetConstraints::resetSelection() {
    data.select_state = 0;
    data.first_element.PrimitiveIsSelected = false;
    data.first_element.m_SelectedPrimitiveId = -1;
    data.second_element.PrimitiveIsSelected = false;
    data.second_element.m_SelectedPrimitiveId = -1;

    // On régénère le setup initial propre via popup_create()
    m_ToolHelper.champMultiple.clear();
    popup_create();
    m_ToolHelper.isButtonOkEnabled = false;
    popup_sendpopup();
}


void Tool_SetConstraints::popup_create(){


    // 1. On prépare la structure de données initiale (Helper)
    m_ToolHelper.title = "Création de contrainte" + QString::fromStdString( CadEvent::Sketch::CadEvent_Sketch_ConstraintSubmode_To_String ( m_mode ) );
    m_ToolHelper.instructionText = "Veuillez cliquer sur deux points ou sur une ligne.";
    m_ToolHelper.isSelectionComplete = false;
    m_ToolHelper.showButtonCancel = true;
    m_ToolHelper.showButtonReset = false;
    m_ToolHelper.showButtonOk = true;
    m_ToolHelper.isButtonOkEnabled = false;


    switch ( m_mode ){
        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        case CadEvent::Sketch::ConstraintSubMode::Vertical:
            m_ToolHelper.instructionText = "Veuillez cliquer sur une ligne.";
            break;
        case CadEvent::Sketch::ConstraintSubMode::Parallel:
        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
            m_ToolHelper.instructionText = "Veuillez cliquer sur deux lignes.";
            break;

        case CadEvent::Sketch::ConstraintSubMode::Distance:
            m_ToolHelper.instructionText = "Veuillez cliquer sur deux points.";
            break;
    };



    // PREMIER champ
    DialogSketchHelper::ChampInputSelection champFirstRef;
    champFirstRef.id = "premiere_entite";
    champFirstRef.title = " ??? ";
    champFirstRef.IsOk = false;
    champFirstRef.field_text = "Ligne ou point (sélectionner)";
    champFirstRef.b_IsFocus = true;      // Met le focus dessus
    champFirstRef.b_IsDisabled = false;  // Actif
    champFirstRef.b_IsValid = false;      // Valide au départ


    DialogSketchHelper::ChampInputSelection champSel;

    switch ( m_mode ){
        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        case CadEvent::Sketch::ConstraintSubMode::Vertical:
        case CadEvent::Sketch::ConstraintSubMode::Parallel:
        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
            champFirstRef.title = "Ligne :";
            break;

        case CadEvent::Sketch::ConstraintSubMode::Distance:
            champFirstRef.title = "Point ou ligne :";
            break;
    };

    m_ToolHelper.champMultiple.push_back(champFirstRef);



    // SECOND champ
    switch ( m_mode ){
        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        case CadEvent::Sketch::ConstraintSubMode::Vertical:
            break;
        case CadEvent::Sketch::ConstraintSubMode::Parallel:
        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
            champSel.id = "sel_point";
            champSel.title = "Ligne";
            champSel.IsOk = false;          // Pas encore sélectionné
            champSel.b_IsFocus = false;
            champSel.b_IsDisabled = true;
            champSel.b_IsValid = false;     // Invalide -> Affichera la bordure/croix rouge
            m_ToolHelper.champMultiple.push_back(champSel);
            break;

        case CadEvent::Sketch::ConstraintSubMode::Distance:
            champSel.id = "sel_point";
            champSel.title = "Point ou ligne :";
            champSel.IsOk = false;          // Pas encore sélectionné
            champSel.b_IsFocus = false;
            champSel.b_IsDisabled = true;
            champSel.b_IsValid = false;     // Invalide -> Affichera la bordure/croix rouge
            m_ToolHelper.champMultiple.push_back(champSel);
            break;
    };


}


void Tool_SetConstraints::popup_sendpopup(){
    CadResponseEvent    l_event;

    l_event.PartId = 0;
    l_event.params = CadEvent::Sketch::RespSendPopupDef { m_ToolHelper };

    m_Parent->CADEvent_RemonterEvent (l_event);
}



void Tool_SetConstraints::popup_StateMachine (int selectedId, const QString& typeName) {
    switch (data.select_state) {
    case 0: // --- ÉTAPE 1 : Sélection du premier élément ---
    {
        data.first_element.PrimitiveIsSelected = true;
        data.first_element.m_SelectedPrimitiveId = selectedId;

        // Mise à jour du premier champ dans le helper
        if (auto* sel = std::get_if<DialogSketchHelper::ChampInputSelection>(&m_ToolHelper.champMultiple[0])) {
            sel->IsOk = true;
            sel->b_IsValid = true;
            sel->field_text = typeName + " id=" + QString::number(selectedId);
            sel->b_IsFocus = false; // Retire le focus du premier
        }

        // Selon le mode, on regarde si on a besoin d'un second élément
        if (m_mode == CadEvent::Sketch::ConstraintSubMode::Parallel ||
            m_mode == CadEvent::Sketch::ConstraintSubMode::Perpendicular ||
            m_mode == CadEvent::Sketch::ConstraintSubMode::Distance) {

            // Passage à l'état 2 : Il faut un deuxième élément
            data.select_state = 1;

            // Activation du second champ s'il existe
            if (m_ToolHelper.champMultiple.size() > 1) {
                if (auto* sel2 = std::get_if<DialogSketchHelper::ChampInputSelection>(&m_ToolHelper.champMultiple[1])) {
                    sel2->b_IsDisabled = false; // On l'active
                    sel2->b_IsFocus = true;    // On met le focus dessus
                    sel2->field_text = "Sélectionner le 2e élément...";
                }
            }
            m_ToolHelper.instructionText = "Veuillez sélectionner le second élément.";
        }
        else {
            // Mode à un seul élément (ex: Horizontal, Vertical) : Tout est OK !
            data.select_state = 2; // État final / Prêt à valider
            m_ToolHelper.isButtonOkEnabled = true;
            m_ToolHelper.instructionText = "Sélection complète. Cliquez sur OK.";
        }

        m_ToolHelper.showButtonReset = true;
        popup_sendpopup();
    }
    break;

    case 1: // --- ÉTAPE 2 : Sélection du second élément (si nécessaire) ---
    {
        data.second_element.PrimitiveIsSelected = true;
        data.second_element.m_SelectedPrimitiveId = selectedId;

        if (m_ToolHelper.champMultiple.size() > 1) {
            if (auto* sel2 = std::get_if<DialogSketchHelper::ChampInputSelection>(&m_ToolHelper.champMultiple[1])) {
                sel2->IsOk = true;
                sel2->b_IsValid = true;
                sel2->field_text = typeName + " id=" + QString::number(selectedId);
                sel2->b_IsFocus = false;
            }
        }

        // Tout est complet, on active le bouton OK
        data.select_state = 2;
        m_ToolHelper.isButtonOkEnabled = true;
        m_ToolHelper.instructionText = "Sélection complète. Cliquez sur OK.";
        popup_sendpopup();
    }
    break;

    default:
        break;
    }
}





