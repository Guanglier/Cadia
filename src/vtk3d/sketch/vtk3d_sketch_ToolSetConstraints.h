#pragma once


#include <QMouseEvent>
#include "cad_events.h"
#include "Dialog_SketchHelper_Popup.h"
#include "cad_events.h"
//#include "vtk3d_sketch.h"









class Vtk3d_Sketch;

struct Tool_SetConstraints  {

    Tool_SetConstraints(Vtk3d_Sketch* parent)  {
        m_Parent = parent;
    }


    CadEvent::Sketch::ConstraintSubMode       m_mode = CadEvent::Sketch::ConstraintSubMode::Horizontal;

    void activate( const CadEvent::Sketch::Tool_SubMode& submode );
    void desactivate();

    bool keyPressEvent(QKeyEvent* event) ;
    bool gererWheelEvent(QWheelEvent* event) ;
    bool gererMouseMove(QMouseEvent* event) ;
    bool gererMouseRelease(QMouseEvent* event) ;
    bool gererMousePress(QMouseEvent* event) ;
    bool gererkeyPressEvent(QKeyEvent* event) ;

    void ajusterEchelleElements( double li_echelle);
    void CADEvent_TraiterCommande(const CadCommandEvent& event);
    void resetSelection ();
    void update_esquisse ();

    //bool    m_b_MouseLIsPressed = false;
    //QPoint     m_MouseclickStartPosition;


    struct{
        int select_state = 0;
        struct{
            int     m_SelectedPrimitiveId = -1;
            bool    PrimitiveIsSelected = false;
        }first_element;
        struct{
            int     m_SelectedPrimitiveId = -1;
            bool    PrimitiveIsSelected = false;
        }second_element;
        //PickResult  FirstElement;
        //PickResult  SecondElement;
    }data;





    Vtk3d_Sketch*   m_Parent = nullptr;


private:
    DialogSketchHelper::Helper m_ToolHelper;
    void popup_create ();
    void popup_sendpopup ();
    void popup_StateMachine ();
    void popup_StateMachine (int selectedId, const QString& typeName);
    void popup_StateMachineOnBtnClicked ( CadEvent::Sketch::CmdPopupToolBtnClicked li_btn);
};






