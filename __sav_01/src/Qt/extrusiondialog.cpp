#include "extrusiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>



ExtrusionDialog::ExtrusionDialog( ExtrusionDialog_Params& params, QWidget* parent)
    : QDialog(parent)
{


    setWindowTitle("Modifier Extrusion");

    auto layout = new QVBoxLayout(this);

    // --- CHAMP SKETCH (Ajouté en premier) ---
    auto sketchLayout = new QHBoxLayout();
    sketchLayout->addWidget(new QLabel("Sketch de référence :"));

    m_comboSketch = new QComboBox(this);

    // On parcourt la liste présente dans params
    for (const auto& sketch : params.ListSketchRef) {
        m_comboSketch->addItem(QString::fromStdString(sketch.name), sketch.id);     // On affiche le nom (.name) et on stocke l'id (.id) en tâche de fond
        std::cout << "ExtrusionDialog : m_comboSketch add  [" << sketch.name << "] ["<<sketch.id << "]" << std::endl;
    }
    sketchLayout->addWidget(m_comboSketch);
    layout->addLayout(sketchLayout);


    // Champ Start
    auto startLayout = new QHBoxLayout();
    startLayout->addWidget(new QLabel("Distance de début (Start) :"));
    m_startSpinBox = new QDoubleSpinBox(this);
    m_startSpinBox->setRange(-10000.0, 10000.0);
    m_startSpinBox->setValue(params.currentStart);
    startLayout->addWidget(m_startSpinBox);
    layout->addLayout(startLayout);

    // Champ Stop
    auto stopLayout = new QHBoxLayout();
    stopLayout->addWidget(new QLabel("Distance de fin (Stop) :"));
    m_stopSpinBox = new QDoubleSpinBox(this);
    m_stopSpinBox->setRange(-10000.0, 10000.0);
    m_stopSpinBox->setValue(params.currentStop);
    stopLayout->addWidget(m_stopSpinBox);
    layout->addLayout(stopLayout);

    //---- opération booléeene -------------------
    auto boolLayout = new QHBoxLayout();
    boolLayout->addWidget(new QLabel("Opération booléenne :"));

    m_comboBoolOp = new QComboBox(this);
    // On insère le texte et on associe l'enum converti en int (userData)
    m_comboBoolOp->addItem("Aucune (None)", static_cast<int>(EBooleanOp::None));
    m_comboBoolOp->addItem("Union (Unite)", static_cast<int>(EBooleanOp::Union));
    m_comboBoolOp->addItem("Soustraction (Substract)", static_cast<int>(EBooleanOp::Substract));
    int index = m_comboBoolOp->findData(static_cast<int>(params.currentOp));    // On cherche l'index correspondant à la valeur entière de l'enum stockée dans les paramètres
    // Si findData trouve une correspondance, il retourne un index >= 0. Sinon, il retourne -1.
    if (index != -1) {
        m_comboBoolOp->setCurrentIndex(index);
    }

    //---- sketch de référence --------------
    std::cout << "ExtrusionDialog : ChoosedSketchRef = " << params.ChoosedSketchRef.id << std::endl;
    int sketchIndex = m_comboSketch->findData(params.ChoosedSketchRef.id);
    if (sketchIndex != -1) {
        std::cout << "ExtrusionDialog : sketchIndex = " << sketchIndex << std::endl;
        m_comboSketch->setCurrentIndex(sketchIndex);
    }else{
        std::cout << "ExtrusionDialog : sketchIndex INVALIDE:" << sketchIndex << std::endl;
    }

    boolLayout->addWidget(m_comboBoolOp);
    layout->addLayout(boolLayout);

    // Boutons OK / Annuler
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}


EBooleanOp ExtrusionDialog::getBooleanOp() const {
    int enumValue = m_comboBoolOp->currentData().toInt();
    return static_cast<EBooleanOp>(enumValue);
}


ExtrusionDialog_Params  ExtrusionDialog::getEntedredParams () const {
    ExtrusionDialog_Params  lparam;
    lparam.currentOp = getBooleanOp();
    lparam.currentStart = m_startSpinBox->value();
    lparam.currentStop = m_stopSpinBox->value();
    lparam.ChoosedSketchRef.id = m_comboSketch->currentData().toInt();
    lparam.ChoosedSketchRef.name = m_comboSketch->currentText().toStdString();
    return lparam;
}


