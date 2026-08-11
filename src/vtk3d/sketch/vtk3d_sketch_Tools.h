#pragma once

#include <variant>
#include "vtk3d_sketch_ToolLine.h"
#include "vtk3d_sketch_ToolCircle.h"
#include "vtk3d_sketch_ToolSelect.h"
#include "vtk3d_sketch_ToolRectangle.h"
#include "vtk3d_sketch_ToolDimensions.h"
#include "vtk3d_sketch_ToolSetConstraints.h"




using Tooltype = std::variant<Tool_Select, Tool_LineDraw, Tool_CircleDraw, Tool_Rectangle, Tool_Dimensions, Tool_SetConstraints>;




