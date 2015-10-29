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
* version.
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

#ifndef DMFT_QMC_WEAK_COUPLING_H
#define DMFT_QMC_WEAK_COUPLING_H

#include <boost/multi_array.hpp>
#include <boost/timer/timer.hpp>

#include <alps/ngs.hpp>
#include <alps/mcbase.hpp>

#include <alps/alea.h>
#include <alps/numeric/matrix.hpp>

#include <cmath>
#include "green_function.h"
#include "alps_solver.h"
#include "types.h"
#include "solver.h"
#include "alps_solver.h"
#include "fouriertransform.h"
#include "U_matrix.h"
#include "operator.hpp"
#include "green_matrix.hpp"
#include <alps/numeric/matrix.hpp>
#include "legendre.h"


/*types*/
class c_or_cdagger;
class vertex;
typedef class histogram simple_hist;
typedef std::vector<vertex> vertex_array;


enum measurement_methods {
  selfenergy_measurement_matsubara, //measurement using self energy method
  selfenergy_measurement_itime_rs, //measurement using self energy method in imag time, real space
};


typedef struct fastupdate_add_helper {
  fastupdate_add_helper(std::size_t num_flavors) : num_new_rows(num_flavors,0) {};

  void clear(size_t n_vertices_add=1) {
    std::fill(num_new_rows.begin(), num_new_rows.end(), 0);
  }

  std::vector<std::size_t> num_new_rows;
} fastupdate_add_helper;

typedef struct fastupdate_remove_helper {
    fastupdate_remove_helper(std::size_t num_flavors)
            //: num_removed_rows(num_flavors,0), rows_cols_removed(num_flavors) {};
      : rows_cols_removed(num_flavors) {};

    std::vector<std::vector<size_t> > rows_cols_removed;

    void sort_rows_cols() {
      for (size_t flavor=0; flavor<rows_cols_removed.size(); ++flavor) {
        std::sort(rows_cols_removed[flavor].begin(),rows_cols_removed[flavor].end());
      }
    }
    void clear() {
      for (size_t flavor=0; flavor<rows_cols_removed.size(); ++flavor) {
        rows_cols_removed[flavor].resize(0);
      }
    }
} fastupdate_remove_helper;


class histogram
{
public:
  
  histogram(unsigned int N):hist_(N, 0){}
  
  unsigned long &operator[](unsigned int n){return hist_[n];}
  const unsigned long &operator[](unsigned int n) const{return hist_[n];}
  unsigned int size() const{return hist_.size();}
  
  unsigned int max_index() const
  { 
    unsigned int max_index=0; 
    double max=0; 
    for(unsigned int i=0;i<hist_.size();++i){
      if(max<hist_[i]){
        max=hist_[i];
        max_index=i;
      }
    }
    return max_index;
  }
  
  unsigned int top_index() const
  { 
    unsigned int top_index=0;
    for(unsigned int i=0;i<hist_.size();++i){
      if(hist_[i]!=0){
        top_index=i;
      }
    }
    return top_index;
  }
  
  double max(const unsigned int index) const
  { 
    double max=0; 
    for(unsigned int i=0;i<index;++i){
      if(max<hist_[i]){
        max=hist_[i];
      }
    }
    return max;
  }
  
  double average(const unsigned int index) const{ 
    double average=0; 
    for(unsigned int i=0;i<index;++i){
      average+=hist_[i];
    }
    return average/index;
  }
  
  bool is_flat(const unsigned int index)const{return max(index)*0.8<average(index);}
  
  void clear()
  {
    for(unsigned int i=0;i<hist_.size();++i){
      hist_[i]=0;
    }
  }

private:
  
  std::vector<unsigned long> hist_;
};



typedef class vertex
{        
public:
  vertex(const spin_t &flavor1, const site_t &site1, const unsigned int &c_dagger_1, const unsigned int &c_1, 
         const spin_t &flavor2, const site_t &site2, const unsigned int &c_dagger_2, const unsigned int &c_2, 
         const double &abs_w)
  {
    z1_=flavor1;
    z2_=flavor2;
    s1_=site1;
    s2_=site2;
    c1dagger_=c_dagger_1;
    c2dagger_=c_dagger_2;
    c1_=c_1;
    c2_=c_2;
    abs_w_=abs_w;
  }
  
  inline const double &abs_w() const {return abs_w_;}
  inline const unsigned int &flavor1() const {return z1_;}
  inline const unsigned int &flavor2() const {return z2_;}
  inline const unsigned int &site1() const {return s1_;}
  inline const unsigned int &site2() const {return s2_;}
  inline void set_site1(site_t site1) {s1_=site1;}
  inline void set_site2(site_t site2) {s2_=site2;}
  inline const unsigned int &c_dagger_1() const {return c1dagger_;}
  inline const unsigned int &c_dagger_2() const {return c2dagger_;}
  inline const unsigned int &c_1() const {return c1_;}
  inline const unsigned int &c_2() const {return c2_;}
  inline unsigned int &flavor1() {return z1_;}
  inline unsigned int &flavor2() {return z2_;}
  inline unsigned int &c_dagger_1() {return c1dagger_;}
  inline unsigned int &c_dagger_2() {return c2dagger_;}
  inline unsigned int &c_1() {return c1_;}
  inline unsigned int &c_2() {return c2_;}
private:
  unsigned int z1_, z2_;
  unsigned int s1_, s2_;
  unsigned int c1_, c2_;
  unsigned int c1dagger_, c2dagger_;
  double abs_w_;
} vertex;


