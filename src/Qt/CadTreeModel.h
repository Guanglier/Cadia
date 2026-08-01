#pragma once
#include <QStandardItemModel>
#include "CAD_Part.h"

class CAD_Part;

class CadTreeModel : public QStandardItemModel {
    Q_OBJECT

public:
    explicit CadTreeModel(QObject* parent = nullptr);

    // Rôle personnalisé pour stocker nos ID d'opérations et de primitives
    enum TreeRoles {
        IdRole = Qt::UserRole + 1,
        TypeRole // Optionnel : pour savoir si c'est une opération, une primitive, etc.
    };

    bool m_DisplaySketchDetails = true;
    bool m_DisplaySolids = true;

    // Recrée complètement l'arbre à partir du Part CAD
    void refreshFromPart(const CAD_Part& part);
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    // Fonctions d'icônes d'exemples (à lier avec tes ressources Qt)
    QIcon getIconForOperation(const CadOperation& op);
};


