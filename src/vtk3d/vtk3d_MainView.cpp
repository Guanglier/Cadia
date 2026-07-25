

// vtk3d_MainView.cpp
#include "vtk3d_MainView.h"
#include "vtk3d_sketch.h"
#include "vtk3d_part.h"
#include "vtk3d_actors.h"

#include <QMouseEvent>


#include "Vtk3d_abstractviewmode.h"


#include <vtkAssemblyPath.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractSelection.h>
#include <vtkFeatureEdges.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGeometryFilter.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkLight.h>
#include <vtkNamedColors.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProp3DCollection.h>
#include <vtkProperty.h>
#include <vtkPropPicker.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTransform.h>


#include <vtkInteractorStyleImage.h>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>                  // Pour manipuler l'objet Arête

#include <Poly_Triangulation.hxx>

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>            // Pour lire la géométrie de l'arête
#include <BRepMesh_IncrementalMesh.hxx>

#include <GCPnts_TangentialDeflection.hxx>  // Algorithme OCC magique pour discrétiser proprement une courbe en points

#include <QVariantAnimation>
#include <QVTKOpenGLNativeWidget.h>
#include <QMouseEvent>
#include <QDebug>


#include "vtk3d_OccToVtkConverter.h"

//   Voici comment s'articule cette structure en 3 niveaux :
//
//   1. Le Niveau "Gestionnaire" : L'ID de l'opération (Le Calque) :
//     Chaque entité de ton arbre de construction possède un identifiant unique de calque.
//     - 999999 : Solide 3D (la pièce finale).
//     - 888888 : L'Esquisse active (les lignes et contraintes 2D).
//     - 777777 : Le repère d'origine (les plans de référence fondamentaux).
//
//     Cet ID sert de clé d'accès dans ton dictionnaire global m_operationActors[layer.id].
//     Chaque calque est matérialisé par un vtkAssembly, qui est une boîte vide (un conteneur)
//     capable de regrouper plusieurs sous-objets graphiques.
//
//   2. Le Niveau "Acteur" : La spécialisation visuelle
//     À l'intérieur de cet vtkAssembly, tu injectes des vtkActor spécialisés.
//     Un acteur ne sait pas ce qu'il dessine, il sait juste comment le dessiner :
//     - solideActor : Configuré avec un rendu de type Surface, une certaine opacité (transparence pour
//       les plans bleus) et des propriétés de réflexion de la lumière.
//     - edgeActor : Configuré avec un rendu de type Wireframe, une épaisseur de ligne (SetLineWidth) et
//       une couleur spécifique (sombre pour les arêtes du solide, vive pour l'esquisse).
//     - axesActor : gestion des systèmes d'axes, on sépare pour pouvoir l'afficher avec des spécificités
//       comme par exemple une taille constante à l'écran
//
//   3. Le Niveau "Data" : Les attributs géométriques (Le PolyData)
//     C'est le niveau le plus bas, le cœur mathématique. C'est le vtkPolyData
//     (alimenté par le moteur OpenCASCADE) qu'on donne à manger au Mapper de chaque acteur.
//     C'est à ce niveau qu'on injecte les fameux tableaux d'IDs personnalisés (CellData) :
//      - Les polygones de la surface reçoivent le tableau "OpenCascadeFaceID".
//      - Les lignes du filaire reçoivent le tableau "OpenCascadeEdgeID".
//
//
//   Grâce à ce découpage, quand l'utilisateur fait un clic à l'écran :
//      - Étape 1 (Le Calque) : Tu remontes au premier parent (vtkAssembly)
//      pour savoir dans quelle opération on a cliqué. Si c'est 777777, tu sais que c'est le repère.
//      - Étape 2 (L'Acteur) : Tu regardes quel acteur a intercepté le clic
//      pour savoir si l'utilisateur visait plutôt une surface ou un contour.
//      - Étape 3 (La Data) : Grâce au cellId, tu plonges dans le tableau (faceIdsArray ou edgeIdsArray)
//      du PolyData pour extraire la référence exacte de la face de ton modèle (Face 1, Face 2 ou Face 3).
//



