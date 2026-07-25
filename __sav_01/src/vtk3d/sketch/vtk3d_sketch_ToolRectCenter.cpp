
#include "vtk3d_sketch_Tools.h"
#include "vtk3d_sketch_ToolRectCenter.h"
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
void Tool_RectCenterDraw::activate() {
    m_drawStep = 0;

    if (!m_Parent->GetView() || !m_Parent->GetView()->renderWindow()) return;

    // Allocation du polygone élastique à 5 points fermés
    m_rectPoints = vtkSmartPointer<vtkPoints>::New();
    m_rectPolyData = vtkSmartPointer<vtkPolyData>::New();
    
    for (int i = 0; i < 5; ++i) {
        m_rectPoints->InsertNextPoint(0.0, 0.0, 0.0);
    }
    m_rectPolyData->SetPoints(m_rectPoints);

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
    m_rectActor->GetProperty()->SetColor(1.0, 0.0, 0.0); 
    m_rectActor->GetProperty()->SetLineWidth(2.0);
    m_rectActor->SetVisibility(false);

    m_Parent->GetView()->getRenderer()->AddActor(m_rectActor);
}

void Tool_RectCenterDraw::desactivate() {
    if (m_rectActor) m_Parent->GetView()->getRenderer()->RemoveActor(m_rectActor);
    m_Parent->getConstraintManager()->masquerFeedback();
}

void Tool_RectCenterDraw::EndDrawRectangle() {
    if (m_drawStep > 0) {
        m_drawStep = 0;
        m_rectActor->SetVisibility(false);
        auto* manager = m_Parent->getConstraintManager();
        manager->masquerFeedback();
        m_Parent->GetView()->renderWindow()->Render();
    }
}

bool Tool_RectCenterDraw::keyPressEvent(QKeyEvent* event)   { return false; }
bool Tool_RectCenterDraw::gererWheelEvent(QWheelEvent* event) { return false; }
bool Tool_RectCenterDraw::gererMouseRelease(QMouseEvent* event) { return false; }

//─────────────────────────────────────────────────────────────────────
// mettre ici tous les acteurs qui ont besion d'être redimensionnés en fonction du zoom
// cette fonction est call par la classe mere sketch
//─────────────────────────────────────────────────────────────────────
void Tool_RectCenterDraw::ajusterEchelleElements( double li_echelle){
    double facteurEchelle = li_echelle * 0.05;
    if (!m_snapPointActor || !m_snapPointActor->GetVisibility() ) return;
    m_snapPointActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}
