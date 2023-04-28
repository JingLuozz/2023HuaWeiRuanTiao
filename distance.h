#ifndef DISTANCEH
#define DISTANCEH
#include <iostream>
#include <algorithm>
#include <math.h>
#include "print.h"
#define pi 3.1415926
#include <cstdlib>
#define fir first
#define sec second
#pragma GCC optimize(2)
using namespace std;

const double avgDis = 5* 0.02;
//计算两点距离
double dis(double sx, double sy, double tx, double ty) {
	return sqrt((sx - tx)*(sx - tx) + (sy - ty)*(sy - ty));
}


int getAngle(double angle) {
	if(angle < 0) angle = angle + 2*pi;
	angle = (180.0/pi*angle) + 0.5;
	return (int) angle;
}

double getPi_to_rightline(pair<double,double> v) {
	double temp = sqrt(v.fir*v.fir + v.sec*v.sec);
	double angle2 = acos(v.fir/ temp);
	if(v.sec < 0) angle2 *= -1;
	if(angle2 <0) angle2 = 2*pi + angle2;
	return angle2;
}

void move_to_target(double stationx, double stationy, double x, double y, int id, double angle, vector<Cmd> &list, double f, int rotkind) {// 工作台初始坐标 float x， float y 返回指令与速度
	double newx = stationx - x, newy = stationy - y;//newx与newy是以机器人为原点下的工作台坐标
	double temp = sqrt(newx*newx + newy * newy);//temp2为斜边长度
	if (temp == 0) return ;
	double angle2 = acos(newx / temp);//angle2为工作台相对于机器人的偏移方向
	if (newy < 0) angle2 *= -1;
	if (angle2 < 0) angle2 = 2 * pi + angle2;
	//double dist = dis(stationx, stationy, x, y);
	double sita = angle2 - angle;
	//if(dist > 0.75) sita += (rand() % 101)*(pi / 600) - (pi / 12);
	if (sita > pi) sita = sita-2*pi;
	double v,w;
	// double v = -24.0 / (pi*pi) * sita * sita + 6.0;
	// if(abs(sita) > pi/6) v = 0; 
	if(sita > 0) v = -0.5628*sita*sita*sita + 3.429*sita*sita - 7.759*sita + 5.991;
	else v = 0.5628*sita*sita*sita + 3.429*sita*sita + 7.759*sita + 5.991;
	if(rotkind == 0) {
		if(sita > 0) v = -0.5609*sita*sita*sita + 3.414*sita*sita - 7.743*sita+ 6.026;
		else v = 0.5609*sita*sita*sita + 3.414*sita*sita + 7.743*sita + 6.026;
	}
	else {
		if(sita > 0) v = -0.8798*sita*sita*sita +5.04*sita*sita - 10.14 *sita+  7.139;
		else v = 0.8798*sita*sita*sita +5.04*sita*sita + 10.14 *sita +  7.139;		
	}
	// if(abs(sita) > pi/6 && abs(sita) < 5*pi/6) v = 0; 

	// double w = 1728.0 / (pi*pi) * sita * sita * sita;
	// if(!forward) sita = sita + pi + (sita > 0? -2*pi : 0);
	if(sita > 0) w = 65.51 *sita *sita * sita - 70.98 *sita*sita + 26.13 *sita;
	else w = -(-65.51 *sita *sita * sita - 70.98 *sita*sita - 26.13 *sita);
	//v = -0.1205 *sita*sita*sita*sita + 7.0725*sita*sita + 6;
	// if(id == 3) logs.fp << stationx << ' ' << stationy << ' ' << x << ' ' << y << ' ' <<sita<< ' '<< v<<' '<<w<<endl;
	list.push_back(Cmd(1,id, w));
	list.push_back(Cmd(0, id, v*f));
	return;
}


int move_to_moon(double stationx, double stationy, double x, double y, int id, double angle, vector<Cmd> &list, double f, int rotkind) {
	double newx = stationx - x, newy = stationy - y;//newx与newy是以机器人为原点下的工作台坐标
	double temp = sqrt(newx*newx + newy * newy);//temp2为斜边长度
	int flg = 0;
	if (temp == 0) return 0;
	double angle2 = acos(newx / temp);//angle2为工作台相对于机器人的偏移方向
	if (newy < 0) angle2 *= -1;
	if (angle2 < 0) angle2 = 2 * pi + angle2;
	//double dist = dis(stationx, stationy, x, y);
	double sita = angle2 - angle;
	//if(dist > 0.75) sita += (rand() % 101)*(pi / 600) - (pi / 12);
	if (sita > pi) sita = sita-2*pi;
	double v,w;
	// double v = -24.0 / (pi*pi) * sita * sita + 6.0;
	// if(abs(sita) > pi/6) v = 0; 
	if(sita > 0) v = -0.5628*sita*sita*sita + 3.429*sita*sita - 7.759*sita + 5.991;
	else v = 0.5628*sita*sita*sita + 3.429*sita*sita + 7.759*sita + 5.991;
	if(rotkind == 0) {
		if(sita > 0) v = -0.5609*sita*sita*sita + 3.414*sita*sita - 7.743*sita+ 6.026;
		else v = 0.5609*sita*sita*sita + 3.414*sita*sita + 7.743*sita + 6.026;
	}
	else {
		if(sita > 0) v = -0.8798*sita*sita*sita +5.04*sita*sita - 10.14 *sita+  7.139;
		else v = 0.8798*sita*sita*sita +5.04*sita*sita + 10.14 *sita +  7.139;		
	}
	if(abs(sita)  > pi/24) v = 0, flg = 0;
	else v = 7, f = 1.0, flg =1;
	// if(abs(sita) > pi/6 && abs(sita) < 5*pi/6) v = 0; 

	// double w = 1728.0 / (pi*pi) * sita * sita * sita;
	// if(!forward) sita = sita + pi + (sita > 0? -2*pi : 0);
	if(sita > 0) w = 65.51 *sita *sita * sita - 70.98 *sita*sita + 26.13 *sita;
	else w = -(-65.51 *sita *sita * sita - 70.98 *sita*sita - 26.13 *sita);
	//v = -0.1205 *sita*sita*sita*sita + 7.0725*sita*sita + 6;
	// if(id == 3) logs.fp << stationx << ' ' << stationy << ' ' << x << ' ' << y << ' ' <<sita<< ' '<< v<<' '<<w<<endl;
	list.push_back(Cmd(1,id, w));
	list.push_back(Cmd(0, id, v*f));	
	return flg;	
}
#endif