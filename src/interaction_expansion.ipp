/*****************************************************************************
 *
 * ALPS DMFT Project
 *
 * Copyright (C) 2005 - 2009 by Emanuel Gull <gull@phys.columbia.edu>
 *                              Philipp Werner <werner@itp.phys.ethz.ch>,
 *                              Sebastian Fuchs <fuchs@theorie.physik.uni-goettingen.de>
 *                              Matthias Troyer <troyer@comp-phys.org>
 *
 *
 * This software is part of the ALPS Applications, published under the ALPS
 * Application License; you can use, redistribute it and/or modify it under
 * the terms of the license, either version 1 or (at your option) any later
 *
 *
 * You should have received a copy of the ALPS Application License along with
 * the ALPS Applications; see the file LICENSE.txt. If not, the license is also
 * available from http://alps.comp-phys.org/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

namespace alps {
    namespace ctint {

#include <ctime>

#include <boost/lexical_cast.hpp>
#include "boost/tuple/tuple.hpp"

#include "interaction_expansion.hpp"

        template<typename T>
        BareGreenInterpolate<T>::BareGreenInterpolate(const alps::params &p) :
          beta_(p["BETA"].template as<int>()),
          temp_(1.0 / beta_),
          ntau_(p["N_TAU"].template as<int>()),
          n_flavors_(p["FLAVORS"].template as<int>()),
          n_sites_(p["SITES"].template as<int>()),
          dbeta_(beta_ / ntau_) {
          green_function<std::complex<double> > bare_green_matsubara(ntau_, n_sites_, n_flavors_),
            bare_green_itime(ntau_ + 1, n_sites_, n_flavors_);

          boost::tie(bare_green_matsubara, bare_green_itime) =
            read_bare_green_functions<std::complex<double> >(p);

          assert(ntau_ == bare_green_itime.ntime() - 1);
          AB_.resize(boost::extents[n_flavors_][n_sites_][n_sites_][ntau_ + 1]);

          for (int flavor = 0; flavor < n_flavors_; ++flavor) {
            for (int site1 = 0; site1 < n_sites_; ++site1) {
              for (int site2 = 0; site2 < n_sites_; ++site2) {
                for (int tau = 0; tau < ntau_; ++tau) {
                  const T a =
                    mycast<T>(
                      (bare_green_itime(tau + 1, site1, site2, flavor) - bare_green_itime(tau, site1, site2, flavor)) /
                      dbeta_
                    );
                  const T b = mycast<T>(bare_green_itime(tau, site1, site2, flavor));

                  AB_[flavor][site1][site2][tau] = std::make_pair(a, b);
                }
                AB_[flavor][site1][site2][ntau_] = std::make_pair(0.0, mycast<T>(
                  bare_green_itime(ntau_, site1, site2, flavor)));
              }
            }
          }
        }

/*
 * -beta <= d tau < beta
 */
        template<typename T>
        T BareGreenInterpolate<T>::operator()(const annihilator &c, const creator &cdagger) const {
          assert(c.flavor() == cdagger.flavor());

          const int flavor = c.flavor();

          const int site1 = c.s(), site2 = cdagger.s();
          double dt = c.t().time() - cdagger.t().time();
          if (dt == 0.0) {
            if (c.t().small_index() > cdagger.t().small_index()) { //G(+delta)
              return AB_[flavor][site1][site2][0].second;
            } else { //G(-delta)
              return -AB_[flavor][site1][site2][ntau_].second;
            }
          } else {
            T coeff = 1.0;
            while (dt >= beta_) {
              dt -= beta_;
              coeff *= -1.0;
            }
            while (dt < 0.0) {
              dt += beta_;
              coeff *= -1.0;
            }

            assert(dt >= 0 && dt <= beta_);
            const int time_index_1 = (int) (dt * ntau_ * temp_);
            return coeff * (AB_[flavor][site1][site2][time_index_1].first * (dt - time_index_1 * dbeta_) +
                            AB_[flavor][site1][site2][time_index_1].second);
          }
        }

