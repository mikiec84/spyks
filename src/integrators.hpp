#ifndef INTEGRATORS_H
#define INTEGRATORS_H

#include <boost/numeric/odeint.hpp>

namespace ode = boost::numeric::odeint;

namespace spyks {

/** This observer does nothing. It's mostly here for benchmarking */
template <typename Model>
struct noop_observer {
        typedef typename Model::state_type state_type;
        py::array X;
        void operator()(state_type const & x, double time) {}
};

/** an observer that writes to a numpy array */
template <typename Model>
struct pyarray_dense {
        typedef typename Model::state_type state_type;
        pyarray_dense(size_t nsteps)
                : nsteps(nsteps), step(0),
                  X(py::dtype::of<double>(), {nsteps, Model::N_STATE}) {}
        void operator()(state_type const & x, double time) {
                if (step < nsteps) {
                        double * dptr = static_cast<double *>(X.mutable_data(step));
                        std::copy_n(x.begin(), Model::N_STATE, dptr);
                }
                ++step;
        }
        const size_t nsteps;
        size_t step;
        py::array X;
};

template <typename Model>
struct pyarray_sparse {
        typedef typename Model::state_type state_type;
        py::list X;
        py::list t;
        void operator()(state_type const & x, double time) {
                X.append(x);
                t.append(t);
        }
};

template <typename state_type>
class resetting_euler {
public:

        typedef typename state_type::value_type value_type;
        typedef state_type deriv_type;
        typedef double time_type;
        typedef unsigned short order_type;
        typedef boost::numeric::odeint::stepper_tag stepper_category;

        static order_type order( void ) { return 1; }

        template <typename System>
        void do_step(System system, state_type & x, time_type t, time_type dt) {
                if (!system.reset(x)) {
                        deriv_type dxdt;
                        system(x, dxdt, t);
                        for (size_t i = 0; i < x.size(); ++i)
                                x[i] += dt * dxdt[i];
                }
        }
};

template<typename Model>
py::array
integrate_reset(Model & model, typename Model::state_type x, double tmax, double dt)
{
        typedef typename Model::state_type state_type;
        double t = 0;
        size_t nsteps = floor(tmax / dt);
        auto obs = pyarray_dense<Model>(nsteps);
        auto stepper = resetting_euler<state_type>();
        ode::integrate_const(stepper, model, x, 0.0, tmax, dt, obs);
        return obs.X;
}

template<typename Model>
py::array
integrate(Model & model, typename Model::state_type x, double tmax, double dt)
{
        typedef typename Model::state_type state_type;
        double t = 0;
        size_t nsteps = ceil(tmax / dt);
        auto obs = pyarray_dense<Model>(nsteps);
        auto stepper = ode::runge_kutta_dopri5<state_type>();
        ode::integrate_const(ode::make_dense_output(1.0e-4, 1.0e-4, stepper),
                             std::ref(model), x, 0.0, tmax, dt, obs);
        return obs.X;
}

template<typename Model>
py::array
integrate_rk4(Model & model, typename Model::state_type x, double tmax, double dt)
{
        typedef typename Model::state_type state_type;
        double t = 0;
        size_t nsteps = ceil(tmax / dt);
        auto obs = pyarray_dense<Model>(nsteps);
        auto stepper = ode::runge_kutta4<state_type>();
        ode::integrate_const(stepper, std::ref(model), x, 0.0, tmax, dt, obs);
        return obs.X;
}


}  // namespace spyks::integrators

#endif /* INTEGRATORS_H */
