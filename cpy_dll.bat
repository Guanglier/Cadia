@echo off
chcp 65001 >nul
echo ========================================================
echo   Déploiement des dépendances Qt (CadIa++) Global
echo ========================================================

:: --- CONFIGURATION DE BASE ---
set "VCPKG_BASE=J:\vcpkg\installed\x64-windows"
set "PARENT_BUILD_DIR=%~dp0build"

:: Vérification que le dossier principal 'build' existe
if not exist "%PARENT_BUILD_DIR%" (
    echo [ERREUR] Le dossier parent 'build' n'existe pas :
    echo "%PARENT_BUILD_DIR%"
    goto :error
)

echo Recherche des dossiers de build dans : %PARENT_BUILD_DIR%
echo.

:: --- BOUCLE SUR TOUS LES SOUS-DOSSIERS DE BUILD ---
:: /D permet de ne boucler que sur les Répertoires (Dossiers)
for /D %%G in ("%PARENT_BUILD_DIR%\*") do (
    
    echo --------------------------------------------------------
    echo Traitement du dossier : %%~nxG
    echo --------------------------------------------------------
    
    :: Entrée sécurisée dans le sous-dossier trouvé
    pushd "%%G"

    :: --- 1. COPIE DES DLL PRINCIPALES ---
    echo Copie des bibliothèques principales...
    copy /Y "%VCPKG_BASE%\tools\Qt6\bin\Qt6Svg.dll" . >nul
    copy /Y "%VCPKG_BASE%\debug\bin\Qt6Svgd.dll" . >nul

    :: --- 2. DOSSIER PLATFORMS ---
    echo Configuration du dossier 'platforms'...
    if not exist "platforms" mkdir "platforms"
    pushd "platforms"
        copy /Y "%VCPKG_BASE%\Qt6\plugins\platforms\qwindows.dll" . >nul
        copy /Y "%VCPKG_BASE%\debug\Qt6\plugins\platforms\qwindowsd.dll" . >nul
    popd

    :: --- 3. DOSSIER IMAGEFORMATS ---
    echo Configuration du dossier 'imageformats'...
    if not exist "imageformats" mkdir "imageformats"
    pushd "imageformats"
        copy /Y "%VCPKG_BASE%\Qt6\plugins\imageformats\qsvg.dll" . >nul
        copy /Y "%VCPKG_BASE%\debug\Qt6\plugins\imageformats\qsvgd.dll" . >nul
    popd

    :: Sortie du sous-dossier actuel pour passer au suivant
    popd
    echo Dossier %%~nxG configuré avec succès !
    echo.
)

echo ========================================================
echo   Déploiement global terminé avec succès !
echo ========================================================
pause
exit /b 0

:error
echo.
echo [ECHEC] Le script a rencontré une erreur.
pause
exit /b 1



