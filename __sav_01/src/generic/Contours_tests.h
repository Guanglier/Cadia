
#pragma once

namespace ContoursTests {

    // Lance l'intégralité de la suite de tests unitaires et renvoie false si au moins un test échoue
    bool RunAllTests();

    // Fonctions de tests individuelles (si tu veux les lancer séparément)
    bool Test_ValidSquareWithHole();
    bool Test_OpenContourError();
    bool Test_CircleIntersectionError();

}

