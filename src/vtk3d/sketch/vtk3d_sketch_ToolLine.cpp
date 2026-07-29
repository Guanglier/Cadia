
#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolLine.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkImageData.h>
#include <vtkTexture.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include "DimensionEngine.h"

//─────────────────────────────────────────────────────────────────────
//               ACTIVATION
//─────────────────────────────────────────────────────────────────────
void Tool_LineDraw::activate () {
    m_startPoint3D = gp_Pnt(0, 0, 0) ;
    m_isDrawingLigne = false;

    if (!m_Parent->GetView() || !m_Parent->GetView()->renderWindow()) return;

    // Ligne élastique locale
    m_linePoints = vtkSmartPointer<vtkPoints>::New();
    m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
    m_linePoints->InsertNextPoint(0, 0, 0);
    m_linePoints->InsertNextPoint(0, 0, 0);
    m_linePolyData->SetPoints(m_linePoints);

    auto line = vtkSmartPointer<vtkLine>::New();
    line->GetPointIds()->SetId(0, 0);
    line->GetPointIds()->SetId(1, 1);

    auto lines = vtkSmartPointer<vtkCellArray>::New();
    lines->InsertNextCell(line);
    m_linePolyData->SetLines(lines);

    auto mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper2->SetInputData(m_linePolyData);

    m_lineActor = vtkSmartPointer<vtkActor>::New();
    m_lineActor->SetMapper(mapper2);
    m_lineActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    m_lineActor->GetProperty()->SetLineWidth(2.0);
    m_lineActor->SetVisibility(false);

    m_Parent->GetView()->getRenderer()->AddActor(m_lineActor);
}



void Tool_LineDraw::desactivate () {
    if (m_lineActor) m_Parent->GetView()->getRenderer()->RemoveActor(m_lineActor);
    // On demande au manager de nettoyer l'écran pour l'outil suivant
    m_Parent->getConstraintManager()->masquerFeedback();
}


void Tool_LineDraw::EndDrawLine (){
    if ( m_isDrawingLigne ){
        m_isDrawingLigne = false;
        m_lineActor->SetVisibility(false);
        auto* manager = m_Parent->getConstraintManager();
        manager->masquerFeedback();
        m_Parent->GetView()->renderWindow()->Render();
    }
}

bool Tool_LineDraw::keyPressEvent(QKeyEvent* event)  {
    return false;
}

bool Tool_LineDraw::gererWheelEvent(QWheelEvent* event){
    return false;
}

//─────────────────────────────────────────────────────────────────────
//       Ajustement des échelles des éléments en fonction des zoom
//─────────────────────────────────────────────────────────────────────
void Tool_LineDraw::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}
//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_LineDraw::gererMouseMove(QMouseEvent* event){
    gp_Pnt currentPoint3D;
    gp_Pnt2d currentPoint2D;

    gp_Pnt TmpPoint3D =  {0,0,0};
    gp_Pnt2d TmpPoint2D = {0,0};

    if (!m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, currentPoint3D)) {
        return false;
    }

    auto* manager = m_Parent->getConstraintManager();

    if (!m_isDrawingLigne) {
        // CAS 1 : Recherche d'aimantation avant le premier clic
        // ici on envoie le point dans le point 2 pour faire le snap, car pas de snap sur le point 1, il n'est
        // utilisé que pour calculer les horizontal vertical etc
        manager->appliqueContraintes2D ( TmpPoint2D, TmpPoint3D, currentPoint2D, currentPoint3D, false, false);
        //std::cout<< " Tool_LineDraw::gererMouseMove " <<currentPoint3D.x <<"," << currentPoint3D.y << std::endl;
    }
    else {
        // CAS 2 : On dessine l'élastique
        // On passe le point de départ, l'aide H/V activée, et en cours de dessin à true
        manager->appliqueContraintes2D ( m_startPoint2D, m_startPoint3D , currentPoint2D, currentPoint3D , true, true);

        // 1. On récupère un pointeur direct sur la distance stockée dans le variant globale de l'outil
        auto* pDescDistance = std::get_if<DimensionEngine::DistanceDescriptor>(&m_currentDimensionDescriptor.data);
        if (!pDescDistance) return false; // Sécurité si le variant ne contient pas une distance
        pDescDistance->pntStop = currentPoint3D;
        m_bDescriptorDefined = true;
        pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
        pDescDistance->offset = 2.0;
        DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry(m_Parent->DocumentRefs.GetSketchPlane(), m_currentDimensionDescriptor);
        m_Parent->m_Cotation->DessinerCotationDepuisResultat(m_Parent->DocumentRefs.GetSketchPlane(), geoResult);


        m_linePoints->SetPoint(1, currentPoint3D.X(), currentPoint3D.Y(), currentPoint3D.Z());
        m_linePoints->Modified();
    }

    m_Parent->GetView()->renderWindow()->Render();
    return true;
}
//─────────────────────────────────────────────────────────────────────
//               MOUSE  RELEASE
//─────────────────────────────────────────────────────────────────────
bool Tool_LineDraw::gererMouseRelease(QMouseEvent* event){
    return false;
}


