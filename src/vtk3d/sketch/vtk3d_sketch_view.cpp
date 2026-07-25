#include "vtk3d_sketch.h"
#include "vtk3d_MainView.h"
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkLine.h>
#include <vtkPolyLine.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPlaneSource.h>
#include <vtkDistanceToCamera.h>
#include <vtkGlyph3D.h>
#include <vtkAppendPolyData.h>
#include <vtkDataObject.h>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <ElSLib.hxx>
#include <gp_Pln.hxx>
#include <cmath>

void Vtk3d_Sketch::rafraichirAffichageEsquisse() {
    if (!m_Operation || !m_view) return;

    auto* sketchParams = std::get_if<SketchParams>(&m_Operation->getParamsMutable());
    if (!sketchParams) return;

    rafraichirGeometrie(sketchParams);
    rafraichirPoignees(sketchParams);
    rafraichirContraintesGeometriques(sketchParams);

    m_view->renderWindow()->Render();
}


void Vtk3d_Sketch::rafraichirAffichageEsquisseInteractif() {
    auto* sketchParams = std::get_if<SketchParams>(&m_Operation->getParamsMutable());
    if (!sketchParams) return;

    // Récupérer le vtkPolyData principal de l'affichage de l'esquisse
    if (!m_ActorSketchDisplay || !m_ActorSketchDisplay->GetMapper()) return;
    auto geomPolyData = vtkPolyData::SafeDownCast(m_ActorSketchDisplay->GetMapper()->GetInput());
    if (!geomPolyData) return;

    vtkPoints* pts = geomPolyData->GetPoints();
    if (!pts) return;

    // ⚠️ Astuce : Si ton mapping d'origine associe chaque sommet de primitive à un index de point VTK précis,
    // il te suffit de boucler sur tes primitives et de faire un setPoint direct :
    // pts->SetPoint(vtkPointId, x, y, z);

    // Exemple de parcours des primitives de ton esquisse :
    int ptIndex = 0;
    for (const auto& primConst : sketchParams->getPrimitives()) {
        const SketchPrimitive& prim = primConst;

        std::visit([&](auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
            if constexpr (std::is_same_v<T, SketchLine>) {
                // Point de départ
                if (ptIndex < pts->GetNumberOfPoints()) {
                    pts->SetPoint(ptIndex++,
                                  concretePrim.start.cache_p3d.X(),
                                  concretePrim.start.cache_p3d.Y(),
                                  concretePrim.start.cache_p3d.Z()
                                  );
                }
                // Point d'arrivée
                if (ptIndex < pts->GetNumberOfPoints()) {
                    pts->SetPoint(ptIndex++, concretePrim.stop.cache_p3d.X(), concretePrim.stop.cache_p3d.Y(), concretePrim.stop.cache_p3d.Z() );
                }
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                // Centre du cercle
                if (ptIndex < pts->GetNumberOfPoints()) {
                    pts->SetPoint(ptIndex++, concretePrim.center.cache_p3d.X(), concretePrim.center.cache_p3d.Y(), concretePrim.center.cache_p3d.Z() );
                }
            }
        }, prim);
    }

    // 💡 Indispensable : notifier VTK que les points ont bougé
    pts->Modified();
    geomPolyData->Modified();

    // Idem pour les poignées (m_ActorSquareOfPrim) si elles affichent les sommets :
    if (m_ActorSquareOfPrim && m_ActorSquareOfPrim->GetMapper()) {
        auto squarePolyData = vtkPolyData::SafeDownCast(
            vtkPolyDataMapper::SafeDownCast(m_ActorSquareOfPrim->GetMapper())->GetInput()
            );
        if (squarePolyData && squarePolyData->GetPoints()) {
            // Mettre à jour les positions des carrés de poignées ici aussi
            squarePolyData->GetPoints()->Modified();
            squarePolyData->Modified();
        }
    }
    m_view->renderWindow()->Render();
    //m_view->renderWindow()->Modified();
    //m_view->renderWindow()->Frame();
}



