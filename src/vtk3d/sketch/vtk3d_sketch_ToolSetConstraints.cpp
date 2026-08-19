
#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolSetConstraints.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include "Logger.h"

#include "tool_constraints/vtk3d_sketch_TSC_Horizontal.h"
#include "tool_constraints/vtk3d_sketch_TSC_Vertical.h"
#include "tool_constraints/vtk3d_sketch_TSC_Distance.h"


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


    // --- FABRIQUE D'OUTILS ---
    switch (m_mode) {
    default:
    case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        m_activeConstraintTool = std::make_unique<ConstraintTool_Horizontal>(this);
        m_activeConstraintTool->SetSketchId( m_Parent->PartRefs.GetSketchId() ) ;
        m_activeConstraintTool->SetSketchParams( m_Parent->PartRefs.GetParams() );
        break;
    case CadEvent::Sketch::ConstraintSubMode::Vertical:
        m_activeConstraintTool = std::make_unique<ConstraintTool_Vertical>(this);
        m_activeConstraintTool->SetSketchId( m_Parent->PartRefs.GetSketchId() ) ;
        m_activeConstraintTool->SetSketchParams( m_Parent->PartRefs.GetParams() );
        break;
    case CadEvent::Sketch::ConstraintSubMode::Distance:
        m_activeConstraintTool = std::make_unique<ConstraintTool_Distance>(this);
        m_activeConstraintTool->SetSketchId( m_Parent->PartRefs.GetSketchId() ) ;
        m_activeConstraintTool->SetSketchParams( m_Parent->PartRefs.GetParams() );
        break;
    }



    m_activeConstraintTool->popup_create();
    //popup_create();
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

    return false;
}




bool Tool_SetConstraints::gererMousePress(QMouseEvent* event) {
    PickResult  l_PickerResult;
    bool        lb_SelectionValide = false;

    if (event->button() != Qt::LeftButton) return false;
    if (data.select_state >= 2) return false;

    l_PickerResult = m_Parent->PickerGetPickedElement(event->position().x(), event->position().y() );
    lb_SelectionValide = m_activeConstraintTool->OnSelectedElement( l_PickerResult.Element );

    if (nullptr != l_PickerResult.sourcePolyData && true == lb_SelectionValide){
        m_Parent->GetView()->m_Chighlighter->mettreEnSurbrillanceEdgeParId(l_PickerResult.sourcePolyData, l_PickerResult.Element.Id);
    }

    popup_sendpopup ();
/*
    switch ( l_PickerResult.type  )
    {
        default:
            break;
        case PickResult::TargetType::Point:{
            LOG_ERROR << " Tool_Select::gererMousePress: Primm = POINT id=" << l_PickerResult.id << std::endl;
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
*/

    m_Parent->GetView()->renderWindow()->Render();
    return true;

}

bool Tool_SetConstraints::gererkeyPressEvent(QKeyEvent* event) {
    //event->accept();

    return false;
}


void Tool_SetConstraints::CADEvent_TraiterCommande(const CadCommandEvent& event){
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdPopupTool>(&event.params)  ) {
        //popup_StateMachineOnBtnClicked ( cmd->btn);
        m_activeConstraintTool->OnBtnClicked( cmd->btn );
    }
    if (auto* cmd = std::get_if<CadEvent::Sketch::CmdPopupTool_Valuechanged>(&event.params)  ) {
        //popup_StateMachineOnBtnClicked ( cmd->btn);
        m_activeConstraintTool->OnInputValueChanged( cmd->String_Id, cmd->value );
    }
}


void Tool_SetConstraints::popup_sendpopup(){
    if ( nullptr != m_activeConstraintTool ){
        CadResponseEvent    l_event;
        l_event.PartId = 0;
        l_event.params = CadEvent::Sketch::RespSendPopupDef { m_activeConstraintTool->GetDialogHelper() };
        m_Parent->CADEvent_RemonterEvent (l_event);
    }
}


