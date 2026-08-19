
#pragma once

#include "CAD_PartOp.h"
#include "Dialog_SketchHelper.h"
#include "Dialog_SketchHelper_Popup.h"
//#include "vtk3d_sketch.h"
#include "cad_events.h"
#include "vtk3d_sketch_picker.h"

struct Tool_SetConstraints;

struct SelectedElement {
    enum class TargetType { None, Point, Primitive };
    TargetType type = TargetType::None;
    int id = -1;             // ID du point ou ID de l'edge/primitive
    gp_Pnt2d Clicked_Point2D;
    gp_Pnt Clicked_Point3D;
};

class ConstraintToolBase {
public:
    virtual ~ConstraintToolBase() = default;
    ConstraintToolBase(Tool_SetConstraints* parent) : m_Parent(parent) {}

    // Méthodes virtuelles pures que chaque contrainte doit implémenter
    virtual void popup_create() = 0;
    //virtual void applyConstraint(PartSketchParams* sketchParams, std::string& errorStr) = 0;

    // Logique commune factorisée
    virtual void OnBtnClicked ( CadEvent::Sketch::CmdPopupToolBtnClicked li_btn ) = 0;
    virtual bool OnSelectedElement ( PickResult_element li_Element ) = 0;       //   retourne true si la sélection est correcte
    virtual void Resetall () = 0;
    virtual void AppliqueContrainte () = 0;
    virtual void OnInputValueChanged(std::string string_id, double value){}

    QString PickResult_ElementType_to_string ( PickResult_element li_Element );

    DialogSketchHelper::Helper& GetDialogHelper (){ return m_ToolHelper; }
    void SetSketchId (uint64_t li_id) { data.u64_SketchId = li_id; }
    void SetSketchParams ( SketchParams* li_SketchParamsPtr){ data.sketchParams = li_SketchParamsPtr; }

    protected:
    Tool_SetConstraints* m_Parent = nullptr;
    DialogSketchHelper::Helper m_ToolHelper;
    
    enum class state{
        Wait_Select_first,
        Wait_Select_second,
        Wait_SelectSecond_or_EnterValue,
        Wait_Enter_Value,
        ReadyToValidate
    };
    struct SelectionData {
        std::vector < PickResult_element > SelectedElementsList;
        double doubleValue = 0.0;
        state etat  = state::Wait_Select_first;
        SketchParams* sketchParams;
        uint64_t    u64_SketchId;
    } data;
};








/*
    // 1. On prépare la structure de données initiale (Helper)
    // m_ToolHelper.title = "Création de contrainte" + QString::fromStdString( CadEvent::Sketch::CadEvent_Sketch_ConstraintSubmode_To_String ( m_mode ) );
    // m_ToolHelper.instructionText = "Veuillez cliquer sur deux points ou sur une ligne.";
    // m_ToolHelper.isSelectionComplete = false;
    // m_ToolHelper.showButtonCancel = true;
    // m_ToolHelper.showButtonReset = false;
    // m_ToolHelper.showButtonOk = true;
    // m_ToolHelper.isButtonOkEnabled = false;


    switch ( m_mode ){
        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        case CadEvent::Sketch::ConstraintSubMode::Vertical:
            //m_ToolHelper.instructionText = "Veuillez cliquer sur une ligne.";
            break;
        case CadEvent::Sketch::ConstraintSubMode::Parallel:
        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
            //m_ToolHelper.instructionText = "Veuillez cliquer sur deux lignes.";
            break;

        case CadEvent::Sketch::ConstraintSubMode::Distance:
            //m_ToolHelper.instructionText = "Veuillez cliquer sur deux points.";
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

    //m_ToolHelper.champMultiple.push_back(champFirstRef);


    DialogSketchHelper::ChampInputSelection SecondChamp;
    // SECOND champ
    switch ( m_mode ){
        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        case CadEvent::Sketch::ConstraintSubMode::Vertical:
            break;
        case CadEvent::Sketch::ConstraintSubMode::Parallel:
        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
            SecondChamp.id = "sel_point";
            SecondChamp.title = "Ligne";
            SecondChamp.IsOk = false;          // Pas encore sélectionné
            SecondChamp.b_IsFocus = false;
            SecondChamp.b_IsDisabled = true;
            SecondChamp.b_IsValid = false;     // Invalide -> Affichera la bordure/croix rouge
            champFirstRef.field_text = "Ligne ou point (sélectionner)";
            //m_ToolHelper.champMultiple.push_back(SecondChamp);
            break;

        case CadEvent::Sketch::ConstraintSubMode::Distance:
            SecondChamp.id = "sel_point";
            SecondChamp.title = "Point ou ligne :";
            SecondChamp.IsOk = false;          // Pas encore sélectionné
            SecondChamp.b_IsFocus = false;
            SecondChamp.b_IsDisabled = true;
            SecondChamp.b_IsValid = false;     // Invalide -> Affichera la bordure/croix rouge
            champFirstRef.field_text = "Ligne ou point (sélectionner)";
            //m_ToolHelper.champMultiple.push_back(SecondChamp);
            break;
    };


    // 3e champ
    DialogSketchHelper::ChampInputDouble TroisiemeChampDistance;

    switch ( m_mode ){
        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
        case CadEvent::Sketch::ConstraintSubMode::Vertical:
        case CadEvent::Sketch::ConstraintSubMode::Parallel:
        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
            break;

        case CadEvent::Sketch::ConstraintSubMode::Distance:
            TroisiemeChampDistance.id = "sel_value";
            TroisiemeChampDistance.title = "Valeur :";
            TroisiemeChampDistance.b_IsFocus = false;
            TroisiemeChampDistance.b_IsDisabled = true;
            TroisiemeChampDistance.b_IsValid = true;
            TroisiemeChampDistance.value = 15.0;
            m_ToolHelper.champMultiple.push_back(TroisiemeChampDistance);
            break;
    };
*/