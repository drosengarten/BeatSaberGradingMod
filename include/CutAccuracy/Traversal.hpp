#pragma once
#include "CutAccuracy/Math.hpp"
#include <optional>
#include <vector>
namespace CutAccuracy { struct OrientedBox{Vec3 center{},axisX{1,0,0},axisY{0,1,0},axisZ{0,0,1},halfExtent{.5,.5,.5};}; struct SaberPlaneSample{double timeSeconds{0};Vec3 point{},normal{};}; double planeBoxClearance(const SaberPlaneSample&,const OrientedBox&); std::optional<double> traversalTimeSeconds(const std::vector<SaberPlaneSample>&,const OrientedBox&,double,double=1e-6); }
