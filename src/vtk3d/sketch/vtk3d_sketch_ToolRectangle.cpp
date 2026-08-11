
#include "vtk3d_sketch_ToolRectangle.h"

#include "vtk3d_sketch_Tools.h"
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <iostream>
#include <cmath>





//─────────────────────────────────────────────────────────────────────
//               ACTIVATION
//─────────────────────────────────────────────────────────────────────
void Tool_Rectangle::activate() {
    rect_center.m_drawStep = 0;
    rect_edges.m_startPoint3D = gp_Pnt(0, 0, 0);
    rect_edges.m_isDrawingRect = false;

    if (! common.m_Parent->GetView() || ! common.m_Parent->GetView()->renderWindow()) return;

    // Allocation du polygone élastique à 5 points fermés
    common.m_rectPoints = vtkSmartPointer<vtkPoints>::New();
    common.m_rectPolyData = vtkSmartPointer<vtkPolyData>::New();

    for (int i = 0; i < 5; ++i) {
        common.m_rectPoints->InsertNextPoint(0.0, 0.0, 0.0);
    }
    common.m_rectPolyData->SetPoints(common.m_rectPoints);


    auto polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(5);
    for (int i = 0; i < 5; ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);
    common.m_rectPolyData->SetLines(cells);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(common.m_rectPolyData);


    common.m_rectActor = vtkSmartPointer<vtkActor>::New();
    common.m_rectActor->SetMapper(mapper);
    common.m_rectActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    common.m_rectActor->GetProperty()->SetLineWidth(2.0);
    common.m_rectActor->SetVisibility(false);

    common.m_Parent->GetView()->getRenderer()->AddActor(common.m_rectActor);


}



void Tool_Rectangle::desactivate() {
    if (common.m_rectActor) common.m_Parent->GetView()->getRenderer()->RemoveActor(common.m_rectActor);
    common.m_Parent->getSnapperManager()->masquerFeedback();
}


//─────────────────────────────────────────────────────────────────────
//     fin de dessin du rectangle
//─────────────────────────────────────────────────────────────────────
void Tool_Rectangle::EndDrawRectangle() {
    switch ( common.m_SubMode )
    {
        case CadEvent::Sketch::RectangleSubMode::ByCenter:
            if (rect_center.m_drawStep > 0) {
                rect_center.m_drawStep = 0;
            }
            break;
        case CadEvent::Sketch::RectangleSubMode::ByEdges:
            if (rect_edges.m_isDrawingRect) {
                rect_edges.m_isDrawingRect = false;
            }
            break;
    };
    common.m_rectActor->SetVisibility(false);
    auto* manager = common.m_Parent->getSnapperManager();
    manager->masquerFeedback();
    common.m_Parent->GetView()->renderWindow()->Render();
}




bool Tool_Rectangle::keyPressEvent(QKeyEvent* event)   { return false; }
bool Tool_Rectangle::gererWheelEvent(QWheelEvent* event) { return false; }
bool Tool_Rectangle::gererMouseRelease(QMouseEvent* event) { return false; }



//─────────────────────────────────────────────────────────────────────
// mettre ici tous les acteurs qui ont besion d'être redimensionnés en fonction du zoom
// cette fonction est call par la classe mere sketch
//─────────────────────────────────────────────────────────────────────
void Tool_Rectangle::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    if (!common.m_snapPointActor || !common.m_snapPointActor->GetVisibility() ) return;
    common.m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}





//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_Rectangle::gererMouseMove(QMouseEvent* event) {

    if ( CadEvent::Sketch::RectangleSubMode::ByCenter == common.m_SubMode )
    {
        RectByCenter_gererMouseMove ( event );
    }else if (CadEvent::Sketch::RectangleSubMode::ByEdges == common.m_SubMode ){
        RectByEdge_gererMouseMove (event);
    }
    common.m_Parent->GetView()->renderWindow()->Render();
    return true;
}


