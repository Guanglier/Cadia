

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
        Draw_RectEdges,
        Draw_RectCenter,
        Select,
        Dimensions,
        SetConstraints
    };
    enum class GeneralMessage{
        SketchChanged,
        SketchActivated
    };
    enum class Constraints{
        Constraint_Resolve,
        Set_Vertical,
        Set_Horizontal,
        Set_Dimension
    };
    struct CmdActivateTool {
        ToolMode toolMode;
        int sub_mode = 0;
    };
    struct CmdCancel {};
    struct CmdSetPrecisionValue { double value; };
    struct CmdConstraints { Constraints cmd; };

    //----------- réponses ou envoi vers qt -------
    struct RespStatus { std::string text; };
    struct RespDimensions { double length; double angle; };
    struct RespChangedTool { ToolMode toolMode; };
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



// Le super-variant qui peut transporter N'IMPORTE QUEL message de l'application
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
    CadEvent::Sketch::CmdConstraints,

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

// Le super-variant remontant
using CadResponseParams = std::variant<
    std::monostate,
    CadEvent::Core::RespProgress,

    CadEvent::Sketch::RespStatus,
    CadEvent::Sketch::RespDimensions,
    CadEvent::Sketch::RespChangedTool,
    CadEvent::Sketch::RespGeneralSignal,
    CadEvent::Sketch::RespSelection,
    CadEvent::Sketch::RespSendPopupDef,

    CadEvent::Part::RespGeneralSignal,

    CadEvent::Assembly::RespMateFailed,

    CadEvent::Drawing::RespOutdatedViews
>;

struct CadResponseEvent {
    uint64_t PartId = 0;
    CadResponseParams params;
};

// Callbacks globaux
using CadCommandCallback  = std::function<void(const CadCommandEvent&)>;
using CadResponseCallback = std::function<void(const CadResponseEvent&)>;






