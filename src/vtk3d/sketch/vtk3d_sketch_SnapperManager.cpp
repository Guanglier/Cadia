

#include "vtk3d_sketch_SnapperManager.h"
#include "vtk3d_sketch.h"
#include "vtk3d_MainView.h"

#include <vtkRegularPolygonSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkImageData.h>
#include <vtkTexture.h>

#include <variant>
#include <type_traits>
#include <cmath>

SketchSnapperManager::SketchSnapperManager(Vtk3d_Sketch* parent) : m_Parent(parent) {}

SketchSnapperManager::~SketchSnapperManager() {
    cleanUp();
}

void SketchSnapperManager::init() {
    if (!m_Parent || !m_Parent->GetView() || !m_Parent->GetView()->getRenderer()){
        std::cout<<" ERROR !! SketchSnapperManager::init "<< std::endl;
        std::cerr<<" ERROR !! SketchSnapperManager::init "<< std::endl;
        return;
    }
    initSnapPointActor();
    initSnapLineActor();
}

void SketchSnapperManager::cleanUp() {
    if (m_Parent && m_Parent->GetView() && m_Parent->GetView()->getRenderer()) {
        auto renderer = m_Parent->GetView()->getRenderer();
        if (m_ActorSnapPoint) renderer->RemoveActor(m_ActorSnapPoint);
        if (m_ActorSnapLine) renderer->RemoveActor(m_ActorSnapLine);
    }
}

void SketchSnapperManager::initSnapPointActor() {
    auto squareSource = vtkSmartPointer<vtkRegularPolygonSource>::New();
    squareSource->SetNumberOfSides(4);
    squareSource->SetRadius(0.4);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(squareSource->GetOutputPort());

    m_ActorSnapPoint = vtkSmartPointer<vtkActor>::New();
    m_ActorSnapPoint->SetMapper(mapper);
    m_ActorSnapPoint->GetProperty()->SetColor(68.0/255.0, 68.0/255.0, 219.0/255.0 );
    m_ActorSnapPoint->GetProperty()->SetLineWidth(2.0);
    m_ActorSnapPoint->GetProperty()->SetRepresentationToWireframe();
    m_ActorSnapPoint->SetVisibility(false);
    m_ActorSnapPoint->PickableOff();

    m_Parent->GetView()->getRenderer()->AddActor(m_ActorSnapPoint);
}

void SketchSnapperManager::initSnapLineActor() {
    m_snapLinePoints   = vtkSmartPointer<vtkPoints>::New();
    m_snapLineCellsArray    = vtkSmartPointer<vtkCellArray>::New();
    m_snapLineTCoords  = vtkSmartPointer<vtkFloatArray>::New();
    m_snapLinePolyData = vtkSmartPointer<vtkPolyData>::New();

    m_snapLineTCoords->SetName("TCoords");
    m_snapLineTCoords->SetNumberOfComponents(2); // Fix moderne OpenGL2 2D

    m_snapLinePolyData->SetPoints(m_snapLinePoints);
    m_snapLinePolyData->SetLines(m_snapLineCellsArray);
    m_snapLinePolyData->GetPointData()->SetTCoords(m_snapLineTCoords);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(m_snapLinePolyData);
    mapper->ScalarVisibilityOff(); // Désactive l'interprétation thermique sauvage

    m_ActorSnapLine = vtkSmartPointer<vtkActor>::New();
    m_ActorSnapLine->SetMapper(mapper);

    auto textureImage = vtkSmartPointer<vtkImageData>::New();
    textureImage->SetDimensions(8, 1, 1);
    textureImage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

    for (int i = 0; i < 8; ++i) {
        unsigned char* pixel = static_cast<unsigned char*>(textureImage->GetScalarPointer(i, 0, 0));
        pixel[0] = 255; pixel[1] = 255; pixel[2] = 255;
        pixel[3] = (i < 4) ? 255 : 0;
    }

    auto dashedTexture = vtkSmartPointer<vtkTexture>::New();
    dashedTexture->SetInputData(textureImage);
    dashedTexture->RepeatOn();
    dashedTexture->InterpolateOff();

    m_ActorSnapLine->SetTexture(dashedTexture);
    m_ActorSnapLine->GetProperty()->SetColor(0.5, 0.5, 0.5);
    m_ActorSnapLine->GetProperty()->SetOpacity(0.6);
    m_ActorSnapLine->GetProperty()->SetLineWidth(1.5);
    m_ActorSnapLine->SetVisibility(false);
    m_ActorSnapLine->PickableOff();

    m_Parent->GetView()->getRenderer()->AddActor(m_ActorSnapLine);
}

