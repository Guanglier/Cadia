// mainwindow.cpp
#include "mainwindow.h"

#include <QDockWidget>
#include <QMdiSubWindow>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include <QInputDialog>
#include <QDebug>
#include <QLabel>
#include "QActionGroup"

#include <QDirIterator>
#include <QDebug>
#include <QFileInfo>

#include "cad_events.h"

// Dans le constructeur de ta MainWindow :
void MainWindow::setupTreeView() {
    m_cadTreeModel = new CadTreeModel(this);
    m_treeView->setModel(m_cadTreeModel);

    // 3. Activation du menu contextuel personnalisé sur la vue
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::onTreeShowContextMenu);
    connect(m_treeView, &QTreeView::doubleClicked, this, &MainWindow::onTreeViewOperation_clicked);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // 1. On donne TOUT DE SUITE la taille finale à la fenêtre principale
    resize(1280, 720);

    m_mdiArea = new QMdiArea(this);
    m_mdiArea->setViewMode(QMdiArea::SubWindowView);
    setCentralWidget(m_mdiArea);

    createActions();
    createMenus();

    // 2. Création de l'arborescence
    QDockWidget* dock = new QDockWidget("Arbre du modèle", this);
    m_treeView = new QTreeView(dock);
    m_treeView->setHeaderHidden(false);
    m_treeView->setIconSize(QSize(20, 20));
    dock->setWidget(m_treeView);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    // 3. Création de la vue 3D (Le showMaximized profitera direct des 1280x720)
    m_view3d = new vtk3d_MainView(this);
    m_view3d->CADEvent_RemonterEvent_SetCallback([this](const CadResponseEvent& resp) {
        if (auto* status = std::get_if<CadEvent::Sketch::RespStatus>(&resp.params)) {
            this->statusBar()->showMessage(QString::fromStdString(status->text));
        }
        if (auto* status = std::get_if<CadEvent::Sketch::RespChangedTool>(&resp.params)) {
            //this->statusBar()->showMessage(QString::fromStdString(status->toolMode));
            Sketch.Tool.actCircle->setChecked(false);
            Sketch.Tool.actLine->setChecked(false);
            Sketch.Tool.actRectCenter->setChecked(false);
            Sketch.Tool.actRectCorners->setChecked(false);
            Sketch.Tool.actSelect->setChecked(false);
            switch(status->toolMode){
                case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_line:
                    Sketch.Tool.actLine->setChecked(true);
                    break;
                case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_Circle:
                    Sketch.Tool.actCircle->setChecked(true);
                    break;
                case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_RectEdges:
                    Sketch.Tool.actRectCorners->setChecked(true);
                    break;
                case CadEvent::Sketch::CadEvent_SketchToolMode::Draw_RectCenter:
                    Sketch.Tool.actRectCenter->setChecked(true);
                    break;
                case CadEvent::Sketch::CadEvent_SketchToolMode::Select:
                    Sketch.Tool.actSelect->setChecked(true);
                    break;
            default:
                this->statusBar()->showMessage( "err CadEvent::Sketch::RespChangedTool" );
                break;
            }
        }


    });

    QMdiSubWindow* subWin = m_mdiArea->addSubWindow(m_view3d);
    subWin->setWindowTitle("Vue 3D - Pièce Prototype");
    subWin->resize(800, 600);
    subWin->showMaximized();

    createRibbon();


    //createToolBars_NewOpenSave();
    //creerToolbarVisibilite ();
    //createToolBars_vues ();

    // 4. Remplissage et calcul du document
    doc.tst_add_repere_origine();
    doc.tst_add_op_sketch_rect();
    doc.tst_add_op_extrude();
    //doc.tst_add_op_sketch_circle();
    //doc.tst_add_op_extrude_2();
    doc.compute_final_topo();
    //doc.tst_dump_all_op_to_file();
    //doc.tst_dump_tree_to_file("__tree.txt");

    // 5. Synchronisation de la géométrie
    m_view3d->synchroniserDocument(1, doc);

    // 6. UI Tree View
    setupTreeView();
    m_cadTreeModel->refreshFromDocument(doc);
    m_treeView->expandAll();

    SetAffichage_Part ();

    //ListeIcones ();
}



