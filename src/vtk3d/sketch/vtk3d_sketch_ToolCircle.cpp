#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolCircle.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <cmath>
#include <iostream>

//─────────────────────────────────────────────────────────────────────
//               ACTIVATION
//─────────────────────────────────────────────────────────────────────
void Tool_CircleDraw::activate() {
    m_centerPoint3D.SetX( 0.0);
    m_centerPoint3D.SetY( 0.0);
    m_centerPoint3D.SetZ( 0.0);
    m_isDrawingCircle = false;

    if (!m_Parent->GetView() || !m_Parent->GetView()->renderWindow()) return;

    // Configuration de la source du cercle (64 côtés pour un rendu parfaitement lisse)
    m_circleSource = vtkSmartPointer<vtkRegularPolygonSource>::New();
    m_circleSource->SetNumberOfSides(64);
    m_circleSource->SetRadius(0.0);
    m_circleSource->SetCenter(0.0, 0.0, 0.0);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(m_circleSource->GetOutputPort());

    m_circleActor = vtkSmartPointer<vtkActor>::New();
    m_circleActor->SetMapper(mapper);
    m_circleActor->GetProperty()->SetColor(1.0, 0.0, 0.0); // Rouge CAO classique pour l'élastique
    m_circleActor->GetProperty()->SetLineWidth(2.0);
    m_circleActor->GetProperty()->SetRepresentationToWireframe(); // Mode fil de fer pour ne pas remplir le cercle
    m_circleActor->SetVisibility(false);

    m_Parent->GetView()->getRenderer()->AddActor(m_circleActor);
    if ( true == m_Parent->m_ContexteEdition.m_EditionEnCours ){

    }
}

void Tool_CircleDraw::desactivate() {
    if (m_circleActor) m_Parent->GetView()->getRenderer()->RemoveActor(m_circleActor);
    m_Parent->getConstraintManager()->masquerFeedback();
}

void Tool_CircleDraw::EndDrawCircle() {
    if (m_isDrawingCircle) {
        m_isDrawingCircle = false;
        m_circleActor->SetVisibility(false);
        auto* manager = m_Parent->getConstraintManager();
        manager->masquerFeedback();
        m_Parent->GetView()->renderWindow()->Render();
    }
}

bool Tool_CircleDraw::keyPressEvent(QKeyEvent* event)   { return false; }
bool Tool_CircleDraw::gererWheelEvent(QWheelEvent* event) { return false; }
bool Tool_CircleDraw::gererMouseRelease(QMouseEvent* event) { return false; }
//─────────────────────────────────────────────────────────────────────
//       Ajustement des échelles des éléments en fonction des zoom
//─────────────────────────────────────────────────────────────────────
void Tool_CircleDraw::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}
//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_CircleDraw::gererMouseMove(QMouseEvent* event) {
    gp_Pnt currentPoint3D;
    gp_Pnt2d currentPoint2D;

    if (!m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, currentPoint3D)) {
        return false;
    }

    auto* manager = m_Parent->getConstraintManager();

    if (!m_isDrawingCircle) {
        // 🟩 CAS 1 : Aimantation libre avant le clic sur le centre
        //manager->appliqueContraintes2D({0,0,0}, currentPoint3D, false, false);
    }
    else {

        // Calcul de la distance 3D sur le plan pour obtenir le rayon
        double radius = std::sqrt(
            std::pow(currentPoint3D.X() - m_centerPoint3D.X(), 2) +
            std::pow(currentPoint3D.Y() - m_centerPoint3D.Y(), 2) +
            std::pow(currentPoint3D.Z() - m_centerPoint3D.Z(), 2)
        );

        // Mise à jour dynamique de la géométrie du cercle VTK
        m_circleSource->SetRadius(radius);
        m_circleSource->Modified();
    }

    m_Parent->GetView()->renderWindow()->Render();
    return true;
}

//─────────────────────────────────────────────────────────────────────
//               MOUSE  PRESS
//─────────────────────────────────────────────────────────────────────
bool Tool_CircleDraw::gererMousePress(QMouseEvent* event) {
    gp_Pnt2d currentPoint2D;

    if (event->button() != Qt::LeftButton) return false;

    auto* manager = m_Parent->getConstraintManager();

    if (!m_isDrawingCircle) {
        // 🟩 PREMIER CLIC : Définition du centre du cercle
        m_isDrawingCircle = true;
        m_mouseCenterPoint.SetX( event->x() );
        m_mouseCenterPoint.SetY(event->y() );

        if (m_Parent->calculerIntersectionSourisSurPlan(m_mouseCenterPoint.X(), m_mouseCenterPoint.Y(), currentPoint2D, m_centerPoint3D)) {
            gp_Dir normalPlan = m_Parent->DocumentRefs.GetSketchPlane().Direction();

            // On cale le centre VTK et on initialise le rayon à 0
            m_circleSource->SetCenter(m_centerPoint3D.X(), m_centerPoint3D.Y(), m_centerPoint3D.Z());
            m_circleSource->SetNormal(normalPlan.X(), normalPlan.Y(), normalPlan.Z());
            m_circleSource->SetRadius(0.0);
            m_circleSource->Modified();

            m_center3D = m_centerPoint3D;
            m_center2D = currentPoint2D;
            m_circleActor->SetVisibility(true);
            m_Parent->GetView()->renderWindow()->Render();
        }
        return true;
    }
    else {
        // 🟦 DEUXIÈME CLIC : Validation du rayon et création de la primitive
        m_mouseEdgePoint.SetX( event->x() );
        m_mouseEdgePoint.SetY( event->y() );

        if (m_Parent->calculerIntersectionSourisSurPlan(m_mouseEdgePoint.X(), m_mouseEdgePoint.Y(), currentPoint2D, m_edge3D)) {
            gp_Vec CenterToMouse (m_center3D, m_edge3D);

            AddCircleToOp(m_center2D, CenterToMouse.Magnitude());
            m_Parent->rafraichirAffichageEsquisse();
            m_Parent->Signaler_ChangementEsquisseIHM ();
        }
        EndDrawCircle();
        return true;
    }
}

bool Tool_CircleDraw::gererkeyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        EndDrawCircle();
    }
    return false;
}

//─────────────────────────────────────────────────────────────────────
//      Ajouter le cercle à l'opération locale OpenCascade
//─────────────────────────────────────────────────────────────────────
void Tool_CircleDraw::AddCircleToOp(gp_Pnt2d centerPoint2D, double radius) {
    if (!m_Parent->DocumentRefs.GetOperation()) {
        std::cerr << "[ERROR] AddCircleToOp: m_Operation est nul !\n";
        return;
    }
    auto* sketchParams = std::get_if<SketchParams>(&m_Parent->DocumentRefs.GetOperation()->getParamsMutable());
    if (!sketchParams) {
        std::cerr << "[ERROR] AddCircleToOp: L'opération cible n'est pas un SketchParams.\n";
        return;
    }
    SketchCircle    cercle(centerPoint2D, radius);
    cercle.center.Update3D( sketchParams->m_sketchPlane );
    uint64_t l_id = sketchParams->addPrimitive( cercle );
}



void Tool_CircleDraw::CADEvent_TraiterCommande(const CadCommandEvent& event){

}


