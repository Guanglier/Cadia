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
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>

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

class Vtk3d_Sketch;

struct Tool_RectCenterDraw {


    Tool_RectCenterDraw(Vtk3d_Sketch* parent) {
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

    void EndDrawRectangle();
    //void ajusterEchelleCarreSnap();
    void ajusterEchelleElements( double li_echelle);
    //void AddCenterRectangleToOp(const gp_Pnt& p0, const gp_Pnt& p1,                                const gp_Pnt& p2, const gp_Pnt& p3);
    void AddCenterRectangleToOp(const gp_Pnt2d& li_PA, const gp_Pnt2d& li_PB, const gp_Pnt2d& li_PC, const gp_Pnt2d& li_PD) ;

    // 🎯 Machine à états : 0 = Inactif, 1 = Centre posé (Règle Largeur/Angle), 2 = Largeur posée (Règle Hauteur)
    int m_drawStep = 0;

    //--- cotation pendant le dessin ----------
    bool m_bDescriptorDefined;
    DimensionEngine::Descriptor m_currentDimensionDescriptor;
    void Cotation_Configure (gp_Pnt liMousePos3D);

    bool m_Cotation2_bDescriptorDefined;
    DimensionEngine::Descriptor m_Cotation2_currentDimensionDescriptor;
    void Cotation2_Configure (gp_Pnt Li_Start3D, gp_Pnt li_Stop3D);


    vtkSmartPointer<vtkActor> m_snapPointActor;
    double m_derniereEchelleCarre = 0.0;

    // Objets VTK pour le rectangle élastique à 5 points
    vtkSmartPointer<vtkPoints>    m_rectPoints;
    vtkSmartPointer<vtkPolyData>  m_rectPolyData;
    vtkSmartPointer<vtkActor>     m_rectActor;

    // Points de mémorisation des clics successifs
    gp_Pnt m_centerPoint3D;
    gp_Pnt m_widthPoint3D;
    gp_Pnt m_heightPoint3D;
    gp_Pnt2d m_centerPoint2D;
    gp_Pnt2d m_widthPoint2D;
    gp_Pnt2d m_heightPoint2D;

    void CADEvent_TraiterCommande(const CadCommandEvent& event);



    Vtk3d_Sketch* m_Parent = nullptr;
};