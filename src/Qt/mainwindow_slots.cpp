
#include "mainwindow.h"
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include "extrusiondialog.h"
#include "CadTreeModel.h"
#include <QMenu>
#include <QAction>
#include "Dialog_SketchHelper_Popup.h"
#include "Dialog_SketchHelper.h"


void MainWindow::onNewPart()
{
    // Ici vous brancherez votre logique de création (ex: QMdiSubWindow avec votre vtk3d_MainView)
    statusBar()->showMessage(tr("Nouveau Part créé."), 2000);
}

void MainWindow::onOpenPart()
{

    statusBar()->showMessage(tr("Ouverture d'un Part..."), 2000);
}

void MainWindow::onSavePart()
{

    statusBar()->showMessage(tr("Part enregistré."), 2000);
}


void MainWindow::onOpenOptions()
{
    // Pour l'instant, on affiche une boîte de dialogue pour valider que le menu fonctionne.
    // Plus tard, vous ouvrirez ici votre propre QDialog (ex: OptionsDialog)
    QMessageBox::information(this, tr("Options"), tr("Ici s'ouvriront les paramètres du logiciel (Unités, Grille, Rendu, Raccourcis)."));
}


void MainWindow::onModelChanged() {
    //m_view3d->updateShape(m_model->getFinalShape());
    //m_treeView->expandAll();
}



//========================================================================
//      Partie TESTS
//========================================================================
void MainWindow::on_test_CreeForme(){
    statusBar()->showMessage(tr("m_TestAction_CreeForme."), 2000);
    m_TestAction_CreeForme->setEnabled(false);

    if ( false == test2_done ){
        //test2_done = true;
        //doc.tst_add_empty_sketch();

        doc.tst_add_op_step_2();
        doc.compute_final_topo();
        //CadPartOp* opDansDoc = doc.trouverOperationMutable(6);
        m_view3d->synchroniserPart(1, doc);
        //if (opDansDoc){
        //    std::cout << "opDansDoc update  " << std::endl;
            m_cadTreeModel->refreshFromPart(doc);
            m_treeView->expandAll();
        //}

    }

}
void MainWindow::on_test_ComputeTopo (){
    doc.compute_final_topo();
    m_view3d->synchroniserPart(1, doc);
    m_view3d->getRenderer()->Render();
}
void MainWindow::on_test_ModeEsquisse (){
    m_view3d->mode_passerModeEsquisse(1);
}
void MainWindow::on_test_Mode3D (){
    m_view3d->mode_passerMode3D();
    SetAffichage_Part ();
}
void MainWindow::on_test_ModifieForme(){
    bool ok = false;

    std::cout << "TEST 2  " << std::endl;

}

void MainWindow::on_test_DumpVTK_ToConsole (){
    m_view3d->diag_dumpArchitecture( std::cout );
    std::cout  << std::endl;
}
void MainWindow::on_test_DumpCAD_PartToConsole (){
    doc.tst_dump_tree_to_console();
}
void MainWindow::on_test_DumpCAD_PartToFiles (){
    doc.tst_dump_tree_to_file("__tree_dump.txt");
    doc.tst_dump_all_op_to_file ();
}


//========================================================================
//      TEST sketch helper
//========================================================================
void MainWindow::on_test_SketchHelperCreate()
{
    // Si la fenêtre n'existe pas encore, on l'instancie
    if (!m_sketchHelperPopup) {
        m_sketchHelperPopup = new Dialog_SketchHelper_Popup(this);

        // Optionnel : Gérer la fermeture pour nettoyer le pointeur proprement
        connect(m_sketchHelperPopup, &QDialog::finished, this, [this]() {
            m_sketchHelperPopup->deleteLater();
            m_sketchHelperPopup = nullptr;
        });

        // Connexion des signaux de la popup vers les slots de ta MainWindow (si besoin)
        connect(m_sketchHelperPopup, &Dialog_SketchHelper_Popup::doubleValueChanged,
                this, [](const QString& id, double val) {
                    qDebug() << "Valeur modifiée pour :" << id << "=" << val;
                });
    }

    // 1. On prépare la structure de données initiale (Helper)
    m_TestUpdatedHelper.title = "Assistant d'Esquisse : Ligne";
    m_TestUpdatedHelper.instructionText = "Veuillez entrer la longueur et sélectionner le point cible.";
    m_TestUpdatedHelper.isSelectionComplete = false;
    m_TestUpdatedHelper.showButtonCancel = true;
    m_TestUpdatedHelper.showButtonReset = false; // Masque le bouton Réinitialiser par exemple
    m_TestUpdatedHelper.showButtonOk = false;
    m_TestUpdatedHelper.isButtonOkEnabled = false;

    // Ajout d'un champ double (ex: distance)
    DialogSketchHelper::ChampInputDouble champDist;
    champDist.id = "val_distance";
    champDist.title = "Longueur :";
    champDist.value = 50.0;
    champDist.b_IsFocus = true;      // Met le focus dessus
    champDist.b_IsDisabled = false;  // Actif
    champDist.b_IsValid = false;      // Valide au départ
    m_TestUpdatedHelper.champMultiple.push_back(champDist);

    // Ajout d'un champ de sélection (ex: point cible)
    DialogSketchHelper::ChampInputSelection champSel;
    champSel.id = "sel_point";
    champSel.title = "Point d'arrivée :";
    champSel.IsOk = false;          // Pas encore sélectionné
    champSel.b_IsFocus = false;
    champSel.b_IsDisabled = true;
    champSel.b_IsValid = false;     // Invalide -> Affichera la bordure/croix rouge
    m_TestUpdatedHelper.champMultiple.push_back(champSel);

    // 2. On envoie les données à la popup
    m_sketchHelperPopup->setHelperData(m_TestUpdatedHelper);

    // 3. On l'affiche (en mode non-bloquant show() pour pouvoir tester le bouton "update")
    m_sketchHelperPopup->show();
    m_sketchHelperPopup->raise();
    m_sketchHelperPopup->activateWindow();
}