void Vtk3d_Sketch::Echelle_ajusterEchelleElements(vtkCamera* camera) {
    if (!camera) return;

    double currentScale = camera->GetParallelScale();
    if (std::abs(currentScale - m_derniereEchelle) < 0.1) {
        return;
    }
    m_derniereEchelle = currentScale;

    std::visit([currentScale](auto& _tool) {
        _tool.ajusterEchelleElements(currentScale);
    }, m_tool);
}

//============================================================
//      Convertir la position 2D en position 3D en faisant
//      l'intersection avec le plan
gp_Pnt Vtk3d_Sketch::convertir2DEn3D(const gp_Pnt2d& point2D) {
    // Équivalent mathématique de : Point3D = Origine + U * AxeX + V * AxeY
    return ElSLib::Value(point2D.X(), point2D.Y(), m_sketchPlane);
}

bool Vtk3d_Sketch::calculerIntersectionSourisSurPlan(int mouseX, int mouseY, gp_Pnt2d &point2DOut, gp_Pnt &point3DOut) {
    if (!m_view || !m_view->getRenderer()) return false;

    vtkRenderer* renderer = m_view->getRenderer();
    int vtkY = m_view->height() - mouseY;

    double coordEcranProche[3] = { static_cast<double>(mouseX), static_cast<double>(vtkY), 0.0 };
    double coordEcranLointain[3] = { static_cast<double>(mouseX), static_cast<double>(vtkY), 1.0 };

    double pProche[4], pLointain[4];

    renderer->SetDisplayPoint(coordEcranProche);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(pProche);

    renderer->SetDisplayPoint(coordEcranLointain);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(pLointain);

    gp_Pnt R1(pProche[0]/pProche[3], pProche[1]/pProche[3], pProche[2]/pProche[3]);
    gp_Pnt R2(pLointain[0]/pLointain[3], pLointain[1]/pLointain[3], pLointain[2]/pLointain[3]);

    gp_Vec V(R1, R2);
    gp_Pnt O = m_sketchPlane.Location();
    gp_Dir N = m_sketchPlane.Direction();

    double denominateur = V.XYZ().Dot(N.XYZ());
    if (std::abs(denominateur) < 1e-6) {
        return false;
    }

    gp_Vec O_R1(R1, O);
    double t = O_R1.XYZ().Dot(N.XYZ()) / denominateur;

    gp_Pnt pointIntersection = R1.Translated(V * t);

    point3DOut = pointIntersection;


    // 2. EXTRACTION CRUCIALE : On convertit ce point 3D en coordonnées 2D locales (U, V) de l'esquisse
    gp_Pln planCalcul(m_sketchPlane);
    double u = 0.0, v = 0.0;
    ElSLib::Parameters(planCalcul, pointIntersection, u, v);

    // 3. On renvoie le point 2D
    point2DOut.SetCoord(u, v);

    return true;
}

