

#include "Contours.h"
#include "Contours_tests.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>


namespace ContoursTests {

    // Petite fonction utilitaire pour générer un segment de ligne rapidement
    ContoursElement CreateTestLine(uint64_t id, double x1, double y1, double x2, double y2) {
        ContoursElement el;
        el.primitiveId = id;
        el.type = ContoursPrimitiveType::Line;
        el.StartCpy2D.SetCoord(x1, y1);
        el.StopCpy2D.SetCoord(x2, y2);
        return el;
    }

    // Petite fonction utilitaire pour générer un cercle complet
    ContoursElement CreateTestCircle(uint64_t id, double cx, double cy, double radius) {
        ContoursElement el;
        el.primitiveId = id;
        el.type = ContoursPrimitiveType::Circle;
        el.CenterCpy2D.SetCoord(cx, cy);
        el.Radius = radius;
        return el;
    }

    //─────────────────────────────────────────────────────────────────────
    // CAS 1 : Une plaque carrée de 100x100 avec un trou circulaire au centre
    // Validation attendue : hasErrors = false, validContours.size() == 2
    // Outer = Anti-Horaire (Aire > 0), Inner = Horaire (Aire < 0)
    //─────────────────────────────────────────────────────────────────────
    bool Test_ValidSquareWithHole() {
        std::cout << "\n[TEST 1] : Plaque carrée 100x100 avec trou circulaire (Validation stricte)" << std::endl;

        std::vector<ContoursElement> primitives;

        // Carré externe : volontairement injecté à l'envers (sens horaire) et dans le désordre
        // ID 3 -> ID 1 -> ID 4 -> ID 2
        primitives.push_back(CreateTestLine(3, 100, 100, 0, 100));
        primitives.push_back(CreateTestLine(1, 0, 0, 100, 0));
        primitives.push_back(CreateTestLine(4, 0, 100, 0, 0));
        primitives.push_back(CreateTestLine(2, 100, 0, 100, 100));

        // Trou circulaire au centre (ID 5)
        primitives.push_back(CreateTestCircle(5, 50, 50, 15));

        // Exécution du moteur
        ContoursTopologyResult result = ContoursEngine::Process(primitives);

        // Affichage pour le log visuel
        ContoursEngine::Contours_DisplayContours(result);

        // --- BATTERIE DE TESTS DE SÉCURITÉ ---
        bool testPassed = true;
        const double EPSILON = 1e-4;

        // 1. Validation globale de la structure
        if (result.hasErrors) {
            std::cout << "  [FAIL] Le moteur a renvoyé une erreur globale inattendue : " << result.errorMessage << std::endl;
            testPassed = false;
        }
        if (result.validContours.size() != 2) {
            std::cout << "  [FAIL] Nombre de contours incorrect. Attendu: 2, Obtenu: " << result.validContours.size() << std::endl;
            testPassed = false;
        }

        if (!testPassed) {
            std::cout << ">> TEST 1 FAILED (Erreurs structurelles fondamentales) !" << std::endl;
            return false;
        }

        // 2. Validation approfondie du Contour Externe (Index 0 - car trié par surface décroissante)
        const Contour& outer = result.validContours[0];
        std::cout << "  -> Verification du contour externe..." << std::endl;

        if (outer.isInternal) {
            std::cout << "  [FAIL] L'enveloppe externe est marquée comme interne (trou) !" << std::endl;
            testPassed = false;
        }
        if (!outer.isClosed) {
            std::cout << "  [FAIL] L'enveloppe externe est marquée comme ouverte !" << std::endl;
            testPassed = false;
        }
        // Aire attendue pour un carré 100x100 en sens anti-horaire = +10000.0
        if (std::abs(outer.Aire - 10000.0) > EPSILON) {
            std::cout << "  [FAIL] L'aire du carre est incorrecte ou mal orientee. Attendue: 10000.0, Obtenue: " << outer.Aire << std::endl;
            testPassed = false;
        }
        if (outer.elements.size() != 4) {
            std::cout << "  [FAIL] Le carre externe ne contient pas 4 elements mais " << outer.elements.size() << std::endl;
            testPassed = false;
        } else {
            // Contrôle strict du chaînage des points géométriques et de l'ordre topologique des primitives (ID)
            // Le chaîneur doit reconstruire le flux continu. Vérifions la continuité géométrique Start/Stop
            for (size_t i = 0; i < outer.elements.size(); ++i) {
                const auto& curr = outer.elements[i];
                const auto& next = outer.elements[(i + 1) % outer.elements.size()]; // Boucle de fermeture

                if (!curr.StopCpy2D.IsEqual(next.StartCpy2D, EPSILON)) {
                    std::cout << "  [FAIL] Rupture géométrique detectee entre l'element " << i << " et " << (i+1) << std::endl;
                    testPassed = false;
                }
            }
        }

        // 3. Validation approfondie du Contour Interne (Index 1 - Le trou)
        const Contour& inner = result.validContours[1];
        std::cout << "  -> Verification du trou circulaire..." << std::endl;

        if (!inner.isInternal) {
            std::cout << "  [FAIL] Le trou central est marqué comme enveloppe externe !" << std::endl;
            testPassed = false;
        }
        if (inner.elements.size() != 1 || inner.elements[0].type != ContoursPrimitiveType::Circle) {
            std::cout << "  [FAIL] Le trou interne doit comporter exactement 1 primitive de type Cercle !" << std::endl;
            testPassed = false;
        } else {
            // Aire d'un cercle R=15 attendue en sens horaire (négatif) = -pi * 15² ≈ -706.8583
            double aireCercleAttendue = -M_PI * 15.0 * 15.0;
            if (std::abs(inner.Aire - aireCercleAttendue) > EPSILON) {
                std::cout << "  [FAIL] L'aire du trou est incorrecte. Attendue: " << aireCercleAttendue << ", Obtenue: " << inner.Aire << std::endl;
                testPassed = false;
            }
            if (inner.elements[0].primitiveId != 5) {
                std::cout << "  [FAIL] L'ID de l'element circulaire a ete altere. Attendu: 5, Obtenu: " << inner.elements[0].primitiveId << std::endl;
                testPassed = false;
            }
        }

        if (testPassed) {
            std::cout << ">> TEST 1 PASSED !" << std::endl;
        } else {
            std::cout << ">> TEST 1 FAILED !" << std::endl;
        }
        return testPassed;
    }


