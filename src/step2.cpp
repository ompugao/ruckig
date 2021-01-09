#include <iomanip>

#include <ruckig/ruckig.hpp>
#include <ruckig/roots.hpp>


namespace ruckig {

Step2::Step2(double tf, double p0, double v0, double a0, double pf, double vf, double af, double vMax, double aMax, double jMax): tf(tf), p0(p0), v0(v0), a0(a0), pf(pf), vf(vf), af(af) {
    // max values needs to be invariant to plus minus sign change
    pd = pf - p0;
    tf_tf = tf * tf;

    vd = vf - v0;
    vd_vd = vd * vd;
    v0_v0 = v0 * v0;
    vf_vf = vf * vf;
    vMax_vMax = vMax * vMax;

    ad = af - a0;
    ad_ad = ad * ad;
    a0_a0 = a0 * a0;
    af_af = af * af;
    aMax_aMax = aMax * aMax;

    a0_p3 = a0 * a0 * a0;
    a0_p4 = a0_a0 * a0_a0;
    af_p3 = af * af * af;
    af_p4 = af_af * af_af;

    jMax_jMax = jMax * jMax;
}

bool Step2::time_up_acc0_acc1_vel(Profile& profile, double vMax, double aMax, double jMax) {
    if (tf < 2*aMax/jMax) {
        return false;
    }

    // Profile UDDU
    {
        const double h0b = aMax_aMax + jMax*(vd - aMax*tf);
        const double h0a = af_af + 2*(af*aMax + aMax_aMax - aMax*jMax*tf - jMax*vd);
        const double h1 = Sqrt(-a0_p4 - af_p4 + 4./3*aMax*(a0_p3 - af_p3) + 2*a0*h0a*(a0 - 2*aMax) + 4*af*h0b*(af + 2*aMax) + 4*(Power(aMax,4) - 2*Power(aMax,3)*jMax*tf + aMax_aMax*jMax_jMax*tf_tf - jMax_jMax*vd_vd + 2*aMax*jMax_jMax*(-2*pd + tf*(v0 + vf))));
        const double h2 = 2*aMax*(ad + 3*aMax - jMax*tf) + h1;
        const double h3 = -4*aMax*jMax;
        const double h4 = a0_a0 - af_af + 2*jMax*vd;

        profile.t[0] = (-a0 + aMax)/jMax;
        profile.t[1] = (h2 - h4)/h3;
        profile.t[2] = profile.t[0] + a0/jMax;
        profile.t[3] = -aMax/jMax - 2*h1/h3;
        profile.t[4] = profile.t[2];
        profile.t[5] = (h2 + h4)/h3;
        profile.t[6] = profile.t[4] + af/jMax;

        if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
            return true;
        }
    }

    // Profile UDUD
    {
        const double h1 = 12*aMax*jMax*(a0_a0 + af_af - 2*(a0 + af)*aMax + 2*(aMax_aMax - aMax*jMax*tf + jMax*vd));
        const double h2 = 3*a0_p4 + 3*af_p4 - 4*a0_p3*aMax - 4*af_p3*aMax;
        const double h3 = -4*af_p3*aMax + 24*(a0 + af)*Power(aMax,3) - 24*af*aMax*jMax*vd - 6*af_af*(aMax_aMax - 2*jMax*vd) + 6*a0_a0*(af_af - 2*af*aMax - aMax_aMax - 2*aMax*jMax*tf + 2*jMax*vd) - 12*(2*Power(aMax,4) - 2*Power(aMax,3)*jMax*tf - 2*aMax*jMax_jMax*(-pd + tf*v0) - jMax_jMax*vd_vd + aMax_aMax*jMax*vd);

        profile.t[0] = (-a0 + aMax)/jMax;
        profile.t[1] = (h2 + h3)/h1;
        profile.t[2] = profile.t[0] + a0/jMax;
        profile.t[3] = -(a0_a0 + af_af - 2*aMax*(a0 + af + jMax*tf) + 4*aMax_aMax + 2*jMax*vd)/(2*aMax*jMax);
        profile.t[4] = profile.t[2];
        profile.t[5] = tf - (profile.t[0] + profile.t[1] + profile.t[2] + profile.t[3] + 2*profile.t[4] + af/jMax);
        profile.t[6] = profile.t[4] - af/jMax;

        if (profile.check<Teeth::UDUD>(tf, pf, vf, af, jMax, vMax, aMax)) {
            return true;
        }
    }
    
    return false;
}

bool Step2::time_up_acc1_vel(Profile& profile, double vMax, double aMax, double jMax) {
    if (tf < aMax/jMax) {
        return false;
    }
    
    // Profile UDDU
    {
        const double ph1 = a0_a0 + af_af + aMax*(a0 + 2*af) + aMax_aMax - 2*jMax*(vd + aMax*tf);
        const double ph2 = -2*aMax*jMax*(-pd + tf*v0) - aMax_aMax*vd + jMax*vd_vd;

        std::array<double, 5> polynom;
        polynom[0] = 1.0;
        polynom[1] = (2*(2*a0 + aMax))/jMax;
        polynom[2] = (4*a0_a0 + ph1 + 3*a0*aMax)/jMax_jMax;
        polynom[3] = (2*a0*ph1)/Power(jMax,3);
        polynom[4] = (3*a0_p4 + 3*af_p4 + 4*a0_p3*aMax + 8*af_p3*aMax + 6*af_af*(aMax_aMax - 2*jMax*vd) + 12*jMax*ph2 - 24*af*aMax*jMax*vd + 6*a0_a0*(af_af + 2*af*aMax + aMax_aMax - 2*jMax*(vd + aMax*tf)))/(12*Power(jMax,4));
        auto roots = Roots::solveQuartMonic(polynom);

        for (double t: roots) {
            if (t < 0.0 || t > tf - aMax/jMax) {
                continue;
            }

            const double h1 = ((a0_a0 + af_af)/2 + jMax*(v0 - vf + 2*a0*t + jMax*Power(t,2)))/aMax;
            
            profile.t[0] = t;
            profile.t[1] = 0;
            profile.t[2] = profile.t[0] + a0/jMax;
            profile.t[3] = -(h1 + aMax + a0 + af)/jMax - (2*t - tf);
            profile.t[4] = aMax/jMax;
            profile.t[5] = (h1 - aMax)/jMax;
            profile.t[6] = profile.t[4] + af/jMax;
            
            if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
                return true;
            }
        }
    }

    // Profile UDUD
    {
        const double ph1 = a0_a0 - af_af + (2*af - a0)*aMax - aMax_aMax - 2*jMax*(vd - aMax*tf);
        const double ph4 = aMax_aMax + 2*jMax*vd;
        const double ph2 = af_af + ph4 - 2*aMax*(af + jMax*tf);
        const double ph3 = 2*aMax*jMax*(-pd + tf*v0) + jMax*vd_vd + aMax_aMax*vd;

        std::array<double, 5> polynom;
        polynom[0] = 1.0;
        polynom[1] = (4*a0 - 2*aMax)/jMax;
        polynom[2] = (4*a0_a0 - 3*a0*aMax + ph1)/jMax_jMax;
        polynom[3] = (2*a0*ph1)/Power(jMax,3);
        polynom[4] = (3*a0_p4 + 3*af_p4 - 4*a0_p3*aMax - 8*af_p3*aMax - 24*af*aMax*jMax*vd + 12*jMax*ph3 - 6*a0_a0*ph2 + 6*af_af*ph4)/(12*Power(jMax,4));

        auto roots = Roots::solveQuartMonic(polynom);
        for (double t: roots) {
            if (t < 0.0 || t > tf - aMax/jMax) {
                continue;
            }

            const double h1 = ((a0_a0 - af_af)/2 + jMax_jMax*Power(t,2) - jMax*(vd - 2*a0*t))/aMax;

            profile.t[0] = t;
            profile.t[1] = 0;
            profile.t[2] = a0/jMax + t;
            profile.t[3] = (h1 + af - a0 - aMax)/jMax - (2*t - tf);
            profile.t[4] = aMax/jMax;
            profile.t[5] = -(h1 + aMax)/jMax;
            profile.t[6] = profile.t[4] - af/jMax;
            
            if (profile.check<Teeth::UDUD>(tf, pf, vf, af, jMax, vMax, aMax)) {
                return true;
            }
        }
    }

    return false;
}

