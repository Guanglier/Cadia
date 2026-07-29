#ifndef CAD_OPERATION_H
#define CAD_OPERATION_H

#include <string>
#include <vector>
#include <variant>
#include <type_traits>
#include <iostream>
#include <algorithm> // Pour std::remove_if

// OpenCASCADE
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Vec.hxx>
#include <string>
#include <ElSLib.hxx>
#include <Bnd_Box2d.hxx>
#include "chrono.h"

#include "Contours.h"

// On indique au compilateur que cette classe existe sans l'inclure tout de suite
class CAD_Document;

// ==========================================
// 1. STRUCTURES DE BASE & ENUMS
// ==========================================

struct Identifiable {
    uint64_t id = 0;
};

enum class ConstraintType {
    Horizontal, Vertical, Parallel, Perpendicular,
    Coincident, Tangent, Distance, Radius
};

enum class ConstraintSubElement {
    Whole, StartPoint, EndPoint, CenterPoint
};
std::ostream& operator<<(std::ostream& os, ConstraintSubElement sub);

struct GeometryReference {
    uint64_t             operationId = 0;
    uint64_t             primitiveId = 0;
    ConstraintSubElement subElement = ConstraintSubElement::Whole;
};

struct SketchConstraint : public Identifiable {
    ConstraintType    type;
    GeometryReference ref1;
    GeometryReference ref2;
    double            value = 0.0;
    bool              isDriven = false;

    bool isEquivalentTo(const SketchConstraint& other) const {
        // 1. Si les types de contraintes diffèrent, elles ne sont pas équivalentes
        if (this->type != other.type) {
            return false;
        }

        // 2. Comparer les identités des entités ou points ciblés
        // (selon la façon dont vos contraintes stockent leurs références, ex: IDs de points ou de primitives)
        // Note : Il faut parfois gérer la symétrie (ex: une contrainte A->B équivaut à B->A pour certaines règles)

        return false;
    }
};

// ==========================================
// 2. PRIMITIVES DE L'ESQUISSE
// ==========================================
struct SketchPoint : public Identifiable{
    uint64_t id;
    gp_Pnt2d p2d; // position 2D dans l'esquisse
    mutable gp_Pnt   cache_p3d; // position 3D dans l'espace de la pièce
    SketchPoint(const gp_Pnt2d& point2D) : p2d(point2D), cache_p3d(0, 0, 0) {}
    void setPoint(const gp_Pnt2d& li_Pnt2D, const gp_Ax3* li_SketchPlane = nullptr) {
        p2d = li_Pnt2D; // On met à jour la position 2D dans tous les cas
        if (li_SketchPlane != nullptr) {        // Si un plan a été fourni (le pointeur n'est pas nul), on met à jour le cache 3D
            cache_p3d = ElSLib::Value(p2d.X(), p2d.Y(), *li_SketchPlane);
        }
    }
    void Update3D(const gp_Ax3& li_SketchPlane ) const {
        cache_p3d = ElSLib::Value(p2d.X(), p2d.Y(), li_SketchPlane);
    }
};

struct SketchLine : public Identifiable {
    uint64_t startPointId = 0; // ID du point de départ dans m_points
    uint64_t stopPointId = 0;  // ID du point de fin dans m_points
    SketchLine(const uint64_t startId, const uint64_t endId)
        : startPointId(startId), stopPointId(endId) {}
    bool    b_IsRef = false;
};

struct SketchCircle : public Identifiable {
    uint64_t centerPointId = 0; // ID du point centre dans m_points
    SketchPoint center;
    double radius;
    SketchCircle(gp_Pnt2d c, double r) : center(c), radius(r) {
    }
    bool    b_IsRef = false;
};


struct SketchArc : public Identifiable {
    uint64_t startPointId = 0;
    uint64_t midPointId = 0;
    uint64_t endPointId = 0;
    gp_Pnt startPoint;
    gp_Pnt midPoint;
    gp_Pnt endPoint;
    SketchArc(gp_Pnt start, gp_Pnt mid, gp_Pnt end) : startPoint(start), midPoint(mid), endPoint(end) {}
    bool    b_IsRef = false;
};


