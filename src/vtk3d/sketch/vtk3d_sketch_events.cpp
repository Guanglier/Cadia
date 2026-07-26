#include "vtk3d_sketch.h"
#include "vtk3d_MainView.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <iostream>
#include <cmath>
#include "2DSolver_Mapper.h"



//#define WHEELEVENT_DEBUG
#ifdef WHEELEVENT_DEBUG
#define LOG_WHEELEVENT_TYPE	LOG_DEBUG
#endif

//#defin ACTIVE_LOG_GERER_MOUSERELEASE
#ifdef ACTIVE_LOG_GERER_MOUSERELEASE
#define LOG_GERER_MOUSERELEASE_TYPE	LOG_DEBUG
#endif

//#defin ACTIVE_LOG_GERER_MOUSEPRESS
#ifdef ACTIVE_LOG_GERER_MOUSEPRESS
#define LOG_GERER_MOUSEPRESS_TYPE	LOG_DEBUG
#endif

bool Vtk3d_Sketch::gererMousePress(QMouseEvent* event) {

    if (event->button() == Qt::MiddleButton) {
        MousePan.m_isPanning = true;
        MousePan.m_lastPanPos = event->pos();
#ifdef ACTIVE_LOG_GERER_MOUSEPRESS
        LOG_GERER_MOUSEPRESS_TYPE << "[DIAG PAN] Début du Pan (Clic milieu) à pos: ("
                                  << MousePan.m_lastPanPos.x() << ", " << MousePan.m_lastPanPos.y() << ")" << std::endl;
#endif
        return true; // On consomme l'événement
    }

    std::visit([event](auto& _tool) {
        _tool.gererMousePress(event);
    }, m_tool);
    return false;
}

bool Vtk3d_Sketch::gererMouseRelease(QMouseEvent* event) {
#ifdef ACTIVE_LOG_GERER_MOUSERELEASE
    LOG_GERER_MOUSERELEASE_TYPE << " Vtk3d_Sketch::gererMouseRelease " << std::flush;
#endif

    if (event->button() == Qt::MiddleButton && MousePan.m_isPanning) {
        MousePan.m_isPanning = false;
#ifdef ACTIVE_LOG_GERER_MOUSERELEASE
        LOG_GERER_MOUSERELEASE_TYPE << "[DIAG PAN] Fin du Pan (Relâchement clic milieu)" << std::endl;
#endif
        return true;
    }

    std::visit([event](auto& _tool) {
        _tool.gererMouseRelease(event);
    }, m_tool);
    return true;
}

bool Vtk3d_Sketch::gererMouseMove(QMouseEvent* event) {
    if (!m_view) return false;


    if (MousePan.m_isPanning ){

        vtkRenderer* renderer = m_view->getRenderer();
        vtkCamera* camera = renderer ? renderer->GetActiveCamera() : nullptr;
        if (!camera) return false;

        int width = m_view->width();
        int height = m_view->height();
        if (width <= 0 || height <= 0) return false;

        // 1. Calcul du déplacement en pixels par rapport à la dernière position
        QPoint currentPos = event->pos();
        QPoint delta = currentPos - MousePan.m_lastPanPos;
        MousePan.m_lastPanPos = currentPos;

        if (delta.isNull()) return true;

        // 2. Convertir le déplacement écran en coordonnées mondiales 3D uniformes
        double scale = camera->GetParallelScale();

        // Le ParallelScale est lié à la hauteur de la vue, donc on divise les deux par 'height'
        double factor = (2.0 * scale) / height;

        // On garde le même coefficient pour X et Y pour que la vitesse soit strictement identique
        double worldDx = static_cast<double>(delta.x()) * factor;
        double worldDy = static_cast<double>(delta.y()) * factor;

        // 3. Récupérer la position actuelle et le point de visée (FocalPoint)
        double fpoint[3];
        camera->GetFocalPoint(fpoint);
        double pos[3];
        camera->GetPosition(pos);

        // 4. Décaler la caméra et le centre de visée selon le plan de la caméra (axes Right et Up)
        double vup[3], vprn[3], right[3];
        camera->GetViewUp(vup);
        camera->GetDirectionOfProjection(vprn);

        // Calcul du vecteur Right (produit vectoriel entre la direction de projection et ViewUp)
        right[0] = vup[1] * vprn[2] - vup[2] * vprn[1];
        right[1] = vup[2] * vprn[0] - vup[0] * vprn[2];
        right[2] = vup[0] * vprn[1] - vup[1] * vprn[0];

        // Normalisation de Right
        double len = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
        if (len > 0.0) {
            right[0] /= len; right[1] /= len; right[2] /= len;
        }

        // Application du déplacement global
        double dx = right[0] * worldDx + vup[0] * worldDy;
        double dy = right[1] * worldDx + vup[1] * worldDy;
        double dz = right[2] * worldDx + vup[2] * worldDy;

        camera->SetFocalPoint(fpoint[0] + dx, fpoint[1] + dy, fpoint[2] + dz);
        camera->SetPosition(pos[0] + dx, pos[1] + dy, pos[2] + dz);

        // 5. Mise à jour du rendu
        camera->Modified();
        renderer->ResetCameraClippingRange();
        m_view->renderWindow()->Render();

    }


    std::visit([event](auto& _tool) {
        _tool.gererMouseMove(event);
    }, m_tool);
    return true;
}





