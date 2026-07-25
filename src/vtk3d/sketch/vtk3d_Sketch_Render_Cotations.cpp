#include "vtk3d_Sketch_Render_Cotations.h"
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkLine.h>
#include <vtkCoordinate.h>
#include <vtkRenderWindow.h>
#include <cmath>
#include <QString>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax3.hxx>
#include <cmath>


struct StrCotationDistanceType {
    struct ligne{
        gp_Pnt Pnt3DStart;
        gp_Pnt Pnt3DStop;
    };
    ligne ligne_para;
    ligne ligne_PerpStart;
    ligne ligne_PerpStop;
};



vtk3d_Sketch_Render_Cotations::vtk3d_Sketch_Render_Cotations(vtkRenderer* renderer)
    : m_renderer(renderer) {
    // Configurer les propriétés graphiques par défaut de l'acteur temporaire
    m_tempActor->GetProperty()->SetColor(0.2, 0.2, 0.2);
    m_tempActor->GetProperty()->SetLineWidth(1.0);

    //RefLine.ref_defined = false;

    m_tempPoints->Reset();
    m_tempLines->Reset();
    m_tempPolyData->AllocateExact(0, 0); // Vide le polydata proprement

    m_tempPolyData->SetPoints(m_tempPoints);
    m_tempPolyData->SetLines(m_tempLines);

    m_tempMapper->SetInputData(m_tempPolyData);
    m_tempActor->SetMapper(m_tempMapper);
}

void vtk3d_Sketch_Render_Cotations::Afficher() {
    // 1. On cache ou on retire les acteurs du renderer pour qu'ils ne soient plus dessinés
    if (m_tempActor) {
        m_tempActor->SetVisibility(true);
        // Optionnel : si tu préfères les retirer complètement :
        // if (m_renderer->HasViewProp(m_tempActor)) m_renderer->RemoveActor(m_tempActor);
    }
    if (m_tempTextActor) {
        m_tempTextActor->SetVisibility(true);
        // if (m_renderer->HasViewProp(m_tempTextActor)) m_renderer->RemoveActor(m_tempTextActor);
    }
    if (m_tempPolyData) {
        m_tempPolyData->SetPoints(m_tempPoints);
        m_tempPolyData->SetLines(m_tempLines);
        m_tempPolyData->Modified();
    }
}

void vtk3d_Sketch_Render_Cotations::masquerEtVider() {
    // 1. On cache ou on retire les acteurs du renderer pour qu'ils ne soient plus dessinés
    if (m_tempActor) {
        m_tempActor->SetVisibility(false);
        // Optionnel : si tu préfères les retirer complètement :
        // if (m_renderer->HasViewProp(m_tempActor)) m_renderer->RemoveActor(m_tempActor);
    }
    if (m_tempTextActor) {
        m_tempTextActor->SetVisibility(false);
        // if (m_renderer->HasViewProp(m_tempTextActor)) m_renderer->RemoveActor(m_tempTextActor);
    }
    // 2. On vide proprement la géométrie pour libérer la mémoire VTK
    if (m_tempPoints) {
        m_tempPoints->Reset(); // Réinitialise sans libérer la capacité allouée (performant pour réutilisation)
    }
    if (m_tempLines) {
        m_tempLines->Reset();
    }
    if (m_tempPolyData) {
        m_tempPolyData->SetPoints(nullptr);
        m_tempPolyData->SetLines(nullptr);
        m_tempPolyData->Modified();
    }
}

void vtk3d_Sketch_Render_Cotations::DessineFleche3D(
    vtkSmartPointer<vtkCellArray> lines,
    vtkSmartPointer<vtkPoints> points,
    const gp_Ax3& sketchPlane,
    const gp_Pnt& pntExtremite,    // Pnt3DStart ou Pnt3DStop de la ligne de cote
    const gp_Vec& dirLigneCote,    // Vecteur directeur normalisé de la ligne de cote (de A vers B)
    double longueur,               // ex: 4.0 mm
    double angleDeg,               // ex: 15.0 degrés
    bool bInverser)                // true pour la flèche de départ, false pour celle de fin
{
    // 1. Déterminer le sens de la flèche sur la ligne
    gp_Vec dirFleche = bInverser ? dirLigneCote : -dirLigneCote;

    // 2. Trouver la perpendiculaire à la ligne de cote RESTANT dans le plan d'esquisse
    gp_Vec perpDansPlan = dirLigneCote.Crossed(gp_Vec(sketchPlane.Direction()));
    perpDansPlan.Normalize();

    // 3. Conversion de l'angle en radians
    double angleRad = angleDeg * (M_PI / 180.0);

    // 4. Calcul des deux directions des barbes de la flèche (Trigonométrie)
    // On combine la direction arrière et la perpendiculaire pour ouvrir la flèche
    gp_Vec dirBarbe1 = (dirFleche * std::cos(angleRad)) + (perpDansPlan * std::sin(angleRad));
    gp_Vec dirBarbe2 = (dirFleche * std::cos(angleRad)) - (perpDansPlan * std::sin(angleRad));

    // 5. Calcul des deux points d'extrémité des barbes en 3D
    gp_Pnt pBarbe1 = pntExtremite.Translated(dirBarbe1 * longueur);
    gp_Pnt pBarbe2 = pntExtremite.Translated(dirBarbe2 * longueur);

    // 6. Injection dans VTK
    // Point de la pointe de la flèche (déjà dans m_tempPoints, mais on le récrée ou on l'utilise)
    vtkIdType pBase = points->InsertNextPoint(pntExtremite.X(), pntExtremite.Y(), pntExtremite.Z());
    vtkIdType pB1   = points->InsertNextPoint(pBarbe1.X(), pBarbe1.Y(), pBarbe1.Z());
    vtkIdType pB2   = points->InsertNextPoint(pBarbe2.X(), pBarbe2.Y(), pBarbe2.Z());

    // Barbe 1
    vtkNew<vtkLine> line1;
    line1->GetPointIds()->SetId(0, pBase);
    line1->GetPointIds()->SetId(1, pB1);
    lines->InsertNextCell(line1);

    // Barbe 2
    vtkNew<vtkLine> line2;
    line2->GetPointIds()->SetId(0, pBase);
    line2->GetPointIds()->SetId(1, pB2);
    lines->InsertNextCell(line2);
}






