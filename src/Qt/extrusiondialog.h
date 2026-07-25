#pragma once
#include <QDialog>
#include <QDoubleSpinBox>
#include <QComboBox>
#include "CAD_Operation.h"


struct ExtrusionDialog_SketchRef{
    std::string name;
    int id;
};

struct ExtrusionDialog_Params {
    double currentStart;
    double currentStop;
    EBooleanOp currentOp;
    std::vector<ExtrusionDialog_SketchRef> ListSketchRef;
    ExtrusionDialog_SketchRef   ChoosedSketchRef;
};

class ExtrusionDialog : public QDialog {
    Q_OBJECT
public:
    //ExtrusionDialog(double currentStart, double currentStop, QWidget* parent = nullptr);
    //ExtrusionDialog(double currentStart, double currentStop, EBooleanOp currentOp = EBooleanOp::None, QWidget* parent = nullptr);
    ExtrusionDialog(ExtrusionDialog_Params& params, QWidget* parent = nullptr);
    
    EBooleanOp getBooleanOp() const;
    ExtrusionDialog_Params  getEntedredParams () const;



private:
    QDoubleSpinBox* m_startSpinBox;
    QDoubleSpinBox* m_stopSpinBox;
    QComboBox* m_comboBoolOp;
    QComboBox* m_comboSketch{nullptr};
};


