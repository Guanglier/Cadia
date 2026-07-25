#pragma once
#include <External/Eigen/Dense>
#include <cmath>
#include <vector>
#include <memory>


// ============================================================================
// INTERFACE DE BASE
// ============================================================================
class IConstraint2D {
public:
    virtual ~IConstraint2D() = default;

    // Renvoie la valeur de l'erreur f(X). Vise 0.0 quand la contrainte est satisfaite.
    virtual double calcError(const Eigen::VectorXd& X) const = 0;

    // Remplissage analytique de la ligne correspondant a cette contrainte dans la Jacobienne
    virtual void fillJacobianRow(const Eigen::VectorXd& X, Eigen::Ref<Eigen::VectorXd> row) const = 0;
};


// ============================================================================
// 1. CONTRAINTES SIMPLES SUR POINTS / VALEURS
// ============================================================================

// --- Point Fixe / Ancre (Fixe X ou Y a une valeur absolue) ---
class ConstraintFixedValue : public IConstraint2D {
private:
    int m_idxVar;
    double m_targetValue;
public:
    ConstraintFixedValue(int idxVar, double targetValue)
        : m_idxVar(idxVar), m_targetValue(targetValue) {}

    double calcError(const Eigen::VectorXd& X) const override {
        return X[m_idxVar] - m_targetValue;
    }

    void fillJacobianRow(const Eigen::VectorXd&, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        row[m_idxVar] = 1.0;
    }
};

// --- Coïncidence de 2 coordonnées (ex: P1_x == P2_x ou P1_y == P2_y) ---
class ConstraintCoincident1D : public IConstraint2D {
private:
    int m_idxA, m_idxB;
public:
    ConstraintCoincident1D(int idxA, int idxB) : m_idxA(idxA), m_idxB(idxB) {}

    double calcError(const Eigen::VectorXd& X) const override {
        return X[m_idxA] - X[m_idxB];
    }

    void fillJacobianRow(const Eigen::VectorXd&, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        row[m_idxA] = 1.0;
        row[m_idxB] = -1.0;
    }
};

// --- Horizontalité entre 2 points (P1_y - P2_y = 0) ---
using ConstraintHorizontal = ConstraintCoincident1D;

// --- Verticalité entre 2 points (P1_x - P2_x = 0) ---
using ConstraintVertical = ConstraintCoincident1D;


// ============================================================================
// 2. CONTRAINTES DE DISTANCE & RAYON
// ============================================================================

// --- Distance Point à Point ---
class ConstraintDistancePointPoint : public IConstraint2D {
private:
    int m_idxX1, m_idxY1;
    int m_idxX2, m_idxY2;
    double m_targetDistance;
public:
    ConstraintDistancePointPoint(int idxX1, int idxY1, int idxX2, int idxY2, double targetDistance)
        : m_idxX1(idxX1), m_idxY1(idxY1), m_idxX2(idxX2), m_idxY2(idxY2), m_targetDistance(targetDistance) {}

    double calcError(const Eigen::VectorXd& X) const override {
        double dx = X[m_idxX2] - X[m_idxX1];
        double dy = X[m_idxY2] - X[m_idxY1];
        double dist = std::sqrt(dx * dx + dy * dy);
        // On évite la singularité si les deux points sont superposés
        return dist - m_targetDistance;
    }

    void fillJacobianRow(const Eigen::VectorXd& X, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        double dx = X[m_idxX2] - X[m_idxX1];
        double dy = X[m_idxY2] - X[m_idxY1];
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 1e-9) {
            row[m_idxX1] = -1.0;
            row[m_idxX2] = 1.0;
            return;
        }

        row[m_idxX1] = -dx / dist;
        row[m_idxY1] = -dy / dist;
        row[m_idxX2] =  dx / dist;
        row[m_idxY2] =  dy / dist;
    }
};

// --- Rayon imposé pour un Cercle (R - R_cible = 0) ---
class ConstraintRadius : public IConstraint2D {
private:
    int m_idxR;
    double m_targetRadius;
public:
    ConstraintRadius(int idxR, double targetRadius)
        : m_idxR(idxR), m_targetRadius(targetRadius) {}

    double calcError(const Eigen::VectorXd& X) const override {
        return X[m_idxR] - m_targetRadius;
    }

    void fillJacobianRow(const Eigen::VectorXd&, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        row[m_idxR] = 1.0;
    }
};


// ============================================================================
// 3. CONTRAINTES ORIENTATION (PARALLÉLISME & PERPENDICULARITÉ)
// ============================================================================

// --- Parallélisme entre deux segments L1(P1, P2) et L2(P3, P4) ---
// Produit vectoriel 2D des deux vecteurs directeurs : V1 ^ V2 = dx1*dy2 - dy1*dx2 = 0
class ConstraintParallel : public IConstraint2D {
private:
    int m_x1, m_y1, m_x2, m_y2; // Segment 1
    int m_x3, m_y3, m_x4, m_y4; // Segment 2
public:
    ConstraintParallel(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2),
        m_x3(x3), m_y3(y3), m_x4(x4), m_y4(y4) {}