//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_Rectangle::RectByEdge_gererMouseMove(QMouseEvent* event) {
    gp_Pnt currentPoint3D;
    gp_Pnt2d currentPoint2D;

    if (!common.m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, currentPoint3D)) {
        return false;
    }

    auto* manager = common.m_Parent->getSnapperManager();

    if (! rect_edges.m_isDrawingRect) {
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
        rect_edges.m_Cotation1_DistDesc.pntStop = currentPoint3D;
        rect_edges.m_Cotation2_bDescriptorDefined = true;
        rect_edges.m_Cotation1_currentDimensionDescriptor.data = rect_edges.m_Cotation1_DistDesc;
        DimensionEngine::GeometryResult Cot1_geoResult = DimensionEngine::ComputeGeometry( common.m_Parent->PartRefs.GetSketchPlane(), rect_edges.m_Cotation1_currentDimensionDescriptor);
        common.m_Parent->m_Cotation2->DessinerCotationDepuisResultat(common.m_Parent->PartRefs.GetSketchPlane(), Cot1_geoResult);


        // On reconstruit la boîte 2D projetée sur le plan 3D d'esquisse (z constant ou plan local)
        common.m_rectPoints->SetPoint(0, rect_edges.m_startPoint3D.X(),     rect_edges.m_startPoint3D.Y(),  rect_edges.m_startPoint3D.Z() ); // Coin d'origine 1
        common.m_rectPoints->SetPoint(1, currentPoint3D.X(),                rect_edges.m_startPoint3D.Y(),  rect_edges.m_startPoint3D.Z() ); // Coin supérieur opposé
        common.m_rectPoints->SetPoint(2, currentPoint3D.X(),                currentPoint3D.Y(),             currentPoint3D.Z() ); // Coin d'arrivée 2
        common.m_rectPoints->SetPoint(3, rect_edges.m_startPoint3D.X(),     currentPoint3D.Y(),             currentPoint3D.Z() ); // Coin inférieur opposé
        common.m_rectPoints->SetPoint(4, rect_edges.m_startPoint3D.X(),     rect_edges.m_startPoint3D.Y(),  rect_edges.m_startPoint3D.Z() ); // Fermeture du polygone

        common.m_rectPoints->Modified();
    }

}

