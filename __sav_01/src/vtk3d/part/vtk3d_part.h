#pragma once

#include "Vtk3d_abstractviewmode.h"

#include <QObject>
#include <vtkSmartPointer.h>
#include <vtkType.h>
#include <QPoint.h>


class vtk3d_MainView;
class QMouseEvent;



// ─────────────────────────────────────────────────────────────────────
// MODE PIÈCE 3D
// ─────────────────────────────────────────────────────────────────────
class Vtk3d_Part : public AbstractViewMode {
private:
    QPoint     m_MouseclickStartPosition;
public:
    explicit Vtk3d_Part(vtk3d_MainView* view);
    ~Vtk3d_Part() override = default;

    void activer() override;
    void desactiver() override;

    bool gererMousePress(QMouseEvent* event) override;
    bool gererMouseMove(QMouseEvent* event) override;
    bool gererMouseRelease(QMouseEvent* event) override;
    bool gererWheelEvent(QWheelEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override {  }

    void CADEvent_TraiterCommande(const CadCommandEvent& event);


};