class inverse_m_matrix
{
public:
  alps::numeric::matrix<double> &matrix() { return matrix_;}
  alps::numeric::matrix<double> const &matrix() const { return matrix_;}
  std::vector<creator> &creators(){ return creators_;}
  const std::vector<creator> &creators() const{ return creators_;}
  std::vector<annihilator> &annihilators(){ return annihilators_;}
  const std::vector<annihilator> &annihilators()const{ return annihilators_;}
  std::vector<double> &alpha(){ return alpha_;}
  const std::vector<double> &alpha() const{ return alpha_;}
  size_t find_row_col(double time) {
    for(std::size_t i=0; i<creators_.size(); ++i) {
      if (time==creators_[i].t()) {
        assert(annihilators_[i].t()==time);
        return i;
      }
    }
    throw std::logic_error("You operator is missing!");
  }
private:
  alps::numeric::matrix<double> matrix_;
  std::vector<creator> creators_;         //an array of creation operators c_dagger corresponding to the row of the matrix
  std::vector<annihilator> annihilators_; //an array of to annihilation operators c corresponding to the column of the matrix
  std::vector<double> alpha_;             //an array of doubles corresponding to the alphas of Rubtsov for the c, cdaggers at the same index.
};

class big_inverse_m_matrix
{
public:
    void push_back(const inverse_m_matrix& new_one) {
      sub_matrices_.push_back(new_one);
    }

    inverse_m_matrix& operator[](size_t flavor) {
      assert(flavor<sub_matrices_.size());
      return sub_matrices_[flavor];
    }

    size_t size() const {
      return sub_matrices_.size();
    };

    void sanity_check() const {
      for (spin_t flavor=0; flavor<size(); ++flavor) {
        assert(sub_matrices_[flavor].creators().size()==sub_matrices_[flavor].annihilators().size());
      }
    }
private:
    std::vector<inverse_m_matrix> sub_matrices_;
};

class InteractionExpansion: public alps::mcbase
{
public:

  InteractionExpansion(const alps::params& p, int rank);
  ~InteractionExpansion() {}
  bool is_thermalized() const {return true;} //thermalization is done in the constructor. It's not a big deal here.
  void update();
  void measure();
  double fraction_completed() const;

  //type of G(tau) This should be std::complex when G(tau) has imaginary parts. At some point, this will be templatized.
  typedef double GTYPE;
//typedef std::vector<std::vector<std::valarray<std::complex<double> > > > Wk_t;
  typedef boost::multi_array<std::complex<double>, 4> Wk_t;

protected:
  
  /*functions*/
  /*io & initialization*/
  void initialize_simulation(const alps::params &parms); // called by constructor
  // in file io.cpp
  void read_bare_green(std::ifstream &G0_omega, std::ifstream &G0_tau);
  void print(std::ostream &os);
  
  /*green's function*/
  // in file spines.cpp
  //double green0_spline(const creator &cdagger, const annihilator &c) const;
  //double green0_spline(const itime_t delta_t, const spin_t flavor, const site_t site1, const site_t site2) const;
  double green0_spline_new(const annihilator &c, const creator &cdagger) const;
  double green0_spline_new(const itime_t delta_t, const spin_t flavor, const site_t site1, const site_t site2) const;
  //double green0_spline(const itime_t delta_t, const spin_t flavor) const;
  
  /*the actual solver functions*/
  // in file solver.cpp
  void interaction_expansion_step(void);
  void reset_perturbation_series(void);
  
  // in file fastupdate.cpp:
  double fastupdate_up(const int flavor, bool compute_only_weight, size_t n_vertices_add);
  double fastupdate_down(const std::vector<size_t>& rows_cols_removed, const int flavor, bool compute_only_weight);
  
  /*measurement functions*/
  // in file measurements.cpp
  void measure_observables(std::valarray<double>& timings);
  void initialize_observables(void);
  
  void compute_W_matsubara();
//  void compute_W_itime();
  void compute_Sl();
  void measure_Wk(Wk_t& Wk, const unsigned int nfreq);
  void measure_densities();
  void sanity_check();
  