void SketchSnapperManager::appliqueContraintes2D(
    gp_Pnt2d& li_P1_2D, gp_Pnt& li_P1_3D,
    gp_Pnt2d& li_P2_2D, gp_Pnt& li_P2_3D,
    bool activeAideHV, bool enCoursDeDessin)
{
    bool alignValide = false;

    // 1. Aimantation sur les points durs existants
    bool snapPointValide = snapToExistingPoints(li_P2_2D, li_P2_3D);

    // 2. Aimantation d'alignement sur les axes X/Y
    if ( false == snapPointValide ){
        alignValide = alignWithExistingPoints(li_P2_2D, li_P2_3D);

        if ( true == alignValide ){
            li_P2_3D = m_Parent->convertir2DEn3D(li_P2_2D);
        }
    }
    //bool alignValide = false;
    //std::cout<<"SketchSnapperManager::appliqueContraintes2D " << li_P1_2D.x << "," << li_P1_2D.y <<" " << std::endl;

    // 3. Gestion logique des aides de tracé{
    li_P2_3D = m_Parent->convertir2DEn3D(li_P2_2D);

    if (snapPointValide || alignValide) {
        m_ActorSnapPoint->SetPosition(li_P2_3D.X(), li_P2_3D.Y(), li_P2_3D.Z());
        ajusterEchelleCarreSnap();
        m_ActorSnapPoint->SetVisibility(true);
    } else {
        m_ActorSnapPoint->SetVisibility(false);
    }

    // L'aide Horizontale/Verticale pure ne s'applique que si on est en train de tracer
    // et qu'on n'est pas déjà aimanté magnétiquement sur un point existant
    if (enCoursDeDessin && activeAideHV && !snapPointValide && !alignValide) {
        //aideHV(li_P1_2D, li_P2_2D);
    }

}


void SketchSnapperManager::snapPointsVisited_Clean (){
    m_SnapPointsVisitedList.clear();
    //std::cout<< "SketchSnapperManager::snapPointsVisited_Clean" << std::endl;
}
void SketchSnapperManager::snapPointsVisited_AddPoint (gp_Pnt2d& vect){
    if ( false == snapPointsVisited_IsPointInTheList ( vect) ){
        m_SnapPointsVisitedList.emplace_back(vect);
        //std::cout<< "SketchSnapperManager::snapPointsVisited_AddPoint" << std::endl;
    }
}
bool SketchSnapperManager::snapPointsVisited_IsPointInTheList (const gp_Pnt2d& vect){
    return std::any_of(m_SnapPointsVisitedList.begin(), m_SnapPointsVisitedList.end(), [&vect](const auto& v) {
        return (v.X() == vect.X() && v.Y() == vect.Y());
    });
}


