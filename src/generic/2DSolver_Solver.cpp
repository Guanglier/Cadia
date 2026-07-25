/**
 * @file 2DSolver_Solver.cpp
 * @brief Implémentation des méthodes de calcul et de résolution de la classe `Solver2D_Solver`.
 *
 * Ce fichier implémente l'algorithme d'optimisation non linéaire de Levenberg-Marquardt
 * utilisé pour converger vers un état géométrique valide. Il gère également le calcul
 * du rang de la matrice Jacobienne pour évaluer en temps réel les degrés de liberté (DOF)
 * du système d'esquisse.
 */

#include "2DSolver_Solver.h"
#include "Logger.h"




/**
 * @brief Calcule le nombre de degrés de liberté restants (DOF) du système.
 * @param X [Entrée] Vecteur d'état actuel des variables.
 * @return int Nombre de degrés de liberté (Variables totales - Rang de la Jacobienne).
 */
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



//
//  Résout le système de contraintes sur le vecteur d'état X (modifié in-place).
//  @param X VectorXd contenant les variables [x1, y1, x2, y2, ...]
//  @param maxIterations Nombre max d'itérations de Newton
//  @param tolerance Seuil de convergence pour l'erreur résiduelle
//  @return true si le système a convergé, false sinon
//
/**
 * @brief Résout le système d'équations de contraintes sur le vecteur d'état X par Levenberg-Marquardt.
 * @param X [Entrée/Sortie] Vecteur d'état contenant les variables [x1, y1, x2, y2, ...], mis à jour in-place.
 * @param maxIterations [Entrée] Nombre maximal d'itérations de Newton autorisées.
 * @param tolerance [Entrée] Seuil de tolérance de convergence pour l'erreur résiduelle globale.
 * @return bool True si le système a convergé sous le seuil de tolérance, false sinon.
 */
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

    //double damping = 1e-2; // Valeur de départ raisonnable
    double damping = m_currentDamping;

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
    LOG_DEBUG << "[Solver Debug] Nombre total de variables dans X : " << m_numVariables << std::endl;

    return (currentError < tolerance);
}