bool Step2::time_up_acc0_vel(Profile& profile, double vMax, double aMax, double jMax) {
    if (tf < aMax/jMax) {
        return false;
    }

    const double ph1 = 12*jMax*(-aMax_aMax*vd - jMax*vd_vd + 2*aMax*jMax*(-pd + tf*vf));
       
    // Profile UDDU
    {
        std::array<double, 5> polynom;
        polynom[0] = 1.0;
        polynom[1] = (2*aMax)/jMax;
        polynom[2] = (a0_a0 - af_af + 2*ad*aMax + aMax_aMax + 2*jMax*(vd - aMax*tf))/jMax_jMax;
        polynom[3] = 0;
        polynom[4] = -(-3*a0_p4 - 3*af_p4 + 8*a0_p3*aMax + 4*af_p3*aMax - 12*a0*aMax*(af_af - 2*jMax*vd) + 6*a0_a0*(af_af - aMax_aMax + 2*jMax*v0 - 2*jMax*vf) + 6*af_af*(aMax_aMax - 2*aMax*jMax*tf + 2*jMax*vd) + ph1)/(12*Power(jMax,4));

        auto roots = Roots::solveQuartMonic(polynom);
        for (double t: roots) {
            if (t < 0.0 || t > tf - aMax/jMax) {
                continue;
            }

            const double h1 = ((a0_a0 - af_af)/2 + jMax*(jMax*Power(t,2) + vd))/aMax;

            profile.t[0] = (-a0 + aMax)/jMax;
            profile.t[1] = (h1 - aMax)/jMax;
            profile.t[2] = aMax/jMax;
            profile.t[3] = -(h1 - a0 + af + aMax)/jMax - (2*t - tf);
            profile.t[4] = t;
            profile.t[5] = 0;
            profile.t[6] = af/jMax + t;
            
            if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
                return true;
            }
        }
    }

    // Profile UDUD
    {
        std::array<double, 5> polynom;
        polynom[0] = 1.0;
        polynom[1] = (-2*aMax)/jMax;
        polynom[2] = -(a0_a0 + af_af - 2*(a0 + af)*aMax + aMax_aMax + 2*jMax*(vd - aMax*tf))/jMax_jMax;
        polynom[3] = 0;
        polynom[4] = (3*a0_p4 + 3*af_p4 - 8*a0_p3*aMax - 4*af_p3*aMax + 6*a0_a0*(af_af + aMax_aMax - 2*jMax*v0 + 2*jMax*vf) - 12*a0*aMax*(af_af + 2*jMax*vd) + 6*af_af*(aMax_aMax - 2*aMax*jMax*tf + 2*jMax*vd) - ph1)/(12*Power(jMax,4));

        auto roots = Roots::solveQuartMonic(polynom);
        for (double t: roots) {
            if (t < 0.0 || t > tf - aMax/jMax) {
                continue;
            }

            const double h1 = ((a0_a0 + af_af)/2 + jMax*(vd - jMax*Power(t,2)))/aMax;

            profile.t[0] = (-a0 + aMax)/jMax;
            profile.t[1] = (h1 - aMax)/jMax;
            profile.t[2] = aMax/jMax;
            profile.t[3] = -(h1 - a0 - af + aMax)/jMax - (2*t - tf);
            profile.t[4] = t;
            profile.t[5] = 0;
            profile.t[6] = -(af/jMax) + t;
            
            if (profile.check<Teeth::UDUD>(tf, pf, vf, af, jMax, vMax, aMax)) {
                return true;
            }
        }
    }

    return false;
}

