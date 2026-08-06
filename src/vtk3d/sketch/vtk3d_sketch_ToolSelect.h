#pragma once



#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>
#include <variant>
#include <qevent.h>
#include <QMouseEvent>

#include <vtkAssemblyPath.h>
#include <vtkAssemblyNode.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkProp.h>
#include <vtkPolyDataMapper.h>
#include <vtkProp3DCollection.h>
#include <vtkPropPicker.h>
#include <vtkLine.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleImage.h>
#include <vtkDataSetMapper.h>
#include <vtkDataObject.h>
#include <vtkCellPicker.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkCamera.h>
#include <vtkSphereSource.h>
#include <vtkCallbackCommand.h>
#include "cad_events.h"

#include "CAD_PartOp.h"

#include "vtk3d_sketch_ToolSelect__SetDimension.h"
using Tool_Select_Subtool = std::variant<ToolSelect_SetDimension>;

class Vtk3d_Sketch;

struct Tool_Select  {

    Tool_Select(Vtk3d_Sketch* parent)  {
        m_Parent = parent;
    }

    void activate();
    void desactivate();

    bool keyPressEvent(QKeyEvent* event) ;
    bool gererWheelEvent(QWheelEvent* event) ;
    bool gererMouseMove(QMouseEvent* event) ;
    bool gererMouseRelease(QMouseEvent* event) ;
    bool gererMousePress(QMouseEvent* event) ;
    bool gererkeyPressEvent(QKeyEvent* event) ;

    bool    PrimitiveIsSelected = false;
    int     SelectedPrimitiveId = 0;

    bool    m_b_MouseLIsPressed = false;

    QPoint     m_MouseclickStartPosition;

    void ajusterEchelleElements( double li_echelle);

    void CADEvent_TraiterCommande(const CadCommandEvent& event);

    Vtk3d_Sketch*   m_Parent = nullptr;

    Tool_Select_Subtool     m_subtool;
    void subtool_Changesubtool(){
        std::visit([this](auto& subtool) {
            using T = std::decay_t < decltype (subtool ) > ;
            if ( std::is_same_v<T,ToolSelect_SetDimension> ){
                subtool.init(this);
            }
        }, m_subtool);
    }
    void  subtool_ExecClic(){
        std::visit([this](auto& subtool) {
            using T = std::decay_t < decltype (subtool ) > ;
            if ( std::is_same_v<T,ToolSelect_SetDimension> ){
                //return subtool.GetPopupDef();
            }
        }, m_subtool);
    }
    DialogSketchHelper::Helper& subtool_GetPopupDef()
    {
        return std::visit([](auto& subtool) -> DialogSketchHelper::Helper& {
            using T = std::decay_t<decltype(subtool)>;

            if constexpr (std::is_same_v<T, ToolSelect_SetDimension>) {
                return subtool.GetPopupDef();
            }
            // Si tu ajoutes d'autres sous-outils plus tard :
            // else if constexpr (std::is_same_v<T, AutreSousOutil>) {
            //     return subtool.GetPopupDef();
            // }
            else {
                // Fallback de sécurité (ex: si le variant contient std::monostate par défaut)
                static DialogSketchHelper::Helper dummyHelper;
                return dummyHelper;
            }
        }, m_subtool);
    }



    enum class DragMode {
        None,
        PointUnique,
        LigneComplete,
        CercleCentre,
        CercleRayon
    };

    struct {
        bool m_isDragging = false;
        DragMode m_mode = DragMode::None;
        gp_Pnt2d m_lastMousePos2D;

        SketchPoint* PtrSelectedPoint = nullptr;


        int m_activePrimitiveId = -1; // Remplace l'ancien edgeId pour être générique (Ligne ou Cercle)

        struct{
            int  IndexX;
            int  IndexY;
        }PointDrag;


        struct{
            gp_Pnt2d mouse_when_clicked_2d;
            struct{
                gp_Vec2d    start;
                gp_Vec2d    stop;
            }line;
            struct{
                gp_Vec2d    center;
            }circle;
        }PrimToMoseVects;
    } DynamicDrag;

};























