
#include "CAD_Document.h"
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <variant>
#include <type_traits>
#include <gp_Pnt.hxx>
#include <GC_MakeCircle.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>



CAD_Document::CAD_Document() {
    // Constructeur
}



CadOperation* CAD_Document::trouverOperationMutable(uint64_t id) {
    for (auto& op : m_operationRegistry.getItemsMutable()) {
        if (op.id == id) return &op;
    }
    return nullptr;
}
const CadOperation* CAD_Document::trouverOperation(uint64_t id) const {
    for (const auto& op : m_operationRegistry.getItems()) {
        if (op.id == id) return &op;
    }
    return nullptr;
}

// Fonction utilitaire globale ou statique
void CAD_Document::sauvegarderOperationEnBrep(uint64_t opId, const std::string& chemin, bool exporterLocal) {
    std::cout << "CAD_Document::sauvegarderOperationEnBrep" << std::endl;
    const CadOperation* op = trouverOperation(opId);
    if (!op) return ;
    const TopoDS_Shape& shapeAExporter = exporterLocal ? op->getLocalTopo() : op->getResultingTopo();
    if (shapeAExporter.IsNull()) {
        std::cerr << "\t->[Export BREP] Impossible d'exporter : la TopoDS_Shape est vide (Null)." << std::endl;
        return ;
    }
    std::ofstream fichier(chemin);
    if (!fichier.is_open()) {
        std::cerr << "\t->[Export BREP] Impossible de créer ou d'ouvrir le fichier : " << chemin << std::endl;
        return ;
    }
    BRepTools::Write(shapeAExporter, fichier);
    fichier.close();
    std::cout << "\t->[Debug Export] Fichier '" << chemin << "' enregistré avec succès." << std::endl;
}



std::ostream& operator<<(std::ostream& os, ConstraintSubElement sub) {
    switch (sub) {
    case ConstraintSubElement::Whole:       os << "Whole"; break;
    case ConstraintSubElement::StartPoint:  os << "StartPoint"; break;
    case ConstraintSubElement::EndPoint:    os << "EndPoint"; break;
    case ConstraintSubElement::CenterPoint: os << "CenterPoint"; break;
    default:                                os << "ERR inconnu"; break;
    }
    return os;
}


uint64_t CAD_Document::add_operation(CadOperation& op) {
    return m_operationRegistry.add(std::move(op));
}
void CAD_Document::revaluerOperation(CadOperation& op) {
    std::cout << "CAD_Document::revaluerOperation -> name = " << op.getName() << "\n";
    op.execute(*this, false);
}