bool SketchSnapperManager::snapToExistingPoints(gp_Pnt2d& plio_Ptr2D, gp_Pnt& plio_Ptr3D, double li_SeuilCoincidence_mm) {
    if (!m_Parent || !m_Parent->PartRefs.GetOperation()) return false;
    auto* sketchParams = m_Parent->PartRefs.GetParams();
    if (!sketchParams) return false;

    const double seuilCoincidenceMm = li_SeuilCoincidence_mm;
    bool aAimante = false;
    double plusProcheDistance = seuilCoincidenceMm;
    gp_Pnt2d v2d_SnappedPoint2D;
    gp_Pnt   v2d_SnappedPoint3D;

    // 1. Parcours direct de tous les points du sketch (origine, points de lignes, centres, etc.)
    for (const auto& sketchPoint : sketchParams->getPoints() ) {
        if (!sketchPoint.b_IsSnappable) continue;

        double dist = std::hypot(plio_Ptr2D.X() - sketchPoint.p2d.X(), plio_Ptr2D.Y() - sketchPoint.p2d.Y());
        if (dist < plusProcheDistance) {
            plusProcheDistance = dist;
            aAimante = true;
            v2d_SnappedPoint2D = sketchPoint.p2d;
            v2d_SnappedPoint3D = sketchPoint.cache_p3d;
            std::cout << "Snap pt " << std::endl;
        }
    }

    // 2. (Optionnel) Si tu dois aussi aimanter sur les milieux des lignes qui ne sont pas forcément
    // stockés comme des SketchPoint explicites dans ta liste globale :
    for (const auto& primitive : sketchParams->getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;

            if constexpr (std::is_same_v<T, SketchLine>) {
                const SketchPoint& cstart = sketchParams->GetPointById(concretePrim.startPointId);
                const SketchPoint& cstop = sketchParams->GetPointById(concretePrim.stopPointId);

                if( true == cstart.b_IsSnappable ){
                    double distStart = std::hypot(plio_Ptr2D.X() - cstart.p2d.X(), plio_Ptr2D.Y() - cstart.p2d.Y());
                    if (distStart < plusProcheDistance) {
                        plusProcheDistance = distStart;
                        aAimante = true;
                        v2d_SnappedPoint2D = cstart.p2d;
                        v2d_SnappedPoint3D = cstart.cache_p3d;
                        std::cout << "Snap L-start " << std::endl;
                    }
                }
                if( true == cstop.b_IsSnappable ){
                    double distStop = std::hypot(plio_Ptr2D.X() - cstop.p2d.X(), plio_Ptr2D.Y() - cstop.p2d.Y());
                    if (distStop < plusProcheDistance) {
                        plusProcheDistance = distStop;
                        aAimante = true;
                        v2d_SnappedPoint2D = cstop.p2d;
                        v2d_SnappedPoint3D = cstop.cache_p3d;
                        std::cout << "Snap L-stop " << std::endl;
                    }
                }

                if (cstart.b_IsSnappable && cstop.b_IsSnappable) {
                    double midX = (cstop.p2d.X() + cstart.p2d.X()) / 2.0;
                    double midY = (cstop.p2d.Y() + cstart.p2d.Y()) / 2.0;
                    double distMidle = std::hypot(plio_Ptr2D.X() - midX, plio_Ptr2D.Y() - midY);

                    if (distMidle < plusProcheDistance) {
                        plusProcheDistance = distMidle;
                        aAimante = true;
                        v2d_SnappedPoint2D.SetX(midX);
                        v2d_SnappedPoint2D.SetY(midY);

                        v2d_SnappedPoint3D.SetX((cstop.cache_p3d.X() + cstart.cache_p3d.X()) / 2.0);
                        v2d_SnappedPoint3D.SetY((cstop.cache_p3d.Y() + cstart.cache_p3d.Y()) / 2.0);
                        v2d_SnappedPoint3D.SetZ((cstop.cache_p3d.Z() + cstart.cache_p3d.Z()) / 2.0);
                        std::cout << "Snap middle " << std::endl;
                    }
                }
            }
        }, primitive);
    }

    if (aAimante) {
        snapPointsVisited_AddPoint(v2d_SnappedPoint2D);
        plio_Ptr2D = v2d_SnappedPoint2D;
        plio_Ptr3D = v2d_SnappedPoint3D;
        return true;
    }
    return false;
}

