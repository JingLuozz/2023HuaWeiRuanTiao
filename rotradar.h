#ifndef RADAR
#define RADAR

#include <math.h>
#include <string.h>
#include "print.h"
#define bias 1e-2
#define PI 3.1415926
#pragma GCC optimize(2)
using namespace std;


double coss[360];
double sinn[360];//下标范围0・359


double angle_to_radian(double degree)
{
	double result =(degree * PI)/180;
	return result;
}

double getdelta(double a, double b, double c) {
	double delta=b*b-4*a*c;
	return sqrt(delta)/a;
}

double get(double a,double b,double c) {
  double delta=b*b-4*a*c;
  return (-b+sqrt(delta))/2*a;
}

double get_1(double a,double b,double c) {
  double delta=b*b-4*a*c;
  return (-b-sqrt(delta))/2*a;
}


void operate_init() {
	for(int i=0;i<360;i++) {
		double temp=angle_to_radian(i);
		coss[i]=cos(temp);
		sinn[i]=sin(temp);
	}
	return ;
}

pair<double,double> get_offset(double distance,int angle) {//angle为角度制
	pair<double,double> temp;
	temp.first=distance*coss[angle];
	temp.second=distance*sinn[angle];
	return temp;
}

class Radar{
public:
	double dist[360];

	Radar() {
		memset(dist, 0, sizeof(dist));
	}

	void updateRadar(int index, double length) {
		dist[index] = length;
		return ;
	}

	// bool isvalid2(int index,double distance,double radis) {
	// 	int left=(index-90+360)%360;
	// 	int right=(index+90+360)%360;
	// 	int i;
	// 	bool tag=true;
	// 	for(i=right;i!=left;i=(i-1+360)%360) {
	// 		double temp2=angle_to_radian(right-i);
	// 		double safe_dist=min(abs(radis/cos(temp2)),distance+radis);
	// 		if(dist[i]>safe_dist) ;
	// 		else {
	// 			// cout<<dist[i]<<","<<safe_dist<<endl;
	// 			tag=false;
	// 			break;
	// 		} 
	// 	}
	// 	return tag;
	// }

	bool isvalid2(int index,double distance,double radis) {
		double maxlen = distance*distance + radis*radis;
		for(int i = 0;i<=90;i++) {
			double saft_dist = radis/coss[i];
			if(maxlen < saft_dist * saft_dist) {
				saft_dist = get(1, -2*coss[90-i]*distance, distance*distance - radis * radis);	
			}
			int lsita = (index + 90 - i + 360) %360;
			int rsita = (index - 90 + i + 360) %360;			
			if(saft_dist > dist[lsita] || saft_dist > dist[rsita]) {
				return false;
			}			
		}
		return true;
	}

	double getmaxdistance(int index, double radis) {
		double mid = 10;
		for(int i = 0; i<=90; i++) {
			double saft_dist = radis/coss[i];
			double mad;
			int lsita = (index + 90 - i + 360) %360;
			int rsita = (index - 90 +i + 360) %360;
			if(dist[lsita] < saft_dist) {
				mad = get_1(1, -2*coss[90-i]*dist[lsita], dist[lsita]*dist[lsita] - radis*radis);
				mid = min(mid, mad);
			}
			if(i == 90) continue;
			if(dist[rsita] < saft_dist) {
				mad = get_1(1, -2*coss[90-i]*dist[rsita], dist[rsita]*dist[rsita] - radis*radis);
				mid = min(mid, mad);				
			}
		}
		return max(0.0, mid);
	}	
};

#endif