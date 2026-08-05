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
#include "Logger.h"

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
    CreateMenu_SketchHelper ();
    createMenus();

    // 2. Création de l'arborescence
    QDockWidget* dock = new QDockWidget("Arbre du modèle", this);
    m_treeView = new QTreeView(dock);
    m_treeView->setHeaderHidden(false);
    m_treeView->setIconSize(QSize(20, 20));
    dock->setWidget(m_treeView);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    // 3. Création de la vue 3D (Le showMaximized profitera direct des 1280x720)
    // et définition de la call back qui va appeler le post event, pour remonter
    // l'évènement de manière différée
    m_view3d = new vtk3d_MainView(this);
    m_view3d->CADEvent_RemonterEvent_SetCallback([this](const CadResponseEvent& resp) {
        QCoreApplication::postEvent(this, new CadResponseCustomEvent(resp));
    });


    QMdiSubWindow* subWin = m_mdiArea->addSubWindow(m_view3d);
    subWin->setWindowTitle("Vue 3D - Pièce Prototype");
    subWin->resize(800, 600);
    subWin->showMaximized();

    createRibbon();


    //createToolBars_NewOpenSave();
    //creerToolbarVisibilite ();
    //createToolBars_vues ();

    // 4. Remplissage et calcul du Part
    doc.tst_add_repere_origine();
    doc.tst_add_op_sketch_rect();
    doc.tst_add_op_extrude();
    //doc.tst_add_op_sketch_circle();
    //doc.tst_add_op_extrude_2();
    doc.compute_final_topo();
    //doc.tst_dump_all_op_to_file();
    //doc.tst_dump_tree_to_file("__tree.txt");

    // 5. Synchronisation de la géométrie
    m_view3d->synchroniserPart(1, doc);

    // 6. UI Tree View
    setupTreeView();
    m_cadTreeModel->refreshFromPart(doc);
    m_treeView->expandAll();

    SetAffichage_Part ();

    //ListeIcones ();
}


void MainWindow::customEvent(QEvent* event) {
    if (event->type() == CadResponseCustomEvent::EventType) {
        auto* cadEvent = static_cast<CadResponseCustomEvent*>(event);
        traiterReponseCad(cadEvent->getResponse()); // Appel de ta fonction de traitement
    } else {
        QMainWindow::customEvent(event); // Toujours propager pour les autres événements natifs
    }
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
    TabLayout_BtnRapides = new QHBoxLayout(esquisseTabContent);
    TabLayout_BtnRapides->setContentsMargins(0, 0, 0, 0);
    TabLayout_BtnRapides->setSpacing(0);
    TabLayout_BtnRapides->setAlignment(Qt::AlignLeft);

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
    TabLayout_BtnRapides->addWidget(groupSortie);

    // -------------------------------------------------------------------------
    // 📦 GROUPE B : PRIMITIVES
    // -------------------------------------------------------------------------
    ToolbarPrimitives_Setup ();


    // -------------------------------------------------------------------------
    // 📦 GROUPE C : Contraintes
    // -------------------------------------------------------------------------
    ToolbarConstraints_Setup ();


    // Ajout de l'onglet complet au composant
    m_ribbonTabWidget->addTab(esquisseTabContent, tr("Esquisse"));

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