//------------------------------------------------------------------------------------
//              constructeur
//------------------------------------------------------------------------------------
vtk3d_MainView::vtk3d_MainView(QWidget* parent) : QVTKOpenGLNativeWidget(parent) {
    // 1. Initialisation de la fenêtre de rendu et de son interacteur
    auto renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    setRenderWindow(renderWindow);

    vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();

    auto interactor = renderWindow->GetInteractor();
    interactor->SetRenderWindow(renderWindow);

    //----- test desactiver le zoom
    auto noOpStyle = vtkSmartPointer<vtkInteractorStyle>::New();
    interactor->SetInteractorStyle(noOpStyle);
    // fin du test


    // 2. Initialisation du Renderer et de la lumière CAO
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->RemoveAllLights(); // On nettoie les lumières d'ambiance par défaut
    //m_renderer->CreateLight();     // Force une "Headlight" (lampe frontale liée à la caméra)




    // On crée une Headlight (lampe frontale)
    auto light = vtkSmartPointer<vtkLight>::New();
    light->SetLightTypeToHeadlight();
    // C'est ici qu'on gère l'intensité globale
    light->SetIntensity(0.9);
    // On force la lumière à ne pas générer de reflets brillants
    light->SetSpecularColor(0.0, 0.0, 0.0);
    m_renderer->AddLight(light);



    // 3. Configuration du fond blanc
    vtkNew<vtkNamedColors> colors;
    m_renderer->SetBackground(colors->GetColor3d("white").GetData());


    // ne pas faire perspective (partie plus proche plus grosse
    // mais avoir projection orthogarphique
    m_renderer->GetActiveCamera()->ParallelProjectionOn();

    renderWindow->AddRenderer(m_renderer);
    renderWindow->SetMultiSamples(4);   //anti crenelage de x4

    m_Chighlighter = std::make_unique<vtk3d_HighLighter>(m_renderer);          //--- système de sélection


    //--------------- REPERE EN BAS A DROITE -----------------------------
    auto axesActor = vtkSmartPointer<vtkAxesActor>::New();
    axesActor->SetXAxisLabelText("X");
    axesActor->SetYAxisLabelText("Y");
    axesActor->SetZAxisLabelText("Z");

    // 2. Allocation du widget (si déclaré dans vtk3d_MainView.h)
    m_axesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    m_axesWidget->SetOrientationMarker(axesActor);
    m_axesWidget->SetViewport(0.8, 0.0, 1.0, 0.2);      // On définit le viewport (0.0 à 1.0) -> en bas à gauche par exemple : (0, 0, 0.2, 0.2)

    // Paramètre indispensable pour éviter le clignotement des polygones superposés (Z-fighting)
    vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
    vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(-1.0, -1.0);


    //--- taille constante pour le repère ----------------
    vtkCamera* camera = m_renderer->GetActiveCamera();
    if (camera) {
        // Création d'un callback VTK qui appelle notre fonction d'ajustement
        auto cameraCallback = vtkSmartPointer<vtkCallbackCommand>::New();
        cameraCallback->SetCallback([](vtkObject* caller, unsigned long eid, void* clientData, void* callData) {
            // Redirection vers notre fonction membre
            auto view = reinterpret_cast<vtk3d_MainView*>(clientData);
            if (view) {
                view->ajusterEchelleRepere();
            }
        });
        cameraCallback->SetClientData(this); // Passage du pointeur 'this' pour le contexte

        // On écoute le ModifiedEvent (déclenché à chaque modification de la caméra)
        camera->AddObserver(vtkCommand::ModifiedEvent, cameraCallback);
    }

    mode_passerMode3D();

}

//------------------------------------------------------------------------------------
//              mise a jour solide
// Transforme une TopoDS_Shape (Solide) en vtkActor (Faces triangulées)
// Met à jour ou crée le PolyData des FACES triangulées
//------------------------------------------------------------------------------------
void vtk3d_MainView::updateSolideActor(vtkActor* actor, const TopoDS_Shape& shape, const float color[3]) {
    if (!actor) return;

    // Étape 1 : Géométrie & Topologie (OCC -> VTK Data)
    auto polyData = Vtk3d_Converter::creerSolidePolyData(shape);

    // Étape 2 : Graphismes & Matériau (VTK Data -> Actor Pipeline)
    Vtk3d_Actors::configurerSolideActor(actor, polyData, color);
}


