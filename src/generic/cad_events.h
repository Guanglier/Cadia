

#pragma once

#include <string>
#include <variant>
#include <functional>
#include <cstdint>

// ============================================================================
// 1. MODULE ESQUISSE (SKETCH)
// ============================================================================
namespace CadEvent::Sketch {



    enum class CadEvent_SketchToolMode{
        Draw_line,
        Draw_Circle,
        Draw_RectEdges,
        Draw_RectCenter,
        Select,
        Dimensions,
        SetConstraints
    };
    enum class CadEvent_SketchGeneralMessage{
        SketchChanged,
        SketchActivated
    };
    enum class CadEvent_SketchConstraints{
        Constraint_Resolve,
        Set_Vertical,
        Set_Horizontal
    };
    struct CmdActivateTool {
        CadEvent_SketchToolMode toolMode;
        int sub_mode = 0;
    };
    struct CmdCancel {};
    struct CmdSetPrecisionValue { double value; };
    struct CmdConstraints { CadEvent_SketchConstraints cmd; };

    struct RespStatus { std::string text; };
    struct RespDimensions { double length; double angle; };
    struct RespChangedTool { CadEvent_SketchToolMode toolMode; };
    struct RespGeneralSignal {CadEvent_SketchGeneralMessage message; };
}

// ============================================================================
// 2. MODULE PART
// ============================================================================
namespace CadEvent::Part {
    enum class CadEvent_PartGeneralMessage{
        Activated
    };

    struct RespGeneralSignal {CadEvent_PartGeneralMessage message; };
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
// 4. MESSAGES GLOBAUX DU DOCUMENT / NOYAU (CORE)
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
    uint64_t documentId = 0;
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

    CadEvent::Part::RespGeneralSignal,

    CadEvent::Assembly::RespMateFailed,

    CadEvent::Drawing::RespOutdatedViews
>;

struct CadResponseEvent {
    uint64_t documentId = 0;
    CadResponseParams params;
};

// Callbacks globaux
using CadCommandCallback  = std::function<void(const CadCommandEvent&)>;
using CadResponseCallback = std::function<void(const CadResponseEvent&)>;






