

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include "Dialog_SketchHelper.h" // Ton fichier de structures

class Dialog_SketchHelper_Popup : public QDialog {
    Q_OBJECT

public:
    explicit Dialog_SketchHelper_Popup(QWidget* parent = nullptr);
    ~Dialog_SketchHelper_Popup() = default;

    // Méthode principale : initialise ou met à jour la popup avec un nouveau Helper
    void setHelperData(const DialogSketchHelper::Helper& newHelper);


    void clickedCancel (void);
    void clickedReset (void);
    void clickedOk (void);

    //double m_value;


signals:
    // Signaux pour remonter les actions à ton outil / moteur
    void doubleValueChanged(const QString& id, double val);
    void fieldClicked(const QString& id);
    void OnClickedButton ( int li_button);

private:
    void buildUI(const DialogSketchHelper::Helper& helper);
    void updateUI(const DialogSketchHelper::Helper& newHelper);
    
    // Application des états visuels (focus, disabled, valide/invalide) sur un widget champ
    void applyAttributes(QWidget* widget, const DialogSketchHelper::AttributsChamps& attrs);

    // Éléments UI principaux de la fenêtre
    QLabel* m_lblTitle = nullptr;
    QLabel* m_lblInstruction = nullptr;
    QVBoxLayout* m_fieldsLayout = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;

    QHBoxLayout* m_buttonLayout = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QPushButton* m_btnReset = nullptr;
    QPushButton* m_btnOk = nullptr;


    // Stockage de référence de l'état actuel
    DialogSketchHelper::Helper m_currentHelper;

    // Indexation des widgets par leur ID unique pour le "diffing" et les mises à jour
    QMap<QString, QWidget*> m_widgetMap;
};