//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_Rectangle::RectByCenter_gererMouseMove(QMouseEvent* event) {
    gp_Pnt2d currentPoint2D;

    gp_Pnt currentPoint3D;
    if (! common.m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, currentPoint3D)) {
        return false;
    }

    auto* manager = common.m_Parent->getSnapperManager();

    if (rect_center.m_drawStep == 0) {
        // 🟩 RECHERCHE DU CENTRE
        //manager->appliqueContraintes2D({0,0,0}, currentPoint3D, false, false);
    }
    else if (rect_center.m_drawStep == 1) {
        // 🟦 ÉTAPE 1 : Le centre est fixe, on étire la LARGEUR et l'ANGLE (Hauteur = 0)
        //manager->appliqueContraintes2D(m_centerPoint3D, currentPoint3D, true, true);

        // Vecteur demi-largeur orienté
        gp_Vec v = gp_Vec (currentPoint3D, rect_center.m_centerPoint3D );

        // Le rectangle est représenté par un simple segment plat centré
        common.m_rectPoints->SetPoint(0, rect_center.m_centerPoint3D.X() + v.X(), rect_center.m_centerPoint3D.Y() + v.Y(), rect_center.m_centerPoint3D.Z() + v.Z());
        common.m_rectPoints->SetPoint(1, rect_center.m_centerPoint3D.X() - v.X(), rect_center.m_centerPoint3D.Y() - v.Y(), rect_center.m_centerPoint3D.Z() - v.Z());
        common.m_rectPoints->SetPoint(2, rect_center.m_centerPoint3D.X() - v.X(), rect_center.m_centerPoint3D.Y() - v.Y(), rect_center.m_centerPoint3D.Z() - v.Z());
        common.m_rectPoints->SetPoint(3, rect_center.m_centerPoint3D.X() + v.X(), rect_center.m_centerPoint3D.Y() + v.Y(), rect_center.m_centerPoint3D.Z() + v.Z());
        common.m_rectPoints->SetPoint(4, rect_center.m_centerPoint3D.X() + v.X(), rect_center.m_centerPoint3D.Y() + v.Y(), rect_center.m_centerPoint3D.Z() + v.Z());
        common.m_rectPoints->Modified();

        gp_Vec TmpVect ( rect_center.m_centerPoint3D, currentPoint3D );
        gp_Pnt pntExtremiteOpposee = rect_center.m_centerPoint3D.Translated(-TmpVect);
        RectByCenter_Cotation_Configure ( pntExtremiteOpposee  );

        // 1. On récupère un pointeur direct sur la distance stockée dans le variant globale de l'outil
        auto* pDescDistance = std::get_if<DimensionEngine::DistanceDescriptor>(&rect_center.m_currentDimensionDescriptor.data);
        if (!pDescDistance) return false; // Sécurité si le variant ne contient pas une distance
        pDescDistance->pntStop = currentPoint3D;
        rect_center.m_bDescriptorDefined = true;
        pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
        pDescDistance->offset = 2.0;
        DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry( common.m_Parent->PartRefs.GetSketchPlane(), rect_center.m_currentDimensionDescriptor);
        common.m_Parent->m_Cotation->DessinerCotationDepuisResultat( common.m_Parent->PartRefs.GetSketchPlane(), geoResult);
    }
    else if (rect_center.m_drawStep == 2) {
        // 🟨 ÉTAPE 2 : Largeur et Angle figés, on règle la HAUTEUR

        // Vecteur 3D reliant le Centre au point de Largeur validé au clic précédent
        gp_Vec v(rect_center.m_centerPoint3D, rect_center.m_widthPoint3D);
        double lenX = v.Magnitude(); // Véritable longueur 3D (remplace hypot)

        if (lenX > 1e-6) {
            // 1. On extrait la direction (unitaire) de notre axe X local
            gp_Dir localX(v);

            // 2. On récupère la normale 3D de ton plan d'esquisse (Axe Z local)
            gp_Dir planeNormal = common.m_Parent->PartRefs.GetSketchPlane().Direction();

            // 3. Produit Vectoriel (Z_local ^ X_local = Y_local) pour obtenir l'axe perpendiculaire exact en 3D
            gp_Dir localY = planeNormal ^ localX;

            // 4. Projection du vecteur de la souris [Centre -> Souris] sur l'axe Y local (via Produit Scalaire .Dot)
            gp_Vec w(rect_center.m_centerPoint3D, currentPoint3D);
            double h = w.Dot(localY); // Hauteur scalaire (positive ou négative selon le côté)

            // 5. Création du vecteur Hauteur 3D final
            gp_Vec vecH = gp_Vec(localY) * h;

            // 6. Calcul géométrique pur des 4 coins en espace 3D absolu
            gp_Pnt p0 = rect_center.m_centerPoint3D.XYZ() + v.XYZ() + vecH.XYZ();
            gp_Pnt p1 = rect_center.m_centerPoint3D.XYZ() - v.XYZ() + vecH.XYZ();
            gp_Pnt p2 = rect_center.m_centerPoint3D.XYZ() - v.XYZ() - vecH.XYZ();
            gp_Pnt p3 = rect_center.m_centerPoint3D.XYZ() + v.XYZ() - vecH.XYZ();

            common.m_rectPoints->SetPoint(0, p0.X(), p0.Y(), p0.Z());
            common.m_rectPoints->SetPoint(1, p1.X(), p1.Y(), p1.Z());
            common.m_rectPoints->SetPoint(2, p2.X(), p2.Y(), p2.Z());
            common.m_rectPoints->SetPoint(3, p3.X(), p3.Y(), p3.Z());
            common.m_rectPoints->SetPoint(4, p0.X(), p0.Y(), p0.Z()); // Fermeture du rectangle

            common.m_rectPoints->Modified();



            gp_Vec TmpVect ( rect_center.m_centerPoint3D, rect_center.m_widthPoint3D );
            gp_Vec TmpVect2 ( rect_center.m_widthPoint3D, currentPoint3D );
            gp_Pnt pntExtremiteOpposee = rect_center.m_centerPoint3D.Translated(-TmpVect);
            pntExtremiteOpposee = pntExtremiteOpposee.Translated(vecH);
            RectByCenter_Cotation_Configure ( pntExtremiteOpposee  );

            auto* pDescDistance = std::get_if<DimensionEngine::DistanceDescriptor>(&rect_center.m_currentDimensionDescriptor.data);
            if (!pDescDistance) return false; // Sécurité si le variant ne contient pas une distance

            gp_Pnt pntExtremite = rect_center.m_centerPoint3D.Translated(TmpVect);
            pntExtremite = pntExtremite.Translated(vecH);
            pDescDistance->pntStop = pntExtremite;
            rect_center.m_bDescriptorDefined = true;
            pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
            pDescDistance->offset = 2.0;
            DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry(common.m_Parent->PartRefs.GetSketchPlane(), rect_center.m_currentDimensionDescriptor);
            common.m_Parent->m_Cotation->DessinerCotationDepuisResultat(common.m_Parent->PartRefs.GetSketchPlane(), geoResult);

            gp_Vec TmpVectCot2 ( rect_center.m_widthPoint3D, currentPoint3D );
            gp_Pnt pntCot2_Start = pntExtremite;
            gp_Pnt pntCot2_End = pntCot2_Start.Translated( -2 * vecH);
            rect_center.m_Cotation2_bDescriptorDefined = true;
            RectByCenter_Cotation2_Configure (pntCot2_Start, pntCot2_End );
            DimensionEngine::GeometryResult Cot2_geoResult = DimensionEngine::ComputeGeometry(common.m_Parent->PartRefs.GetSketchPlane(), rect_center.m_Cotation2_currentDimensionDescriptor);
            common.m_Parent->m_Cotation2->DessinerCotationDepuisResultat(common.m_Parent->PartRefs.GetSketchPlane(), Cot2_geoResult);
        }
    }
}

