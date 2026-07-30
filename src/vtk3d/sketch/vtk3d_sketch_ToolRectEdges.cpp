#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolRectEdges.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <iostream>

//─────────────────────────────────────────────────────────────────────
//               ACTIVATION
//─────────────────────────────────────────────────────────────────────
void Tool_RectEdgesDraw::activate() {
    m_startPoint3D = gp_Pnt(0, 0, 0);
    m_isDrawingRect = false;

    if (!m_Parent->GetView() || !m_Parent->GetView()->renderWindow()) return;

    // Allocation des 5 points nécessaires pour boucler le rectangle (0 -> 1 -> 2 -> 3 -> 0)
    m_rectPoints = vtkSmartPointer<vtkPoints>::New();
    m_rectPolyData = vtkSmartPointer<vtkPolyData>::New();
    
    for (int i = 0; i < 5; ++i) {
        m_rectPoints->InsertNextPoint(0.0, 0.0, 0.0);
    }
    m_rectPolyData->SetPoints(m_rectPoints);

    // Liaison des points pour former une ligne polygonale fermée
    auto polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(5);
    for (int i = 0; i < 5; ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);
    m_rectPolyData->SetLines(cells);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(m_rectPolyData);

    m_rectActor = vtkSmartPointer<vtkActor>::New();
    m_rectActor->SetMapper(mapper);
    m_rectActor->GetProperty()->SetColor(1.0, 0.0, 0.0); // Rouge élastique synchro
    m_rectActor->GetProperty()->SetLineWidth(2.0);
    m_rectActor->SetVisibility(false);

    m_Parent->GetView()->getRenderer()->AddActor(m_rectActor);
}

void Tool_RectEdgesDraw::desactivate() {
    if (m_rectActor) m_Parent->GetView()->getRenderer()->RemoveActor(m_rectActor);
    m_Parent->getSnapperManager()->masquerFeedback();
}

void Tool_RectEdgesDraw::EndDrawRectangle() {
    if (m_isDrawingRect) {
        m_isDrawingRect = false;
        m_rectActor->SetVisibility(false);
        auto* manager = m_Parent->getSnapperManager();
        manager->masquerFeedback();
        m_Parent->GetView()->renderWindow()->Render();
    }
}

bool Tool_RectEdgesDraw::keyPressEvent(QKeyEvent* event)   { return false; }
bool Tool_RectEdgesDraw::gererWheelEvent(QWheelEvent* event) { return false; }
bool Tool_RectEdgesDraw::gererMouseRelease(QMouseEvent* event) { return false; }

//─────────────────────────────────────────────────────────────────────
// mettre ici tous les acteurs qui ont besion d'être redimensionnés en fonction du zoom
// cette fonction est call par la classe mere sketch
//─────────────────────────────────────────────────────────────────────
void Tool_RectEdgesDraw::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}

//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_RectEdgesDraw::gererMouseMove(QMouseEvent* event) {
    gp_Pnt currentPoint3D;
    gp_Pnt2d currentPoint2D;

    if (!m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, currentPoint3D)) {
        return false;
    }

    auto* manager = m_Parent->getSnapperManager();

    if (!m_isDrawingRect) {
        // 🟩 CAS 1 : Aimantation libre sur la grille avant le premier coin
        //manager->appliqueContraintes2D({0,0,0}, currentPoint3D, false, false);
        // m_Cotation1_DistDesc.pntStop = currentPoint3D;
        // m_Cotation2_bDescriptorDefined = true;
        // m_Cotation1_currentDimensionDescriptor.data = m_Cotation1_DistDesc;
        // DimensionEngine::GeometryResult Cot1_geoResult = DimensionEngine::ComputeGeometry(m_Parent->GetSketchPlane(), m_Cotation1_currentDimensionDescriptor);
        // m_Parent->m_Cotation2->DessinerCotationDepuisResultat(m_Parent->GetSketchPlane(), Cot1_geoResult);

    }
    else {
        // 🟦 CAS 2 : Calcul et mise à jour des 4 coins du rectangle élastique
        //manager->appliqueContraintes2D(m_startPoint3D, currentPoint3D, true, true);
        m_Cotation1_DistDesc.pntStop = currentPoint3D;
        m_Cotation2_bDescriptorDefined = true;
        m_Cotation1_currentDimensionDescriptor.data = m_Cotation1_DistDesc;
        DimensionEngine::GeometryResult Cot1_geoResult = DimensionEngine::ComputeGeometry( m_Parent->DocumentRefs.GetSketchPlane(), m_Cotation1_currentDimensionDescriptor);
        m_Parent->m_Cotation2->DessinerCotationDepuisResultat(m_Parent->DocumentRefs.GetSketchPlane(), Cot1_geoResult);


        // On reconstruit la boîte 2D projetée sur le plan 3D d'esquisse (z constant ou plan local)
        m_rectPoints->SetPoint(0, m_startPoint3D.X(),  m_startPoint3D.Y(),  m_startPoint3D.Z() ); // Coin d'origine 1
        m_rectPoints->SetPoint(1, currentPoint3D.X(),  m_startPoint3D.Y(),  m_startPoint3D.Z() ); // Coin supérieur opposé
        m_rectPoints->SetPoint(2, currentPoint3D.X(),  currentPoint3D.Y(),  currentPoint3D.Z() ); // Coin d'arrivée 2
        m_rectPoints->SetPoint(3, m_startPoint3D.X(),  currentPoint3D.Y(),  currentPoint3D.Z() ); // Coin inférieur opposé
        m_rectPoints->SetPoint(4, m_startPoint3D.X(),  m_startPoint3D.Y(),  m_startPoint3D.Z() ); // Fermeture du polygone

        m_rectPoints->Modified();
    }

    m_Parent->GetView()->renderWindow()->Render();
    return true;
}