    //─────────────────────────────────────────────────────────────────────
    //  Le contour externe est désormais un grand cercle (trié en premier grâce à sa surface),
    //  le contour interne (le trou) est un rectangle formé de 4 segments de lignes injectés
    //  volontairement de manière désordonnée et inversée.
    //─────────────────────────────────────────────────────────────────────
    bool Test_ValidCircleWithRectangleHole() {
        std::cout << "\n[TEST 2] : Disque circulaire (externe) avec trou rectangulaire (Validation stricte)" << std::endl;

        std::vector<ContoursElement> primitives;

        // 1. Enveloppe externe : Un grand cercle (ID 1, Centre: X=0, Y=0, Rayon=50)
        primitives.push_back(CreateTestCircle(1, 0.0, 0.0, 50.0));

        // 2. Trou interne : Rectangle 40x20 centré en (0,0) -> X de -20 à 20, Y de -10 à 10
        // Injecté volontairement dans le désordre et avec des orientations de segments inversées
        primitives.push_back(CreateTestLine(4, -20.0, -10.0, -20.0, 10.0)); // Gauche (Y descend/monte)
        primitives.push_back(CreateTestLine(2, -20.0, 10.0, 20.0, 10.0));   // Haut (X gauche->droite)
        primitives.push_back(CreateTestLine(5, 20.0, -10.0, -20.0, -10.0)); // Bas (X droite->gauche)
        primitives.push_back(CreateTestLine(3, 20.0, 10.0, 20.0, -10.0));   // Droite (Y monte/descend)

        // Exécution du moteur
        ContoursTopologyResult result = ContoursEngine::Process(primitives);

        // Affichage pour le log visuel
        ContoursEngine::Contours_DisplayContours(result);

        // --- BATTERIE DE TESTS DE SÉCURITÉ ---
        bool testPassed = true;
        const double EPSILON = 1e-4;

        // 1. Validation globale de la structure
        if (result.hasErrors) {
            std::cout << "  [FAIL] Le moteur a renvoyé une erreur globale inattendue : " << result.errorMessage << std::endl;
            testPassed = false;
        }
        if (result.validContours.size() != 2) {
            std::cout << "  [FAIL] Nombre de contours incorrect. Attendu: 2, Obtenu: " << result.validContours.size() << std::endl;
            testPassed = false;
        }

        if (!testPassed) {
            std::cout << ">> TEST 2 FAILED (Erreurs structurelles fondamentales) !" << std::endl;
            return false;
        }

        // 2. Validation approfondie du Contour Externe (Index 0 - Le grand cercle)
        const Contour& outer = result.validContours[0];
        std::cout << "  -> Verification du contour externe (Cercle)..." << std::endl;

        if (outer.isInternal) {
            std::cout << "  [FAIL] L'enveloppe circulaire externe est marquée comme interne (trou) !" << std::endl;
            testPassed = false;
        }
        if (!outer.isClosed) {
            std::cout << "  [FAIL] L'enveloppe circulaire externe est marquée comme ouverte !" << std::endl;
            testPassed = false;
        }
        if (outer.elements.size() != 1 || outer.elements[0].type != ContoursPrimitiveType::Circle) {
            std::cout << "  [FAIL] L'enveloppe externe doit comporter exactement 1 primitive de type Cercle !" << std::endl;
            testPassed = false;
        } else {
            // Aire attendue pour un cercle R=50 en sens anti-horaire = +M_PI * 50² ≈ +7853.9816
            double aireCercleAttendue = M_PI * 50.0 * 50.0;
            if (std::abs(outer.Aire - aireCercleAttendue) > EPSILON) {
                std::cout << "  [FAIL] L'aire du cercle externe est incorrecte ou mal orientee. Attendue: " << aireCercleAttendue << ", Obtenue: " << outer.Aire << std::endl;
                testPassed = false;
            }
            if (outer.elements[0].primitiveId != 1) {
                std::cout << "  [FAIL] L'ID du cercle externe a ete altere. Attendu: 1, Obtenu: " << outer.elements[0].primitiveId << std::endl;
                testPassed = false;
            }
        }

        // 3. Validation approfondie du Contour Interne (Index 1 - Le trou rectangulaire)
        const Contour& inner = result.validContours[1];
        std::cout << "  -> Verification du trou rectangulaire..." << std::endl;

        if (!inner.isInternal) {
            std::cout << "  [FAIL] Le rectangle interne est marqué comme enveloppe externe !" << std::endl;
            testPassed = false;
        }
        if (!inner.isClosed) {
            std::cout << "  [FAIL] Le rectangle interne est marqué comme ouvert !" << std::endl;
            testPassed = false;
        }
        // Aire d'un rectangle 40x20 attendue en sens horaire (négatif) = -800.0
        if (std::abs(inner.Aire - (-800.0)) > EPSILON) {
            std::cout << "  [FAIL] L'aire du trou rectangulaire est incorrecte. Attendue: -800.0, Obtenue: " << inner.Aire << std::endl;
            testPassed = false;
        }
        if (inner.elements.size() != 4) {
            std::cout << "  [FAIL] Le trou rectangulaire ne contient pas 4 elements mais " << inner.elements.size() << std::endl;
            testPassed = false;
        } else {
            // Contrôle strict du chaînage des points géométriques sur la chaîne reconstruite
            for (size_t i = 0; i < inner.elements.size(); ++i) {
                const auto& curr = inner.elements[i];
                const auto& next = inner.elements[(i + 1) % inner.elements.size()]; // Boucle de fermeture

                if (!curr.StopCpy2D.IsEqual(next.StartCpy2D, EPSILON)) {
                    std::cout << "  [FAIL] Rupture géométrique détectée dans le trou rectangulaire entre l'élément " << i << " et " << (i+1) << std::endl;
                    testPassed = false;
                }
            }
        }

        // --- RÉSULTAT FINAL ---
        if (testPassed) {
            std::cout << ">> TEST 2 PASSED !" << std::endl;
        } else {
            std::cout << ">> TEST 2 FAILED !" << std::endl;
        }
        return testPassed;
    }


