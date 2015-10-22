#include "gtest/gtest.h"

#include <boost/random.hpp>
#include <boost/multi_array.hpp>

#include "gtest.hpp"

TEST(LegendreMeasurement, Tnl)
{
    const int n_matsubara = 1;
    const int n_legendre = 1000;
    LegendreTransformer trans(n_matsubara, n_legendre);

    //Check if norm is conserved
    for (std::size_t im=0; im<n_matsubara; ++im) {
        std::complex<double> tmp;
        for (std::size_t il=0; il<n_legendre; ++il) {
            tmp += trans.Tnl()(im,il)*std::conj(trans.Tnl()(im,il));
        }
        ASSERT_TRUE(std::abs(tmp-1.)<1E-5) << "norm is not conserved = " << tmp;
    }

    std::vector<double> legendre_values(n_legendre);
    const double xval = 0.2;
    trans.compute_legendre(xval, legendre_values);
    ASSERT_TRUE(std::abs(legendre_values[1]-xval)<1E-8);
    for (std::size_t il=1; il<n_legendre-1; ++il) {
        double left_val = (il+1)*legendre_values[il+1];
        double right_val = (2*il+1)*xval*legendre_values[il]-il*legendre_values[il-1];
        ASSERT_TRUE(std::abs(left_val-right_val)<1E-8);
    }
}

//template<class T>
//class TypedFastUpdateTest : public testing::Test {};
//typedef ::testing::Types<double, std::complex<double> > TestTypes;
////typedef ::testing::Types<std::complex<double> > TestTypes;
//TYPED_TEST_CASE(TypedFastUpdateTest, TestTypes);

/*
TYPED_TEST(TypedFastUpdateTest, real_ins) {
    typedef TypeParam SCALAR;
    typedef blas::square_matrix<SCALAR> MAT;
    typedef std::vector<std::vector<std::vector<SCALAR> > > HYB;
    typedef blas::vector<SCALAR> VEC;

    const int FLAVORS=3;
    const int Np=10;
    const int Np1=Np+1;
    const double BETA=10.0;
    const int N_test = 10;
    const int N_init_pairs=4;

    boost::random::mt19937 gen(100);
    boost::uniform_smallint<> dist(0,FLAVORS-1);
    boost::uniform_real<> uni_dist(0,1);

    for (int itest=0; itest<N_test;) {
        HYB F;
        operator_container_t operators, creation_operators, annihilation_operators;

        init_config<SCALAR>(100, FLAVORS, Np1, BETA, N_init_pairs, F, operators, creation_operators, annihilation_operators);
        MAT M; 
        SCALAR det = cal_det(creation_operators, annihilation_operators, M, BETA, F);

        */
/* fast update *//*

        const int flavor_ins = dist(gen);
        const int flavor_rem = dist(gen);
        const double t_ins = BETA*uni_dist(gen);
        const double t_rem = BETA*uni_dist(gen);
        VEC sign_Fs(creation_operators.size()), Fe_M(annihilation_operators.size());
        int column=0;   // creation operator position
        for (operator_container_t::iterator it=creation_operators.begin(); it!=creation_operators.end(); it++) {
            if (it->time()<t_ins) {
                column++;
            } else {
                break;
            }
        }
        int row=0;		// annihilation operator position
        for (operator_container_t::iterator it=annihilation_operators.begin(); it!=annihilation_operators.end(); it++) {
            if (it->time()<t_rem) {
                row++;
            } else {
                break;
            }
        }
        const SCALAR det_rat = det_rat_row_column_up(flavor_ins, flavor_rem, t_ins, t_rem, row, column, M, creation_operators, annihilation_operators, F, sign_Fs, Fe_M, BETA);
        creation_operators.insert(psi(t_ins,0,flavor_ins));
        annihilation_operators.insert(psi(t_rem,1,flavor_rem));
        compute_M_row_column_up(row, column, M, sign_Fs, Fe_M, det_rat);
        det *= det_rat;

        */