void Vtk3d_Sketch::rafraichirPoignees(SketchParams* sketchParams) {
	// 1. Initialisation des conteneurs VTK pour stocker les positions et les métadonnées
    auto pointsHandles = vtkSmartPointer<vtkPoints>::New();
    auto edgeIdsHandles = vtkSmartPointer<vtkIntArray>::New(); 
    edgeIdsHandles->SetName("OpenCascadeEdgeID");  // Tableau pour associer chaque point à l'ID de sa primitive d'origine

    //2ème tableau (Rôle de la poignée)
    auto ArrayTypeHandle = vtkSmartPointer<vtkIntArray>::New();
    ArrayTypeHandle->SetName("ArrayTypeHandle");

	// 2. Extraction des points géométriques depuis les primitives du Sketch (C++17 std::visit)
    for (const auto& primitive : sketchParams->getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
			
			// Si la primitive est un segment de ligne (SketchLine)
            if constexpr (std::is_same_v<T, SketchLine>) {
				
				// Point de départ de la ligne
                pointsHandles->InsertNextPoint(concretePrim.start.cache_p3d.X(), concretePrim.start.cache_p3d.Y(), concretePrim.start.cache_p3d.Z());
                edgeIdsHandles->InsertNextValue(static_cast<int>(concretePrim.id));
                ArrayTypeHandle->InsertNextValue(static_cast<int>(1));

				// Point d'arrivée de la ligne
                pointsHandles->InsertNextPoint(concretePrim.stop.cache_p3d.X(), concretePrim.stop.cache_p3d.Y(), concretePrim.stop.cache_p3d.Z());
                edgeIdsHandles->InsertNextValue(static_cast<int>(concretePrim.id));
                ArrayTypeHandle->InsertNextValue(static_cast<int>(2));
            }
			
            // pour un cercle
            if constexpr (std::is_same_v<T, SketchCircle>) {
                pointsHandles->InsertNextPoint(concretePrim.center.cache_p3d.X(), concretePrim.center.cache_p3d.Y(), concretePrim.center.cache_p3d.Z());
                edgeIdsHandles->InsertNextValue(static_cast<int>(concretePrim.id));
                ArrayTypeHandle->InsertNextValue(static_cast<int>(3));
            }


            // Note : D'autres types (arcs, cercles...) pourraient être gérés ici via d'autres 'if constexpr'
			
        }, primitive);
    }

	// 3. Gestion du cas où aucune poignée n'est à afficher
    if (pointsHandles->GetNumberOfPoints() == 0) {
        if (m_ActorSquareOfPrim) m_ActorSquareOfPrim->VisibilityOff();
        return;
    }

	// 4. Définition du motif visuel de la poignée (ici, un petit carré plan de 1x1 centré)
    auto handleSource = vtkSmartPointer<vtkPlaneSource>::New();
    handleSource->SetOrigin(-0.5, -0.5, 0.0);
    handleSource->SetPoint1( 0.5, -0.5, 0.0);
    handleSource->SetPoint2(-0.5,  0.5, 0.0);

	// Assemblage des points et de leurs attributs (IDs) dans un PolyData
    auto polyDataHandles = vtkSmartPointer<vtkPolyData>::New();
    polyDataHandles->SetPoints(pointsHandles);
    polyDataHandles->GetPointData()->AddArray(edgeIdsHandles);
    polyDataHandles->GetPointData()->AddArray(ArrayTypeHandle);

	// 5. Calcul de la distance à la caméra pour garder une taille de poignée constante à l'écran
    auto distFilter = vtkSmartPointer<vtkDistanceToCamera>::New();
    distFilter->SetInputData(polyDataHandles);
    distFilter->SetRenderer(m_view->getRenderer());
    distFilter->SetScreenSize(8.0);

	// 6. Application du "Glyphing" : dupliquer le carré (handleSource) sur chaque point collecté
    auto glyphFilter = vtkSmartPointer<vtkGlyph3D>::New();
    glyphFilter->SetInputConnection(distFilter->GetOutputPort());
    glyphFilter->SetSourceConnection(handleSource->GetOutputPort());
    glyphFilter->SetScaleModeToScaleByScalar();	// Mise à l'échelle basée sur la distance calculée par distFilter
    glyphFilter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
    glyphFilter->SetColorModeToColorByScalar();

    if (!m_ActorSquareOfPrim) {
        m_ActorSquareOfPrim = vtkSmartPointer<vtkActor>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->ScalarVisibilityOff();
        m_ActorSquareOfPrim->SetPickable(true);
        m_ActorSquareOfPrim->SetMapper(mapper);
        m_ActorSquareOfPrim->GetProperty()->SetColor(1.0, 50.0 / 255.0, 50.0 / 255.0);
        m_view->getRenderer()->AddActor(m_ActorSquareOfPrim);
    }

	// 8. Mise à jour du Mapper avec le nouveau pipeline de poignées et affichage
    if (auto mapper = vtkPolyDataMapper::SafeDownCast(m_ActorSquareOfPrim->GetMapper())) {
        mapper->SetInputConnection(glyphFilter->GetOutputPort());
        mapper->ScalarVisibilityOff();
        mapper->Modified();
    }
	
	if (!m_ActorSquareOfPrim) {
		m_ActorSquareOfPrim->VisibilityOn();
	}

}

