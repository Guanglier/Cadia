#pragma once



#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>
#include <variant>
#include <qevent.h>
#include <QMouseEvent>

#include <vtkAssemblyPath.h>
#include <vtkAssemblyNode.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkProp.h>
#include <vtkPolyDataMapper.h>
#include <vtkProp3DCollection.h>
#include <vtkPropPicker.h>
#include <vtkLine.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleImage.h>
#include <vtkDataSetMapper.h>
#include <vtkDataObject.h>
#include <vtkCellPicker.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkCamera.h>
#include <vtkSphereSource.h>
#include <vtkCallbackCommand.h>
#include "cad_events.h"

#include "CAD_PartOp.h"

class Vtk3d_Sketch;

struct Tool_Select  {

    Tool_Select(Vtk3d_Sketch* parent)  {
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

    QPoint     m_MouseclickStartPosition;

    void ajusterEchelleElements( double li_echelle);

    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch*   m_Parent = nullptr;


    enum class DragMode {
        None,
        PointUnique,
        LigneComplete,
        CercleCentre,
        CercleRayon
    };

    struct {
        bool m_isDragging = false;
        DragMode m_mode = DragMode::None;
        gp_Pnt2d m_lastMousePos2D;

        SketchPoint* PtrSelectedPoint = nullptr;


        int m_activePrimitiveId = -1; // Remplace l'ancien edgeId pour être générique (Ligne ou Cercle)




        struct{
            gp_Pnt2d mouse_when_clicked_2d;
            struct{
                gp_Vec2d    start;
                gp_Vec2d    stop;
            }line;
            struct{
                gp_Vec2d    center;
            }circle;
        }PrimToMoseVects;
    } DynamicDrag;

};