void Tool_Rectangle::RectByCenter_Cotation_Configure (gp_Pnt liMousePos3D){
    DimensionEngine::DistanceDescriptor  DistDesc;
    DistDesc.pntStart = liMousePos3D;
    DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    DistDesc.offset = 0.0;
    rect_center.m_currentDimensionDescriptor.data = DistDesc;
    rect_center.m_currentDimensionDescriptor.id = 0;
    rect_center.m_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    common.m_Parent->m_Cotation->Afficher ();
}
void Tool_Rectangle::RectByCenter_Cotation2_Configure (gp_Pnt Li_Start3D, gp_Pnt li_Stop3D){
    DimensionEngine::DistanceDescriptor  Dist2Desc;
    Dist2Desc.pntStart = Li_Start3D;
    Dist2Desc.pntStop = li_Stop3D;
    Dist2Desc.mode = DimensionEngine::DimMode::PointToPoint;
    Dist2Desc.offset = 2.0;
    rect_center.m_Cotation2_currentDimensionDescriptor.data = Dist2Desc;
    rect_center.m_Cotation2_currentDimensionDescriptor.id = 0;
    rect_center.m_Cotation2_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    common.m_Parent->m_Cotation2->Afficher ();
}


void Tool_Rectangle::RectByEdge_Cotation1_Configure (gp_Pnt liMousePos3D){
    rect_edges.m_Cotation1_DistDesc.pntStart = liMousePos3D;
    rect_edges.m_Cotation1_DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    rect_edges.m_Cotation1_DistDesc.offset = 2.0;
    rect_edges.m_Cotation1_currentDimensionDescriptor.data = rect_edges.m_Cotation1_DistDesc;
    rect_edges.m_Cotation1_currentDimensionDescriptor.id = 0;
    rect_edges.m_Cotation2_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    common.m_Parent->m_Cotation->Afficher ();
}

