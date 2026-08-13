
#include <QMenuBar>
#include <QStatusBar>
#include "mainwindow.h"
#include "Logger.h"


void MainWindow::traiterReponseCad(const CadResponseEvent& resp) {
    if (auto* status = std::get_if<CadEvent::Sketch::RespStatus>(&resp.params)) {
        this->statusBar()->showMessage(QString::fromStdString(status->text));
    }
    if (auto* status = std::get_if<CadEvent::Sketch::RespChangedTool>(&resp.params)) {
        Sketch.Tool.uncheck_all();
        Sketch.Constraints.uncheck_all();

        switch(status->toolMode)
        {
            case CadEvent::Sketch::ToolMode::Draw_line:
                Sketch.Tool.actLine->setChecked(true);
                break;
            case CadEvent::Sketch::ToolMode::Draw_Circle:
                Sketch.Tool.actCircle->setChecked(true);
                break;
            case CadEvent::Sketch::ToolMode::Draw_Rectangle:
                if (auto* substatus = std::get_if<CadEvent::Sketch::RectangleSubMode>(&status->sub_mode)) {
                    switch ( *substatus ){
                        case CadEvent::Sketch::RectangleSubMode::ByCenter:
                            Sketch.Tool.actRectCenter->setChecked(true);
                            break;
                        case CadEvent::Sketch::RectangleSubMode::ByEdges:
                            Sketch.Tool.actRectCorners->setChecked(true);
                            break;
                        default:
                            LOG_ERROR << "MainWindow::traiterReponseCad -> CadEvent::Sketch::ToolMode::Draw_Rectangle -> switch -> substatus -> default "<< std::endl;
                            break;
                    }
                }else{
                    LOG_ERROR << "MainWindow::traiterReponseCad -> CadEvent::Sketch::ToolMode::Draw_Rectangle -> switch default "<< std::endl;
                }
                break;

            case CadEvent::Sketch::ToolMode::Select:
                Sketch.Tool.actSelect->setChecked(true);
                break;
            case CadEvent::Sketch::ToolMode::SetConstraints:
                if ( auto* substatus = std::get_if<CadEvent::Sketch::ConstraintSubMode>(&status->sub_mode)){
                    switch ( *substatus){
                        case CadEvent::Sketch::ConstraintSubMode::Horizontal:
                            Sketch.Constraints.actConstHorizontal->setChecked(true);
                            break;
                        case CadEvent::Sketch::ConstraintSubMode::Vertical:
                            Sketch.Constraints.actConstVertical->setChecked(true);
                            break;
                        case CadEvent::Sketch::ConstraintSubMode::Parallel:
                            Sketch.Constraints.actConstParallel->setChecked(true);
                            break;
                        case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
                            Sketch.Constraints.actConstPerpendicular->setChecked(true);
                            break;
                        case CadEvent::Sketch::ConstraintSubMode::Distance:
                            Sketch.Constraints.actConstDistance->setChecked(true);
                            break;
                    };
                }else{
                    LOG_ERROR << "MainWindow::traiterReponseCad -> CadEvent::Sketch::ToolMode::SetConstraints -> switch default "<< std::endl;
                }

                break;
            default:
                this->statusBar()->showMessage( "err CadEvent::Sketch::RespChangedTool" );
                break;
        }
    }

    if (auto* status = std::get_if<CadEvent::Sketch::RespGeneralSignal>(&resp.params)) {
        switch ( status->message ){

        case CadEvent::Sketch::GeneralMessage::SketchChanged:
            m_cadTreeModel->m_DisplaySolids = false;
            m_cadTreeModel->m_DisplaySketchDetails = true;
            TreeView_Cfg_SketchView ();
            break;
        case CadEvent::Sketch::GeneralMessage::SketchActivated:
            m_cadTreeModel->m_DisplaySolids = false;
            m_cadTreeModel->m_DisplaySketchDetails = true;
            TreeView_Cfg_SketchView ();
            break;
        default:
            LOG_ERROR << "ERROR default getif CadEvent::Sketch::RespGeneralSignal " << std::endl;
            break;
        }
    }
    if (auto* status = std::get_if<CadEvent::Sketch::RespSelection>(&resp.params)) {
        if (m_activeConstraintPopup) {
            m_activeConstraintPopup->setSelectionStatus( QString::fromStdString(status->text)  );
        }
    }

    if (auto* status = std::get_if<CadEvent::Sketch::RespSendPopupDef>(&resp.params)) {

        DialogSketchHelper::Helper popup_def = status->popup_def;


        // 3. On l'affiche (en mode non-bloquant show() pour pouvoir tester le bouton "update")
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
            connect(m_sketchHelperPopup, &Dialog_SketchHelper_Popup::OnClickedButton,
                    this, [this](const int li_button)
                    {
                        std::cout << "Valeur modifiée pour :" << li_button << std::endl;
                        CadCommandEvent evt;

                        switch ( li_button){
                            case 0:
                                evt.params = CadEvent::Sketch::CmdPopupTool{  CadEvent::Sketch::CmdPopupToolBtnClicked{CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Cancel} };
                                break;
                            case 1:
                                evt.params = CadEvent::Sketch::CmdPopupTool{  CadEvent::Sketch::CmdPopupToolBtnClicked{CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Reset} };
                                break;
                            case 2:
                                evt.params = CadEvent::Sketch::CmdPopupTool{  CadEvent::Sketch::CmdPopupToolBtnClicked{CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_OK} };
                                break;
                            default:
                                evt.params = CadEvent::Sketch::CmdPopupTool{  CadEvent::Sketch::CmdPopupToolBtnClicked{CadEvent::Sketch::CmdPopupToolBtnClicked::Btn_Cancel} };
                                break;
                        }
                        this->m_view3d->CADEvent_TraiterCommande(evt);
                    });
        }
        m_sketchHelperPopup->setHelperData(popup_def);

        m_sketchHelperPopup->show();
        m_sketchHelperPopup->raise();
        m_sketchHelperPopup->activateWindow();
    }

    if ( auto* status=std::get_if<CadEvent::Part::RespGeneralSignal>(&resp.params)) {
        switch ( status->message ){
            case CadEvent::Part::GeneralMessage::Activated:
                TreeView_Cfg_PartView();
                break;
            default:
                LOG_ERROR << "if ( auto* status=std::get_if<CadEvent::Solid::RespGeneralSignal>(&resp.params)) "<< std::endl;
                break;
            }
    }
}









