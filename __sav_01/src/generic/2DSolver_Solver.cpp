


#include "2DSolver_Solver.h"

#include "Logger.h"




int Solver2D_Solver::calculateDegreesOfFreedom(const Eigen::VectorXd& X) const {
    if (m_numVariables == 0) return 0;
    if (m_constraints.empty()) return m_numVariables;

    const int numConstraints = static_cast<int>(m_constraints.size());
    Eigen::MatrixXd J(numConstraints, m_numVariables);
    Eigen::VectorXd rowTemp(m_numVariables);

    for (int i = 0; i < numConstraints; ++i) {
        m_constraints[i]->fillJacobianRow(X, rowTemp);
        J.row(i) = rowTemp;
    }

    // Le rang représente le nombre de contraintes réellement indépendantes
    Eigen::FullPivLU<Eigen::MatrixXd> luDecomp(J);
    int rank = static_cast<int>(luDecomp.rank());

    return m_numVariables - rank;
}


// à faire : ajustement automatique de lambda en fonction de la nouvelle erreur
// si elle diminue on diminue lambda et on accepte le step, si elle augmente
// on augmente lambda et on jette le step
/**
     * Résout le système de contraintes sur le vecteur d'état X (modifié in-place).
     * @param X VectorXd contenant les variables [x1, y1, x2, y2, ...]
     * @param maxIterations Nombre max d'itérations de Newton
     * @param tolerance Seuil de convergence pour l'erreur résiduelle
     * @return true si le système a convergé, false sinon
     */

/*
bool Solver2D_Solver::solve(Eigen::VectorXd& X, int maxIterations,  double tolerance) {
    if (m_numVariables == 0 || m_constraints.empty()) {
        return true; // Rien à résoudre
    }

    const int numConstraints = static_cast<int>(m_constraints.size());

    // Matrices de travail
    Eigen::VectorXd F(numConstraints);                     // Vecteur d'erreurs
    Eigen::MatrixXd J(numConstraints, m_numVariables);    // Matrice Jacobienne
    Eigen::VectorXd rowTemp(m_numVariables);

    double damping = 1e-3; // Facteur d'amortissement (Levenberg-Marquardt)

    for (int iter = 0; iter < maxIterations; ++iter) {

        std::cout<<"iter: "<< iter <<std::endl;
        // 1. Évaluation des erreurs F(X) et construction de la Jacobienne J(X)
        for (int i = 0; i < numConstraints; ++i) {
            F[i] = m_constraints[i]->calcError(X);
            m_constraints[i]->fillJacobianRow(X, rowTemp);
            J.row(i) = rowTemp;
        }

        // 2. Vérification de la convergence : norme RMS de l'erreur
        double currentError = F.norm();
        if (currentError < tolerance) {
            return true; // Systèmes stabilisé !
        }

        // 3. Formule de Levenberg-Marquardt : (J^T * J + damping * I) * deltaX = -J^T * F
        Eigen::MatrixXd JT = J.transpose();
        Eigen::MatrixXd A = JT * J;

        // Ajout du facteur de régularisation sur la diagonale
        A.diagonal().array() += damping;

        Eigen::VectorXd b = -JT * F;

        // 4. Résolution du système linéaire via SVD (robuste aux matrices singulières / sous-contraintes)
        Eigen::VectorXd deltaX = A.bdcSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);

        // 5. Mise à jour de l'état
        X += deltaX;

        // Si le pas devient insignifiant, on s'arrête
        if (deltaX.norm() < 1e-12) {
            break;
        }
    }

    // Vérification finale après épuisement des itérations
    for (int i = 0; i < numConstraints; ++i) {
        F[i] = m_constraints[i]->calcError(X);
    }
    return (F.norm() < tolerance);
}
*/



// proto avec ajustement automatique du lambda

bool Solver2D_Solver::solve(Eigen::VectorXd& X, int maxIterations, double tolerance) {

    LOG_DEBUG <<"Solver2D_Solver::solve" << std::endl;

    if (m_numVariables == 0 || m_constraints.empty()) {
        LOG_WARN << "\tFIN : if (m_numVariables == 0 || m_constraints.empty())" << std::endl;
        return true;
    }

    const int numConstraints = static_cast<int>(m_constraints.size());

    Eigen::VectorXd F(numConstraints);
    Eigen::MatrixXd J(numConstraints, m_numVariables);
    Eigen::VectorXd rowTemp(m_numVariables);

    // 1. Évaluation initiale des erreurs
    for (int i = 0; i < numConstraints; ++i) {
        F[i] = m_constraints[i]->calcError(X);
    }
    double currentError = F.norm();
    if (currentError < tolerance) return true;

    double damping = 1e-2; // Valeur de départ raisonnable

    for (int iter = 0; iter < maxIterations; ++iter) {

        // Construction de la Jacobienne pour l'état X actuel
        for (int i = 0; i < numConstraints; ++i) {
            m_constraints[i]->fillJacobianRow(X, rowTemp);
            J.row(i) = rowTemp;
        }

        Eigen::MatrixXd JT = J.transpose();
        Eigen::MatrixXd A_base = JT * J;
        Eigen::VectorXd b = -JT * F;

        bool stepAccepted = false;
        Eigen::VectorXd X_new;
        double newError = currentError;

        // Boucle d'essai de Levenberg-Marquardt (on ajuste damping jusqu'à ce que l'erreur baisse)
        for (int innerIter = 0; innerIter < 10; ++innerIter) {

            LOG_DEBUG <<"\t\titer: "<< iter << " innerIter=" <<  innerIter << " damping=" << damping<<std::endl;

            // Construction de la matrice amortie
            Eigen::MatrixXd A = A_base;
            A.diagonal().array() += damping;

            // Résolution
            Eigen::VectorXd deltaX = A.bdcSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);

            if (deltaX.norm() < 1e-12) {
                LOG_DEBUG << "\tERTURN : if (deltaX.norm() < 1e-12) : deltaX.norm = " << deltaX.norm() << " RETURN" << std::endl;
                return currentError < tolerance; // Pas trop petit, on arrête
            }

            // Test du pas
            X_new = X + deltaX;

            // Calcul de l'erreur avec ce nouveau pas
            Eigen::VectorXd F_new(numConstraints);
            for (int i = 0; i < numConstraints; ++i) {
                F_new[i] = m_constraints[i]->calcError(X_new);
            }
            newError = F_new.norm();

            // Si l'erreur diminue, le pas est validé !
            if (newError < currentError) {
                X = X_new;
                F = F_new;
                currentError = newError;
                damping = std::max(1e-7, damping * 0.1); // On réduit l'amortissement pour accélérer la prochaine fois
                stepAccepted = true;
                break;
            } else {
                // Échec : le pas empire les choses, on augmente l'amortissement et on réessaie
                damping *= 10.0;
            }
            LOG_DEBUG << "\tError=" << newError << std::endl;
        }

        // Si après plusieurs essais de damping le pas n'a pas pu être validé
        if (!stepAccepted) {
            LOG_WARN << "\tWARN : le solver n'a pas converge" << std::endl;
            break; // Le solveur bloque, on s'arrête là
        }

        if (currentError < tolerance) {
            LOG_DEBUG <<"\tFIN sur erreur acceptable" << std::endl;
            return true;
        }
    }

    LOG_DEBUG <<"\tFIN sur iter max" << std::endl;
    return (currentError < tolerance);
}





