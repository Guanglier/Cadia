#pragma once
#include "Dialog_SketchHelper_Popup.h"


struct Tool_Select;


enum class EtatTool {
    TSSD_Init,
    TSSD_WaitClic
};




struct ToolSelect_SetDimension{

public:

    void init( Tool_Select* parent);

    Tool_Select* m_Parent = nullptr;



    DialogSketchHelper::Helper& GetPopupDef (){
        return m_ToolHelper;
    }

    void OnAction ();

private:

    EtatTool    etat;
    DialogSketchHelper::Helper m_ToolHelper;
    void popup_create ();

};



