using SketchPrimitive = std::variant<SketchLine, SketchCircle, SketchArc>;

//using SketchPrimitive = std::variant<SketchLine, SketchCircle>;


// ==========================================
// 3. LE GESTIONNAIRE D'ID UNIQUE (TEMPLATE)
// ==========================================

template <typename T>
class IdRegistry {
private:
    std::vector<T> m_items;
    uint64_t       m_nextId = 0;

public:
    const std::vector<T>& getItems() const { return m_items; }
    std::vector<T>& getItemsMutable() { return m_items; }

    T* findMutable(uint64_t id) {
        for (auto& item : m_items) {
            uint64_t itemId = 0;

            if constexpr (std::is_same_v<T, SketchPrimitive>) {
                itemId = std::visit([](const auto& arg) { return arg.id; }, item);
            }
            else {
                itemId = item.id;
            }
            if (itemId == id) {
                return &item; // Trouvé ! On renvoie l'adresse de l'élément dans le vecteur
            }
        }
        return nullptr; // Non trouvé
    }
    const T* find(uint64_t id) const {
        for (const auto& item : m_items) {
            uint64_t itemId = 0;
            if constexpr (std::is_same_v<T, SketchPrimitive>) {
                itemId = std::visit([](const auto& arg) { return arg.id; }, item);
            } else {
                itemId = item.id;
            }
            if (itemId == id) {
                return &item;
            }
        }
        return nullptr;
    }
    uint64_t add(T item) {
        uint64_t assignedId = m_nextId++;
        if constexpr (std::is_same_v<T, SketchPrimitive>) {
            std::visit([assignedId](auto& arg) { arg.id = assignedId; }, item);
        }
        else {
            item.id = assignedId;
        }
        m_items.push_back(std::move(item));
        return assignedId;
    }

    void remove(uint64_t id) {
        m_items.erase(
            std::remove_if(m_items.begin(), m_items.end(), [id](const T& item) {
                if constexpr (std::is_same_v<T, SketchPrimitive>) {
                    return std::visit([](const auto& arg) { return arg.id; }, item) == id;
                }
                else {
                    return item.id == id;
                }
                }),
            m_items.end()
        );
    }

    void load(T item) {
        uint64_t loadedId = 0;
        if constexpr (std::is_same_v<T, SketchPrimitive>) {
            loadedId = std::visit([](const auto& arg) { return arg.id; }, item);
        }
        else {
            loadedId = item.id;
        }
        if (loadedId >= m_nextId) {
            m_nextId = loadedId + 1;
        }
        m_items.push_back(std::move(item));
    }
};


// ==========================================
// 4. PARAMS DES OPERATIONS (FEATURES)
// ==========================================

struct SketchParams {
private:
    IdRegistry<SketchPrimitive>     m_primitiveRegistry;
    IdRegistry<SketchConstraint>    m_constraintRegistry;
    IdRegistry<SketchPoint>         m_points;

    std::vector<ContoursElement> PrepareEnginePrimitives() const ;

public:

    SketchParams() : m_sketchPlane(gp_Ax3()) {}
    explicit SketchParams(const gp_Ax3& plane) : m_sketchPlane(plane) {}

    uint64_t referenceCoordinateSystemId = 0;
    gp_Ax3  m_sketchPlane;

    const std::vector<SketchPrimitive>& getPrimitives() const { return m_primitiveRegistry.getItems(); }
    const std::vector<SketchConstraint>& getConstraints() const { return m_constraintRegistry.getItems(); }
    const std::vector<SketchPoint>& getPoints()const { return m_points.getItems(); }

    uint64_t    addPrimitive(SketchPrimitive primitive) { return m_primitiveRegistry.add(std::move(primitive)); }
    //uint64_t    addConstraint(SketchConstraint constraint) { return m_constraintRegistry.add(std::move(constraint)); }
    uint64_t    addConstraint(SketchConstraint constraint);
    void        loadPrimitive(SketchPrimitive primitive) { m_primitiveRegistry.load(std::move(primitive)); }
    void        loadConstraint(SketchConstraint constraint) { m_constraintRegistry.load(std::move(constraint)); }

