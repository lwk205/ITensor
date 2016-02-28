//
// Distributed under the ITensor Library License, Version 1.2
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_QN_H
#define __ITENSOR_QN_H

#include "itensor/global.h"
#include "itensor/arrow.h"
#include "itensor/util/readwrite.h"

namespace itensor {

//
// QN
//
// Quantum number label for IQIndexes.
//

class QN;

//
// QN Convenience Constructor Functions
//

//Sz in units of spin 1/2
//(1 => 1/2, 2 => 1, -1 => -1/2, etc.)
QN spin(int Sz);

//spinless hard-core boson
QN boson(int Nb);

//hard-core boson with spin
//Sz in units of spin 1/2
QN spinboson(int Sz, int Nb);

//spinless fermion
QN fermion(int Nf);

//spinless fermion, only parity conserved,
//particle number not conserved
QN fparity(int Pf);

//fermion with spin
//Sz in units of spin 1/2
QN electron(int Sz, int Nf);

//QN conserving electron spin and parity, not total charge
//Sz in units of spin 1/2
QN elparity(int Sz, int Pf);

//"clock" degree of freedom
//for example Z3 clock QNs are clock(0,3); clock(1,3); clock(2,3);
QN clock(int n, int N);

size_t inline constexpr
QNSize() { return 4ul; }

//QNVal: storage element type for QN
//Represents a number with a Z (integer)
//or Z_M (integer mod M) addition rule
//
// Meaning of mod field:
// mod == 1  => Z addition
// mod >  1  => Z_M addition
// mod <  0  => same as above, fermionic
// mod == 0  => inactive/not used
struct QNVal
    {
    using qn_t = int;
    private:
    qn_t val_ = 0,
         mod_ = 0;
    public:

    QNVal() { }

    QNVal(qn_t v) : val_(v), mod_(1) { }

    QNVal(qn_t v, qn_t m) : mod_(m) { set(v); }

    QNVal(std::initializer_list<qn_t> qv)
        {
        if(qv.size() != 2) Error("initializer_list arg to QNVal must have two elements");
        mod_ = *(qv.begin()+1);
        set(*(qv.begin()));
        }

    qn_t
    mod() const { return mod_; }

    qn_t
    val() const { return val_; }

    void
    set(qn_t v);

    QNVal&
    operator-() { set(-val_); return *this; }
    };

//
// QN - quantum number label for IQIndex sectors
//

class QN
    {
    public:
    using qn_t = QNVal::qn_t;
    using storage_type = std::array<QNVal,QNSize()>;
    using iterator = storage_type::iterator;
    using const_iterator = storage_type::const_iterator;
    private:
    storage_type qn_{};
    public:

    QN() { }

    // Takes QNVal arguments,
    // specifying both a qn value in each
    // sector and a mod factor
    // Since integer (qn_t) 
    // or two-element initializer_list
    // convertible to QNVal, can use
    // as QN(3,4,5) or as QN({0,2},{1,2})
    explicit
    QN(QNVal v0,
       QNVal v1 = QNVal{},
       QNVal v2 = QNVal{},
       QNVal v3 = QNVal{}) 
     : qn_{{v0,v1,v2,v3}} 
       { }

    explicit
    QN(std::initializer_list<qn_t> qv)
        {
        qn_[0] = qv;
        }

    explicit operator bool() const { return qn_.front().mod() != 0; }

    qn_t
    operator[](size_t n) const
        { 
#ifdef DEBUG
        return qn_.at(n).val(); 
#else
        return qn_[n].val(); 
#endif
        }

    //1-indexed
    qn_t
    operator()(size_t n) const { return operator[](n-1); }

    //1-indexed
    qn_t
    mod(size_t n) const { return qn_.at(n-1).mod(); }

    size_t
    size() const { return qn_.size(); }

    //0-indexed
    QNVal &
    val0(size_t n)
        { 
#ifdef DEBUG
        return qn_.at(n); 
#else
        return qn_[n]; 
#endif
        }

    //0-indexed
    QNVal const&
    val0(size_t n) const 
        { 
#ifdef DEBUG
        return qn_.at(n); 
#else
        return qn_[n]; 
#endif
        }

    void
    modAssign(QN const& qo);

    storage_type &
    store() { return qn_; }

    storage_type const&
    store() const { return qn_; }
    };


//
// QNVal functions
// 

bool inline
isFermionic(QNVal const& qv) { return qv.mod() < 0; }

bool inline
isActive(QNVal const& qv) { return qv.mod() != 0; }

void
operator+=(QNVal& qva, QNVal const& qvb);

void
operator-=(QNVal& qva, QNVal const& qvb);

void
operator*=(QNVal& qva, Arrow dir);

bool
operator==(QNVal const& qva, QNVal const& qvb);

bool
operator!=(QNVal const& qva, QNVal const& qvb);

void
read(std::istream & s, QNVal & q);

void
write(std::ostream & s, QNVal const& q);


//
// QN functions
// 

//1-indexed
bool inline
isActive(QN const& q, size_t n) { return isActive(q.val0(n-1)); }

bool inline
isFermionic(QN const& q, size_t n) { return q.mod(n) < 0; }

bool
operator==(QN const& qa, QN const& qb);

bool inline
operator!=(QN const& qa, QN const& qb) { return !operator==(qa,qb); }

bool
operator<(QN const& qa, QN const& qb);

QN
operator-(QN q);

void
operator+=(QN & qa, QN const& qb);

void
operator-=(QN & qa, QN const& qb);

void
operator*=(QN & qa, Arrow dir);

QN inline
operator+(QN qa, QN const& qb) { qa += qb; return qa; }

QN inline
operator-(QN qa, QN const& qb) { qa -= qb; return qa; }

QN inline
operator*(QN q, Arrow dir) { q *= dir; return q; }

QN inline
operator*(Arrow dir, QN q) { q *= dir; return q; }

std::ostream& 
operator<<(std::ostream & s, QN const& q);

//returns -1 if any sector of the QN is fermionic and odd-parity
//otherwise returns +1
int
paritySign(QN const& q);

//returns true if any sector of the QN is fermionic
bool
isFermionic(QN const& q);

void
read(std::istream & s, QN & q);

void
write(std::ostream & s, QN const& q);

void
printFull(QN const& q);


//
// QN Convenience Constructor Functions
//

QN inline
spin(int Sz) { return QN({Sz,1}); }

QN inline
boson(int Nb) { return QN({Nb,1}); }

QN inline
spinboson(int Sz, int Nb) { return QN({Sz,1},{Nb,1}); }

QN inline
fermion(int Nf) { return QN({Nf,-1}); }

QN inline
fparity(int Pf) { return QN({Pf,-2}); }

QN inline
electron(int Sz, int Nf) { return QN({Sz,1},{Nf,-1}); }

QN inline
elparity(int Sz, int Pf) { return QN({Sz,1},{Pf,-2}); }

QN inline
clock(int n, int N) { return QN({n,N}); }


} //namespace itensor

#endif
