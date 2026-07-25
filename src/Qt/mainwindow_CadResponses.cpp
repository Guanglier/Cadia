
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
        case CadEvent::Sketch::CadEvent_SketchToolMode::SetConstraints:
            Sketch.Constraints.actConstHorizontal->setChecked(true);
            break;
        default:
            this->statusBar()->showMessage( "err CadEvent::Sketch::RespChangedTool" );
            break;
        }
    }

    if (auto* status = std::get_if<CadEvent::Sketch::RespGeneralSignal>(&resp.params)) {
        switch ( status->message ){

        case CadEvent::Sketch::CadEvent_SketchGeneralMessage::SketchChanged:
            m_cadTreeModel->m_DisplaySolids = false;
            m_cadTreeModel->m_DisplaySketchDetails = true;
            TreeView_Cfg_SketchView ();
            break;
        case CadEvent::Sketch::CadEvent_SketchGeneralMessage::SketchActivated:
            m_cadTreeModel->m_DisplaySolids = false;
            m_cadTreeModel->m_DisplaySketchDetails = true;
            TreeView_Cfg_SketchView ();
            break;
        default:
            LOG_ERROR << "ERROR default getif CadEvent::Sketch::RespGeneralSignal " << std::endl;
            break;
        }
    }

    if ( auto* status=std::get_if<CadEvent::Part::RespGeneralSignal>(&resp.params)) {
        switch ( status->message ){
            case CadEvent::Part::CadEvent_PartGeneralMessage::Activated:
                TreeView_Cfg_PartView();
                break;
            default:
                LOG_ERROR << "if ( auto* status=std::get_if<CadEvent::Solid::RespGeneralSignal>(&resp.params)) "<< std::endl;
                break;
            }
    }
}