void Tool_RectEdgesDraw::Cotation1_Configure (gp_Pnt liMousePos3D){
    m_Cotation1_DistDesc.pntStart = liMousePos3D;
    m_Cotation1_DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    m_Cotation1_DistDesc.offset = 2.0;
    m_Cotation1_currentDimensionDescriptor.data = m_Cotation1_DistDesc;
    m_Cotation1_currentDimensionDescriptor.id = 0;
    m_Cotation2_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    m_Parent->m_Cotation->Afficher ();
}

void Tool_RectEdgesDraw::Cotation2_Configure (gp_Pnt liMousePos3D){
    m_Cotation2_DistDesc.pntStart = liMousePos3D;
    m_Cotation2_DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    m_Cotation2_DistDesc.offset = 2.0;
    m_Cotation2_currentDimensionDescriptor.data = m_Cotation2_DistDesc;
    m_Cotation2_currentDimensionDescriptor.id = 0;
    m_Cotation2_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    m_Parent->m_Cotation->Afficher ();
}

//─────────────────────────────────────────────────────────────────────
//               MOUSE  PRESS
//─────────────────────────────────────────────────────────────────────
bool Tool_RectEdgesDraw::gererMousePress(QMouseEvent* event) {
    gp_Pnt2d currentPoint2D;

    if (event->button() != Qt::LeftButton) return false;

    auto* manager = m_Parent->getSnapperManager();

    if (!m_isDrawingRect) {
        // 🟩 PREMIER CLIC : Premier coin de la boîte
        m_isDrawingRect = true;
        //m_RectStart2D.SetCoord(static_cast<double>(event->x()), static_cast<double>(event->y())  );

        if (m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), m_RectStart2D, m_startPoint3D)) {
            //manager->appliqueContraintes2D({0,0,0}, m_startPoint3D, false, false);

            // Initialisation de l'élastique à plat sur le point d'ancrage
            for (int i = 0; i < 5; ++i) {
                m_rectPoints->SetPoint(i, m_startPoint3D.X(), m_startPoint3D.Y(), m_startPoint3D.Z());
            }
            m_rectPoints->Modified();

            m_point1_3D = m_startPoint3D;
            m_rectActor->SetVisibility(true);
            m_Parent->GetView()->renderWindow()->Render();
            Cotation1_Configure ( m_startPoint3D );
            Cotation2_Configure ( m_startPoint3D );

        }
        return true;
    }
    else {
        // 🟦 DEUXIÈME CLIC : Coin diagonal opposé, validation géométrique
        //m_RectEnd2D.SetCoord(static_cast<double>(event->x()), static_cast<double>(event->y())  );

        if (m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), m_RectEnd2D, m_point2_3D)) {
            //manager->appliqueContraintes2D(m_point1_3D, m_point2_3D, true, true);
            //m_mouseStartPoint.SetCoord( m_point1_3D.X(), m_point1_3D.Y());
            //m_mouseEndPoint.SetCoord( m_point2_3D.X(), m_point2_3D.Y() );

            AddRectangleToOp(m_RectStart2D, m_RectEnd2D);
            m_Parent->rafraichirAffichageEsquisse();
            m_Parent->Signaler_ChangementEsquisseIHM ();
        }

        EndDrawRectangle();
        return true;
    }
}

bool Tool_RectEdgesDraw::gererkeyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        EndDrawRectangle();
    }
    return false;
}

//─────────────────────────────────────────────────────────────────────
//      Ajouter le rectangle à l'opération locale OpenCascade
//─────────────────────────────────────────────────────────────────────
void Tool_RectEdgesDraw::AddRectangleToOp(gp_Pnt2d& p1_2D, gp_Pnt2d& p2_2D) {
    if (!m_Parent->DocumentRefs.GetOperation()) {
        std::cerr << "[ERROR] AddRectangleToOp: m_Operation est nul !\n";
        return;
    }
    auto* sketchParams = m_Parent->DocumentRefs.GetParams();

    if (!sketchParams) {
        std::cerr << "[ERROR] AddRectangleToOp: L'opération cible n'est pas un SketchParams.\n";
        return;
    }

    // 📐 Calcul des coordonnées des 4 coins à partir des 2 points de la diagonale
    gp_Pnt2d B(p2_2D.X(), p1_2D.Y()); // Coin Supérieur Droit
    gp_Pnt2d D(p1_2D.X(), p2_2D.Y()); // Coin Inférieur Gauche

    sketchParams->addLine(p1_2D, B);
    sketchParams->addLine(B, p2_2D);
    sketchParams->addLine(p2_2D, D);
    sketchParams->addLine(D, p1_2D);

/*
    SketchLine   Line_AB(p1_2D, B);
    SketchLine   Line_BC(B, p2_2D);
    SketchLine   Line_CD(p2_2D, D);
    SketchLine   Line_DA(D, p1_2D);

    Line_AB.start.Update3D( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_AB.stop.Update3D ( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_BC.start.Update3D( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_BC.stop.Update3D ( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_CD.start.Update3D( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_CD.stop.Update3D ( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_DA.start.Update3D( m_Parent->DocumentRefs.GetSketchPlane() );
    Line_DA.stop.Update3D ( m_Parent->DocumentRefs.GetSketchPlane() );


    // 🚀 Ajout des 4 lignes horizontales et verticales dans l'esquisse
    sketchParams->addPrimitive(  Line_AB );
    sketchParams->addPrimitive(  Line_BC );
    sketchParams->addPrimitive(  Line_CD );
    sketchParams->addPrimitive(  Line_DA );
    */
}



void Tool_RectEdgesDraw::CADEvent_TraiterCommande(const CadCommandEvent& event){

}




