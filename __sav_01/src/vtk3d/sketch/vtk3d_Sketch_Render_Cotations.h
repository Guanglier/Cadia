#pragma once


#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkTextActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vector>
#include <gp_Pnt2d.hxx>
#include <gp_Pnt.hxx>
#include "DimensionEngine.h"


class vtkRenderer;

enum class CotationType{
    Cotation_Line_PointToPoint,
    Cotation_Line_Horizontal,
    Cotation_Line_Vertical
};





class vtk3d_Sketch_Render_Cotations {
public:
    vtk3d_Sketch_Render_Cotations(vtkRenderer* renderer);
    ~vtk3d_Sketch_Render_Cotations();


    //void DessinerCotationDepuisResultat( const gp_Ax3& sketchPlane, const DimensionEngine::GeometryResult& geo);

    // --- 1. COTE DYNAMIQUE (SURVOL / APERÇU EN DIRECT) ---
    // Appelée par Tool_LineDraw, Tool_Dimensions, etc. dans leur MouseMove
    void MettreAJourCoteTemporaire(const gp_Pnt& start,
                                   const gp_Pnt& stop,
                                   const gp_Pnt& mousePos,
                                   bool isHorizontalOrVertical,
                                   bool isMousePerpendi);

    // Masque instantanément la cote de survol
    //void EffacerCoteTemporaire();

    // --- 3. CONTRÔLE GLOBAL ---
    //void SetVisible(bool visible);
    //void ClearAll();

    void masquerEtVider ();
    void Afficher();

    struct RefLine_type{
        bool ref_defined;
        gp_Pnt PntStart;
        gp_Pnt PntStop;
        gp_Vec VecPerpToRefLine;
        double DecalageCotationToLine;
        CotationType    Type;
    };

    void DessinerCotationLigne(const gp_Ax3& sketchPlane, RefLine_type& RefStruct);
    void DessinerCotationDepuisResultat(const gp_Ax3& sketchPlane, const DimensionEngine::GeometryResult& geo) ;
    void PrepareStructCotation(const gp_Ax3& sketchPlane, gp_Pnt& Ptn3D_Mouse, RefLine_type& RefStruct);

private:


    void MettreAJourTexte3D(const gp_Ax3& sketchPlane, const gp_Pnt& start3D, const gp_Pnt& stop3D );
    void DessineFleche3D(
        vtkSmartPointer<vtkCellArray> lines,
        vtkSmartPointer<vtkPoints> points,
        const gp_Ax3& sketchPlane,
        const gp_Pnt& pntExtremite,    // Pnt3DStart ou Pnt3DStop de la ligne de cote
        const gp_Vec& dirLigneCote,    // Vecteur directeur normalisé de la ligne de cote (de A vers B)
        double longueur,               // ex: 4.0 mm
        double angleDeg,               // ex: 15.0 degrés
        bool bInverser);                // true pour la flèche de départ, false pour celle de fin

    bool IsMousePerpendi (const gp_Ax3& sketchPlane, gp_Pnt& currentPoint3D, RefLine_type& RefStruct);

    void AssurerActeurAjoute(vtkActor* propActor);
    void DrawPrepairedCotation ( vtkCellArray* lines, vtkPoints* points);



    // --- PIPELINE VTK POUR LA COTE TEMPORAIRE ---
    vtkRenderer*              m_renderer = nullptr;
    vtkNew<vtkActor>          m_tempActor;
    vtkNew<vtkTextActor>      m_tempTextActor;
    vtkNew<vtkPolyData>       m_tempPolyData;
    vtkNew<vtkPolyDataMapper> m_tempMapper;
    vtkNew<vtkPoints>         m_tempPoints;
    vtkNew<vtkCellArray>      m_tempLines;
    //bool                      m_bTempTopologyInitialized = false;

    // --- PIPELINE VTK POUR LES COTES PERMANENTES ---
    vtkNew<vtkActor>          m_permActor;
    vtkNew<vtkPolyData>       m_permPolyData;
    vtkNew<vtkPolyDataMapper> m_permMapper;
    vtkNew<vtkPoints>         m_permPoints;
    vtkNew<vtkCellArray>      m_permLines;
    std::vector<vtkSmartPointer<vtkTextActor>> m_permTextActors;
};





