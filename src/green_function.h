#pragma once

#include "types.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/multi_array.hpp>

#include "util.h"
#include "U_matrix.h"
#include "operator.hpp"

#include "spline.h"


namespace alps {
    namespace ctint {

        typedef std::valarray<int> quantum_number_t;

        template <typename T> class green_function {
        public:
            green_function() : data_(), tau_(), beta_(0.0) {}

            void read_itime_data(const std::string& input_file, double beta, int flavors, int sites) {
              beta_ = beta;

              std::ifstream ifs(input_file);
              if (!ifs.is_open()) {
                throw std::runtime_error(input_file+" does not exist!");
              }
              int n_flavor, n_site, n_tau;
              double beta_in;
              ifs >> n_flavor >> n_site >> n_tau >> beta_in;
              if (flavors != n_flavor || sites != n_site) {
                throw std::runtime_error("Wrong # of sites or flavors is given in " + input_file);
              }
              if (std::abs(beta_in - beta) > 1e-8) {
                throw std::runtime_error("Wrong value of beta is given in " + input_file);
              }

              tau_.resize(n_tau);
              dtau_ = beta/(n_tau-1);
              inv_dtau_ = 1/dtau_;
              ntau_ = n_tau;
              data_.resize(boost::extents[n_flavor][n_site][n_site][n_tau]);
              splines_re_.resize(boost::extents[n_flavor][n_site][n_site]);
              splines_im_.resize(boost::extents[n_flavor][n_site][n_site]);
              spline_coeff_.resize(boost::extents[n_flavor][n_site][n_site][n_tau-1][4]);


              for (int t=0; t < n_tau; ++t) {
                tau_[t] = beta * static_cast<double>(t)/(n_tau-1);
              }

              // Read data
              int flavor_tmp, itmp, itmp2, itmp3;
              std::vector<double> y_re(n_tau), y_im(n_tau);
              double re, im;
              int line = 1 + n_tau;
              for (spin_t flavor=0; flavor<n_flavor; ++flavor) {
                for (std::size_t site1=0; site1<n_site; ++site1) {
                  for (std::size_t site2=0; site2<n_site; ++site2) {
                    for (std::size_t itau = 0; itau < n_tau; itau++) {
                      ifs >> flavor_tmp >> itmp >> itmp2 >> itmp3 >> re >> im;

                      if (flavor_tmp != flavor) {
                        throw std::runtime_error(
                          (boost::format("Bad format in G0_TAU: We expect %1% at the first column of the line %2%") % flavor %
                           line).str().c_str());
                      }
                      if (itmp != site1) {
                        throw std::runtime_error(
                          (boost::format("Bad format in G0_TAU: We expect %1% at the second column of the line %2%") % site1 %
                           line).str().c_str());
                      }
                      if (itmp2 != site2) {
                        throw std::runtime_error(
                          (boost::format("Bad format in G0_TAU: We expect %1% at the third column of the line %2%") %
                           site2 % line).str().c_str());
                      }
                      if (itmp3 != itau) {
                        throw std::runtime_error(
                          (boost::format("Bad format in G0_TAU: We expect %1% at the fourth column of the line %2%") % itau %
                           line).str().c_str());
                      }
                      data_[flavor][site1][site2][itau] = mycast<T>(std::complex<double>(re, im));
                      y_re[itau] = std::real(data_[flavor][site1][site2][itau]);
                      y_im[itau] = std::imag(data_[flavor][site1][site2][itau]);
                      ++line;
                    }

                    // cublic spline
                    splines_re_[flavor][site1][site2].set_points(tau_, y_re);
                    splines_im_[flavor][site1][site2].set_points(tau_, y_im);
                    for (int t=0; t < n_tau-1; ++t) {
                      for (int p=0; p < 4; ++p) {
                        spline_coeff_[flavor][site1][site2][t][p] =
                          std::complex<double>(
                            splines_re_[flavor][site1][site2].get_coeff(t,p),
                            splines_im_[flavor][site1][site2].get_coeff(t,p)
                          );
                      }
                    }
                  }
                }
              }

              for (int flavor=0; flavor < num_flavors(); ++flavor) {
                for (int site=0; site<num_sites(); ++site) {
                for (int site2=0; site2<num_sites(); ++site2) {
                for (auto tau : {0.0, 0.5 * beta, beta}) {
                  if(std::abs(interpolate(flavor, site, site2, tau) - std::conj(interpolate(flavor, site2, site, tau))) > 1e-8) {
                    throw std::runtime_error("G0 is not hermite!");
                  }
                }
                }
                }
              }
            }

            /*
             * Intepolate G(tau) as <T c c^dagger>
             */
            T operator()(const annihilator &c, const creator &cdagger) const {
              assert(c.flavor() == cdagger.flavor());

              const int flavor = c.flavor();

              const int site1 = c.s(), site2 = cdagger.s();
              double dt = c.t().time() - cdagger.t().time();

              if (dt == 0.0) {
                if (c.t().small_index() > cdagger.t().small_index()) { //G(+delta)
                  return interpolate(flavor, site1, site2, 0.0);
                } else { //G(-delta)
                  return -interpolate(flavor, site1, site2, beta_);
                }
              } else {
                double sign = 1.0;

                while (dt >= beta_) {
                  dt -= beta_;
                  sign *= -1.0;
                }
                while (dt < 0.0) {
                  dt += beta_;
                  sign *= -1.0;
                }

                assert(dt >= 0 && dt <= beta_);

                return sign * interpolate(flavor, site1, site2, dt);
              }
            }

            int num_flavors() const {
              return data_.shape()[0];
            }

            int num_sites() const {
              return data_.shape()[1];
            }

            int nsite() const {
              return num_sites();
            }

            int nflavor() const {
              return num_flavors();
            }

            int num_tau_points() const {
              return data_.shape()[3];
            }

            double tau(int itau) const {
              return tau_[itau];
            }

            // Interpolate G(tau) for 0 <= tau <= beta. We assume G(tau) is continous in this interval.
            T interpolate(int flavor, int site, int site2, double tau) const {
              assert(tau >= 0 && tau <= beta_);

              int idx = static_cast<std::size_t>(tau * inv_dtau_);
              if (idx == ntau_-1) {
                idx = ntau_-2;
              }
#ifndef NDEBUG
              if (idx < 0 || idx >= ntau_-1) {
                std::cerr << "interpolation error idx " << idx << " tau " << tau << std::endl;
              }
#endif
              assert(idx < ntau_-1);
              double h = tau - idx * dtau_;

              std::complex<double> y = spline_coeff_[flavor][site][site2][idx][0];
              std::complex<double> c = spline_coeff_[flavor][site][site2][idx][1];
              std::complex<double> b = spline_coeff_[flavor][site][site2][idx][2];
              std::complex<double> a = spline_coeff_[flavor][site][site2][idx][3];
              std::complex<double> intpl_val = ((a*h + b)*h + c)*h + y;

#ifndef NDEBUG
              std::complex<double> intpl_val_debug = mycast<T>(
                std::complex<double>(splines_re_[flavor][site][site2](tau), splines_im_[flavor][site][site2](tau))
              );
              assert(std::abs(intpl_val-intpl_val_debug) < 1e-8);
#endif

              return mycast<T>(intpl_val);
            }

            /*
             * if delta_t = n beta (n = 0, +/- 1, ...), it assume delta_t = +0
             */
            T operator()(double delta_t, int flavor, int site1, int site2) const {
              double dt = delta_t;
              T sign = 1.0;
              while (dt >= beta_) {
                dt -= beta_;
                sign *= -1.0;
              }
              while (dt < 0.0) {
                dt += beta_;
                sign *= -1.0;
              }

              if (dt == 0.0) dt += 1E-8;

              return sign * interpolate(flavor, site1, site2, dt);
            }

            bool is_zero(int flavor, int site1, int site2, double eps) const {
              return std::abs(interpolate(flavor, site1, site2, beta_* 1E-5)) < eps &&
                     std::abs(interpolate(flavor, site1, site2, beta_ * (1 - 1E-5))) < eps;
            }

        private:
            // flavor, site, site, tau
            boost::multi_array<T,4> data_;
            double dtau_, inv_dtau_;
            int ntau_;
            std::vector<double> tau_;
            boost::multi_array<tk::spline,3> splines_re_, splines_im_;
            boost::multi_array<std::complex<double>,5> spline_coeff_;//n_flavor, n_site, n_site, ntau-1, 3
            double beta_;
        };

//groups(groups, sites belonging to groups)
        template<class T>
        void
        make_groups(size_t N, const T& connected, std::vector<std::vector<size_t> >& groups, std::vector<int>& map) {
          map.resize(N);
          std::fill(map.begin(),map.end(),-1);
          groups.clear();

          for (size_t site1=0; site1<N; ++site1) {
            int connected_to_where = -1;
            for (size_t site2=0; site2<site1; ++site2) {
              if (connected[site1][site2] && map[site2]!=-1) {
                connected_to_where = map[site2];
                break;
              }
            }
            if (connected_to_where==-1) {
              //create a new group
              groups.resize(groups.size()+1);
              groups[groups.size()-1].push_back(site1);
              map[site1] = groups.size()-1;
            } else {
              groups[connected_to_where].push_back(site1);
              map[site1] = connected_to_where;
            }
          }
#ifndef NDEBUG
          {
            size_t t_sum = 0;
            for (size_t ig=0; ig<groups.size(); ++ig) {
              t_sum += groups[ig].size();
            }
            assert(t_sum==N);
            for (size_t i=0; i<N; ++i) {
              assert(map[i]>=0);
            }
          }
#endif
        }

//template<class T>
//bool compare_array_size(std::vector<T>& array1, std::vector<T>& array2) {
        //return array1.size()<array2.size();
//}