void Vtk3d_Sketch::rafraichirGeometrie(SketchParams* sketchParams) {
    auto pointsGeom = vtkSmartPointer<vtkPoints>::New();
    auto linesGeom = vtkSmartPointer<vtkCellArray>::New();

    auto edgeIdsGeom = vtkSmartPointer<vtkIntArray>::New();
    edgeIdsGeom->SetName("OpenCascadeEdgeID");

    auto colorsGeom = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colorsGeom->SetName("Colors");
    colorsGeom->SetNumberOfComponents(3);

    unsigned char colorLine[3] = { 60, 191, 60 };
    unsigned char colorCircle[3] = { 60, 191, 60 };
    vtkIdType ptCounterGeom = 0;

    for (const auto& primitive : sketchParams->getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                pointsGeom->InsertNextPoint(concretePrim.start.cache_p3d.X(), concretePrim.start.cache_p3d.Y(), concretePrim.start.cache_p3d.Z());
                pointsGeom->InsertNextPoint(concretePrim.stop.cache_p3d.X(), concretePrim.stop.cache_p3d.Y(), concretePrim.stop.cache_p3d.Z());

                auto vtkLineObj = vtkSmartPointer<vtkLine>::New();
                vtkLineObj->GetPointIds()->SetId(0, ptCounterGeom);
                vtkLineObj->GetPointIds()->SetId(1, ptCounterGeom + 1);
                linesGeom->InsertNextCell(vtkLineObj);

                edgeIdsGeom->InsertNextValue(static_cast<int>(concretePrim.id));
                colorsGeom->InsertNextTypedTuple(colorLine);
                ptCounterGeom += 2;
            }
            else if constexpr (std::is_same_v<T, SketchCircle>) {
                const int numSegments = 64;
                auto polyLineObj = vtkSmartPointer<vtkPolyLine>::New();
                polyLineObj->GetPointIds()->SetNumberOfIds(numSegments + 1);

                // Trouver les coordonnées U,V locales du centre 3D actuel sur le plan
                double centerU = 0.0, centerV = 0.0;
                ElSLib::Parameters(gp_Pln(m_sketchPlane), concretePrim.center.cache_p3d, centerU, centerV);

                for (int i = 0; i <= numSegments; ++i) {
                    double angle = 2.0 * M_PI * i / numSegments;

                    // Calcul en 2D locale autour du centre local
                    double localU = centerU + concretePrim.radius * std::cos(angle);
                    double localV = centerV + concretePrim.radius * std::sin(angle);

                    // Projection 3D fidèle à l'orientation du plan
                    gp_Pnt p3d = ElSLib::Value(localU, localV, m_sketchPlane);

                    pointsGeom->InsertNextPoint(p3d.X(), p3d.Y(), p3d.Z());
                    polyLineObj->GetPointIds()->SetId(i, ptCounterGeom + i);
                }

                linesGeom->InsertNextCell(polyLineObj);
                edgeIdsGeom->InsertNextValue(static_cast<int>(concretePrim.id));
                colorsGeom->InsertNextTypedTuple(colorCircle);

                ptCounterGeom += (numSegments + 1);
            }
        }, primitive);
    }

    if (pointsGeom->GetNumberOfPoints() == 0) {
        if (m_ActorSketchDisplay) m_ActorSketchDisplay->VisibilityOff();
        return;
    }

    auto polyDataGeom = vtkSmartPointer<vtkPolyData>::New();
    polyDataGeom->SetPoints(pointsGeom);
    polyDataGeom->SetLines(linesGeom);
    polyDataGeom->GetCellData()->AddArray(edgeIdsGeom);
    polyDataGeom->GetCellData()->SetPedigreeIds(edgeIdsGeom);
    polyDataGeom->GetCellData()->SetScalars(colorsGeom);

    if (!m_ActorSketchDisplay) {
        m_ActorSketchDisplay = vtkSmartPointer<vtkActor>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetScalarModeToUseCellData();
        mapper->SelectColorArray("Colors");

        m_ActorSketchDisplay->SetMapper(mapper);
        m_ActorSketchDisplay->GetProperty()->SetLineWidth(2.5);
       // m_ActorSketchDisplay->GetProperty()->RenderLinesAsTubesOn();

        m_view->getRenderer()->AddActor(m_ActorSketchDisplay);
    }

    if (auto mapper = vtkPolyDataMapper::SafeDownCast(m_ActorSketchDisplay->GetMapper())) {
        polyDataGeom->Modified();
        mapper->SetInputData(polyDataGeom);
        mapper->SetScalarModeToUseCellData();
        mapper->SelectColorArray("Colors");
        mapper->ScalarVisibilityOn();
        mapper->Modified();
    }

    m_ActorSketchDisplay->VisibilityOn();
    m_ActorSketchDisplay->PickableOn();
}