bool Step2::time_up_vel(Profile& profile, double vMax, double aMax, double jMax) {
    // Profile UDDU
    {
        // Find root of 5th order polynom
        std::array<double, 6> polynom;
        polynom[0] = 1.0;
        polynom[1] = (15*a0_a0 + af_af + 4*af*jMax*tf - 16*a0*(af - jMax*tf) - 2*jMax*(jMax*tf_tf - 3*v0 + 3*vf))/(4*jMax*(a0 - af + jMax*tf));
        polynom[2] = (29*a0_p3 - 2*af_p3 - 33*a0_a0*(af - jMax*tf) + 6*jMax_jMax*(-pd + tf*v0) + 6*af*jMax*vd + 6*a0*(af_af + 4*af*jMax*tf - 2*jMax*(jMax*tf_tf - 3*v0 + 3*vf)))/(6.*jMax_jMax*(a0 - af + jMax*tf));
        polynom[3] = (61*a0_p4 + af_p4 + 8*af_p3*jMax*tf - 76*a0_p3*(af - jMax*tf) - 24*Power(jMax,3)*tf*(-pd + tf*v0) - 16*a0*(af_p3 - 3*jMax_jMax*(-pd + tf*v0) - 3*af*jMax*vd) - 12*af_af*jMax*vd + 36*jMax_jMax*vd_vd + 24*af*jMax_jMax*(-pd + 2*tf*v0 - tf*vf) + 30*a0_a0*(af_af + 4*af*jMax*tf - 2*jMax*(jMax*tf_tf - 3*v0 + 3*vf)))/(24*Power(jMax,3)*(a0 - af + jMax*tf));
        polynom[4] = (a0*(7*a0_p4 + af_p4 + 8*af_p3*jMax*tf - 10*a0_p3*(af - jMax*tf) - 24*Power(jMax,3)*tf*(-pd + tf*v0) - 4*a0*(af_p3 - 3*jMax_jMax*(-pd + tf*v0) - 3*af*jMax*vd) - 12*af_af*jMax*vd + 36*jMax_jMax*vd_vd + 24*af*jMax_jMax*(-pd + 2*tf*v0 - tf*vf) + 6*a0_a0*(af_af + 4*af*jMax*tf - 2*jMax*(jMax*tf_tf - 3*v0 + 3*vf))))/(12*Power(jMax,4)*(a0 - af + jMax*tf));
        polynom[5] = (7*Power(a0,6) + Power(af,6) - 12*Power(a0,5)*(af - jMax*tf) + 48*af_p3*jMax_jMax*(-pd + tf*v0) - 8*a0_p3*(af_p3 - 3*jMax_jMax*(-pd + tf*v0) - 3*af*jMax*vd) - 72*Power(jMax,3)*(jMax*Power(-pd + tf*v0,2) - Power(v0 - vf,3)) - 6*af_p4*jMax*vd - 144*af*Power(jMax,3)*(-pd + tf*v0)*vd + 36*af_af*jMax_jMax*vd_vd + 9*a0_p4*(af_af + 4*af*jMax*tf - 2*jMax*(jMax*tf_tf - 3*v0 + 3*vf)) + 3*a0_a0*(af_p4 + 8*af_p3*jMax*tf - 24*Power(jMax,3)*tf*(-pd + tf*v0) - 12*af_af*jMax*vd + 36*jMax_jMax*vd_vd + 24*af*jMax_jMax*(-pd + 2*tf*v0 - tf*vf)))/(144*Power(jMax,5)*(a0 - af + jMax*tf));

        std::array<double, 5> deriv;
        deriv[0] = 1;
        deriv[1] = 4./5 * polynom[1];
        deriv[2] = 3./5 * polynom[2];
        deriv[3] = 2./5 * polynom[3];
        deriv[4] = 1./5 * polynom[4];

        // Solve 4th order derivative analytically
        auto extremas = Roots::solveQuartMonic(deriv[1], deriv[2], deriv[3], deriv[4]);
        std::set<std::tuple<double, double>> tz_intervals;

        double tz_min {0.0};
        double tz_max = std::min<double>(tf, (tf - a0/jMax) / 2);
        double tz_current {tz_min};

        for (double tz: extremas) {
            if (tz <= 0.0 || tz >= tz_max) {
                continue;
            }

            // Check that polynom(lower) and polynom(upper) have different signs (should only happen at first and last boundary)
            double val_current = Roots::polyEval(polynom, tz_current);
            double val_new = Roots::polyEval(polynom, tz);
            // std::cout << "tz: " << tz << " " << val_new << " " << val_current << std::endl;
            if (std::abs(val_new) < 1e-15) {
                tz_intervals.insert({tz - 2e-16, tz + 2e-16});
                tz += 1e-15;
                
            } else if (val_current * val_new < 0) {
                tz_intervals.insert({tz_current, tz});
            }
            tz_current = tz;
        }
        if (Roots::polyEval(polynom, tz_current) * Roots::polyEval(polynom, tz_max) < 0) {
            tz_intervals.insert({tz_current, tz_max});
        }

        for (auto interval: tz_intervals) {
            // Use safe Newton method
            const double lower = std::get<0>(interval);
            const double upper = std::get<1>(interval);
            double t = Roots::shrinkInterval(polynom, lower, upper, 1e-16);

            // Single Newton step
            {
                const double h2 = Sqrt(2*(a0_a0 + af_af + 4*a0*jMax*t + 2*jMax*(jMax*Power(t,2) - vd)))/Abs(jMax);
                const double orig = -pd - (2*a0_p3 + 4*af_p3 + 24*a0*jMax*t*(af + jMax*(t - tf) + jMax*h2/2) + 6*a0_a0*(af + jMax*(2*t - tf) + jMax*h2/2) + 6*af_af*jMax*h2/2 + 12*af*jMax*(jMax*Power(t,2) - vd) + 12*jMax_jMax*(jMax*Power(t,2)*(t - tf) - tf*v0 - h2/2*(vd - jMax*Power(t,2))))/(12*jMax_jMax);                
                const double deriv = -(a0 + jMax*t)*(3*((a0_a0 + af_af) + 2*jMax_jMax*Power(t,2) + 2*jMax*(2*a0*t - vd))/(h2*jMax_jMax) + (a0 + 2*af)/jMax + (3*t - 2*tf));

                t = t - orig / deriv;
            }

            if (t < 0.0 || t > tf) {
                continue;
            }
            
            profile.t[0] = t;
            profile.t[1] = 0;
            profile.t[2] = profile.t[0] + a0/jMax;
            profile.t[4] = Sqrt(a0_a0/2 + af_af/2 + jMax*(2*a0*t + jMax*Power(t,2) - vd))/Abs(jMax);
            profile.t[5] = 0;
            profile.t[6] = profile.t[4] + af/jMax;
            profile.t[3] = tf - (profile.t[0] + profile.t[2] + profile.t[4] + profile.t[6]);

            if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
                return true;
            }
        }
    }

    // Profile UDUD
    {
        const double ph1 = af_af - 4*af*jMax*tf - 2*jMax*(jMax*tf_tf - 3*vd);
        const double ph2 = af_p3 - 3*jMax_jMax*(-pd + tf*v0) + 3*af*jMax*vd;
        const double ph3 = 2*jMax*tf*(-pd + tf*v0) + 3*vd_vd;

        // Find root of 6th order polynom
        std::array<double, 7> polynom;
        polynom[0] = 1.0;
        polynom[1] = -((-5*a0 + af + jMax*tf)/jMax);
        polynom[2] = (39*a0_a0 - ph1 - 16*a0*(af + jMax*tf))/(4*jMax_jMax);
        polynom[3] = (55*a0_p3 - 33*a0_a0*(af + jMax*tf) - 6*a0*ph1 + 2*ph2)/(6.*Power(jMax,3));
        polynom[4] = (101*a0_p4 + af_p4 - 8*af_p3*jMax*tf - 76*a0_p3*(af + jMax*tf) - 30*a0_a0*ph1 + 12*jMax_jMax*ph3 + 12*af_af*jMax*vd + 24*af*jMax_jMax*(-pd + 2*tf*v0 - tf*vf) + 16*a0*ph2)/(24*Power(jMax,4));
        polynom[5] = (a0*(11*a0_p4 + af_p4 - 8*af_p3*jMax*tf - 10*a0_p3*(af + jMax*tf) - 6*a0_a0*ph1 + 12*jMax_jMax*ph3 + 12*af_af*jMax*vd + 24*af*jMax_jMax*(-pd + 2*tf*v0 - tf*vf) + 4*a0*ph2))/(12*Power(jMax,5));
        polynom[6] = (11*Power(a0,6) - Power(af,6) - 12*Power(a0,5)*(af + jMax*tf) - 48*af_p3*jMax_jMax*(-pd + tf*v0) - 9*a0_p4*ph1 + 72*Power(jMax,3)*(jMax*Power(-pd + tf*v0,2) + Power(v0 - vf,3)) - 6*af_p4*jMax*vd - 144*af*Power(jMax,3)*(-pd + tf*v0)*vd - 36*af_af*jMax_jMax*vd_vd + 8*a0_p3*ph2 + 3*a0_a0*(af_p4 - 8*af_p3*jMax*tf + 12*jMax_jMax*ph3 + 12*af_af*jMax*vd + 24*af*jMax_jMax*(-pd + 2*tf*v0 - tf*vf)))/(144*Power(jMax,6));

        std::array<double, 6> deriv;
        deriv[0] = 1.0;
        deriv[1] = 5./6 * polynom[1];
        deriv[2] = 4./6 * polynom[2];
        deriv[3] = 3./6 * polynom[3];
        deriv[4] = 2./6 * polynom[4];
        deriv[5] = 1./6 * polynom[5];

        auto dd_extremas = Roots::solveQuartMonic(4./5 * deriv[1], 3./5 * deriv[2], 2./5 * deriv[3], 1./5 * deriv[4]);
        std::set<std::tuple<double, double>> dd_tz_intervals;

        double tz_min {0.0};
        double tz_max = std::min<double>(tf, (tf - a0/jMax) / 2);
        double dd_tz_current {tz_min};

        for (double tz: dd_extremas) {
            if (tz <= 0.0 || tz >= tz_max) {
                continue;
            }

            // Check that polynom(lower) and polynom(upper) have different signs (should only happen at first and last boundary)
            if (Roots::polyEval(deriv, dd_tz_current) * Roots::polyEval(deriv, tz) < 0) {
                dd_tz_intervals.insert({dd_tz_current, tz});
            }
            dd_tz_current = tz;
        }
        if (Roots::polyEval(deriv, dd_tz_current) * Roots::polyEval(deriv, tz_max) < 0) {
            dd_tz_intervals.insert({dd_tz_current, tz_max});
        }

        std::set<std::tuple<double, double>> tz_intervals;
        double tz_current {tz_min};

        for (auto interval: dd_tz_intervals) {
            const double lower = std::get<0>(interval);
            const double upper = std::get<1>(interval);
            const double tz = Roots::shrinkInterval(deriv, lower, upper, 1e-14);

            if (tz <= 0.0 || tz >= tz_max) {
                continue;
            }
            // Check that polynom(lower) and polynom(upper) have different signs (should only happen at first and last boundary)
            if (Roots::polyEval(polynom, tz_current) * Roots::polyEval(polynom, tz) < 0) {
                tz_intervals.insert({tz_current, tz});
            }
            tz_current = tz;
        }
        if (Roots::polyEval(polynom, tz_current) * Roots::polyEval(polynom, tz_max) < 0) {
            tz_intervals.insert({tz_current, tz_max});
        }

        for (auto interval: tz_intervals) {
            // Use safe Newton method
            const double lower = std::get<0>(interval);
            const double upper = std::get<1>(interval);
            const double t = Roots::shrinkInterval(polynom, lower, upper, 1e-14);

            profile.t[0] = t;
            profile.t[1] = 0;
            profile.t[2] = profile.t[0] + a0/jMax;
            profile.t[4] = Sqrt(af_af/2 - a0_a0/2 - jMax*(2*a0*t + jMax*Power(t,2) - vd))/Abs(jMax);
            profile.t[5] = 0;
            profile.t[6] = profile.t[4] - af/jMax;
            profile.t[3] = tf - (profile.t[0] + profile.t[2] + profile.t[4] + profile.t[6]);

            if (profile.check<Teeth::UDUD>(tf, pf, vf, af, jMax, vMax, aMax)) {
                return true;
            }
        }
    }

    return false;
}

