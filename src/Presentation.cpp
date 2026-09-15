#include "CutAccuracy/Presentation.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
namespace CutAccuracy { namespace {
std::string metric(const ComponentAverage& a){if(!a.samples)return "   - ";char b[64];std::snprintf(b,sizeof(b),"<color=%s>%5.1f</color>",accuracyBandHex(a.pct),a.pct);return b;}
void addRow(std::string& out,const char* label,const ComponentAverage& l,const ComponentAverage&r){char b[256];auto ls=metric(l),rs=metric(r);std::snprintf(b,sizeof(b),"\n%-12s %s   %s",label,ls.c_str(),rs.c_str());out+=b;}
}
HudPresentation buildHudPresentation(const SessionStats& stats,ScoringMode mode,AccuracyMethod,bool){
    const auto s=stats.averages();
    std::string heading=mode==ScoringMode::Off ? "CUT ACCURACY — OFF" : "CUT ACCURACY";
    std::string table="<mspace=0.6em>                 L       R";
    if(mode!=ScoringMode::Off){
        addRow(table,"Before swing",s.left.before,s.right.before);
        addRow(table,"After swing",s.left.after,s.right.after);
        addRow(table,"Accuracy",s.left.accuracy,s.right.accuracy);
    }
    table+="</mspace>";
    return{s.levelAccuracyPct,heading,table};
}
std::string formatPerNoteScore(const ScoredNote&s){char b[32];std::snprintf(b,sizeof(b),"%.0f",std::clamp(s.totalScore,0.0,s.maxScore));return b;} std::string formatFixedScore(double s,double m){char b[32];std::snprintf(b,sizeof(b),"%.0f",m>0?std::clamp(s,0.0,m):0.0);return b;}
int accuracyBand5(double p){double c=std::isfinite(p)?std::clamp(p,0.0,100.0):0.0;return static_cast<int>(c/5.0)*5;} const char* accuracyBandHex(double p){static constexpr const char* b[]={"#ff0038","#ff102f","#ff2025","#ff321b","#ff4612","#ff5a08","#ff7000","#ff8700","#ff9f00","#ffb800","#ffd200","#ffee00","#e2ff00","#c4ff00","#a5ff00","#83ff00","#5fff00","#38ff00","#00ff2f","#00ff66","#00ffaa"};return b[accuracyBand5(p)/5];} AccuracyRgb accuracyBandRgb(double p){static constexpr AccuracyRgb b[]={{1,0,.22f},{1,.063f,.184f},{1,.125f,.145f},{1,.196f,.106f},{1,.275f,.071f},{1,.353f,.031f},{1,.439f,0},{1,.529f,0},{1,.624f,0},{1,.722f,0},{1,.824f,0},{1,.933f,0},{.886f,1,0},{.769f,1,0},{.647f,1,0},{.514f,1,0},{.373f,1,0},{.220f,1,0},{0,1,.184f},{0,1,.4f},{0,1,.667f}};return b[accuracyBand5(p)/5];}
}
