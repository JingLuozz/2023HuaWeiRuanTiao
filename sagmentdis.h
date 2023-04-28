#ifndef SAGMENT
#define SAGMENT
#include <algorithm>
#include <cmath>
#define fir first
#define sec second
#include "distance.h"
#pragma GCC optimize(2)
using namespace std;

double dianJi(pair<double,double> a, pair<double,double> b, pair<double,double> c){
    pair<double,double> aTob = pair<double,double>(b.fir-a.fir,b.sec-a.sec);
    pair<double,double> aToc = pair<double,double>(c.fir-a.fir,c.sec-a.sec);
    return aTob.fir*aToc.fir + aTob.sec*aToc.sec;
}

double chaJi(pair<double,double> a,pair<double,double> b,pair<double,double> c) {
    pair<double,double> aTob = {b.fir-a.fir,b.sec-a.sec};
    pair<double,double> aToc = {c.fir-a.fir,c.sec-a.sec};
    return aTob.fir*aToc.sec + aTob.sec*aToc.fir;
}

double distBetweenPoints(pair<double,double> a,pair<double,double> b){
    return (a.fir-b.fir)*(a.fir-b.fir) + (a.sec-b.sec)*(a.sec-b.sec);
}

double disBetweenPointAndLine(pair<double,double> a,pair<double,double> b,pair<double,double> c){
    if (chaJi(a,b,c) > 0) return 1;
    else if (chaJi(a,b,c) < 0) return -1;
    else if (dianJi(a,b,c) < 0) return -2;
    else if ((dianJi(a,b,c) >= 0) && distBetweenPoints(a,b) >= distBetweenPoints(a,c)){
        if (distBetweenPoints(a,b) < distBetweenPoints(a,c))
            return 2;
        return 0;
    }
    return 0;
}

double minDistance(pair<double,double> a,pair<double,double> b,pair<double,double> c){
    double temp = ((c.fir - a.fir) * (b.fir - a.fir) + (c.sec - a.sec) * (b.sec - a.sec)) / distBetweenPoints(a, b);
    if (temp <= 0) return sqrt(distBetweenPoints(a, c));
    else if (temp >= 1) return sqrt(distBetweenPoints(b, c));
    else{
        double aToC = temp * sqrt(distBetweenPoints(a, b));
        return sqrt(distBetweenPoints(a, c) - aToC * aToC);
    }
}

double minDis(pair<double,double> a,pair<double,double> b,pair<double,double> c, pair<double,double> d){
    if (disBetweenPointAndLine(a,b,c) * disBetweenPointAndLine(a,b,c) <= 0 && disBetweenPointAndLine(c,d,a) * disBetweenPointAndLine(c,d,a) <= 0)
        return 0;
    else
        return min({minDistance(a, b, c), minDistance(a, b, d), minDistance(c, d, a), minDistance(c, d, b)});
};

double ptoldis(pair<double,double> line1,pair<double,double> line2,pair<double,double> point) {
    double d1=dis(point.first,point.second,line1.first,line1.second);
    double d2=dis(point.first,point.second,line2.first,line2.second);
    double d3=dis(line1.first,line1.second,line2.first,line2.second);

    if (d1*d1 > (d2*d2 + d3*d3))
        return d2;
    if (d2*d2 > (d1*d1 + d3*d3))
        return d1;
    double l = (d1+d2+d3)/2;
    double s = sqrt(l*(l-d1)*(l-d2)*(l-d3));
    return 2*s/d3;  
}
#endif