/*************************************************************************************

    grid` physics library, www.github.com/paboyle/Grid 

    Copyright (C) 2015

Author: Azusa Yamaguchi <ayamaguc@staffmail.ed.ac.uk>
Author: Peter Boyle <paboyle@ph.ed.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    See the full license in the file "LICENSE" in the top level distribution directory
*************************************************************************************/
/*  END LEGAL */
//#include <Grid/Grid.h>

#ifndef GRID_QCD_GAUGE_FIX_H
#define GRID_QCD_GAUGE_FIX_H

NAMESPACE_BEGIN(Grid);


template <class Gimpl> 
class FourierAcceleratedGaugeFixer  : public Gimpl {
public:
  INHERIT_GIMPL_TYPES(Gimpl);

  typedef typename Gimpl::GaugeLinkField GaugeMat;
  typedef typename Gimpl::GaugeField GaugeLorentz;

  static void GaugeLinkToLieAlgebraField(const std::vector<GaugeMat> &U,std::vector<GaugeMat> &A) {
    for(int mu=0;mu<Nd;mu++){
      Complex cmi(0.0,-1.0);
      A[mu] = Ta(U[mu]) * cmi;
    }
  }
  static void DmuAmu(const std::vector<GaugeMat> &A,GaugeMat &dmuAmu,int orthog) {
    dmuAmu=Zero();
    for(int mu=0;mu<Nd;mu++){
      if ( mu != orthog ) {
	dmuAmu = dmuAmu + A[mu] - Cshift(A[mu],mu,-1);
      }
    }
  }  

  static void SteepestDescentGaugeFix(GaugeLorentz &Umu,Real & alpha,int maxiter,Real Omega_tol, Real Phi_tol,bool Fourier=false,int orthog=-1,bool err_on_no_converge=true) {
    GridBase *grid = Umu.Grid();
    GaugeMat xform(grid);
    SteepestDescentGaugeFix(Umu,xform,alpha,maxiter,Omega_tol,Phi_tol,Fourier,orthog,err_on_no_converge);
  }
  static void SteepestDescentGaugeFix(GaugeLorentz &Umu,GaugeMat &xform,Real & alpha,int maxiter,Real Omega_tol, Real Phi_tol,bool Fourier=false,int orthog=-1,bool err_on_no_converge=true) {

    GridBase *grid = Umu.Grid();

    Real org_plaq      =WilsonLoops<Gimpl>::avgPlaquette(Umu);
    Real org_link_trace=WilsonLoops<Gimpl>::linkTrace(Umu); 
    Real old_trace = org_link_trace;
    Real trG;
    
    xform=1.0;

    std::vector<GaugeMat> U(Nd,grid);
    GaugeMat dmuAmu(grid);

    {
      Real plaq      =WilsonLoops<Gimpl>::avgPlaquette(Umu);
      Real link_trace=WilsonLoops<Gimpl>::linkTrace(Umu); 
      if( (orthog>=0) && (orthog<Nd) ){
	std::cout << GridLogMessage << " Gauge fixing to Coulomb gauge time="<<orthog<< " plaq= "<<plaq<<" link trace = "<<link_trace<<  std::endl;
      } else { 
	std::cout << GridLogMessage << " Gauge fixing to Landau gauge plaq= "<<plaq<<" link trace = "<<link_trace<<  std::endl;
      }
    }
    for(int i=0;i<maxiter;i++){

      for(int mu=0;mu<Nd;mu++) U[mu]= PeekIndex<LorentzIndex>(Umu,mu);

      if ( Fourier==false ) { 
	trG = SteepestDescentStep(U,xform,alpha,dmuAmu,orthog);
      } else { 
	trG = FourierAccelSteepestDescentStep(U,xform,alpha,dmuAmu,orthog);
      }

      //      std::cout << GridLogMessage << "trG   "<< trG<< std::endl;
      //      std::cout << GridLogMessage << "xform "<< norm2(xform)<< std::endl;
      //      std::cout << GridLogMessage << "dmuAmu "<< norm2(dmuAmu)<< std::endl;

      for(int mu=0;mu<Nd;mu++) PokeIndex<LorentzIndex>(Umu,U[mu],mu);
      // Monitor progress and convergence test 
      // infrequently to minimise cost overhead
      if ( i %20 == 0 ) { 
	Real plaq      =WilsonLoops<Gimpl>::avgPlaquette(Umu);
	Real link_trace=WilsonLoops<Gimpl>::linkTrace(Umu); 

	if (Fourier) 
	  std::cout << GridLogMessage << "Fourier Iteration "<<i<< " plaq= "<<plaq<< " dmuAmu " << norm2(dmuAmu)<< std::endl;
	else 
	  std::cout << GridLogMessage << " Iteration "<<i<< " plaq= "<<plaq<< " dmuAmu " << norm2(dmuAmu)<< std::endl;
	
	Real Phi  = 1.0 - old_trace / link_trace ;
	Real Omega= 1.0 - trG;

	std::cout << GridLogMessage << " Iteration "<<i<< " Phi= "<<Phi<< " Omega= " << Omega<< " trG " << trG <<std::endl;
	if ( (Omega < Omega_tol) && ( ::fabs(Phi) < Phi_tol) ) {
	  std::cout << GridLogMessage << "Converged ! "<<std::endl;
	  return;
	}

	old_trace = link_trace;

      }
    }
    std::cout << GridLogError << "Gauge fixing did not converge in " << maxiter << " iterations." << std::endl;
    if (err_on_no_converge) assert(0);
  };
  static Real SteepestDescentStep(std::vector<GaugeMat> &U,GaugeMat &xform,Real & alpha, GaugeMat & dmuAmu,int orthog) {
    GridBase *grid = U[0].Grid();

    std::vector<GaugeMat> A(Nd,grid);
    GaugeMat g(grid);

    GaugeLinkToLieAlgebraField(U,A);
    ExpiAlphaDmuAmu(A,g,alpha,dmuAmu,orthog);


    Real vol = grid->gSites();
    Real trG = TensorRemove(sum(trace(g))).real()/vol/Nc;

    xform = g*xform ;
    SU<Nc>::GaugeTransform(U,g);

    return trG;
  }