vtkSmartPointer<vtkPolyData> Vtk3d_Sketch::creerSymboleLigne(bool horizontal) {
    auto points = vtkSmartPointer<vtkPoints>::New();
    if (horizontal) {
        points->InsertNextPoint(-0.5, 0.0, 0.0);
        points->InsertNextPoint( 0.5, 0.0, 0.0);
    } else {
        points->InsertNextPoint(0.0, -0.5, 0.0);
        points->InsertNextPoint(0.0,  0.5, 0.0);
    }

    auto line = vtkSmartPointer<vtkLine>::New();
    line->GetPointIds()->SetId(0, 0);
    line->GetPointIds()->SetId(1, 1);

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(line);

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(cells);

    return polyData;
}



vtkSmartPointer<vtkPolyData> Vtk3d_Sketch::creerSymbolePerpendiculaire() {
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();

    // Définition d'un petit carré ou d'un symbole d'angle droit en coordonnées locales (ex: centré autour de 0 ou dans [0, 1])
    // Exemple d'un petit carré de taille fixe (ex: de 0 à 1 en X et Y)
    points->InsertNextPoint(0.0, 0.0, 0.0); // 0
    points->InsertNextPoint(1.0, 0.0, 0.0); // 1
    points->InsertNextPoint(1.0, 1.0, 0.0); // 2
    points->InsertNextPoint(0.0, 1.0, 0.0); // 3

    // Création des 4 segments du carré
    auto addSegment = [&](vtkIdType p1, vtkIdType p2) {
        auto line = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, p1);
        line->GetPointIds()->SetId(1, p2);
        lines->InsertNextCell(line);
    };

    addSegment(0, 1);
    addSegment(1, 2);
    addSegment(2, 3);
    addSegment(3, 0);

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(lines);

    return polyData;
}

