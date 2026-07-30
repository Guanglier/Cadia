#pragma once


#include <vtkRegularPolygonSource.h>
#include <vtkActor.h> // Indispensable pour vtkSmartPointer<vtkActor>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <QMouseEvent>
#include <QWheelEvent>

// Vos en-têtes
#include "DimensionEngine.h"
#include "cad_events.h"


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
    void ajusterEchelleElements( double li_echelle);

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



    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch* m_Parent = nullptr;
};