        template<class T>
        void print_group(const std::vector<std::vector<T> >& group) {
          for (int i=0; i<group.size(); ++i) {
            std::cout << "   Group "<<i<<" consists of site ";
            for (int j=0; j<group[i].size(); ++j) {
              std::cout << group[i][j];
              if (j<group[i].size()-1)
                std::cout << ", ";
            }
            std::cout << "." << std::endl;
          }
        }

        template<typename T, typename S, typename G>//Expected T,S=double or T=std::complex<double>
        std::vector<std::vector<quantum_number_t> >
        make_quantum_numbers(const G& gf, const std::vector<vertex_definition<S> >& vertices,
        std::vector<std::vector<std::vector<size_t> > >& groups,
          std::vector<std::vector<int> >& group_map,
        double eps=1E-10) {
        const size_t n_site = gf.nsite();
        const size_t n_flavors = gf.nflavor();

        //See if two sites are connected by nonzero G
        boost::multi_array<bool,2> connected(boost::extents[n_site][n_site]);
        groups.clear(); groups.resize(n_flavors);
        group_map.clear(); group_map.resize(n_flavors);
        std::vector<size_t> num_groups(n_flavors);

        //figure out how sites are connected by GF
        for (spin_t flavor=0; flavor<n_flavors; ++flavor) {
        for (size_t site1 = 0; site1 < n_site; ++site1) {
        for (size_t site2 = 0; site2 < n_site; ++site2) {
        connected[site1][site2] = !gf.is_zero(site1, site2, flavor, eps);
    }
}
make_groups(n_site, connected, groups[flavor], group_map[flavor]);
num_groups[flavor] = groups[flavor].size();
}

//determine the dimension of quantum numbers
const size_t n_dim = *std::max_element(num_groups.begin(), num_groups.end());

//compute quantum number for each vertex
const size_t Nv = vertices.size();
std::vector<std::vector<quantum_number_t> > qn_vertices(Nv);
for (size_t iv=0; iv<Nv; ++iv) {
const vertex_definition<S>& vd = vertices[iv];
const int num_af = vd.num_af_states();
for (int i_af=0; i_af<num_af; ++i_af) {
std::valarray<int> qn_diff(0, n_dim*n_flavors);
assert(qn_diff.size()==n_dim*n_flavors);
for (size_t i_rank=0; i_rank<vd.rank(); ++i_rank) {
const spin_t flavor = vd.flavors()[i_rank];
const size_t site1 = vd.sites()[2*i_rank];//c_dagger
const size_t site2 = vd.sites()[2*i_rank+1];//c

int PH;
if (site1==site2) {
//density type
PH = std::abs(vd.get_alpha(i_af,i_rank))<std::abs(vd.get_alpha(i_af,i_rank)-1.0) ? 1 : -1;
} else {
//non-density type
PH = 1;
}

//C^dagger
assert(n_dim*flavor+group_map[flavor][site1]<qn_diff.size());
qn_diff[n_dim*flavor+group_map[flavor][site1]] += PH;
//c
assert(n_dim*flavor+group_map[flavor][site2]<qn_diff.size());
qn_diff[n_dim*flavor+group_map[flavor][site2]] -= PH;
}
qn_vertices[iv].push_back(qn_diff);
}
}

return qn_vertices;
}
}
}
