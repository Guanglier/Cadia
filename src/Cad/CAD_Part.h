#pragma once
#ifndef CAD_Part_H
#define CAD_Part_H

#include <string>
#include <vector>
#include "CAD_PartOp.h"

//std::ostream& operator<<(std::ostream& os, PartSketchConstraint::SubElement sub);


class CAD_Part {
private:
    std::string                 m_filename = "";
    std::string                 m_path = "";
    IdRegistry<CadPartOp>       m_operationRegistry;
    uint64_t                    m_operation_nextId = 1;

    void tst_dump_tree(std::ostream& flux_out = std::cout) const;

public:
    CAD_Part();

    void revaluerOperation(CadPartOp& op);
    uint64_t add_operation(CadPartOp& op);

    //TopoDS_Shape evaluerGeometrieSketch(const SketchParams& sketch);
    void tst_add_repere_origine ();
    void tst_add_op_sketch_rect();
    void tst_add_op_sketch_circle();
    void tst_add_op_extrude();
    void tst_add_op_extrude_2();
    void tst_add_op_step_2 ();
    void tst_add_empty_sketch ();
    void tst_dump_tree_to_console ();
    void tst_dump_tree_to_file (const std::string& filename);
    void tst_dump_all_op_to_file ();

    void sauvegarderOperationEnBrep(uint64_t opId, const std::string& chemin, bool exporterLocal);
    void compute_final_topo();

    CadPartOp* trouverOperationMutable(uint64_t id);
    const CadPartOp* trouverOperation(uint64_t id) const;
    //void reconstruirePlanEsquisse(SketchParams& sketch);


    const IdRegistry<CadPartOp>& getOperationRegistry() const {
        return m_operationRegistry;
    }

};

#endif // CAD_Part_H