//─────────────────────────────────────────────────────────────────────
//               MOUSE  MOVE
//─────────────────────────────────────────────────────────────────────
bool Tool_RectCenterDraw::gererMouseMove(QMouseEvent* event) {
    gp_Pnt2d currentPoint2D;

    gp_Pnt currentPoint3D;
    if (!m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), currentPoint2D, currentPoint3D)) {
        return false;
    }

    auto* manager = m_Parent->getConstraintManager();

    if (m_drawStep == 0) {
        // 🟩 RECHERCHE DU CENTRE
        //manager->appliqueContraintes2D({0,0,0}, currentPoint3D, false, false);
    }
    else if (m_drawStep == 1) {
        // 🟦 ÉTAPE 1 : Le centre est fixe, on étire la LARGEUR et l'ANGLE (Hauteur = 0)
        //manager->appliqueContraintes2D(m_centerPoint3D, currentPoint3D, true, true);

        // Vecteur demi-largeur orienté
        //gp_Pnt v = currentPoint3D - m_centerPoint3D;
        gp_Vec v = gp_Vec (currentPoint3D, m_centerPoint3D );

        // Le rectangle est représenté par un simple segment plat centré
        m_rectPoints->SetPoint(0, m_centerPoint3D.X() + v.X(), m_centerPoint3D.Y() + v.Y(), m_centerPoint3D.Z() + v.Z());
        m_rectPoints->SetPoint(1, m_centerPoint3D.X() - v.X(), m_centerPoint3D.Y() - v.Y(), m_centerPoint3D.Z() - v.Z());
        m_rectPoints->SetPoint(2, m_centerPoint3D.X() - v.X(), m_centerPoint3D.Y() - v.Y(), m_centerPoint3D.Z() - v.Z());
        m_rectPoints->SetPoint(3, m_centerPoint3D.X() + v.X(), m_centerPoint3D.Y() + v.Y(), m_centerPoint3D.Z() + v.Z());
        m_rectPoints->SetPoint(4, m_centerPoint3D.X() + v.X(), m_centerPoint3D.Y() + v.Y(), m_centerPoint3D.Z() + v.Z());
        m_rectPoints->Modified();

        gp_Vec TmpVect ( m_centerPoint3D, currentPoint3D );
        gp_Pnt pntExtremiteOpposee = m_centerPoint3D.Translated(-TmpVect);
        Cotation_Configure ( pntExtremiteOpposee  );

        // 1. On récupère un pointeur direct sur la distance stockée dans le variant globale de l'outil
        auto* pDescDistance = std::get_if<DimensionEngine::DistanceDescriptor>(&m_currentDimensionDescriptor.data);
        if (!pDescDistance) return false; // Sécurité si le variant ne contient pas une distance
        pDescDistance->pntStop = currentPoint3D;
        m_bDescriptorDefined = true;
        pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
        pDescDistance->offset = 2.0;
        DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry(m_Parent->GetSketchPlane(), m_currentDimensionDescriptor);
        m_Parent->m_Cotation->DessinerCotationDepuisResultat(m_Parent->GetSketchPlane(), geoResult);
    }
    else if (m_drawStep == 2) {
        // 🟨 ÉTAPE 2 : Largeur et Angle figés, on règle la HAUTEUR

        // Vecteur 3D reliant le Centre au point de Largeur validé au clic précédent
        gp_Vec v(m_centerPoint3D, m_widthPoint3D);
        double lenX = v.Magnitude(); // Véritable longueur 3D (remplace hypot)

        if (lenX > 1e-6) {
            // 1. On extrait la direction (unitaire) de notre axe X local
            gp_Dir localX(v);

            // 2. On récupère la normale 3D de ton plan d'esquisse (Axe Z local)
            gp_Dir planeNormal = m_Parent->GetSketchPlane().Direction();

            // 3. Produit Vectoriel (Z_local ^ X_local = Y_local) pour obtenir l'axe perpendiculaire exact en 3D
            gp_Dir localY = planeNormal ^ localX;

            // 4. Projection du vecteur de la souris [Centre -> Souris] sur l'axe Y local (via Produit Scalaire .Dot)
            gp_Vec w(m_centerPoint3D, currentPoint3D);
            double h = w.Dot(localY); // Hauteur scalaire (positive ou négative selon le côté)

            // 5. Création du vecteur Hauteur 3D final
            gp_Vec vecH = gp_Vec(localY) * h;

            // 6. Calcul géométrique pur des 4 coins en espace 3D absolu
            gp_Pnt p0 = m_centerPoint3D.XYZ() + v.XYZ() + vecH.XYZ();
            gp_Pnt p1 = m_centerPoint3D.XYZ() - v.XYZ() + vecH.XYZ();
            gp_Pnt p2 = m_centerPoint3D.XYZ() - v.XYZ() - vecH.XYZ();
            gp_Pnt p3 = m_centerPoint3D.XYZ() + v.XYZ() - vecH.XYZ();

            m_rectPoints->SetPoint(0, p0.X(), p0.Y(), p0.Z());
            m_rectPoints->SetPoint(1, p1.X(), p1.Y(), p1.Z());
            m_rectPoints->SetPoint(2, p2.X(), p2.Y(), p2.Z());
            m_rectPoints->SetPoint(3, p3.X(), p3.Y(), p3.Z());
            m_rectPoints->SetPoint(4, p0.X(), p0.Y(), p0.Z()); // Fermeture du rectangle

            m_rectPoints->Modified();



            gp_Vec TmpVect ( m_centerPoint3D, m_widthPoint3D );
            gp_Vec TmpVect2 ( m_widthPoint3D, currentPoint3D );
            gp_Pnt pntExtremiteOpposee = m_centerPoint3D.Translated(-TmpVect);
            pntExtremiteOpposee = pntExtremiteOpposee.Translated(vecH);
            Cotation_Configure ( pntExtremiteOpposee  );

            auto* pDescDistance = std::get_if<DimensionEngine::DistanceDescriptor>(&m_currentDimensionDescriptor.data);
            if (!pDescDistance) return false; // Sécurité si le variant ne contient pas une distance

            gp_Pnt pntExtremite = m_centerPoint3D.Translated(TmpVect);
            pntExtremite = pntExtremite.Translated(vecH);
            pDescDistance->pntStop = pntExtremite;
            m_bDescriptorDefined = true;
            pDescDistance->mode = DimensionEngine::DimMode::PointToPoint;
            pDescDistance->offset = 2.0;
            DimensionEngine::GeometryResult geoResult = DimensionEngine::ComputeGeometry(m_Parent->GetSketchPlane(), m_currentDimensionDescriptor);
            m_Parent->m_Cotation->DessinerCotationDepuisResultat(m_Parent->GetSketchPlane(), geoResult);

            gp_Vec TmpVectCot2 ( m_widthPoint3D, currentPoint3D );
            gp_Pnt pntCot2_Start = pntExtremite;
            gp_Pnt pntCot2_End = pntCot2_Start.Translated( -2 * vecH);
            m_Cotation2_bDescriptorDefined = true;
            Cotation2_Configure (pntCot2_Start, pntCot2_End );
            DimensionEngine::GeometryResult Cot2_geoResult = DimensionEngine::ComputeGeometry(m_Parent->GetSketchPlane(), m_Cotation2_currentDimensionDescriptor);
            m_Parent->m_Cotation2->DessinerCotationDepuisResultat(m_Parent->GetSketchPlane(), Cot2_geoResult);


        }



    }

    m_Parent->GetView()->renderWindow()->Render();
    return true;
}

