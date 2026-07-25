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
#include <vtkTextActor.h>

#include "vtk3d_Sketch_Render_Cotations.h"

#include "DimensionEngine.h"
#include "cad_events.h"

class Vtk3d_Sketch;

//

struct DimensionDessin {
    struct point{
        double x, y;
    };
    struct ligne{
        point start;
        point stop;
    };
    ligne ligne_para;
    ligne ligne_PerpStart;
    ligne ligne_PerpStop;
    ligne fleche_D;
    ligne fleche_G;
};

struct Tool_Dimensions  {

    Tool_Dimensions(Vtk3d_Sketch* parent)  {
        m_Parent = parent;
    }

    // Objets VTK pour la ligne élastique temporaire
    vtkSmartPointer<vtkPoints>      m_linePoints;
    vtkSmartPointer<vtkPolyData>    m_linePolyData;
    vtkSmartPointer<vtkActor>       m_lineActor;

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
    int     m_SelectedPrimitiveSub = -1;

    vtk3d_Sketch_Render_Cotations::RefLine_type    CotationStrRef;


    // struct{
    //     bool ref_defined;
    //     struct{
    //         double x;
    //         double y;
    //     }start;
    //     struct{
    //         double x;
    //         double y;
    //     }stop;
    //     double a, b;
    // }RefLine;

    vtkNew<vtkActor> actor;
    vtkNew<vtkTextActor> textActor;

    vtkNew<vtkPolyData>         m_polyData;
    vtkNew<vtkPolyDataMapper>   m_mapper;
    vtkNew<vtkPoints>           m_points;
    vtkNew<vtkCellArray>        m_lines;


    bool m_bDescriptorDefined;
    DimensionEngine::Descriptor m_currentDimensionDescriptor;

    QPoint     m_MouseclickStartPosition;

    void ajusterEchelleElements( double li_echelle);
    void Cotation_Configure( int li_PrimId, int li_PrimSubElmt);
    bool Cotation_MouseMove (gp_Pnt& PtnMouse3D);
    //void DessinerDimension ( gp_Pnt currentPoint3D  );

    //void DessinerCotation_PointToPoint ( vtkNew<vtkCellArray>& lines, vtkNew<vtkPoints>& points);
    bool IsMousePerpendi (gp_Pnt currentPoint3D);

    void MettreAJourTexte2D(double start_x, double start_y, double stop_x, double stop_y);
    void DessinerCotation_PointToPoint ( gp_Pnt currentPoint3D  );
    void DessinerCotation_HV ( gp_Pnt mouse_point , bool b_IsHorizontal );
    void DrawPrepairedCotation ( vtkNew<vtkCellArray>& lines, vtkNew<vtkPoints>& points);
    void DessineFleche ( vtkNew<vtkCellArray> &lines, vtkNew<vtkPoints> &points,
                       double li_AngleRefRad,
                       double li_AngleFlecheDeg,
                       double li_Longueur_mm,
                       double li_x, double li_y,
                       bool li_LeftToRight);

    void DessinerCotationLigne (gp_Pnt currentPoint3D);
    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch*   m_Parent = nullptr;
};







