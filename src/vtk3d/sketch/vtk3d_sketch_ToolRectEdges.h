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

struct Tool_RectEdgesDraw {


    Tool_RectEdgesDraw(Vtk3d_Sketch* parent) {
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
    void AddRectangleToOp(gp_Pnt2d& p1_2D, gp_Pnt2d& p2_2D);

    bool m_isDrawingRect; // Flag pour l'état de la boîte élastique

    vtkSmartPointer<vtkActor> m_snapPointActor;
    double m_derniereEchelleCarre = 0.0;

    // Objets VTK pour le rectangle élastique
    vtkSmartPointer<vtkPoints>    m_rectPoints;
    vtkSmartPointer<vtkPolyData>  m_rectPolyData;
    vtkSmartPointer<vtkActor>     m_rectActor;

    gp_Pnt m_startPoint3D;
    gp_Pnt m_endPoint3D;

    gp_Pnt2d m_RectStart2D;
    gp_Pnt2d m_RectEnd2D;
    gp_Pnt m_point1_3D;
    gp_Pnt m_point2_3D;

    void Cotation1_Configure (gp_Pnt liMousePos3D);
    void Cotation2_Configure (gp_Pnt liMousePos3D);

    bool m_Cotation2_bDescriptorDefined;
    DimensionEngine::Descriptor m_Cotation2_currentDimensionDescriptor;
    DimensionEngine::DistanceDescriptor  m_Cotation2_DistDesc;
    void Cotation2_Configure (gp_Pnt Li_Start3D, gp_Pnt li_Stop3D);

    bool m_Cotation1_bDescriptorDefined;
    DimensionEngine::Descriptor m_Cotation1_currentDimensionDescriptor;
    DimensionEngine::DistanceDescriptor  m_Cotation1_DistDesc;
    void Cotation1_Configure (gp_Pnt Li_Start3D, gp_Pnt li_Stop3D);

    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch* m_Parent = nullptr;
};