void MainWindow::on_test_SketchHelperUpdate_First()
{
    if (!m_sketchHelperPopup || !m_sketchHelperPopup->isVisible()) {
        qDebug() << "La popup du Sketch Helper n'est pas ouverte ! Cliquez d'abord sur 'create'.";
        return;
    }

    // On prépare une structure modifiée (par exemple, l'utilisateur a entré une valeur,
    // et la sélection a maintenant réussi, rendant le champ valide)
    //DialogSketchHelper::Helper updatedHelper;
    m_TestUpdatedHelper.title = "Assistant d'Esquisse : Ligne (Mis à jour)";
    m_TestUpdatedHelper.instructionText = "Sélection réussie ! Vous pouvez valider.";
    m_TestUpdatedHelper.isSelectionComplete = true;
    m_TestUpdatedHelper.showButtonOk = true;
    m_TestUpdatedHelper.showButtonReset = true;
    m_TestUpdatedHelper.showButtonCancel = true;

    // Champ double mis à jour (ex: valeur changée à 75.5)
    DialogSketchHelper::ChampInputDouble champDist;
    champDist.id = "val_distance";
    champDist.title = "Longueur :";
    champDist.value = 75.5;
    champDist.b_IsFocus = false;
    champDist.b_IsDisabled = false;
    champDist.b_IsValid = true;
    m_TestUpdatedHelper.champMultiple.push_back(champDist);

    // Champ de sélection mis à jour (IsOk = true, b_IsValid = true -> passe en vert/normal)
    DialogSketchHelper::ChampInputSelection champSel;
    champSel.id = "sel_point";
    champSel.title = "Point d'arrivée :";
    champSel.IsOk = true;           // Maintenant sélectionné !
    champSel.b_IsFocus = false;
    champSel.b_IsDisabled = false;
    champSel.b_IsValid = true;      // Devient valide (disparition de la croix rouge)
    m_TestUpdatedHelper.champMultiple.push_back(champSel);

    // On envoie la nouvelle structure : la popup fera son "diffing" intelligemment
    m_sketchHelperPopup->setHelperData(m_TestUpdatedHelper);

    //m_sketchHelperPopup->updateUI ( m_TestUpdatedHelper);
}

void MainWindow::on_test_SketchHelperUpdate_Second(){

}


//========================================================================
//      Partie TREEVIEW
//========================================================================