/*
void Tool_SetConstraints::popup_StateMachineOnBtnClicked ( CadEvent::Sketch::CmdPopupToolBtnClicked li_btn){
    std::string  l_string = "";

    switch ( li_btn ){
        case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Cancel:
            //std::cout<< "Tool_SetConstraints::CADEvent_TraiterCommande : Btn_Cancel" << std::endl;
            break;
        case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_OK:
            //std::cout<< "Tool_SetConstraints::CADEvent_TraiterCommande : Btn_OK" << std::endl;
            if ( 2 == data.select_state )
            {

                auto* sketchParams = m_Parent->PartRefs.GetParams();
                if (!sketchParams) return;
                switch ( m_mode ){
                case CadEvent::Sketch::ConstraintSubMode::Horizontal:{
                    PartSketchConstraint::SketchConstraint ConstHori;
                    ConstHori.data = PartSketchConstraint::HorizontalConstraint{{
                            m_Parent->PartRefs.GetSketchId(),           // operationId (TEST BUG ATTENTION)
                            (uint64_t) data.first_element.m_SelectedPrimitiveId,
                            PartSketchConstraint::SubElement::Whole
                        }
                    };
                    sketchParams->addConstraint(ConstHori, l_string);
                    update_esquisse ();
                    resetSelection();
                    break;
                };
                case CadEvent::Sketch::ConstraintSubMode::Vertical:{
                    PartSketchConstraint::SketchConstraint vert1;
                    vert1.data = PartSketchConstraint::VerticalConstraint{{
                            m_Parent->PartRefs.GetSketchId(),           // operationId (TEST BUG ATTENTION)
                            (uint64_t) data.first_element.m_SelectedPrimitiveId,
                            PartSketchConstraint::SubElement::Whole
                        }
                    };
                    sketchParams->addConstraint(vert1, l_string);
                    update_esquisse ();
                    resetSelection();
                    break;
                };
                case CadEvent::Sketch::ConstraintSubMode::Parallel:
                case CadEvent::Sketch::ConstraintSubMode::Perpendicular:

                    break;

                case CadEvent::Sketch::ConstraintSubMode::Distance:

                    break;
                };


            }
            break;
        case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Reset:
            //std::cout<< "Tool_SetConstraints::CADEvent_TraiterCommande : Btn_Reset" << std::endl;
            resetSelection();
            break;
    }
}
*/


void Tool_SetConstraints::update_esquisse () {
    auto* sketchParams = m_Parent->PartRefs.GetParams();
    if (!sketchParams){
        LOG_INFO << "Tool_SetConstraints::popup_StateMachineOnBtnClicked !sketchParams" << std::endl;
        return;
    }else{
        m_Parent->m_SolverSession.Initialize(*sketchParams);
        m_Parent->m_SolverSession.Step(*sketchParams);
        m_Parent->GetView()->m_Chighlighter->masquerSurbrillance();
        m_Parent->rafraichirAffichageEsquisse();
        m_Parent->Signaler_ChangementEsquisseIHM ();
        LOG_INFO << "OK Tool_SetConstraints::popup_StateMachineOnBtnClicked" << std::endl;
    }
}

/*
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
                    sel2->field_text = "Ligne ou point";
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
        data.select_state = 2;

        if ( m_mode == CadEvent::Sketch::ConstraintSubMode::Distance) {
            if (m_ToolHelper.champMultiple.size() > 1) {
                if (auto* sel3 = std::get_if<DialogSketchHelper::ChampInputDouble>(&m_ToolHelper.champMultiple[2])) {
                    sel3->b_IsValid = true;
                    sel3->b_IsFocus = true;
                    sel3->value = 123.56;
                    sel3->b_IsDisabled = false;
                }
            }
        }else{
            // Tout est complet, on active le bouton OK
            m_ToolHelper.isButtonOkEnabled = true;
            m_ToolHelper.instructionText = "Sélection complète. Cliquez sur OK.";
        }
        popup_sendpopup();
    }
    break;

    case 2:
        break;

    default:
        break;
    }


}
*/