void Vtk3d_Sketch::rafraichirContraintesGeometriques(SketchParams* sketchParams) {
    auto pointsHorizontal   = vtkSmartPointer<vtkPoints>::New();
    auto pointsVertical     = vtkSmartPointer<vtkPoints>::New();
    auto pointsPerpendicular = vtkSmartPointer<vtkPoints>::New();

    for (const auto& constraint : sketchParams->getConstraints()) {
        size_t targetId = constraint.ref1.primitiveId;
        auto* primitiveVariant = sketchParams->GetPrimitiveMutable(targetId);
        if (!primitiveVariant) continue;

        if (auto* line = std::get_if<SketchLine>(primitiveVariant)) {
            double glyphPos[3];
            glyphPos[0] = (line->start.cache_p3d.X() + line->stop.cache_p3d.X()) / 2.0;
            glyphPos[1] = (line->start.cache_p3d.Y() + line->stop.cache_p3d.Y()) / 2.0;
            glyphPos[2] = (line->start.cache_p3d.Z() + line->stop.cache_p3d.Z()) / 2.0;

            if (constraint.type == ConstraintType::Horizontal) {
                double glyphPosBas[3] = { glyphPos[0], glyphPos[1] - 1.0, glyphPos[2] };
                pointsHorizontal->InsertNextPoint(glyphPosBas);

                double glyphPosHaut[3] = { glyphPos[0], glyphPos[1] + 1.0, glyphPos[2] };
                pointsHorizontal->InsertNextPoint(glyphPosHaut);
            }
            else if (constraint.type == ConstraintType::Vertical) {
                glyphPos[0] -= 1.0;
                pointsVertical->InsertNextPoint(glyphPos);
            }
            else if (constraint.type == ConstraintType::Perpendicular) {
                // Utilisation directe de ref2 sans redéclaration
                size_t targetId2 = constraint.ref2.primitiveId;
                auto* primitiveVariant2 = sketchParams->GetPrimitiveMutable(targetId2);

                if (primitiveVariant2) {
                    if (auto* line2 = std::get_if<SketchLine>(primitiveVariant2)) {
                        double interX = 0, interY = 0, interZ = 0;
                        bool hasIntersection = false;

                        const double eps = 1e-6;
                        auto ptsMatch = [&](const auto& p1, const auto& p2) {
                            return std::abs(p1.cache_p3d.X() - p2.cache_p3d.X()) < eps &&
                                   std::abs(p1.cache_p3d.Y() - p2.cache_p3d.Y()) < eps;
                        };

                        if (ptsMatch(line->start, line2->start) || ptsMatch(line->start, line2->stop)) {
                            interX = line->start.cache_p3d.X();
                            interY = line->start.cache_p3d.Y();
                            interZ = line->start.cache_p3d.Z();
                            hasIntersection = true;
                        }
                        else if (ptsMatch(line->stop, line2->start) || ptsMatch(line->stop, line2->stop)) {
                            interX = line->stop.cache_p3d.X();
                            interY = line->stop.cache_p3d.Y();
                            interZ = line->stop.cache_p3d.Z();
                            hasIntersection = true;
                        }

                        if (!hasIntersection) {
                            interX = glyphPos[0];
                            interY = glyphPos[1];
                            interZ = glyphPos[2];
                        }

                        double perpPos[3] = { interX, interY, interZ };
                        pointsPerpendicular->InsertNextPoint(perpPos);
                    }
                }
            }
        }
    }

    bool hasH = (pointsHorizontal->GetNumberOfPoints() > 0);
    bool hasV = (pointsVertical->GetNumberOfPoints() > 0);
    bool hasPerp = (pointsPerpendicular->GetNumberOfPoints() > 0);

    if (!hasH && !hasV && !hasPerp) {
        if (m_constraintsDisplayActor) m_constraintsDisplayActor->VisibilityOff();
        return;
    }

    auto appender = vtkSmartPointer<vtkAppendPolyData>::New();

    // Helper mis à jour pour accepter directement la source du symbole (vtkPolyData)
    auto traiterType = [&](vtkPoints* pts, vtkPolyData* symboleSource) {
        if (!pts || pts->GetNumberOfPoints() == 0 || !symboleSource) return;

        auto polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->SetPoints(pts);

        auto distFilter = vtkSmartPointer<vtkDistanceToCamera>::New();
        distFilter->SetInputData(polyData);
        distFilter->SetRenderer(m_view->getRenderer());
        distFilter->SetScreenSize(12.0);

        auto glyphFilter = vtkSmartPointer<vtkGlyph3D>::New();
        glyphFilter->SetInputConnection(distFilter->GetOutputPort());
        glyphFilter->SetSourceData(symboleSource);
        glyphFilter->SetScaleModeToScaleByScalar();
        glyphFilter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
        glyphFilter->Update();

        appender->AddInputConnection(glyphFilter->GetOutputPort());
    };

    if (hasH)    traiterType(pointsHorizontal, creerSymboleLigne(true));
    if (hasV)    traiterType(pointsVertical, creerSymboleLigne(false));
    if (hasPerp) traiterType(pointsPerpendicular, creerSymbolePerpendiculaire()); // Remplace par ta fonction de carré

    appender->Update();

    if (!m_constraintsDisplayActor) {
        m_constraintsDisplayActor = vtkSmartPointer<vtkActor>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_constraintsDisplayActor->SetMapper(mapper);
        m_view->getRenderer()->AddActor(m_constraintsDisplayActor);
    }

    if (auto mapper = vtkPolyDataMapper::SafeDownCast(m_constraintsDisplayActor->GetMapper())) {
        mapper->SetInputConnection(appender->GetOutputPort());
        mapper->ScalarVisibilityOff();
        mapper->Modified();
    }

    m_constraintsDisplayActor->GetProperty()->SetColor(0.57, 0.12, 0.71);
    m_constraintsDisplayActor->GetProperty()->SetLineWidth(2.0);
    m_constraintsDisplayActor->VisibilityOn();
}