bool Step2::time_up_acc0_acc1(Profile& profile, double vMax, double aMax, double jMax) {
    if (std::abs(a0) < DBL_EPSILON && std::abs(af) < DBL_EPSILON) {
        const double h1 = (vd_vd + 4*aMax*pd)/(2*aMax_aMax*tf);

        profile.t[0] = tf/2 - h1 + (v0 + vf)/aMax;
        profile.t[1] = -(tf - 4*h1 + (5*v0 + 3*vf)/aMax)/2;
        profile.t[2] = profile.t[0];
        profile.t[3] = 0;
        profile.t[4] = profile.t[0];
        profile.t[5] = -(tf - 4*h1 + (3*v0 + 5*vf)/aMax)/2;
        profile.t[6] = profile.t[0];
        double jMaxNew = aMax/profile.t[0];

        return profile.check<Teeth::UDDU>(tf, pf, vf, af, jMaxNew, vMax, aMax, jMax);
    }

    double h0a = af_af*(-2*pd + tf*(v0 + vf)) + 2*af*aMax*(-2*pd + tf*(v0 + vf));
    double h1 = Sqrt(6*aMax*(6*Power(aMax,5)*tf_tf + 3*af_p4*(-2*pd + tf*(aMax*tf + 2*v0)) + 3*a0_p4*(-2*pd + tf*(aMax*tf + 2*vf)) + 6*af_af*aMax*(aMax_aMax*tf_tf + 2*vd_vd + aMax*(4*pd + tf*v0 - 5*tf*vf)) + 4*af_p3*(2*aMax_aMax*tf_tf + vd_vd + 2*aMax*(-pd + 2*tf*v0 - tf*vf)) - 4*a0_p3*(2*aMax_aMax*tf_tf + vd_vd + 2*aMax*(-pd - tf*v0 + 2*tf*vf)) - 12*af*aMax_aMax*(-vd_vd + aMax*(-4*pd + tf*v0 + 3*tf*vf)) - 6*a0_a0*(h0a - aMax*(aMax_aMax*tf_tf + 2*vd_vd + aMax*(4*pd - 5*tf*v0 + tf*vf))) + 12*a0*aMax*(h0a + aMax*(-vd_vd + aMax*(-4*pd + 3*tf*v0 + tf*vf)))));
    double h2 = -2*ad*(3*a0_p3 - 3*af_p3 + a0_a0*(3*af - 4*aMax) - 4*af_af*aMax + 12*af*aMax_aMax + 24*Power(aMax,3) - a0*(3*af_af + 16*af*aMax + 12*aMax_aMax));
    double h3 = (-6*a0_a0*af_af*tf + 6*af_p4*tf - 4*a0_p3*aMax*tf - 12*a0_a0*af*aMax*tf + 16*af_p3*aMax*tf + 18*a0_a0*aMax_aMax*tf + 30*af_af*aMax_aMax*tf + 36*ad*Power(aMax,3)*tf + 48*Power(aMax,4)*tf - 4*a0_p3*vd + 12*a0*af_af*vd - 8*af_p3*vd + 24*a0_a0*aMax*vd + 24*a0*af*aMax*vd - 48*af_af*aMax*vd - 24*a0*aMax_aMax*vd - 72*af*aMax_aMax*vd);
    double h4 = aMax_aMax*tf_tf - vd_vd + 2*aMax*(-2*pd + tf*(v0 + vf));
    double h5 = 2*Power(aMax,3)*tf + (af_af + 2*af*aMax)*(aMax*tf - vd) + (a0_a0 - 2*a0*aMax)*(aMax*tf + vd);
    double jMaxNew = (3*aMax*tf*(a0_a0 + af_af + 2*ad*aMax + 2*aMax_aMax) + 3*a0_a0*vd - 3*af_af*vd - 6*(a0 + af)*aMax*vd + Sqrt(9*Power(h5,2) - 3*ad*h2*h4))/(6*h4);

    profile.t[0] = (-a0 + aMax)/jMaxNew;
    profile.t[1] = (h3 + h1*(a0_a0 - af_af - 2*ad*aMax - 8*aMax_aMax)/aMax)/h2;
    profile.t[2] = profile.t[0] + a0/jMaxNew;
    profile.t[3] = 0;
    profile.t[4] = profile.t[2];
    profile.t[5] = tf - (profile.t[0] + profile.t[1] + profile.t[2] + profile.t[3] + 2*profile.t[4] + af/jMaxNew);
    profile.t[6] = profile.t[4] + af/jMaxNew;
    
    return profile.check<Teeth::UDDU>(tf, pf, vf, af, jMaxNew, vMax, aMax, jMax);
}

