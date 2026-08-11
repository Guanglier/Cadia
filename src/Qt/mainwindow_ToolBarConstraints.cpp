
#include "mainwindow.h"
#include "Dialog_ConstraintPopup.h"

#include <QLabel>
#include <qboxlayout.h>


void MainWindow::ToolbarConstraints_Setup (){

    QWidget* groupConstraints = new QWidget(this);
    groupConstraints->setObjectName("RibbonGroup");

    QVBoxLayout* layoutConstraints = new QVBoxLayout(groupConstraints);
    layoutConstraints->setContentsMargins(0, 0, 0, 0);
    layoutConstraints->setSpacing(0);

    QToolBar* barConstraints = new QToolBar(this);
    barConstraints->setToolButtonStyle(Qt::ToolButtonIconOnly);
    barConstraints->setIconSize(QSize(24, 24));
    barConstraints->setStyleSheet(
        "QToolBar { spacing: 2px; padding: 4px; }"
        "QToolButton { padding: 3px; margin: 0px; }"
        );


    // Ajout des boutons d'outils de dessin geometry
    Sketch.Constraints.actConstHorizontal = barConstraints->addAction(QIcon(":/icons/sketch_constraint_horizontal.svg"), tr("Line"));
    Sketch.Constraints.actConstHorizontal->setToolTip(tr("Horizontal constraint"));
    connect(Sketch.Constraints.actConstHorizontal, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::SetConstraints,
            CadEvent::Sketch::Tool_SubMode{ CadEvent::Sketch::ConstraintSubMode::Horizontal}
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });


    Sketch.Constraints.actConstVertical = barConstraints->addAction(QIcon(":/icons/sketch_constraint_vertical.svg"), tr("Line"));
    Sketch.Constraints.actConstVertical->setToolTip(tr("Vertical constraint"));
    connect(Sketch.Constraints.actConstVertical, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::SetConstraints,
            CadEvent::Sketch::Tool_SubMode{ CadEvent::Sketch::ConstraintSubMode::Vertical}
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Constraints.actConstPerpendicular = barConstraints->addAction(QIcon(":/icons/sketch_constraint_perpendicular.svg"), tr("Line"));
    Sketch.Constraints.actConstPerpendicular->setToolTip(tr("Perpendicular constraint"));
    connect(Sketch.Constraints.actConstPerpendicular, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::SetConstraints,
            CadEvent::Sketch::Tool_SubMode{ CadEvent::Sketch::ConstraintSubMode::Perpendicular}
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    //------------- DISTANCE -----------------------------
    Sketch.Constraints.actConstDistance = barConstraints->addAction(QIcon(":/icons/sketch_constraint_distance.svg"), tr("Line"));
    Sketch.Constraints.actConstDistance->setToolTip(tr("Distance constraint"));
    connect(Sketch.Constraints.actConstDistance, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::SetConstraints,
            CadEvent::Sketch::Tool_SubMode{ CadEvent::Sketch::ConstraintSubMode::Distance}
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });




    Sketch.Constraints.actConstParallel = barConstraints->addAction(QIcon(":/icons/sketch_constraint_parallel.svg"), tr("Line"));
    Sketch.Constraints.actConstParallel->setToolTip(tr("Parallel constraint"));
    connect(Sketch.Constraints.actConstParallel, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::SetConstraints,
            CadEvent::Sketch::Tool_SubMode{ CadEvent::Sketch::ConstraintSubMode::Parallel}
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Constraints.actConstResolve = barConstraints->addAction(QIcon(":/icons/sketch_constraint_resolve.svg"), tr("Line"));
    Sketch.Constraints.actConstResolve->setToolTip(tr("Resolve constraint"));
    /*
    connect(Sketch.Constraints.actConstResolve, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdConstraints{
            CadEvent::Sketch::Constraints::Constraint_Resolve
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });
*/


    Sketch.Tool.configure_all();
    Sketch.Constraints.configure_all();


    // Un petit QLabel pour le bandeau bas
    QLabel* lblConstraints = new QLabel(tr("CONTRAINTES 2D"), this);
    lblConstraints->setAlignment(Qt::AlignCenter);
    lblConstraints->setFixedHeight(16);
    lblConstraints->setStyleSheet("font-size: 9px; color: #8A8A8A; font-weight: bold; border-top: 1px solid #EAEAEA; background: #ECECEC; margin: 0px;");

    layoutConstraints->addWidget(barConstraints, 1);
    layoutConstraints->addWidget(lblConstraints);
    TabLayout_BtnRapides->addWidget(groupConstraints);


}






