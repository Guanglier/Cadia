#pragma once

#include <vtkSmartPointer.h>
#include <vector>
#include <gp_Vec.hxx>

class Vtk3d_Sketch;
class vtkActor;
class vtkPoints;
class vtkCellArray;
class vtkFloatArray;
class vtkPolyData;

class SketchConstraintManager {
public:
    explicit SketchConstraintManager(Vtk3d_Sketch* parent);
    ~SketchConstraintManager();

    // Initialisation globale (création des acteurs VTK de feedback)
    void init();
    
    // Nettoyage complet (retrait des acteurs du renderer)
    void cleanUp();

    // 🚀 La fonction maîtresse : applique tout le pipeline de contraintes d'un coup
    void appliqueContraintes2D(
        gp_Pnt2d& li_P1_2D, gp_Pnt& li_P1_3D,
        gp_Pnt2d& li_P2_2D, gp_Pnt& li_P2_3D,
        bool activeAideHV, bool enCoursDeDessin);

    // Fonctions d'aimantation unitaires (accessibles individuellement si besoin)
    bool snapToExistingPoints(gp_Pnt& pointCible, double li_SeuilCoincidence_mm = 0.5);
    bool snapToExistingPoints(gp_Pnt2d& plio_Ptr2D, gp_Pnt& plio_Ptr3D, double li_SeuilCoincidence_mm = 0.5) ;
    bool alignWithExistingPoints(gp_Pnt2d& lio_Point2D, gp_Pnt& lio_Point3D);
    void aideHV(const gp_Pnt& p1, gp_Pnt& p2);
    void aideHV(const gp_Pnt2d& p1, gp_Pnt2d& p2);

    // Utilitaires graphiques
    void ajusterEchelleCarreSnap();
    void masquerFeedback();

    void snapPointsVisited_Clean ();
    void snapPointsVisited_AddPoint (gp_Pnt2d& vect);
    bool snapPointsVisited_IsPointInTheList (const gp_Pnt2d& vect);



private:
    void initSnapPointActor();
    void initSnapLineActor();
    void updateSnapLineActor(bool li_AlignX, bool li_AlignY,
                             const gp_Pnt& pointCibleVert3D,
                             const gp_Pnt& pointCibleHor3D,
                             const gp_Pnt& mousePoint3D);




    Vtk3d_Sketch* m_parent = nullptr;
    double m_derniereEchelleCarre = 0.0;

    std::vector<gp_Pnt2d> m_SnapPointsVisitedList;


    // Feedback Point (Carré vert)
    vtkSmartPointer<vtkActor> m_ActorSnapPoint;

    // Feedback Lignes de guidage (Pointillés avec correction GPU 2D intégrée)
    vtkSmartPointer<vtkActor> m_ActorSnapLine;
    vtkSmartPointer<vtkPoints> m_snapLinePoints;
    vtkSmartPointer<vtkCellArray> m_snapLineCellsArray;
    vtkSmartPointer<vtkFloatArray> m_snapLineTCoords;
    vtkSmartPointer<vtkPolyData> m_snapLinePolyData;
};