/* actual update *//*

        MAT M_ref;
        SCALAR det_ref = cal_det(creation_operators, annihilation_operators, M_ref, BETA, F);

        if (std::abs(det)<1e-12 && std::abs(det_ref)<1e-12) {
            continue;
        }
        //std::cout << " debug " << det << " " << det_ref << std::endl;

        ASSERT_TRUE(std::abs((det_ref-det)/det)<1e-8) << "Updating det does not work correctly " << " itest = " << itest << " det= " << det << " det_ref= " << det_ref << std::endl;
        ASSERT_EQ(M.size1(), M_ref.size1()) << "Size1 of M are different" << " itest = " << itest << std::endl;
        ASSERT_EQ(M.size2(), M_ref.size2()) << "Size2 of M are different" << " itest = " << itest << std::endl;
        for (int i=0; i<M.size1(); ++i) {
            for (int j=0; j<M.size2(); ++j) {
                ASSERT_TRUE(std::abs(M_ref(i,j)-M(i,j))<1e-8) << "M(" << i << "," << j << ")= " << M(i,j) << " " << M_ref(i,j) << std::endl;
            }
        }
        ++itest;
    }
}

TYPED_TEST(TypedFastUpdateTest, real_rem) {
    typedef TypeParam SCALAR;
    //typedef blas::general_matrix<SCALAR> MAT;
    typedef blas::square_matrix<SCALAR> MAT;
    typedef std::vector<std::vector<std::vector<SCALAR> > > HYB;

    const int FLAVORS=3;
    const int Np=10;
    const int Np1=Np+1;
    const double BETA=10.0;
    const int N_test = 10;
    const int N_init_pairs=4;

    boost::random::mt19937 gen(100);
    boost::uniform_smallint<> dist(0,FLAVORS-1);
    boost::uniform_real<> uni_dist(0,1);

    for (int itest=0; itest<N_test;) {
        HYB F;
        operator_container_t operators, creation_operators, annihilation_operators;

        init_config<SCALAR>(100, FLAVORS, Np1, BETA, N_init_pairs, F, operators, creation_operators, annihilation_operators);
        MAT M; 
        SCALAR det = cal_det(creation_operators, annihilation_operators, M, BETA, F);

        */
/* fast update *//*

        operator_container_t::iterator it_c = creation_operators.begin();
        operator_container_t::iterator it_a = annihilation_operators.begin();
        const int position_c = 0;
        const int position_a = 0;
        det *= det_rat_row_column_down<MAT>(position_c, position_a, M);
        compute_M_row_column_down(position_c, position_a, M);

        */
/* actual update *//*

        operators_remove_nocopy(operators, it_c->time(), it_a->time(), it_c->flavor(), it_a->flavor());
        creation_operators.erase(it_c);
        annihilation_operators.erase(it_a);
        MAT M_ref;
        SCALAR det_ref = cal_det(creation_operators, annihilation_operators, M_ref, BETA, F);

        if (std::abs(det)<1e-12 && std::abs(det_ref)<1e-12) {
            continue;
        }
        //std::cout << " debug " << det << " " << det_ref << std::endl;

        ASSERT_TRUE(std::abs((det_ref-det)/det)<1e-8) << "Updating det does not work correctly " << " det= " << det << " det_ref= " << det_ref << std::endl;
        ASSERT_EQ(M.size1(), M_ref.size1()) << "Size1 of M are different" << std::endl;
        ASSERT_EQ(M.size2(), M_ref.size2()) << "Size2 of M are different" << std::endl;
        for (int i=0; i<M.size1(); ++i) {
            for (int j=0; j<M.size2(); ++j) {
                ASSERT_TRUE(std::abs(M_ref(i,j)-M(i,j))<1e-8) << "M(" << i << "," << j << ")= " << M(i,j) << " " << M_ref(i,j) << std::endl;
            }
        }
        ++itest;
    }
}


TYPED_TEST(TypedFastUpdateTest, shift) {
    namespace bll = boost::lambda;
    typedef TypeParam SCALAR;
    typedef operator_container_t::iterator it_t;
    //typedef blas::general_matrix<TypeParam> MAT;
    typedef blas::square_matrix<TypeParam> MAT;
    typedef std::vector<std::vector<std::vector<TypeParam> > > HYB;

    const int FLAVORS=3;
    const int Np=10;
    const int Np1=Np+1;
    const double BETA=10.0;
    const int N_test = 10;
    const int N_init_pairs=4;
    const double tau_low=0, tau_high=BETA;

    boost::random::mt19937 gen(100);
    boost::uniform_smallint<> dist(0,FLAVORS-1);
    boost::uniform_real<> uni_dist(0,1);

    for (int itest=0; itest<N_test;) {
        for (int type=0; type<2; ++type) {//type==0: shift creation op, type==1: shift annihilation op
            HYB F;
            operator_container_t operators, creation_operators, annihilation_operators;

            init_config<SCALAR>(100, FLAVORS, Np1, BETA, N_init_pairs, F, operators, creation_operators, annihilation_operators);

            MAT M; 
            TypeParam det = cal_det(creation_operators, annihilation_operators, M, BETA, F);
            TypeParam det_old = det;
    
            */