//------------------------------------------------------------------------------------
//              mise a jour wireframe
// Transforme les arêtes d'une forme en lignes VTK (Filaire)
//------------------------------------------------------------------------------------
void vtk3d_MainView::updateWireframeActor(vtkActor* actor, const TopoDS_Shape& shape, const float color[3]) {
    if (!actor) return;

    // 1. Étape conversion des données (OCC -> VTK)
    auto polyData = Vtk3d_Converter::creerWireframePolyData(shape);

    // 2. Étape application du style et rendu pipeline
    Vtk3d_Actors::configurerWireframeActor(actor, polyData, color);
}


void vtk3d_MainView::updateRepereDorigineActor(vtkActor* actor, const TopoDS_Shape& shape, const float color[3], float opacity) {
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto triangles = vtkSmartPointer<vtkCellArray>::New();
    auto faceIdsArray = vtkSmartPointer<vtkIntArray>::New();
    faceIdsArray->SetName("OpenCascadeFaceID");

    // On extrait les plans via ton convertisseur maillé
    Vtk3d_Converter::extraireFacesOccVersVtk(shape, points, triangles, faceIdsArray);

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetPolys(triangles);
    polyData->GetCellData()->AddArray(faceIdsArray);
    polyData->GetCellData()->SetPedigreeIds(faceIdsArray);

    vtkSmartPointer<vtkPolyDataMapper>mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (!mapper) {
        mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetRepresentationToSurface();
        //actor->GetProperty()->EdgeVisibilityOn(); // On affiche les contours directement ici !
        actor->GetProperty()->EdgeVisibilityOff();
        //actor->GetProperty()->SetEdgeColor(color[0]*0.2f, color[1]*0.2f, color[2]*0.2f);
        //actor->GetProperty()->SetLineWidth(1.0);
        actor->GetProperty()->LightingOff();
    }

    mapper->SetInputData(polyData);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetOpacity(opacity);
}


void vtk3d_MainView::ajusterEchelleRepere() {
    if (!m_currentDoc) return;

    vtkActor* axesActor = nullptr;

    // 🎯 On récupère l'acteur d'origine dédié
    for (auto& [docId, node] : m_piecesNodes) {
        if (node.originActor) {
            axesActor = node.originActor;
            break;
        }
    }

    if (!axesActor || !axesActor->GetVisibility()) return;

    // Appliquer le facteur d'échelle compensatoire par rapport à la caméra
    vtkCamera* camera = m_renderer->GetActiveCamera();
    if (!camera) return;
    double facteurEchelle = camera->GetParallelScale() * 0.05;

#define TOL_SCALE_ECHELLE   0.1
    if (m_dernierParallelScale > 0.0) {
        double ratio = facteurEchelle / m_dernierParallelScale;
        if (ratio > (1.0-TOL_SCALE_ECHELLE) && ratio < (1.0+TOL_SCALE_ECHELLE) ) {
            return;
        }
    }
    m_dernierParallelScale = facteurEchelle;

    // Réactivation de la mise à l'échelle sur l'acteur dédié !
    axesActor->SetScale(facteurEchelle, facteurEchelle, facteurEchelle);
    m_Chighlighter->set_axisActorScale(facteurEchelle);

    this->renderWindow()->Render();
}


vtkSmartPointer<vtkAssembly> vtk3d_MainView::buildSketchAssembly(const CadOperation& op) {
    auto assembly = vtkSmartPointer<vtkAssembly>::New();

    // 1. Récupération sûre des paramètres de la Sketch via std::get si besoin
    // (Utile si tu as besoin d'accéder aux primitives internes comme op.getParams() -> getPrimitives())
    const auto& sketchParams = std::get<SketchParams>(op.getParams());
    const float* color = op.getColor();

    // 2. On récupère la TopoDS_Shape locale stockée dans l'opération enveloppe
    TopoDS_Shape sketchShape = op.getLocalTopo();

    if (!sketchShape.IsNull()) {
        // On met à jour ou crée l'acteur filaire pour le profil de la Sketch
        auto mainWireActor = vtkSmartPointer<vtkActor>::New();
        updateWireframeActor(mainWireActor, sketchShape, color);
        assembly->AddPart(mainWireActor);
    }
    return assembly;
}