void Tool_Rectangle::RectByEdge_Cotation2_Configure (gp_Pnt liMousePos3D){
    rect_edges.m_Cotation2_DistDesc.pntStart = liMousePos3D;
    rect_edges.m_Cotation2_DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    rect_edges.m_Cotation2_DistDesc.offset = 2.0;
    rect_edges.m_Cotation2_currentDimensionDescriptor.data = rect_edges.m_Cotation2_DistDesc;
    rect_edges.m_Cotation2_currentDimensionDescriptor.id = 0;
    rect_edges.m_Cotation2_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    common.m_Parent->m_Cotation->Afficher ();
}



//─────────────────────────────────────────────────────────────────────
//               MOUSE  PRESS
//─────────────────────────────────────────────────────────────────────
bool Tool_Rectangle::gererMousePress(QMouseEvent* event) {
    gp_Pnt2d clickedPoint2D;

    if (event->button() != Qt::LeftButton) return false;

    auto* manager = common.m_Parent->getSnapperManager();
    gp_Pnt clickedPoint3D;

    if (!common.m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), clickedPoint2D, clickedPoint3D)) {
        return false;
    }



    if ( CadEvent::Sketch::RectangleSubMode::ByCenter == common.m_SubMode )
    {



        if (rect_center.m_drawStep == 0) {
            // 🟩 CLIC 1 : Fixation du centre
            //manager->appliqueContraintes2D({0,0,0}, clickedPoint3D, false, false);
            rect_center.m_centerPoint3D = clickedPoint3D;
            rect_center.m_centerPoint2D = clickedPoint2D;

            rect_center.m_drawStep = 1;
            common.m_rectActor->SetVisibility(true);
        }
        else if (rect_center.m_drawStep == 1) {
            // 🟦 CLIC 2 : Fixation de la Largeur et de l'Angle
            //manager->appliqueContraintes2D(m_centerPoint3D, clickedPoint3D, true, true);
            rect_center.m_widthPoint3D = clickedPoint3D;
            rect_center.m_widthPoint2D = clickedPoint2D;
            rect_center.m_drawStep = 2;

            gp_Vec TmpVect ( rect_center.m_centerPoint3D, rect_center.m_widthPoint3D );
            gp_Pnt pntExtremiteOpposee = rect_center.m_centerPoint3D.Translated(-TmpVect);
            RectByCenter_Cotation_Configure ( pntExtremiteOpposee  );

        }
        else if (rect_center.m_drawStep == 2) {
            // 🟨 CLIC 3 : Fixation de la Hauteur et Envoi final au modèle CAO
            //manager->appliqueContraintes2D(m_widthPoint3D, clickedPoint3D, true, true);
            rect_center.m_heightPoint3D = clickedPoint3D;
            rect_center.m_heightPoint2D = clickedPoint2D;

            common.m_Parent->m_Cotation->masquerEtVider();
            common.m_Parent->m_Cotation2->masquerEtVider();

            // Récupération des coordonnées des sommets calculées dans le vtkPoints
            double p0[3], p1[3], p2[3], p3[3];
            common.m_rectPoints->GetPoint(0, p0);
            common.m_rectPoints->GetPoint(1, p1);
            common.m_rectPoints->GetPoint(2, p2);
            common.m_rectPoints->GetPoint(3, p3);

            gp_Pln plan(common.m_Parent->PartRefs.GetSketchPlane());

            gp_Pnt p0_3d(p0[0], p0[1], p0[2]);
            gp_Pnt p1_3d(p1[0], p1[1], p1[2]);
            gp_Pnt p2_3d(p2[0], p2[1], p2[2]);
            gp_Pnt p3_3d(p3[0], p3[1], p3[2]);

            double u = 0.0, v = 0.0;

            ElSLib::Parameters(plan, p0_3d, u, v);
            gp_Pnt2d Pnt_A(u, v);

            ElSLib::Parameters(plan, p1_3d, u, v);
            gp_Pnt2d Pnt_B(u, v);

            ElSLib::Parameters(plan, p2_3d, u, v);
            gp_Pnt2d Pnt_C(u, v);

            ElSLib::Parameters(plan, p3_3d, u, v);
            gp_Pnt2d Pnt_D(u, v);

            AddCenterRectangleToOp(Pnt_A, Pnt_B, Pnt_C, Pnt_D );

            common.m_Parent->rafraichirAffichageEsquisse();
            EndDrawRectangle();
            common.m_Parent->Signaler_ChangementEsquisseIHM ();
        }
    }else if (CadEvent::Sketch::RectangleSubMode::ByEdges == common.m_SubMode ){
        gp_Pnt2d currentPoint2D;

        if (event->button() != Qt::LeftButton) return false;

        auto* manager = common.m_Parent->getSnapperManager();

        if (! rect_edges.m_isDrawingRect) {
            // 🟩 PREMIER CLIC : Premier coin de la boîte
            rect_edges.m_isDrawingRect = true;
            //m_RectStart2D.SetCoord(static_cast<double>(event->x()), static_cast<double>(event->y())  );

            if (common.m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), rect_edges.m_RectStart2D, rect_edges.m_startPoint3D)) {
                //manager->appliqueContraintes2D({0,0,0}, m_startPoint3D, false, false);

                // Initialisation de l'élastique à plat sur le point d'ancrage
                for (int i = 0; i < 5; ++i) {
                    common.m_rectPoints->SetPoint(i, rect_edges.m_startPoint3D.X(), rect_edges.m_startPoint3D.Y(), rect_edges.m_startPoint3D.Z());
                }
                common.m_rectPoints->Modified();

                rect_edges.m_point1_3D = rect_edges.m_startPoint3D;
                common.m_rectActor->SetVisibility(true);
                common.m_Parent->GetView()->renderWindow()->Render();
                RectByEdge_Cotation1_Configure ( rect_edges.m_startPoint3D );
                RectByEdge_Cotation2_Configure ( rect_edges.m_startPoint3D );

            }
            return true;
        }
        else {
            // 🟦 DEUXIÈME CLIC : Coin diagonal opposé, validation géométrique

            if (common.m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), rect_edges.m_RectEnd2D, rect_edges.m_point2_3D)) {
                AddRectangleToOp(rect_edges.m_RectStart2D, rect_edges.m_RectEnd2D);
                common.m_Parent->rafraichirAffichageEsquisse();
                common.m_Parent->Signaler_ChangementEsquisseIHM ();
            }

            EndDrawRectangle();
            return true;
        }
    }
    common.m_Parent->GetView()->renderWindow()->Render();
    return true;
}







