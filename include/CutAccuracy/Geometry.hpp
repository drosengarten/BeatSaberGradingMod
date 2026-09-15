#pragma once
#include "CutAccuracy/Math.hpp"
#include <vector>
namespace CutAccuracy {
using Face=std::vector<Vec3>;
struct ConvexPolyhedron { std::vector<Face> faces; static ConvexPolyhedron unitCube(double halfExtent=0.5); ConvexPolyhedron clipped(const Plane& plane,double eps=1e-9)const; double volume()const; std::vector<Vec3> uniqueVertices(double eps=1e-8)const; };
struct MiniNoteVolumes { double positiveSide{0},negativeSide{0},total{0}; };
struct DepthSplitMiniNoteVolumes { MiniNoteVolumes negativeDepth; MiniNoteVolumes positiveDepth; };
enum class CutDirection { Up,Down,Left,Right,UpLeft,UpRight,DownLeft,DownRight,Any,None };
// Signed note-local axis. Positive half is always the Upper pair: the half toward
// the mapped cut direction/arrow. Negative half is the Lower pair. Therefore
// opposite note directions intentionally reverse Upper and Lower assignment.
Vec3 splitAxisForCutDirection(CutDirection direction);
Vec3 splitAxisForCutDirection(CutDirection direction,double angleOffsetDegrees);
bool hasDirectionalSplit(CutDirection direction);
ConvexPolyhedron makeMiniNote(const Vec3& directionAxis,bool positiveHalf);
ConvexPolyhedron makeDepthSplitMiniNote(const Vec3& directionAxis,bool positiveHalf,bool positiveDepthHalf);
MiniNoteVolumes cutPolyhedronVolumes(const ConvexPolyhedron& polyhedron,const Plane& saberCutPlaneLocal);
MiniNoteVolumes cutMiniNoteVolumes(const Vec3& directionAxis,bool positiveHalf,const Plane& saberCutPlaneLocal);
DepthSplitMiniNoteVolumes cutDepthSplitMiniNoteVolumes(const Vec3& directionAxis,bool positiveHalf,const Plane& saberCutPlaneLocal);
}