    //─────────────────────────────────────────────────────────────────────
    // CAS 2 : Un contour ouvert (il manque un côté au carré)
    // Validation attendue : hasErrors = true, diagnostic individuel "Contour ouvert"
    //─────────────────────────────────────────────────────────────────────
    bool Test_OpenContourError() {
        std::cout << "\n[TEST 2] : Contour ouvert (Segment manquant)" << std::endl;

        std::vector<ContoursElement> primitives;
        // Il manque le segment du haut (100,100 -> 0,100)
        primitives.push_back(CreateTestLine(1, 0, 0, 100, 0));
        primitives.push_back(CreateTestLine(2, 100, 0, 100, 100));
        primitives.push_back(CreateTestLine(4, 0, 100, 0, 0));

        ContoursTopologyResult result = ContoursEngine::Process(primitives);
        ContoursEngine::Contours_DisplayContours(result);

        if (result.hasErrors) {
            std::cout << ">> TEST 2 PASSED (L'erreur a bien ete interceptee) !" << std::endl;
            return true;
        } else {
            std::cout << ">> TEST 2 FAILED !" << std::endl;
            return false;
        }
    }

    //─────────────────────────────────────────────────────────────────────
    // CAS 3 : Collision / Intersection (Le cercle coupe le bord externe)
    // Validation attendue : hasErrors = true, message d'intersection
    //─────────────────────────────────────────────────────────────────────
    bool Test_CircleIntersectionError() {
        std::cout << "\n[TEST 3] : Collision (Le cercle chevauche le bord externe)" << std::endl;

        std::vector<ContoursElement> primitives;
        // Carré externe 100x100
        primitives.push_back(CreateTestLine(1, 0, 0, 100, 0));
        primitives.push_back(CreateTestLine(2, 100, 0, 100, 100));
        primitives.push_back(CreateTestLine(3, 100, 100, 0, 100));
        primitives.push_back(CreateTestLine(4, 0, 100, 0, 0));

        // Cercle positionné sur le bord droit (Centre X=100, Y=50, Rayon=10) -> Il intersecte !
        primitives.push_back(CreateTestCircle(5, 100, 50, 10));

        ContoursTopologyResult result = ContoursEngine::Process(primitives);
        ContoursEngine::Contours_DisplayContours(result);

        if (result.hasErrors) {
            std::cout << ">> TEST 3 PASSED (L'intersection a bien ete detectee) !" << std::endl;
            return true;
        } else {
            std::cout << ">> TEST 3 FAILED !" << std::endl;
            return false;
        }
    }

    //─────────────────────────────────────────────────────────────────────
    // Point d'entrée principal des tests
    //─────────────────────────────────────────────────────────────────────
    bool RunAllTests() {
        std::cout << "========================================" << std::endl;
        std::cout << "=== DEBUT DU BANC D'ESSAI GEOMETRIQUE ===" << std::endl;
        std::cout << "========================================" << std::endl;

        bool allOk = true;

        allOk &= Test_ValidSquareWithHole();
        std::cout << "----------------------------------------" << std::endl;
        allOk &= Test_ValidCircleWithRectangleHole();
        std::cout << "----------------------------------------" << std::endl;


        allOk &= Test_OpenContourError();
        std::cout << "----------------------------------------" << std::endl;
        allOk &= Test_CircleIntersectionError();

        std::cout << "\n========================================" << std::endl;
        if (allOk) {
            std::cout << "=== RÉSULTAT GLOBAL : TOUS LES TESTS PASSENT ! ===" << std::endl;
        } else {
            std::cout << "=== RÉSULTAT GLOBAL : ERREUR (Au moins un test a échoué) ===" << std::endl;
        }
        std::cout << "========================================" << std::endl;

        return allOk;
    }


}