/*
bool SketchSnapperManager::snapToExistingPoints(gp_Pnt2d& plio_Ptr2D, gp_Pnt& plio_Ptr3D, double li_SeuilCoincidence_mm) {
    if (!m_Parent || !m_Parent->PartRefs.GetOperation()) return false;
    auto* sketchParams = m_Parent->PartRefs.GetParams();
    if (!sketchParams) return false;

    const double seuilCoincidenceMm = li_SeuilCoincidence_mm;
    bool aAimante = false;
    double plusProcheDistance = seuilCoincidenceMm;
    //gp_Pnt pointCibleExact;
    gp_Pnt2d v2d_SnappedPoint2D;
    gp_Pnt   v2d_SnappedPoint3D;


       //        // snapping sur un point
    //        if constexpr (std::is_same_v<T, SketchPoint>) {
    //            SketchPoint& ptn = sketchParams->GetPointById(concretePrim.startPointId);
    //            if( true == ptn.b_IsSnappable ){
    //                double distStart = std::hypot(plio_Ptr2D.X() - ptn.p2d.X(), plio_Ptr2D.Y() - ptn.p2d.Y());
    //                if (distStart < plusProcheDistance) {
    //                    plusProcheDistance = distStart;
    //                    aAimante = true;
    //                    v2d_SnappedPoint2D = ptn.p2d;
    //                    v2d_SnappedPoint3D = ptn.cache_p3d;
    //                }
    //            }
    //        }



    for (const auto& primitive : sketchParams->getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
            if constexpr (std::is_same_v<T, SketchLine>) {

                SketchPoint& cstart = sketchParams->GetPointById(concretePrim.startPointId);
                SketchPoint& cstop = sketchParams->GetPointById(concretePrim.stopPointId);

                // TO DO : supprimer les std::hypot qui consomment du temps de calcul pour prendre une formule plus simple
                if( true == cstart.b_IsSnappable ){
                    double distStart = std::hypot(plio_Ptr2D.X() - cstart.p2d.X(), plio_Ptr2D.Y() - cstart.p2d.Y());
                    if (distStart < plusProcheDistance) {
                        plusProcheDistance = distStart;
                        aAimante = true;
                        v2d_SnappedPoint2D = cstart.p2d;
                        v2d_SnappedPoint3D = cstart.cache_p3d;
                    }
                }
                if( true == cstop.b_IsSnappable ){
                    double distStop = std::hypot(plio_Ptr2D.X() - cstop.p2d.X(), plio_Ptr2D.Y() - cstop.p2d.Y());
                    if (distStop < plusProcheDistance) {
                        plusProcheDistance = distStop;
                        aAimante = true;
                        v2d_SnappedPoint2D = cstop.p2d;
                        v2d_SnappedPoint3D = cstop.cache_p3d;
                    }
                }
                if ( (true == cstart.b_IsSnappable) && (true == cstop.b_IsSnappable) ){
                    double distMidle = std::hypot(
                        plio_Ptr2D.X() - ((cstop.p2d.X()+cstart.p2d.X() )/2) ,
                        plio_Ptr2D.Y() - ((cstop.p2d.Y()+cstart.p2d.Y() )/2 ) );
                    if (distMidle < plusProcheDistance) {
                        plusProcheDistance = distMidle;
                        v2d_SnappedPoint2D.SetX ( ((cstop.p2d.X()+cstart.p2d.X() )/2 ) );
                        v2d_SnappedPoint2D.SetY ( ((cstop.p2d.Y()+cstart.p2d.Y() )/2 ) );

                        v2d_SnappedPoint3D.SetX ( ((cstop.cache_p3d.X()+cstart.cache_p3d.X() )/2 ) );
                        v2d_SnappedPoint3D.SetY ( ((cstop.cache_p3d.Y()+cstart.cache_p3d.Y() )/2 ) );
                        v2d_SnappedPoint3D.SetZ ( ((cstop.cache_p3d.Z()+cstart.cache_p3d.Z() )/2 ) );
                        aAimante = true;
                    }
                }
                // TO DO : gérer le cas ou on voit le point milieu
            }



        }, primitive);
    }

    if (aAimante) {
        snapPointsVisited_AddPoint ( v2d_SnappedPoint2D );
        plio_Ptr2D = v2d_SnappedPoint2D;
        plio_Ptr3D = v2d_SnappedPoint3D;
        return true;
    }
    return false;
}
*/


