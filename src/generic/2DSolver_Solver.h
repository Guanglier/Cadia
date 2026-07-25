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

    // Définit le nombre total de scalaires à résoudre (ex: 4 pour deux points 2D)
    void setNumVariables(int numVars) {
        m_numVariables = numVars;
    }

    // Ajoute une contrainte au système (transfert de propriété)
    void addConstraint(std::unique_ptr<IConstraint2D> constraint) {
        m_constraints.push_back(std::move(constraint));
    }

    // Vide les contraintes pour reconstruire un système
    void clearConstraints() {
        m_constraints.clear();
    }



    bool solve(Eigen::VectorXd& X, int maxIterations = 50, double tolerance = 1e-6) ;

    /**
     * Calcule le nombre de degrés de liberté restants (DOF)
     * DOF = Nombre de variables - Rang de la Jacobienne
     */
    int calculateDegreesOfFreedom(const Eigen::VectorXd& X) const ;
};





