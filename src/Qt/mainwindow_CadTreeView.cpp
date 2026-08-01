#include "mainwindow.h"




void MainWindow::TreeView_Cfg_SketchView (){
    m_cadTreeModel->m_DisplaySketchDetails = true;
    m_cadTreeModel->m_DisplaySolids = false;

    m_cadTreeModel->refreshFromPart(doc);

    // On déplie les niveaux 1 et 2
    for (int i = 0; i < m_cadTreeModel->rowCount(); ++i) {
        QModelIndex opIndex = m_cadTreeModel->index(i, 0);
        m_treeView->setExpanded(opIndex, true); // Niveau 1 (les opérations)

        // Niveau 2 (les sous-dossiers Primitives, Contraintes, ou paramètres d'extrusion)
        for (int j = 0; j < m_cadTreeModel->rowCount(opIndex); ++j) {
            QModelIndex childIndex = m_cadTreeModel->index(j, 0, opIndex);
            m_treeView->setExpanded(childIndex, true);
        }
    }
}
void MainWindow::TreeView_Cfg_PartView (){
    m_cadTreeModel->m_DisplaySketchDetails = false;
    m_cadTreeModel->m_DisplaySolids = true;
    m_cadTreeModel->refreshFromPart(doc);
    m_treeView->expandAll();
}
void MainWindow::TreeView_Cfg_AllView (){
    m_treeView->expandAll();
}