void MainWindow::onTreeViewOperation_clicked(const QModelIndex& index){
    if (!index.isValid()) return;

    std::cout<< "onTreeViewOperation_clicked -> ";

    // 1. Récupérer le nom ou le type de l'opération stockée dans le modèle
    QString operationType = index.data(Qt::DisplayRole).toString();

    QVariant idVariant = index.data(CadTreeModel::IdRole);
    if (idVariant.isValid()) {
        uint64_t opId = idVariant.toULongLong();
        std::cout << "Id=" << opId << " ";

        CadPartOp*  ptr_op = doc.trouverOperationMutable (opId);
        if (nullptr == ptr_op){
            std::cout << " -> ERR nullptr ptr_op = doc.trouverOperationMutable (opId) " << std::endl;
            return;
        }

        // récupérer une référence pour pouvoir la modifier
        OperationParams& ptr_op_param = ptr_op->getParamsMutable();

        if (auto* extrudeParams = std::get_if<ExtrudeParams>(&ptr_op_param)) {


            //--- listing des sketch dans le Part pour les proposer à la sélection
            const IdRegistry<CadPartOp>& operation_list = doc.getOperationRegistry();
            std::vector<CadPartOp> op_list = operation_list.getItems();
            std::vector<ExtrusionDialog_SketchRef> SketchRefList;
            for (size_t i = 0; i < op_list.size(); ++i) {
                const CadPartOp& op_i = op_list[i];
                const OperationParams& params = op_i.getParams();

                // On vérifie si l'opération courante encapsule un SketchParams, si oui on l'enregistre dans la liste.
                if (const auto* sketch_params = std::get_if<SketchParams>(&params)) {
                    ExtrusionDialog_SketchRef   tmp_ref;
                    tmp_ref.id = static_cast<int>(i);
                    tmp_ref.name = op_i.getName();
                    //std::cout<<" ---> enregistre SketchId " << tmp_ref.id << " name=" << tmp_ref.name << std::endl;
                    SketchRefList.push_back ( tmp_ref );
                }
            }

            //---- reprise des paramètres utilisés de l'opération
            ExtrusionDialog_Params  param_extrusion;
            ExtrusionDialog_SketchRef param_sketchref;
            param_extrusion.currentStart = extrudeParams->start;
            param_extrusion.currentStop  = extrudeParams->end;
            param_extrusion.currentOp = extrudeParams->EboolOp;
            param_extrusion.ListSketchRef = SketchRefList;
            param_extrusion.ChoosedSketchRef.id = extrudeParams->SketchId;
            std::cout<<" param_extrusion.ChoosedSketchRef.id  ---> SketchId " << param_extrusion.ChoosedSketchRef.id  << std::endl;
            //---- lancement de la fenetre d'édition
            ExtrusionDialog dialog(param_extrusion, this);


            // Si l'utilisateur clique sur "OK"
            if (dialog.exec() == QDialog::Accepted) {
                ExtrusionDialog_Params  param_extrusionEntered = dialog.getEntedredParams();

                double newStart = param_extrusionEntered.currentStart;
                double newEnd = param_extrusionEntered.currentStop;
                EBooleanOp bool_op = param_extrusionEntered.currentOp;
                extrudeParams->end = newEnd;
                extrudeParams->start = newStart;
                extrudeParams->EboolOp = bool_op;
                extrudeParams->SketchId = param_extrusionEntered.ChoosedSketchRef.id;
                std::cout << " -> Chgmt valeurs : " << newStart << " -> " << newEnd << "  sketch id " << extrudeParams->SketchId << " recalcul..." << std::endl;

                doc.revaluerOperation( *ptr_op );
                doc.compute_final_topo();
                m_view3d->synchroniserPart(1, doc);
            }

        }else if (auto* extrudeParams = std::get_if<SketchParams>(&ptr_op_param)) {
            //CadPartOp*  ptr_op = doc.trouverOperationMutable (opId);
            m_view3d->mode_passerModeEsquisse(opId);
            SetAffichage_Esquisse ();
            std::cout << " -> SketchParams ! ";

        }else if(auto* extrudeParams = std::get_if<CoordinateSystem>(&ptr_op_param)) {
            std::cout << " -> CoordinateSystem ! ";
        }else if(auto* extrudeParams = std::get_if<BooleanParams>(&ptr_op_param)) {
            std::cout << " -> BooleanParams ! ";
        }else{
            std::cout << " -> INCONNU ! ! ";
        }

    }

    std::cout << std::endl;


}

void MainWindow::onTreeShowContextMenu(const QPoint& pos) {
    // Récupère l'index sous la souris
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) return; // Clic dans le vide de l'arbre, on fait rien

    // Extraction de l'ID stocké via ton IdRole
    int opId = index.data(CadTreeModel::IdRole).toInt();
    CadPartOp*  ptr_op = doc.trouverOperationMutable (opId);
    if (nullptr == ptr_op){
        std::cout << " -> ERR nullptr ptr_op = doc.trouverOperationMutable (opId) " << std::endl;
    }
    OperationParams& ptr_op_param = ptr_op->getParamsMutable();
    if (auto* extrudeParams = std::get_if<ExtrudeParams>(&ptr_op_param)) {
        // Création du menu
        QMenu contextMenu(tr("Options"), m_treeView);
        QAction* editAction = contextMenu.addAction("Modifier l'opération...");
        QAction* deleteAction = contextMenu.addAction("Supprimer");

        // Affichage au pixel près sous le curseur
        QAction* selectedAction = contextMenu.exec(m_treeView->viewport()->mapToGlobal(pos));

        // Traitement de l'action
        if (selectedAction == editAction) {
            // C'est ici que tu appelleras ta logique pour récupérer l'opération
            // avec 'opId' et ouvrir ton ExtrusionDialog !
            //this->openEditDialog(opId);
        }
        else if (selectedAction == deleteAction) {
            // doc.removeOperation(opId);
        }
    }else if (auto* extrudeParams = std::get_if<SketchParams>(&ptr_op_param)) {
        std::cout << " -> SketchParams ! ";
    }else if(auto* extrudeParams = std::get_if<CoordinateSystem>(&ptr_op_param)) {
        std::cout << " -> CoordinateSystem ! ";
    }else if(auto* extrudeParams = std::get_if<BooleanParams>(&ptr_op_param)) {
        std::cout << " -> BooleanParams ! ";
    }else{
        std::cout << " -> INCONNU ! ! ";
    }

}