bool Step2::time_up_acc1(Profile& profile, double vMax, double aMax, double jMax) {
    // a3 != 0
    
    // Case UDDU, Solution 2
    {
        const double h0a = a0_p3 - af_p3 + 3*a0_a0*aMax + 3*a0*aMax_aMax + 3*aMax_aMax*jMax*tf - 3*af*aMax*(aMax - 2*jMax*tf) - 3*af_af*(aMax - jMax*tf) - 3*jMax_jMax*(-2*pd + aMax*tf_tf + 2*tf*vf);
        const double h0b = a0_a0 + af_af + 2*(a0 + af)*aMax + 2*(aMax_aMax - jMax*(aMax*tf + vd));
        const double h0c = a0_p4 + 3*af_p4 + 4*a0_p3*aMax + 8*af_p3*aMax + 6*a0_a0*aMax_aMax + 6*af_af*(aMax_aMax - 2*jMax*vd) + 12*jMax*(-2*aMax*jMax*(-pd + tf*v0) - aMax_aMax*vd + jMax*vd_vd) - 24*af*aMax*jMax*vd - 4*a0*(af_p3 + 3*af*aMax*(aMax - 2*jMax*tf) + 3*af_af*(aMax - jMax*tf) + 3*jMax*(-(aMax_aMax*tf) + jMax*(-2*pd + aMax*tf_tf + 2*tf*vf)));
        const double h1 = Abs(jMax)/jMax*Sqrt(4*Power(h0a,2) - 6*h0b*h0c);
        const double h2 = 6*jMax*h0b;
        const double h3 = 4*a0_p3 + 6*a0*af_af + 2*af_p3 + 12*a0_a0*aMax + 12*(a0 + af)*af*aMax + 18*(a0 + af)*aMax_aMax + 12*Power(aMax,3) - 12*jMax_jMax*pd - 6*af_af*jMax*tf - 12*(a0 + af)*aMax*jMax*tf - 18*aMax_aMax*jMax*tf + 6*aMax*jMax_jMax*tf_tf - 12*(a0 + aMax)*jMax*vd + 12*jMax_jMax*tf*vf;

        profile.t[0] = 0;
        profile.t[1] = 0;
        profile.t[2] = (2*h0a + h1)/h2;
        profile.t[3] = -(h3 + h1)/h2;
        profile.t[4] = (aMax + a0)/jMax - profile.t[2];
        profile.t[5] = tf - (profile.t[2] + profile.t[3] + profile.t[4] + (af + aMax)/jMax);
        profile.t[6] = (af + aMax)/jMax;

        if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
            return true;
        }
    }

    // Case UDUD, Solution 1
    {
        const double h0a = -a0_p3 + af_p3 + 3*a0_a0*aMax - 3*a0*aMax_aMax + 3*af*aMax*(aMax - 2*jMax*tf) - 3*af_af*(aMax - jMax*tf) + 3*jMax*(aMax_aMax*tf + jMax*(-2*pd - aMax*tf_tf + 2*tf*vf));
        const double h0b = a0_a0 - af_af + 2*ad*aMax + 2*jMax*(aMax*tf - vd);
        const double h0c = a0_p4 + 3*af_p4 - 4*a0_p3*aMax - 8*af_p3*aMax + 6*a0_a0*aMax_aMax - 24*af*aMax*jMax*vd + 12*jMax*(2*aMax*jMax*(-pd + tf*v0) + jMax*vd_vd + aMax_aMax*vd) + 6*af_af*(aMax_aMax + 2*jMax*vd) - 4*a0*(af_p3 + 3*af*aMax*(aMax - 2*jMax*tf) - 3*af_af*(aMax - jMax*tf) + 3*jMax*(aMax_aMax*tf + jMax*(-2*pd - aMax*tf_tf + 2*tf*vf)));
        const double h1 = Abs(jMax)/jMax*Sqrt(4*Power(h0a,2) - 6*h0b*h0c);
        const double h2 = 3*jMax*h0b;

        profile.t[0] = 0;
        profile.t[1] = 0;
        profile.t[2] = -(h0a + h1/2)/h2;
        profile.t[3] = h1/h2;
        profile.t[4] = (aMax - a0)/jMax + profile.t[2];
        profile.t[5] = tf - (profile.t[2] + profile.t[3] + profile.t[4] + (-af + aMax)/jMax);
        profile.t[6] = (-af + aMax)/jMax;

        if (profile.check<Teeth::UDUD>(tf, pf, vf, af, jMax, vMax, aMax)) {
            return true;
        }
    }
    return false;
}

