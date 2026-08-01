#include "CadTreeModel.h"
#include "CAD_Part.h"
#include <QIcon>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QImageReader>

CadTreeModel::CadTreeModel(QObject* parent) : QStandardItemModel(parent) {
    // Définir l'en-tête de la colonne principale
    setHorizontalHeaderLabels({ tr("Arborescence du modèle") });
}

Qt::ItemFlags CadTreeModel::flags(const QModelIndex& index) const {
    // 1. Récupère les flags de base générés par QStandardItemModel
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);
    // 2. Si l'index est valide, on retire le droit d'édition
    if (index.isValid()) {
        return defaultFlags & ~Qt::ItemIsEditable;
    }
    return defaultFlags;
}

void CadTreeModel::refreshFromPart(const CAD_Part& part) {
    this->clear(); // On vide l'arbre avant de le reconstruire
    setHorizontalHeaderLabels({ tr("Arborescence du modèle") });

    // Récupération des opérations (on accède aux items constants)
    // Note : Pense à ajouter un accesseur public const sur m_operationRegistry dans CAD_Part si ce n'est pas fait.
    //const auto& operations = part.trouverToutesLesOperations(); // À adapter selon ton accesseur
    const auto& operations = part.getOperationRegistry().getItems();



    for (const auto& op : operations) {
        // 1. Création de l'item principal de l'opération
        QStandardItem* opItem = new QStandardItem();
        QString  qs_item_text = QString::number ( op.id ) + " : " + QString::fromStdString(op.getName() );
        //opItem->setText(QString::fromStdString(op.getName()));
        opItem->setText(qs_item_text );
        opItem->setIcon(getIconForOperation(op));
        
        // Crucial : On stocke l'ID unique dans l'item Qt
        opItem->setData(static_cast<qlonglong>(op.id), IdRole);

        // Optionnel : Mise en forme (ex: texte en italique si l'opération est modifiée)
        /*
        if (op.isModified()) {
            QFont font = opItem->font();
            font.setItalic(true);
            opItem->setFont(font);
            opItem->setForeground(Qt::gray); // Style "dirty"
        }
        */

        // 2. Inspection du contenu de l'opération via std::visit (comme dans ton tst_dump_tree)
        // de getparams -> SketchParams qui contient les primitives et les contraintes
        std::visit([this, opItem, op](const auto& params) {
            using T = std::decay_t<decltype(params)>;

            if constexpr (std::is_same_v<T, SketchParams>) {
                if ( true == m_DisplaySketchDetails )
                {
                    // On ajoute des sous-éléments pour les primitives de l'esquisse
                    if (!params.getPrimitives().empty()) {
                        QStandardItem* primsFolder = new QStandardItem(tr("Primitives"));
                        primsFolder->setIcon(QIcon(":/icons/folder_geom.png"));

                        for (const auto& primitive : params.getPrimitives()) {
                            std::visit([this, primsFolder, op](const auto& prim) {
                                using PType = std::decay_t<decltype(prim)>;
                                QString label;
                                if constexpr (std::is_same_v<PType, SketchLine>) label = QString("%1 - Ligne").arg(prim.id);
                                else if constexpr (std::is_same_v<PType, SketchCircle>) label = QString("%1 - Cercle (R=%2)").arg(prim.id).arg(prim.radius);
                                else if constexpr (std::is_same_v<PType, SketchArc>) label = QString("%1 - Arc").arg(prim.id);

                                QStandardItem* primItem = new QStandardItem(label);
                                primItem->setIcon(QIcon(":/icons/geometry.png"));
                                // On stocke l'ID de l'opération parente ET de la primitive si besoin
                                primItem->setData(static_cast<qlonglong>(op.id), IdRole);
                                primsFolder->appendRow(primItem);
                            }, primitive);
                        }

                        opItem->appendRow(primsFolder);
                    }
                    if (!params.getConstraints().empty()) {
                        QStandardItem* contraintesFolder = new QStandardItem(tr("Contraintes"));
                        //contraintesFolder->setIcon(QIcon(":/icons/folder_geom.png"));

                        for (const auto& constraint : params.getConstraints()) {
                            QString label = QString::fromStdString(op.getConstraintTypeString(constraint));
                            QStandardItem *contrainteType = new QStandardItem(label);

                            std::visit([&](const auto& c) {
                                using T = std::decay_t<decltype(c)>;

                                if constexpr (std::is_same_v<T, PartSketchConstraint::ParallelConstraint> ||
                                              std::is_same_v<T, PartSketchConstraint::DistanceConstraint> ||
                                              std::is_same_v<T, PartSketchConstraint::PerpendicularConstraint> ||
                                              std::is_same_v<T, PartSketchConstraint::CoincidentConstraint>) {
                                    // Cas des contraintes à 2 références (ref1 et ref2)
                                    QString const_ref1 = "op:" + QString::number(c.ref1.operationId) + "; primId:" + QString::number(c.ref1.primitiveId) + " " +
                                                         QString::fromStdString(op.getConstraintSubElementString(c.ref1.subElement));
                                    QString const_ref2 = "op:" + QString::number(c.ref2.operationId) + " primId:" + QString::number(c.ref2.primitiveId) + " " +
                                                         QString::fromStdString(op.getConstraintSubElementString(c.ref2.subElement));

                                    contrainteType->appendRow(new QStandardItem(const_ref1));
                                    contrainteType->appendRow(new QStandardItem(const_ref2));
                                }
                                else if constexpr (std::is_same_v<T, PartSketchConstraint::VerticalConstraint> ||
                                                   std::is_same_v<T, PartSketchConstraint::HorizontalConstraint>) {
                                    // Cas des contraintes à 1 seule référence nommée ref
                                    QString const_ref = "op:" + QString::number(c.ref.operationId) + "; primId:" + QString::number(c.ref.primitiveId) + " " +
                                                        QString::fromStdString(op.getConstraintSubElementString(c.ref.subElement));

                                    contrainteType->appendRow(new QStandardItem(const_ref));
                                }
                                else if constexpr (std::is_same_v<T, PartSketchConstraint::RadiusConstraint>) {
                                    // Cas de la contrainte Radius (qui utilise ref1 seule)
                                    QString const_ref1 = "op:" + QString::number(c.ref1.operationId) + "; primId:" + QString::number(c.ref1.primitiveId) + " " +
                                                         QString::fromStdString(op.getConstraintSubElementString(c.ref1.subElement));

                                    contrainteType->appendRow(new QStandardItem(const_ref1));
                                }
                            }, constraint.data);

                            contraintesFolder->appendRow(contrainteType);
                        }

                        opItem->appendRow(contraintesFolder);
                    }
                }//end if ( true == m_DisplaySketchDetails )
            }
            else if constexpr (std::is_same_v<T, ExtrudeParams>) {

                if ( true == m_DisplaySolids )
                {
                    QStandardItem* infoItem = new QStandardItem(QString(tr("Start : %1 mm")).arg(params.start));
                    infoItem->setEnabled(false); // Juste informatif, non cliquable
                    opItem->appendRow(infoItem);
                    infoItem = nullptr;

                    QStandardItem* infoItem2 = new QStandardItem(QString(tr("End : %1 mm")).arg(params.end));
                    infoItem2->setEnabled(false); // Juste informatif, non cliquable
                    opItem->appendRow(infoItem2);
                    infoItem2 = nullptr;

                    switch ( params.EboolOp ){
                        default:
                            infoItem = new QStandardItem(QString(tr("bool : aucun")));
                            break;
                        case EBooleanOp::Substract:
                            infoItem = new QStandardItem(QString(tr("bool : substract")));
                            break;
                        case EBooleanOp::Union:
                            infoItem = new QStandardItem(QString(tr("bool : union")));
                            break;
                        case EBooleanOp::Intersect:
                            infoItem = new QStandardItem(QString(tr("bool : intersect")));
                            break;
                    }
                    if (nullptr != infoItem){
                        infoItem->setEnabled(false);
                        opItem->appendRow(infoItem);
                    }

                }

            }
        }, op.getParams());

        // Ajouter l'opération complète à la racine de l'arbre
        this->appendRow(opItem);
    }
}

QIcon CadTreeModel::getIconForOperation(const CadPartOp& op) {
    return std::visit([](const auto& params) -> QIcon {
        using T = std::decay_t<decltype(params)>;
        if constexpr (std::is_same_v<T, CoordinateSystem>) return QIcon(":/icons/referentiel_1_opt.svg");
        else if constexpr (std::is_same_v<T, SketchParams>) return QIcon(":/icons/sketch_2_opt.svg");
        else if constexpr (std::is_same_v<T, ExtrudeParams>){
            QString path = ":/icons/extrude_01_opt.svg";
            bool exists = QFile::exists(path);
            //qDebug() << "[CAD Tree] Passage dans ExtrudeParams. Fichier ressources existant ?" << exists << "\n";
            //qDebug() << "Contenu de /icons :" << QDir(":/icons").entryList() <<  "\n";
            //qDebug() << "Contenu de / :" << QDir(":/").entryList() <<  "\n";
            //qDebug() << "Formats d'images supportés par Qt :" << QImageReader::supportedImageFormats()<<  "\n";
            return QIcon(path);
        }
        else return QIcon(":/icons/default.png");
    }, op.getParams());
}

