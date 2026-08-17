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

    // Central direction splits remain exactly half-volume for arbitrary XY angles.
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

    // Perfect centered cut: all four depth mini-notes are 50/50 and worth 25 each.
    Plane centeredVertical = Plane::throughPoint({1,0,0}, {0,0,0});
    auto topDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, true, centeredVertical);
    auto bottomDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, false, centeredVertical);
    expectNear(makeDepthSplitMiniNote({0,1,0}, true, false).volume(), 0.25);
    expectNear(makeDepthSplitMiniNote({0,1,0}, true, true).volume(), 0.25);
    expectNear(smallerRatio(topDepth.negativeDepth), 0.5);
    expectNear(smallerRatio(topDepth.positiveDepth), 0.5);
    expectNear(smallerRatio(bottomDepth.negativeDepth), 0.5);
    expectNear(smallerRatio(bottomDepth.positiveDepth), 0.5);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(topDepth.negativeDepth)), 25.0);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(topDepth.positiveDepth)), 25.0);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(bottomDepth.negativeDepth)), 25.0);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(bottomDepth.positiveDepth)), 25.0);
    expectNear(miniNoteScoreFromDepthSplitVolumes(topDepth), 25.0);
    expectNear(miniNoteScoreFromDepthSplitVolumes(bottomDepth), 25.0);

    // Linear accuracy: a 60/40 volume split scores 20/25, not a curved 24+ score.
    Plane xOffset = Plane::throughPoint({1,0,0}, {0.1,0,0});
    topDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, true, xOffset);
    bottomDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, false, xOffset);
    expectNear(smallerRatio(topDepth.negativeDepth), 0.4, 1e-5);
    expectNear(miniNoteScoreFromSmallerRatio(0.4), 20.0);
    expectNear(miniNoteScoreFromDepthSplitVolumes(topDepth), 20.0, 1e-5);
    expectNear(miniNoteScoreFromDepthSplitVolumes(bottomDepth), 20.0, 1e-5);

    // Depth drift is exposed by the four-mini-note model even when the old whole
    // upper/lower halves look centered overall.
    Plane depthAngled = Plane::throughPoint({1,0,1}, {0,0,0});
    auto topWhole = cutMiniNoteVolumes({0,1,0}, true, depthAngled);
    auto bottomWhole = cutMiniNoteVolumes({0,1,0}, false, depthAngled);
    expectNear(smallerRatio(topWhole), 0.5, 1e-6);
    expectNear(smallerRatio(bottomWhole), 0.5, 1e-6);
    topDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, true, depthAngled);
    bottomDepth = cutDepthSplitMiniNoteVolumes({0,1,0}, false, depthAngled);
    expectNear(smallerRatio(topDepth.negativeDepth), 0.25, 1e-6);
    expectNear(smallerRatio(topDepth.positiveDepth), 0.25, 1e-6);
    expectNear(smallerRatio(bottomDepth.negativeDepth), 0.25, 1e-6);
    expectNear(smallerRatio(bottomDepth.positiveDepth), 0.25, 1e-6);
    expectNear(miniNoteScoreFromDepthSplitVolumes(topDepth), 12.5, 1e-6);
    expectNear(miniNoteScoreFromDepthSplitVolumes(bottomDepth), 12.5, 1e-6);

    // Direction axis offset and diagonal mapping regression.
    auto rightAxis = splitAxisForCutDirection(CutDirection::Right);
    expectNear(rightAxis.x, 1.0);
    expectNear(rightAxis.y, 0.0);
    auto rotatedRight = splitAxisForCutDirection(CutDirection::Right, 90.0);
    expectNear(rotatedRight.x, 0.0, 1e-6);
    expectNear(rotatedRight.y, 1.0, 1e-6);

    // Beat Saber scoring-type policy.
    require((scoreObjectRuleForScoringType(1).kind == ScoreObjectKind::FullNote), "normal full note");
    require((scoreObjectRuleForScoringType(2).kind == ScoreObjectKind::FullNote), "arc head full note");
    require((scoreObjectRuleForScoringType(3).kind == ScoreObjectKind::FullNote), "arc tail full note");
    require((scoreObjectRuleForScoringType(4).kind == ScoreObjectKind::FullNote), "chain head full note");
    require((scoreObjectRuleForScoringType(6).kind == ScoreObjectKind::FullNote), "arc head/tail full note");
    require((scoreObjectRuleForScoringType(7).kind == ScoreObjectKind::FullNote), "chain head arc tail full note");
    require((scoreObjectRuleForScoringType(9).kind == ScoreObjectKind::FullNote), "defensive combined full note");
    require((scoreObjectRuleForScoringType(10).kind == ScoreObjectKind::FullNote), "defensive combined full note 10");
    expectNear(scoreObjectRuleForScoringType(4).maxScore, 100.0);
    require((scoreObjectRuleForScoringType(5).kind == ScoreObjectKind::ChainLink), "chain link");
    require((scoreObjectRuleForScoringType(8).kind == ScoreObjectKind::ChainLink), "chain link arc head");
    expectNear(scoreObjectRuleForScoringType(5).maxScore, 20.0);
    require((scoreObjectRuleForScoringType(0).kind == ScoreObjectKind::Excluded), "NoScore excluded");
    require((scoreObjectRuleForScoringType(-1).kind == ScoreObjectKind::Excluded), "Ignore excluded");
    require((scoreObjectRuleForScoringType(99).kind == ScoreObjectKind::Excluded), "unknown excluded");

    // Fixed swing targets and standard 70/30 split: 100 degrees before, 60 after.
    ScoreWeights weights{};
    expectNear(weights.beforeSwingFullAngleDeg, 100.0);
    expectNear(weights.afterSwingFullAngleDeg, 60.0);
    expectNear(beforeSwingScore(100.0, weights), 70.0);
    expectNear(afterSwingScore(60.0, weights), 30.0);
    expectNear(beforeSwingScore(50.0, weights), 35.0);
    expectNear(afterSwingScore(30.0, weights), 15.0);
    expectNear(beforeSwingScore(140.0, weights), 70.0);
    expectNear(afterSwingScore(90.0, weights), 30.0);

    // Three presets use swing-only, Beat Saber-style 87/13 weighting, and accuracy-only.
    NoteComponents blendExample{20.0, 20.0, 35.0, 15.0, 0}; // accuracy 80, swing 50.
    expectNear(blendExample.noteAccuracyScore(), 80.0);
    expectNear(blendExample.swingAngleScore(), 50.0);
    expectNear(blendExample.total(), 50.0); // Classic Feel
    blendExample.accuracyWeightPercent = 13;
    expectNear(blendExample.total(), 53.9); // Standard Beat Saber weighting
    blendExample.accuracyWeightPercent = 100;
    expectNear(blendExample.total(), 80.0); // Precision Mode
    blendExample.accuracyWeightPercent = 37;
    expectNear(blendExample.total(), 61.1);
    require((clampAccuracyWeightPercent(-10) == 0), "slider clamps below zero");
    require((clampAccuracyWeightPercent(101) == 100), "slider clamps above 100");

    // Raw-measurement scoring uses linear mini-note accuracy and 100/60 swing targets.
    RawMeasurements m{0.5, 0.4, 100.0, 30.0};
    auto scored = score(m, 50);
    expectNear(scored.firstMini, 25.0);
    expectNear(scored.secondMini, 20.0);
    expectNear(scored.noteAccuracyScore(), 90.0);
    expectNear(scored.beforeSwing, 70.0);
    expectNear(scored.afterSwing, 15.0);
    expectNear(scored.swingAngleScore(), 85.0);
    expectNear(scored.total(), 87.5);

    // Score remains bounded 0..100 at every slider setting under extreme inputs.
    std::uniform_real_distribution<double> ratioDist(-1.0, 2.0);
    std::uniform_real_distribution<double> degreeDist(-100.0, 500.0);
    std::uniform_int_distribution<int> weightDist(-50, 150);
    for (int i=0; i<10000; ++i) {
        const auto r = score({ratioDist(rng), ratioDist(rng), degreeDist(rng), degreeDist(rng)}, weightDist(rng));
        require((r.total() >= -1e-8 && r.total() <= 100.0 + 1e-8), "score bounded 0..100");
    }

    // Perfect full note in any blend is 100/100; a miss remains zero in totals.
    NoteComponents perfect{25,25,70,30,50};
    expectNear(perfect.total(), 100.0);
    SaberStats stats;
    stats.add(perfect);
    stats.addMiss();
    auto avg = stats.averages();
    expectNear(avg.accuracyPct, 50.0);
    expectNear(avg.firstMiniPct, 100.0);
    expectNear(avg.secondMiniPct, 100.0);
    expectNear(avg.beforeSwingPct, 100.0);
    expectNear(avg.afterSwingPct, 100.0);

    // Combined average is note-weighted, not a simple left/right mean.
    SessionStats session;
    session.left.add(perfect);
    session.right.add(perfect);
    session.right.addMiss();
    session.right.addMiss();
    auto sessionAvg = session.averages();
    expectNear(sessionAvg.left.accuracyPct, 100.0);
    expectNear(sessionAvg.right.accuracyPct, 100.0/3.0);
    expectNear(sessionAvg.accuracyPct, 50.0);

    // Combo-weighted level accuracy differs from raw accuracy.
    SaberStats comboStats;
    comboStats.add(perfect, 8, 8);
    comboStats.add(perfect, 1, 8);
    auto comboAvg = comboStats.averages();
    expectNear(comboAvg.rawAccuracyPct, 100.0);
    expectNear(comboAvg.levelAccuracyPct, 900.0 / 1600.0 * 100.0);

    // Built-in cut-score override encodes the already-blended integer across the
    // native 70/30 swing buckets. This keeps Beat Saber's swing-rating counter alive.
    auto fullParts = beatSaberCutScoreParts(perfect);
    require((fullParts.centerDistance == 0 && fullParts.before == 70 && fullParts.after == 30), "perfect custom score encoded as 70/30");
    require((fullParts.fixed == 0), "built-in fixed bucket unused");

    NoteComponents sixtyFive{20,20,35,15,50};
    auto partialParts = beatSaberCutScoreParts(sixtyFive);
    require((partialParts.centerDistance == 0 && partialParts.before == 65 && partialParts.after == 0), "partial custom score stored exactly");

    NoteComponents eightyFive{25,25,70,0,50}; // accuracy 100, swing 70 => 85 blended.
    auto upperBucketParts = beatSaberCutScoreParts(eightyFive);
    require((upperBucketParts.centerDistance == 0 && upperBucketParts.before == 70 && upperBucketParts.after == 15), "scores above 70 spill exactly into after bucket");
    require((upperBucketParts.before + upperBucketParts.after == roundedClampedToInt(eightyFive.total(), 0, 100)), "encoded buckets preserve exact /100 score");

    SaberStats roundedStats;
    NoteComponents fractional{24.4,24.4,70,30,50}; // acc 97.6, swing 100 => 98.8 -> 99 internal.
    roundedStats.add(fractional);
    auto roundedAvg = roundedStats.averages();
    expectNear(roundedAvg.rawAccuracyPct, 99.0);
    expectNear(roundedAvg.levelAccuracyPct, 99.0);
    expectNear(roundedAvg.firstMiniPct, 97.6);

    // Chain links stay fixed 20/0 and do not populate the four full-note HUD rows.
    SaberStats linkStats;
    linkStats.addFixed(20, 20, 8, 8);
    auto linkAvg = linkStats.averages();
    expectNear(linkAvg.rawAccuracyPct, 100.0);
    expectNear(linkAvg.levelAccuracyPct, 100.0);
    require((linkAvg.componentSamples == 0), "chain link has no component samples");
    linkStats.addMissWeighted(20, 8);
    linkAvg = linkStats.averages();
    expectNear(linkAvg.rawAccuracyPct, 50.0);
    expectNear(linkAvg.levelAccuracyPct, 50.0);

    SaberStats mixedStats;
    mixedStats.add(perfect);
    mixedStats.addFixed(20, 20);
    mixedStats.addMissWeighted(20);
    auto mixedAvg = mixedStats.averages();
    expectNear(mixedAvg.rawAccuracyPct, 120.0 / 140.0 * 100.0);
    expectNear(mixedAvg.firstMiniPct, 100.0);
    require((mixedAvg.componentSamples == 1), "only full note populates components");
    require((formatFixedScore(20, 20) == "20"), "fixed score format 20");
    require((formatFixedScore(31, 20) == "20"), "fixed score clamps");

    // Internal /100 score denominator remains custom rather than vanilla 115.
    require((customInternalMaxScore(10000.0) == 10000), "custom max exact");
    require((customInternalScoreFromCustomLevel(9500.0, 10000.0) == 9500), "custom score 95%");
    expectNear(customInternalScoreFromCustomLevel(9500.0, 10000.0) /
        static_cast<double>(customInternalMaxScore(10000.0)) * 100.0, 95.0);
    require((customInternalMaxScore(140.0) == 140), "mixed chain custom max");
    require((customInternalScoreFromCustomLevel(120.0, 140.0) == 120), "mixed chain custom score");
    require((customInternalScoreFromCustomLevel(-5.0, 100.0) == 0), "negative custom earned clamps");
    require((customInternalScoreFromCustomLevel(150.0, 100.0) == 100), "overfull custom earned clamps");
    require((vanillaCompatibleScoreFromCustomLevel(9500.0, 10000.0, 11500) == 10925), "legacy projection helper");

    // HUD has exactly the four requested metric rows. Missing saber metrics use one '-'.
    SessionStats emptySession;
    auto emptyHud = buildHudPresentation(emptySession);
    require((emptyHud.heading.find("-") != std::string::npos), "empty heading uses dash");
    require((emptyHud.table.find("-") != std::string::npos), "empty table uses dash");
    require((emptyHud.table.find("Speed") == std::string::npos), "speed row removed");

    auto hud = buildHudPresentation(session);
    require((hud.heading.find("LEVEL ACC") != std::string::npos), "LEVEL ACC present");
    require((hud.heading.find("RAW ACC") != std::string::npos), "RAW ACC present");
    require((hud.table.find("Upper") != std::string::npos), "Upper present");
    require((hud.table.find("Lower") != std::string::npos), "Lower present");
    require((hud.table.find("Before") != std::string::npos), "Before present");
    require((hud.table.find("After") != std::string::npos), "After present");
    require((hud.table.find("Speed") == std::string::npos), "Speed absent");
    require((formatPerNoteScore(sixtyFive) == "65"), "per-note score is blended /100 value");

    // Traversal utility remains host-tested as geometry infrastructure, but it no
    // longer contributes to scoring or HUD output.
    OrientedBox box{};
    std::vector<SaberPlaneSample> samples{
        {0.0, {-1,0,0}, {1,0,0}},
        {0.2, {1,0,0}, {1,0,0}}
    };
    auto dt = traversalTimeSeconds(samples, box, 0.1);
    require((dt.has_value()), "traversal utility still resolves interval");
    expectNear(*dt, 0.1, 1e-6);

    std::cout << "All CutAccuracy core tests passed.\n";
    return 0;
}