    uint64_t        addLine ( gp_Pnt2d li_PntStart2d, gp_Pnt2d li_PntStop2d ){
        uint64_t u64_IdStart = addPoint ( li_PntStart2d );
        uint64_t u64_IdStop = addPoint ( li_PntStop2d );
        SketchLine  line( u64_IdStart, u64_IdStop );
        line.b_IsRef = false;
        return addPrimitive ( line);
    }
    uint64_t  addCircle ( gp_Pnt2d li_PntCenter2d, double radius){
        uint64_t u64_IdCenter = addPoint ( li_PntCenter2d );
        SketchCircle  circle( li_PntCenter2d, radius );
        circle.centerPointId = u64_IdCenter;
        return addPrimitive(circle);
    }


    bool        PointExists ( const gp_Pnt2d& li_Pnt2D, uint64_t &lo_PointId){
        const auto& items = m_points.getItems();

        double tolerance = 1e-6;
        auto it = std::find_if(items.begin(), items.end(), [&](const SketchPoint& point) {
            return li_Pnt2D.IsEqual( point.p2d, tolerance);
        });
        if (it != items.end()) {
            lo_PointId = it->id; // Récupération de l'ID associé
            return true;         // Point trouvé
        }
        return false; // Point non trouvé
    }

    uint64_t    addPoint (const gp_Pnt2d& li_Pnt2D) {
        uint64_t lid;
        if ( false == PointExists(li_Pnt2D,  lid)){
            SketchPoint l_point(li_Pnt2D);
            l_point.Update3D( m_sketchPlane);
            return m_points.add(l_point);
        }else{
            return lid;
        }
    }
    bool removePoint( uint64_t li_id){
        m_points.remove( li_id );
    }

    SketchPoint& GetPointById ( uint64_t li_id){
        SketchPoint* pt = m_points.findMutable(li_id);
        if (pt != nullptr) {
            return *pt; // On déréférence pour renvoyer une référence SketchPoint&
        }
        // 2. Gestion d'erreur si l'ID n'existe pas (par exemple, lancer une exception)
        throw std::runtime_error("Point ID non trouvé dans le registre !");
    }

    SketchPrimitive* GetPrimitiveMutable(uint64_t id) {    return m_primitiveRegistry.findMutable(id); }

    void removePrimitive(uint64_t idASupprimer) {
        m_primitiveRegistry.remove(idASupprimer);
        auto& constraints = m_constraintRegistry.getItemsMutable();
        constraints.erase(
            std::remove_if(constraints.begin(), constraints.end(), [idASupprimer](const SketchConstraint& c) {
                return c.ref1.primitiveId == idASupprimer || c.ref2.primitiveId == idASupprimer;
                }),
            constraints.end()
        );
    }

    // Fonction de synchronisation 2D -> 3D
    void recomputeGeometry3D() const {
        for (const auto& point : getPoints()) {
            point.Update3D(m_sketchPlane);
        }
        /*
        // On parcourt le registre des primitives
        for (const auto& primitive : getPrimitives()) {
            // Utilisation de std::visit pour modifier les caches 3D des primitives mutables
            std::visit([&](auto& concretePrim) {
                using T = std::decay_t<decltype(concretePrim)>;

                if constexpr (std::is_same_v<T, SketchLine>) {
                    // Utilisation de ElSLib::Value pour projeter la 2D locale en 3D absolue
                    concretePrim.start.Update3D (m_sketchPlane);
                    concretePrim.stop.Update3D (m_sketchPlane);
                }
                else if constexpr (std::is_same_v<T, SketchCircle>) {
                    concretePrim.center.Update3D (m_sketchPlane);       // Pour le cercle, on projette uniquement son centre 2D en 3D
                }
            }, const_cast<SketchPrimitive&>(primitive)); // Le const_cast permet de mettre à jour le cache interne d'affichage
        }
        */
    }