void Tool_RectCenterDraw::Cotation_Configure (gp_Pnt liMousePos3D){
    DimensionEngine::DistanceDescriptor  DistDesc;
    DistDesc.pntStart = liMousePos3D;
    DistDesc.mode = DimensionEngine::DimMode::PointToPoint;
    DistDesc.offset = 0.0;
    m_currentDimensionDescriptor.data = DistDesc;
    m_currentDimensionDescriptor.id = 0;
    m_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    m_Parent->m_Cotation->Afficher ();
}
void Tool_RectCenterDraw::Cotation2_Configure (gp_Pnt Li_Start3D, gp_Pnt li_Stop3D){
    DimensionEngine::DistanceDescriptor  Dist2Desc;
    Dist2Desc.pntStart = Li_Start3D;
    Dist2Desc.pntStop = li_Stop3D;
    Dist2Desc.mode = DimensionEngine::DimMode::PointToPoint;
    Dist2Desc.offset = 2.0;
    m_Cotation2_currentDimensionDescriptor.data = Dist2Desc;
    m_Cotation2_currentDimensionDescriptor.id = 0;
    m_Cotation2_bDescriptorDefined = false; // Flag à mettre dans votre Tool_Dimensions.h
    m_Parent->m_Cotation2->Afficher ();
}