//if delta_t==0, we assume delta_t = +0
        template<typename T>
        T BareGreenInterpolate<T>::operator()(double delta_t, int flavor, int site1, int site2) const {
          double dt = delta_t;
          T coeff = 1.0;
          while (dt >= beta_) {
            dt -= beta_;
            coeff *= -1.0;
          }
          while (dt < 0.0) {
            dt += beta_;
            coeff *= -1.0;
          }

          if (dt == 0.0) dt += 1E-8;

          const int time_index_1 = (int) (dt * ntau_ * temp_);
          return coeff * (AB_[flavor][site1][site2][time_index_1].first * (dt - time_index_1 * dbeta_) +
                          AB_[flavor][site1][site2][time_index_1].second);
        }

        template<typename T>
        bool BareGreenInterpolate<T>::is_zero(int site1, int site2, int flavor, double eps) const {
          return std::abs(operator()(beta_ * 1E-5, flavor, site1, site2)) < eps &&
                 std::abs(operator()(beta_ * (1 - 1E-5), flavor, site1, site2)) < eps;
        }

        template<class TYPES>
        InteractionExpansion<TYPES>::InteractionExpansion(parameters_type const &params, std::size_t seed_offset)
          : InteractionExpansionBase(parameters, seed_offset),
            parms(parameters),
//node(node),
            max_order(parms["MAX_ORDER"]),
            n_flavors(parms["FLAVORS"]),
            n_site(parms["SITES"]),
            n_matsubara(parms["N_MATSUBARA"]),
            n_matsubara_measurements(parms["NMATSUBARA_MEASUREMENTS"]),
            n_tau(parms["N_TAU"]),
            n_tau_inv(1. / n_tau),
            n_self(parms["NSELF"]),
            n_legendre(parms["N_LEGENDRE"]),
            mc_steps((boost::uint64_t) parms["SWEEPS"]),
            therm_steps(parms["THERMALIZATION"]),
            max_time_in_seconds(parms["MAX_TIME"]),
            beta(parms["BETA"]),
            temperature(1. / beta),
            Uijkl(parms),
            num_U_scale(parms["NUM_U_SCALE"]),
            min_U_scale(parms["MIN_U_SCALE"]),
            U_scale_vals(num_U_scale),
            U_scale_index(num_U_scale),
            M_flavors(n_flavors),
            recalc_period(parms["RECALC_PERIOD"]),
            measurement_period(
              parms.defined("MEASUREMENT_PERIOD") ? parms["MEASUREMENT_PERIOD"] : 500 * n_flavors * n_site),
            convergence_check_period(recalc_period),
            almost_zero(1.e-16),
