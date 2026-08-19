

#include "vtk3d_sketch_TSC_Model.h"
#include "vtk3d_sketch_ToolSetConstraints.h"
#include "tool_constraints/vtk3d_sketch_TSC_Distance.h"

//---------------------------------------------------------
//		
//---------------------------------------------------------
void ConstraintTool_Distance::popup_create()  {
    m_ToolHelper.title = "Contrainte Distance";
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
    champFirstRef.field_text = "Ligne ou point)";
    champFirstRef.b_IsFocus = true;      // Met le focus dessus
    champFirstRef.b_IsDisabled = false;  // Actif
    champFirstRef.b_IsValid = false;      // Valide au départ
    m_ToolHelper.champMultiple.push_back(champFirstRef);


    DialogSketchHelper::ChampInputSelection champSecondRef;
    champSecondRef.id = "seconde_entite";
    champSecondRef.title = "Ligne :";
    champSecondRef.IsOk = false;
    champSecondRef.field_text = "Ligne ou point)";
    champSecondRef.b_IsFocus = false;      // Met le focus dessus
    champSecondRef.b_IsDisabled = false;  // Actif
    champSecondRef.b_IsValid = false;      // Valide au départ
    m_ToolHelper.champMultiple.push_back(champSecondRef);


    DialogSketchHelper::ChampInputDouble champValue;
    champValue.id = "valeur";
    champValue.title = "Valeur :";
    champValue.b_IsFocus = false;      // Met le focus dessus
    champValue.b_IsDisabled = false;  // Actif
    champValue.b_IsValid = false;      // Valide au départ
    champValue.value = 0.0;
    m_ToolHelper.champMultiple.push_back(champValue);

    data.SelectedElementsList.clear();
    data.etat = state::Wait_Select_first;
}


float ConstraintTool_Distance::CalculeDistance ( ){
    if ( data.SelectedElementsList.size() == 2){
        gp_Pnt p1 = data.SelectedElementsList[0].Clicked_Point3D;
        gp_Pnt p2 = data.SelectedElementsList[1].Clicked_Point3D;
        gp_Vec v(p1, p2);
        return static_cast<float>(v.Magnitude());
    }else{
        return 10.0;
    }
}

//---------------------------------------------------------
//
//---------------------------------------------------------
bool ConstraintTool_Distance::OnSelectedElement (  PickResult_element li_Element  ){

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
        if (auto* sel = std::get_if<DialogSketchHelper::ChampInputSelection>(&m_ToolHelper.champMultiple[1])) {
            sel->b_IsFocus = true; // Retire le focus du premier
        }
        m_ToolHelper.instructionText = "Sélectionnez le second élément.";
        m_ToolHelper.showButtonReset = true;
        data.etat = state::Wait_Select_second;
        break;
    case state::Wait_Select_second:
        data.SelectedElementsList.emplace_back( li_Element );

        // Mise à jour du premier champ dans le helper
        if (auto* sel = std::get_if<DialogSketchHelper::ChampInputSelection>(&m_ToolHelper.champMultiple[1])) {
            sel->IsOk = true;
            sel->b_IsValid = true;
            sel->field_text = PickResult_ElementType_to_string(li_Element) + " id=" + QString::number(li_Element.Id);
            sel->b_IsFocus = false; // Retire le focus du premier
        }

        if (auto* sel = std::get_if<DialogSketchHelper::ChampInputDouble>(&m_ToolHelper.champMultiple[2])) {
            sel->value = CalculeDistance ();
            m_value  = sel->value;
            sel->b_IsFocus = true;
        }

        m_ToolHelper.isButtonOkEnabled = true;
        m_ToolHelper.instructionText = "Sélection complète. Cliquez sur OK.";
        m_ToolHelper.showButtonReset = true;
        data.etat = state::ReadyToValidate;
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
void ConstraintTool_Distance::AppliqueContrainte (){
    std::string  l_string = "";
    if ( data.SelectedElementsList.size() != 2 ){
        return;
    }

    if ( data.SelectedElementsList[0].type != PickResult_element::TargetType::Point  ||
         data.SelectedElementsList[1].type != PickResult_element::TargetType::Point){
        return;
    }

    if (!data.sketchParams) return;

    PartSketchConstraint::SketchConstraint ConstDist;

    ConstDist.data = PartSketchConstraint::DistanceConstraint{
        { data.u64_SketchId, static_cast<uint64_t> ( data.SelectedElementsList[0].Id) ,  PartSketchConstraint::TargetType::Point, PartSketchConstraint::SubElement::Whole },
        { data.u64_SketchId, static_cast<uint64_t> ( data.SelectedElementsList[1].Id) , PartSketchConstraint::TargetType::Point, PartSketchConstraint::SubElement::Whole },
        m_value,
        false
    };
    data.sketchParams->addConstraint(ConstDist, l_string);
}

void ConstraintTool_Distance::OnInputValueChanged(std::string string_id, double value){
    m_value = value;
    std::cout << "ConstraintTool_Distance::OnInputValueChanged -> " << string_id << " = " << value << std::endl;
}