void vtk3d_MainView::synchroniserDocument(uint64_t documentId, CAD_Document& doc) {
    m_currentDoc = &doc;
    const auto& operations = doc.getOperationRegistry().getItems();

    m_CurrentDocumentId = documentId;

    // 🎯 1. ON RÉCUPÈRE L'UNIQUE NOEUD DE LA PIÈCE
    auto& node = m_piecesNodes[m_CurrentDocumentId];
    if (!node.rootAssembly) {
        node.pieceId = m_CurrentDocumentId;
        node.rootAssembly = vtkSmartPointer<vtkAssembly>::New();
        m_renderer->AddViewProp(node.rootAssembly);
    }

    // 2. Si le document est vide, on nettoie TOUS les acteurs internes
    if (operations.empty()) {
        if (node.originActor) node.rootAssembly->RemovePart(node.originActor);
        if (node.solidActor)  node.rootAssembly->RemovePart(node.solidActor);
        if (node.edgeActor)   node.rootAssembly->RemovePart(node.edgeActor);
        if (node.sketchesAssembly) node.rootAssembly->RemovePart(node.sketchesAssembly);
        node.originActor = nullptr;
        node.solidActor = nullptr;
        node.edgeActor = nullptr;
        node.sketchesAssembly = nullptr;
        this->renderWindow()->Render();
        return;
    }

    // 🎯 3. BALAYAGE DE L'HISTORIQUE : TROUVER LE DERNIER SOLIDE ACTIF
    uint64_t dernierVolumiqueId = 0;
    for (auto it = operations.rbegin(); it != operations.rend(); ++it) {
        const auto& op = *it;
        if (op.isVisible() && (std::holds_alternative<ExtrudeParams>(op.getParams()) ||
                               std::holds_alternative<BooleanParams>(op.getParams()))) {
            dernierVolumiqueId = op.id;
            break;
        }
    }

    // 🎯 4. GESTION DE LA COUCHE DES ESQUISSES
    if (!node.sketchesAssembly) {
        node.sketchesAssembly = vtkSmartPointer<vtkAssembly>::New();
        node.rootAssembly->AddPart(node.sketchesAssembly);
    }
    auto parts = node.sketchesAssembly->GetParts();
    if (parts) {
        parts->RemoveAllItems();
    }

    // 🎯 5. ALIMENTATION DES CALQUES DE LA PIÈCE
    for (const auto& op : operations) {
        if (!op.isVisible()) continue;

        // CAS A : Le Repère d'Origine dédié
        if (std::holds_alternative<CoordinateSystem>(op.getParams())) {
            if (!node.originActor) {
                node.originActor = vtkSmartPointer<vtkActor>::New();
                node.rootAssembly->AddPart(node.originActor);
            }
            updateRepereDorigineActor(node.originActor, op.getLocalTopo(), op.getColor(), op.getOpacity());
            node.originActor->VisibilityOn();
            node.originActor->PickableOn();
        }
        // CAS B : Les Esquisses
        else if (std::holds_alternative<SketchParams>(op.getParams())) {
            auto singleSketchAssembly = buildSketchAssembly(op);
            node.sketchesAssembly->AddPart(singleSketchAssembly);
            node.originActor->PickableOn();
        }
        // CAS C : Le Solide final (Unique)
        else if (op.id == dernierVolumiqueId) {
            if (!node.solidActor) {
                node.solidActor = vtkSmartPointer<vtkActor>::New();
                node.rootAssembly->AddPart(node.solidActor);
            }
            if (!node.edgeActor) {
                node.edgeActor = vtkSmartPointer<vtkActor>::New();
                node.rootAssembly->AddPart(node.edgeActor);
            }

            updateSolideActor(node.solidActor, op.getResultingTopo(), op.getColor());
            node.solidActor->GetProperty()->SetOpacity(op.getOpacity());
            node.solidActor->GetProperty()->LightingOff(); // Ton rendu CAO propre
            node.solidActor->VisibilityOn();
            node.solidActor->PickableOn();
            node.rootAssembly->PickableOn();

            const float* baseColor = op.getColor();
            float darkColor[3] = { baseColor[0] * 0.4f, baseColor[1] * 0.4f, baseColor[2] * 0.4f };
            updateWireframeActor(node.edgeActor, op.getResultingTopo(), darkColor);
            node.edgeActor->GetProperty()->SetLineWidth(1.5);
            node.edgeActor->VisibilityOn();
            node.edgeActor->PickableOn();
        }
    }

    // On active la pièce complète
    node.rootAssembly->VisibilityOn();
    node.rootAssembly->PickableOn();

    // Ajustement automatique de la caméra sur le volume utile
    if (dernierVolumiqueId != 0) {
        m_renderer->ResetCamera();
    }

    m_renderer->ResetCamera();

    // Ajustement de l'échelle adaptative du repère d'origine
    ajusterEchelleRepere();

    //this->getRenderer()->ResetCameraClippingRange();
    this->renderWindow()->Render();
}

