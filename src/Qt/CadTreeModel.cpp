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

    const auto& operations = part.getOperationRegistry().getItems();

    for (const auto& op : operations) {
        // 1. Création de l'item principal de l'opération
        QStandardItem* opItem = new QStandardItem();
        QString qs_item_text = QString::number(op.id) + " : " + QString::fromStdString(op.getName());
        opItem->setText(qs_item_text);
        opItem->setIcon(getIconForOperation(op));

        // Crucial : On stocke l'ID unique dans l'item Qt
        opItem->setData(static_cast<qlonglong>(op.id), IdRole);

        // 2. Inspection du contenu de l'opération via std::visit
        std::visit([this, opItem, op](const auto& params) {
            using T = std::decay_t<decltype(params)>;

            if constexpr (std::is_same_v<T, SketchParams>) {
                if (true == m_DisplaySketchDetails) {

                    // --- AJOUT : Listing des Points ---
                    if (!params.getPoints().empty()) {
                        QStandardItem* pointsFolder = new QStandardItem(tr("Points"));
                        pointsFolder->setIcon(QIcon(":/icons/folder_geom.png")); // Adapte ton icône

                        for (const auto& point : params.getPoints()) {
                            QString ptLabel = QString("Point %1 : (%2, %3)")
                            .arg(point.id)
                                .arg(point.p2d.X())
                                .arg(point.p2d.Y());

                            if (point.b_Locked) {
                                ptLabel += " [Verrouillé]";
                            }

                            QStandardItem* ptItem = new QStandardItem(ptLabel);
                            ptItem->setIcon(QIcon(":/icons/point.png")); // Adapte ton icône de point
                            ptItem->setData(static_cast<qlonglong>(op.id), IdRole);
                            pointsFolder->appendRow(ptItem);
                        }
                        opItem->appendRow(pointsFolder);
                    }

                    // --- Listing des Primitives (avec mise à jour des Lignes) ---
                    if (!params.getPrimitives().empty()) {
                        QStandardItem* primsFolder = new QStandardItem(tr("Primitives"));
                        primsFolder->setIcon(QIcon(":/icons/folder_geom.png"));

                        for (const auto& primitive : params.getPrimitives()) {
                            std::visit([this, primsFolder, op](const auto& prim) {
                                using PType = std::decay_t<decltype(prim)>;
                                QString label;

                                if constexpr (std::is_same_v<PType, SketchLine>) {
                                    // Complété pour afficher les IDs des points référencés
                                    label = QString("%1 - Ligne [Start: Pt %2 -> End: Pt %3]")
                                                .arg(prim.id)
                                                .arg(prim.startPointId)
                                                .arg(prim.stopPointId);
                                    if (prim.b_Locked) label += " [Verrouillé]";
                                }
                                else if constexpr (std::is_same_v<PType, SketchCircle>) {
                                    label = QString("%1 - Cercle (Centre: Pt %2, R=%3)")
                                    .arg(prim.id)
                                        .arg(prim.centerPointId)
                                        .arg(prim.radius);
                                }
                                else if constexpr (std::is_same_v<PType, SketchArc>) {
                                    label = QString("%1 - Arc").arg(prim.id);
                                }

                                QStandardItem* primItem = new QStandardItem(label);
                                primItem->setIcon(QIcon(":/icons/geometry.png"));
                                primItem->setData(static_cast<qlonglong>(op.id), IdRole);
                                primsFolder->appendRow(primItem);
                            }, primitive);
                        }

                        opItem->appendRow(primsFolder);
                    }

                    // --- Listing des Contraintes ---
                    if (!params.getConstraints().empty()) {
                        QStandardItem* contraintesFolder = new QStandardItem(tr("Contraintes"));

                        for (const auto& constraint : params.getConstraints()) {
                            QString label = QString::fromStdString(op.getConstraintTypeString(constraint));
                            QStandardItem *contrainteType = new QStandardItem(label);

                            std::visit([&](const auto& c) {
                                using TC = std::decay_t<decltype(c)>;

                                if constexpr (std::is_same_v<TC, PartSketchConstraint::ParallelConstraint> ||
                                              std::is_same_v<TC, PartSketchConstraint::DistanceConstraint> ||
                                              std::is_same_v<TC, PartSketchConstraint::PerpendicularConstraint> ||
                                              std::is_same_v<TC, PartSketchConstraint::CoincidentConstraint>) {

                                    QString const_ref1 = "op:" + QString::number(c.ref1.operationId) + "; primId:" + QString::number(c.ref1.Id) + " " +
                                                         QString::fromStdString(op.getConstraintSubElementString(c.ref1.subElement));
                                    QString const_ref2 = "op:" + QString::number(c.ref2.operationId) + " primId:" + QString::number(c.ref2.Id) + " " +
                                                         QString::fromStdString(op.getConstraintSubElementString(c.ref2.subElement));

                                    contrainteType->appendRow(new QStandardItem(const_ref1));
                                    contrainteType->appendRow(new QStandardItem(const_ref2));

                                    if constexpr (std::is_same_v<TC, PartSketchConstraint::DistanceConstraint>){
                                        QString const_dist = "Distance: " + QString::number( c.value ) + " mm" ;
                                        contrainteType->appendRow(new QStandardItem(const_dist));
                                    }
                                }
                                else if constexpr (std::is_same_v<TC, PartSketchConstraint::VerticalConstraint> ||
                                                   std::is_same_v<TC, PartSketchConstraint::HorizontalConstraint>) {

                                    QString const_ref = "op:" + QString::number(c.ref.operationId) + "; primId:" + QString::number(c.ref.Id) + " " +
                                                        QString::fromStdString(op.getConstraintSubElementString(c.ref.subElement));

                                    contrainteType->appendRow(new QStandardItem(const_ref));
                                }
                                else if constexpr (std::is_same_v<TC, PartSketchConstraint::RadiusConstraint>) {

                                    QString const_ref1 = "op:" + QString::number(c.ref1.operationId) + "; primId:" + QString::number(c.ref1.Id) + " " +
                                                         QString::fromStdString(op.getConstraintSubElementString(c.ref1.subElement));

                                    contrainteType->appendRow(new QStandardItem(const_ref1));
                                }
                            }, constraint.data);

                            contraintesFolder->appendRow(contrainteType);
                        }

                        opItem->appendRow(contraintesFolder);
                    }
                }
            }
            else if constexpr (std::is_same_v<T, ExtrudeParams>) {
                if (true == m_DisplaySolids) {
                    QStandardItem* infoItem = new QStandardItem(QString(tr("Start : %1 mm")).arg(params.start));
                    infoItem->setEnabled(false);
                    opItem->appendRow(infoItem);

                    QStandardItem* infoItem2 = new QStandardItem(QString(tr("End : %1 mm")).arg(params.end));
                    infoItem2->setEnabled(false);
                    opItem->appendRow(infoItem2);

                    QStandardItem* boolItem = nullptr;
                    switch (params.EboolOp) {
                    default:
                        boolItem = new QStandardItem(QString(tr("bool : aucun")));
                        break;
                    case EBooleanOp::Substract:
                        boolItem = new QStandardItem(QString(tr("bool : substract")));
                        break;
                    case EBooleanOp::Union:
                        boolItem = new QStandardItem(QString(tr("bool : union")));
                        break;
                    case EBooleanOp::Intersect:
                        boolItem = new QStandardItem(QString(tr("bool : intersect")));
                        break;
                    }
                    if (boolItem != nullptr) {
                        boolItem->setEnabled(false);
                        opItem->appendRow(boolItem);
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

