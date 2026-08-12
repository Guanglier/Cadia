#include "cad_events.h"



std::string CadEvent::Sketch::CadEvent_Sketch_ConstraintSubmode_To_String ( CadEvent::Sketch::ConstraintSubMode li_submode) {
	std::string l_string = "";
	switch ( li_submode ){
	default :
		l_string = " ERREUR default";
		break;
	case CadEvent::Sketch::ConstraintSubMode::Distance:
		l_string = " Distance";
		break;
	case CadEvent::Sketch::ConstraintSubMode::Horizontal:
		l_string = " Horizontal";
		break;
	case CadEvent::Sketch::ConstraintSubMode::Parallel:
		l_string = " Parallel";
		break;
	case CadEvent::Sketch::ConstraintSubMode::Perpendicular:
		l_string = " Perpendicular";
		break;
	case CadEvent::Sketch::ConstraintSubMode::Vertical:
		l_string = " Vertical";
		break;
	};
	return l_string;
}
