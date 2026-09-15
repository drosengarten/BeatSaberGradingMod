#include "CutAccuracy/Geometry.hpp"
#include "CutAccuracy/Presentation.hpp"
#include "CutAccuracy/Scoring.hpp"
#include "CutAccuracy/Stats.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
using namespace CutAccuracy;
static void req(bool c,const char*m){if(!c){std::cerr<<"FAIL: "<<m<<"\n";std::abort();}}
static void near(double a,double b,double e=1e-6){if(std::abs(a-b)>e){std::cerr<<"Expected "<<b<<" got "<<a<<"\n";std::abort();}}
int main(){

  // Leaderboard safety contract: Simple and Advanced use custom scoring, so
  // BeatLeader/ScoreSaber-compatible MetaCore submission must be blocked.
  // Off is vanilla Beat Saber scoring and releases CutAccuracy's block.
  req(scoreSubmissionAllowedForMode(ScoringMode::Off),"vanilla mode allows external score submission");
  req(!scoreSubmissionAllowedForMode(ScoringMode::Simple),"simple custom scoring disables external score submission");
  req(!scoreSubmissionAllowedForMode(ScoringMode::Advanced),"advanced custom scoring disables external score submission");
  req(!customScoringActiveForMode(ScoringMode::Off),"off mode leaves vanilla scoring active");
  req(customScoringActiveForMode(ScoringMode::Simple)&&customScoringActiveForMode(ScoringMode::Advanced),"custom modes run CutAccuracy scoring");

  // Exercise the exact profile edits used by the per-note method selectors
  // through the scoring function consumed by CommitGood in the Quest hook.
  for(std::size_t i=0;i<kProfileCount;++i){
    auto profiles=defaultAdvancedConfig();
    auto& p=profiles.profiles[i];
    p.flat={false,0};p.before={true,20,100};p.after={true,20,60};
    setAccuracyWeight(p,60);
    ScoringInput input{};input.preciseAvailable=true;input.mini={1,1,0,0};
    input.centerQuality=.2;input.beforeSwingDeg=100;input.afterSwingDeg=60;
    setAccuracyMethod(p,ProfileAccuracyMethod::BeatSaber);
    near(scoreValidCut(input,p).totalScore,p.maxScore*.52);
    req(!p.precise.enabled && p.center.enabled,"Beat Saber uses center only");
    setAccuracyMethod(p,ProfileAccuracyMethod::Precise);
    near(scoreValidCut(input,p).totalScore,p.maxScore*.70);
    req(p.precise.enabled && !p.center.enabled,"Precise enables geometry capture");
    p.precise.upperPairWeightPct=100;
    near(scoreValidCut(input,p).totalScore,p.maxScore);
    p.precise.upperPairWeightPct=50;p.preciseMixPct=50;
    setAccuracyMethod(p,ProfileAccuracyMethod::Blended);
    near(scoreValidCut(input,p).totalScore,p.maxScore*.61);
    req(p.precise.enabled && p.center.enabled,"Blend consumes both measurements");
    req(accuracyWeight(p)==60 && p.before.weightPct==20 && p.after.weightPct==20,"method switch preserves weight budget");
    setAccuracyWeight(p,100);
    req(accuracyWeight(p)==60,"combined accuracy cannot exceed remaining budget");
    for(int total=0;total<=60;++total)for(int mix=0;mix<=100;++mix){
      p.preciseMixPct=mix;setAccuracyWeight(p,total);
      req(accuracyWeight(p)==total && enabledWeightSum(p)<=100,"blend rounding conserves total");
    }
    setAccuracyWeight(p,0);setAccuracyMethod(p,ProfileAccuracyMethod::Precise);
    req(p.accuracyMethod==ProfileAccuracyMethod::Precise && accuracyWeight(p)==0,"zero weight keeps method selection");
    setAccuracyWeight(p,10);req(p.precise.weightPct==10,"restored weight uses selected method");
    const auto other=(i+1)%kProfileCount;
    req(profiles.profiles[other].accuracyMethod==ProfileAccuracyMethod::BeatSaber,"editing one note type leaves others alone");
  }

  near(ConvexPolyhedron::unitCube().volume(),1.0);
  near(makeDepthSplitMiniNote({0,1,0},true,false).volume(),.25);
  near(makeDepthSplitMiniNote({0,1,0},true,true).volume(),.25);

  // Upper/Lower assignment is SIGNED by note direction. Opposites reverse.
  auto up=splitAxisForCutDirection(CutDirection::Up),down=splitAxisForCutDirection(CutDirection::Down);
  near(up.y,1); near(down.y,-1); near(dot(up,down),-1);
  auto left=splitAxisForCutDirection(CutDirection::Left),right=splitAxisForCutDirection(CutDirection::Right);
  near(left.x,-1);near(right.x,1);
  auto ul=splitAxisForCutDirection(CutDirection::UpLeft),dr=splitAxisForCutDirection(CutDirection::DownRight);
  near(dot(ul,dr),-1,1e-6);
  // Positive half literally follows the arrow: Up-positive is physical +Y, Down-positive is physical -Y.
  auto upUpper=makeMiniNote(up,true).uniqueVertices();
  auto downUpper=makeMiniNote(down,true).uniqueVertices();
  for(const auto& v:upUpper) req(v.y>=-1e-8,"Up Upper occupies +Y half");
  for(const auto& v:downUpper) req(v.y<=1e-8,"Down Upper occupies -Y half");

  // Four independent mini-notes remain four independent measurements.
  Plane centered=Plane::throughPoint({1,0,0},{0,0,0});
  auto upper=cutDepthSplitMiniNoteVolumes(up,true,centered);
  auto lower=cutDepthSplitMiniNoteVolumes(up,false,centered);
  auto four=fourMiniQualityFromVolumes(upper,lower);
  near(four.upperNegativeDepth,1);near(four.upperPositiveDepth,1);near(four.lowerNegativeDepth,1);near(four.lowerPositiveDepth,1);
  near(four.preciseQuality(50),1);

  // Pair weighting affects Upper vs Lower PAIRS only, not the two depth minis inside each pair.
  FourMiniNoteQuality q{1.0,.6,.2,.2}; // Upper pair .8, Lower pair .2
  near(q.upperPairQuality(),.8);near(q.lowerPairQuality(),.2);
  near(q.preciseQuality(50),.5);
  near(q.preciseQuality(75),.65);
  near(q.preciseQuality(100),.8);
  near(q.preciseQuality(0),.2);

  // Weight edits never exceed the currently available budget and never move another slider.
  SimpleConfig simple{};
  setSimpleWeight(simple,ComponentId::Precise,80);
  req(simple.accuracyWeightPct==60&&simple.beforeWeightPct==20&&simple.afterWeightPct==20,"simple accuracy capped by other weights");
  simple={}; simple.accuracyWeightPct=50;simple.beforeWeightPct=30;simple.afterWeightPct=20;
  setSimpleWeight(simple,ComponentId::Precise,40);
  req(simple.accuracyWeightPct==40&&simple.beforeWeightPct==30&&simple.afterWeightPct==20,"simple decrease leaves others untouched");
  req(simpleWeightMaxPct(simple,ComponentId::Precise)==50,"simple slider max is 100 minus others");

  // Simple Precise: all four minis feed Precise, with pair split above them.
  simple={}; simple.fullNoteMax=100; simple.accuracyMethod=AccuracyMethod::Precise; simple.accuracyWeightPct=60; simple.beforeWeightPct=20; simple.afterWeightPct=20; simple.preciseUpperPairWeightPct=75;
  auto sp=profileFromSimple(simple,ProfileKind::DirectionalNote);
  ScoringInput in{};in.preciseAvailable=true;in.mini=q;in.beforeSwingDeg=60;in.afterSwingDeg=60;
  auto s=scoreValidCut(in,sp);
  near(s.preciseQuality,.65); near(s.precisePoints,39); near(s.beforePoints,20); near(s.afterPoints,20); near(s.totalScore,79);

  // Beat Saber accuracy swaps only the accuracy method, preserving weights.
  simple.accuracyMethod=AccuracyMethod::BeatSaber;
  sp=profileFromSimple(simple,ProfileKind::DirectionalNote);
  in.centerQuality=.5;
  s=scoreValidCut(in,sp);
  near(s.centerPoints,30); near(s.totalScore,70);
  req(!s.preciseEnabled&&s.centerEnabled,"Beat Saber center enabled only");

  // Beat Saber scoring types route to independent profiles; normal dot notes split from directional notes.
  req(profileKindForScoringType(1,false)==ProfileKind::DirectionalNote,"normal directional profile");
  req(profileKindForScoringType(1,true)==ProfileKind::DotNote,"normal dot profile");
  req(profileKindForScoringType(2)==ProfileKind::ArcHead,"arc head profile");
  req(profileKindForScoringType(3)==ProfileKind::ArcTail,"arc tail profile");
  req(profileKindForScoringType(4)==ProfileKind::ChainHead,"chain head profile");
  req(profileKindForScoringType(5)==ProfileKind::ChainLink,"chain link profile");
  req(profileKindForScoringType(6)==ProfileKind::ArcHeadArcTail,"combined profile 6");
  req(profileKindForScoringType(7)==ProfileKind::ChainHeadArcTail,"combined profile 7");
  req(profileKindForScoringType(8)==ProfileKind::ChainLinkArcHead,"combined profile 8");
  req(profileKindForScoringType(9)==ProfileKind::ChainHeadArcHead,"combined profile 9");
  req(profileKindForScoringType(10)==ProfileKind::ChainHeadArcHeadArcTail,"combined profile 10");

  // Default mode's Advanced profile set is Beat Saber-equivalent by scoring type.
  auto adv=defaultAdvancedConfig();
  auto vanillaDir=adv.profile(ProfileKind::DirectionalNote);
  near(vanillaDir.maxScore,115);req(vanillaDir.flat.weightPct==0,"vanilla flat off");req(!vanillaDir.precise.enabled,"vanilla precise off");req(vanillaDir.center.enabled&&vanillaDir.before.enabled&&vanillaDir.after.enabled,"vanilla normal components");
  req(vanillaDir.center.weightPct==13&&vanillaDir.before.weightPct==61&&vanillaDir.after.weightPct==26,"rounded normal weights");
  near(vanillaDir.before.fullCreditAngleDeg,100);near(vanillaDir.after.fullCreditAngleDeg,60);
  for(std::size_t i=0;i<kProfileCount;++i){
    const auto& dp=adv.profiles[i];
    req(enabledWeightSum(dp)<=100,"default profile weight budget <=100");
    if(enabledWeightSum(dp)>0) req(enabledWeightSum(dp)==100,"default profile weight budget fully allocated");
  }
  auto vanillaArcHead=adv.profile(ProfileKind::ArcHead);near(vanillaArcHead.maxScore,115);req(vanillaArcHead.flat.weightPct==26&&vanillaArcHead.center.weightPct==13&&vanillaArcHead.before.weightPct==61,"rounded arc-head weights");req(!vanillaArcHead.after.enabled,"arc head fixed after becomes flat weight");
  auto vanillaArcTail=adv.profile(ProfileKind::ArcTail);near(vanillaArcTail.maxScore,115);req(vanillaArcTail.flat.weightPct==61&&vanillaArcTail.center.weightPct==13&&vanillaArcTail.after.weightPct==26,"rounded arc-tail weights");req(!vanillaArcTail.before.enabled,"arc tail fixed before becomes flat weight");
  near(adv.profile(ProfileKind::ChainHead).maxScore,85);near(adv.profile(ProfileKind::ChainLink).maxScore,20);req(adv.profile(ProfileKind::ChainLink).flat.weightPct==100,"chain link flat 100%");
  req(!hasMeasuredComponents(adv.profile(ProfileKind::ChainLink)),"default chain link needs no measurement carrier");
  req(hasMeasuredComponents(adv.profile(ProfileKind::DirectionalNote)),"normal note needs measurement carrier");
  req(usesNativeFixedCarrier(ProfileKind::ChainLink,adv.profile(ProfileKind::ChainLink)),"flat-only chain link keeps native fixed carrier");
  req(!usesNativeFixedCarrier(ProfileKind::DirectionalNote,adv.profile(ProfileKind::DirectionalNote)),"normal note uses measurement carrier");
  auto measuredChain=adv.profile(ProfileKind::ChainLink);setComponentWeight(measuredChain,ComponentId::Flat,80);setComponentWeight(measuredChain,ComponentId::Precise,20);req(!usesNativeFixedCarrier(ProfileKind::ChainLink,measuredChain),"measured chain switches to measurement carrier");
  req(adv.profile(ProfileKind::ArcHeadArcTail).flat.weightPct==87,"arc head+tail flat 87%");req(adv.profile(ProfileKind::ChainHeadArcTail).flat.weightPct==87,"chain head+tail flat 87%");req(adv.profile(ProfileKind::ChainHeadArcHeadArcTail).flat.weightPct==87,"combined flat 87%");

  // Rounded Beat Saber-like profiles reproduce their documented whole-percent arithmetic.
  ScoringInput vanillaInput{};vanillaInput.centerQuality=.5;vanillaInput.beforeSwingDeg=50;vanillaInput.afterSwingDeg=30;
  near(scoreValidCut(vanillaInput,adv.profile(ProfileKind::DirectionalNote)).totalScore,57.5,1e-6); // rounded weights still sum to 100%, so uniform 50% quality remains 57.5
  near(scoreValidCut(vanillaInput,adv.profile(ProfileKind::ArcHead)).totalScore,72.45,1e-6); // 115*(26% + 13%*.5 + 61%*.5)
  near(scoreValidCut(vanillaInput,adv.profile(ProfileKind::ArcTail)).totalScore,92.575,1e-6); // 115*(61% + 13%*.5 + 26%*.5)
  near(scoreValidCut(vanillaInput,adv.profile(ProfileKind::ChainHead)).totalScore,42.5,1e-6); // 85*(18%*.5 + 82%*.5)

  // Every advanced note profile is independent, including max score and angles.
  auto& dir=adv.profile(ProfileKind::DirectionalNote);
  auto& dotp=adv.profile(ProfileKind::DotNote);
  dir.maxScore=115; dir.before.fullCreditAngleDeg=100; dir.after.fullCreditAngleDeg=60;
  dotp.maxScore=80; dotp.before.fullCreditAngleDeg=30; dotp.after.fullCreditAngleDeg=30;
  req(adv.profile(ProfileKind::DirectionalNote).maxScore==115,"directional max independent");
  req(adv.profile(ProfileKind::DotNote).maxScore==80,"dot max independent");
  near(adv.profile(ProfileKind::DirectionalNote).before.fullCreditAngleDeg,100);
  near(adv.profile(ProfileKind::DotNote).before.fullCreditAngleDeg,30);

  // Flat is a weight in the same direct max-score budget as every other component.
  ScoringProfile p{};p.maxScore=150;p.flat={true,20};p.precise={true,40,50};p.center={true,10};p.before={true,20,60};p.after={true,10,60};
  in={};in.preciseAvailable=true;in.mini={1,1,1,1};in.centerQuality=1;in.beforeSwingDeg=60;in.afterSwingDeg=60;
  s=scoreValidCut(in,p);near(s.flatPoints,30);near(s.precisePoints,60);near(s.centerPoints,15);near(s.beforePoints,30);near(s.afterPoints,15);near(s.totalScore,150);

  // Component edits never redistribute other weights; each slider max is the remaining budget.
  p.flat={false,0};p.precise={true,50,50};p.center={false,0};p.before={true,25,60};p.after={true,25,60};
  setComponentEnabled(p,ComponentId::Center,true,20);
  near(p.center.weightPct,0);near(componentWeightMaxPct(p,ComponentId::Center),0);
  setComponentWeight(p,ComponentId::Before,10);
  near(p.before.weightPct,10);near(p.precise.weightPct,50);near(p.after.weightPct,25);near(p.center.weightPct,0);
  near(componentWeightMaxPct(p,ComponentId::Center),15);
  setComponentWeight(p,ComponentId::Center,50);
  near(p.center.weightPct,15);
  near(enabledWeightSum(p),100);
  // Flat uses the same live maximum rule as every other component.
  setComponentWeight(p,ComponentId::Before,0);
  req(!p.before.enabled,"zero weight is canonical OFF state");
  setComponentWeight(p,ComponentId::Before,1);
  req(p.before.enabled&&p.before.weightPct==1,"positive weight re-enables component");
  setComponentWeight(p,ComponentId::Before,0);
  setComponentWeight(p,ComponentId::Flat,10);
  req(p.flat.weightPct==10,"flat weight editable");
  req(componentWeightMaxPct(p,ComponentId::Flat)==10,"flat slider max is remaining budget");

  // UI safety contract: both an oversized slider request and repeated + steps
  // are clamped by the scoring setter, so total weight can never exceed 100%.
  setComponentWeight(p,ComponentId::Flat,999);
  req(p.flat.weightPct==10,"oversized slider request clamps to remaining budget");
  req(enabledWeightSum(p)==100,"oversized slider request cannot exceed 100 total");
  for(int i=0;i<20;++i) setComponentWeight(p,ComponentId::Flat,p.flat.weightPct+1);
  req(p.flat.weightPct==10,"repeated plus steps stop at remaining budget");
  req(enabledWeightSum(p)==100,"plus steps cannot exceed 100 total");

  // Failure rules are per profile and respect each profile's max score.
  p.maxScore=200;p.badCut={FailureMode::PercentOfMax,10};p.miss={FailureMode::FixedPoints,7};
  near(failureScore(p.badCut,p.maxScore),20);near(failureScore(p.miss,p.maxScore),7);
  p.badCut={FailureMode::PercentOfMax,140};p.miss={FailureMode::FixedPoints,250};
  normalizeFailureRule(p.badCut,p.maxScore);normalizeFailureRule(p.miss,p.maxScore);
  near(p.badCut.value,100);near(p.miss.value,200);

  // Chain link is an ordinary profile whose default binary behavior emerges from Flat=100%.
  auto chain=profileFromSimple(SimpleConfig{},ProfileKind::ChainLink);
  near(chain.maxScore,20);req(chain.flat.weightPct==100,"chain flat is 100%");req(!chain.precise.enabled&&!chain.before.enabled&&!chain.after.enabled,"binary chain defaults");
  s=scoreValidCut({},chain);near(s.totalScore,20);

  // Arbitrary maxima flow through stats without normalization back to /100 points.
  SaberStats stats;ScoringProfile huge=p;huge.maxScore=500;huge.flat={false,0};huge.precise={true,100,50};huge.center={false,0};huge.before={false,0,60};huge.after={false,0,60};
  in={};in.preciseAvailable=true;in.mini={.8,.8,.8,.8};s=scoreValidCut(in,huge);near(s.totalScore,400);stats.addScored(s,1,1);near(stats.rawEarned(),400);near(stats.rawMax(),500);near(stats.averages().rawAccuracyPct,80);
  auto scaled=vanillaLikeProfile(100,0,40,30,30);
  in={};in.centerQuality=.75;in.beforeSwingDeg=50;in.afterSwingDeg=30;
  auto scaled100=scoreValidCut(in,scaled);
  scaled.maxScore=500;
  auto scaled500=scoreValidCut(in,scaled);
  near(scaled100.normalizedScore(),scaled500.normalizedScore());
  near(scaled500.totalScore,scaled100.totalScore*5.0);

  // Native carrier is normalized only; custom absolute score is independent.
  auto parts=beatSaberCutScoreParts(s);req(parts.before==70&&parts.after==10,"80 percent carrier = 70+10");
  auto carrier100=beatSaberMeasurementCarrier(100);
  req(carrier100.centerDistanceMax+carrier100.beforeMax+carrier100.afterMax==100,"custom max 100 carrier exposes max 100 to Beat Saber");
  auto parts100=beatSaberCutScoreParts(scaled100,carrier100);
  req(parts100.centerDistance+parts100.before+parts100.after==static_cast<int>(std::lround(scaled100.normalizedScore()*100.0)),"custom max 100 carrier encodes matching note score");
  auto fixed100=beatSaberFixedCarrier(100);
  req(fixed100.fixed==100&&fixed100.maxScore()==100,"flat-only carrier exposes custom fixed max");


  // Per-cut carrier snapshots prevent a shared Beat Saber score definition from
  // leaking one note type's denominator into another note type's finish math.
  auto carrier115=beatSaberMeasurementCarrier(115);
  auto carrier80=beatSaberMeasurementCarrier(80);
  auto parts115=beatSaberCutScoreParts(scaled100,carrier115);
  auto parts80=beatSaberCutScoreParts(scaled100,carrier80);
  req(parts115.centerDistance+parts115.before+parts115.after==static_cast<int>(std::lround(scaled100.normalizedScore()*115.0)),"carrier 115 encodes against its own max");
  req(parts80.centerDistance+parts80.before+parts80.after==static_cast<int>(std::lround(scaled100.normalizedScore()*80.0)),"carrier 80 encodes against its own max");
  req(carrier115.maxScore()!=carrier80.maxScore(),"independent carriers keep different note-type maxima separate");

  // HUD contains dynamic components and optional four-mini diagnostic pair rows.
  SessionStats session;session.left.addScored(s);
  for(auto mode:{ScoringMode::Simple,ScoringMode::Advanced})for(bool diagnostics:{false,true}){
    auto hud=buildHudPresentation(session,mode,AccuracyMethod::Precise,diagnostics);
    req(hud.table.find("Before swing")!=std::string::npos && hud.table.find("After swing")!=std::string::npos && hud.table.find("Accuracy")!=std::string::npos,"three HUD metrics");
    req(std::count(hud.table.begin(),hud.table.end(),'\n')==3,"exactly three HUD rows");
    req(hud.table.find("Upper")==std::string::npos && hud.table.find("Precise")==std::string::npos,"no extra diagnostic rows");
    req(hud.heading.find("LEVEL")==std::string::npos && hud.heading.find("RAW")==std::string::npos,"no extra total scores");
  }
  {
    SessionStats mixed;
    auto profile=vanillaLikeProfile(100,0,100,0,0);
    ScoringInput input{};input.centerQuality=.2;input.preciseAvailable=true;input.mini={.8,.8,.8,.8};
    mixed.left.addScored(scoreValidCut(input,profile));
    setAccuracyMethod(profile,ProfileAccuracyMethod::Precise);
    mixed.left.addScored(scoreValidCut(input,profile));
    profile.preciseMixPct=25;setAccuracyMethod(profile,ProfileAccuracyMethod::Blended);
    auto blended=scoreValidCut(input,profile);near(blended.accuracyQuality,.35);
    mixed.right.addScored(blended);
    near(mixed.averages().left.accuracy.pct,50);near(mixed.averages().right.accuracy.pct,35);
    setAccuracyWeight(profile,0);mixed.right.addScored(scoreValidCut(input,profile));
    req(mixed.averages().right.accuracy.samples==1,"disabled accuracy does not dilute average");
    mixed.reset();req(mixed.averages().left.accuracy.samples==0,"accuracy resets between levels");
  }

  // Advanced weights are whole percentages only. Legacy/fractional values are rounded
  // before use, and stable normalization caps the total at 100 without proportional scaling.
  ScoringProfile integerWeights{};
  integerWeights.precise={true,34,50}; integerWeights.center={true,34};
  integerWeights.before={true,33,100}; integerWeights.after={false,0,60};
  normalizeEnabledWeights(integerWeights);
  req(integerWeights.precise.weightPct==34&&integerWeights.center.weightPct==34&&integerWeights.before.weightPct==32,"whole-percent normalization caps final component");
  req(enabledWeightSum(integerWeights)==100,"whole-percent profile budget <=100");
  setComponentWeight(integerWeights,ComponentId::Center,33);
  req(integerWeights.center.weightPct==33&&componentWeightMaxPct(integerWeights,ComponentId::Center)==34,"whole-percent slider uses remaining budget");

  req(clampMaxScore(1500)==1000,"note max clamped to 1000");

  // Non-finite runtime inputs fail closed instead of propagating NaN/Inf into score state.
  const double nan=std::numeric_limits<double>::quiet_NaN();
  const double inf=std::numeric_limits<double>::infinity();
  near(clampQuality(nan),0); near(clampQuality(inf),0); near(clampMaxScore(nan),0);
  req(roundedNonNegativeToInt(nan)==0&&roundedNonNegativeToInt(inf)==0,"non-finite integer conversion fails to zero");
  near(swingQuality(nan,60),0); near(swingQuality(60,nan),0);
  FailureRule invalidFixed{FailureMode::FixedPoints,nan}; normalizeFailureRule(invalidFixed,100); near(invalidFixed.value,0);
  FailureRule invalidPct{FailureMode::PercentOfMax,inf}; normalizeFailureRule(invalidPct,100); near(invalidPct.value,0);

  // Property audit: arbitrary edits can never push any Advanced profile above
  // its 100% budget, and scoring always remains inside 0..maxScore.
  std::mt19937 fuzzRng(0xC07ACC);
  std::uniform_int_distribution<int> pctDist(-250,350);
  std::uniform_real_distribution<double> qualityDist(-1.0,2.0);
  std::uniform_real_distribution<double> angleDist(-100.0,300.0);
  std::uniform_real_distribution<double> maxDist(-500.0,1500.0);
  for(int trial=0;trial<20000;++trial){
    ScoringProfile fp{};
    fp.maxScore=maxDist(fuzzRng);
    fp.flat={true,pctDist(fuzzRng)};
    fp.precise={true,pctDist(fuzzRng),pctDist(fuzzRng)};
    fp.center={true,pctDist(fuzzRng)};
    fp.before={true,pctDist(fuzzRng),angleDist(fuzzRng)};
    fp.after={true,pctDist(fuzzRng),angleDist(fuzzRng)};
    normalizeEnabledWeights(fp);
    req(enabledWeightSum(fp)>=0&&enabledWeightSum(fp)<=100,"fuzz profile budget bounded");
    for(auto id:allComponents()){
      setComponentWeight(fp,id,pctDist(fuzzRng));
      req(enabledWeightSum(fp)>=0&&enabledWeightSum(fp)<=100,"fuzz edit budget bounded");
    }
    ScoringInput fi{};
    fi.preciseAvailable=(trial%3)!=0;
    fi.mini={qualityDist(fuzzRng),qualityDist(fuzzRng),qualityDist(fuzzRng),qualityDist(fuzzRng)};
    fi.centerQuality=qualityDist(fuzzRng);
    fi.beforeSwingDeg=angleDist(fuzzRng);
    fi.afterSwingDeg=angleDist(fuzzRng);
    auto fs=scoreValidCut(fi,fp);
    req(fs.maxScore>=0.0&&fs.maxScore<=1000.0,"fuzz max score bounded");
    req(fs.totalScore>=-1e-9&&fs.totalScore<=fs.maxScore+1e-9,"fuzz score inside max");
  }

  std::cout<<"CutAccuracy core tests passed\n";
}
