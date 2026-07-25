/**
 * @file 2DSolver_Solver.h
 * @brief Solveur numérique 2D basé sur la méthode de Levenberg-Marquardt.
 *
 * Ce fichier définit la classe principale `Solver2D_Solver` qui gère un système
 * d'équations de contraintes géométriques et un vecteur d'état vectoriel.
 * Il utilise une approche itérative (Jacobienne et moindres carrés amortis) pour
 * ajuster les coordonnées et satisfaire l'ensemble des contraintes appliquées
 * à une esquisse 2D.
 *
 * Utilisation :
 * 1. Instancier le solveur : `Solver2D_Solver solver;`
 * 2. Définir le nombre de variables scalaires : `solver.setNumVariables(n);`
 * 3. Ajouter les contraintes géométriques via : `solver.addConstraint(...);`
 * 4. Lancer la résolution sur un vecteur d'état `Eigen::VectorXd X` :
 *    `bool success = solver.solve(X);`
 */

#pragma once

#include "2DSolver_Constraints.h"
#include <External/Eigen/Dense>
#include <vector>
#include <memory>
#include <iostream>

class Solver2D_Solver {
private:
    int m_numVariables = 0;
    std::vector<std::unique_ptr<IConstraint2D>> m_constraints;
    double m_currentDamping = 1e-2;

public:
    Solver2D_Solver() = default;

    /**
     * @brief Définit le nombre total de scalaires à résoudre.
     * @param numVars [Entrée] Nombre total de variables (ex: 4 pour deux points 2D).
     * @return void
     */
    // Définit le nombre total de scalaires à résoudre (ex: 4 pour deux points 2D)
    void setNumVariables(int numVars) {
        m_numVariables = numVars;
    }

    /**
     * @brief Ajoute une contrainte au système avec transfert de propriété.
     * @param constraint [Entrée] Pointeur intelligent unique vers la contrainte IConstraint2D à ajouter.
     * @return void
     */
    // Ajoute une contrainte au système (transfert de propriété)
    void addConstraint(std::unique_ptr<IConstraint2D> constraint) {
        m_constraints.push_back(std::move(constraint));
    }

    /**
     * @brief Vide l'ensemble des contraintes pour reconstruire un nouveau système.
     * @return void
     */
    // Vide les contraintes pour reconstruire un système
    void clearConstraints() {
        m_constraints.clear();
    }


    /**
     * @brief Résout le système d'équations non linéaires par la méthode de Levenberg-Marquardt.
     * @param X [Entrée/Sortie] Vecteur d'état contenant les variables modifiées in-place après convergence.
     * @param maxIterations [Entrée] Nombre maximal d'itérations autorisées (défaut : 50).
     * @param tolerance [Entrée] Seuil de tolérance de l'erreur résiduelle (défaut : 1e-6).
     * @return bool True si le système a convergé avec succès, false sinon.
     */
    bool solve(Eigen::VectorXd& X, int maxIterations = 50, double tolerance = 1e-6) ;

    /**
     * Calcule le nombre de degrés de liberté restants (DOF)
     * DOF = Nombre de variables - Rang de la Jacobienne
     */
    /**
     * @brief Calcule le nombre de degrés de liberté restants (DOF) du système.
     * @param X [Entrée] Vecteur d'état actuel des variables.
     * @return int Nombre de degrés de liberté (DOF = Nombre de variables - Rang de la Jacobienne).
     */
    int calculateDegreesOfFreedom(const Eigen::VectorXd& X) const ;
};