/*
void vtk3d_MainView::CameraFitAll (){
    // 1. On récupère les limites 3D de tout ce qui est affiché dans le renderer
    double bounds[6];
    m_renderer->ComputeBounds();
    m_renderer->GetBounds(bounds);

    // 2. On fait le ResetCamera UNIQUEMENT sur ces frontières.
    // VTK va reculer ou avancer la caméra sur son axe à 45° pour que tout rentre dans l'écran,
    // SANS changer l'angle de visée !
    m_renderer->ResetCamera(bounds);

    // 3. On applique le style et on render
    vtkSmartPointer<vtkInteractorStyleImage> imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
    interactor->SetInteractorStyle(imageStyle);

    m_renderer->ResetCameraClippingRange();
    interactor->GetRenderWindow()->Render();
}
*/
void vtk3d_MainView::mousePressEvent(QMouseEvent* event) {
    //std::cout << "vtk3d_MainView::mousePressEvent " << std::flush;
    if ( nullptr != m_modeActif ){
        if ( true == m_modeActif->gererMousePress(event) ){
            return;
        }
    }
    QVTKOpenGLNativeWidget::mousePressEvent(event);     // On laisse toujours le comportement natif (indispensable pour la navigation VTK)
}
void vtk3d_MainView::mouseReleaseEvent(QMouseEvent* event) {
    //std::cout << "vtk3d_MainView::mouseReleaseEvent "<< std::flush;

    if ( nullptr != m_modeActif ){
        if ( true == m_modeActif->gererMouseRelease(event) ){
            return;
        }
    }

    QVTKOpenGLNativeWidget::mouseReleaseEvent(event);
}
void vtk3d_MainView::mouseMoveEvent(QMouseEvent* event) {
    if ( nullptr != m_modeActif ){
        if ( true == m_modeActif->gererMouseMove(event) ){
            return;
        }
    }
    QVTKOpenGLNativeWidget::mouseMoveEvent(event);
}
void vtk3d_MainView::wheelEvent(QWheelEvent* event) {
    if ( nullptr != m_modeActif ){
        if (true == m_modeActif->gererWheelEvent(event)) {
            event->accept();
            return;
        }
    }

    // Si aucun mode n'intercepte l'événement, laisser VTK gérer par défaut
    QVTKOpenGLNativeWidget::wheelEvent(event);
}

void vtk3d_MainView::keyPressEvent(QKeyEvent* event){
    std::cout << "vtk3d_MainView::keyPressEvent "<< std::flush;
    if ( nullptr != m_modeActif ){
        m_modeActif->keyPressEvent(event);
        return;
    }
    QVTKOpenGLNativeWidget::keyPressEvent(event);
}


