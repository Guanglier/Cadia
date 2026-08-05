#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

enum class ConstraintPanelType {
    Horizontal,
    Vertical,
    Parallel,
    Perpendicular,
    Coincident,
    Distance,
    Radius,
    Angle
};

class Dialog_ConstraintPopup : public QDialog {
    Q_OBJECT
public:
    explicit Dialog_ConstraintPopup(ConstraintPanelType type, QWidget* parent = nullptr);
    ~Dialog_ConstraintPopup() = default;

    void setSelectionStatus(const QString& statusText);
    void setSuggestedValue(double value);
    double getTargetValue() const;

public slots:
    void updateSelectionStep(const QString& infoText, bool isComplete = false);     // Permet de mettre à jour le texte d'aide au fil de la sélection (ex: "1er point sélectionné. Choisissez le 2e point.")


signals:
    void valueChanged(double newValue);
    void constraintCancelled();
    void constraintValidated();

private:
    ConstraintPanelType m_constraintType;
    QLabel* m_lblTitle;
    QLabel* m_lblSelectionInfo;
    
    QWidget* m_valueContainer;
    QLineEdit* m_txtValue;
    
    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;

    void setupUI();
};