//seed(parms["SEED"].template as<int>()+seed_offset),
            bare_green_matsubara(n_matsubara, n_site, n_flavors),
            bare_green_itime(n_tau + 1, n_site, n_flavors),
            pert_hist(max_order),
            legendre_transformer(n_matsubara, n_legendre),
            is_thermalized_in_previous_step_(false),
            n_ins_rem(parms["N_INS_REM_VERTEX"]),
            n_shift(parms["N_VERTEX_SHIFT"]),
            n_spin_flip(parms["N_SPIN_FLIP"]),
            force_quantum_number_conservation(false),
            single_vertex_update_non_density_type(false),
            pert_order_hist(max_order + 1),
            comm(),
            g0_intpl(parms),
            update_manager(parms, Uijkl, g0_intpl, comm.rank() == 0) {
          //other parameters
          step = 0;
          start_time = time(NULL);
          measurement_time = 0;
          update_time = 0;

          //initialize the simulation variables
          initialize_simulation(parms);

          //submatrix update
          itime_vertex_container itime_vertices_init;
          if (parms.defined("PREFIX_LOAD_CONFIG")) {
            std::ifstream is((parms["PREFIX_LOAD_CONFIG"].template as<std::string>()
                              + std::string("-config-node") + boost::lexical_cast<std::string>(comm.rank()) +
                              std::string(".txt")).c_str());
            load_config<typename TYPES::M_TYPE>(is, Uijkl, itime_vertices_init);
          }

          update_manager.create_observables(measurements);

          if (min_U_scale < 1.0 && num_U_scale == 1) {
            throw std::runtime_error("Invalid min_U_scale and num_U_scale!");
          }

          //submatrix_update =
          //new SubmatrixUpdate<M_TYPE>(
          //(parms["K_INS_MAX"] | 32), n_flavors,
          //g0_intpl, &Uijkl, beta, itime_vertices_init);
          for (int iu = 0; iu < num_U_scale; ++iu) {
            U_scale_vals[iu] = num_U_scale != 1 ? iu * (1.0 - min_U_scale) / (num_U_scale - 1.0) + min_U_scale : 1.0;
            U_scale_index[iu] = iu;
            walkers.push_back(
              WALKER_P_TYPE(
                new SubmatrixUpdate<M_TYPE>(
                  parms["K_INS_MAX"], n_flavors,
                  g0_intpl, &Uijkl, beta, itime_vertices_init)
              )
            );
          }
          submatrix_update = walkers[walkers.size() - 1];
#ifndef NDEBUG
          for (int i_walker = 0; i_walker < num_U_scale; ++i_walker) {
            std::cout << " step " << step << " node " << comm.rank() << " w " << i_walker << " pert "
                      << walkers[i_walker]->pert_order() << std::endl;
          }
#endif

          vertex_histograms = new simple_hist *[n_flavors];
          vertex_histogram_size = 100;
          for (unsigned int i = 0; i < n_flavors; ++i) {
            vertex_histograms[i] = new simple_hist(vertex_histogram_size);
          }

        }

        template<class TYPES>
        InteractionExpansion<TYPES>::~InteractionExpansion() {
          //delete submatrix_update;
        }

        template<class TYPES>
        void InteractionExpansion<TYPES>::update() {
          //std::valarray<double> t_meas(0.0, 3*num_U_scale), t_meas_tot(0.0, 3);

          pert_order_hist = 0.;

          for (std::size_t i = 0; i < measurement_period; ++i) {
#ifndef NDEBUG
            std::cout << " step " << step << std::endl;
#endif
            step++;

            for (int i_walker = 0; i_walker < num_U_scale; ++i_walker) {
              int U_index_walker;
              for (int iu = 0; iu < num_U_scale; ++iu) {
                if (U_scale_index[iu] == i_walker) {
                  U_index_walker = iu;
                  break;
                }
              }
              const double U_scale_walker = U_scale_vals[U_index_walker];
              //std::cout << "   walker " << i_walker << " " << U_scale_walker << " " << walkers[i_walker]->pert_order() << std::endl;
              //if (U_scale_walker+0.1<random()) {
              //continue;
              //}
              //if (i_walker==0) {
              //std::cout << " U_scale " << step << " " <<  U_scale_walker << std::endl;
              //}
#ifndef NDEBUG
              //std::cout << " walker " << i_walker << std::endl;
              std::cout << " step " << step << " node " << comm.rank() << " w " << i_walker << " pert "
                        << walkers[i_walker]->pert_order() << std::endl;
#endif
              //boost::timer::cpu_timer timer;

              for (int i_ins_rem = 0; i_ins_rem < n_ins_rem; ++i_ins_rem) {
                update_manager.do_ins_rem_update(*walkers[i_walker], Uijkl, random, U_scale_walker);
              }

              //double t_m = timer.elapsed().wall;

              for (int i_shift = 0; i_shift < n_shift; ++i_shift) {
                update_manager.do_shift_update(*walkers[i_walker], Uijkl, random, !is_thermalized());
                //update_manager.do_shift_update(*walkers[i_walker], Uijkl, random, false);
              }
              //std::cout << "   walker " << i_walker << " " << U_scale_walker << " " << walkers[i_walker]->pert_order() << std::endl;

              for (int i_spin_flip = 0; i_spin_flip < n_spin_flip; ++i_spin_flip) {
                update_manager.do_spin_flip_update(*walkers[i_walker], Uijkl, random);
              }

              //t_meas[0+3*U_index_walker] += t_m;
              //t_meas[1+3*U_index_walker] += (timer.elapsed().wall-t_m);

              //t_meas_tot[0] += t_m;
              //t_meas_tot[1] += (timer.elapsed().wall-t_m);

              if (step % recalc_period == 0) {
                //boost::timer::cpu_timer timer2;
                submatrix_update->recompute_matrix(true);

                update_manager.global_updates(walkers[i_walker], Uijkl, g0_intpl, random);
                //t_meas[2+3*U_index_walker] += timer2.elapsed().wall;
                //t_meas_tot[2] += timer2.elapsed().wall;
              }

              //measurement for walker with physical U
              if (U_scale_walker == 1.0) {
                if (submatrix_update->pert_order() < max_order) {
                  pert_hist[submatrix_update->pert_order()]++;
                }
                assert(submatrix_update->pert_order() < pert_order_hist.size());
                ++pert_order_hist[submatrix_update->pert_order()];

                for (spin_t flavor = 0; flavor < n_flavors; ++flavor) {
                  vertex_histograms[flavor]->count(submatrix_update->invA()[flavor].creators().size());
                }
              }
            }//i_walker

            //swaps U_SCALE
            exchange_update();
          }//for (int i_walker=0;

          //Save pertubation order for walker with physical U
          if (parms.defined("PREFIX_OUTPUT_TIME_SERIES")) {
            std::valarray<double> pert_vertex(Uijkl.n_vertex_type());
            const itime_vertex_container &itime_vertices = walkers[U_scale_index[U_scale_index.size() -
                                                                                 1]]->itime_vertices();
            assert(U_scale_vals[U_scale_index[U_scale_index.size() - 1]] == 1.0);
            for (std::vector<itime_vertex>::const_iterator it = itime_vertices.begin();
                 it != itime_vertices.end(); ++it) {
              assert(it->type() >= 0 && it->type() < Uijkl.n_vertex_type());
              ++pert_vertex[it->type()];
            }
            for (unsigned i_vt = 0; i_vt < Uijkl.n_vertex_type(); ++i_vt) {
              pert_order_dynamics.push_back(static_cast<double>(pert_vertex[i_vt]));
            }
          }

          //t_meas *= 1E-6/measurement_period;
          //measurements["UpdateTimeMsec"] << t_meas;
          //measurements["UpdateTimeMsecAllWalkers"] << t_meas_tot;

        }

        template<class TYPES>
        void InteractionExpansion<TYPES>::exchange_update() {
          if (num_U_scale == 1) {
            return;
          }
          std::valarray<double> acc(0.0, num_U_scale - 1);
          //for (int iu=0; iu<num_U_scale; ++iu) {
          //std::cout << "U_scale_index[iu] " << iu << " " << U_scale_index[iu] << std::endl;
          //}
          //const int iu_begin = random()<0.5 ? 0 : 1;
          for (int iu1 = 0; iu1 < num_U_scale - 1; ++iu1) {
            for (int iu2 = iu1 + 1; iu2 < num_U_scale; ++iu2) {
              const int i_walker1 = U_scale_index[iu1];
              const int i_walker2 = U_scale_index[iu2];

              const double prob = std::pow(U_scale_vals[iu2] / U_scale_vals[iu1],
                                           walkers[i_walker1]->pert_order() - walkers[i_walker2]->pert_order());

              //std::cout << "exchange rate " << iu1 << " " << iu2 << " : " << i_walker1 << " " << i_walker2 << " " << prob << std::endl;

              if (prob > random()) {
                if (iu2 - iu1 == 1) {
                  acc[iu1] = 1.0;
                }
                std::swap(U_scale_index[iu1], U_scale_index[iu2]);
              }
            }
          }
          submatrix_update = walkers[U_scale_index[num_U_scale - 1]];
          measurements["ACCEPTANCE_RATE_EXCHANGE"] << acc;

          if (num_U_scale > 2 && !is_thermalized()) {
            const double magic_number = 1.01;
            std::valarray<double> diff_U_scale(num_U_scale - 1);
            double sum_tmp = 0.0;
            for (int iu = 0; iu < num_U_scale - 1; ++iu) {
              diff_U_scale[iu] = acc[iu] > 0 ? (U_scale_vals[iu + 1] - U_scale_vals[iu]) * magic_number :
                                 (U_scale_vals[iu + 1] - U_scale_vals[iu]) / magic_number;
              sum_tmp += diff_U_scale[iu];
            }
            diff_U_scale *= (1.0 - min_U_scale) / sum_tmp;
            for (int iu = 0; iu < num_U_scale - 2; ++iu) {
              U_scale_vals[iu + 1] = U_scale_vals[iu] + diff_U_scale[iu];
            }
          }
          //if (node==0) {
          //std::cout << " Pert " << step << " ";
          //for (int iu=0; iu<num_U_scale; ++iu) {
          //std::cout << walkers[iu]->pert_order() << " ";
          //}
          //}
          //std::cout << std::endl;
          //if (node==1) {
          //std::cout << " U_scale_end " << step << " " <<  U_scale_index[num_U_scale-1] << " " << walkers[U_scale_index[num_U_scale-1]]->pert_order() << std::endl;
          //std::cout << " U_scale_begin " << step << " " <<  U_scale_index[0] << " " << walkers[U_scale_index[0]]->pert_order() << std::endl;
          //}
#ifndef NDEBUG
          std::cout << " U_scale_index: ";
          for (int iu = 0; iu < num_U_scale; ++iu) {
            std::cout << U_scale_index[iu] << " ";
          }
          std::cout << std::endl;
#endif
        }

        template<class TYPES>
        void InteractionExpansion<TYPES>::measure() {
          //In the below, real physical quantities are measured.
          //std::valarray<double> timings(2);
          measure_observables();
          //measurements["MeasurementTimeMsec"] << timings;
          update_manager.measure_observables(measurements);
        }

