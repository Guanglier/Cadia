#include "mainwindow.h"

#include <QLabel>


void MainWindow::ToolbarPrimitives_Setup (){
	
	
	
    QWidget* groupPrimitives = new QWidget(this);
    groupPrimitives->setObjectName("RibbonGroup");

    QVBoxLayout* layoutPrimitives = new QVBoxLayout(groupPrimitives);
    layoutPrimitives->setContentsMargins(0, 0, 0, 0);
    layoutPrimitives->setSpacing(0);

    QToolBar* barPrimitives = new QToolBar(this);
    barPrimitives->setToolButtonStyle(Qt::ToolButtonIconOnly);
    //barPrimitives->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    barPrimitives->setIconSize(QSize(24, 24));
    barPrimitives->setStyleSheet(
        "QToolBar { spacing: 2px; padding: 4px; }"
        "QToolButton { padding: 3px; margin: 0px; }"
        );


    // Ajout des boutons d'outils de dessin geometry
    Sketch.Tool.actSelect = barPrimitives->addAction(QIcon(":/icons/sketch_select.svg"), tr("Line"));
    Sketch.Tool.actSelect->setToolTip(tr("Select primitives"));
    connect(Sketch.Tool.actSelect, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::Select
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    // Ajout des boutons d'outils de dessin geometry
    Sketch.Tool.actLine = barPrimitives->addAction(QIcon(":/icons/sketch_line.svg"), tr("Line"));
    //actLine->setCheckable(true); // Devient enfoncé quand on clique dessus
    Sketch.Tool.actLine->setToolTip(tr("Create a line by start and stop clic"));
    connect(Sketch.Tool.actLine, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::Draw_line
        };
        // Tu envoies la commande à ta vue 3D (qui la fera descendre vers l'outil)
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Tool.actCircle = barPrimitives->addAction(QIcon(":/icons/sketch_circle.svg"), tr("Circle"));
    //actCircle->setCheckable(true);
    Sketch.Tool.actCircle->setToolTip(tr("Create a circle by center and diameter"));
    connect(Sketch.Tool.actCircle, &QAction::triggered, this, [this]() {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::Draw_Circle
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Tool.actRectCenter = barPrimitives->addAction(QIcon(":/icons/sketch_rectangle_center.svg"), tr("Rectangle"));
    //actRectCenter->setCheckable(true);
    Sketch.Tool.actRectCenter->setToolTip(tr("Create a rectangle by center and dimension"));
    connect(Sketch.Tool.actRectCenter, &QAction::triggered, this, [this]() {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::Draw_RectCenter
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Tool.actRectCorners = barPrimitives->addAction(QIcon(":/icons/sketch_rectangle_edges.svg"), tr("Rectangle"));
    //actRectCorners->setCheckable(true);
    Sketch.Tool.actRectCorners->setToolTip(tr("Create a rectangle by its corners"));
    connect(Sketch.Tool.actRectCorners, &QAction::triggered, this, [this]() {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::ToolMode::Draw_RectEdges
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    // Un petit QLabel pour le bandeau bas
    QLabel* lblPrimitives = new QLabel(tr("PRIMITIVES 2D"), this);
    lblPrimitives->setAlignment(Qt::AlignCenter);
    lblPrimitives->setFixedHeight(16);
    lblPrimitives->setStyleSheet("font-size: 9px; color: #8A8A8A; font-weight: bold; border-top: 1px solid #EAEAEA; background: #ECECEC; margin: 0px;");

    layoutPrimitives->addWidget(barPrimitives, 1);
    layoutPrimitives->addWidget(lblPrimitives);
    TabLayout_BtnRapides->addWidget(groupPrimitives);



    // 🛑 PAR DÉFAUT : On cache l'onglet Esquisse au démarrage de l'appli
    // L'index de l'onglet Esquisse est 1 (Général = 0, Esquisse = 1, Part = 2, Visibilité = 3)
    //m_ribbonTabWidget->setTabVisible(2, false);


    // Tu créées un groupe pour que les boutons soient exclusifs entre eux


    //--- partie pour n'avoir qu'un bouton enfoncé, et ce de manière automatique -----
    // Tu rends les actions "checkable" et tu les ajoutes au groupe
	
	


}








