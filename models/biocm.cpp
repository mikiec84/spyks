#include <array>
#include <cmath>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "utility.hpp"
#include "integrators.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

namespace spyks { namespace models {

template <typename value_type, typename time_type=double>
struct biocm {
        static const size_t N_PARAM = 92;
        static const size_t N_STATE = 12;
        static const size_t N_FORCING = 1;
        typedef typename std::array<value_type, N_STATE> state_type;
        value_type const * params;
        value_type const * forcing;
        time_type dt;

        biocm(value_type const * parameters, value_type const * forcing, time_type forcing_dt)
                : params(parameters), forcing(forcing), dt(forcing_dt) {}

        void operator()(state_type const & X, state_type & dXdt, time_type t) const {
                const value_type &C(params[0]), &E_l(params[1]),
                        &E_na(params[2]), &E_k(params[3]), &E_h(params[4]),
                        &g_l(params[5]), &g_na(params[6]), &g_kdr(params[7]),
                        &g_ka(params[8]), &g_kht(params[9]), &g_klt(params[10]),
                        &g_hcn(params[11]), &nam_v(params[12]), &nam_dv(params[13]),
                        &nam_tau(params[14]), &nah_v(params[15]), &nah_dv(params[16]),
                        nah_t0(params[17]), nah_t1(params[18]), &nah_tv(params[19]),
                        nah_tdv1(params[20]), nah_t2(params[21]), nah_tdv2(params[22]),
                        &kdrm_v(params[23]), &kdrm_dv(params[24]),
                        &kdrm_tau(params[25]), &kam_v(params[26]), &kam_dv(params[27]),
                        &kam_p(params[28]), kam_t0(params[29]), kam_t1(params[30]),
                        &kam_tv(params[31]), kam_tdv1(params[32]), kam_t2(params[33]),
                        kam_tdv2(params[34]), &kah_v(params[35]), &kah_dv(params[36]),
                        &kah_p(params[37]), kah_t0(params[38]), kah_t1(params[39]),
                        &kah_tv(params[40]), kah_tdv1(params[41]), kah_t2(params[42]),
                        kah_tdv2(params[43]), kac_t0(params[44]), kac_t1(params[45]),
                        &kac_tv(params[46]), kac_tdv1(params[47]), &kltm_v(params[48]),
                        &kltm_dv(params[49]), &kltm_p(params[50]), kltm_t0(params[51]),
                        kltm_t1(params[52]), &kltm_tv(params[53]),
                        kltm_tdv1(params[54]), kltm_t2(params[55]),
                        kltm_tdv2(params[56]), &klth_z(params[57]), &klth_v(params[58]),
                        &klth_dv(params[59]), klth_t0(params[60]), klth_t1(params[61]),
                        &klth_tv(params[62]), klth_tdv1(params[63]),
                        klth_t2(params[64]), klth_tdv2(params[65]),
                        &kht_phi(params[66]), &khtm_v(params[67]), &khtm_dv(params[68]),
                        &khtm_p(params[69]), khtm_t0(params[70]), khtm_t1(params[71]),
                        &khtm_tv(params[72]), khtm_tdv1(params[73]),
                        khtm_t2(params[74]), khtm_tdv2(params[75]), &khtn_v(params[76]),
                        &khtn_dv(params[77]), khtn_t0(params[78]), khtn_t1(params[79]),
                        &khtn_tv(params[80]), khtn_tdv1(params[81]),
                        khtn_t2(params[82]), khtn_tdv2(params[83]), &hcnh_v(params[84]),
                        &hcnh_dv(params[85]), hcnh_t0(params[86]), hcnh_t1(params[87]),
                        &hcnh_tv(params[88]), hcnh_tdv1(params[89]),
                        hcnh_t2(params[90]), hcnh_tdv2(params[91]);
                const value_type &V = X[0], &na_m = X[1], &na_h = X[2],
                        &kdr_m = X[3], &ka_m = X[4], &ka_h = X[5],
                        &ka_c = X[6], &klt_m = X[7], &klt_h = X[8],
                        &kht_m = X[9], &kht_n = X[10], &hcn_h = X[11];
                value_type  &dV = dXdt[0], &dna_m = dXdt[1], &dna_h = dXdt[2],
                        &dkdr_m = dXdt[3], &dka_m = dXdt[4], &dka_h = dXdt[5],
                        &dka_c = dXdt[6], &dklt_m = dXdt[7], &dklt_h = dXdt[8],
                        &dkht_m = dXdt[9], &dkht_n = dXdt[10], &dhcn_h = dXdt[11];
                const value_type I = interpolate(t, forcing, dt, N_FORCING)[0];
                value_type sstate, tau;

                // dV
                dV = 1/C * (g_l * (E_l - V) +
                            // Traub-Miles sodium
                            g_na * na_m * na_m * na_m * na_h * (E_na - V) +
                            // Traub-Miles delayed rectifier
                            g_kdr * kdr_m * kdr_m * kdr_m * kdr_m * (E_k - V) +
                            // from Rothman and Manis
                            g_ka * ka_m * ka_m * ka_m * ka_m * ka_h  * ka_c * (E_k - V) +
                            g_klt * klt_m * klt_m * klt_m * klt_m * klt_h * (E_k - V) +
                            g_kht * (kht_phi * kht_m * kht_m + (1 - kht_phi) * kht_n) * (E_k - V) +
                            g_hcn * hcn_h * (E_h - V) +
                            I);
                dna_m = dna_h = dkdr_m = dka_m = dka_h =
                        dka_c = dklt_m = dklt_h = dkht_m = dkht_n = dhcn_h = 0;
                // Na_m
                sstate = 1/(1 + exp(-(V - nam_v) / nam_dv));
                dna_m = (sstate - na_m) / nam_tau;

                // Na_h
                sstate = 1/(1 + exp((V - nah_v) / nah_dv));
                tau = nah_t0 + 1/(nah_t1 * exp((V - nah_tv) / nah_tdv1) +
                                  nah_t2 * exp(-(V - nah_tv) / nah_tdv2));
                dna_h = (sstate - na_h) / tau;

                // KDR
                sstate = 1/(1 + exp(-(V - kdrm_v) / kdrm_dv));
                dkdr_m = (sstate - kdr_m) / kdrm_tau;

                // KA_m
                sstate = pow(1 + exp(-(V - kam_v) / kam_dv), kam_p);
                tau = kam_t0 + 1/(kam_t1 * exp((V - kam_tv) / kam_tdv1) +
                                  kam_t2 * exp(-(V - kam_tv) / kam_tdv2));
                dka_m = (sstate - ka_m) / tau;

                // KA_h
                sstate = pow(1 + exp((V - kah_v) / kah_dv), kah_p);
                tau = kah_t0 + 1/(kah_t1 * exp((V - kah_tv) / kah_tdv1) +
                                  kah_t2 * exp(-(V - kah_tv) / kah_tdv2));
                dka_h = (sstate - ka_h) / tau;

                // KA_c (secondary inactivation variable)
                // steady-state is same as h
                tau = kac_t0 + kac_t1/(1 + exp(-(V - kac_tv) / kah_tdv1));
                dka_c = (sstate - ka_c) / tau;

                // KLT_m
                sstate = pow(1 + exp(-(V - kltm_v) / kltm_dv), kltm_p);
                tau = kltm_t0 + 1/(kltm_t1 * exp((V - kltm_tv) / kltm_tdv1) +
                                   kltm_t2 * exp(-(V - kltm_tv) / kltm_tdv2));
                dklt_m = (sstate - klt_m) / tau;

                // KLT_h
                sstate = (1 - klth_z) / (1 + exp((V - klth_v) / klth_dv)) + klth_z;
                tau = klth_t0 + 1/(klth_t1 * exp((V - klth_tv) / klth_tdv1) +
                                   klth_t2 * exp(-(V - klth_tv) / klth_tdv2));
                dklt_h = (sstate - klt_h) / tau;

                // // KHT_m
                sstate = pow(1 + exp(-(V - khtm_v) / khtm_dv), khtm_p);
                tau = khtm_t0 + 1/(khtm_t1 * exp((V - khtm_tv) / khtm_tdv1) +
                                   khtm_t2 * exp(-(V - khtm_tv) / khtm_tdv2));
                dkht_m = (sstate - kht_m) / tau;

                // KHT_n (second activation variable)
                sstate = 1/(1 + exp(-(V - khtn_v) / khtn_dv));
                tau = khtn_t0 + 1/(khtn_t1 * exp((V - khtn_tv) / khtn_tdv1) +
                                   khtn_t2 * exp(-(V - khtn_tv) / khtn_tdv2));
                dkht_n = (sstate - kht_n) / tau;

                // HCN
                sstate = 1/(1 + exp((V - hcnh_v) / hcnh_dv));
                tau = hcnh_t0 + 1/(hcnh_t1 * exp((V - hcnh_tv) / hcnh_tdv1) +
                                   hcnh_t2 * exp(-(V - hcnh_tv) / hcnh_tdv2));
                dhcn_h = (sstate - hcn_h) / tau;
        }
};

}}

