



#include "CAD_Document.h"
#include <iostream>
#include <string>
#include <variant>
#include <ostream>

// dump des brep dans des fichiers, à ouvrir par cad assistant par exemple
void CAD_Document::tst_dump_all_op_to_file (){
    const auto& operations = m_operationRegistry.getItems();
    if (operations.empty()) {
        std::cerr << " (Document vide)" << std::endl;
        return;
    }
    for (size_t i = 0; i < operations.size(); ++i) {
        const auto& op = operations[i];
        std::string  string_filename = "_";
        string_filename = string_filename + std::to_string(op.id) + "_"  + op.getTypeName();
        std::cout<< "CAD_Document::tst_dump_all_op_to_file : [" << string_filename << "]" << std::endl;
        std::string  string_filename_local = string_filename + "_local.brep";
        std::string  string_filename_result = string_filename + "_result.brep";
        sauvegarderOperationEnBrep ( op.id, string_filename_local, true);
        sauvegarderOperationEnBrep ( op.id, string_filename_result, false);
    }
}

void CAD_Document::tst_dump_tree(std::ostream& flux_out ) const {
    flux_out << "\n=== ARBORESCENCE DU DOCUMENT CAD ===" << std::endl;

    const auto& operations = m_operationRegistry.getItems();

    if (operations.empty()) {
        flux_out << " (Document vide)" << std::endl;
        return;
    }

    for (size_t i = 0; i < operations.size(); ++i) {
        const auto& op = operations[i];

        bool isLastOp = (i == operations.size() - 1);
        std::string branch = isLastOp ? "|-- " : "|-- ";
        std::string subPrefix = isLastOp ? "    " : "|   ";

        // 1. Affichage de l'opération principale
        flux_out << branch << "[" << op.id << "] " << op.getTypeName() << ":"  << op.getName() << std::endl;

        // 2. Inspection du contenu de l'opération via le Variant
        // CORRECTION 2 : Utilisation de op.getParams() (version const) au lieu de getParamsMutable()
        std::visit([&subPrefix,&flux_out,&op](const auto& params) {
            using T = std::decay_t<decltype(params)>;

            if constexpr (std::is_same_v<T, CoordinateSystem>) {
                flux_out << subPrefix << "|-- Type: Repere Absolu XYZ" << std::endl;
                const auto& repere = params;
                const gp_Ax2& ax2 = repere.AxisSystem;

                gp_Pnt loc = ax2.Location();
                flux_out << subPrefix << "    |-- Origine : ("
                         << loc.X() << ", " << loc.Y() << ", " << loc.Z() << ")" << std::endl;

                gp_Dir dirZ = ax2.Direction();
                gp_Dir dirX = ax2.XDirection();
                gp_Dir dirY = ax2.YDirection();

                flux_out << subPrefix << "    |-- Axe X : ["
                         << dirX.X() << ", " << dirX.Y() << ", " << dirX.Z() << "]" << std::endl;
                flux_out << subPrefix << "    |-- Axe Y : ["
                         << dirY.X() << ", " << dirY.Y() << ", " << dirY.Z() << "]" << std::endl;
                flux_out << subPrefix << "    |-- Axe Z : ["
                         << dirZ.X() << ", " << dirZ.Y() << ", " << dirZ.Z() << "]" << std::endl;
            }
            else if constexpr (std::is_same_v<T, SketchParams>)
            {
                //flux_out << subPrefix << "|-- Support : " << params.targetPlane << std::endl;

                gp_Pnt origin3D = params.m_sketchPlane.Location();
                flux_out << subPrefix << "    |-- Origine 3D : ("
                         << origin3D.X() << ", " << origin3D.Y() << ", " << origin3D.Z() << ")" << std::endl;

                const auto& primitives = params.getPrimitives();
                const auto& constraints = params.getConstraints();

                if (!primitives.empty()) {
                    flux_out << subPrefix << "|--  Primitives :" << std::endl;
                    for (size_t p = 0; p < primitives.size(); ++p) {
                        bool isLastPrim = (p == primitives.size() - 1) && constraints.empty();
                        std::string pBranch = isLastPrim ? "|-- " : "|-- ";

                        std::visit([&subPrefix, &pBranch, &flux_out](const auto& prim) {
                            using PType = std::decay_t<decltype(prim)>;
                            if constexpr (std::is_same_v<PType, SketchLine>) {
                                //flux_out << subPrefix << "|   " << pBranch << "Ligne [ID: " << prim.id << "]" << std::endl;
                                flux_out << subPrefix << "|   " << pBranch << "Ligne [ID: " << prim.id << "]" ;
                                flux_out << " 2D ["<<prim.start.p2d.X() <<";" << prim.start.p2d.Y() << "] -> [" <<prim.stop.p2d.X() <<";" << prim.stop.p2d.Y() << "] ";
                                flux_out << " 3D : ";
                                flux_out << " [" << prim.start.cache_p3d.X() << ";" << prim.start.cache_p3d.Y() << ";" << prim.start.cache_p3d.Z() << "] -> " ;
                                flux_out << " [" << prim.stop.cache_p3d.X() << ";" << prim.stop.cache_p3d.Y() << ";" << prim.stop.cache_p3d.Z() << "] " << std::endl;
                            }
                            else if constexpr (std::is_same_v<PType, SketchCircle>) {
                                flux_out << subPrefix << "|   " << pBranch << "Cercle [ID: " << prim.id << ", R: " << prim.radius << "]" << std::endl;
                            }
                            else if constexpr (std::is_same_v<PType, SketchArc>) {
                                flux_out << subPrefix << "|   " << pBranch << "Arc [ID: " << prim.id << "]" << std::endl;
                            }
                        }, primitives[p]);
                    }
                }

                if (!constraints.empty()) {
                    flux_out << subPrefix << "|--  Contraintes :" << std::endl;
                    for (size_t c = 0; c < constraints.size(); ++c) {
                        bool isLastConst = (c == constraints.size() - 1);
                        std::string cBranch = isLastConst ? "|-- " : "|-- ";
                        const auto& ct = constraints[c];

                        flux_out << subPrefix << "    " << cBranch;
                        if (ct.type == ConstraintType::Coincident) flux_out << "Coincident";
                        else if (ct.type == ConstraintType::Parallel) flux_out << "Parallel";
                        else if (ct.type == ConstraintType::Perpendicular) flux_out << "Perpendicular";
                        else if (ct.type == ConstraintType::Distance) flux_out << "Distance : " << ct.value << "mm";
                        else if (ct.type == ConstraintType::Horizontal) flux_out << "Horizontal ";
                        else if (ct.type == ConstraintType::Vertical) flux_out << "Vertical ";
                        else if (ct.type == ConstraintType::Radius) flux_out << "Radius ";
                        else if (ct.type == ConstraintType::Tangent) flux_out << "Tangent ";

                        flux_out << " : Op" << ct.ref1.operationId << ":Prim" << ct.ref1.primitiveId << ":" << ct.ref1.subElement
                                 << " <-> Op" << ct.ref2.operationId << ":Prim" << ct.ref2.primitiveId << ":" << ct.ref2.subElement << " "
                                 << std::endl;
                    }
                }
            }
            else if constexpr (std::is_same_v<T, ExtrudeParams>) {
                flux_out << subPrefix << "|-- Type: Extrusion" << std::endl;
                flux_out << subPrefix << "    |-- Start : " << params.start << " mm" << std::endl;
                flux_out << subPrefix << "    |-- End : " << params.end << " mm" << std::endl;
                flux_out << subPrefix << "    |-- Esquisse parente ID : " << params.SketchId << std::endl;
                flux_out << subPrefix << "    |-- Bool : " << EBooleanOpToString( params.EboolOp) << std::endl;
            }
            else if constexpr (std::is_same_v<T, BooleanParams>) {
                flux_out << subPrefix << "|-- Type: Operation Bouléenne" << std::endl;
            }
        }, op.getParams()); // Changé ici en op.getParams() pour la const-correctness
    }
    flux_out << "====================================\n" << std::endl;
}

