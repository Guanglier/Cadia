

#pragma once
#include <QString>
#include <vector>
#include <variant>

namespace DialogSketchHelper{
	
	
	struct AttributsChamps{
		bool	b_IsFocus = false;		// mettre le focus car l'utilisateur doit le rentrer (ou cliquer sur le bon element dans la 3d pour le remplir)
		bool	b_IsDisabled = true;	// desactiver car pas encore actif (ex un champ à sélectionner plus tard)
		bool	b_IsValid = false;		// si c'est valide alors l'entrée est ok
	};

	// Champ pour faire rentrer ou afficher une valeur double
	struct ChampInputDouble : public AttributsChamps{
		QString id;             // Identifiant unique (ex: "val_distance", "val_angle")
		QString title;          // Libellé affiché (ex: "Distance :", "Angle :")
		double  value = 0.0;    // Valeur par défaut ou actuelle
	};

	// Champ pour afficher une sélection
	struct ChampInputSelection : public AttributsChamps{
		QString id;             // Identifiant unique (ex: "sel_point1", "sel_line2")
		QString title;          // Libellé affiché (ex: "Premier point :")
        QString field_text;     // texte affiché dans la zone de sélection du champ
		bool    IsOk = false;   // Vrai si l'élément a été sélectionné
	};

	// Champ pour afficher une image ou un statut visuel
	struct ChampInputImage {
		QString id;             // Identifiant unique (ex: "img_status")
		QString title;          // Libellé (optionnel)
		QString imagePath;      // Chemin de l'image
	};

	using ChampMultiple = std::variant<ChampInputDouble, ChampInputSelection, ChampInputImage>;

	struct Helper {
		QString title;                  
		QString instructionText;        
		bool    isSelectionComplete = false;
        std::vector<ChampMultiple> champMultiple;
        bool showButtonCancel = true;      // Afficher ou masquer le bouton Annuler
        bool showButtonReset = true;       // Afficher ou masquer le bouton Réinitialiser
        bool showButtonOk = true;          // Afficher ou masquer le bouton OK
        bool isButtonOkEnabled = false;    // Activer ou griser le bouton OK
	};


};