using spyks::models::biocm;

PYBIND11_PLUGIN(biocm) {
        typedef double value_type;
        typedef double time_type;
        typedef biocm<value_type, time_type> dbiocm;
        py::module m("biocm", "adaptive exponential model");
        py::class_<dbiocm>(m, "model")
                .def("__init__",
                     [](dbiocm &m,
                        py::array_t<value_type, py::array::c_style | py::array::forcecast> params,
                        py::array_t<value_type, py::array::c_style | py::array::forcecast> forcing,
                        time_type forcing_dt) {
                             auto pptr = static_cast<value_type const *>(params.data());
                             auto dptr = static_cast<value_type const *>(forcing.data());
                             new (&m) dbiocm(pptr, dptr, forcing_dt);
                     })
                .def("__call__", [](dbiocm const & m, dbiocm::state_type const & X, time_type t) {
                                dbiocm::state_type out;
                                m(X, out, t);
                                return out;
                        });
        m.def("integrate", [](py::array_t<value_type, py::array::c_style | py::array::forcecast> params,
                              dbiocm::state_type x0,
                              py::array_t<value_type, py::array::c_style | py::array::forcecast> forcing,
                              time_type forcing_dt, time_type stepping_dt) -> py::array {
                      auto pptr = static_cast<value_type const *>(params.data());
                      py::buffer_info forcing_info = forcing.request();
                      auto dptr = static_cast<value_type const *>(forcing_info.ptr);
                      time_type tmax = forcing_info.shape[0] * forcing_dt;
                      dbiocm model(pptr, dptr, forcing_dt);
                      return spyks::integrate(model, x0, tmax, stepping_dt);
              },
              "Integrates model from starting state x0 over the duration of the forcing timeseries",
              "params"_a, "x0"_a, "forcing"_a, "forcing_dt"_a, "stepping_dt"_a);
        m.def("integrate", &spyks::integrate<dbiocm>);
        return m.ptr();
}