void CAD_Document::tst_dump_tree_to_console (){tst_dump_tree(); }

void CAD_Document::tst_dump_tree_to_file (const std::string& filename){
    std::ofstream logFile(filename);
    if (logFile.is_open()) {
        tst_dump_tree(logFile);
        logFile.close();
    }
}

void CAD_Document::tst_add_empty_sketch (){

    //------------ construction de la sketch -------------------------------
    CadOperation cad_op_sketch("Esquisse 04", SketchParams());
    uint64_t opId_sketch_3 = this->add_operation(cad_op_sketch);


    // 3. On récupère l'accès direct et modifiable à l'esquisse qui vit DANS le document
    CadOperation* opDansDocSketch = this->trouverOperationMutable(opId_sketch_3);
    if (!opDansDocSketch) return; // Sécurité

    auto& sketch3 = std::get<SketchParams>(opDansDocSketch->getParamsMutable());

    sketch3.referenceCoordinateSystemId = 0;
    //sketch3.targetPlane = ReferencePlane::XY;
    //this->reconstruirePlanEsquisse(sketch3);

    revaluerOperation(*opDansDocSketch);

}

void CAD_Document::tst_add_repere_origine() {

    //------------ construction de l'axe d'origine -------------------------------
    CoordinateSystem origineDefaut; // gp::XOY() crée un gp_Ax2 à (0,0,0) avec Z=(0,0,1) et X=(1,0,0)
    gp_Pnt nouvellePosition(0.0, 0.0, 0.0);
    origineDefaut.AxisSystem = gp::XOY();
    origineDefaut.AxisSystem.SetLocation(nouvellePosition);

    CadOperation opOrigine ("Repere 01", origineDefaut) ;
    opOrigine.getParamsMutable() = origineDefaut;
    uint64_t u64_id_repere = this->add_operation(opOrigine);

    //---- pour le test on retrouve le pointeur vers l'op avec l'id et on réévalue
    CadOperation* opRepere = this->trouverOperationMutable(u64_id_repere);
    if (!opRepere) return; // Sécurité
    revaluerOperation(*opRepere);
    opRepere->setOpacity(0.2f);
    opRepere->setColor(37.0f/255.0f, 150.0f/255.0f, 190.0f/255.0f);


}

