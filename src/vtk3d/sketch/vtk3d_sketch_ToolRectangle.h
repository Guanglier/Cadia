#pragma once

#include <variant>
#include <cmath>
#include <QMouseEvent>

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>

#include "DimensionEngine.h"
#include "cad_events.h"


#include <vtkPoints.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

class Vtk3d_Sketch;

struct Tool_Rectangle {


    Tool_Rectangle(Vtk3d_Sketch* parent) {
        common.m_Parent = parent;
    }

    void activate( const CadEvent::Sketch::Tool_SubMode& submode );
    void desactivate();

    bool keyPressEvent(QKeyEvent* event);
    bool gererWheelEvent(QWheelEvent* event);

    bool gererMouseMove(QMouseEvent* event);
    bool RectByEdge_gererMouseMove(QMouseEvent* event);
    bool RectByCenter_gererMouseMove(QMouseEvent* event);

    bool gererMousePress(QMouseEvent* event);
    bool RectByEdges_gererMousePress(QMouseEvent* event);
    bool RectByCenter_gererMousePress(QMouseEvent* event);

    bool gererMouseRelease(QMouseEvent* event);

    bool gererkeyPressEvent(QKeyEvent* event);

    void EndDrawRectangle();
    void ajusterEchelleElements( double li_echelle);
    void AddCenterRectangleToOp(const gp_Pnt2d& li_PA, const gp_Pnt2d& li_PB, const gp_Pnt2d& li_PC, const gp_Pnt2d& li_PD) ;
	void AddRectangleToOp(gp_Pnt2d& p1_2D, gp_Pnt2d& p2_2D);
	



    //--- cotation pendant le dessin ----------
    void RectByCenter_Cotation_Configure (gp_Pnt liMousePos3D);
    void RectByCenter_Cotation2_Configure (gp_Pnt Li_Start3D, gp_Pnt li_Stop3D);

    void RectByEdge_Cotation1_Configure (gp_Pnt liMousePos3D);
    void RectByEdge_Cotation2_Configure (gp_Pnt liMousePos3D);

    double m_derniereEchelleCarre = 0.0;



	struct{
			
		// Objets VTK pour le rectangle élastique à 5 points
		vtkSmartPointer<vtkPoints>    m_rectPoints;
		vtkSmartPointer<vtkPolyData>  m_rectPolyData;
		vtkSmartPointer<vtkActor>     m_rectActor;
        vtkSmartPointer<vtkActor> m_snapPointActor;
        Vtk3d_Sketch* m_Parent = nullptr;
        CadEvent::Sketch::Tool_SubMode    m_SubMode = CadEvent::Sketch::RectangleSubMode::ByCenter;
	}common;
	
	struct {
		// Points de mémorisation des clics successifs
		gp_Pnt m_centerPoint3D;
		gp_Pnt m_widthPoint3D;
		gp_Pnt m_heightPoint3D;
		gp_Pnt2d m_centerPoint2D;
		gp_Pnt2d m_widthPoint2D;
		gp_Pnt2d m_heightPoint2D;
		
		
		int m_drawStep = 0;		//Machine à états : 0 = Inactif, 1 = Centre posé (Règle Largeur/Angle), 2 = Largeur posée (Règle Hauteur)
		
		bool m_bDescriptorDefined;
		DimensionEngine::Descriptor m_currentDimensionDescriptor;

		bool m_Cotation2_bDescriptorDefined;
		DimensionEngine::Descriptor m_Cotation2_currentDimensionDescriptor;
	}rect_center;
	
	struct { 
		gp_Pnt m_startPoint3D;
		gp_Pnt m_endPoint3D;

		gp_Pnt2d m_RectStart2D;
		gp_Pnt2d m_RectEnd2D;
		gp_Pnt m_point1_3D;
		gp_Pnt m_point2_3D;
		
		bool m_isDrawingRect; // Flag pour l'état de la boîte
		
		bool m_Cotation1_bDescriptorDefined;
		DimensionEngine::Descriptor 			m_Cotation1_currentDimensionDescriptor;
		DimensionEngine::DistanceDescriptor  	m_Cotation1_DistDesc;
		
		bool m_Cotation2_bDescriptorDefined;
		DimensionEngine::Descriptor 			m_Cotation2_currentDimensionDescriptor;
		DimensionEngine::DistanceDescriptor  	m_Cotation2_DistDesc;
	}rect_edges;

    void CADEvent_TraiterCommande(const CadCommandEvent& event);




};


