//
// Distributed under the ITensor Library License, Version 1.2
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_MPS_IMPL_H_
#define __ITENSOR_MPS_IMPL_H_

namespace itensor {


inline SiteSet const& MPS::
sites() const 
    { 
    if(not sites_) Error("MPS SiteSet is default-initialized");
    return sites_; 
    }

template <class BigMatrixT>
Spectrum MPS::
svdBond(int b, ITensor const& AA, Direction dir, 
        BigMatrixT const& PH, Args const& args)
    {
    setBond(b);
    if(dir == Fromleft && b-1 > leftLim())
        {
        printfln("b=%d, l_orth_lim_=%d",b,leftLim());
        Error("b-1 > l_orth_lim_");
        }
    if(dir == Fromright && b+2 < rightLim())
        {
        printfln("b=%d, r_orth_lim_=%d",b,rightLim());
        Error("b+2 < r_orth_lim_");
        }

    auto noise = args.getReal("Noise",0.);
    auto cutoff = args.getReal("Cutoff",MIN_CUT);
    auto usesvd = args.getBool("UseSVD",false);

    Spectrum res;

    if(usesvd || (noise == 0 && cutoff < 1E-12))
        {
        //Need high accuracy, use svd which calls the
        //accurate SVD method in the MatrixRef library
        ITensor D;
        res = svd(AA,A_[b],D,A_[b+1],args);

        //Normalize the ortho center if requested
        if(args.getBool("DoNormalize",false))
            {
            D *= 1./itensor::norm(D);
            }

        //Push the singular values into the appropriate site tensor
        if(dir == Fromleft) A_[b+1] *= D;
        else                A_[b]   *= D;
        }
    else
        {
        //If we don't need extreme accuracy
        //or need to use noise term
        //use density matrix approach
        res = denmatDecomp(AA,A_[b],A_[b+1],dir,PH,args);

        //Normalize the ortho center if requested
        if(args.getBool("DoNormalize",false))
            {
            ITensor& oc = (dir == Fromleft ? A_[b+1] : A_[b]);
            auto nrm = itensor::norm(oc);
            if(nrm > 1E-16) oc *= 1./nrm;
            }
        }

    if(dir == Fromleft)
        {
        l_orth_lim_ = b;
        if(r_orth_lim_ < b+2) r_orth_lim_ = b+2;
        }
    else //dir == Fromright
        {
        if(l_orth_lim_ > b-1) l_orth_lim_ = b-1;
        r_orth_lim_ = b+1;
        }

    return res;
    }

bool inline
isComplex(MPS const& psi)
    {
    for(auto j : range1(psi.N()))
        {
        if(itensor::isComplex(psi.A(j))) return true;
        }
    return false;
    }

bool inline
isOrtho(MPS const& psi)
    {
    return psi.leftLim()+1 == psi.rightLim()-1;
    }

int inline
orthoCenter(MPS const& psi)
    {
    if(!isOrtho(psi)) Error("orthogonality center not well defined.");
    return (psi.leftLim() + 1);
    }

Real inline
norm(MPS const& psi)
    {
    if(not isOrtho(psi)) Error("\
MPS must have well-defined ortho center to compute norm; \
call .position(j) or .orthogonalize() to set ortho center");
    return itensor::norm(psi.A(orthoCenter(psi)));
    }

Real inline
normalize(MPS & psi)
    {
    auto nrm = norm(psi);
    if(std::fabs(nrm) < 1E-20) Error("Zero norm");
    psi /= nrm;
    return nrm;
    }

Index inline
linkInd(MPS const& psi, int b)
    { 
    return commonIndex(psi.A(b),psi.A(b+1),Link); 
    }

Index inline
rightLinkInd(MPS const& psi, int i)
    { 
    return commonIndex(psi.A(i),psi.A(i+1),Link); 
    }

Index inline
leftLinkInd(MPS const& psi, int i)
    { 
    return commonIndex(psi.A(i),psi.A(i-1),Link); 
    }

Real inline
averageM(MPS const& psi)
    {
    Real avgm = 0;
    for(int b = 1; b < psi.N(); ++b) 
        {
        avgm += linkInd(psi,b).m();
        }
    avgm /= (psi.N()-1);
    return avgm;
    }

int inline
maxM(MPS const& psi)
    {
    int maxM_ = 0;
    for(int b = 1; b < psi.N(); ++b) 
        {
        int mb = commonIndex(psi.A(b),psi.A(b+1)).m();
        maxM_ = std::max(mb,maxM_);
        }
    return maxM_;
    }

void inline
applyGate(ITensor const& gate, 
          MPS & psi,
          Args const& args)
    {
    auto fromleft = args.getBool("Fromleft",true);
    const int c = psi.orthoCenter();
    ITensor AA = psi.A(c) * psi.A(c+1) * gate;
    AA.noprime();
    if(fromleft) psi.svdBond(c,AA,Fromleft,args);
    else         psi.svdBond(c,AA,Fromright,args);
    }

bool 
checkOrtho(MPS const& psi,
           int i, 
           bool left)
    {
    Index link = (left ? rightLinkInd(psi,i) : leftLinkInd(psi,i));
    ITensor rho = psi.A(i) * dag(prime(psi.A(i),link,4));
    ITensor Delta = delta(link, prime(link, 4));
    ITensor Diff = rho - Delta;

    const
    Real threshold = 1E-13;
    if(norm(Diff) < threshold) 
        {
        return true;
        }

    //Print any helpful debugging info here:
    println("checkOrtho: on line ",__LINE__," of mps.h,");
    println("checkOrtho: Tensor at position ",i," failed to be ",left?"left":"right"," ortho.");
    printfln("checkOrtho: norm(Diff) = %E",norm(Diff));
    printfln("checkOrtho: Error threshold set to %E",threshold);
    //-----------------------------

    return false;
    }

bool inline
checkOrtho(MPS const& psi)
    {
    for(int i = 1; i <= psi.leftLim(); ++i)
    if(!checkOrtho(psi,i,true))
        {
        std::cout << "checkOrtho: A_[i] not left orthogonal at site i=" 
                  << i << std::endl;
        return false;
        }

    for(int i = psi.N(); i >= psi.rightLim(); --i)
    if(!checkOrtho(psi,i,false))
        {
        std::cout << "checkOrtho: A_[i] not right orthogonal at site i=" 
                  << i << std::endl;
        return false;
        }
    return true;
    }

Cplx inline
overlapC(MPS const& psi, 
         MPS const& phi)
    {
    auto N = psi.N();
    if(N != phi.N()) Error("overlap: mismatched N");

    auto l1 = linkInd(psi,1);
    auto L = phi.A(1);
    if(l1) L *= dag(prime(psi.A(1),l1)); 
    else   L *= dag(psi.A(1));

    if(N == 1) return L.cplx();

    for(decltype(N) i = 2; i < N; ++i) 
        { 
        L = L * phi.A(i) * dag(prime(psi.A(i),Link)); 
        }
    L = L * phi.A(N);

    auto lNm = linkInd(psi,N-1);
    if(lNm) return (dag(prime(psi.A(N),lNm))*L).cplx();
    return (dag(psi.A(N))*L).cplx();
    }

void inline
overlap(MPS const& psi, MPS const& phi, Real& re, Real& im)
    {
    auto z = overlapC(psi,phi);
    re = z.real();
    im = z.imag();
    }

Real inline
overlap(MPS const& psi, MPS const& phi) //Re[<psi|phi>]
    {
    Real re, im;
    overlap(psi,phi,re,im);
    if(std::fabs(im) > (1E-12 * std::fabs(re)) )
        printfln("Real overlap: WARNING, dropping non-zero imaginary part (=%.5E) of expectation value.",im);
    return re;
    }

Complex inline
psiphiC(MPS const& psi, 
        MPS const& phi)
    {
    return overlapC(psi,phi);
    }

void 
psiphi(MPS const& psi, MPS const& phi, Real& re, Real& im)
    {
    overlap(psi,phi,re,im);
    }

Real inline
psiphi(MPS const& psi, MPS const& phi) //Re[<psi|phi>]
    {
    return overlap(psi,phi);
    }

MPS inline
sum(MPS const& L, 
    MPS const& R, 
    Args const& args)
    {
    auto res = L;
    res.plusEq(R,args);
    return res;
    }

MPS
sum(std::vector<MPS> const& terms, 
    Args const& args)
    {
    auto Nt = terms.size();
    if(Nt == 2)
        { 
        return sum(terms.at(0),terms.at(1),args);
        }
    else 
    if(Nt == 1) 
        {
        return terms.at(0);
        }
    else 
    if(Nt > 2)
        {
        //Add all MPS in pairs
        auto nsize = (Nt%2==0 ? Nt/2 : (Nt-1)/2+1);
        std::vector<MPS> newterms(nsize); 
        for(decltype(Nt) n = 0, np = 0; n < Nt-1; n += 2, ++np)
            {
            newterms.at(np) = sum(terms.at(n),terms.at(n+1),args);
            }
        if(Nt%2 == 1) newterms.at(nsize-1) = terms.back();

        //Recursively call sum again
        return sum(newterms,args);
        }
    return MPS();
    }

} //namespace itensor

#endif
