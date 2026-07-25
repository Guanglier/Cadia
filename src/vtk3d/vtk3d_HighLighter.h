

#pragma once
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkPolyData.h>


/**
 * =================================================================================
 * DESCRIPTION DU MÉCANISME DE SÉLECTION ET MISE À L'ÉCHELLE DYNAMIQUE DU REPÈRE
 * =================================================================================
 * * 1. PRINCIPE DE LA TAILLE CONSTANTE (vtk3d_MainView)
 * ---------------------------------------------------------------------------------
 * La vue CAO utilise une projection parallèle (orthographique). Lors d'un zoom,
 * la caméra ne change pas de position dans l'espace, seul son 'ParallelScale' varie.
 * * Pour maintenir les plans du repère d'origine (Calque 777777) à une taille visuelle
 * constante à l'écran (environ 25% de la hauteur de la vue), la fonction
 * vtk3d_MainView::ajusterEchelleRepere() est appelée via un callback VTK branché sur les
 * mouvements de la caméra.
 * * Performance (Throttling par seuil) :
 * Pour éviter des appels en boucle à Render() qui feraient ramer la souris, la fonction
 * compare le zoom actuel avec le dernier mémorisé. La matrice de l'acteur n'est
 * mise à jour que si le niveau de zoom a changé de plus de 15%.
 * * 2. ARCHITECTURE DE SÉLECTION ET ACTEUR DÉDIÉ (vtk3d_HighLighter)
 * ---------------------------------------------------------------------------------
 * Pour éviter tout conflit visuel ou géométrique avec les sélections de solides 3D
 * classiques (qui doivent rester à l'échelle 1.0), la sélection du repère possède
 * son propre acteur dédié : `m_highlightAxisActor`.
 * * Lors d'un clic (mouseReleaseEvent) détecté comme "SelectionType::Axis" :
 * a) Le Picker extrait l'ID de l'élément cliqué sur le repère.
 * b) On calcule le facteur d'échelle actuel de la scène.
 * c) On appelle `mettreEnSurbrillanceAxeParId(..., facteurEchelle)` qui isole
 * la géométrie sélectionnée et l'applique à `m_highlightAxisActor`.
 * * 3. LE PONT DE SYNCHRONISATION EN COURS DE ZOOM
 * ---------------------------------------------------------------------------------
 * Si l'utilisateur manipule la molette de la souris *pendant* qu'un plan du repère
 * est sélectionné (orange), la taille de l'acteur principal (`axesActor`) change.
 * Si l'acteur de surbrillance restait figé, on observerait un décalage visuel (Z-fighting
 * ou surbrillance minuscule restée au centre).
 * * Le pont est assuré par : `m_Chighlighter->set_axisActorScale(facteurEchelle);`
 * Appelé directement à l'intérieur de `ajusterEchelleRepere()`, ce mécanisme pousse
 * instantanément la nouvelle matrice d'échelle vers `m_highlightAxisActor`.
 * * Résultat : Le repère et sa surbrillance grandissent et rétrécissent en parfaite
 * synchronisation visuelle, sans jamais altérer le comportement des autres acteurs
 * de sélection de l'application (solides, arêtes).
 * Attention une vérification est faite par rapport au changement de zoom pour éviter
 * les boucles de mise a jour infinies qui ralentiraient le soft
 *
 * * 4. MÉCANISME DE DÉSELECTION / NETTOYAGE
 * ---------------------------------------------------------------------------------
 * Lors d'un clic dans le vide, `masquerSurbrillance()` passe l'acteur à `VisibilityOff()`.
 * Grâce à l'indépendance de la variable d'échelle mise à jour en tâche de fond par la vue,
 * l'acteur est automatiquement réaligné sur la bonne matrice dès le prochain affichage.
 */



class vtk3d_HighLighter {


public:
	

    explicit vtk3d_HighLighter(vtkRenderer* renderer);		// Le constructeur reçoit le renderer de la vue pour pouvoir lui injecter ses acteurs
	~vtk3d_HighLighter() = default;


    void mettreEnSurbrillanceFaceParId(vtkPolyData* sourcePolyData, int faceId);
    void mettreEnSurbrillanceEdgeParId(vtkPolyData* sourcePolyData, int edgeId);
    void mettreEnSurbrillanceAxeParId(vtkPolyData* sourcePolyData, int axeId, double scale = 1.0);
    void masquerSurbrillance();
    void set_axisActorScale ( double li_scale){m_highlightAxisActor->SetScale(li_scale, li_scale, li_scale);}
	
private:
    vtkRenderer* m_renderer = nullptr; // Pointeur vers le renderer principal

    // Les acteurs de surbrillance internes à la classe
    vtkSmartPointer<vtkActor> m_highlightFaceActor;
    vtkSmartPointer<vtkActor> m_highlightEdgeActor;
    vtkSmartPointer<vtkActor> m_highlightAxisActor;
    //vtkSmartPointer<vtkActor> m_highlightAxesActor;
    void extraireEtAfficherSelection(vtkPolyData* sourcePolyData,
                                     int id,
                                     const std::string& arrayName,
                                     vtkActor* targetActor,
                                     const std::string& debugLabel,
                                     double scale = 1.0);

};














