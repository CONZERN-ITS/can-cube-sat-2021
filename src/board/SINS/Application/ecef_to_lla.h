/*
 * ecef_to_lla.h
 *
 *  Created on: Sep 18, 2021
 *      Author: sereshotes
 */

#ifndef ECEF_TO_LLA_H_
#define ECEF_TO_LLA_H_

#include <math.h>


static void ecef_to_lla(const double xyz[3], double sph[3]) {
    double x = xyz[0];
    double y = xyz[1];
    double z = xyz[2];

    const static double a = 6378137;

    const static double b = 6.3568e6;

    const double e = sqrt((pow(a, 2) - pow(b, 2)) / pow(a, 2));
    const double e2 = sqrt((pow(a, 2) - pow(b, 2)) / pow(b, 2));

    double lat, lon, height, N , theta, p;

    p = sqrt(pow(x, 2) + pow(y, 2));

    theta = atan2((z * a) , (p * b));

    lon = atan2(y , x);

    lat = atan2((z + pow(e2, 2) * b * pow(sin(theta), 3)) , ((p - pow(e, 2) * a * pow(cos(theta), 3))));
    N = a / (sqrt(1 - (pow(e, 2) * pow(sin(lat), 2))));

    double m = (p / cos(lat));
    height = m - N;


    sph[0] = height;
    sph[1] = lat;
    sph[2] = lon;
}

#endif /* ECEF_TO_LLA_H_ */