bool SketchSnapperManager::alignWithExistingPoints(gp_Pnt2d& lio_Point2D, gp_Pnt& lio_Point3D) {
    if (!m_Parent || !m_Parent->PartRefs.GetOperation()) return false;
    auto* sketchParams = m_Parent->PartRefs.GetParams();
    if (!sketchParams) return false;

    const double seuilCoincidenceMm = 0.5;
    bool AlignX = false;
    bool AlignY = false;
    double plusProcheDistanceX = seuilCoincidenceMm;
    double plusProcheDistanceY = seuilCoincidenceMm;

    gp_Pnt2d pointCibleExactVert2D;
    gp_Pnt2d pointCibleExactHor2D;
    gp_Pnt pointCibleExactVert3D;
    gp_Pnt pointCibleExactHor3D;

    // --- LA LAMBDA DE FACTORISATION ---
    auto evaluerPoint = [&](const gp_Pnt2d& pt2D, const gp_Pnt& pt3D) {
        // Test sur l'axe X (alignement vertical)
        double distX = std::abs(lio_Point2D.X() - pt2D.X());
        if (distX < plusProcheDistanceX) {
            plusProcheDistanceX = distX;
            pointCibleExactVert2D = pt2D;
            pointCibleExactVert3D = pt3D;
            AlignX = true;
        }

        // Test sur l'axe Y (alignement horizontal)
        double distY = std::abs(lio_Point2D.Y() - pt2D.Y());
        if (distY < plusProcheDistanceY) {
            plusProcheDistanceY = distY;
            pointCibleExactHor2D = pt2D;
            pointCibleExactHor3D = pt3D;
            AlignY = true;
        }
    };

    // --- 1. PARCOURS DIRECT DE TOUS LES POINTS GLOBAUX ---
    // (Inclut l'origine (0,0), les centres de cercles, extrémités de lignes, etc.)
    for (const auto& sketchPoint : sketchParams->getPoints()) {
        if (!sketchPoint.b_IsSnappable) continue;
        evaluerPoint(sketchPoint.p2d, sketchPoint.cache_p3d);
    }

    // --- 2. PARCOURS DES MILIEUX DE LIGNES (Optionnel si tu veux pouvoir t'aligner sur les milieux) ---
    for (const auto& primitive : sketchParams->getPrimitives()) {
        std::visit([&](const auto& concretePrim) {
            using T = std::decay_t<decltype(concretePrim)>;
            if constexpr (std::is_same_v<T, SketchLine>) {
                const SketchPoint& cstart = sketchParams->GetPointById(concretePrim.startPointId);
                const SketchPoint& cstop = sketchParams->GetPointById(concretePrim.stopPointId);

                if (cstart.b_IsSnappable && cstop.b_IsSnappable) {
                    gp_Pnt2d ptnConcreteMiddle2D(
                        (cstart.p2d.X() + cstop.p2d.X()) / 2.0,
                        (cstart.p2d.Y() + cstop.p2d.Y()) / 2.0
                        );
                    gp_Pnt ptnConcreteMiddle3D(
                        (cstart.cache_p3d.X() + cstop.cache_p3d.X()) / 2.0,
                        (cstart.cache_p3d.Y() + cstop.cache_p3d.Y()) / 2.0,
                        (cstart.cache_p3d.Z() + cstop.cache_p3d.Z()) / 2.0
                        );
                    evaluerPoint(ptnConcreteMiddle2D, ptnConcreteMiddle3D);
                }
            }
        }, primitive);
    }

    if (AlignX) {
        lio_Point2D.SetX(pointCibleExactVert2D.X());
        lio_Point3D.SetX(pointCibleExactVert3D.X());
    }
    if (AlignY) {
        lio_Point2D.SetY(pointCibleExactHor2D.Y());
        lio_Point3D.SetY(pointCibleExactHor3D.Y());
    }

    updateSnapLineActor(AlignX, AlignY, pointCibleExactVert3D, pointCibleExactHor3D, lio_Point3D);

    return (AlignX || AlignY);
}