void MainWindow::createRibbon()
{
    // 1️⃣ Création du conteneur principal tout en haut
    QToolBar* ribbonContainer = addToolBar(tr("RibbonContainer"));
    ribbonContainer->setMovable(false);
    ribbonContainer->setFloatable(false);
    ribbonContainer->setAllowedAreas(Qt::TopToolBarArea);
    ribbonContainer->setStyleSheet("QToolBar { background: #F5F5F5; border: none; padding: 0px; margin: 0px; }");

    m_ribbonTabWidget = new QTabWidget(this);
    m_ribbonTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // 📐 Hauteur totale du ruban légèrement réduite et verrouillée à 100px pour être très compact
    m_ribbonTabWidget->setFixedHeight(100);

    // 🎨 QSS global pour transformer les boutons et caler les sections
    m_ribbonTabWidget->setStyleSheet(
        "QTabWidget::pane { border-top: 1px solid #C5C5C5; position: absolute; top: -1px; background: #F5F5F5; }"
        "QTabBar::tab { background: #E1E1E1; border: 1px solid #C5C5C5; padding: 5px 18px; margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; font-size: 11px; }"
        "QTabBar::tab:selected { background: #F5F5F5; border-bottom-color: #F5F5F5; font-weight: bold; }"
        "QTabBar::tab:hover:!selected { background: #ECECEC; }"

        // Bordure droite pour séparer les catégories
        "QWidget#RibbonGroup { border-right: 1px solid #D0D0D0; background: #F5F5F5; }"

        // Style "Flat" pour les boutons du ruban (Style Office/CAD)
        "QToolBar { background: transparent; border: none; padding: 4px; spacing: 4px; }"
        "QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 4px 8px; font-size: 11px; }"
        "QToolButton:hover { background: #EAEAEA; border: 1px solid #C5C5C5; }"
        "QToolButton:pressed, QToolButton:checked { background: #DBDBDB; border: 1px solid #A5A5A5; }"
        );

    // =========================================================================
    // OGLER : GENERAL (Exemple rapide)
    // =========================================================================
    QToolBar* generalToolBar = new QToolBar(this);
    generalToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    generalToolBar->addAction(m_newAction);
    generalToolBar->addAction(m_openAction);
    generalToolBar->addAction(m_saveAction);
    generalToolBar->addSeparator();
    generalToolBar->addAction(m_optionsAction);
    m_ribbonTabWidget->addTab(generalToolBar, tr("Général"));

    // =========================================================================
    // 2️⃣ ONGLET : VISIBILITÉ (Optimisé Marges Piles + Titres full-width)
    // =========================================================================
    QWidget* visTabContent = new QWidget(this);
    QHBoxLayout* visTabLayout = new QHBoxLayout(visTabContent);
    visTabLayout->setContentsMargins(0, 0, 0, 0);
    visTabLayout->setSpacing(0);
    visTabLayout->setAlignment(Qt::AlignLeft);

    // -------------------------------------------------------------------------
    // 📦 GROUPE A : FILTRES
    // -------------------------------------------------------------------------
    QWidget* groupFiltres = new QWidget(this);
    groupFiltres->setObjectName("RibbonGroup");
    QVBoxLayout* layoutFiltres = new QVBoxLayout(groupFiltres);
    layoutFiltres->setContentsMargins(0, 0, 0, 0); // 🔥 Crucial : Zéro marge pour coller aux bords !
    layoutFiltres->setSpacing(0);

    QToolBar* barFiltres = new QToolBar(this);
    barFiltres->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* actRepere = barFiltres->addAction(tr("Repère"));
    actRepere->setCheckable(true); actRepere->setChecked(true);
    connect(actRepere, &QAction::toggled, m_view3d, [this](bool checked) {
        this->m_view3d->setCategoryVisibility(SelectionType::Axis, checked);
    });

    QAction* actSolides = barFiltres->addAction(tr("Solides"));
    actSolides->setCheckable(true); actSolides->setChecked(true);
    connect(actSolides, &QAction::toggled, this, [this](bool checked) {
        this->m_view3d->setCategoryVisibility(SelectionType::Face, checked);
    });

    QAction* actSketches = barFiltres->addAction(tr("Esquisses"));
    actSketches->setCheckable(true); actSketches->setChecked(true);
    connect(actSketches, &QAction::toggled, this, [this](bool checked) {
        this->m_view3d->setCategoryVisibility(SelectionType::Sketch, checked);
    });

    // Bandeau de texte en bas : prend 100% de la largeur de la section
    QLabel* lblFiltres = new QLabel(tr("FILTRES"), this);
    lblFiltres->setAlignment(Qt::AlignCenter);
    lblFiltres->setFixedHeight(16); // 📐 Hauteur "juste ce qu'il faut"
    lblFiltres->setStyleSheet("font-size: 9px; color: #8A8A8A; font-weight: bold; border-top: 1px solid #EAEAEA; background: #ECECEC; margin: 0px;");

    layoutFiltres->addWidget(barFiltres, 1); // La toolbar prend tout l'espace du haut
    layoutFiltres->addWidget(lblFiltres);    // Le titre se cale pile en bas
    visTabLayout->addWidget(groupFiltres);

    // -------------------------------------------------------------------------
    // 📦 GROUPE B : VUES
    // -------------------------------------------------------------------------
    QWidget* groupVues = new QWidget(this);
    groupVues->setObjectName("RibbonGroup");
    QVBoxLayout* layoutVues = new QVBoxLayout(groupVues);
    layoutVues->setContentsMargins(0, 0, 0, 0); // 🔥 Zéro marge ici aussi
    layoutVues->setSpacing(0);

    QToolBar* barVues = new QToolBar(this);
    barVues->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* actFront = barVues->addAction(tr("Vue Face"));
    QAction* actSide = barVues->addAction(tr("Vue Côté"));
    QAction* actIso = barVues->addAction(tr("Vue Iso"));
    connect(actFront, &QAction::triggered, m_view3d, &vtk3d_MainView::setViewFront);
    connect(actSide,  &QAction::triggered, m_view3d, &vtk3d_MainView::setViewSide);
    connect(actIso,   &QAction::triggered, m_view3d, &vtk3d_MainView::setViewIsometric);

    // Bandeau de texte pour les Vues
    QLabel* lblVues = new QLabel(tr("VUES INTERNES"), this);
    lblVues->setAlignment(Qt::AlignCenter);
    lblVues->setFixedHeight(16); // 📐 Même hauteur
    lblVues->setStyleSheet("font-size: 9px; color: #8A8A8A; font-weight: bold; border-top: 1px solid #EAEAEA; background: #ECECEC; margin: 0px;");

    layoutVues->addWidget(barVues, 1);
    layoutVues->addWidget(lblVues);
    visTabLayout->addWidget(groupVues);

    // Intégration du contenu
    m_ribbonTabWidget->addTab(visTabContent, tr("Visibilité"));
    ribbonContainer->addWidget(m_ribbonTabWidget);


    // =========================================================================
    // 2️ ONGLET : ESQUISSE (Contextuel & Dynamique)
    // =========================================================================
    QWidget* esquisseTabContent = new QWidget(this);
    esquisseTabContent->setObjectName("TabEsquisse");
    QHBoxLayout* esquisseTabLayout = new QHBoxLayout(esquisseTabContent);
    esquisseTabLayout->setContentsMargins(0, 0, 0, 0);
    esquisseTabLayout->setSpacing(0);
    esquisseTabLayout->setAlignment(Qt::AlignLeft);

    // -------------------------------------------------------------------------
    // 📦 GROUPE A : GESTION / SORTIE
    // -------------------------------------------------------------------------
    QWidget* groupSortie = new QWidget(this);
    groupSortie->setObjectName("RibbonGroup");
    QVBoxLayout* layoutSortie = new QVBoxLayout(groupSortie);
    layoutSortie->setContentsMargins(0, 0, 0, 0);
    layoutSortie->setSpacing(0);

    QToolBar* barSortie = new QToolBar(this);
    barSortie->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    // Bouton Quitter connecté directement à ton slot de retour au Mode 3D[cite: 7]
    QAction* actExitSketch = barSortie->addAction(QIcon(":/icons/exit.png"), tr("Quitter"));
    connect(actExitSketch, &QAction::triggered, this, &MainWindow::on_test_Mode3D);

    QLabel* lblSortie = new QLabel(tr("EDITION"), this);
    lblSortie->setAlignment(Qt::AlignCenter);
    lblSortie->setFixedHeight(16);
    lblSortie->setStyleSheet("font-size: 9px; color: #8A8A8A; font-weight: bold; border-top: 1px solid #EAEAEA; background: #ECECEC; margin: 0px;");

    layoutSortie->addWidget(barSortie, 1);
    layoutSortie->addWidget(lblSortie);
    esquisseTabLayout->addWidget(groupSortie);

    // -------------------------------------------------------------------------
    // 📦 GROUPE B : PRIMITIVES
    // -------------------------------------------------------------------------
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
            CadEvent::Sketch::CadEvent_SketchToolMode::Select
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
            CadEvent::Sketch::CadEvent_SketchToolMode::Draw_line
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
            CadEvent::Sketch::CadEvent_SketchToolMode::Draw_Circle
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Tool.actRectCenter = barPrimitives->addAction(QIcon(":/icons/sketch_rectangle_center.svg"), tr("Rectangle"));
    //actRectCenter->setCheckable(true);
    Sketch.Tool.actRectCenter->setToolTip(tr("Create a rectangle by center and dimension"));
    connect(Sketch.Tool.actRectCenter, &QAction::triggered, this, [this]() {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::CadEvent_SketchToolMode::Draw_RectCenter
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Tool.actRectCorners = barPrimitives->addAction(QIcon(":/icons/sketch_rectangle_edges.svg"), tr("Rectangle"));
    //actRectCorners->setCheckable(true);
    Sketch.Tool.actRectCorners->setToolTip(tr("Create a rectangle by its corners"));
    connect(Sketch.Tool.actRectCorners, &QAction::triggered, this, [this]() {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdActivateTool{
            CadEvent::Sketch::CadEvent_SketchToolMode::Draw_RectEdges
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
    esquisseTabLayout->addWidget(groupPrimitives);

    // Ajout de l'onglet complet au composant
    m_ribbonTabWidget->addTab(esquisseTabContent, tr("Esquisse"));

    // 🛑 PAR DÉFAUT : On cache l'onglet Esquisse au démarrage de l'appli
    // L'index de l'onglet Esquisse est 1 (Général = 0, Esquisse = 1, Part = 2, Visibilité = 3)
    //m_ribbonTabWidget->setTabVisible(2, false);


    // Tu créées un groupe pour que les boutons soient exclusifs entre eux


    //--- partie pour n'avoir qu'un bouton enfoncé, et ce de manière automatique -----
    // Tu rends les actions "checkable" et tu les ajoutes au groupe
    Sketch.Tool.actSelect->setCheckable(true);
    Sketch.Tool.actLine->setCheckable(true);
    Sketch.Tool.actCircle->setCheckable(true);
    Sketch.Tool.actRectCenter->setCheckable(true);
    Sketch.Tool.actRectCorners->setCheckable(true);



    // -------------------------------------------------------------------------
    // 📦 GROUPE C : Contraintes
    // -------------------------------------------------------------------------
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
        //CadCommandEvent evt;
        //evt.params = CadEvent::Sketch::CmdActivateTool{
        //    CadEvent::Sketch::CadEvent_SketchToolMode::Select
        //};
        //m_view3d->CADEvent_TraiterCommande(evt);
    });
    Sketch.Constraints.actConstVertical = barConstraints->addAction(QIcon(":/icons/sketch_constraint_vertical.svg"), tr("Line"));
    Sketch.Constraints.actConstVertical->setToolTip(tr("Vertical constraint"));
    connect(Sketch.Constraints.actConstVertical, &QAction::triggered, this, [this](bool checked) {
        //CadCommandEvent evt;
        //evt.params = CadEvent::Sketch::CmdActivateTool{
        //    CadEvent::Sketch::CadEvent_SketchToolMode::Select
        //};
        //m_view3d->CADEvent_TraiterCommande(evt);
    });
    Sketch.Constraints.actConstPerpendicular = barConstraints->addAction(QIcon(":/icons/sketch_constraint_perpendicular.svg"), tr("Line"));
    Sketch.Constraints.actConstPerpendicular->setToolTip(tr("Perpendicular constraint"));
    connect(Sketch.Constraints.actConstPerpendicular, &QAction::triggered, this, [this](bool checked) {
        //CadCommandEvent evt;
        //evt.params = CadEvent::Sketch::CmdActivateTool{
        //    CadEvent::Sketch::CadEvent_SketchToolMode::Select
        //};
        //m_view3d->CADEvent_TraiterCommande(evt);
    });
    Sketch.Constraints.actConstDistance = barConstraints->addAction(QIcon(":/icons/sketch_constraint_distance.svg"), tr("Line"));
    Sketch.Constraints.actConstDistance->setToolTip(tr("Distance constraint"));
    connect(Sketch.Constraints.actConstDistance, &QAction::triggered, this, [this](bool checked) {
        //CadCommandEvent evt;
        //evt.params = CadEvent::Sketch::CmdActivateTool{
        //    CadEvent::Sketch::CadEvent_SketchToolMode::Select
        //};
        //m_view3d->CADEvent_TraiterCommande(evt);
    });
    Sketch.Constraints.actConstParallel = barConstraints->addAction(QIcon(":/icons/sketch_constraint_parallel.svg"), tr("Line"));
    Sketch.Constraints.actConstParallel->setToolTip(tr("Parallel constraint"));
    connect(Sketch.Constraints.actConstParallel, &QAction::triggered, this, [this](bool checked) {
        //CadCommandEvent evt;
        //evt.params = CadEvent::Sketch::CmdActivateTool{
        //    CadEvent::Sketch::CadEvent_SketchToolMode::Select
        //};
        //m_view3d->CADEvent_TraiterCommande(evt);
    });

    Sketch.Constraints.actConstResolve = barConstraints->addAction(QIcon(":/icons/sketch_constraint_resolve.svg"), tr("Line"));
    Sketch.Constraints.actConstResolve->setToolTip(tr("Resolve constraint"));
    connect(Sketch.Constraints.actConstResolve, &QAction::triggered, this, [this](bool checked) {
        CadCommandEvent evt;
        evt.params = CadEvent::Sketch::CmdConstraints{
            CadEvent::Sketch::CadEvent_SketchConstraints::Constraint_Resolve
        };
        m_view3d->CADEvent_TraiterCommande(evt);
    });



    // Un petit QLabel pour le bandeau bas
    QLabel* lblConstraints = new QLabel(tr("CONTRAINTES 2D"), this);
    lblConstraints->setAlignment(Qt::AlignCenter);
    lblConstraints->setFixedHeight(16);
    lblConstraints->setStyleSheet("font-size: 9px; color: #8A8A8A; font-weight: bold; border-top: 1px solid #EAEAEA; background: #ECECEC; margin: 0px;");

    layoutConstraints->addWidget(barConstraints, 1);
    layoutConstraints->addWidget(lblConstraints);
    esquisseTabLayout->addWidget(groupConstraints);


}
/*
bool MainWindow::3DView_Sketch_SetToolMode ( SketchTool_mode li_mode){
    m_view3d->mode_passer
}
*/


void MainWindow::ListeIcones (){
    // --- Code de diagnostic des ressources ---
    qDebug() << "=== LISTE DES RESSOURCES EMBARQUÉES ===";
    // Le préfixe ":" indique à Qt de fouiller dans l'arbre des ressources virtuelles
    QDirIterator it(":", QDirIterator::Subdirectories);
    bool fileFound = false;

    while (it.hasNext()) {
        QString resPath = it.next();
        qDebug() << "Ressource trouvée :" << resPath;
/*
        // Test spécifique pour votre icône de rectangle
        if (resPath.contains("sketch_rectangle_center.svg")) {
            fileFound = true;
            // Vérification que le fichier n'est pas vide
            QFileInfo info(resPath);
            qDebug() << "   -> [OK] Taille du fichier :" << info.size() << "octets";
        }
        */
    }
    if (!fileFound) {
        qDebug() << "   -> [ERREUR] 'sketch_rectangle_center.svg' est INTROUVABLE dans les ressources !";
    }
    qDebug() << "=======================================";
}


void MainWindow::SetAffichage_Esquisse(){
    int index = ribbon_findTabByName("TabEsquisse");
    if (index != -1) {
        m_ribbonTabWidget->setTabVisible(index, true);
        m_ribbonTabWidget->setCurrentIndex(index);
    }
}
void MainWindow::SetAffichage_Part(){
    int index = ribbon_findTabByName("TabEsquisse");
    if (index != -1) {
        m_ribbonTabWidget->setTabVisible(index, false);
    }
    m_ribbonTabWidget->setCurrentIndex(1);
}

int MainWindow::ribbon_findTabByName(const QString& objectName)
{
    for (int i = 0; i < m_ribbonTabWidget->count(); ++i) {
        // On récupère le widget interne de l'onglet à l'index i
        QWidget* tabWidget = m_ribbonTabWidget->widget(i);
        if (tabWidget && tabWidget->objectName() == objectName) {
            return i; // Trouvé !
        }
    }
    return -1; // Non trouvé
}


MainWindow::~MainWindow()
{
}