void CAD_Document::tst_add_op_sketch_rect() {

    

    //------------ construction de la sketch -------------------------------
    CadOperation cad_op("Esquisse 01", SketchParams());
    uint64_t opId = this->add_operation(cad_op);


    // 3. On récupère l'accès direct et modifiable à l'esquisse qui vit DANS le document
    CadOperation* opDansDoc = this->trouverOperationMutable(opId);
    if (!opDansDoc) return; // Sécurité

    opDansDoc->setColor(0.4, 0.7, 0.4);

    auto& sketch = std::get<SketchParams>(opDansDoc->getParamsMutable());


    sketch.referenceCoordinateSystemId = 0;
    //sketch.targetPlane = ReferencePlane::XY;
    //this->reconstruirePlanEsquisse(sketch);

    // 4. On crée nos points géométriques
    // gp_Pnt p1(-25,-10, 0);
    // gp_Pnt p2(25,-10, 0);
    // gp_Pnt p3(25,10, 0);
    // gp_Pnt p4(-25,10, 0);


    //gp_Pnt2d p1 (-25, -10);
    //gp_Pnt2d p2 (25, -10);
    //gp_Pnt2d p3 (25, 10);
    //gp_Pnt2d p4 (-25, 10);

    gp_Pnt2d p1 (-25, -12);
    gp_Pnt2d p2 (-30, 14);
    gp_Pnt2d p3 (30, 16);
    gp_Pnt2d p4 (25, -10);


    // 5. On ajoute les primitives directement dans l'esquisse du document
    // Les ID (1, 2, 3, 4) sont générés à la volée sur place
    uint64_t l1_id = sketch.addPrimitive(SketchLine(p1, p2));       //horizontale
    uint64_t l2_id = sketch.addPrimitive(SketchLine(p2, p3));       //verticale
    uint64_t l3_id = sketch.addPrimitive(SketchLine(p3, p4));       // horizontale droite vers gauche
    uint64_t l4_id = sketch.addPrimitive(SketchLine(p4, p1));       // verticale haut vers bas


    gp_Pnt2d p21 (-2.5, -1.0);
    gp_Pnt2d p22 (2.5, -1.0);
    gp_Pnt2d p23 (2.5, 1.0);
    gp_Pnt2d p24 (-2.5, 1.0);
    sketch.addPrimitive(SketchLine(p21, p22));       //horizontale
    sketch.addPrimitive(SketchLine(p22, p23));       //verticale
    sketch.addPrimitive(SketchLine(p23, p24));       // horizontale droite vers gauche
    sketch.addPrimitive(SketchLine(p24, p21));






    // 6. On configure et on ajoute la contrainte
    SketchConstraint c1;
    c1.type = ConstraintType::Coincident;
    c1.ref1.operationId = opId;
    c1.ref1.primitiveId = l1_id;
    c1.ref1.subElement = ConstraintSubElement::EndPoint;
    c1.ref2.operationId = opId;
    c1.ref2.primitiveId = l2_id;
    c1.ref2.subElement = ConstraintSubElement::StartPoint;
    sketch.addConstraint(c1);

    SketchConstraint c2;
    c2.type = ConstraintType::Coincident;
    c2.ref1.operationId = opId;
    c2.ref1.primitiveId = l2_id;
    c2.ref1.subElement = ConstraintSubElement::EndPoint;
    c2.ref2.operationId = opId;
    c2.ref2.primitiveId = l3_id;
    c2.ref2.subElement = ConstraintSubElement::StartPoint;
    sketch.addConstraint(c2);


    SketchConstraint c3;
    c3.type = ConstraintType::Coincident;
    c3.ref1.operationId = opId;
    c3.ref1.primitiveId = l3_id;
    c3.ref1.subElement = ConstraintSubElement::EndPoint;
    c3.ref2.operationId = opId;
    c3.ref2.primitiveId = l4_id;
    c3.ref2.subElement = ConstraintSubElement::StartPoint;
    sketch.addConstraint(c3);


    SketchConstraint c4;
    c4.type = ConstraintType::Coincident;
    c4.ref1.operationId = opId;
    c4.ref1.primitiveId = l4_id;
    c4.ref1.subElement = ConstraintSubElement::EndPoint;
    c4.ref2.operationId = opId;
    c4.ref2.primitiveId = l1_id;
    c4.ref2.subElement = ConstraintSubElement::StartPoint;
    sketch.addConstraint(c4);


/*
    SketchConstraint dist1;
    dist1.type = ConstraintType::Distance;
    dist1.value = 100.0;
    dist1.ref1.operationId = opId;
    dist1.ref1.primitiveId = l1_id;
    dist1.ref1.subElement = ConstraintSubElement::StartPoint;
    dist1.ref2.operationId = opId;
    dist1.ref2.primitiveId = l3_id;
    dist1.ref2.subElement = ConstraintSubElement::EndPoint;
    sketch.addConstraint(dist1);


    SketchConstraint dist2;
    dist2.type = ConstraintType::Distance;
    dist2.value = 100.0;
    dist2.ref1.operationId = opId;
    dist2.ref1.primitiveId = l2_id;
    dist2.ref1.subElement = ConstraintSubElement::StartPoint;
    dist2.ref2.operationId = opId;
    dist2.ref2.primitiveId = l2_id;
    dist2.ref2.subElement = ConstraintSubElement::EndPoint;
    sketch.addConstraint(dist2);


    SketchConstraint dist3;
    dist3.type = ConstraintType::Distance;
    dist3.value = 30.0;
    dist3.ref1.operationId = opId;
    dist3.ref1.primitiveId = l1_id;
    dist3.ref1.subElement = ConstraintSubElement::StartPoint;
    dist3.ref2.operationId = opId;
    dist3.ref2.primitiveId = l1_id;
    dist3.ref2.subElement = ConstraintSubElement::EndPoint;
    sketch.addConstraint(dist3);

    SketchConstraint dist4;
    dist4.type = ConstraintType::Distance;
    dist4.value = 30.0;
    dist4.ref1.operationId = opId;
    dist4.ref1.primitiveId = l2_id;
    dist4.ref1.subElement = ConstraintSubElement::EndPoint;
    dist4.ref2.operationId = opId;
    dist4.ref2.primitiveId = l4_id;
    dist4.ref2.subElement = ConstraintSubElement::StartPoint;
    sketch.addConstraint(dist4);
*/

    SketchConstraint perp1;
    perp1.type = ConstraintType::Perpendicular;
    perp1.ref1.operationId = opId;
    perp1.ref1.primitiveId = l1_id;
    perp1.ref1.subElement = ConstraintSubElement::Whole;
    perp1.ref2.operationId = opId;
    perp1.ref2.primitiveId = l4_id;
    perp1.ref2.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(perp1);


    SketchConstraint hor1;
    hor1.type = ConstraintType::Horizontal;
    hor1.ref1.operationId = opId;
    hor1.ref1.primitiveId = l2_id;
    hor1.ref1.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(hor1);


    SketchConstraint hor2;
    hor2.type = ConstraintType::Horizontal;
    hor2.ref1.operationId = opId;
    hor2.ref1.primitiveId = l4_id;
    hor2.ref1.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(hor2);

    /*
    SketchConstraint para1;
    para1.type = ConstraintType::Parallel;
    para1.ref1.operationId = opId;
    para1.ref1.primitiveId = l1_id;
    para1.ref1.subElement = ConstraintSubElement::Whole;
    para1.ref2.operationId = opId;
    para1.ref2.primitiveId = l3_id;
    para1.ref2.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(para1);



    SketchConstraint para2;
    para2.type = ConstraintType::Parallel;
    para2.ref1.operationId = opId;
    para2.ref1.primitiveId = l2_id;
    para2.ref1.subElement = ConstraintSubElement::Whole;
    para2.ref2.operationId = opId;
    para2.ref2.primitiveId = l4_id;
    para2.ref2.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(para2);
*/



/*

*/
/*
    SketchConstraint hor1;
    hor1.type = ConstraintType::Horizontal;
    hor1.ref1.operationId = opId;
    hor1.ref1.primitiveId = l1_id;
    hor1.ref1.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(hor1);

    SketchConstraint hor2;
    hor2.type = ConstraintType::Horizontal;
    hor2.ref1.operationId = opId;
    hor2.ref1.primitiveId = l3_id;
    hor2.ref1.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(hor2);

    SketchConstraint vert1;
    vert1.type = ConstraintType::Vertical;
    vert1.ref1.operationId = opId;
    vert1.ref1.primitiveId = l2_id;
    vert1.ref1.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(vert1);

    SketchConstraint vert2;
    vert2.type = ConstraintType::Vertical;
    vert2.ref1.operationId = opId;
    vert2.ref1.primitiveId = l4_id;
    vert2.ref1.subElement = ConstraintSubElement::Whole;
    sketch.addConstraint(vert2);
*/
    //sketch.Contours_ProcessAndValidate();
    revaluerOperation(*opDansDoc);




    //sauvegarderOperationEnBrep(opId, "_Sketch_1_local.brep", true);

    
}

