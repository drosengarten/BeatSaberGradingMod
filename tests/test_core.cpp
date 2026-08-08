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

    // Plane x=0.1 through a unit-width cube gives 60/40 in BOTH mini-notes.
    Plane xOffset = Plane::throughPoint({1,0,0}, {0.1,0,0});
    top = cutMiniNoteVolumes({0,1,0}, true, xOffset);
    bottom = cutMiniNoteVolumes({0,1,0}, false, xOffset);
    expectNear(smallerRatio(top), 0.4, 1e-5);
    expectNear(smallerRatio(bottom), 0.4, 1e-5);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(top)), 20.0, 1e-5);
    expectNear(miniNoteScoreFromSmallerRatio(smallerRatio(bottom)), 20.0, 1e-5);


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
    expectNear(s.secondMini, 20);
    expectNear(s.beforeSwing, 20);
    expectNear(s.afterSwing, 10);
    expectNear(s.speed, 10);
    expectNear(s.total(), 85);
    expectNear(speedScore(0.2), 5);
    expectNear(speedScore(0.05), 10);

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
    // x=-1 at t=0; x=+1 at t=.2 => intersection from x=-.5 to +.5 => .1 sec.
    for (int i=0;i<=20;++i) {
        double t = i * 0.01;
        double x = -1.0 + 10.0*t;
        samples.push_back({t, {x,0,0}, {1,0,0}});
    }
    auto dt = traversalTimeSeconds(samples, box, 0.1);
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
    expectNear(speedScore(*dt), 5.0, 1e-6);

    std::cout << "All CutAccuracy core tests passed.\n";
    return 0;
}