/**
 * Finalize the Monte Carlo simulation, e.g., write some data to disk.
 */
        template<class TYPES>
        void InteractionExpansion<TYPES>::finalize() {

          std::string node_str = boost::lexical_cast<std::string>(comm.rank());

          if (pert_order_dynamics.size() > 0) {
            std::ofstream ofs(
              (parms["PREFIX_OUTPUT_TIME_SERIES"].template as<std::string>() + std::string("-pert_order-node") +
               node_str + std::string(".txt")).c_str());
            unsigned int n_data = pert_order_dynamics.size() / Uijkl.n_vertex_type();
            unsigned int i = 0;
            for (unsigned int i_data = 0; i_data < n_data; ++i_data) {
              ofs << i_data << " ";
              for (unsigned int i_vt = 0; i_vt < Uijkl.n_vertex_type(); ++i_vt) {
                ofs << pert_order_dynamics[i] << " ";
                ++i;
              }
              ofs << std::endl;
            }
          }

          if (Wk_dynamics.size() > 0) {
            std::ofstream ofs(
              (parms["PREFIX_OUTPUT_TIME_SERIES"].template as<std::string>() + std::string("-Wk-node") + node_str +
               std::string(".txt")).c_str());
            unsigned int n_data = Wk_dynamics.size() / (n_flavors * n_site);
            unsigned int i = 0;
            for (unsigned int i_data = 0; i_data < n_data; ++i_data) {
              ofs << i_data << " ";
              for (unsigned int flavor = 0; flavor < n_flavors; ++flavor) {
                for (unsigned int site1 = 0; site1 < n_site; ++site1) {
                  ofs << Wk_dynamics[i].real() << " " << Wk_dynamics[i].imag() << "   ";
                  ++i;
                }
              }
              ofs << std::endl;
            }
          }

          if (Sl_dynamics.size() > 0) {
            std::ofstream ofs(
              (parms["PREFIX_OUTPUT_TIME_SERIES"].template as<std::string>() + std::string("-Sl-node") + node_str +
               std::string(".txt")).c_str());
            unsigned int n_data = Sl_dynamics.size() / (n_flavors * n_site);
            unsigned int i = 0;
            for (unsigned int i_data = 0; i_data < n_data; ++i_data) {
              ofs << i_data << " ";
              for (unsigned int flavor = 0; flavor < n_flavors; ++flavor) {
                for (unsigned int site1 = 0; site1 < n_site; ++site1) {
                  ofs << Sl_dynamics[i].real() << " " << Sl_dynamics[i].imag() << "   ";
                  ++i;
                }
              }
              ofs << std::endl;
            }
          }

          if (parms.defined("PREFIX_DUMP_CONFIG")) {
            std::ofstream os((parms["PREFIX_DUMP_CONFIG"].template as<std::string>()
                              + std::string("-config-node") + node_str + std::string(".txt")).c_str());
            dump(os, submatrix_update->itime_vertices());
          }

        }


        template<class TYPES>
        double InteractionExpansion<TYPES>::fraction_completed() const {
          if (!is_thermalized()) {
            return 0.;
          } else {
            return ((step - therm_steps) / (double) mc_steps);
          }
        }


