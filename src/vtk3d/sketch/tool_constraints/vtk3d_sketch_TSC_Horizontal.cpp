

#include "vtk3d_sketch_TSC_Model.h"
#include "vtk3d_sketch_ToolSetConstraints.h"
#include "tool_constraints/vtk3d_sketch_TSC_Horizontal.h"





//---------------------------------------------------------
//
//---------------------------------------------------------
void ConstraintTool_Horizontal::popup_create()  {
    m_ToolHelper.title = "Contrainte Horizontale";
    m_ToolHelper.instructionText = "Veuillez cliquer sur une ligne.";
    m_ToolHelper.showButtonOk = true;
    m_ToolHelper.showButtonCancel = true;
    m_ToolHelper.showButtonReset = false;
    m_ToolHelper.isButtonOkEnabled = false;
    m_ToolHelper.isSelectionComplete = false;

    m_ToolHelper.champMultiple.clear();
    DialogSketchHelper::ChampInputSelection champFirstRef;
    champFirstRef.id = "premiere_entite";
    champFirstRef.title = "Ligne :";
    champFirstRef.IsOk = false;
    champFirstRef.field_text = "Ligne (sélectionner)";
    champFirstRef.b_IsFocus = true;      // Met le focus dessus
    champFirstRef.b_IsDisabled = false;  // Actif
    champFirstRef.b_IsValid = false;      // Valide au départ


    m_ToolHelper.champMultiple.push_back(champFirstRef);

    data.SelectedElementsList.clear();
    data.etat = state::Wait_Select_first;
}
//---------------------------------------------------------
//
//---------------------------------------------------------
void ConstraintTool_Horizontal::Resetall(){
    data.SelectedElementsList.clear();
    data.etat = state::Wait_Select_first;
    popup_create();
}
//---------------------------------------------------------
//
//---------------------------------------------------------
void ConstraintTool_Horizontal::OnBtnClicked ( CadEvent::Sketch::CmdPopupToolBtnClicked li_btn){
    std::string  l_string = "";

    switch ( li_btn )
    {
    case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Cancel:
        Resetall();
        m_Parent->popup_sendpopup ();
        break;
    case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_OK:{
        if ( data.etat != state::ReadyToValidate ){
            return;
        }
        AppliqueContrainte ();
        m_Parent->update_esquisse();
        Resetall();
        m_Parent->popup_sendpopup ();
    }
    break;

    case CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Reset:
        Resetall();
        m_Parent->popup_sendpopup ();
        break;
    }
}



//---------------------------------------------------------
//
//---------------------------------------------------------
bool ConstraintTool_Horizontal::OnSelectedElement (  PickResult_element li_Element  ){

    switch ( data.etat )
    {
    case state::Wait_Select_first:
        data.SelectedElementsList.emplace_back( li_Element );

        // Mise à jour du premier champ dans le helper
        if (auto* sel = std::get_if<DialogSketchHelper::ChampInputSelection>(&m_ToolHelper.champMultiple[0])) {
            sel->IsOk = true;
            sel->b_IsValid = true;
            sel->field_text = PickResult_ElementType_to_string(li_Element) + " id=" + QString::number(li_Element.Id);
            sel->b_IsFocus = false; // Retire le focus du premier
        }
        m_ToolHelper.isButtonOkEnabled = true;
        m_ToolHelper.instructionText = "Sélection complète. Cliquez sur OK.";
        m_ToolHelper.showButtonReset = true;
        data.etat = state::ReadyToValidate;
        break;
    case state::Wait_Select_second:
        break;
    case state::Wait_Enter_Value:
        break;
    case state::Wait_SelectSecond_or_EnterValue:
        break;
    case state::ReadyToValidate:
        break;

    }
    return true;
}


//---------------------------------------------------------
//
//---------------------------------------------------------
void ConstraintTool_Horizontal::AppliqueContrainte (){
    std::string  l_string = "";

    if (!data.sketchParams) return;
    PartSketchConstraint::SketchConstraint ConstHori;
    ConstHori.data = PartSketchConstraint::HorizontalConstraint{{
            data.u64_SketchId,
            (uint64_t) data.SelectedElementsList[0].Id,
            PartSketchConstraint::TargetType::Primitive,
            PartSketchConstraint::SubElement::Whole
        }
    };
    data.sketchParams->addConstraint(ConstHori, l_string);
}