#define DEBG_CONSOLE_ANALYSERCLIC
SelectionResult vtk3d_MainView::analyserClic(vtkActor* pickedActor, vtkIdType cellId) {
    SelectionResult result;
    result.type = SelectionType::None;
    result.operationId = 0;    // ID du Document (PieceId)
    result.internalVtkId = -1;  // Contiendra le VRAI ID CAO

#ifdef DEBG_CONSOLE_ANALYSERCLIC
    std::cout<<"vtk3d_MainView::analyserClic -> " ;
#endif

    if (!pickedActor || cellId < 0){
        #ifdef DEBG_CONSOLE_ANALYSERCLIC
                std::cout<<"\tRETOUR if (!pickedActor || cellId < 0)" << std::endl;
        #endif
        return result;
    }

    // Récupération du PolyData pour lire les tableaux d'IDs personnalisés
    auto polyData = vtkPolyData::SafeDownCast(pickedActor->GetMapper()->GetInput());

    // 🎯 RECHERCHE DIRECTE
    for (const auto& [docId, node] : m_piecesNodes) {

        // 0. Est-ce le repère d'origine de ce document ?
        if (node.originActor && node.originActor == pickedActor) {
#ifdef DEBG_CONSOLE_ANALYSERCLIC
            std::cout<<"\tnode.originActor" << std::endl;
#endif
            result.operationId = docId;
            result.internalVtkId = static_cast<int>(cellId);
            result.type = SelectionType::Axis;
            if (polyData) {
                auto faceIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeFaceID"));
                if (faceIdsArray && cellId < faceIdsArray->GetNumberOfValues()) {
                    result.internalVtkId = faceIdsArray->GetValue(cellId); // 🏆 On récupère le bon ID !
                } else {
                    result.internalVtkId = static_cast<int>(cellId);
                }
            } else {
                result.internalVtkId = static_cast<int>(cellId);
            }

            return result;
        }

        // 1. Est-ce le solide 3D (Faces) de ce document ?
        if (node.solidActor && node.solidActor == pickedActor) {
#ifdef DEBG_CONSOLE_ANALYSERCLIC
            std::cout<<"\tnode.solidActor" << std::endl;
#endif
            result.operationId = docId;
            result.type = SelectionType::Face;

            // 🎯 CORRECTION : Extraction du vrai ID de face OpenCASCADE
            if (polyData) {
                auto faceIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeFaceID"));
                if (faceIdsArray && cellId < faceIdsArray->GetNumberOfValues()) {
                    result.internalVtkId = faceIdsArray->GetValue(cellId); // 🏆 On récupère le bon ID !
                } else {
                    result.internalVtkId = static_cast<int>(cellId);
                }
            } else {
                result.internalVtkId = static_cast<int>(cellId);
            }
            return result;
        }

        // 2. Est-ce le rendu filaire (Arêtes) de ce document ?
        if (node.edgeActor && node.edgeActor == pickedActor) {
#ifdef DEBG_CONSOLE_ANALYSERCLIC
            std::cout<<"\tnode.edgeActor" << std::endl;
#endif
            result.operationId = docId;
            result.type = SelectionType::Edge;

            // 🎯 CORRECTION : Extraction du vrai ID d'arête OpenCASCADE
            if (polyData) {
                auto edgeIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeEdgeID"));
                if (edgeIdsArray && cellId < edgeIdsArray->GetNumberOfValues()) {
                    result.internalVtkId = edgeIdsArray->GetValue(cellId); // 🏆 On récupère le bon ID !
                } else {
                    result.internalVtkId = static_cast<int>(cellId);
                }
            } else {
                result.internalVtkId = static_cast<int>(cellId);
            }
            return result;
        }

        // 3. Est-ce une des esquisses logées dans le sketchesAssembly de ce document ?
        if (node.sketchesAssembly) {

#ifdef DEBG_CONSOLE_ANALYSERCLIC
            std::cout<<"\tnode.sketchesAssembly";
#endif

            // 🎯 STRATÉGIE DE SÉCURITÉ :
            // Si le polyData possède le tableau d'identifiants propre aux esquisses,
            // on sait à 100% qu'on a cliqué sur une entité d'esquisse.
            if (polyData) {
                // Remplacer "OpenCascadeSketchEdgeID" par le vrai nom du tableau
                // que tu injectes dans ta fonction buildSketchAssembly
                auto sketchIdsArray = vtkIntArray::SafeDownCast(polyData->GetCellData()->GetArray("OpenCascadeEdgeID"));

                if (sketchIdsArray && cellId < sketchIdsArray->GetNumberOfValues()) {
                    result.operationId = docId;
                    result.internalVtkId = sketchIdsArray->GetValue(cellId); // 🏆 Le VRAI ID de la ligne d'esquisse
                    result.type = SelectionType::Sketch;
#ifdef DEBG_CONSOLE_ANALYSERCLIC
                    std::cout<<"\tOK retour" << std::endl;
#endif
                    return result;
                }else{
#ifdef DEBG_CONSOLE_ANALYSERCLIC
                    std::cout<<"\tERR if (sketchIdsArray && cellId < sketchIdsArray->GetNumberOfValues()" << std::endl;
#endif
                }
            }else{
#ifdef DEBG_CONSOLE_ANALYSERCLIC
                std::cout<<"\tERR !polyData" << std::endl;
#endif
            }

            // 🎯 FALLBACK : Si le tableau n'est pas encore implémenté mais qu'on a cliqué sur un acteur
            // qui n'est ni le solide, ni les arêtes, ni le repère d'origine... c'est forcément une esquisse !
            if (pickedActor != node.solidActor && pickedActor != node.edgeActor && pickedActor != node.originActor) {
                result.operationId = docId;
                result.internalVtkId = static_cast<int>(cellId);
                result.type = SelectionType::Sketch;
                return result;
            }
        }
    }

    return result;
}




