



#include "Dialog_SketchHelper_Popup.h"
#include <QDoubleValidator>
#include <QPushButton>
#include <QVariant>

Dialog_SketchHelper_Popup::Dialog_SketchHelper_Popup(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    m_mainLayout = new QVBoxLayout(this);

    m_lblTitle = new QLabel(this);
    QFont titleFont = m_lblTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_lblTitle->setFont(titleFont);
    m_mainLayout->addWidget(m_lblTitle);

    m_lblInstruction = new QLabel(this);
    m_lblInstruction->setWordWrap(true);
    m_mainLayout->addWidget(m_lblInstruction);

    // Layout conteneur pour les champs dynamiques
    m_fieldsLayout = new QVBoxLayout();
    m_mainLayout->addLayout(m_fieldsLayout);

    m_mainLayout->addStretch(); // Pousse tout vers le haut

    // --- Création de la zone de boutons du bas ---
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch(); // Aligne les boutons à droite (standard UI)

    m_btnCancel = new QPushButton(tr("Annuler"), this);
    m_btnReset = new QPushButton(tr("Réinitialiser"), this);
    m_btnOk = new QPushButton(tr("OK"), this);

    connect(m_btnCancel, &QPushButton::clicked, this, &Dialog_SketchHelper_Popup::clickedCancel);
    connect(m_btnReset, &QPushButton::clicked, this, &Dialog_SketchHelper_Popup::clickedReset);
    connect(m_btnOk, &QPushButton::clicked, this, &Dialog_SketchHelper_Popup::clickedOk);

    m_buttonLayout->addWidget(m_btnCancel);
    m_buttonLayout->addWidget(m_btnReset);
    m_buttonLayout->addWidget(m_btnOk);

    m_mainLayout->addLayout(m_buttonLayout);
}

void Dialog_SketchHelper_Popup::setHelperData(const DialogSketchHelper::Helper& newHelper)
{
    //if (m_widgetMap.isEmpty() || m_currentHelper.champMultiple.size() != newHelper.champMultiple.size()) {
        buildUI(newHelper);
    //} else {
   //     updateUI(newHelper);
   // }

    m_currentHelper = newHelper;
}

