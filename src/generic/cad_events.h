

#pragma once

#include <string>
#include <variant>
#include <functional>
#include <cstdint>
#include "Dialog_SketchHelper.h"

// ============================================================================
// 1. MODULE ESQUISSE (SKETCH)
// ============================================================================
namespace CadEvent::Sketch {



    enum class ToolMode{
        Draw_line,
        Draw_Circle,
        Draw_Rectangle,
        Select,
        Dimensions,
        SetConstraints
    };
    enum class GeneralMessage{
        SketchChanged,
        SketchActivated
    };


    enum class LineSubMode { TwoPoints, Normal, Tangent };
    enum class CircleSubMode { CenterPoint };
    enum class ConstraintSubMode { Horizontal, Vertical, Parallel, Perpendicular, Distance };
    std::string CadEvent_Sketch_ConstraintSubmode_To_String ( ConstraintSubMode li_submode) ;
    enum class RectangleSubMode { ByEdges, ByCenter };
    using Tool_SubMode = std::variant<
        std::monostate, // Pour les outils sans sous-mode
        LineSubMode,
        RectangleSubMode,
        CircleSubMode,
        ConstraintSubMode
        >;

    //--------- commandes : QT vers couches inférieures ----------------
    struct CmdActivateTool {
        ToolMode toolMode;
        Tool_SubMode sub_mode;
    };
    struct CmdCancel {};
    struct CmdSetPrecisionValue { double value; };
    enum class CmdPopupToolBtnClicked { Btn_Cancel, Btn_Reset, Btn_OK };
    struct CmdPopupTool { CmdPopupToolBtnClicked btn; };

    //----------- réponses ou envoi vers QT -------
    struct RespStatus { std::string text; };
    struct RespDimensions { double length; double angle; };
    struct RespChangedTool { ToolMode toolMode;  Tool_SubMode sub_mode;};
    struct RespGeneralSignal {GeneralMessage message; };
    struct RespSelection { std::string text; };
    struct RespSendPopupDef { DialogSketchHelper::Helper popup_def;};
}



// ============================================================================
// 2. MODULE PART
// ============================================================================
namespace CadEvent::Part {
    enum class GeneralMessage{
        Activated
    };

    struct RespGeneralSignal {GeneralMessage message; };
}




// ============================================================================
// 2. MODULE ASSEMBLAGE (ASSEMBLY)
// ============================================================================
namespace CadEvent::Assembly {
    struct CmdInsertComponent { uint64_t componentId; };
    struct CmdAddMate { int mateType; uint64_t entity1; uint64_t entity2; };
    
    struct RespMateFailed { std::string reason; };
    struct RespInterferenceDetected { double volume; };
}

// ============================================================================
// 3. MODULE MISE EN PLAN (DRAWING / DRAFTING)
// ============================================================================
namespace CadEvent::Drawing {
    struct CmdInsertView { int viewOrientation; double scale; };
    struct CmdAddAutoDimensions {};
    
    struct RespOutdatedViews { bool needsRecompute; };
}

// ============================================================================
// 4. MESSAGES GLOBAUX DU Part / NOYAU (CORE)
// ============================================================================
namespace CadEvent::Core {
    struct CmdRecomputeModel {};
    struct CmdUndo {};
    struct CmdRedo {};
    
    struct RespProgress { int percentage; std::string stepName; };
}


//---------------------------------------------------------------------------
// commandes depuis QT vers les couches inférieures
using CadCommandParams = std::variant<
    std::monostate,
    // Core
    CadEvent::Core::CmdRecomputeModel,
    CadEvent::Core::CmdUndo,
    CadEvent::Core::CmdRedo,

    // Sketch
    CadEvent::Sketch::CmdActivateTool,
    CadEvent::Sketch::CmdCancel,
    CadEvent::Sketch::CmdSetPrecisionValue,
    CadEvent::Sketch::CmdPopupTool,


    // Assembly
    CadEvent::Assembly::CmdInsertComponent,
    CadEvent::Assembly::CmdAddMate,

    // Drawing
    CadEvent::Drawing::CmdInsertView,
    CadEvent::Drawing::CmdAddAutoDimensions
>;

struct CadCommandEvent {
    uint64_t PartId = 0;
    CadCommandParams params;
};




// Le super-variant remontant vers QT
using CadResponseParams = std::variant<
    std::monostate,
    CadEvent::Core::RespProgress,           // progression d'une opération en cours

    CadEvent::Sketch::RespStatus,           // affichage dans la barre de statuts d'un message
    CadEvent::Sketch::RespDimensions,
    CadEvent::Sketch::RespChangedTool,      //l'outil actif a été changé
    CadEvent::Sketch::RespGeneralSignal,
    CadEvent::Sketch::RespSelection,
    CadEvent::Sketch::RespSendPopupDef,

    CadEvent::Part::RespGeneralSignal


>;


struct CadResponseEvent {
    uint64_t PartId = 0;
    CadResponseParams params;
};

// Callbacks globaux
using CadCommandCallback  = std::function<void(const CadCommandEvent&)>;
using CadResponseCallback = std::function<void(const CadResponseEvent&)>;