bool Vtk3d_Sketch::gererWheelEvent(QWheelEvent* event) {
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "\n--- [DIAG WHEEL] DÉBUT ---" << std::endl;
#endif
    if (!m_view) {
#ifdef WHEELEVENT_DEBUG
        LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] ERREUR : m_view est nullptr !" << std::endl;
#endif
        return false;
    }

    vtkRenderer* renderer = m_view->getRenderer();
    if (!renderer) {
#ifdef WHEELEVENT_DEBUG
        LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] ERREUR : renderer est nullptr !" << std::endl;
#endif
        return false;
    }

    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) {
#ifdef WHEELEVENT_DEBUG
        LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] ERREUR : camera active est nullptr !" << std::endl;
#endif
        return false;
    }

    if (!camera->GetParallelProjection()) {
#ifdef WHEELEVENT_DEBUG
        LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] ATTENTION : La caméra n'est PAS en projection parallèle !" << std::endl;
#endif
        return false;
    }

    double delta = event->angleDelta().y();
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] Delta molette : " << delta << std::endl;
#endif
    if (delta == 0) return false;

    double factor = (delta > 0) ? 1.15 : (1.0 / 1.15);

    int width = m_view->width();
    int height = m_view->height();
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] Dimensions widget (w x h) : " << width << " x " << height << std::endl;
#endif
    if (width <= 0 || height <= 0) return false;

    // 1. Position de la souris
    double mouseX = event->position().x();
    double mouseY = event->position().y();
    double vtkMouseY = height - mouseY;
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] Souris Qt (X, Y) = (" << mouseX << ", " << mouseY << ") | VTK Y = " << vtkMouseY << std::endl;
#endif

    // 2. Trouver le point 3D exact sous la souris AVANT le zoom
    double worldBefore[4];
    renderer->SetDisplayPoint(mouseX, vtkMouseY, 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(worldBefore);
    if (worldBefore[3] != 0.0) {
        worldBefore[0] /= worldBefore[3];
        worldBefore[1] /= worldBefore[3];
        worldBefore[2] /= worldBefore[3];
    }
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] Monde AVANT zoom = (" << worldBefore[0] << ", " << worldBefore[1] << ", " << worldBefore[2] << ")" << std::endl;
#endif

    // 3. Appliquer le changement d'échelle
    double oldScale = camera->GetParallelScale();
    double newScale = oldScale / factor;
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] ParallelScale : Ancien = " << oldScale << " | Nouveau = " << newScale << std::endl;
#endif

    if (newScale <= 1e-4 || newScale >= 1e7) {
#ifdef WHEELEVENT_DEBUG
        LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] ERREUR : Échelle hors limites !" << std::endl;
