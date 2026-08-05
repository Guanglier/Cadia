
#include "mainwindow.h"
#include <QDockWidget>
#include <QMdiSubWindow>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QLabel>
#include <QStatusBar>



void MainWindow::createMenus()
{
    // On récupère la barre de menu supérieure par défaut de la QMainWindow
    QMenuBar* barreMenu = menuBar();

    // Création du menu Fichier et assignation des actions
    m_fileMenu = barreMenu->addMenu(tr("&Fichier"));
    m_fileMenu->addAction(m_newAction);
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_saveAction);
    m_fileMenu->addSeparator(); // Petite ligne de séparation visuelle
    m_fileMenu->addAction(m_exitAction);

    // Création du menu Édition (souvent là qu'on place les Préférences/Options sous Windows)
    m_editMenu = barreMenu->addMenu(tr("&Édition"));
    m_editMenu->addAction(m_optionsAction);

    //--- partie menu de TEST ----------------
    m_TestMenu = barreMenu->addMenu(tr("&Test"));
    m_TestMenu->addAction(m_TestAction_CreeForme);
    //m_editMenu->addAction(m_TestAction_ModifieForme);
    m_TestMenu->addAction(m_TestAction_Dump_CAD_PartToConsole);
    m_TestMenu->addAction(m_TestAction_Dump_CAD_PartToFiles);
    m_TestMenu->addAction(m_TestAction_Dump_VTK);
    m_TestMenu->addAction(m_TestAction_ModeEsquisse);
    m_TestMenu->addAction(m_TestAction_Mode3D);
    m_TestMenu->addAction(m_TestAction_ComputeTopo);


    m_TestMenu->addSeparator();
    m_TestMenu->addMenu(Menus.SketchHelper.m_testSketchHelperMenu);

}


void MainWindow::createActions()
{
    // --- ACTIONS DU MENU FICHIER ---

    m_newAction = new QAction(tr("&Nouveau"), this);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip(tr("Créer un nouveau Part CAO"));
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onNewPart);

    m_openAction = new QAction(tr("&Ouvrir..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Ouvrir un fichier existant"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenPart);

    m_saveAction = new QAction(tr("&Enregistrer"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("Enregistrer le Part actuel"));
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSavePart);

    m_exitAction = new QAction(tr("&Quitter"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Quitter l'application"));
    connect(m_exitAction, &QAction::triggered, qApp, &QApplication::quit);




    //-------------- partie de tests -----------------
    m_TestAction_CreeForme = new QAction(tr("Ajouter forme-> tst sketch"), this);
    m_TestAction_CreeForme->setShortcut(QKeySequence::Save);
    m_TestAction_CreeForme->setStatusTip(tr("Ajouter une forme pour le test"));
    connect(m_TestAction_CreeForme, &QAction::triggered, this, &MainWindow::on_test_CreeForme);

    m_TestAction_ModifieForme = new QAction(tr("Modifier forme"), this);
    m_TestAction_ModifieForme->setShortcut(QKeySequence::Save);
    m_TestAction_ModifieForme->setStatusTip(tr("Modifier une forme pour le test"));
    connect(m_TestAction_ModifieForme, &QAction::triggered, this, &MainWindow::on_test_ModifieForme);

    m_TestAction_Dump_VTK = new QAction(tr("Dump VTK -> console"), this);
    m_TestAction_Dump_VTK->setShortcut(QKeySequence::Save);
    m_TestAction_Dump_VTK->setStatusTip(tr("dump de la structure VTK"));
    connect(m_TestAction_Dump_VTK, &QAction::triggered, this, &MainWindow::on_test_DumpVTK_ToConsole);

    m_TestAction_Dump_CAD_PartToConsole = new QAction(tr("Dump CAD_Part -> console"), this);
    m_TestAction_Dump_CAD_PartToConsole->setShortcut(QKeySequence::Save);
    m_TestAction_Dump_CAD_PartToConsole->setStatusTip(tr("dump de la structure CAD_Part"));
    connect(m_TestAction_Dump_CAD_PartToConsole, &QAction::triggered, this, &MainWindow::on_test_DumpCAD_PartToConsole);

    m_TestAction_Dump_CAD_PartToFiles = new QAction(tr("Dump CAD_Part -> fichiers"), this);
    m_TestAction_Dump_CAD_PartToFiles->setShortcut(QKeySequence::Save);
    m_TestAction_Dump_CAD_PartToFiles->setStatusTip(tr("dump de la structure CAD_Part dans des fichiers"));
    connect(m_TestAction_Dump_CAD_PartToFiles, &QAction::triggered, this, &MainWindow::on_test_DumpCAD_PartToFiles);

    m_TestAction_ModeEsquisse = new QAction(tr("Mode esquisse"), this);
    m_TestAction_ModeEsquisse->setShortcut(QKeySequence::Save);
    m_TestAction_ModeEsquisse->setStatusTip(tr("Mode esquisse"));
    connect(m_TestAction_ModeEsquisse, &QAction::triggered, this, &MainWindow::on_test_ModeEsquisse);

    m_TestAction_Mode3D = new QAction(tr("Mode 3D"), this);
    m_TestAction_Mode3D->setShortcut(QKeySequence::Save);
    m_TestAction_Mode3D->setStatusTip(tr("Mode 3D"));
    connect(m_TestAction_Mode3D, &QAction::triggered, this, &MainWindow::on_test_Mode3D);

    m_TestAction_ComputeTopo = new QAction(tr("Compute TOPO"), this);
    m_TestAction_ComputeTopo->setShortcut(QKeySequence::Save);
    m_TestAction_ComputeTopo->setStatusTip(tr("Compute TOPO"));
    connect(m_TestAction_ComputeTopo, &QAction::triggered, this, &MainWindow::on_test_ComputeTopo);


    // --- ACTION OPTIONS ---

    m_optionsAction = new QAction(tr("&Options..."), this);
    m_optionsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma)); // Raccourci standard Ctrl + ,
    m_optionsAction->setStatusTip(tr("Ouvrir les paramètres de l'application"));
    connect(m_optionsAction, &QAction::triggered, this, &MainWindow::onOpenOptions);
}




void MainWindow::CreateMenu_SketchHelper (){


    // 1. Création des actions du sous-menu
    Menus.SketchHelper.m_testSketchCreateAction = new QAction(tr("create"), this);
    Menus.SketchHelper.m_testSketchCreateAction->setStatusTip(tr("Créer et initialiser le sketch helper"));
    connect(Menus.SketchHelper.m_testSketchCreateAction, &QAction::triggered, this, &MainWindow::on_test_SketchHelperCreate); // Remplace par ta fonction slot

    Menus.SketchHelper.m_testSketchUpdateAction = new QAction(tr("update value"), this);
    Menus.SketchHelper.m_testSketchUpdateAction->setStatusTip(tr("Mettre à jour les valeurs du sketch helper"));
    connect(Menus.SketchHelper.m_testSketchUpdateAction, &QAction::triggered, this, &MainWindow::on_test_SketchHelperUpdate); // Remplace par ta fonction slot

    // 2. Création du menu parent (le sous-menu)
    Menus.SketchHelper.m_testSketchHelperMenu = new QMenu(tr("Test sketch helper"), this);

    // 3. Ajout des actions dans ce sous-menu
    Menus.SketchHelper.m_testSketchHelperMenu->addAction(Menus.SketchHelper.m_testSketchCreateAction);
    Menus.SketchHelper.m_testSketchHelperMenu->addAction(Menus.SketchHelper.m_testSketchUpdateAction);
}