void vtk3d_MainView::setViewFront() {
    auto camera = m_renderer->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(0.0, -1.0, 0.0); // Vue selon l'axe Y positif
    camera->SetViewUp(0.0, 0.0, 1.0);    // L'axe Z reste vers le haut
    m_renderer->ResetCamera();
    this->renderWindow()->Render();
}

void vtk3d_MainView::setViewSide() {
    auto camera = m_renderer->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(1.0, 0.0, 0.0); // Vue selon l'axe X
    camera->SetViewUp(0.0, 0.0, 1.0);    // L'axe Z reste vers le haut
    m_renderer->ResetCamera();
    this->renderWindow()->Render();
}

void vtk3d_MainView::setViewIsometric() {
    auto camera = m_renderer->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    // Position équilibrée sur les 3 axes (1, 1, 1) pour l'effet standard 3D CAO
    camera->SetPosition(1.0, -1.0, 1.0);
    camera->SetViewUp(0.0, 0.0, 1.0);    // L'axe Z reste vers le haut
    m_renderer->ResetCamera();
    this->renderWindow()->Render();
}

void vtk3d_MainView::showEvent (QShowEvent* event){
    QVTKOpenGLNativeWidget::showEvent(event);

    if (m_axesWidget && !m_axesWidget->GetEnabled()) {
        m_axesWidget->SetInteractor(this->renderWindow()->GetInteractor()); // On donne d'abord l'interacteur
        m_axesWidget->SetEnabled(1);        // On l'active en DERNIER
        m_axesWidget->InteractiveOff();     // On désactive l'interactivité AVANT d'activer le widget
        this->renderWindow()->Render();
    }
}