///do all the setup that has to be done before running the simulation.
        template<class TYPES>
        void InteractionExpansion<TYPES>::initialize_simulation(const alps::params &parms) {
          pert_hist.clear();
          initialize_observables();
        }

/*
template<class TYPES>
bool InteractionExpansion<TYPES>::is_quantum_number_conserved(const itime_vertex_container& vertices) {
  const int Nv = vertices.size();

  if (Nv==0)
    return true;

  std::valarray<int> qn_t(0, qn_dim), qn_max(0, qn_dim), qn_min(0, qn_dim);
  itime_vertex_container vertices_sorted(vertices);//sort vertices in decreasing order (in time)
  std::sort(vertices_sorted.begin(), vertices_sorted.end());

  for (int iv=0; iv<Nv; ++iv) {
    const vertex_definition<M_TYPE> vd = Uijkl.get_vertex(vertices_sorted[iv].type());
    vd.apply_occ_change(vertices_sorted[iv].af_state(), qn_t, qn_max, qn_min);
  }

  //check if the quantum number is conserved
  for (int i=0; i<qn_t.size(); ++i) {
    if (qn_t[i]!=0) {
      return false;
    }
  }

  return true;
}
 */


//This makes sence in the absence of a bath
/*
template<class TYPES>
bool InteractionExpansion<TYPES>::is_quantum_number_within_range(const itime_vertex_container& vertices) {
  const int Nv = vertices.size();

  if (Nv==0)
    return true;

  std::valarray<int> qn_t(0, qn_dim), qn_max(0, qn_dim), qn_min(0, qn_dim);
  itime_vertex_container vertices_sorted(vertices);//sort vertices in decreasing order (in time)
  std::sort(vertices_sorted.begin(), vertices_sorted.end());

  for (int iv=0; iv<Nv; ++iv) {
    const vertex_definition<M_TYPE> vd = Uijkl.get_vertex(vertices_sorted[iv].type());
    vd.apply_occ_change(vertices_sorted[iv].af_state(), qn_t, qn_max, qn_min);
  }

  //check if the quantum number is within range
  for (int iq=0; iq<qn_dim; ++iq) {
    //note: group_dim[iq] is zero for non-existing group
    if (qn_max[iq]-qn_min[iq]>group_dim[iq]) {
      return false;
    }
  }

  return true;
}
*/

        template<class TYPES>
        void InteractionExpansion<TYPES>::sanity_check() {
#ifndef NDEBUG
#endif
        }


        template<class TYPES>
        void InteractionExpansion<TYPES>::prepare_for_measurement() {
          update_manager.prepare_for_measurement_steps();
        }

        template<typename T, typename SPLINE_G0>
        T
        compute_weight(const general_U_matrix<T> &Uijkl, const SPLINE_G0 &spline_G0,
                       const itime_vertex_container &itime_vertices) {
          T weight_U = 1.0, weight_det = 1.0;

          const int nflavors = Uijkl.nf();
          std::vector<std::vector<annihilator> > annihilators(nflavors);
          std::vector<std::vector<creator> > creators(nflavors);
          std::vector<std::vector<T> > alpha(nflavors);
          for (int iv = 0; iv < itime_vertices.size(); ++iv) {
            const itime_vertex &v = itime_vertices[iv];
            const vertex_definition<T> &vdef = Uijkl.get_vertex(v.type());
            weight_U *= -vdef.Uval();
            for (int rank = 0; rank < v.rank(); ++rank) {
              const int flavor_rank = vdef.flavors()[rank];
              operator_time op_t(v.time(), -rank);
              creators[flavor_rank].push_back(
                creator(flavor_rank, vdef.sites()[2 * rank], op_t)
              );
              annihilators[flavor_rank].push_back(
                annihilator(flavor_rank, vdef.sites()[2 * rank + 1], op_t)
              );
              alpha[flavor_rank].push_back(vdef.get_alpha(v.af_state(), rank));
            }
          }

          alps::numeric::matrix<T> G0, work;
          for (int flavor = 0; flavor < nflavors; ++flavor) {
            const int Nv = alpha[flavor].size();
            G0.destructive_resize(Nv, Nv);
            work.destructive_resize(Nv, Nv);
            for (int j = 0; j < Nv; ++j) {
              for (int i = 0; i < Nv; ++i) {
                G0(i, j) = spline_G0(annihilators[flavor][i], creators[flavor][j]);
              }
              G0(j, j) -= alpha[flavor][j];
            }
            weight_det *= G0.safe_determinant();
          }

          return weight_U * weight_det;
        }

        template<class TYPES>
        void InteractionExpansion<TYPES>::print(std::ostream &os) {
          os
            << "***********************************************************************************************************"
            << std::endl;
          os
            << "*** InteractionExpansion solver based on ALPSCore for multi-orbital cluster impurity model              ***"
            << std::endl;
          os
            << "***     Hiroshi Shinaoka and Yusuke Nomura                                                              ***"
            << std::endl;
          os
            << "*** This code implements the interaction expansion algorithm by Rubtsov et al., JETP Letters 80, 61.    ***"
            << std::endl;
          os
            << "***                                                                                                     ***"
            << std::endl;
          os
            << "***********************************************************************************************************"
            << std::endl;
          os << parms << std::endl;
        }
    }
}
