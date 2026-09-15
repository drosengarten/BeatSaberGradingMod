#pragma once
#include "CutAccuracy/Math.hpp"
#include "CutAccuracy/Traversal.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include <cmath>
namespace CutAccuracyQuest {
inline CutAccuracy::Vec3 ToCore(UnityEngine::Vector3 v){return {v.x,v.y,v.z};}
inline UnityEngine::Vector3 ToUnity(const CutAccuracy::Vec3&v){return {static_cast<float>(v.x),static_cast<float>(v.y),static_cast<float>(v.z)};}
inline CutAccuracy::Vec3 ProjectNotePlane(CutAccuracy::Vec3 v){v.z=0;return CutAccuracy::normalized(v);}
inline CutAccuracy::OrientedBox FreezeBox(UnityEngine::Transform*t){constexpr double h=.5;auto s=t->get_lossyScale();return{ToCore(t->get_position()),CutAccuracy::normalized(ToCore(t->get_right())),CutAccuracy::normalized(ToCore(t->get_up())),CutAccuracy::normalized(ToCore(t->get_forward())),{std::abs(s.x)*h,std::abs(s.y)*h,std::abs(s.z)*h}};}
inline CutAccuracy::Plane WorldPlaneToLocal(UnityEngine::Transform*t,UnityEngine::Vector3 point,UnityEngine::Vector3 normal){auto lp=t->InverseTransformPoint(point);auto ln=t->InverseTransformDirection(normal);return CutAccuracy::Plane::throughPoint(ToCore(ln),ToCore(lp));}
inline CutAccuracy::Vec3 WorldDirectionToLocalNotePlane(UnityEngine::Transform*t,const CutAccuracy::Vec3&w){if(!t)return{};return ProjectNotePlane(ToCore(t->InverseTransformDirection(ToUnity(w))));}
inline CutAccuracy::Vec3 LocalNoteUpWorld(UnityEngine::Transform*t){if(!t)return{};return ProjectNotePlane(ToCore(t->TransformDirection({0,1,0})));}
inline CutAccuracy::Vec3 LocalDirectionToNotePlane(UnityEngine::Transform*t,UnityEngine::Vector3 w){if(!t)return{};return ProjectNotePlane(ToCore(t->InverseTransformDirection(w)));}
}
