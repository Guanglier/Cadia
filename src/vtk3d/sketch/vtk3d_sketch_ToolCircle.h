#pragma once

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
#include <vtkFloatArray.h>
#include <vtkMapper.h>
#include <vtkRegularPolygonSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>
#include <QMouseEvent>
#include <cmath>
#include <variant>
#include <qevent.h>
#include "DimensionEngine.h"
#include "cad_events.h"

//#include "vtk3d_sketch_Tools.h"


class Vtk3d_Sketch;

struct Tool_CircleDraw {


    Tool_CircleDraw(Vtk3d_Sketch* parent) {
        m_Parent = parent;
    }

    void activate();
    void desactivate();

    bool keyPressEvent(QKeyEvent* event);
    bool gererWheelEvent(QWheelEvent* event);
    bool gererMouseMove(QMouseEvent* event);
    bool gererMouseRelease(QMouseEvent* event);
    bool gererMousePress(QMouseEvent* event);
    bool gererkeyPressEvent(QKeyEvent* event);

    void EndDrawCircle();
    //void ajusterEchelleCarreSnap();
    void ajusterEchelleElements( double li_echelle);
    //void AddCircleToOp(Vector2D<double> centerPoint2D, Vector2D<double> edgePoint2D);
    //void AddCircleToOp(gp_Pnt2d centerPoint2D, gp_Pnt2d edgePoint2D);
    //void AddCircleToOp(gp_Pnt centerPoint3D, double radius);
    void AddCircleToOp(gp_Pnt2d centerPoint2D, double radius);


    bool m_isDrawingCircle; // Flag pour l'état du cercle élastique

    vtkSmartPointer<vtkActor> m_snapPointActor;
    double m_derniereEchelleCarre = 0.0; // Cache de performance

    // Composants VTK mis à jour pour le cercle dynamique
    vtkSmartPointer<vtkRegularPolygonSource> m_circleSource;
    vtkSmartPointer<vtkActor>                m_circleActor;

    gp_Pnt m_centerPoint3D;
    gp_Pnt m_edgePoint3D;

    gp_Pnt2d m_mouseCenterPoint;
    gp_Pnt2d m_mouseEdgePoint;
    gp_Pnt m_center3D;
    gp_Pnt2d m_center2D;
    gp_Pnt m_edge3D;

    // Vector3D<double> m_centerPoint3D;
    // Vector3D<double> m_edgePoint3D;

    // Vector2D<double> m_mouseCenterPoint;
    // Vector2D<double> m_mouseEdgePoint;
    // Vector3D<double> m_center3D;
    // Vector3D<double> m_edge3D;

    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch* m_Parent = nullptr;
};