  static Real FourierAccelSteepestDescentStep(std::vector<GaugeMat> &U,GaugeMat &xform,Real & alpha, GaugeMat & dmuAmu,int orthog) {

    GridBase *grid = U[0].Grid();

    Real vol = grid->gSites();

    FFT theFFT((GridCartesian *)grid);

    LatticeComplex  Fp(grid);
    LatticeComplex  psq(grid); psq=Zero();
    LatticeComplex  pmu(grid); 
    LatticeComplex   one(grid); one = Complex(1.0,0.0);

    GaugeMat g(grid);
    GaugeMat dmuAmu_p(grid);
    std::vector<GaugeMat> A(Nd,grid);

    GaugeLinkToLieAlgebraField(U,A);

    DmuAmu(A,dmuAmu,orthog);

    std::vector<int> mask(Nd,1);
    for(int mu=0;mu<Nd;mu++) if (mu==orthog) mask[mu]=0;
    theFFT.FFT_dim_mask(dmuAmu_p,dmuAmu,mask,FFT::forward);

    //////////////////////////////////
    // Work out Fp = psq_max/ psq...
    // Avoid singularities in Fp
    //////////////////////////////////
    Coordinate latt_size = grid->GlobalDimensions();
    Coordinate coor(grid->_ndimension,0);
    for(int mu=0;mu<Nd;mu++) {
      if ( mu != orthog ) { 
	Real TwoPiL =  M_PI * 2.0/ latt_size[mu];
	LatticeCoordinate(pmu,mu);
	pmu = TwoPiL * pmu ;
	psq = psq + 4.0*sin(pmu*0.5)*sin(pmu*0.5); 
      }
    }

    Complex psqMax(16.0);
    Fp =  psqMax*one/psq;

    pokeSite(TComplex(16.0),Fp,coor);
    if( (orthog>=0) && (orthog<Nd) ){
      for(int t=0;t<grid->GlobalDimensions()[orthog];t++){
	coor[orthog]=t;
	pokeSite(TComplex(16.0),Fp,coor);
      }
    }
    
    dmuAmu_p  = dmuAmu_p * Fp; 

    theFFT.FFT_dim_mask(dmuAmu,dmuAmu_p,mask,FFT::backward);

    GaugeMat ciadmam(grid);
    Complex cialpha(0.0,-alpha);
    ciadmam = dmuAmu*cialpha;
    SU<Nc>::taExp(ciadmam,g);

    Real trG = TensorRemove(sum(trace(g))).real()/vol/Nc;

    xform = g*xform ;
    SU<Nc>::GaugeTransform(U,g);

    return trG;
  }

  static void ExpiAlphaDmuAmu(const std::vector<GaugeMat> &A,GaugeMat &g,Real & alpha, GaugeMat &dmuAmu,int orthog) {
    GridBase *grid = g.Grid();
    Complex cialpha(0.0,-alpha);
    GaugeMat ciadmam(grid);
    DmuAmu(A,dmuAmu,orthog);
    ciadmam = dmuAmu*cialpha;
    SU<Nc>::taExp(ciadmam,g);
  }  
};

NAMESPACE_END(Grid);

#endif