void CAD_Document::compute_final_topo() {
    std::cout << "\nCAD_Document::compute_final_topo" << std::endl;

    auto& operations = m_operationRegistry.getItemsMutable();
    TopoDS_Shape solideGlobalPrecedent;
    bool unAncetreAChange = false;

    // Parcourir toutes les opérations séquentiellement
    for (size_t i = 0; i < operations.size(); ++i) {
        auto& op = operations[i];

        // 1. Détection des changements dans l'arbre
        if (op.hasLocaleTopoChanged()) {
            unAncetreAChange = true;
            std::cout << "\t-> Changement détecté sur l'opération [" << op.getName() << "]" << std::endl;
        }

        // 2. Exécution / Évaluation de la géométrie locale
        op.execute(*this, unAncetreAChange);
        op.setLocaleTopoChanged(false);

        // 3. Calcul de la topologie GLOBALE (m_resultingTopo)
        if (i == 0) {
            // Première opération de l'arbre (généralement le repère d'origine ou la forme de base)
            op.setResultingTopo(op.getLocalTopo());
        }
        else if (unAncetreAChange) {
            // Un ancêtre ou l'opération elle-même a bougé : on recalcule l'accumulation de matière

            if (std::holds_alternative<ExtrudeParams>(op.getParams()) ||
                std::holds_alternative<BooleanParams>(op.getParams())) {

                TopoDS_Shape maGeometriePropre = op.getLocalTopo();

                if (!solideGlobalPrecedent.IsNull() && !maGeometriePropre.IsNull()) {

                    if (std::holds_alternative<ExtrudeParams>(op.getParams())) {
                        const ExtrudeParams& ext = std::get<ExtrudeParams>(op.getParams());

                        if (ext.EboolOp == EBooleanOp::Union) {
                            BRepAlgoAPI_Fuse fusion(solideGlobalPrecedent, maGeometriePropre);
                            if (fusion.IsDone()) {
                                op.setResultingTopo(fusion.Shape());
                                std::cerr << "\t-> bool : fusion " << op.getName() << std::endl;
                            }
                            else {
                                std::cerr << "\t-> Échec de la fusion pour " << op.getName() << std::endl;
                                op.setResultingTopo(solideGlobalPrecedent); // Secours
                            }
                        }
                        else if (ext.EboolOp == EBooleanOp::Substract) {
                            BRepAlgoAPI_Cut cut(solideGlobalPrecedent, maGeometriePropre);
                            if (cut.IsDone()) {
                                op.setResultingTopo(cut.Shape());
                                std::cerr << "\t-> bool : substract " << op.getName() << std::endl;
                            }
                            else {
                                std::cerr << "\t-> Échec de la soustraction pour " << op.getName() << std::endl;
                                op.setResultingTopo(solideGlobalPrecedent); // Secours
                            }
                        }
                        else if (ext.EboolOp == EBooleanOp::Intersect) {
                            BRepAlgoAPI_Common intersection(solideGlobalPrecedent, maGeometriePropre);
                            if (intersection.IsDone()) {
                                op.setResultingTopo(intersection.Shape());
                                std::cerr << "\t-> bool : intersect " << op.getName() << std::endl;
                            }
                            else {
                                std::cerr << "\t-> Échec de l'intersection pour " << op.getName() << std::endl;
                                op.setResultingTopo(solideGlobalPrecedent); // Secours
                            }
                        }
                        else if (ext.EboolOp == EBooleanOp::None) {
                            // On crée un composé (Compound) pour stocker des volumes disjoints sans les fusionner mathématiquement
                            BRep_Builder builder;
                            TopoDS_Compound assemblage;
                            builder.MakeCompound(assemblage);

                            if (!solideGlobalPrecedent.IsNull()) {
                                builder.Add(assemblage, solideGlobalPrecedent);
                            }
                            if (!maGeometriePropre.IsNull()) {
                                builder.Add(assemblage, maGeometriePropre);
                            }
                            op.setResultingTopo(assemblage);
                            std::cerr << "\t-> bool : None (Compound) " << op.getName() << std::endl;
                        }
                        else {
                            std::cerr << "\tERREUR : EBooleanOp non reconnu dans " << op.getName() << std::endl;
                        }
                    }
                    else {
                        // TODO: Traiter ici le cas de la structure purement BooleanParams si elle diffère d'ExtrudeParams
                        std::cerr << "\t-> Traitement optionnel de BooleanParams pour " << op.getName() << std::endl;
                    }
                }
                else {
                    // Si aucun solide global n'existait avant, le global actuel devient le local propre
                    std::cout << "\t-> Precedent vide ou local vide pour [" << op.getName() << "] " << std::endl;
                    op.setResultingTopo(maGeometriePropre.IsNull() ? solideGlobalPrecedent : maGeometriePropre);
                }
            }
            else {
                // C'est une Sketch ou un Repère : ces éléments n'altèrent pas le volume solide global.
                // CORRECTION : On évite d'écraser leur propre topologie résultante avec le solide complet,
                // on se contente de logger et on passe notre chemin.
                std::cout << "\t-> [Donnee de construction] Pas de modification de solide pour [" << op.getName() << "]" << std::endl;
            }
        }
        else {
            // Performance : Rien n'a bougé sur cette opération ni en amont, le cache interne d'OpenCASCADE fait foi.
            std::cout << "\t-> [Cache Reutilise] L'opération [" << op.getName() << "] est inchangee." << std::endl;
        }

        // 4. MISE À JOUR DE LA CHAÎNE DE TRANSMISSION
        // CORRECTION : On ne met à jour 'solideGlobalPrecedent' QUE si l'opération courante est un élément volumique.
        // Si c'est une Sketch ou un Repère, 'solideGlobalPrecedent' conserve sa valeur de l'étape précédente
        // et saute par-dessus la Sketch pour nourrir l'opération 3D suivante sans interruption.
        if (std::holds_alternative<ExtrudeParams>(op.getParams()) ||
            std::holds_alternative<BooleanParams>(op.getParams())) {

            solideGlobalPrecedent = op.getResultingTopo();
        }
    }

    std::cout << "\t-> FIN compute_final_topo\n" << std::endl;
}





