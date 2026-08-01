#include "CadTree_SketchModel.h"
#include "CAD_Part.h"
#include <QIcon>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QImageReader>

CadTree_SketchModel::CadTree_SketchModel(QObject* parent) : QStandardItemModel(parent) {
    // Définir l'en-tête de la colonne principale
    setHorizontalHeaderLabels({ tr("Arborescence du modèle") });
}

Qt::ItemFlags CadTree_SketchModel::flags(const QModelIndex& index) const {
    // 1. Récupère les flags de base générés par QStandardItemModel
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);
    // 2. Si l'index est valide, on retire le droit d'édition
    if (index.isValid()) {
        return defaultFlags & ~Qt::ItemIsEditable;
    }
    return defaultFlags;
}

void CadTree_SketchModel::refreshFromPart(const CAD_Part& part) {
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
        std::visit([this, opItem, op](const auto& params) {
            using T = std::decay_t<decltype(params)>;

            if constexpr (std::is_same_v<T, SketchParams>) {

                // On ajoute des sous-éléments pour les primitives de l'esquisse
                if (!params.getPrimitives().empty()) {
                    QStandardItem* primsFolder = new QStandardItem(tr("Primitives"));
                    primsFolder->setIcon(QIcon(":/icons/folder_geom.png"));
                    
                    for (const auto& primitive : params.getPrimitives()) {
                        std::visit([this, primsFolder, op](const auto& prim) {
                            using PType = std::decay_t<decltype(prim)>;
                            QString label;
                            if constexpr (std::is_same_v<PType, SketchLine>) label = QString("Ligne %1").arg(prim.id);
                            else if constexpr (std::is_same_v<PType, SketchCircle>) label = QString("Cercle %1 (R=%2)").arg(prim.id).arg(prim.radius);
                            else if constexpr (std::is_same_v<PType, SketchArc>) label = QString("Arc %1").arg(prim.id);

                            QStandardItem* primItem = new QStandardItem(label);
                            primItem->setIcon(QIcon(":/icons/geometry.png"));
                            // On stocke l'ID de l'opération parente ET de la primitive si besoin
                            primItem->setData(static_cast<qlonglong>(op.id), IdRole); 
                            primsFolder->appendRow(primItem);
                        }, primitive);
                    }
                    opItem->appendRow(primsFolder);
                }

            }
            else if constexpr (std::is_same_v<T, ExtrudeParams>) {

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
        }, op.getParams());

        // Ajouter l'opération complète à la racine de l'arbre
        this->appendRow(opItem);
    }
}

QIcon CadTree_SketchModel::getIconForOperation(const CadOperation& op) {
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