void Dialog_SketchHelper_Popup::buildUI(const DialogSketchHelper::Helper& helper)
{
    QLayoutItem* item;
    while ((item = m_fieldsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    m_widgetMap.clear();

    m_lblTitle->setText(helper.title);
    m_lblInstruction->setText(helper.instructionText);

    // Application de la visibilité et de l'état des boutons lors de la construction
    m_btnCancel->setVisible(helper.showButtonCancel);
    m_btnReset->setVisible(helper.showButtonReset);
    m_btnOk->setVisible(helper.showButtonOk);
    m_btnOk->setEnabled(helper.isButtonOkEnabled);

    for (const auto& champVariant : helper.champMultiple) {
        std::visit([this](auto&& champ) {
            using T = std::decay_t<decltype(champ)>;

            if constexpr (std::is_same_v<T, DialogSketchHelper::ChampInputDouble>) {
                QWidget* container = new QWidget(this);
                QHBoxLayout* hLayout = new QHBoxLayout(container);
                hLayout->setContentsMargins(0, 0, 0, 0);

                QLabel* label = new QLabel(champ.title, container);
                QLineEdit* lineEdit = new QLineEdit(container);
                lineEdit->setText(QString::number(champ.value));
                lineEdit->setProperty("field_id", champ.id);
                lineEdit->setValidator(new QDoubleValidator(lineEdit));

                connect(lineEdit, &QLineEdit::textChanged, this, [this, id = champ.id](const QString& text) {
                    bool ok = false;
                    double val = text.toDouble(&ok);
                    if (ok) {
                        emit doubleValueChanged(id, val);
                    }
                });

                hLayout->addWidget(label);
                hLayout->addWidget(lineEdit);

                applyAttributes(lineEdit, champ);
                m_fieldsLayout->addWidget(container);
                m_widgetMap[champ.id] = lineEdit;

            } else if constexpr (std::is_same_v<T, DialogSketchHelper::ChampInputSelection>) {
                QWidget* container = new QWidget(this);
                QHBoxLayout* hLayout = new QHBoxLayout(container);
                hLayout->setContentsMargins(0, 0, 0, 0);

                QLabel* label = new QLabel(champ.title, container);
                QLineEdit* lineEdit = new QLineEdit(container);
                lineEdit->setText(  champ.field_text );
                lineEdit->setProperty("field_id", champ.id);
                lineEdit->setReadOnly(true);

                hLayout->addWidget(label);
                hLayout->addWidget(lineEdit);

                applyAttributes(lineEdit, champ);
                m_fieldsLayout->addWidget(container);
                m_widgetMap[champ.id] = lineEdit;

            } else if constexpr (std::is_same_v<T, DialogSketchHelper::ChampInputImage>) {
                QLabel* imgLabel = new QLabel(this);
                if (!champ.imagePath.isEmpty()) {
                    imgLabel->setPixmap(QPixmap(champ.imagePath));
                } else {
                    imgLabel->setText(champ.title);
                }
                m_fieldsLayout->addWidget(imgLabel);
                m_widgetMap[champ.id] = imgLabel;
            }
        }, champVariant);
    }
}
/*
void Dialog_SketchHelper_Popup::updateUI(const DialogSketchHelper::Helper& newHelper)
{
    m_lblTitle->setText(newHelper.title);
    m_lblInstruction->setText(newHelper.instructionText);

    // Mise à jour de la visibilité et de l'état des boutons du bas
    m_btnCancel->setVisible(newHelper.showButtonCancel);
    m_btnReset->setVisible(newHelper.showButtonReset);
    m_btnOk->setVisible(newHelper.showButtonOk);
    m_btnOk->setEnabled(newHelper.isButtonOkEnabled);

    for (const auto& champVariant : newHelper.champMultiple) {
        std::visit([this](auto&& champ) {
            using T = std::decay_t<decltype(champ)>;

            if constexpr (std::is_same_v<T, DialogSketchHelper::ChampInputDouble> ||
                          std::is_same_v<T, DialogSketchHelper::ChampInputSelection>) {

                if (m_widgetMap.contains(champ.id)) {
                    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(m_widgetMap[champ.id]);
                    if (lineEdit) {
                        if constexpr (std::is_same_v<T, DialogSketchHelper::ChampInputDouble>) {
                            double currentVal = lineEdit->text().toDouble();
                            if (currentVal != champ.value) {
                                lineEdit->setText(QString::number(champ.value));
                            }
                        } else if constexpr (std::is_same_v<T, DialogSketchHelper::ChampInputSelection>) {
                            QString expectedText = champ.IsOk ? "Sélectionné [OK]" : "En attente de sélection...";
                            if (lineEdit->text() != expectedText) {
                                lineEdit->setText(expectedText);
                            }
                        }
                        applyAttributes(lineEdit, champ);
                    }
                }
            }
        }, champVariant);
    }
}
*/
void Dialog_SketchHelper_Popup::applyAttributes(QWidget* widget, const DialogSketchHelper::AttributsChamps& attrs)
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
    if (!lineEdit) return;

    lineEdit->setEnabled(!attrs.b_IsDisabled);

    if (attrs.b_IsFocus) {
        lineEdit->setFocus();
    }

    if (!attrs.b_IsValid && !attrs.b_IsDisabled) {
        lineEdit->setStyleSheet("QLineEdit { border: 2px solid #e74c3c; border-radius: 4px; background-color: #fdf2f2; padding-right: 20px; }");
        if (lineEdit->actions().isEmpty()) {
            QAction* errorAction = lineEdit->addAction(QIcon(":/icons/red_cross.png"), QLineEdit::TrailingPosition);
            errorAction->setToolTip("Entrée invalide ou incomplète");
        }
    } else if (attrs.b_IsValid) {
        lineEdit->setStyleSheet("QLineEdit { border: 2px solid #2ecc71; border-radius: 4px; background-color: #f4fcf7; }");
        //lineEdit->clear();
    } else {
        lineEdit->setStyleSheet("");
        //lineEdit->clear();
    }
}


void Dialog_SketchHelper_Popup::clickedCancel (void){
    emit OnClickedButton(0);
}
void Dialog_SketchHelper_Popup::clickedReset (void){
    emit OnClickedButton(1);
}
void Dialog_SketchHelper_Popup::clickedOk (void){
    emit OnClickedButton(2);
}



