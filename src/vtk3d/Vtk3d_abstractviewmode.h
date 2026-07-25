
#pragma once

class vtk3d_MainView;
class QMouseEvent;
class QWheelEvent;
#include <QKeyEvent>
#include "cad_events.h"

// ─────────────────────────────────────────────────────────────────────
// INTERFACE DE BASE ABSTRAITE
// ─────────────────────────────────────────────────────────────────────
class AbstractViewMode {
protected:
    vtk3d_MainView* m_view; // Pointeur vers la classe mère pour utiliser ses outils

public:
    explicit AbstractViewMode(vtk3d_MainView* view) : m_view(view) {}
    virtual ~AbstractViewMode() = default;

    vtk3d_MainView* GetView(){ return m_view;}

    // Cycle de vie du mode
    virtual void activer() = 0;
    virtual void desactiver() = 0;

    // Événements souris transférés depuis Qt
    virtual bool gererMousePress(QMouseEvent* event) = 0;
    virtual bool gererMouseRelease(QMouseEvent* event) = 0;
    virtual bool gererMouseMove(QMouseEvent* event) = 0;
    virtual bool gererWheelEvent(QWheelEvent* event) = 0;

    virtual void keyPressEvent(QKeyEvent* event) = 0;

    virtual void CADEvent_TraiterCommande(const CadCommandEvent& event) = 0;
};


