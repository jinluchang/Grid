/*
 *	Policy classes for the HMC
 *	Author: Guido Cossu
*/

#ifndef ADJOINT_H
#define ADJOINT_H

namespace Grid {
namespace QCD {

/*
* This is an helper class for the HMC
* Should contain only the data for the adjoint representation
* and the facility to convert from the fundamental -> adjoint
*/

template <int ncolour>
class AdjointRep {
 public:
  // typdef to be used by the Representations class in HMC to get the
  // types for the higher representation fields
  typedef typename SU_Adjoint<ncolour>::LatticeAdjMatrix LatticeMatrix;
  typedef typename SU_Adjoint<ncolour>::LatticeAdjField LatticeField;
  static const int Dimension = ncolour * ncolour - 1;

  LatticeField U;

  explicit AdjointRep(GridBase *grid) : U(grid) {}
  void update_representation(const LatticeGaugeField &Uin) {
    // Uin is in the fundamental representation
    // get the U in AdjointRep
    // (U_adj)_B = tr[e^a U e^b U^dag]
    // e^a = t^a/sqrt(T_F)
    // where t^a is the generator in the fundamental
    // T_F is 1/2 for the fundamental representation
    conformable(U, Uin);
    U = zero;
    LatticeColourMatrix tmp(Uin._grid);

    Vector<typename SU<ncolour>::Matrix> ta(Dimension);

    // FIXME probably not very efficient to get all the generators
    // everytime
    for (int a = 0; a < Dimension; a++) SU<ncolour>::generator(a, ta[a]);

    for (int mu = 0; mu < Nd; mu++) {
      auto Uin_mu = peekLorentz(Uin, mu);
      auto U_mu = peekLorentz(U, mu);
      for (int a = 0; a < Dimension; a++) {
        tmp = 2.0 * adj(Uin_mu) * ta[a] * Uin_mu;
        for (int b = 0; b < (ncolour * ncolour - 1); b++)
          pokeColour(U_mu, trace(tmp * ta[b]), a, b);
      }
      pokeLorentz(U, U_mu, mu);
    }
  }

  LatticeGaugeField RtoFundamentalProject(const LatticeField &in,
                                          Real scale = 1.0) const {
    LatticeGaugeField out(in._grid);

    for (int mu = 0; mu < Nd; mu++) {
      LatticeColourMatrix out_mu(in._grid);  // fundamental representation
      LatticeMatrix in_mu = peekLorentz(in, mu);

      out_mu = zero;

      typename SU<ncolour>::LatticeAlgebraVector h(in._grid);
      projectOnAlgebra(h, in_mu, scale);
      FundamentalLieAlgebraMatrix(h, out_mu, 1.0);  // apply scale only once
      pokeLorentz(out, out_mu, mu);
    }
    return out;
  }

 private:
  void projectOnAlgebra(typename SU<ncolour>::LatticeAlgebraVector &h_out,
                        const LatticeMatrix &in, Real scale = 1.0) const {
    SU_Adjoint<ncolour>::projectOnAlgebra(h_out, in, scale);
  }

  void FundamentalLieAlgebraMatrix(
      typename SU<ncolour>::LatticeAlgebraVector &h,
      typename SU<ncolour>::LatticeMatrix &out, Real scale = 1.0) const {
    SU<ncolour>::FundamentalLieAlgebraMatrix(h, out, scale);
  }
};

typedef AdjointRep<Nc> AdjointRepresentation;
}
}

#endif