    double calcError(const Eigen::VectorXd& X) const override {
        double dx1 = X[m_x2] - X[m_x1];
        double dy1 = X[m_y2] - X[m_y1];
        double dx2 = X[m_x4] - X[m_x3];
        double dy2 = X[m_y4] - X[m_y3];
        return (dx1 * dy2) - (dy1 * dx2);
    }

    void fillJacobianRow(const Eigen::VectorXd& X, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        double dx1 = X[m_x2] - X[m_x1];
        double dy1 = X[m_y2] - X[m_y1];
        double dx2 = X[m_x4] - X[m_x3];
        double dy2 = X[m_y4] - X[m_y3];

        row[m_x1] = -dy2;
        row[m_y1] =  dx2;
        row[m_x2] =  dy2;
        row[m_y2] = -dx2;

        row[m_x3] =  dy1;
        row[m_y3] = -dx1;
        row[m_x4] = -dy1;
        row[m_y4] =  dx1;
    }
};

// --- Perpendicularité entre deux segments L1(P1, P2) et L2(P3, P4) ---
// Produit scalaire des deux vecteurs directeurs : V1 . V2 = dx1*dx2 + dy1*dy2 = 0
class ConstraintPerpendicular : public IConstraint2D {
private:
    int m_x1, m_y1, m_x2, m_y2; // Segment 1
    int m_x3, m_y3, m_x4, m_y4; // Segment 2
public:
    ConstraintPerpendicular(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2),
        m_x3(x3), m_y3(y3), m_x4(x4), m_y4(y4) {}

    double calcError(const Eigen::VectorXd& X) const override {
        double dx1 = X[m_x2] - X[m_x1];
        double dy1 = X[m_y2] - X[m_y1];
        double dx2 = X[m_x4] - X[m_x3];
        double dy2 = X[m_y4] - X[m_y3];
        return (dx1 * dx2) + (dy1 * dy2);
    }

    void fillJacobianRow(const Eigen::VectorXd& X, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        double dx1 = X[m_x2] - X[m_x1];
        double dy1 = X[m_y2] - X[m_y1];
        double dx2 = X[m_x4] - X[m_x3];
        double dy2 = X[m_y4] - X[m_y3];

        row[m_x1] = -dx2;
        row[m_y1] = -dy2;
        row[m_x2] =  dx2;
        row[m_y2] =  dy2;

        row[m_x3] = -dx1;
        row[m_y3] = -dy1;
        row[m_x4] =  dx1;
        row[m_y4] =  dy1;
    }
};


// ============================================================================
// 4. TANGENCE
// ============================================================================

// --- Tangence Ligne L1(P1, P2) et Cercle (Center C, Rayon R) ---
// La distance orthogonale du centre C à la droite (P1,P2) doit égaler R.
// Distance = |(X_c - X1)*dy - (Y_c - Y1)*dx| / ||V|| = R
class ConstraintTangentLineCircle : public IConstraint2D {
private:
    int m_x1, m_y1, m_x2, m_y2; // Ligne
    int m_cx, m_cy, m_r;        // Cercle
public:
    ConstraintTangentLineCircle(int x1, int y1, int x2, int y2, int cx, int cy, int r)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2), m_cx(cx), m_cy(cy), m_r(r) {}

    double calcError(const Eigen::VectorXd& X) const override {
        double dx = X[m_x2] - X[m_x1];
        double dy = X[m_y2] - X[m_y1];
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-9) return 0.0;

        // Numérateur de la distance point-droite
        double num = (X[m_cx] - X[m_x1]) * dy - (X[m_cy] - X[m_y1]) * dx;

        return std::abs(num) / len - X[m_r];
    }

    void fillJacobianRow(const Eigen::VectorXd& X, Eigen::Ref<Eigen::VectorXd> row) const override {
        row.setZero();
        // Pour la Jacobienne de la tangence, l'utilisation de différences finies
        // ou d'une approximation locale simplifie grandement l'implémentation sans perdre la convergence :
        const double h = 1e-7;
        Eigen::VectorXd X_plus = X;

        // Approximation numérique rapide par gradient pour cette contrainte composite
        int indices[] = {m_x1, m_y1, m_x2, m_y2, m_cx, m_cy, m_r};
        for (int idx : indices) {
            X_plus[idx] += h;
            double errPlus = calcError(X_plus);
            X_plus[idx] = X[idx] - h;
            double errMinus = calcError(X_plus);
            X_plus[idx] = X[idx];

            row[idx] = (errPlus - errMinus) / (2.0 * h);
        }
    }
};