bool Step2::time_up_acc0(Profile& profile, double vMax, double aMax, double jMax) {
    // a3 != 0

    const double h0a = a0_p3 + 2*af_p3 - 6*af_af*aMax - 6*Power(aMax,3) - 6*(a0 + af)*aMax*jMax*tf + 9*aMax_aMax*(af + jMax*tf) + 3*a0*aMax*(-2*af + 3*aMax) + 3*a0_a0*(af - 2*aMax + jMax*tf) - 6*jMax_jMax*(-pd + tf*v0) + 6*af*jMax*vd - 3*aMax*jMax*(jMax*tf_tf + 2*vd);
    const double h0b = a0_a0 + af_af - 2*(a0 + af)*aMax + 2*(aMax_aMax - aMax*jMax*tf + jMax*vd);
    const double h1 = Abs(jMax)/jMax*Sqrt(4*Power(h0a,2) - 18*Power(h0b,3));
    const double h2 = 6*jMax*(a0_a0 + af_af + 2*(aMax_aMax - (a0 + af)*aMax - aMax*jMax*tf + jMax*vd));
    const double h3 = (2*a0_p3 + 6*a0_a0*af + 4*af_p3 - 12*af_af*aMax + (18*aMax - 12*a0)*(a0 + af)*aMax - 12*Power(aMax,3) + 12*jMax_jMax*pd + 6*a0_a0*jMax*tf + (18*aMax - 12*(a0 + af))*aMax*jMax*tf - 6*jMax_jMax*tf*(aMax*tf + 2*v0) + 12*(af - aMax)*jMax*vd);

    // Solution 1, UDDU?
    profile.t[0] = (-a0 + aMax)/jMax;
    profile.t[2] = -(h3 + h1)/h2;
    profile.t[3] = (h3 - h1)/h2;
    profile.t[1] = ad/jMax - 2 * profile.t[0] - profile.t[3] + tf;
    profile.t[4] = tf - (profile.t[0] + profile.t[1] + profile.t[2] + profile.t[3]);
    profile.t[5] = 0;
    profile.t[6] = 0;

    if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
        return true;
    }

    return false;
}