bool Tool_Rectangle::gererkeyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        EndDrawRectangle();
    }
    return false;
}


// ############# A MUTUALISER #####################
//─────────────────────────────────────────────────────────────────────
//      Ajouter le rectangle sous forme de 4 SketchLine distinctes
//─────────────────────────────────────────────────────────────────────
void Tool_Rectangle::AddCenterRectangleToOp(const gp_Pnt2d& li_PA, const gp_Pnt2d& li_PB, const gp_Pnt2d& li_PC, const gp_Pnt2d& li_PD) {
    if (!common.m_Parent->PartRefs.GetOperation()) return;
    auto* sketchParams = common.m_Parent->PartRefs.GetParams();
    if (!sketchParams) {
        std::cerr << "[ERROR] AddRectangleToOp: L'opération cible n'est pas un SketchParams.\n";
        return;
    }

    sketchParams->addLine(li_PA, li_PB);
    sketchParams->addLine(li_PB, li_PC);
    sketchParams->addLine(li_PC, li_PD);
    sketchParams->addLine(li_PD, li_PA);
}

//─────────────────────────────────────────────────────────────────────
//      Ajouter le rectangle à l'opération locale OpenCascade
//─────────────────────────────────────────────────────────────────────
void Tool_Rectangle::AddRectangleToOp(gp_Pnt2d& p1_2D, gp_Pnt2d& p2_2D) {
    if (!common.m_Parent->PartRefs.GetOperation()) {
        std::cerr << "[ERROR] AddRectangleToOp: m_Operation est nul !\n";
        return;
    }
    auto* sketchParams = common.m_Parent->PartRefs.GetParams();

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
}

void Tool_Rectangle::CADEvent_TraiterCommande(const CadCommandEvent& event){

}





