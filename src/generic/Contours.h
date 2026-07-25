
#pragma once

//#include <TopoDS_Shape.hxx>
//#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
//#include <gp_Ax2.hxx>
//#include <gp_Ax3.hxx>
//#include <gp_Vec.hxx>
//#include <string>
//#include <ElSLib.hxx>
#include <Bnd_Box2d.hxx>



enum class ContoursPrimitiveType {
    Line,
    Arc,
    Circle
};



struct ContoursElement {
    uint64_t                primitiveId;
    ContoursPrimitiveType   type = ContoursPrimitiveType::Line;
    gp_Pnt2d                StartCpy2D;
    gp_Pnt2d                StopCpy2D;

    // Données spécifiques directes (Le buffer de performance)
    // On ne stocke que le strict minimum pour éviter d'interroger la structure mère
    gp_Pnt2d      CenterCpy2D;   // Utilisé par l'Arc et le Cercle
    double        Radius = 0.0;  // Utilisé par l'Arc et le Cercle
};

struct Contour {
    std::vector<ContoursElement>    elements;     // On stocke les IDs des primitives qui forment ce contour, dans l'ordre de parcours !
    bool                            isClosed = false;                  // Indique si le contour est bien fermé (début du premier = fin du dernier)
    bool                            isInternal = false;                // Indique si c'est le contour extérieur (Outer) ou un évidement intérieur (Inner/Hole)
    bool                            hasError = false;
    double                          Aire = 0.0;
    Bnd_Box2d                       BoundingBox2D;
    std::string                     diagnosticMessage;
};




// Structure de sortie mise à jour
struct ContoursTopologyResult {
    std::vector<Contour>         validContours;     // L'index dans ce vecteur sert d'ID naturel
    std::vector<ContoursElement> danglingElements;  // Les segments orphelins
    bool                         hasErrors = false;
    bool                        hasWarnings = false;
    std::string                  warningMessage;
    std::string                  errorMessage;
};

class ContoursEngine {
public:
    ContoursEngine() = default;
    ~ContoursEngine() = default;

    static ContoursTopologyResult Process(const std::vector<ContoursElement>& inputPrimitives);

    static void Contours_DisplayContours(const ContoursTopologyResult &ContourResultInput);
    static void Contours_DisplayContour(const Contour& current);

private:
    static std::vector<Contour> Contours_IdentifieContours(const std::vector<ContoursElement>& inputPrimitives);

};






