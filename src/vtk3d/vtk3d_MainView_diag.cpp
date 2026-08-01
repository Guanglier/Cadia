

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vtkMapper.h>
#include <vtkCellData.h>
#include "vtk3d_MainView.h"

#include "vtk3d_MainView.h"
#include <iomanip>
#include <vtkAssembly.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkMapper.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
#include <vtkProp3DCollection.h>


// Fonction utilitaire récursive pour inspecter l'arborescence VTK
void vtk3d_MainView::diag_dumpProp3D(vtkProp3D* prop, std::ostream& out, const PieceRenderNode& node, int depth) {
    if (!prop) return;

    // Indentation proportionnelle à la profondeur dans l'arbre
    std::string indent(depth * 4, ' ');

    // 1. Récupération des infos de classe de base
    std::string className = prop->GetClassName();
    int visible = prop->GetVisibility();
    int pickable = prop->GetPickable();
    
    out << indent << "|-- [" << className << "] Address: " << prop << " Var: " << ptr_to_string (prop, node)
        << " | Visible: " << (visible ? "YES" : "NO")
        << " | Pickable: " << (pickable ? "YES" : "NO");

    // 2. Si c'est un Acteur (Feuille graphique)
    if (auto actor = vtkActor::SafeDownCast(prop)) {
        if (auto mapper = actor->GetMapper()) {
            if (auto polyData = vtkPolyData::SafeDownCast(mapper->GetInput())) {
                out << " | Cells: " << polyData->GetNumberOfCells()
                    << " | Points: " << polyData->GetNumberOfPoints();
                
                // Inspecter la présence de nos tableaux d'ID fétiches
                auto cellData = polyData->GetCellData();
                if (cellData) {
                    out << " | Arrays: (";
                    bool hasArrays = false;
                    if (cellData->GetArray("OpenCascadeFaceID")) { out << "FaceID "; hasArrays = true; }
                    if (cellData->GetArray("OpenCascadeEdgeID")) { out << "EdgeID "; hasArrays = true; }
                    if (cellData->GetArray("OpenCascadeSketchEdgeID")) { out << "SketchID "; hasArrays = true; }
                    if (cellData->GetArray("OpenCascadeAxisID")) { out << "AxisID "; hasArrays = true; }
                    if (!hasArrays) out << "None";
                    out << ")";
                }
            }
        }
    }
    out << "\n";

    // 3. Si c'est un Assembly (Conteneur nœud de branches) -> Descente récursive
    if (auto assembly = vtkAssembly::SafeDownCast(prop)) {
        auto parts = assembly->GetParts();
        if (parts) {
            parts->InitTraversal();
            while (auto nextPart = vtkProp3D::SafeDownCast(parts->GetNextProp())) {
                diag_dumpProp3D(nextPart, out, node, depth + 1);
            }
        }
    }
}



std::string vtk3d_MainView::ptr_to_string(vtkProp* li_ptr, const PieceRenderNode& node) {
    if (!li_ptr) return "unknown";

    if (li_ptr == node.rootAssembly.GetPointer())     { return "rootAssembly"; }
    if (li_ptr == node.solidActor.GetPointer())       { return "solidActor"; }
    if (li_ptr == node.edgeActor.GetPointer())        { return "edgeActor"; }
    if (li_ptr == node.originActor.GetPointer())      { return "originActor"; } // Ajout de l'origine au passage !
    if (li_ptr == node.sketchesAssembly.GetPointer()) { return "sketchesAssembly"; }

    return "unknown";
}

// Fonction principale de Dump
void vtk3d_MainView::diag_dumpArchitecture(std::ostream& out) {
    out << "\n============================================================\n";
    out << "       vtk3d_MainView : DUMP ARCHITECTURE DES PIECES NODES       \n";
    out << "============================================================\n";
    
    if (m_piecesNodes.empty()) {
        out << "Aucun part/nœud actif dans la vue 3D.\n";
        return;
    }

    for (const auto& [docId, node] : m_piecesNodes) {
        out << "\n part ID: " << docId << " (Node Piece ID: " << node.pieceId << ")\n";
        
        // Pointers de tracking du Node
        out << "  |-- Pointers Tracked:\n";
        out << "  |   |-- rootAssembly:     " << node.rootAssembly.GetPointer() << "\n";
        out << "  |   |-- solidActor:       " << node.solidActor.GetPointer() << "\n";
        out << "  |   |-- edgeActor:        " << node.edgeActor.GetPointer() << "\n";
        out << "  |   |-- originActor:      " << node.originActor.GetPointer() << "\n";
        out << "  |   |-- sketchesAssembly: " << node.sketchesAssembly.GetPointer() << "\n";
        out << "  |-- VTK Tree Hierarchy:\n";

        if (node.rootAssembly) {
            diag_dumpProp3D(node.rootAssembly, out, node, 1);
        } else {
            out << "  │   [!] rootAssembly is NULL\n";
        }
    }
    out << "============================================================\n\n" ;
}

