#pragma once

#include "Vtk3d_abstractviewmode.h"
#include "vtk3d_sketch_SnapperManager.h"
#include "CAD_PartOp.h"
#include "vtk3d_sketch_Tools.h"

#include <QObject>
#include <vtkSmartPointer.h>
#include <vtkType.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <gp_Ax3.hxx>
#include "QWheelEvent"
#include "vtk3d_sketch_Context.h"
#include "vtk3d_Sketch_Render_Cotations.h"
#include "2DSolver_Mapper.h"

#include <variant>

class vtk3d_MainView;
class QMouseEvent;






// ─────────────────────────────────────────────────────────────────────
// MODE ESQUISSE 2D
// ─────────────────────────────────────────────────────────────────────
class Vtk3d_Sketch : public AbstractViewMode {

    friend class Tool_Select;
    friend class Tool_LineDraw;
    friend struct Tool_CircleDraw;
    friend struct Tool_RectEdgesDraw;
    friend struct Tool_RectCenterDraw;

protected :
    sketch_Context m_ContexteEdition;


private:

    std::unique_ptr<SketchSnapperManager> m_SnapperManager;
    vtkSmartPointer<vtkActor> m_constraintsDisplayActor;
    //vtkSmartPointer<vtkProp3D> m_constraintsDisplayActor;


    //------------ mode de fonctionnement --------------------
    SketchTool_mode m_mode = SketchTool_mode::Tool_Select;
    Tooltype    m_tool;


    void rafraichirGeometrie(SketchParams* sketchParams);
    void rafraichirPoignees(SketchParams* sketchParams);
    void rafraichirContraintesGeometriques(SketchParams* sketchParams);
    vtkSmartPointer<vtkPolyData> creerSymboleLigne(bool horizontal);
    vtkSmartPointer<vtkPolyData> creerSymbolePerpendiculaire() ;


public:

    struct{
    private:
        gp_Ax3          m_sketchPlane;  // Contient l'origine, l'axe X, Y et Z du plan
        CadPartOp*      m_Operation;
        uint64_t        m_sketchId;
    public:
        void            SetSketchPlane (gp_Ax3 sketchPlane) { m_sketchPlane = sketchPlane; }
        void SetOperation (CadPartOp* li_op) { m_Operation = li_op; if (m_Operation) { m_sketchId = m_Operation->id;} else { m_sketchId = 0;}  }
        gp_Ax3&         GetSketchPlane_Mutable () { return m_sketchPlane; }
        const gp_Ax3&   GetSketchPlane () const { return m_sketchPlane; }
        CadPartOp*      GetOperation () const { return m_Operation; }
        uint64_t        GetSketchId() const {return m_sketchId;}
        auto*           GetParams () { return std::get_if<SketchParams>(&GetOperation()->getParamsMutable()); }
    }PartRefs;


    vtkSmartPointer<vtkActor>       m_ActorSketchDisplay = nullptr;
    vtkSmartPointer<vtkActor>       m_ActorSquareOfPrim = nullptr;    // pour les carrés des lignes

    vtk3d_Sketch_Render_Cotations*       m_Cotation;
    vtk3d_Sketch_Render_Cotations*       m_Cotation2;

    SketchSnapperManager* getSnapperManager() const {
        return m_SnapperManager.get();
    }

    struct {
        bool m_isPanning = false;
        QPoint m_lastPanPos;
    }MousePan;


    void rafraichirAffichageEsquisse();
    void rafraichirAffichageEsquisseInteractif();

    //Vtk3d_Sketch(vtk3d_MainView* view, uint64_t sketchId);
    Vtk3d_Sketch(vtk3d_MainView* view, CadPartOp* li_ptr_Operation);

    ~Vtk3d_Sketch();

    void activer() override;
    void desactiver () override;

    bool gererMousePress(QMouseEvent* event) override;
    bool gererMouseMove(QMouseEvent* event) override;
    bool gererMouseRelease(QMouseEvent* event) override;
    bool gererWheelEvent(QWheelEvent* event) override;

    void sketch_ActivateTool ( SketchTool_mode li_tool );

    void keyPressEvent(QKeyEvent* event) override;

    //bool calculerIntersectionSourisSurPlan(int mouseX, int mouseY, double point3DOut[3]);
    //bool calculerIntersectionSourisSurPlan(int mouseX, int mouseY, gp_Pnt &point3DOut );
    bool calculerIntersectionSourisSurPlan(int mouseX, int mouseY, gp_Pnt2d &point2DOut, gp_Pnt &point3DOut);
    gp_Pnt convertir2DEn3D(const gp_Pnt2d& point2D);

    double Echelle_calculerFacteurEchelle(vtkCamera* camera, double& derniereEchelle, double sensibilite);
    void Echelle_ajusterEchelleActor(vtkActor* actor, double facteurEchelle);
    void Echelle_ajusterEchelleElements (vtkCamera* camera);
    double m_derniereEchelle;

    void SolveEsquisse ();
    SolverInteractiveSession        m_SolverSession;


    void CADEvent_TraiterCommande(const CadCommandEvent& event) override ;
    void CADEvent_RemonterEvent(const CadResponseEvent& event);    // Méthode centrale de remontée
    inline SketchTool_mode CadEventSketchMode_To_SketchToolMode (CadEvent::Sketch::CadEvent_SketchToolMode eventMode);
    inline CadEvent::Sketch::CadEvent_SketchToolMode SketchToolMode_To_CadEventSketchMode(SketchTool_mode internalMode);
    void Signaler_ChangementEsquisseIHM ();
    void Signaler_ActivationModeEsquisse();
    void Signaler_ActivationModePart ();
    void Signaler_Selection ( std::string li_string );

    //- restreindre la copie et le déplacement
    Vtk3d_Sketch(const Vtk3d_Sketch&) = delete;
    Vtk3d_Sketch& operator=(const Vtk3d_Sketch&) = delete;
    Vtk3d_Sketch(Vtk3d_Sketch&&) = delete;
    Vtk3d_Sketch& operator=(Vtk3d_Sketch&&) = delete;

};