void SketchSnapperManager::updateSnapLineActor(bool li_AlignX, bool li_AlignY,
                                                  const gp_Pnt& pointCibleVert3D,
                                                  const gp_Pnt& pointCibleHor3D,
                                                  const gp_Pnt& mousePoint3D) {
    m_snapLinePoints->Reset();
    m_snapLineCellsArray->Reset();
    m_snapLineTCoords->Reset();

    if (!li_AlignX && !li_AlignY) {
        m_ActorSnapLine->SetVisibility(false);
        return;
    }

    const double pasMm = 4.0;
    // 1. Calculer un pas dynamique en fonction du zoom de la caméra
    auto* camera = m_Parent->GetView()->getRenderer()->GetActiveCamera();
    double pasDynamiqueMm = 4.0; // Valeur par défaut de secours

    if (camera->GetParallelProjection()) {
        // En mode Orthographique (2D d'esquisse standard) :
        // ParallelScale donne la hauteur de la vue en unités du monde (mm).
        // On veut que notre motif de pointillé représente environ 1.5% de la hauteur de l'écran.
        pasDynamiqueMm = camera->GetParallelScale() * 0.015*3;
    } else {
        // En mode Perspective (si jamais vous basculez en vue 3D) :
        double distance = camera->GetDistance();
        pasDynamiqueMm = distance * 0.008*3;
    }
    // Sécurité pour éviter une division par zéro si la caméra bugue
    if (pasDynamiqueMm < 0.001) pasDynamiqueMm = 0.001;



    if (li_AlignX) {
        double distY = std::abs(mousePoint3D.Y() - pointCibleVert3D.Y());
        vtkIdType p1 = m_snapLinePoints->InsertNextPoint(pointCibleVert3D.X(), pointCibleVert3D.Y(), pointCibleVert3D.Z());
        vtkIdType p2 = m_snapLinePoints->InsertNextPoint(pointCibleVert3D.X(), mousePoint3D.Y(), mousePoint3D.Z());
        m_snapLineCellsArray->InsertNextCell({p1, p2});
        m_snapLineTCoords->InsertNextTuple2(0.0, 0.0);
        m_snapLineTCoords->InsertNextTuple2(distY / pasDynamiqueMm, 0.0);
    }

    if (li_AlignY) {
        double distX = std::abs(mousePoint3D.X() - pointCibleHor3D.X());
        vtkIdType p3 = m_snapLinePoints->InsertNextPoint(pointCibleHor3D.X(), pointCibleHor3D.Y(), pointCibleHor3D.Z());
        vtkIdType p4 = m_snapLinePoints->InsertNextPoint(mousePoint3D.X(), pointCibleHor3D.Y(), pointCibleHor3D.Z());
        m_snapLineCellsArray->InsertNextCell({p3, p4});
        m_snapLineTCoords->InsertNextTuple2(0.0, 0.0);
        m_snapLineTCoords->InsertNextTuple2(distX / pasDynamiqueMm, 0.0);
    }

    m_snapLinePoints->Modified();
    m_snapLineCellsArray->Modified();
    m_snapLineTCoords->Modified();
    m_ActorSnapLine->SetVisibility(true);
}

void SketchSnapperManager::aideHV(const gp_Pnt& p1, gp_Pnt& p2) {
    const double seuilAimantation = 1.8;
    const double zoneMorte = 5.0;

    double dx = std::abs(p1.X() - p2.X());
    double dy = std::abs(p1.Y() - p2.Y());

    if (dx < zoneMorte && dy < zoneMorte) return;

    if (dy < seuilAimantation) {
        p2.SetY( p1.Y() );
    } else if (dx < seuilAimantation) {
        p2.SetX( p1.X() );
    }
}
void SketchSnapperManager::aideHV(const gp_Pnt2d& p1, gp_Pnt2d& p2) {
    const double seuilAimantation = 1.8;
    const double zoneMorte = 5.0;

    double dx = std::abs(p1.X() - p2.X());
    double dy = std::abs(p1.Y() - p2.Y());

    if (dx < zoneMorte && dy < zoneMorte) return;

    if (dy < seuilAimantation) {
        p2.SetY( p1.Y() );
    } else if (dx < seuilAimantation) {
        p2.SetX( p1.X() );
    }
}

void SketchSnapperManager::ajusterEchelleCarreSnap() {
    if (!m_ActorSnapPoint || !m_ActorSnapPoint->GetVisibility() || !m_Parent->GetView()) return;
    vtkCamera* camera = m_Parent->GetView()->getRenderer()->GetActiveCamera();
    if (!camera) return;

    double currentScale = camera->GetParallelScale();
    if (std::abs(currentScale - m_derniereEchelleCarre) < 0.001) return;

    m_derniereEchelleCarre = currentScale;
    double facteurEchelle = currentScale * 0.08;
    m_ActorSnapPoint->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
}

void SketchSnapperManager::masquerFeedback() {
    if (m_ActorSnapPoint) m_ActorSnapPoint->SetVisibility(false);
    if (m_ActorSnapLine) m_ActorSnapLine->SetVisibility(false);
}








