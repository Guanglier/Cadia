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
#include <vtkFloatArray.h>

#include <vtkMapper.h>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>
#include <QMouseEvent>
#include <cmath>
#include <vtkRegularPolygonSource.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkLine.h>
#include <vtkPolyLine.h>
#include <cmath>


#include <variant>
#include <qevent.h>
#include <QMouseEvent>

#include "vtk3d_sketch_Context.h"
#include "DimensionEngine.h"

#include "cad_events.h"

class Vtk3d_Sketch;



struct Tool_LineDraw {


    Tool_LineDraw(Vtk3d_Sketch* parent)  {
        m_Parent = parent;
    }


    void activate ();
    void desactivate ();

    bool keyPressEvent(QKeyEvent* event) ;
    bool gererWheelEvent(QWheelEvent* event) ;
    bool gererMouseMove(QMouseEvent* event) ;
    bool gererMouseRelease(QMouseEvent* event) ;
    bool gererMousePress(QMouseEvent* event) ;
    bool gererkeyPressEvent(QKeyEvent* event) ;

    void EndDrawLine ();


    bool m_isDrawingLigne; // Pour la gestion de la ligne élastique

    vtkSmartPointer<vtkActor>        m_snapPointActor;


    bool m_bDescriptorDefined;
    DimensionEngine::Descriptor m_currentDimensionDescriptor;
    void Cotation_Configure (gp_Pnt liMousePos3D);


    double                    m_derniereEchelleCarre = 0.0; // Cache de performance

    void ajusterEchelleElements( double li_echelle);
    void AddLineToOp (gp_Pnt2d& StartPoint2D, gp_Pnt2d& StopPoint2D);

    // Objets VTK pour la ligne élastique temporaire
    vtkSmartPointer<vtkPoints>      m_linePoints;
    vtkSmartPointer<vtkPolyData>    m_linePolyData;
    vtkSmartPointer<vtkActor>       m_lineActor;

    gp_Pnt      m_startPoint3D;
    gp_Pnt2d    m_startPoint2D;
    gp_Pnt      m_endPoint3D;
    gp_Pnt2d    m_endPoint2D;

    gp_Pnt2d    m_MousePoint1;
    gp_Pnt2d    m_MousePoint2;
    gp_Pnt2d    m_Point1_2D;
    gp_Pnt2d    m_Point2_2D;

    Vtk3d_Sketch*   m_Parent = nullptr;


    void CADEvent_TraiterCommande(const CadCommandEvent& event);

};





