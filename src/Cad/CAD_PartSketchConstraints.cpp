#include "CAD_PartSketchConstraints.h"







bool PartSketchConstraint::isEquivalentTo(const PartSketchConstraint& other) const {
	// 1. Si les types de contraintes diffèrent, elles ne sont pas équivalentes
	if (this->type != other.type) {
		return false;
	}

	// 2. Comparer les identités des entités ou points ciblés
	// (selon la façon dont vos contraintes stockent leurs références, ex: IDs de points ou de primitives)
	// Note : Il faut parfois gérer la symétrie (ex: une contrainte A->B équivaut à B->A pour certaines règles)

	return false;
}











