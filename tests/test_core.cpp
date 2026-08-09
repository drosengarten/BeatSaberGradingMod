#include "CutAccuracy/Geometry.hpp"
#include "CutAccuracy/Presentation.hpp"
#include "CutAccuracy/Scoring.hpp"
#include "CutAccuracy/Stats.hpp"
#include "CutAccuracy/Traversal.hpp"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace CutAccuracy;

static void require(bool condition, const char* expression) {
    if (!condition) {
        std::cerr << "Requirement failed: " << expression << "\n";
        std::abort();
    }
}

static void expectNear(double actual, double expected, double eps=1e-6) {
    if (std::abs(actual-expected) > eps) {
        std::cerr << "Expected " << expected << ", got " << actual << "\n";
        std::abort();
    }
}

int main() {
    // Geometry basics.
    expectNear(ConvexPolyhedron::unitCube().volume(), 1.0);
    expectNear(makeMiniNote({0,1,0}, true).volume(), 0.5);
    expectNear(makeMiniNote({1,1,0}, true).volume(), 0.5);
    expectNear(makeMiniNote({-1,1,0}, false).volume(), 0.5);

    // Central direction splits must remain exactly half-volume for arbitrary XY angles.
    std::mt19937 rng(123456u);
    std::uniform_real_distribution<double> angleDist(0.0, 6.283185307179586);
    std::uniform_real_distribution<double> offsetDist(-0.7, 0.7);
    for (int i=0; i<500; ++i) {
        const double a = angleDist(rng);
        Vec3 axis{std::cos(a), std::sin(a), 0.0};
        expectNear(makeMiniNote(axis, true).volume(), 0.5, 1e-6);
        expectNear(makeMiniNote(axis, false).volume(), 0.5, 1e-6);

        Vec3 saberN{std::cos(angleDist(rng)), std::sin(angleDist(rng)), 0.35};
        Plane p = Plane::throughPoint(saberN, {offsetDist(rng), offsetDist(rng), 0.0});
        for (bool positiveHalf : {false, true}) {
            auto v = cutMiniNoteVolumes(axis, positiveHalf, p);
            require((v.positiveSide >= -1e-8), "v.positiveSide >= -1e-8");
            require((v.negativeSide >= -1e-8), "v.negativeSide >= -1e-8");
            require((v.positiveSide <= v.total + 1e-7), "v.positiveSide <= v.total + 1e-7");
            expectNear(v.positiveSide + v.negativeSide, v.total, 1e-7);
            const double r = smallerRatio(v);
            require((r >= -1e-8 && r <= 0.5 + 1e-7), "r >= -1e-8 && r <= 0.5 + 1e-7");
        }
    }

    // A central vertical saber plane x=0 splits BOTH upper/lower mini-notes 50/50.
    Plane centeredVertical = Plane::throughPoint({1,0,0}, {0,0,0});
    auto top = cutMiniNoteVolumes({0,1,0}, true, centeredVertical);
    auto bottom = cutMiniNoteVolumes({0,1,0}, false, centeredVertical);
    expectNear(smallerRatio(top), 0.5);
    expectNear(smallerRatio(bottom), 0.5);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(top)), 25.0);
    auto topDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, true, centeredVertical);
    auto bottomDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, false, centeredVertical);
    expectNear(makeDepthSplitMiniNote({0,1,0}, true, false).volume(), 0.25);
    expectNear(makeDepthSplitMiniNote({0,1,0}, true, true).volume(), 0.25);
    expectNear(smallerRatio(topDepth.negativeDepth), 0.5);
    expectNear(smallerRatio(topDepth.positiveDepth), 0.5);
    expectNear(smallerRatio(bottomDepth.negativeDepth), 0.5);
    expectNear(smallerRatio(bottomDepth.positiveDepth), 0.5);
    expectNear(miniNoteScoreFromDepthSplitVolumes(topDepth), 25.0);
    expectNear(miniNoteScoreFromDepthSplitVolumes(bottomDepth), 25.0);

    // Plane x=0.1 through a unit-width cube gives 60/40 in BOTH mini-notes.
    Plane xOffset = Plane::throughPoint({1,0,0}, {0.1,0,0});
    top = cutMiniNoteVolumes({0,1,0}, true, xOffset);
    bottom = cutMiniNoteVolumes({0,1,0}, false, xOffset);
    expectNear(smallerRatio(top), 0.4, 1e-5);
    expectNear(smallerRatio(bottom), 0.4, 1e-5);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(top)), 24.375, 1e-5);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(bottom)), 24.375, 1e-5);
    topDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, true, xOffset);
    bottomDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, false, xOffset);
    expectNear(miniNoteScoreFromDepthSplitVolumes(topDepth), 24.375, 1e-5);
    expectNear(miniNoteScoreFromDepthSplitVolumes(bottomDepth), 24.375, 1e-5);

    // A saber plane angled through note depth can split the old upper/lower
    // mini-notes 50/50 overall, while still drifting from front to back.
    Plane depthAngled = Plane::throughPoint({1,0,1}, {0,0,0});
    top = cutMiniNoteVolumes({0,1,0}, true, depthAngled);
    bottom = cutMiniNoteVolumes({0,1,0}, false, depthAngled);
    expectNear(smallerRatio(top), 0.5, 1e-6);
    expectNear(smallerRatio(bottom), 0.5, 1e-6);
    topDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, true, depthAngled);
    bottomDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, false, depthAngled);
    expectNear(smallerRatio(topDepth.negativeDepth), 0.25, 1e-6);
    expectNear(smallerRatio(topDepth.positiveDepth), 0.25, 1e-6);
    expectNear(smallerRatio(bottomDepth.negativeDepth), 0.25, 1e-6);
    expectNear(smallerRatio(bottomDepth.positiveDepth), 0.25, 1e-6);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(top)), 25.0, 1e-6);
    expectNear(miniNoteScoreFromDepthSplitVolumes(topDepth), 20.25, 1e-6);
    expectNear(miniNoteScoreFromDepthSplitVolumes(bottomDepth), 20.25, 1e-6);

    // Direction axis offset and diagonal mapping regression.
    auto rightAxis = splitAxisForCutDirection(CutDirection::Right);
    expectNear(rightAxis.x, 1.0);
    expectNear(rightAxis.y, 0.0);
    auto rotatedRight = splitAxisForCutDirection(CutDirection::Right, 90.0);
    expectNear(rotatedRight.x, 0.0, 1e-6);
    expectNear(rotatedRight.y, 1.0, 1e-6);

    // Beat Saber scoring-type policy for CutAccuracy v0.11.
    require((scoreObjectRuleForScoringType(1).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(1).kind == ScoreObjectKind::FullNote");     // Normal
    require((scoreObjectRuleForScoringType(2).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(2).kind == ScoreObjectKind::FullNote");     // ArcHead
    require((scoreObjectRuleForScoringType(3).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(3).kind == ScoreObjectKind::FullNote");     // ArcTail
    require((scoreObjectRuleForScoringType(4).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(4).kind == ScoreObjectKind::FullNote");     // ChainHead
    require((scoreObjectRuleForScoringType(6).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(6).kind == ScoreObjectKind::FullNote");     // ArcHeadArcTail
    require((scoreObjectRuleForScoringType(7).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(7).kind == ScoreObjectKind::FullNote");     // ChainHeadArcTail
    require((scoreObjectRuleForScoringType(9).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(9).kind == ScoreObjectKind::FullNote");     // Defensive combined type
    require((scoreObjectRuleForScoringType(10).kind == ScoreObjectKind::FullNote), "scoreObjectRuleForScoringType(10).kind == ScoreObjectKind::FullNote");    // Defensive combined type
    expectNear(scoreObjectRuleForScoringType(4).maxScore, 100.0);
    require((scoreObjectRuleForScoringType(5).kind == ScoreObjectKind::ChainLink), "scoreObjectRuleForScoringType(5).kind == ScoreObjectKind::ChainLink");    // ChainLink
    require((scoreObjectRuleForScoringType(8).kind == ScoreObjectKind::ChainLink), "scoreObjectRuleForScoringType(8).kind == ScoreObjectKind::ChainLink");    // ChainLinkArcHead
    expectNear(scoreObjectRuleForScoringType(5).maxScore, 20.0);
    require((scoreObjectRuleForScoringType(0).kind == ScoreObjectKind::Excluded), "scoreObjectRuleForScoringType(0).kind == ScoreObjectKind::Excluded");     // NoScore
    require((scoreObjectRuleForScoringType(-1).kind == ScoreObjectKind::Excluded), "scoreObjectRuleForScoringType(-1).kind == ScoreObjectKind::Excluded");    // Ignore
    require((scoreObjectRuleForScoringType(99).kind == ScoreObjectKind::Excluded), "scoreObjectRuleForScoringType(99).kind == ScoreObjectKind::Excluded");    // Unknown/unhittable

    // Scoring curve requested by design.
    RawMeasurements m{0.5, 0.4, 60, 30, 0.100};
    auto s = score(m);
    expectNear(s.firstMini, 25);
    expectNear(s.secondMini, 24.375);
    expectNear(s.beforeSwing, 20);
    expectNear(s.afterSwing, 16.2);
    expectNear(s.speed, 10);
    expectNear(s.total(), 95.575);
    expectNear(speedScore(0.3), 8.1);
    expectNear(speedScore(0.15), 10);

    const auto easyWeights = scoreWeightsForDifficulty(DifficultyProfile::Easy);
    const auto normalWeights = scoreWeightsForDifficulty(DifficultyProfile::Normal);
    const auto hardWeights = scoreWeightsForDifficulty(DifficultyProfile::Hard);
    const auto expertWeights = scoreWeightsForDifficulty(DifficultyProfile::Expert);
    const auto expertPlusWeights = scoreWeightsForDifficulty(DifficultyProfile::ExpertPlus);
    expectNear(easyWeights.swingFullAngleDeg, 50.0);
    expectNear(normalWeights.swingFullAngleDeg, 60.0);
    expectNear(hardWeights.swingFullAngleDeg, 70.0);
    expectNear(expertWeights.swingFullAngleDeg, 80.0);
    expectNear(expertPlusWeights.swingFullAngleDeg, 90.0);
    expectNear(easyWeights.speedFullTimeSeconds, 0.200);
    expectNear(normalWeights.speedFullTimeSeconds, 0.150);
    expectNear(hardWeights.speedFullTimeSeconds, 0.125);
    expectNear(expertWeights.speedFullTimeSeconds, 0.100);
    expectNear(expertPlusWeights.speedFullTimeSeconds, 0.086);
    require((difficultyProfileFromIndex(-4) == DifficultyProfile::Normal), "invalid difficulty index should fall back to Normal");
    require((difficultyProfileFromIndex(4) == DifficultyProfile::ExpertPlus), "index 4 should be Expert+");
    require((std::string(difficultyProfileName(DifficultyProfile::ExpertPlus)) == "Expert+"), "Expert+ profile name");

    expectNear(qualityCurveValue(0.6, easyWeights), 0.93);
    expectNear(qualityCurveValue(0.6, normalWeights), 0.90);
    expectNear(qualityCurveValue(0.6, hardWeights), 0.84);
    expectNear(qualityCurveValue(0.6, expertWeights), 0.79);
    expectNear(qualityCurveValue(0.6, expertPlusWeights), 0.73);
    expectNear(qualityCurveValue(0.9, easyWeights), 0.996);
    expectNear(qualityCurveValue(0.9, normalWeights), 0.99);
    expectNear(qualityCurveValue(0.9, hardWeights), 0.984);
    expectNear(qualityCurveValue(0.9, expertWeights), 0.976);
    expectNear(qualityCurveValue(0.9, expertPlusWeights), 0.968);

    const double imperfectRatio = 0.25;
    expectNear(miniNoteScoreFromSmallerRatio(imperfectRatio, easyWeights), 21.5);
    expectNear(miniNoteScoreFromSmallerRatio(imperfectRatio, normalWeights), 20.25);
    expectNear(miniNoteScoreFromSmallerRatio(imperfectRatio, hardWeights), 18.25);
    expectNear(miniNoteScoreFromSmallerRatio(imperfectRatio, expertWeights), 16.5);
    expectNear(miniNoteScoreFromSmallerRatio(imperfectRatio, expertPlusWeights), 14.5);
    expectNear(speedScore(0.200, easyWeights), 10.0);
    require((speedScore(0.250, easyWeights) > speedScore(0.250, normalWeights)), "easy speed should be easier than normal");
    require((speedScore(0.250, normalWeights) > speedScore(0.250, hardWeights)), "hard speed should be harder than normal");
    require((speedScore(0.250, hardWeights) > speedScore(0.250, expertWeights)), "expert speed should be harder than hard");
    require((speedScore(0.250, expertWeights) > speedScore(0.250, expertPlusWeights)), "expert+ speed should be hardest");
    expectNear(swingScore(50.0, easyWeights.beforeSwingMax, easyWeights), 20.0);
    require((swingScore(50.0, expertPlusWeights.beforeSwingMax, expertPlusWeights) < 20.0), "expert+ swing should require more angle");

    // Score is bounded 0..100 under extreme/random inputs.
    std::uniform_real_distribution<double> ratioDist(-1.0, 2.0);
    std::uniform_real_distribution<double> degreeDist(-100.0, 500.0);
    std::uniform_real_distribution<double> timeDist(-0.2, 1.0);
    for (int i=0; i<10000; ++i) {
        const auto r = score({ratioDist(rng), ratioDist(rng), degreeDist(rng), degreeDist(rng), timeDist(rng)});
        require((r.total() >= -1e-8 && r.total() <= 100.0 + 1e-8), "r.total() >= -1e-8 && r.total() <= 100.0 + 1e-8");
    }

    // Misses count as zero in averages.
    SaberStats stats;
    stats.add({25,25,20,20,10});
    stats.addMiss();
    auto avg = stats.averages();
    expectNear(avg.accuracyPct, 50);
    expectNear(avg.firstMiniPct, 100);
    // Component rows are based on observed full-note cuts only; misses remain in raw/level totals.
    // Speed row is based on observed speed samples only.
    expectNear(avg.speedPct, 100);

    // Combined average must be note-weighted, not a simple left/right mean.
    SessionStats session;
    session.left.add({25,25,20,20,10});       // 100
    session.right.add({25,25,20,20,10});      // 100
    session.right.addMiss();                   // 0
    session.right.addMiss();                   // 0
    auto sessionAvg = session.averages();
    expectNear(sessionAvg.left.accuracyPct, 100.0);
    expectNear(sessionAvg.right.accuracyPct, 100.0/3.0);
    expectNear(sessionAvg.accuracyPct, 50.0); // (100 + 100 + 0 + 0) / 4 notes.



    // Combo-weighted level accuracy differs from raw accuracy.
    SaberStats comboStats;
    comboStats.add({25,25,20,20,10}, 8, 8); // 800/800
    comboStats.add({25,25,20,20,10}, 1, 8); // 100/800 after combo loss
    auto comboAvg = comboStats.averages();
    expectNear(comboAvg.rawAccuracyPct, 100.0);
    expectNear(comboAvg.levelAccuracyPct, 900.0 / 1600.0 * 100.0);

    // Unknown traversal is neutral for total scoring during headset validation,
    // but omitted from the speed-row denominator.
    auto unknownSpeed = scoreWithUnknownSpeed({0.5,0.5,60,60,0.0});
    expectNear(unknownSpeed.total(), 100.0);
    require((!unknownSpeed.speedObserved), "!unknownSpeed.speedObserved");
    SaberStats unknownSpeedStats;
    unknownSpeedStats.add(unknownSpeed);
    auto unknownAvg = unknownSpeedStats.averages();
    expectNear(unknownAvg.rawAccuracyPct, 100.0);
    require((unknownAvg.speedSamples == 0), "unknownAvg.speedSamples == 0");

    auto fullParts = beatSaberCutScoreParts({25,25,20,20,10});
    require((fullParts.centerDistance == 60), "fullParts.centerDistance == 60");
    require((fullParts.before == 20), "fullParts.before == 20");
    require((fullParts.after == 20), "fullParts.after == 20");
    require((fullParts.fixed == 0), "fullParts.fixed == 0");
    require((fullParts.centerDistance + fullParts.before + fullParts.after + fullParts.fixed == 100),
            "fullParts total should be 100");

    auto slowParts = beatSaberCutScoreParts({25,25,20,20,4.4});
    require((slowParts.centerDistance == 54), "slowParts.centerDistance == 54");
    require((slowParts.centerDistance + slowParts.before + slowParts.after + slowParts.fixed == 94),
            "slowParts total should round to 94");

    SaberStats roundedStats;
    roundedStats.add({24.4,24.4,20,19,10}); // fractional total 97.8, Beat Saber cut score 98.
    auto roundedAvg = roundedStats.averages();
    expectNear(roundedAvg.rawAccuracyPct, 98.0);
    expectNear(roundedAvg.levelAccuracyPct, 98.0);
    expectNear(roundedAvg.firstMiniPct, 97.6);

    // Chain links are fixed 20/0 objects. They affect RAW/LEVEL denominators,
    // but they do not pollute the upper/lower/swing/speed component rows.
    SaberStats linkStats;
    linkStats.addFixed(20, 20, 8, 8);      // hit chain link: full 20
    auto linkAvg = linkStats.averages();
    expectNear(linkAvg.rawAccuracyPct, 100.0);
    expectNear(linkAvg.levelAccuracyPct, 100.0);
    require((linkAvg.componentSamples == 0), "linkAvg.componentSamples == 0");
    require((linkAvg.speedSamples == 0), "linkAvg.speedSamples == 0");
    linkStats.addMissWeighted(20, 8);      // missed chain link: 0/20
    linkAvg = linkStats.averages();
    expectNear(linkAvg.rawAccuracyPct, 50.0);
    expectNear(linkAvg.levelAccuracyPct, 50.0);

    SaberStats mixedStats;
    mixedStats.add({25,25,20,20,10});      // full note 100/100
    mixedStats.addFixed(20, 20);           // chain link 20/20
    mixedStats.addMissWeighted(20);        // missed chain link 0/20
    auto mixedAvg = mixedStats.averages();
    expectNear(mixedAvg.rawAccuracyPct, 120.0 / 140.0 * 100.0);
    expectNear(mixedAvg.firstMiniPct, 100.0); // component rows still based on full-note samples only.
    require((mixedAvg.componentSamples == 1), "mixedAvg.componentSamples == 1");
    require((formatFixedScore(20, 20) == "20"), "formatFixedScore(20, 20) == \"20\"");
    require((formatFixedScore(31, 20) == "20"), "formatFixedScore(31, 20) == \"20\"");


    // Built-in score override conversion: CutAccuracy must never be shown as
    // custom points divided by Beat Saber's 115-point vanilla denominator. v0.11
    // true-internal mode stores both the CutAccuracy earned score and the
    // CutAccuracy max score, instead of projecting into vanilla max-score space.
    require((customInternalMaxScore(10000.0) == 10000), "customInternalMaxScore exact normal map");
    require((customInternalScoreFromCustomLevel(9500.0, 10000.0) == 9500), "customInternalScoreFromCustomLevel 95% normal map");
    expectNear(customInternalScoreFromCustomLevel(9500.0, 10000.0) / static_cast<double>(customInternalMaxScore(10000.0)) * 100.0, 95.0);
    expectNear(9500.0 / 11500.0 * 100.0, 82.6086956521739, 1e-9);
    require((customInternalMaxScore(140.0) == 140), "mixed chain-link custom max should stay custom, not vanilla 161");
    require((customInternalScoreFromCustomLevel(120.0, 140.0) == 120), "mixed chain-link custom score should stay custom");
    require((customInternalScoreFromCustomLevel(-5.0, 100.0) == 0), "negative custom earned clamps to zero");
    require((customInternalScoreFromCustomLevel(150.0, 100.0) == 100), "overfull custom earned clamps to custom max");
    // Legacy projection helper remains available as a fallback, but it is no
    // longer the primary Option B path.
    require((vanillaCompatibleScoreFromCustomLevel(9500.0, 10000.0, 11500) == 10925), "legacy vanilla projection helper");

    // HUD formatting regression: all requested categories are present, and
    // a saber with no samples displays -- rather than a misleading 0.0%.
    SessionStats emptySession;
    auto emptyHud = buildHudPresentation(emptySession);
    require((emptyHud.heading.find("--") != std::string::npos), "emptyHud.heading.find(\"--\") != std::string::npos");
    require((emptyHud.table.find("--") != std::string::npos), "emptyHud.table.find(\"--\") != std::string::npos");

    auto hud = buildHudPresentation(session);
    require((hud.heading.find("LEVEL ACC") != std::string::npos), "hud.heading.find(\"LEVEL ACC\") != std::string::npos");
    require((hud.heading.find("RAW ACC") != std::string::npos), "hud.heading.find(\"RAW ACC\") != std::string::npos");
    require((hud.table.find("Upper") != std::string::npos), "hud.table.find(\"Upper\") != std::string::npos");
    require((hud.table.find("Lower") != std::string::npos), "hud.table.find(\"Lower\") != std::string::npos");
    require((hud.table.find("Before") != std::string::npos), "hud.table.find(\"Before\") != std::string::npos");
    require((hud.table.find("After") != std::string::npos), "hud.table.find(\"After\") != std::string::npos");
    require((hud.table.find("Speed") != std::string::npos), "hud.table.find(\"Speed\") != std::string::npos");
    require((formatPerNoteScore({24.4,24.4,20,19,10}) == "98"), "formatPerNoteScore({24.4,24.4,20,19,10}) == \"98\"");

    // Traversal test: plane normal X moves from x=-1 through a centered unit cube to +1.
    OrientedBox box{};
    std::vector<SaberPlaneSample> samples;
    samples = {
        {0.0, {-1,0,0}, {1,0,0}},
        {0.2, {1,0,0}, {1,0,0}}
    };
    auto dt = traversalTimeSeconds(samples, box, 0.1);
    require((dt.has_value()), "dt.has_value()");
    expectNear(*dt, 0.1, 1e-6);

    samples = {
        {0.0, {-1,0,0}, {1,0,0}},
        {0.4, {1,0,0}, {1,0,0}}
    };
    dt = traversalTimeSeconds(samples, box, 0.2);
    require((dt.has_value()), "dt.has_value()");
    expectNear(*dt, 0.2, 1e-6);

    samples = {
        {0.0, {-1.2,0,0}, {1,0,0}},
        {0.2, {-0.9,0,0}, {1,0,0}}
    };
    dt = traversalTimeSeconds(samples, box, 0.1);
    require((!dt.has_value()), "!dt.has_value()");

    samples.clear();
    // x=-1 at t=0; x=+1 at t=.2 => intersection from x=-.5 to +.5 => .1 sec.
    for (int i=0;i<=20;++i) {
        double t = i * 0.01;
        double x = -1.0 + 10.0*t;
        samples.push_back({t, {x,0,0}, {1,0,0}});
    }
    dt = traversalTimeSeconds(samples, box, 0.1);
    require((dt.has_value()), "dt.has_value()");
    expectNear(*dt, 0.1, 1e-6);
    expectNear(speedScore(*dt), 10.0, 1e-6);

    // Same path at half the velocity takes 200 ms through the block => 5/10.
    samples.clear();
    for (int i=0;i<=40;++i) {
        double t = i * 0.01;
        double x = -1.0 + 5.0*t;
        samples.push_back({t, {x,0,0}, {1,0,0}});
    }
    dt = traversalTimeSeconds(samples, box, 0.2);
    require((dt.has_value()), "dt.has_value()");
    expectNear(*dt, 0.2, 1e-6);
    expectNear(speedScore(*dt), 9.643229166666666, 1e-6);

    std::cout << "All CutAccuracy core tests passed.\n";
    return 0;
}