bool Step2::time_up_none(Profile& profile, double vMax, double aMax, double jMax) {
    if (std::abs(v0) < DBL_EPSILON && std::abs(a0) < DBL_EPSILON && std::abs(vf) < DBL_EPSILON && std::abs(af) < DBL_EPSILON) {
        const double jMaxNew = 32*pd/Power(tf,3);

        profile.t[0] = tf/4;
        profile.t[1] = 0;
        profile.t[2] = profile.t[0];
        profile.t[3] = 0;
        profile.t[4] = profile.t[0];
        profile.t[5] = 0;
        profile.t[6] = profile.t[0];

        return profile.check<Teeth::UDDU>(tf, pf, vf, af, jMaxNew, vMax, aMax, jMax);
    }
    
    if (std::abs(v0) < DBL_EPSILON && std::abs(a0) < DBL_EPSILON) {
        const double h1 = Sqrt(tf_tf*vf_vf + Power(4*pd - tf*vf,2));
        const double jMaxNew = 4*(4*pd - 2*tf*vf + h1)/Power(tf,3);
        
        profile.t[0] = (-4*pd + 3*tf*vf + h1)/(4*vf);
        profile.t[1] = 0;
        profile.t[2] = profile.t[0];
        profile.t[3] = 0;
        profile.t[4] = profile.t[0];
        profile.t[5] = 0;
        profile.t[6] = profile.t[0];

        if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMaxNew, vMax, aMax, jMax)) {
            return true;
        }
    }

    /* if (std::abs(a0) < DBL_EPSILON && std::abs(vf) < DBL_EPSILON) {
        // Solution 1
        {
            double jMaxNew = (-4*(-4*pd*tf + 2*tf_tf*v0 + Sqrt(tf_tf*(tf_tf*v0_v0 + 4*Power(-2*pd + tf*v0,2)))))/Power(tf,4);

            profile.t[0] = (4*pd - tf*v0 + Sqrt(tf_tf*(tf_tf*v0_v0 + 4*Power(-2*pd + tf*v0,2)))/tf)/(4*v0);
            profile.t[1] = 0;
            profile.t[2] = profile.t[0];
            profile.t[3] = 0;
            profile.t[4] = (-4*pd + 3*tf*v0 - Sqrt(tf_tf*(tf_tf*v0_v0 + 4*Power(-2*pd + tf*v0,2)))/tf)/(4*v0);
            profile.t[5] = 0;
            profile.t[6] = profile.t[4];

            if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMaxNew, vMax, aMax, jMax)) {
                return true;
            }
        }
    } */

    if (std::abs(a0) < DBL_EPSILON && std::abs(af) < DBL_EPSILON) {
        // Solution 1
        {
            const double h1 = Sqrt(tf_tf*vd_vd + 4*Power(2*pd - tf*(v0 + vf),2));
            const double jMaxNew = 4*(4*pd - 2*tf*(v0 + vf) - Sqrt(16*(Power(pd,2) - pd*tf*(v0 + vf)) + tf_tf*(5*v0_v0 + 6*v0*vf + 5*vf_vf)))/Power(tf,3);
        
            profile.t[0] = (-4*pd + tf*(v0 + 3*vf) - h1)/(4*vd);
            profile.t[1] = 0;
            profile.t[2] = profile.t[0];
            profile.t[3] = 0;
            profile.t[4] = -(-4*pd + tf*(3*v0 + vf) - h1)/(4*vd);
            profile.t[5] = 0;
            profile.t[6] = profile.t[4];
            
            if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMaxNew, vMax, aMax, jMax)) {
                return true;
            }
        }
    }

    // Profiles with a3 != 0, Solution UDDU
    {
        // First acc, then constant
        {
            std::array<double, 5> polynom;
            polynom[0] = 1.0;
            polynom[1] = (-2*(-a0 + af + jMax*tf))/jMax;
            polynom[2] = (2*a0_a0 + 2*af_af + 2*af*jMax*tf - 4*a0*(af + jMax*tf) + jMax*(jMax*tf_tf + 2*vd))/jMax_jMax;
            polynom[3] = (2*(a0_p3 - af_p3 - 3*af_af*jMax*tf - 3*a0_a0*(af + jMax*tf) + 3*a0*Power(af + jMax*tf,2) - 6*jMax_jMax*(-pd + tf*vf)))/(3*Power(jMax,3));
            polynom[4] = (a0_p4 + af_p4 + 4*af_p3*jMax*tf + 6*af_af*jMax_jMax*tf_tf - 4*a0_p3*(af + jMax*tf) + 6*a0_a0*Power(af + jMax*tf,2) + 24*af*jMax_jMax*(-pd + tf*v0) - 4*a0*(af_p3 + 3*af_af*jMax*tf + 6*jMax_jMax*(-pd + tf*vf)) + 12*jMax_jMax*(vd_vd + jMax*tf*(-2*pd + tf*(v0 + vf))))/(12*Power(jMax,4));
            auto roots = Roots::solveQuartMonic(polynom);

            for (double t: roots) {
                if (t < 0.0 || t > tf) {
                    continue;
                }

                profile.t[0] = t;
                profile.t[1] = 0;
                profile.t[2] = (a0_a0 + af_af - 2*af*jMax*t - 2*a0*(af + jMax*(-t + tf)) + 2*jMax*(jMax*t*(t - tf) + vd))/(2*jMax*(a0 - af + 2*jMax*t - jMax*tf));
                profile.t[3] = ad/jMax + (tf - 2*t);
                profile.t[4] = tf - (t + profile.t[2] + profile.t[3]);
                profile.t[5] = 0;
                profile.t[6] = 0;

                if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
                    return true;
                }
            }
        }

        // First constant, then acc
        {
            const double ph1 = ad_ad + 2*(af + a0)*jMax*tf - jMax*(jMax*tf_tf + 4*vd);
            const double ph2 = jMax*tf_tf*(-pd + tf*v0) - vd*(-2*pd - tf*v0 + 3*tf*vf);
            const double ph3 = 5*af_af - 8*af*jMax*tf + 2*jMax*(2*jMax*tf_tf - vd);
            const double ph4 = jMax_jMax*Power(tf,4) - 2*vd_vd + 8*jMax*tf*(-pd + tf*vf);
            const double ph5 = (5*af_p4 - 8*af_p3*jMax*tf - 12*af_af*jMax*(jMax*tf_tf + vd) + 24*af*jMax_jMax*(-2*pd + jMax*Power(tf,3) + 2*tf*vf) - 6*jMax_jMax*ph4);

            std::array<double, 5> polynom;
            polynom[0] = 1.0;
            polynom[1] = -(4*a0_p3 - 4*af_p3 - 12*a0_a0*(af - jMax*tf) + 6*a0*(2*af_af - 2*af*jMax*tf + jMax*(jMax*tf_tf - 2*vd)) + 6*af*jMax*(3*jMax*tf_tf + 2*vd) - 6*jMax_jMax*(-4*pd + jMax*Power(tf,3) - 2*tf*v0 + 6*tf*vf))/(3*jMax*ph1);
            polynom[2] = -(-a0_p4 - af_p4 + 4*a0_p3*(af - jMax*tf) + a0_a0*(-6*af_af + 8*af*jMax*tf - 4*jMax*(jMax*tf_tf - vd)) + 2*af_af*jMax*(jMax*tf_tf + 2*vd) - 4*af*jMax_jMax*(-3*pd + jMax*Power(tf,3) + 2*tf*v0 + tf*vf) + jMax_jMax*(jMax_jMax*Power(tf,4) - 8*vd_vd + 4*jMax*tf*(-3*pd + tf*v0 + 2*tf*vf)) + 2*a0*(2*af_p3 - 2*af_af*jMax*tf + af*jMax*(-3*jMax*tf_tf + 4*v0 - 4*vf) + jMax_jMax*(-6*pd + jMax*Power(tf,3) - 4*tf*v0 + 10*tf*vf)))/(jMax_jMax*ph1);
            polynom[3] = -(Power(a0,5) - Power(af,5) + af_p4*jMax*tf - 5*a0_p4*(af - jMax*tf) + 2*a0_p3*ph3 + 4*af_p3*jMax*(jMax*tf_tf + vd) - 12*af_af*jMax_jMax*(-2*pd + tf*(v0 + vf)) + 12*af*jMax_jMax*(-vd_vd + jMax*tf*(-2*pd + 3*tf*v0 - tf*vf)) - 2*a0_a0*(5*af_p3 - 9*af_af*jMax*tf - 6*af*jMax*vd + 6*jMax_jMax*(-2*pd - tf*v0 + 3*tf*vf)) - 12*Power(jMax,3)*ph2 + a0*ph5)/(3*Power(jMax,3)*ph1);
            polynom[4] = -(-Power(a0,6) - Power(af,6) + 6*Power(a0,5)*(af - jMax*tf) - 48*af_p3*jMax_jMax*(-pd + tf*v0) + 72*Power(jMax,3)*(jMax*Power(-pd + tf*v0,2) - Power(v0 - vf,3)) - 3*a0_p4*ph3 + 144*af*Power(jMax,3)*(-pd + tf*v0)*vd - 36*af_af*jMax_jMax*vd_vd + 6*af_p4*jMax*vd + 4*a0_p3*(5*af_p3 - 9*af_af*jMax*tf - 6*af*jMax*vd + 6*jMax_jMax*(-2*pd - tf*v0 + 3*tf*vf)) - 3*a0_a0*ph5 + 6*a0*(Power(af,5) - af_p4*jMax*tf - 4*af_p3*jMax*(jMax*tf_tf + vd) + 12*af_af*jMax_jMax*(-2*pd + tf*(v0 + vf)) - 12*af*jMax_jMax*(-vd_vd + jMax*tf*(-2*pd + 3*tf*v0 - tf*vf)) + 12*Power(jMax,3)*ph2))/(18*Power(jMax,4)*ph1);
            auto roots = Roots::solveQuartMonic(polynom);

            for (double t: roots) {
                if (t < 0.0 || t > tf) {
                    continue;
                }

                // Solution 2 with aPlat
                profile.t[0] = 0;
                profile.t[1] = 0;
                profile.t[2] = t;
                profile.t[3] = tf - 2*t - ad/jMax - Sqrt(2*(a0_a0 + af_af + 2*af*jMax*t - 2*a0*(af + jMax*(t - tf)) + 2*jMax*(jMax*t*(t - tf) - vd)))/Abs(jMax);
                profile.t[4] = -ad/(2*jMax) - (2 * t + profile.t[3] - tf)/2;
                profile.t[5] = 0;
                profile.t[6] = tf - (t + profile.t[3] + profile.t[4]);

                if (profile.check<Teeth::UDDU>(tf, pf, vf, af, jMax, vMax, aMax)) {
                    return true;
                }
            }       
        }
    }

    // Profiles with a3 != 0, Solution UDUD
    {
        // First constant, then acc
        {
            const double ph2 = jMax*tf_tf*(-pd + tf*v0) - vd*(-2*pd - tf*v0 + 3*tf*vf);
            const double ph3 = 5*af_af - 8*af*jMax*tf + 2*jMax*(2*jMax*tf_tf - vd);
            const double ph4 = jMax_jMax*Power(tf,4) - 2*vd_vd + 8*jMax*tf*(-pd + tf*vf);
            const double ph5 = (5*af_p4 - 8*af_p3*jMax*tf - 12*af_af*jMax*(jMax*tf_tf + vd) + 24*af*jMax_jMax*(-2*pd + jMax*Power(tf,3) + 2*tf*vf) - 6*jMax_jMax*ph4);
            const double ph6 = -vd_vd + jMax*tf*(-2*pd + 3*tf*v0 - tf*vf);

            std::array<double, 5> polynom;
            polynom[0] = 1.0;
            polynom[1] = (4*af*tf - 2*jMax*tf_tf - 4*vd)/(a0 - af + jMax*tf);
            polynom[2] = (-2*a0_p4 - 2*af_p4 + 8*af_p3*jMax*tf + 6*af_af*jMax_jMax*tf_tf + 8*a0_p3*(af - jMax*tf) - 12*a0_a0*Power(af - jMax*tf,2) - 12*af*jMax_jMax*(-pd + jMax*Power(tf,3) - 2*tf*v0 + 3*tf*vf) + 2*a0*(4*af_p3 - 12*af_af*jMax*tf + 9*af*jMax_jMax*tf_tf - 3*jMax_jMax*(2*pd + jMax*Power(tf,3) - 2*tf*vf)) + 3*jMax_jMax*(jMax_jMax*Power(tf,4) + 4*vd_vd - 4*jMax*tf*(pd + tf*v0 - 2*tf*vf)))/(3*jMax_jMax*Power(a0 - af + jMax*tf,2));
            polynom[3] = (-Power(a0,5) + Power(af,5) - af_p4*jMax*tf + 5*a0_p4*(af - jMax*tf) - 2*a0_p3*ph3 - 4*af_p3*jMax*(jMax*tf_tf + vd) + 12*af_af*jMax_jMax*(-2*pd + tf*(v0 + vf)) - 12*af*jMax_jMax*ph6 + 2*a0_a0*(5*af_p3 - 9*af_af*jMax*tf - 6*af*jMax*vd + 6*jMax_jMax*(-2*pd - tf*v0 + 3*tf*vf)) + 12*Power(jMax,3)*ph2 + a0*(-5*af_p4 + 8*af_p3*jMax*tf + 12*af_af*jMax*(jMax*tf_tf + vd) - 24*af*jMax_jMax*(-2*pd + jMax*Power(tf,3) + 2*tf*vf) + 6*jMax_jMax*ph4))/(3*Power(jMax,3)*Power(a0 - af + jMax*tf,2));
            polynom[4] = -(Power(a0,6) + Power(af,6) - 6*Power(a0,5)*(af - jMax*tf) + 48*af_p3*jMax_jMax*(-pd + tf*v0) - 72*Power(jMax,3)*(jMax*Power(-pd + tf*v0,2) - Power(v0 - vf,3)) + 3*a0_p4*ph3 - 6*af_p4*jMax*vd - 144*af*Power(jMax,3)*(-pd + tf*v0)*vd + 36*af_af*jMax_jMax*vd_vd - 4*a0_p3*(5*af_p3 - 9*af_af*jMax*tf - 6*af*jMax*vd + 6*jMax_jMax*(-2*pd - tf*v0 + 3*tf*vf)) + 3*a0_a0*ph5 - 6*a0*(Power(af,5) - af_p4*jMax*tf - 4*af_p3*jMax*(jMax*tf_tf + vd) + 12*af_af*jMax_jMax*(-2*pd + tf*(v0 + vf)) - 12*af*jMax_jMax*ph6 + 12*Power(jMax,3)*ph2))/(18*Power(jMax,4)*Power(a0 - af + jMax*tf,2));
            auto roots = Roots::solveQuartMonic(polynom);

            for (double t: roots) {
                if (t < 0.0 || t > tf) {
                    continue;
                }

                double h1 = Sqrt(((a0_a0 + af_af)/2 - af*(a0 + jMax*t) + a0*jMax*(t + tf) + jMax*(jMax*t*tf - vd)))/Abs(jMax);

                profile.t[0] = t;
                profile.t[1] = -ad/jMax + tf - 2 * h1;
                profile.t[2] = h1;
                profile.t[3] = 0;
                profile.t[4] = ad/jMax + h1 - t;
                profile.t[5] = 0;
                profile.t[6] = 0;
 
                if (profile.check<Teeth::UDUD>(tf, pf, vf, af, jMax, vMax, aMax)) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool Step2::time_down_acc0_acc1_vel(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_acc0_acc1_vel(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_acc1_vel(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_acc1_vel(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_acc0_vel(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_acc0_vel(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_vel(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_vel(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_acc0_acc1(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_acc0_acc1(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_acc1(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_acc1(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_acc0(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_acc0(profile, -vMax, -aMax, -jMax);
}

bool Step2::time_down_none(Profile& profile, double vMax, double aMax, double jMax) {
    return time_up_none(profile, -vMax, -aMax, -jMax);
}

bool Step2::get_profile(Profile& profile, double vMax, double aMax, double jMax) {
    profile.a[0] = a0;
    profile.v[0] = v0;
    profile.p[0] = p0;

    // Test all cases to get ones that match
    if (pf > p0) {
        if (time_up_acc0_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_none(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_down_none(profile, vMax, aMax, jMax)) {
        } else {
            return false;
        }

    } else {
        if (time_down_acc0_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc1_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_vel(profile, vMax, aMax, jMax)) {
        } else if (time_up_vel(profile, vMax, aMax, jMax)) {
        } else if (time_down_none(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_down_acc0_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_up_acc0_acc1(profile, vMax, aMax, jMax)) {
        } else if (time_up_none(profile, vMax, aMax, jMax)) {
        } else {
            return false;
        }
    }
    return true;
}

} // namespace ruckig