void vtk3d_Sketch_Render_Cotations::DrawPrepairedCotation ( vtkCellArray* lines, vtkPoints* points){
    if ( !m_renderer->HasViewProp( m_tempActor )){
        m_renderer->AddActor(m_tempActor);
    }

    //à ne pas faire ici, il faut le faire à la fin de l'évènement,
    // pour ne pas désynchroniser la machnine d'état VTK
    //m_renderer->Render();
}




void vtk3d_Sketch_Render_Cotations::DessinerCotationDepuisResultat(const gp_Ax3& sketchPlane, const DimensionEngine::GeometryResult& geo) {
    m_tempPoints->Reset();
    m_tempLines->Reset();

    // 1. Dessiner toutes les lignes générées par le moteur
    for (const auto& edge : geo.lines) {
        vtkIdType p0 = m_tempPoints->InsertNextPoint(edge.first.X(), edge.first.Y(), edge.first.Z());
        vtkIdType p1 = m_tempPoints->InsertNextPoint(edge.second.X(), edge.second.Y(), edge.second.Z());
        // Ajouter la cellule vtkLine...

        vtkNew<vtkLine> line;
        line->GetPointIds()->SetId(0, p0);
        line->GetPointIds()->SetId(1, p1);
        m_tempLines->InsertNextCell(line);
    }

    // 2. Dessiner tous les arcs générés (ex: pour les angles)
    for (const auto& arc : geo.arcs) {
        // Discrétiser l'arc en petits segments de lignes et les pousser dans VTK
    }

    // 3. Dessiner toutes les flèches demandées
    for (const auto& arrow : geo.arrows) {
        DessineFleche3D(m_tempLines, m_tempPoints, sketchPlane, arrow.position, arrow.direction, 2.0, 15.0, arrow.bInverted);
    }

    // 4. Mettre à jour le texte
    std::string texteDistance = QString::number(geo.measuredValue, 'f', 1).toStdString() + " mm";
    m_tempTextActor->SetInput(texteDistance.c_str());
    m_tempTextActor->GetTextProperty()->SetFontSize(12);
    m_tempTextActor->GetTextProperty()->SetColor(0.2, 0.2, 0.2);
    m_tempTextActor->GetTextProperty()->SetJustificationToCentered();
    m_tempTextActor->GetTextProperty()->SetVerticalJustificationToCentered();

    // Appliquer l'offset de -1mm au texte (perpendiculairement à la ligne de cote, dans le plan)
    gp_Vec perpDansPlan = geo.textDirection.Crossed(gp_Vec(sketchPlane.Direction())).Normalized();
    gp_Pnt positionTexte3D = geo.textPosition.Translated(perpDansPlan * -1.0);

    m_tempTextActor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
    m_tempTextActor->GetPositionCoordinate()->SetValue(positionTexte3D.X(), positionTexte3D.Y(), positionTexte3D.Z());

    // Calcul de l'orientation du texte (aligné avec la direction du texte calculée par le moteur)
    double angle_deg = std::atan2(geo.textDirection.Dot(sketchPlane.YDirection()), geo.textDirection.Dot(sketchPlane.XDirection())) * (180.0 / M_PI);
    if (angle_deg > 90.0)  angle_deg -= 180.0;
    if (angle_deg < -90.0) angle_deg += 180.0;
    m_tempTextActor->SetOrientation(angle_deg);

    if (!m_renderer->HasViewProp(m_tempTextActor)) {
        m_renderer->AddActor(m_tempTextActor);
    }

    DrawPrepairedCotation(m_tempLines, m_tempPoints);
}