//─────────────────────────────────────────────────────────────────────
//               MOUSE  PRESS
//─────────────────────────────────────────────────────────────────────
bool Tool_RectCenterDraw::gererMousePress(QMouseEvent* event) {
    gp_Pnt2d clickedPoint2D;

    if (event->button() != Qt::LeftButton) return false;

    auto* manager = m_Parent->getConstraintManager();
    gp_Pnt clickedPoint3D;

    if (!m_Parent->calculerIntersectionSourisSurPlan(event->x(), event->y(), clickedPoint2D, clickedPoint3D)) {
        return false;
    }

    if (m_drawStep == 0) {
        // 🟩 CLIC 1 : Fixation du centre
        //manager->appliqueContraintes2D({0,0,0}, clickedPoint3D, false, false);
        m_centerPoint3D = clickedPoint3D;
        m_centerPoint2D = clickedPoint2D;
        
        m_drawStep = 1;
        m_rectActor->SetVisibility(true);


    }
    else if (m_drawStep == 1) {
        // 🟦 CLIC 2 : Fixation de la Largeur et de l'Angle
        //manager->appliqueContraintes2D(m_centerPoint3D, clickedPoint3D, true, true);
        m_widthPoint3D = clickedPoint3D;
        m_widthPoint2D = clickedPoint2D;
        m_drawStep = 2;

        gp_Vec TmpVect ( m_centerPoint3D, m_widthPoint3D );
        gp_Pnt pntExtremiteOpposee = m_centerPoint3D.Translated(-TmpVect);
        Cotation_Configure ( pntExtremiteOpposee  );

    }
    else if (m_drawStep == 2) {
        // 🟨 CLIC 3 : Fixation de la Hauteur et Envoi final au modèle CAO
        //manager->appliqueContraintes2D(m_widthPoint3D, clickedPoint3D, true, true);
        m_heightPoint3D = clickedPoint3D;
        m_heightPoint2D = clickedPoint2D;

        m_Parent->m_Cotation->masquerEtVider();
        m_Parent->m_Cotation2->masquerEtVider();

        // Récupération des coordonnées des sommets calculées dans le vtkPoints
        double p0[3], p1[3], p2[3], p3[3];
        m_rectPoints->GetPoint(0, p0);
        m_rectPoints->GetPoint(1, p1);
        m_rectPoints->GetPoint(2, p2);
        m_rectPoints->GetPoint(3, p3);

        gp_Pln plan(m_Parent->GetSketchPlane());

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

        m_Parent->rafraichirAffichageEsquisse();
        EndDrawRectangle();
    }

    m_Parent->GetView()->renderWindow()->Render();
    return true;
}

bool Tool_RectCenterDraw::gererkeyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        EndDrawRectangle();
    }
    return false;
}

//─────────────────────────────────────────────────────────────────────
//      Ajouter le rectangle sous forme de 4 SketchLine distinctes
//─────────────────────────────────────────────────────────────────────
void Tool_RectCenterDraw::AddCenterRectangleToOp(const gp_Pnt2d& li_PA, const gp_Pnt2d& li_PB, const gp_Pnt2d& li_PC, const gp_Pnt2d& li_PD) {
    if (!m_Parent->m_Operation) return;
    auto* sketchParams = std::get_if<SketchParams>(&m_Parent->m_Operation->getParamsMutable());
    if (!sketchParams) return;

    SketchLine L_AB(li_PA, li_PB);
    L_AB.start.Update3D(m_Parent->GetSketchPlane() );
    L_AB.stop.Update3D(m_Parent->GetSketchPlane() );

    SketchLine L_BC(li_PB, li_PC);
    L_BC.start.Update3D(m_Parent->GetSketchPlane() );
    L_BC.stop.Update3D(m_Parent->GetSketchPlane() );

    SketchLine L_CD(li_PC, li_PD);
    L_CD.start.Update3D(m_Parent->GetSketchPlane() );
    L_CD.stop.Update3D(m_Parent->GetSketchPlane() );

    SketchLine L_DA(li_PD, li_PA);
    L_DA.start.Update3D(m_Parent->GetSketchPlane() );
    L_DA.stop.Update3D(m_Parent->GetSketchPlane() );

    // Injection des 4 lignes constituant le rectangle incliné
    sketchParams->addPrimitive( L_AB );
    sketchParams->addPrimitive( L_BC );
    sketchParams->addPrimitive( L_CD );
    sketchParams->addPrimitive( L_DA );
}


void Tool_RectCenterDraw::CADEvent_TraiterCommande(const CadCommandEvent& event){

}