#endif
        return false;
    }
    camera->SetParallelScale(newScale);

    // 4. Trouver où ce même point se projette à l'écran APRÈS le changement d'échelle
    renderer->SetWorldPoint(worldBefore[0], worldBefore[1], worldBefore[2], 1.0);
    renderer->WorldToDisplay();
    double displayAfter[3];
    renderer->GetDisplayPoint(displayAfter);

    // 5. Calculer le décalage en coordonnées monde pour ramener le point sous la souris
    renderer->SetDisplayPoint(mouseX, vtkMouseY, displayAfter[2]);
    renderer->DisplayToWorld();
    double worldTarget[4];
    renderer->GetWorldPoint(worldTarget);
    if (worldTarget[3] != 0.0) {
        worldTarget[0] /= worldTarget[3];
        worldTarget[1] /= worldTarget[3];
        worldTarget[2] /= worldTarget[3];
    }
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] Cible monde APRÈS zoom = (" << worldTarget[0] << ", " << worldTarget[1] << ", " << worldTarget[2] << ")" << std::endl;
#endif

    // 6. Déplacer la caméra (FocalPoint et Position) de la différence
    double fpoint[3];
    camera->GetFocalPoint(fpoint);
    double pos[3];
    camera->GetPosition(pos);

    double dx = worldBefore[0] - worldTarget[0];
    double dy = worldBefore[1] - worldTarget[1];
    double dz = worldBefore[2] - worldTarget[2];
#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "[DIAG WHEEL] Décalage appliqué (dx, dy, dz) = (" << dx << ", " << dy << ", " << dz << ")" << std::endl;
#endif

    camera->SetFocalPoint(fpoint[0] + dx, fpoint[1] + dy, fpoint[2] + dz);
    camera->SetPosition(pos[0] + dx, pos[1] + dy, pos[2] + dz);

    Echelle_ajusterEchelleElements(camera);

    camera->Modified();
    renderer->ResetCameraClippingRange();
    m_view->renderWindow()->Render();

#ifdef WHEELEVENT_DEBUG
    LOG_WHEELEVENT_TYPE << "--- [DIAG WHEEL] SUCCÈS ---\n" << std::endl;
#endif
    return true;
}







void Vtk3d_Sketch::keyPressEvent(QKeyEvent* event) {
    std::visit([event](auto& activeTool) {
        activeTool.gererkeyPressEvent(event);
    }, m_tool);

    if (event->key() == Qt::Key_Delete) {
#ifdef DBG_keyPressEvent
        std::cout << "🎯 Touche [Suppr] détectée dans le Sketcher !" << std::endl;
#endif
        return;
    }

    if (event->key() == Qt::Key_S) sketch_ActivateTool(SketchTool_mode::Tool_Select);
    if (event->key() == Qt::Key_L) sketch_ActivateTool(SketchTool_mode::Tool_Line);
    if (event->key() == Qt::Key_C) sketch_ActivateTool(SketchTool_mode::Tool_CircleDraw);
    if (event->key() == Qt::Key_R) sketch_ActivateTool(SketchTool_mode::Tool_RectEdgesDraw);
    if (event->key() == Qt::Key_T) sketch_ActivateTool(SketchTool_mode::Tool_RectCenterDraw);
    if (event->key() == Qt::Key_D) sketch_ActivateTool(SketchTool_mode::Tool_Dimensions);


    if (event->key() == Qt::Key_M){
        if ( nullptr != DocumentRefs.GetOperation() ){
            DocumentRefs.GetOperation()->setLocaleTopoChanged(true);
            auto *SketchParam = std::get_if<SketchParams> (&DocumentRefs.GetOperation()->getParamsMutable() );
            if ( nullptr != SketchParam ){
                //Solver2D_Mapper::Solve(*SketchParam);
                //Solver2D_Mapper::SolveWithDiagnostics(*SketchParam);
                //Solver2D_Mapper::PrepareAndSolve(*SketchParam);
                SolveEsquisse();
            }

        }
    }


}