void Tool_LineDraw::Cotation_Configure (gp_Pnt liMousePos3D){
    DimensionEngine::DistanceDescriptor  DistDesc;
    DistDesc.pntStart = liMousePos3D;
    DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    DistDesc.offset = 0.0;
    m_currentDimensionDescriptor.data = DistDesc;
    m_currentDimensionDescriptor.id = 0;
    m_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    m_Parent->m_Cotation->Afficher ();
}



//─────────────────────────────────────────────────────────────────────
//               MOUSE  PRESS
//─────────────────────────────────────────────────────────────────────
bool Tool_LineDraw::gererMousePress(QMouseEvent* event){
    //gp_Pnt2d m_startPoint2D;
    gp_Pnt TmpPoint3D =  {0,0,0};
    gp_Pnt2d TmpPoint2D = {0,0};

    if (event->button() != Qt::LeftButton) return false;

    auto* manager = m_Parent->getConstraintManager();

    if (!m_isDrawingLigne) {
        // PREMIER CLIC
        m_isDrawingLigne = true;
        m_MousePoint1 = gp_Pnt2d ( event->x(), event->y() );
        if (m_Parent->calculerIntersectionSourisSurPlan(m_MousePoint1.X(), m_MousePoint1.Y(), m_startPoint2D, m_startPoint3D)) {
            // On calcule l'aimantation immédiate du premier point cliqué

            //std::cout<<"Tool_LineDraw::gererMousePress mouse " << m_startPoint2D.X() << "," << m_startPoint2D.Y() <<" " << std::endl;
            manager->appliqueContraintes2D ( TmpPoint2D, TmpPoint3D, m_startPoint2D, m_startPoint3D , false, false);
            //std::cout<<"Tool_LineDraw::gererMousePress snap " << currentPoint2D.X() << "," << currentPoint2D.Y() <<" " << std::endl;

            m_linePoints->SetPoint(0, m_startPoint3D.X(), m_startPoint3D.Y(), m_startPoint3D.Z());
            m_linePoints->SetPoint(1, m_startPoint3D.X(), m_startPoint3D.Y(), m_startPoint3D.Z());
            m_linePoints->Modified();

            m_Point1_2D = m_startPoint2D;
            m_lineActor->SetVisibility(true);
            m_Parent->GetView()->renderWindow()->Render();

            Cotation_Configure (m_startPoint3D);

        }
        return true;
    }
    else {
        // DEUXIÈME CLIC
        m_Parent->m_Cotation->masquerEtVider();
        m_MousePoint2 = gp_Pnt2d ( event->x(), event->y() );

        if (m_Parent->calculerIntersectionSourisSurPlan(m_MousePoint2.X(), m_MousePoint2.Y(), m_Point2_2D, m_endPoint3D)) {
            // Applique les contraintes finales sur le point d'arrivée avant l'enregistrement
            manager->appliqueContraintes2D ( m_startPoint2D, m_startPoint3D, m_Point2_2D, m_endPoint3D, true, true);

            AddLineToOp(m_Point1_2D, m_Point2_2D  );
            m_Parent->rafraichirAffichageEsquisse();
        }
        m_Parent->m_constraintManager->snapPointsVisited_Clean();
        EndDrawLine();
        m_Parent->m_Cotation->masquerEtVider();
        m_Parent->Signaler_ChangementEsquisseIHM ();
        return true;
    }
}
//─────────────────────────────────────────────────────────────────────
//               KEY  RELEASE
//─────────────────────────────────────────────────────────────────────
bool Tool_LineDraw::gererkeyPressEvent(QKeyEvent* event){
    if (event->key() == Qt::Key_Escape) {
        EndDrawLine();
    }
    return false;
}



//─────────────────────────────────────────────────────────────────────
//      ajtouer la ligne à l'opération locale
//─────────────────────────────────────────────────────────────────────
void Tool_LineDraw::AddLineToOp (gp_Pnt2d& StartPoint2D, gp_Pnt2d& StopPoint2D){
    if (!m_Parent->DocumentRefs.GetOperation()) {
        std::cerr << "[ERROR] AddLineToOp: m_Operation est nul !\n";
        return;
    }
    auto* sketchParams = std::get_if<SketchParams>(&m_Parent->DocumentRefs.GetOperation()->getParamsMutable());

    if (!sketchParams) {
        std::cerr << "[ERROR] AddLineToOp: L'opération cible n'est pas un SketchParams.\n";
        return;
    }
    sketchParams->addLine(StartPoint2D, StopPoint2D);
    //SketchLine  Line(StartPoint2D, StopPoint2D);
    //Line.start.Update3D( m_Parent->DocumentRefs.GetSketchPlane() );
    //Line.stop.Update3D( m_Parent->DocumentRefs.GetSketchPlane() );
    //uint64_t l1_id = sketchParams->addPrimitive( Line );
}


void Tool_LineDraw::CADEvent_TraiterCommande(const CadCommandEvent& event){

}


