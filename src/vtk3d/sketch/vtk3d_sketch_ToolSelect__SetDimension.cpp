#include "vtk3d_sketch_ToolSelect__SetDimension.h"





void ToolSelect_SetDimension::init( Tool_Select* parent){
    m_Parent = parent;
    etat = EtatTool::TSSD_Init;
    popup_create ();
}





void ToolSelect_SetDimension::OnAction(){

    switch( etat )
    {

    case EtatTool::TSSD_Init:
        popup_create ();
        etat = EtatTool::TSSD_WaitClic;
        break;

    case EtatTool::TSSD_WaitClic:
        break;
    }
}




void ToolSelect_SetDimension::popup_create(){


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

