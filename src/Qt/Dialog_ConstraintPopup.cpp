#include "Dialog_ConstraintPopup.h"
#include <QHBoxLayout>

Dialog_ConstraintPopup::Dialog_ConstraintPopup(ConstraintPanelType type, QWidget* parent)
    : QDialog(parent), m_constraintType(type) {
    setupUI();
}

void Dialog_ConstraintPopup::setupUI() {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_lblTitle = new QLabel(this);
    QString titleStr;
    bool needsValue = false;

    switch (m_constraintType) {
        case ConstraintPanelType::Horizontal:     titleStr = tr("Contrainte : Horizontale"); break;
        case ConstraintPanelType::Vertical:       titleStr = tr("Contrainte : Verticale"); break;
        case ConstraintPanelType::Parallel:       titleStr = tr("Contrainte : Parallèle"); break;
        case ConstraintPanelType::Perpendicular:  titleStr = tr("Contrainte : Perpendiculaire"); break;
        case ConstraintPanelType::Coincident:     titleStr = tr("Contrainte : Coïncidente"); break;
        case ConstraintPanelType::Distance:       titleStr = tr("Contrainte : Distance"); needsValue = true; break;
        case ConstraintPanelType::Radius:         titleStr = tr("Contrainte : Rayon"); needsValue = true; break;
        case ConstraintPanelType::Angle:          titleStr = tr("Contrainte : Angle"); needsValue = true; break;
    }

    setWindowTitle(titleStr);
    m_lblTitle->setText("<b>" + titleStr + "</b>");
    mainLayout->addWidget(m_lblTitle);

    m_lblSelectionInfo = new QLabel(tr("Sélectionnez les entités requises dans la vue 3D..."), this);
    m_lblSelectionInfo->setWordWrap(true);
    m_lblSelectionInfo->setStyleSheet("color: #555555; margin-bottom: 5px;");
    mainLayout->addWidget(m_lblSelectionInfo);

    m_valueContainer = new QWidget(this);
    if (needsValue) {
        QHBoxLayout* valLayout = new QHBoxLayout(m_valueContainer);
        valLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* lblVal = new QLabel(tr("Valeur cible :"), this);
        m_txtValue = new QLineEdit(this);
        m_txtValue->setPlaceholderText("0.00");
        
        valLayout->addWidget(lblVal);
        valLayout->addWidget(m_txtValue);

        connect(m_txtValue, &QLineEdit::textChanged, this, [this](const QString& text) {
            bool ok = false;
            double val = text.toDouble(&ok);
            if (ok) emit valueChanged(val);
        });
    } else {
        m_txtValue = nullptr;
        m_valueContainer->hide();
    }
    mainLayout->addWidget(m_valueContainer);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnOk = new QPushButton(tr("Valider"), this);
    m_btnCancel = new QPushButton(tr("Annuler"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOk);
    btnLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(btnLayout);

    connect(m_btnOk, &QPushButton::clicked, this, [this]() {
        emit constraintValidated();
        accept();
    });
    connect(m_btnCancel, &QPushButton::clicked, this, [this]() {
        emit constraintCancelled();
        reject();
    });
}

void Dialog_ConstraintPopup::setSelectionStatus(const QString& statusText) {
    m_lblSelectionInfo->setText(statusText);
}

void Dialog_ConstraintPopup::setSuggestedValue(double value) {
    if (m_txtValue) {
        m_txtValue->setText(QString::number(value, 'f', 3));
    }
}

double Dialog_ConstraintPopup::getTargetValue() const {
    if (!m_txtValue) return 0.0;
    bool ok = false;
    double val = m_txtValue->text().toDouble(&ok);
    return ok ? val : 0.0;
}





void Dialog_ConstraintPopup::updateSelectionStep(const QString& infoText, bool isComplete) {
    m_lblSelectionInfo->setText(infoText);
    if (isComplete) {
        // Tu peux par exemple changer la couleur du texte ou activer le bouton OK visuellement
        m_lblSelectionInfo->setStyleSheet("color: #2e7d32; font-weight: bold; margin-bottom: 5px;"); // Vert succès
    } else {
        m_lblSelectionInfo->setStyleSheet("color: #d32f2f; font-weight: bold; margin-bottom: 5px;"); // Rouge / en cours
    }
}