void CAD_Document::tst_add_op_extrude() {
    CadOperation cad_op("Extrusion 01", ExtrudeParams());
    uint64_t opId = this->add_operation(cad_op);

    // 3. On récupère l'accès direct et modifiable
    CadOperation* opDansDoc = this->trouverOperationMutable(opId);
    if (!opDansDoc) return; // Sécurité
    auto& extrusion_param = std::get<ExtrudeParams>(opDansDoc->getParamsMutable());

    extrusion_param.start = -2.5;
    extrusion_param.end = 2.5;
    extrusion_param.SketchId = 1;   //on donne l'id de la sketch qui est ici 1 car après le repère
    opDansDoc->setOpacity(1.0f);
    opDansDoc->setColor(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f);

    revaluerOperation(*opDansDoc);
    //sauvegarderOperationEnBrep(opId, "_Extrude_1_local.brep", true);
}

void CAD_Document::tst_add_op_sketch_circle() {

    uint64_t u64_id_repere = 0;

    //------------ construction de la sketch -------------------------------
    CadOperation cad_op("Esquisse 02", SketchParams());
    uint64_t opId = this->add_operation(cad_op);


    // 3. On récupère l'accès direct et modifiable à l'esquisse qui vit DANS le document
    CadOperation* opDansDoc = this->trouverOperationMutable(opId);
    if (!opDansDoc) return; // Sécurité
    opDansDoc->setColor(0.5, 0.8, 0.5);

    auto& sketch = std::get<SketchParams>(opDansDoc->getParamsMutable());


    sketch.referenceCoordinateSystemId = u64_id_repere;
    //sketch.targetPlane = ReferencePlane::XY;
    //this->reconstruirePlanEsquisse(sketch);

    // --- C'est ici qu'on dessine le cercle
    gp_Pnt2d centreCercle(-25, 0.0);
    //gp_Pnt2d centreCercle(-25, 0.0);
    double rayon = 5.0;
    sketch.addPrimitive(SketchCircle(centreCercle, rayon));

    // Forcer le calcul géométrique du Wire OpenCASCADE de la Sketch
    this->revaluerOperation(*opDansDoc);
    //sauvegarderOperationEnBrep(opId, "_Sketch_2_local.brep", true);
}