  /*abstract virtual functions. Implement these for specific models.*/
  virtual double try_add(fastupdate_add_helper&,size_t)=0;
  virtual void perform_add(fastupdate_add_helper&,size_t)=0;
  virtual void reject_add(fastupdate_add_helper&,size_t)=0;
  virtual double try_remove(unsigned int vertex_nr, fastupdate_remove_helper&)=0;
  virtual void perform_remove(unsigned int vertex_nr, fastupdate_remove_helper&)=0;
  virtual void reject_remove(fastupdate_remove_helper&)=0;
  
  /*private member variables, constant throughout the simulation*/
  const unsigned int max_order;                        
  const spin_t n_flavors;                                //number of flavors (called 'flavors') in InteractionExpansion
  const site_t n_site;                                //number of sites
  const frequency_t n_matsubara;        //number of matsubara freq
  const frequency_t n_matsubara_measurements;        //number of measured matsubara freq
  const itime_index_t n_tau;                        //number of imag time slices
  const itime_t n_tau_inv;                        //the inverse of n_tau
  const frequency_t n_self;                        //number of self energy (W) binning points
  const std::size_t n_legendre;
  const boost::uint64_t mc_steps;                        
  const unsigned long therm_steps;                
  const double max_time_in_seconds;
  
  const double beta;                                
  const double temperature;                        //only for performance reasons: avoid 1/beta computations where possible        
  const double onsite_U;                        
  const double alpha;                                
  const U_matrix U;
  const general_U_matrix<GTYPE> Uijkl; //for any general two-body interaction
  
  
  const unsigned int recalc_period;                
  const unsigned int measurement_period;        
  const unsigned int convergence_check_period;        
  
  /*InteractionExpansion's roundoff threshold*/
  const double almost_zero;                        
  /*PRNG seed*/
  const int seed;                                
  
  /*private member variables*/
  matsubara_green_function_t green_matsubara;
  matsubara_green_function_t bare_green_matsubara;
  itime_green_function_t bare_green_itime;
  itime_green_function_t green_itime;
  std::vector<green_matrix> g0;
  boost::shared_ptr<FourierTransformer> fourier_ptr;
  
  //vertex_array vertices;
  std::vector<itime_vertex> vertices_new;
  big_inverse_m_matrix M;
    //std::vector<inverse_m_matrix> M;

  double weight;
  double sign;
  unsigned int measurement_method;
  bool thermalized;
  
  simple_hist pert_hist;
  unsigned int hist_max_index;
  simple_hist **vertex_histograms;
  unsigned int vertex_histogram_size;
  
  unsigned long step;        
  time_t start_time;
  clock_t update_time;
  clock_t measurement_time;

  LegendreTransformer legendre_transformer;
};



/*aux functions*/
std::ostream& operator << (std::ostream& os, const std::vector<double>& v);
std::ostream& operator << (std::ostream &os, const vertex_array &vertices);
std::ostream& operator << (std::ostream &os, const vertex &v);
std::ostream& operator << (std::ostream &os, const c_or_cdagger &c);
std::ostream& operator << (std::ostream& os, const simple_hist &h);

/*


//Use this for the most simple single site Hubbard model.
class HalfFillingHubbardInteractionExpansion: public InteractionExpansion{
public:
  HalfFillingHubbardInteractionExpansion(const alps::params& p, int rank)
    :InteractionExpansion(p, rank)
  {
    if(n_flavors !=1){throw std::invalid_argument("you need a different model for n_flavors!=1.");}
  }
  double try_add();
  void perform_add();
  void reject_add();
  double try_remove(unsigned int vertex_nr);
  void perform_remove(unsigned int vertex_nr);
  void reject_remove();
};
*/



class HubbardInteractionExpansion: public InteractionExpansion{
public:
  HubbardInteractionExpansion(const alps::params& p, int rank)
    :InteractionExpansion(p, rank)
  {
    if(n_flavors !=2){throw std::invalid_argument("you need a different model for n_flavors!=2.");}
  }
  double try_add(fastupdate_add_helper&,size_t);
  void perform_add(fastupdate_add_helper&,size_t);
  void reject_add(fastupdate_add_helper&,size_t);
  double try_remove(unsigned int vertex_nr, fastupdate_remove_helper&);
  void perform_remove(unsigned int vertex_nr, fastupdate_remove_helper&);
  void reject_remove(fastupdate_remove_helper&);
};


/*
//Use this for multiple bands where you have terms Un_i n_j
class MultiBandDensityHubbardInteractionExpansion: public InteractionExpansion{
public:
  MultiBandDensityHubbardInteractionExpansion(const alps::params& p, int rank)
    :InteractionExpansion(p, rank)
  {
    if(n_site !=1){throw std::invalid_argument("you need a different model for n_site!=1.");}
  }
  
  double try_add();
  void perform_add();
  void reject_add();
  double try_remove(unsigned int vertex_nr);
  void perform_remove(unsigned int vertex_nr);
  void reject_remove();
};*/

#endif
