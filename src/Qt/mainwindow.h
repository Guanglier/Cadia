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
#include "CAD_Part.h"
//#include "vtk3d_sketch_Tools.h"
#include "CadResponseCustomEvent.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    CAD_Part    doc;


private slots:
    void onModelChanged();


    void onNewPart();
    void onOpenPart();
    void onSavePart();
    void onOpenOptions();
    //void onTestPart();
    //void onTest2Part();
    void on_test_DumpVTK_ToConsole ();
    void on_test_DumpCAD_PartToConsole ();
    void on_test_DumpCAD_PartToFiles ();
    void on_test_CreeForme();
    void on_test_ModifieForme();
    void on_test_ModeEsquisse ();
    void on_test_Mode3D ();
    void on_test_ComputeTopo ();
    void ListeIcones ();

    void onTreeViewOperation_clicked(const QModelIndex& index);
    void onTreeShowContextMenu(const QPoint& pos);

    void TreeView_Cfg_SketchView ();
    void TreeView_Cfg_PartView ();
    void TreeView_Cfg_AllView ();


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
    QAction* m_TestAction_Dump_CAD_PartToConsole;
    QAction* m_TestAction_Dump_CAD_PartToFiles;
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
            QAction* actSelect = nullptr;
            QAction* actLine = nullptr;
            QAction* actCircle = nullptr;
            QAction* actRectCenter = nullptr;
            QAction* actRectCorners = nullptr;
            void configure_all(){
                if( nullptr!= actSelect){ actSelect->setCheckable(true);}
                if( nullptr!= actLine){ actLine->setCheckable(true);}
                if( nullptr!= actCircle){actCircle->setCheckable(true);}
                if( nullptr!= actRectCenter){actRectCenter->setCheckable(true);}
                if( nullptr!= actRectCorners){actRectCorners->setCheckable(true);}
            }
            void uncheck_all(){
                if( nullptr!= actSelect){ actSelect->setChecked(false);}
                if( nullptr!= actLine){ actLine->setChecked(false);}
                if( nullptr!= actCircle){actCircle->setChecked(false);}
                if( nullptr!= actRectCenter){actRectCenter->setChecked(false);}
                if( nullptr!= actRectCorners){actRectCorners->setChecked(false);}
            }
        }Tool;
        struct{
            QAction* actConstHorizontal = nullptr;
            QAction* actConstVertical = nullptr;
            QAction* actConstDistance = nullptr;
            QAction* actConstPerpendicular = nullptr;
            QAction* actConstParallel = nullptr;
            QAction* actConstResolve = nullptr;
            void configure_all(){
                if( nullptr!= actConstHorizontal   ){ actConstHorizontal->setCheckable(true);}
                if( nullptr!= actConstVertical     ){ actConstVertical->setCheckable(true);}
                if( nullptr!= actConstDistance     ){ actConstDistance->setCheckable(true);}
                if( nullptr!= actConstPerpendicular){ actConstPerpendicular->setCheckable(true);}
                if( nullptr!= actConstParallel     ){ actConstParallel->setCheckable(true);}
                if( nullptr!= actConstResolve      ){ actConstResolve->setCheckable(false);}
            }
            void uncheck_all(){
                if( nullptr!= actConstHorizontal   ){ actConstHorizontal->setChecked(false);}
                if( nullptr!= actConstVertical     ){ actConstVertical->setChecked(false);}
                if( nullptr!= actConstDistance     ){ actConstDistance->setChecked(false);}
                if( nullptr!= actConstPerpendicular){ actConstPerpendicular->setChecked(false);}
                if( nullptr!= actConstParallel     ){ actConstParallel->setChecked(false);}
                if( nullptr!= actConstResolve      ){ actConstResolve->setChecked(false);}
            }
        }Constraints;
    }Sketch;


    void traiterReponseCad(const CadResponseEvent& resp);

protected:
    void customEvent(QEvent* event) override;

};











#endif // MAINWINDOW_H
