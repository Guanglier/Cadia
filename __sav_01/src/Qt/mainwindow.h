#ifndef MAINWINDOW_H
#define MAINWINDOW_H


// mainwindow.h
#pragma once
#include <QMainWindow>
#include <QMdiArea>
#include <QTreeView>
#include "vtk3d_MainView.h"
#include <QToolBar>
#include <QAction>

#include "CadTreeModel.h"
#include "CAD_Document.h"
//#include "vtk3d_sketch_Tools.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    CAD_Document    doc;


private slots:
    void onModelChanged();


    void onNewDocument();
    void onOpenDocument();
    void onSaveDocument();
    void onOpenOptions();
    //void onTestDocument();
    //void onTest2Document();
    void on_test_DumpVTK_ToConsole ();
    void on_test_DumpCAD_DocumentToConsole ();
    void on_test_DumpCAD_DocumentToFiles ();
    void on_test_CreeForme();
    void on_test_ModifieForme();
    void on_test_ModeEsquisse ();
    void on_test_Mode3D ();
    void on_test_ComputeTopo ();
    void ListeIcones ();

    void onTreeViewOperation_clicked(const QModelIndex& index);
    void onTreeShowContextMenu(const QPoint& pos);



private:
    QMdiArea* m_mdiArea;
    QTreeView* m_treeView;
    vtk3d_MainView* m_view3d;

    //--- ribbon
    QTabWidget* m_ribbonTabWidget; // Le conteneur du Ruban
    void createRibbon();           // La méthode de construction du Ruban
    int ribbon_findTabByName(const QString& objectName);

    void SetAffichage_Esquisse();
    void SetAffichage_Part();

    bool test2_done = false;

    void createActions();
    void createMenus();
    void createToolBars_NewOpenSave();
    void creerToolbarVisibilite();
    void createToolBars_vues ();

    // Les Menus principaux
    QMenu* m_fileMenu;
    QMenu* m_editMenu; // Utile pour y glisser les options en bas

    // La Barre d'outils principale
    QToolBar* m_mainToolBar;

    // Les Actions du menu Fichier
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_exitAction;
    QAction* m_TestAction_CreeForme;
    QAction* m_TestAction_ModifieForme;
    QAction* m_TestAction_Dump_CAD_DocumentToConsole;
    QAction* m_TestAction_Dump_CAD_DocumentToFiles;
    QAction* m_TestAction_Dump_VTK;
    QAction* m_TestAction_ModeEsquisse;
    QAction* m_TestAction_Mode3D;
    QAction* m_TestAction_ComputeTopo;

    // L'Action pour les Options
    QAction* m_optionsAction;

    void setupTreeView ();
    CadTreeModel *m_cadTreeModel;

    //bool 3DView_Sketch_SetToolMode ( SketchTool_mode li_mode);

    struct{
        struct{
            QAction* actSelect;
            QAction* actLine;
            QAction* actCircle;
            QAction* actRectCenter;
            QAction* actRectCorners ;
        }Tool;
        struct{
            QAction* actConstHorizontal;
            QAction* actConstVertical;
            QAction* actConstDistance;
            QAction* actConstPerpendicular;
            QAction* actConstParallel;
            QAction* actConstResolve;
        }Constraints;
    }Sketch;

    /*
    struct  ToolSketchToolsActionType{
        QAction* actSelect;
        QAction* actLine;
        QAction* actCircle;
        QAction* actRectCenter;
        QAction* actRectCorners ;
    };
    ToolSketchToolsActionType   m_SketchToolActions;
    */
};











#endif // MAINWINDOW_H
