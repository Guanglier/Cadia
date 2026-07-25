/**
 * @file 2DSolver_Constraints.h
 * @brief Définition des différentes contraintes géométriques 2D.
 *
 * Ce fichier implémente l'interface de base `IConstraint2D` ainsi que toutes
 * les classes dérivées représentant des contraintes géométriques spécifiques
 * (points fixes, coïncidences, distances, rayons, parallélisme, perpendicularité, tangence).
 * Chaque contrainte est capable de calculer sa propre erreur par rapport à l'état
 * actuel et de remplir sa ligne correspondante dans la matrice Jacobienne.
 *
 * Utilisation :
 * - Instancier la contrainte souhaitée en passant les indices des variables du vecteur
 *   d'état ou les valeurs cibles (ex: `std::make_unique<ConstraintDistancePointPoint>(...)`).
 * - Intégrer la contrainte dans le solveur via `addConstraint()`.
 */

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

    /**
     * @brief Renvoie la valeur de l'erreur f(X). Vise 0.0 quand la contrainte est satisfaite.
     * @param X [Entrée] Vecteur d'état actuel des variables.
     * @return double Valeur de l'erreur calculée.
     */
    // Renvoie la valeur de l'erreur f(X). Vise 0.0 quand la contrainte est satisfaite.
    virtual double calcError(const Eigen::VectorXd& X) const = 0;

    /**
     * @brief Remplissage analytique de la ligne correspondant a cette contrainte dans la Jacobienne.
     * @param X [Entrée] Vecteur d'état actuel des variables.
     * @param row [Sortie] Référence vers la ligne de la matrice Jacobienne à remplir.
     * @return void
     */
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
    /**
     * @brief Constructeur de la contrainte de valeur fixe.
     * @param idxVar [Entrée] Index de la variable dans le vecteur d'état.
     * @param targetValue [Entrée] Valeur cible absolue à respecter.
     */
    ConstraintFixedValue(int idxVar, double targetValue)
        : m_idxVar(idxVar), m_targetValue(targetValue) {}

    /**
     * @brief Calcule l'erreur de la valeur fixe.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Erreur (X[m_idxVar] - m_targetValue).
     */
    double calcError(const Eigen::VectorXd& X) const override {
        return X[m_idxVar] - m_targetValue;
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour la valeur fixe.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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
    /**
     * @brief Constructeur de la coïncidence 1D.
     * @param idxA [Entrée] Index de la première variable.
     * @param idxB [Entrée] Index de la seconde variable.
     */
    ConstraintCoincident1D(int idxA, int idxB) : m_idxA(idxA), m_idxB(idxB) {}

    /**
     * @brief Calcule l'erreur de coïncidence 1D.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Erreur (X[m_idxA] - X[m_idxB]).
     */
    double calcError(const Eigen::VectorXd& X) const override {
        return X[m_idxA] - X[m_idxB];
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour la coïncidence 1D.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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


using ConstraintHorizontalAlign = ConstraintCoincident1D;

using ConstraintVerticalAlign = ConstraintCoincident1D;



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
    /**
     * @brief Constructeur de la contrainte de distance entre deux points.
     * @param idxX1 [Entrée] Index X du premier point.
     * @param idxY1 [Entrée] Index Y du premier point.
     * @param idxX2 [Entrée] Index X du second point.
     * @param idxY2 [Entrée] Index Y du second point.
     * @param targetDistance [Entrée] Distance cible à maintenir.
     */
    ConstraintDistancePointPoint(int idxX1, int idxY1, int idxX2, int idxY2, double targetDistance)
        : m_idxX1(idxX1), m_idxY1(idxY1), m_idxX2(idxX2), m_idxY2(idxY2), m_targetDistance(targetDistance) {}

    /**
     * @brief Calcule l'erreur de distance point à point.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Écart entre la distance actuelle et la distance cible.
     */
    double calcError(const Eigen::VectorXd& X) const override {
        double dx = X[m_idxX2] - X[m_idxX1];
        double dy = X[m_idxY2] - X[m_idxY1];
        double dist = std::sqrt(dx * dx + dy * dy);
        // On évite la singularité si les deux points sont superposés
        return dist - m_targetDistance;
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour la distance point à point.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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
    /**
     * @brief Constructeur de la contrainte de rayon.
     * @param idxR [Entrée] Index de la variable de rayon dans le vecteur d'état.
     * @param targetRadius [Entrée] Valeur cible du rayon.
     */
    ConstraintRadius(int idxR, double targetRadius)
        : m_idxR(idxR), m_targetRadius(targetRadius) {}

    /**
     * @brief Calcule l'erreur de rayon.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Erreur (X[m_idxR] - m_targetRadius).
     */
    double calcError(const Eigen::VectorXd& X) const override {
        return X[m_idxR] - m_targetRadius;
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour le rayon.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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
    /**
     * @brief Constructeur de la contrainte de parallélisme.
     * @param x1 [Entrée] Index X du premier point du segment 1.
     * @param y1 [Entrée] Index Y du premier point du segment 1.
     * @param x2 [Entrée] Index X du second point du segment 1.
     * @param y2 [Entrée] Index Y du second point du segment 1.
     * @param x3 [Entrée] Index X du premier point du segment 2.
     * @param y3 [Entrée] Index Y du premier point du segment 2.
     * @param x4 [Entrée] Index X du second point du segment 2.
     * @param y4 [Entrée] Index Y du second point du segment 2.
     */
    ConstraintParallel(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2),
        m_x3(x3), m_y3(y3), m_x4(x4), m_y4(y4) {}

    /**
     * @brief Calcule l'erreur de parallélisme via le produit vectoriel 2D.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Valeur de l'erreur (dx1 * dy2 - dy1 * dx2).
     */
    double calcError(const Eigen::VectorXd& X) const override {
        double dx1 = X[m_x2] - X[m_x1];
        double dy1 = X[m_y2] - X[m_y1];
        double dx2 = X[m_x4] - X[m_x3];
        double dy2 = X[m_y4] - X[m_y3];
        return (dx1 * dy2) - (dy1 * dx2);
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour le parallélisme.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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
    /**
     * @brief Constructeur de la contrainte de perpendicularité.
     * @param x1 [Entrée] Index X du premier point du segment 1.
     * @param y1 [Entrée] Index Y du premier point du segment 1.
     * @param x2 [Entrée] Index X du second point du segment 1.
     * @param y2 [Entrée] Index Y du second point du segment 1.
     * @param x3 [Entrée] Index X du premier point du segment 2.
     * @param y3 [Entrée] Index Y du premier point du segment 2.
     * @param x4 [Entrée] Index X du second point du segment 2.
     * @param y4 [Entrée] Index Y du second point du segment 2.
     */
    ConstraintPerpendicular(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2),
        m_x3(x3), m_y3(y3), m_x4(x4), m_y4(y4) {}

    /**
     * @brief Calcule l'erreur de perpendicularité via le produit scalaire.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Valeur de l'erreur (dx1 * dx2 + dy1 * dy2).
     */
    double calcError(const Eigen::VectorXd& X) const override {
        double dx1 = X[m_x2] - X[m_x1];
        double dy1 = X[m_y2] - X[m_y1];
        double dx2 = X[m_x4] - X[m_x3];
        double dy2 = X[m_y4] - X[m_y3];
        return (dx1 * dx2) + (dy1 * dy2);
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour la perpendicularité.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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
    /**
     * @brief Constructeur de la contrainte de tangence entre une ligne et un cercle.
     * @param x1 [Entrée] Index X du début de la ligne.
     * @param y1 [Entrée] Index Y du début de la ligne.
     * @param x2 [Entrée] Index X de la fin de la ligne.
     * @param y2 [Entrée] Index Y de la fin de la ligne.
     * @param cx [Entrée] Index X du centre du cercle.
     * @param cy [Entrée] Index Y du centre du cercle.
     * @param r [Entrée] Index du rayon du cercle.
     */
    ConstraintTangentLineCircle(int x1, int y1, int x2, int y2, int cx, int cy, int r)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2), m_cx(cx), m_cy(cy), m_r(r) {}

    /**
     * @brief Calcule l'erreur de tangence ligne-cercle.
     * @param X [Entrée] Vecteur d'état actuel.
     * @return double Écart entre la distance centre-droite et le rayon.
     */
    double calcError(const Eigen::VectorXd& X) const override {
        double dx = X[m_x2] - X[m_x1];
        double dy = X[m_y2] - X[m_y1];
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-9) return 0.0;

        // Numérateur de la distance point-droite
        double num = (X[m_cx] - X[m_x1]) * dy - (X[m_cy] - X[m_y1]) * dx;

        return std::abs(num) / len - X[m_r];
    }

    /**
     * @brief Remplit la ligne de la Jacobienne pour la tangence par différences finies.
     * @param X [Entrée] Vecteur d'état actuel.
     * @param row [Sortie] Ligne de la Jacobienne à remplir.
     * @return void
     */
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