    void removeConstraint(uint64_t id) { m_constraintRegistry.remove(id); }
    TopoDS_Shape evaluate(const CAD_Document& doc) const;

    void        Contours_ProcessAndValidate();


};

enum class EBooleanOp { None, Union, Substract, Intersect };
std::string EBooleanOpToString(EBooleanOp type);

struct BooleanParams {
    uint64_t    ToolId;
    uint64_t    TargetId;
    EBooleanOp boolOp = EBooleanOp::None;

    // Obligatoire pour le std::visit
    TopoDS_Shape evaluate(const CAD_Document& doc) const { return TopoDS_Shape(); }
};

struct ExtrudeParams {
    double      start, end;
    EBooleanOp  EboolOp = EBooleanOp::None;
    uint64_t    SketchId;
    gp_Vec      vecteurExtrusion;

    // Declaration seule
    TopoDS_Shape evaluate(const CAD_Document& doc) const;
};

struct CoordinateSystem {
    gp_Ax2 AxisSystem;
    TopoDS_Shape evaluate(const CAD_Document& doc) const;
};

using OperationParams = std::variant<SketchParams, ExtrudeParams, CoordinateSystem, BooleanParams>;

// ==========================================
// 5. L'OPERATION DE L'ARBRE (FEATURE)
// ==========================================

class CadOperation : public Identifiable {
private:
    TopoDS_Shape    m_Topo_locale;          //forme isolée générée par l'opération
    TopoDS_Shape    m_Topo_resulting;       // état de la pièce après l'application de l'opération, transmise a l'étape suivante
    std::unordered_map<int, int> m_TableLocal;
    std::string     m_customName;
    OperationParams m_params;
    bool            m_Topo_locale_changed = false;

    float           m_opacity = 1.0f;       // 1.0 = opaque, 0.2 = translucide
    bool            m_isVisible = true;     // Permet de masquer un élément
    float           m_color[3] = {0.75f, 0.75f, 0.75f}; // Couleur RVB par défaut


public:
    CadOperation(std::string name, OperationParams params)
        : m_customName(std::move(name)), m_params(std::move(params)) {
        m_opacity = 1.0f;
    }
    bool    hasLocaleTopoChanged() const { return m_Topo_locale_changed; }
    void    setLocaleTopoChanged(bool b_mod) { m_Topo_locale_changed = b_mod;}


    const   TopoDS_Shape&       getLocalTopo() const { return m_Topo_locale; }
            void                setLocalTopo(const TopoDS_Shape& topo) { m_Topo_locale = topo; }
    const   TopoDS_Shape&       getResultingTopo() const { return m_Topo_resulting; }
            void                setResultingTopo(const TopoDS_Shape& topo) { m_Topo_resulting = topo; }
    std::string getTypeName() const;


    float getOpacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = opacity; }

    bool isVisible() const { return m_isVisible; }
    void setVisible(bool visible) { m_isVisible = visible; }

    const float* getColor() const { return m_color; }
    void setColor(float r, float g, float b) { m_color[0] = r; m_color[1] = g; m_color[2] = b; }

    const std::string& getName() const { return m_customName; }
    void setName(const std::string& name) { m_customName = name; }
    const OperationParams&      getParams() const { return m_params; }
    OperationParams&            getParamsMutable() { return m_params; }
    const OperationParams&      getParamsConst() const { return m_params; }

    // La methode execute ne recalcule m_localTopo QUE si necessaire
    void execute(const CAD_Document& doc, bool forceRecalcul) {
        m_Topo_locale = std::visit([&doc](const auto& params) -> TopoDS_Shape {
            return params.evaluate(doc);
        }, m_params);
        setLocaleTopoChanged (true); //la topo locale a ete modifiee il faut recalculer la topo resulting
    }

    std::string getConstraintTypeString( const ConstraintType li_ConstType) const ;
    std::string getConstraintSubElementString( const ConstraintSubElement li_SubElmt) const ;



};

#endif // CAD_OPERATION_H