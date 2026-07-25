
#include "mainwindow.h"
#include <QLabel>



void MainWindow::createToolBars_NewOpenSave()
{
    // On crée la barre d'outils et on la fixe en haut
    m_mainToolBar = addToolBar(tr("Fichier"));
    m_mainToolBar->setMovable(false); // Optionnel : empêche l'utilisateur de la détacher

    // On y glisse les mêmes actions que le menu (les boutons vont se créer tout seuls)
    m_mainToolBar->addAction(m_newAction);
    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addAction(m_saveAction);
    //m_mainToolBar->addAction(m_TestAction_CreeForme);
    //m_mainToolBar->addAction(m_Test2Action);

}


void MainWindow::createToolBars_vues()
{
    // Exemple de création des boutons dans ta Toolbar
    QToolBar* toolBar = addToolBar("Vues");

    QAction* actFront = toolBar->addAction("Vue de Face");
    QAction* actSide = toolBar->addAction("Vue de Côté");
    QAction* actIso = toolBar->addAction("Vue Isométrique");

    // Connexion directe aux méthodes du widget VTK (nommé ici m_cadView)
    connect(actFront, &QAction::triggered, m_view3d, &vtk3d_MainView::setViewFront);
    connect(actSide,  &QAction::triggered, m_view3d, &vtk3d_MainView::setViewSide);
    connect(actIso,   &QAction::triggered, m_view3d, &vtk3d_MainView::setViewIsometric);


    // Exemple de création des boutons dans ta Toolbar
    QToolBar* toolBarActions = addToolBar("Actions");
    QAction* actNewSketch = toolBarActions->addAction(QIcon(":/icons/sketch_new_1.svg"),"Sketch");
    QAction* actNewExtrude = toolBarActions->addAction(QIcon(":/icons/extrude_new_1.svg"),"Extrusion");
    toolBarActions->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);


}


void MainWindow::creerToolbarVisibilite() {
    QToolBar* visToolbar = addToolBar(tr("Visibilité"));

    // ─── AJOUT DU TITRE DE LA TOOLBAR ──────────────────────────────────
    QLabel* titreLabel = new QLabel(tr("Visibilité : "), this);
    // Optionnel : un petit style pour le mettre en valeur (gras, petite marge)
    titreLabel->setStyleSheet("font-weight: bold; margin-left: 5px; margin-right: 5px;");
    visToolbar->addWidget(titreLabel);
    // ───────────────────────────────────────────────────────────────────

    // 1. Bouton pour le Repère d'Origine
    QAction* actRepere = visToolbar->addAction(QIcon(":/icons/origin.png"), tr("Repère"));
    actRepere->setCheckable(true);
    actRepere->setChecked(true);

    connect(actRepere, &QAction::toggled, m_view3d, [this](bool checked) {
        this->m_view3d->setCategoryVisibility(SelectionType::Axis, checked);
    });

    // 2. Bouton pour les Solides 3D
    QAction* actSolides = visToolbar->addAction(QIcon(":/icons/solids.png"), tr("Solides"));
    actSolides->setCheckable(true);
    actSolides->setChecked(true);
    connect(actSolides, &QAction::toggled, this, [this](bool checked) {
        this->m_view3d->setCategoryVisibility(SelectionType::Face, checked);
    });

    // 3. Bouton pour les Esquisses (2D)
    QAction* actSketches = visToolbar->addAction(QIcon(":/icons/sketches.png"), tr("Esquisses"));
    actSketches->setCheckable(true);
    actSketches->setChecked(true);
    connect(actSketches, &QAction::toggled, this, [this](bool checked) {
        this->m_view3d->setCategoryVisibility(SelectionType::Sketch, checked);
    });
}




