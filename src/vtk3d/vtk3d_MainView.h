// vtk3d_MainView.h
#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkAssembly.h>
#include <TopoDS_Shape.hxx>
#include "CAD_PartOp.h"
#include <set>
#include <variant>
#include <type_traits>
#include "CAD_Part.h"
#include "vtk3d_HighLighter.h"
#include <vtkOrientationMarkerWidget.h>
#include <QVTKOpenGLNativeWidget.h>
#include <memory>
#include "cad_events.h"


// Prédéclaration pour éviter les inclusions cycliques
class AbstractViewMode;




enum class SelectionType {
    None,
    Face,
    Edge,
    Vertex,
    Axis,       // Axe du repère d'origine
    OriginPoint, // Point central de l'origine
    Sketch
};

// Une structure propre qui contient TOUTES les infos d'un clic réussi
struct SelectionResult {
    SelectionType type = SelectionType::None;
    uint64_t operationId = 0; // 999999 (Solide), 888888 (Esquisse)...
    int internalVtkId = -1;   // Le cellId ou faceIndex extrait
};




class vtk3d_MainView : public QVTKOpenGLNativeWidget {
    Q_OBJECT

    friend class Vtk3d_Part;
    friend class Vtk3d_Sketch;


public:

    enum class LayerCategory{
        OriginRef,
        Solid,
        Sketch
    };

    explicit vtk3d_MainView(QWidget* parent = nullptr);

    void setViewFront();
    void setViewSide();
    void setViewIsometric();

    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void keyPressEvent(QKeyEvent* event);

    //void CameraFitAll ();


    void synchroniserPart(uint64_t PartId,  CAD_Part& part);
    SelectionResult analyserClic(vtkActor* pickedActor, vtkIdType cellId);
    void mettreEnSurbrillanceFaceParId(vtkPolyData* sourcePolyData, int faceId);
    //void masquerSurbrillance ();
    //void setCategoryVisibility(SelectionType type, bool visible);

    void setCategoryVisibility(SelectionType type, bool visible);   // Nouvelle méthode pour piloter la visibilité par catégorie

    std::string ptr_to_string (const vtkSmartPointer<vtkAssembly>  *li_ptr);

    vtkRenderer* getRenderer() const { return m_renderer.Get(); }


    enum class SketchTool {
        None,   // Mode 3D standard (rotation, zoom, pan)
        Line,   // Dessin de lignes
        Circle  // Dessin de cercles
    };
    // Définition de la structure pour un composant/pièce de l'assemblage
    struct PieceRenderNode {
        uint64_t pieceId = 0;   //duplication de l'index de la structure dans la liste m_piecesNodes, permet de s'y retrouver au picker
        vtkSmartPointer<vtkAssembly> rootAssembly = nullptr;        // Le conteneur racine VTK pour cette pièce

        // Les calques (layers) graphiques dédiés
        vtkSmartPointer<vtkActor> originActor = nullptr;
        vtkSmartPointer<vtkActor> solidActor = nullptr;      // Le volume 3D (Faces)
        vtkSmartPointer<vtkActor> edgeActor = nullptr;       // Les arêtes (Wires/Edges)
        vtkSmartPointer<vtkAssembly> sketchesAssembly = nullptr; // Les esquisses 2D rattachées

        // Ajoute ici tes futurs besoins (dimensionsAssembly, etc.) sans casser le reste
    };

    std::string ptr_to_string(vtkProp* li_ptr, const PieceRenderNode& node);
    void diag_dumpArchitecture(std::ostream& out);
    void diag_dumpProp3D(vtkProp3D* prop, std::ostream& out, const PieceRenderNode& node, int depth = 0);

    void mode_passerModeEsquisse (uint64_t id);
    void mode_passerMode3D ();
    void distribuerCommande(const CadCommandEvent& event);

    std::unique_ptr<vtk3d_HighLighter> m_Chighlighter;

    CAD_Part* GetCurrentPart () { return m_currentPart; }

    void CADEvent_TraiterCommande(const CadCommandEvent& event);
    void CADEvent_RemonterEvent(const CadResponseEvent& event);
    void CADEvent_RemonterEvent_SetCallback(CadResponseCallback cb);

private:
    vtkSmartPointer<vtkRenderer> m_renderer;
    std::unique_ptr<AbstractViewMode> m_modeActif = nullptr;
    std::map<uint64_t, vtkSmartPointer<vtkActor>> m_solidesActors;      // 2. LES ACTEURS POUR LA 3D (Permanents)

    // 3. LES ACTEURS POUR L'ESQUISSE (Permanents aussi !)
    vtkSmartPointer<vtkActor> m_grilleActor;         // La grille de fond
    vtkSmartPointer<vtkActor> m_ligneElastiqueActor; // La ligne qui suit la souris

    // Les lignes et cercles de l'esquisse en cours
    std::map<uint64_t, vtkSmartPointer<vtkActor>> m_sketchGeometryActors;



    vtkSmartPointer<vtkOrientationMarkerWidget> m_axesWidget;
    void showEvent (QShowEvent* event);

    void ajusterEchelleRepere();


    std::map<uint64_t, PieceRenderNode> m_piecesNodes;  // map de pièces qui contiendra les acteurs pour la pièce
    CAD_Part* m_currentPart = nullptr; // Pointeur vers le part actif

    // Tes fonctions d'extraction actuelles encapsulées proprement
    void    updateSolideActor(vtkActor* actor, const TopoDS_Shape& shape, const float color[3]);
    void    updateWireframeActor(vtkActor* actor, const TopoDS_Shape& shape, const float color[3]);
    void    updateRepereDorigineActor(vtkActor* actor, const TopoDS_Shape& shape, const float color[3], float opacity);

    vtkSmartPointer<vtkAssembly>    buildSketchAssembly(const CadPartOp& op);     // Pour gérer l'affichage d'une sketch complète (profils + axes) via un Assembly VTK


    CadResponseCallback m_onResponseCallback;


    void animateCameraTo(double targetPos[3], double targetUp[3]);

    SketchTool m_currentTool = SketchTool::None; // Outil actif par défaut
    bool       m_isDrawing = false;              // Est-on au milieu d'un tracé ?
    QPoint     m_MouseclickStartPosition;
    double      m_dernierParallelScale = -1.0;
    int         m_CurrentPartId;       //le dernier part ID affiché

};