void CAD_Document::tst_add_op_extrude_2() {
    CadOperation cad_op("Extrusion 02", ExtrudeParams());
    uint64_t opId = this->add_operation(cad_op);

    // 3. On récupère l'accès direct et modifiable
    CadOperation* opDansDoc = this->trouverOperationMutable(opId);
    if (!opDansDoc) return; // Sécurité
    auto& extrusion_param = std::get<ExtrudeParams>(opDansDoc->getParamsMutable());
    opDansDoc->setOpacity(1);

    extrusion_param.start = -10.0;
    extrusion_param.end = 10;
    extrusion_param.SketchId = 3;   //on donne l'id de la sketch qui est ici 3
    extrusion_param.EboolOp = EBooleanOp::Substract;

    revaluerOperation(*opDansDoc);
    //sauvegarderOperationEnBrep(opId, "_Extrude_2_local.brep", true);
}

void CAD_Document::tst_add_op_step_2 (){

    //------------ construction de la sketch -------------------------------
    CadOperation cad_op_sketch("Esquisse 03", SketchParams());
    uint64_t opId_sketch_3 = this->add_operation(cad_op_sketch);


    // 3. On récupère l'accès direct et modifiable à l'esquisse qui vit DANS le document
    CadOperation* opDansDocSketch = this->trouverOperationMutable(opId_sketch_3);
    if (!opDansDocSketch) return; // Sécurité

    auto& sketch3 = std::get<SketchParams>(opDansDocSketch->getParamsMutable());


    sketch3.referenceCoordinateSystemId = 0;
    //sketch3.targetPlane = ReferencePlane::XY;
    //this->reconstruirePlanEsquisse(sketch3);

    // 4. On crée nos points géométriques



    gp_Pnt2d p1(0, 0);
    gp_Pnt2d p2(5, 5);
    gp_Pnt2d p3(10, 0);
    gp_Pnt2d p4(5, -5);


    // 5. On ajoute les primitives directement dans l'esquisse du document
    uint64_t l1_id = sketch3.addPrimitive(SketchLine(p1, p2));
    uint64_t l2_id = sketch3.addPrimitive(SketchLine(p2, p3));
    uint64_t l3_id = sketch3.addPrimitive(SketchLine(p3, p4));
    uint64_t l4_id = sketch3.addPrimitive(SketchLine(p4, p1));

    revaluerOperation(*opDansDocSketch);

    //----------------- extrusion ----------------------------
    CadOperation cad_op3("Extrusion 03", ExtrudeParams());
    uint64_t opIdExtrude3 = this->add_operation(cad_op3);

    // 3. On récupère l'accès direct et modifiable
    CadOperation* opDansDoc3 = this->trouverOperationMutable(opIdExtrude3);
    if (!opDansDoc3) return; // Sécurité
    auto& extrusion_param3 = std::get<ExtrudeParams>(opDansDoc3->getParamsMutable());

    extrusion_param3.start = 0.0;
    extrusion_param3.end = 15.0;
    extrusion_param3.SketchId = opId_sketch_3;   //on donne l'id de la sketch
    extrusion_param3.EboolOp = EBooleanOp::Union;

    opDansDoc3->setOpacity(1);
    revaluerOperation(*opDansDoc3);
}



