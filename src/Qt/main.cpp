

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include "mainwindow.h"
#include <QImageReader>
#include <QDebug>

#include "Contours_tests.h"
#include "Logger.h"


#include <External/Eigen/Dense>
#include "2DSolver_Solver.h"

int test_solver() {
    std::cout << "==========================================" << std::endl;
    std::cout << "   TEST DU SOLVEUR DE CONTRAINTES 2D      " << std::endl;
    std::cout << "==========================================" << std::endl;

    // 1. Définition du vecteur d'état X (4 variables)
    // [0] = P1_x,  [1] = P1_y
    // [2] = P2_x,  [3] = P2_y
    Eigen::VectorXd X(4);

    // Positions initiales "gribouillées" à la souris :
    // P1 est en (10, 20)
    // P2 est en (50, 80) -> Distance initiale ~ 67.08, non horizontal (dy = 60)
    X << 10.0, 20.0, 50.0, 80.0;

    std::cout << "\n[ETAT INITIAL]" << std::endl;
    std::cout << "P1 : (" << X[0] << ", " << X[1] << ")" << std::endl;
    std::cout << "P2 : (" << X[2] << ", " << X[3] << ")" << std::endl;

    // 2. Initialisation du solveur
    Solver2D_Solver solver;
    solver.setNumVariables(4);

    // 3. Ajout des contraintes
    // Contrainte A : Distance P1-P2 = 100.0 (indices: X1=0, Y1=1, X2=2, Y2=3)
    solver.addConstraint(std::make_unique<ConstraintDistancePointPoint>(0, 1, 2, 3, 100.0));

    // Contrainte B : Horizontalité P1-P2 (indices: Y1=1, Y2=3)
    solver.addConstraint(std::make_unique<ConstraintHorizontal>(1, 3));

    // 4. Calcul des Degrés de Liberté (DOF) avant résolution
    int dofBefore = solver.calculateDegreesOfFreedom(X);
    std::cout << "\nDegres de liberte (DOF) disponibles : " << dofBefore << " (sur 4 variables)" << std::endl;

    // 5. Résolution du système
    bool success = solver.solve(X);

    // 6. Affichage des résultats
    std::cout << "\n[RESULTAT DU SOLVEUR]" << std::endl;
    if (success) {
        std::cout << "Status : CONVERGE avec succes !" << std::endl;
    } else {
        std::cout << "Status : ECHEC de convergence." << std::endl;
    }

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "P1 final : (" << X[0] << ", " << X[1] << ")" << std::endl;
    std::cout << "P2 final : (" << X[2] << ", " << X[3] << ")" << std::endl;

    // 7. Vérifications géométriques
    double dx = X[2] - X[0];
    double dy = X[3] - X[1];
    double distFinale = std::sqrt(dx * dx + dy * dy);

    std::cout << "\n[VERIFICATION DES CONTRAINTES]" << std::endl;
    std::cout << "Distance P1-P2 reelle : " << distFinale << " (Cible: 100.0)" << std::endl;
    std::cout << "Delta Y (P2_y - P1_y)  : " << dy << " (Cible: 0.0)" << std::endl;

    return 0;
}

void testEigenIntegration() {
    std::cout << "--- TEST EIGEN DANS CADIA++ ---" << std::endl;

    // 1. Creation d'une matrice 2x2 (comme une petite Jacobienne)
    Eigen::Matrix2d A;
    A << 1.0, 2.0,
        3.0, 4.0;

    // 2. Vector b (les erreurs de tes contraintes)
    Eigen::Vector2d b(5.0, 6.0);

    // 3. Resolution du systeme A * x = b via SVD (JacobiSVD)
    Eigen::Vector2d x = A.bdcSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);

    // 4. Verification de l'erreur residuelle: ||A*x - b||
    double error = (A * x - b).norm();

    std::cout << "Solution x:\n" << x << std::endl;
    std::cout << "Erreur residuelle : " << error << std::endl;

    if (error < 1e-9) {
        std::cout << "SUCCESS: Eigen est parfaitement integre et fonctionnel !" << std::endl;
    } else {
        std::cout << "WARNING: La resolution a produit une erreur inattendue." << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
}





//#define TESTS_CONTOURS
int main(int argc, char *argv[])
{

    //testEigenIntegration ();
    //test_solver ();
    //return -25864;

    //Logger::GetInstance().EnableFile("application.log");
    Logger::GetInstance().SetLevel(LogLevel::Error);
    LOG_INFO << "Application lancée.";




#ifdef TESTS_CONTOURS
    bool testsPassed = ContoursTests::RunAllTests();
    if (!testsPassed) {
        std::cerr << "Arrêt de l'application : Échec des tests géométriques critiques." << std::endl;
        return -1;
    }else{
        std::cout << "ContoursTests::RunAllTests : ok"<<std::endl;
    }
    return -1;  //dans tous les cas on stoppe poru les tests unitaires
#endif



    //qputenv("QT_DEBUG_PLUGINS", "1");
    QApplication a(argc, argv);
    //qDebug() << "Formats supportés :" << QImageReader::supportedImageFormats();


    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Cad_IA_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    // w.showMaximized();
    return QApplication::exec();
}
