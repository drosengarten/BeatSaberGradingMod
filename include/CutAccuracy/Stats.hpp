#pragma once
#include "CutAccuracy/Scoring.hpp"
#include <cstddef>
namespace CutAccuracy {
enum class SaberSide { Left, Right };
struct ComponentAverage { std::size_t samples{0}; double pct{0.0}; };
struct SaberAverages {
    std::size_t notes{0};
    double accuracyPct{0.0};
    double rawAccuracyPct{0.0};
    double levelAccuracyPct{0.0};
    ComponentAverage accuracy{};
    ComponentAverage precise{};
    ComponentAverage center{};
    ComponentAverage before{};
    ComponentAverage after{};
    ComponentAverage upperPair{};
    ComponentAverage lowerPair{};
};
struct SessionAverages { std::size_t notes{0}; double accuracyPct{0.0}; double rawAccuracyPct{0.0}; double levelAccuracyPct{0.0}; SaberAverages left{},right{}; };
class SaberStats {
public:
    void reset();
    void addScored(const ScoredNote& scored,int actualMultiplier=1,int maxMultiplier=1);
    void addFixed(double score,double objectMaxScore,int actualMultiplier=1,int maxMultiplier=1);
    void addMiss(double objectMaxScore,int maxMultiplier=1);
    SaberAverages averages() const;
    std::size_t notes()const{return notes_;} double rawEarned()const{return rawEarned_;} double rawMax()const{return rawMax_;} double levelEarned()const{return levelEarned_;} double levelMax()const{return levelMax_;}
private:
    std::size_t notes_{0};
    std::size_t accuracySamples_{0};
    double accuracySum_{0};
    std::size_t preciseSamples_{0},centerSamples_{0},beforeSamples_{0},afterSamples_{0},upperSamples_{0},lowerSamples_{0};
    double preciseSum_{0},centerSum_{0},beforeSum_{0},afterSum_{0},upperSum_{0},lowerSum_{0};
    double rawEarned_{0},rawMax_{0},levelEarned_{0},levelMax_{0};
};
struct SessionStats {
    SaberStats left,right;
    void reset(){left.reset();right.reset();}
    SaberStats& forSide(SaberSide s){return s==SaberSide::Left?left:right;} const SaberStats& forSide(SaberSide s)const{return s==SaberSide::Left?left:right;}
    double rawEarned()const{return left.rawEarned()+right.rawEarned();} double rawMax()const{return left.rawMax()+right.rawMax();} double levelEarned()const{return left.levelEarned()+right.levelEarned();} double levelMax()const{return left.levelMax()+right.levelMax();}
    SessionAverages averages()const;
};
}