void vtk3d_MainView::setCategoryVisibility(SelectionType type, bool visible) {
    // On parcourt toutes nos pièces/documents enregistrés
    for (auto& [docId, node] : m_piecesNodes) {
        if (!node.rootAssembly) continue;

        // Déclaration récursive pour propager le flag de sélection (picking) aux feuilles VTK
        std::function<void(vtkProp3D*, bool)> propagerPicking = [&](vtkProp3D* prop, bool pickable) {
            if (!prop) return;
            prop->SetPickable(pickable ? 1 : 0);
            if (auto assembly = vtkAssembly::SafeDownCast(prop)) {
                auto parts = assembly->GetParts();
                if (parts) {
                    parts->InitTraversal();
                    while (auto nextPart = vtkProp3D::SafeDownCast(parts->GetNextProp())) {
                        propagerPicking(nextPart, pickable);
                    }
                }
            }
        };

        // 🎯 On applique le masquage et le verrouillage du clic sur le calque concerné
        if (type == SelectionType::Axis) {
            // Cible directement le calque d'origine dédié !
            if (node.originActor) {
                node.originActor->SetVisibility(visible ? 1 : 0);
                propagerPicking(node.originActor, visible);
            }
        }
        else if (type == SelectionType::Face || type == SelectionType::Edge) {
            // Les volumes 3D (Faces et Arêtes)
            if (node.solidActor) {
                node.solidActor->SetVisibility(visible ? 1 : 0);
                propagerPicking(node.solidActor, visible);
            }
            if (node.edgeActor) {
                node.edgeActor->SetVisibility(visible ? 1 : 0);
                propagerPicking(node.edgeActor, visible);
            }
        }
        else if (type == SelectionType::Sketch) { // 🎯 Cible explicitement la catégorie Sketch
            // Toutes les esquisses du document d'un seul coup
            if (node.sketchesAssembly) {
                int visFlag = visible ? 1 : 0;

                // 1. Déclaration d'une fonction locale pour forcer la visibilité en profondeur
                std::function<void(vtkProp3D*, int)> propagerVisibilite = [&](vtkProp3D* prop, int vis) {
                    if (!prop) return;
                    prop->SetVisibility(vis);

                    if (auto assembly = vtkAssembly::SafeDownCast(prop)) {
                        auto parts = assembly->GetParts();
                        if (parts) {
                            parts->InitTraversal();
                            while (auto nextPart = vtkProp3D::SafeDownCast(parts->GetNextProp())) {
                                propagerVisibilite(nextPart, vis); // Récursion !
                            }
                        }
                    }
                };

                // 2. On applique le traitement sur l'arbre de l'esquisse
                propagerVisibilite(node.sketchesAssembly, visFlag);
                propagerPicking(node.sketchesAssembly, visible);
            }
        }
    }

    // Sécurité : Si on cache des éléments, on coupe la surbrillance active
    if (!visible && m_Chighlighter) {
        m_Chighlighter->masquerSurbrillance();
    }

    this->renderWindow()->Render();
}


void vtk3d_MainView::mode_passerModeEsquisse( uint64_t id ){
    if ( nullptr != m_currentDoc ){
        //CadOperation* opDansDoc = m_currentDoc->trouverOperationMutable(1);
        CadOperation* opDansDoc = m_currentDoc->trouverOperationMutable(id);
        if (opDansDoc){
            if (m_modeActif) {
                m_modeActif->desactiver();
            }
            m_modeActif = std::make_unique<Vtk3d_Sketch>(this, opDansDoc );
            m_modeActif->activer();
            return;
        }else{
            std::cerr << "ERR vtk3d_MainView::mode_passerModeEsquisse operation ["<<id<<"] non trouvée !"<<std::endl;
        }
    }
    m_modeActif = std::make_unique<Vtk3d_Sketch>(this, nullptr);
}


void vtk3d_MainView::mode_passerMode3D(){
    if (m_modeActif) {
        m_modeActif->desactiver();
    }
    if( nullptr != m_currentDoc ){
        m_currentDoc->compute_final_topo();
        synchroniserDocument ( m_CurrentDocumentId, *m_currentDoc);
    }
    m_modeActif = std::make_unique<Vtk3d_Part>(this);
    if (m_modeActif) {
        m_modeActif->activer();
    }
}


void vtk3d_MainView::CADEvent_TraiterCommande(const CadCommandEvent& event) {
    if (!m_modeActif) return;

    // Transmet l'événement au mode actif courant (Esquisse, Assemblage, Mise en plan...)
    m_modeActif->CADEvent_TraiterCommande(event);
}


void vtk3d_MainView::CADEvent_RemonterEvent(const CadResponseEvent& event){
    // On exécute le callback donné par Qt s'il existe
    if (m_onResponseCallback) {
        m_onResponseCallback(event);
    }
}
void vtk3d_MainView::CADEvent_RemonterEvent_SetCallback(CadResponseCallback cb) {
    m_onResponseCallback = cb;
}