/* fast update *//*

            operator_container_t operators_new(operators);
            operator_container_t::iterator it;
            int position;
            if (type == 0) {
                std::pair<it_t,it_t> range = creation_operators.range(tau_low<=bll::_1, bll::_1<=tau_high);
                int num_ops = std::distance(range.first,range.second);
                position = std::distance(creation_operators.begin(),range.first)+(int) (uni_dist(gen)*num_ops);
                it = creation_operators.begin();
                advance(it, position);
            } else {
                std::pair<it_t,it_t> range = annihilation_operators.range(tau_low<=bll::_1, bll::_1<=tau_high);
                int num_ops = std::distance(range.first,range.second);
                position = std::distance(annihilation_operators.begin(),range.first)+(int) (uni_dist(gen)*num_ops);
                it = annihilation_operators.begin();
                advance(it, position);
            }

            const int flavor = it->flavor();
            double old_t = it->time();
            double new_t = uni_dist(gen)*BETA;
            double op_distance = std::abs(old_t-new_t);

            double time_min, time_max;
            if (old_t < new_t) {
                time_min = old_t;
                time_max = new_t;
            } else {
                time_min = new_t;
                time_max = old_t;
            }

            operator_container_t::iterator it_op = operators_new.begin();
            operator_container_t::iterator it_op_min;

            psi new_operator(new_t, type, flavor);
    
            while (it_op->time() < time_min) {
                it_op++;
            }

            it_op_min = it_op;            // point to first operator >= time_min
    
            if (it_op->time() == old_t) {
                it_op++;
            }
    
            while (it_op != operators_new.end() && it_op->time() < time_max) {
                it_op++;
            }

            if (it_op != operators_new.end() && it_op->time() == time_max) {
                operators_new.erase(it_op);
                operators_new.insert(new_operator);
            } else {
                operators_new.insert(new_operator);
                operators_new.erase(it_op_min);
            }

            TypeParam det_rat = (type == 0 ? det_rat_shift_start<TypeParam,MAT,HYB>(new_t, position, flavor, M, annihilation_operators, F, BETA) :
                      det_rat_shift_end<TypeParam,MAT,HYB>(new_t, position, flavor, M, creation_operators, F, BETA));

            int num_row_or_column_swaps;
            if (type == 0) {
                psi op = *it;
                creation_operators.erase(it);
                op.set_time(new_t);
                creation_operators.insert(op);

                int new_position = 0;
                for (operator_container_t::iterator it_tmp = creation_operators.begin(); it_tmp != creation_operators.end(); ++it_tmp) {
                    if (it_tmp->time()<new_t) {
                        ++new_position;
                    }
                }
                num_row_or_column_swaps = compute_M_shift_start(new_t, position, new_position, flavor, M, annihilation_operators, F, BETA, det_rat);
            } else {
                psi op = *it;
                annihilation_operators.erase(it);
                op.set_time(new_t);
                annihilation_operators.insert(op);

                int new_position = 0;
                for (operator_container_t::iterator it_tmp = annihilation_operators.begin(); it_tmp != annihilation_operators.end(); ++it_tmp) {
                    if (it_tmp->time()<new_t) {
                        ++new_position;
                    }
                }
                num_row_or_column_swaps = compute_M_shift_end(new_t, position, new_position, flavor, M, creation_operators, F, BETA, det_rat);
            }
            swap(operators, operators_new);
            if (num_row_or_column_swaps % 2 == 1) {
                det *= -det_rat;
            } else {
                det *= det_rat;
            }

            */
/* actual update *//*

            MAT M_ref;
            TypeParam det_ref = cal_det(creation_operators, annihilation_operators, M_ref, BETA, F);

            if (std::abs(det)<1e-12 && std::abs(det_ref)<1e-12) {
                continue;
            }

            EXPECT_TRUE(std::abs((det_ref-det)/det)<1e-8)
                << " type " << type
                << " Updating det does not work correctly "
                << " det_old= " << det_old
                << " det (fast update)= " << det
                << " det_rat (fast update)= " << det_rat
                << " det (M, fast update) = " << det_ref
                << " row_or_column_swapped = " << num_row_or_column_swaps
                << std::endl;
            EXPECT_EQ(M.size1(), M_ref.size1())
                << " type " << type
                << " Size1 of M are different"
                << std::endl;
            EXPECT_EQ(M.size2(), M_ref.size2())
                << " type " << type
                << " Size2 of M are different"
                << std::endl;
            for (int i=0; i<M.size1(); ++i) {
                for (int j=0; j<M.size2(); ++j) {
                    EXPECT_TRUE(std::abs(M_ref(i,j)-M(i,j))<1e-8) << " type " << type << "M(" << i << "," << j << ")= " << M(i,j) << " " << M_ref(i,j) << std::endl;
                }
            }
            ++itest;
        }
    }
}
*/
