
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
    if (event->button() == Qt::LeftButton) {
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








