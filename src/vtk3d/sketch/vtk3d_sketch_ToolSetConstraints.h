#pragma once


#include <QMouseEvent>
#include "cad_events.h"


class Vtk3d_Sketch;

struct Tool_SetConstraints  {

    Tool_SetConstraints(Vtk3d_Sketch* parent)  {
        m_Parent = parent;
    }

    void activate();
    void desactivate();

    bool keyPressEvent(QKeyEvent* event) ;
    bool gererWheelEvent(QWheelEvent* event) ;
    bool gererMouseMove(QMouseEvent* event) ;
    bool gererMouseRelease(QMouseEvent* event) ;
    bool gererMousePress(QMouseEvent* event) ;
    bool gererkeyPressEvent(QKeyEvent* event) ;

    bool    PrimitiveIsSelected = false;
    int     SelectedPrimitiveId = 0;

    bool    m_b_MouseLIsPressed = false;
    bool    b_IsSomethingSelected = false;
    int     m_SelectedPrimitiveId = -1;

    QPoint     m_MouseclickStartPosition;

    void ajusterEchelleElements( double li_echelle);

    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch*   m_Parent = nullptr;
};









