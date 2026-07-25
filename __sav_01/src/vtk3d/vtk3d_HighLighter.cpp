
#include "vtk3d_HighLighter.h"


#include <vtkSelectionNode.h>
#include <vtkSelection.h>
#include <vtkExtractSelection.h>
#include <vtkDataSetMapper.h>
#include <vtkIntArray.h>
#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkGeometryFilter.h>

#include <vtkpolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>



void vtk3d_HighLighter::extraireEtAfficherSelection(vtkPolyData* sourcePolyData,
                                                  int id,
                                                  const std::string& arrayName,
                                                  vtkActor* targetActor,
                                                  const std::string& debugLabel,
                                                  double scale )
{
    if (!sourcePolyData || id < 0 || !targetActor) return;

    // 1. Pipeline de sélection
    auto selectionNode = vtkSmartPointer<vtkSelectionNode>::New();
    selectionNode->SetContentType(vtkSelectionNode::PEDIGREEIDS);
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), 1);
    selectionNode->GetProperties()->Set(vtkDataObject::FIELD_NAME(), arrayName.c_str());

    auto selectionList = vtkSmartPointer<vtkIntArray>::New();
    selectionList->InsertNextValue(id);
    selectionNode->SetSelectionList(selectionList);

    auto selection = vtkSmartPointer<vtkSelection>::New();
    selection->AddNode(selectionNode);

    // 2. Extraction géométrique
    auto extractor = vtkSmartPointer<vtkExtractSelection>::New();
    extractor->SetInputData(0, sourcePolyData);
    extractor->SetInputData(1, selection);

    auto geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
    geometryFilter->SetInputConnection(extractor->GetOutputPort());
    geometryFilter->Update();

    // 3. Validation et Logs
    vtkPolyData* outputSelection = geometryFilter->GetOutput();
    if (!outputSelection || outputSelection->GetNumberOfCells() == 0) {
        std::cout << "[" << debugLabel << "] ATTENTION : 0 cellules extraites pour l'ID " << id << std::endl;
        targetActor->VisibilityOff();
        return;
    } else {
        std::cout << "[" << debugLabel << "] Succes ! " << outputSelection->GetNumberOfCells()
        << " cellules extraites pour l'ID " << id << std::endl;
    }

    // 4. Réutilisation ou création du Mapper (Évite les fuites de ressources)
    vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(targetActor->GetMapper());

    if (!mapper) {
        // Premier appel : On crée le mapper et on configure ses propriétés permanentes
        auto newMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        newMapper->ScalarVisibilityOff();

        // Décalages relatifs pour passer devant l'esquisse d'origine au niveau du Z-buffer
        newMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-2.0, -2.0);
        newMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-2.0, -2.0);

        targetActor->SetMapper(newMapper);
        mapper = newMapper; // On récupère le pointeur nu pour la suite
    }

    // 5. Mise à jour dynamique de la connexion de données
    mapper->SetInputConnection(geometryFilter->GetOutputPort());
    mapper->Modified();

    // 6. Configuration de l'Actor
    targetActor->SetScale(scale, scale, scale);
    targetActor->GetProperty()->SetLineWidth(3);
    targetActor->VisibilityOn();
}



void vtk3d_HighLighter::mettreEnSurbrillanceFaceParId(vtkPolyData* sourcePolyData, int faceId) {
    masquerSurbrillance();
    extraireEtAfficherSelection(sourcePolyData, faceId, "OpenCascadeFaceID", m_highlightFaceActor, "Surbrillance Face");
}
void vtk3d_HighLighter::mettreEnSurbrillanceEdgeParId(vtkPolyData* sourcePolyData, int edgeId) {
    masquerSurbrillance();
    extraireEtAfficherSelection(sourcePolyData, edgeId, "OpenCascadeEdgeID", m_highlightEdgeActor, "Surbrillance Edge");
}
void vtk3d_HighLighter::mettreEnSurbrillanceAxeParId(vtkPolyData* sourcePolyData, int axeId, double scale) {
    masquerSurbrillance();
    extraireEtAfficherSelection(sourcePolyData, axeId, "OpenCascadeFaceID", m_highlightAxisActor, "Surbrillance axe", scale);
    std::cout<< "vtk3d_HighLighter::mettreEnSurbrillanceAxeParId scale=" << scale << std::endl;
}





vtk3d_HighLighter::vtk3d_HighLighter(vtkRenderer* renderer) : m_renderer(renderer){
    if (!m_renderer){
        std::cerr << "ERROR vtk3d_HighLighter::vtk3d_HighLighter ->  !m_renderer "<< std::endl;
        return;
    }

    // 1. Initialisation de l'acteur pour les FACES (Orange translucide)
    m_highlightFaceActor = vtkSmartPointer<vtkActor>::New();
    m_highlightFaceActor->GetProperty()->SetColor(1.0, 0.5, 0.0);
    m_highlightFaceActor->GetProperty()->SetOpacity(0.6);
    m_highlightFaceActor->GetProperty()->LightingOff();
    m_highlightFaceActor->VisibilityOff();
    m_renderer->AddActor(m_highlightFaceActor);

    // 2. Initialisation de l'acteur pour les ARÊTES (Bleu épais)
    m_highlightEdgeActor = vtkSmartPointer<vtkActor>::New();
    m_highlightEdgeActor->GetProperty()->SetColor(0.0, 0.6, 1.0);
    m_highlightEdgeActor->GetProperty()->SetLineWidth(6.0);
    m_highlightEdgeActor->GetProperty()->SetOpacity(0.9);
    m_highlightEdgeActor->GetProperty()->LightingOff();
    m_highlightEdgeActor->GetProperty()->RenderLinesAsTubesOn();
    m_highlightEdgeActor->VisibilityOff();
    m_renderer->AddActor(m_highlightEdgeActor);

    m_highlightAxisActor = vtkSmartPointer<vtkActor>::New();
    m_highlightAxisActor->GetProperty()->SetColor(1.0, 0.5, 0.0);
    m_highlightAxisActor->GetProperty()->SetOpacity(0.6);
    m_highlightAxisActor->GetProperty()->LightingOff();
    m_highlightAxisActor->VisibilityOff();
    m_renderer->AddActor(m_highlightAxisActor);


}


void vtk3d_HighLighter::masquerSurbrillance() {
    if (m_highlightFaceActor) {
        m_highlightFaceActor->VisibilityOff();
    }
    if (m_highlightEdgeActor) {
        m_highlightEdgeActor->VisibilityOff();
    }
    if (m_highlightAxisActor) {
        m_highlightAxisActor->VisibilityOff();
    }
}

