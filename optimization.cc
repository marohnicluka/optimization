/*
 * optimization.cc
 *
 * Copyright 2017 Luka Marohnić
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * __________________________________________________________________________
 * |Example of using 'extrema', 'minimize' and 'maximize' functions to solve|
 * |the set of exercises found in:                                          |
 * |https://math.feld.cvut.cz/habala/teaching/veci-ma2/ema2r3.pdf           |
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * 1) Input:
 *        extrema(2x^3+9x*y^2+15x^2+27y^2,[x,y])
 *    Result: we get local minimum at origin, local maximum at (-5,0)
 *            and (-3,2),(-3,-2) as saddle points.
 *
 * 2) Input:
 *        extrema(x^3-2x^2+y^2+z^2-2x*y+x*z-y*z+3z,[x,y,z])
 *    Result: the given function has local minimum at (2,1,-2).
 *
 * 3) Input:
 *        minimize(x^2+2y^2,x^2-2x+2y^2+4y=0,[x,y]);
 *        maximize(x^2+2y^2,x^2-2x+2y^2+4y=0,[x,y])
 *    Result: the minimal value of x^2+2y^2 is 0 and the maximal is 12.
 *
 * 4) We need to minimize f(x,y,z)=x^2+(y+3)^2+(z-2)^2 for points (x,y,z)
 *    lying in plane x+y-z=1. Since the feasible area is not bounded, we're
 *    using the function 'extrema' because obviously the function has single
 *    local minimum.
 *    Input:
 *        extrema(x^2+(y+3)^2+(z-2)^2,x+y-z=1,[x,y,z])
 *    Result: the point closest to P in x+y-z=1 is (2,-1,0), and the distance
 *            is equal to sqrt(f(2,-1,0))=2*sqrt(3).
 *
 * 5) We're using the same method as in exercise 4.
 *    Input:
 *        extrema((x-1)^2+(y-2)^2+(z+1)^2,[x+y+z=1,2x-y+z=3],[x,y,z])
 *    Result: the closest point is (2,0,-1) and the corresponding distance
 *            equals to sqrt(5).
 *
 * 6) First we need to determine the feasible area. Plot its bounds with:
 *        implicitplot(x^2+(y+1)^2-4,x,y);line(y=-1);line(y=x+1)
 *    Now we see that the feasible area is given by set of inequalities:
 *        cond:=[x^2+(y+1)^2<=4,y>=-1,y<=x+1]
 *    Draw this area with:
 *        plotinequation(cond,[x,y],xstep=0.05,ystep=0.05)
 *    Now calculate global minimum and maximum of f(x,y)=x^2+4y^2 on that area:
 *        f(x,y):=x^2+4y^2;
 *        minimize(f(x,y),cond,[x,y]);maximize(f(x,y),cond,[x,y])
 *    Result: the minimum is 0 and the maximum is 8.
 *
 * 7) Input:
 *        minimize(x^2+y^2-6x+6y,x^2+y^2<=4,[x,y]);
 *        maximize(x^2+y^2-6x+6y,x^2+y^2<=4,[x,y])
 *    Result: the minimum is 4-12*sqrt(2) and the maximum is 4+12*sqrt(2).
 *
 * 8) Input:
 *        extrema(y,y^2+2x*y=2x-4x^2,[x,y])
 *    Result: we obtain (1/2,-1) as local minimum and (1/6,1/3) as local
 *            maximum of f(x,y)=y. Therefore, the maximal value is y(1/2)=-1
 *            and the maximal value is y(1/6)=1/3.
 *
 * The above set of exercises could be turned into an example Xcas worksheet.
 */

#include "giacPCH.h"
#include "giac.h"
#include "optimization.h"
#include "signalprocessing.h"
#include <sstream>
#include <bitset>

using namespace std;

#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC

#define GOLDEN_RATIO 1.61803398875
typedef unsigned long ulong;

gen make_idnt(const char* name,int index=-1,bool intern=true) {
    stringstream ss;
    if (intern)
        ss << " ";
    ss << string(name);
    if (index>=0)
        ss << index;
    return identificateur(ss.str().c_str());
}

/*
 * Return true iff the expression 'e' is constant with respect to
 * variables in 'vars'.
 */
bool is_constant_wrt_vars(const gen &e,vecteur &vars,GIAC_CONTEXT) {
    for (const_iterateur it=vars.begin();it!=vars.end();++it) {
        if (!is_constant_wrt(e,*it,contextptr))
            return false;
    }
    return true;
}

/*
 * Return true iff the expression 'e' is rational with respect to
 * variables in 'vars'.
 */
bool is_rational_wrt_vars(const gen &e,const vecteur &vars,GIAC_CONTEXT) {
    for (const_iterateur it=vars.begin();it!=vars.end();++it) {
        vecteur l(rlvarx(e,*it));
        if (l.size()>1)
            return false;
    }
    return true;
}

/*
 * Solves a system of equations.
 * This function is based on _solve but handles cases where a variable
 * is found inside trigonometric, hyperbolic or exponential functions.
 */
vecteur solve2(const vecteur &e_orig,const vecteur &vars_orig,GIAC_CONTEXT) {
    int m=e_orig.size(),n=vars_orig.size(),i=0;
    for (;i<m;++i) {
        if (!is_rational_wrt_vars(e_orig[i],vars_orig,contextptr))
            break;
    }
    if (n==1 || i==m)
        return *_solve(makesequence(e_orig,vars_orig),contextptr)._VECTptr;
    vecteur e(*halftan(_texpand(hyp2exp(e_orig,contextptr),contextptr),contextptr)._VECTptr);
    vecteur lv(*exact(lvar(_evalf(lvar(e),contextptr)),contextptr)._VECTptr);
    vecteur deps(n),depvars(n,gen(0));
    vecteur vars(vars_orig);
    const_iterateur it=lv.begin();
    for (;it!=lv.end();++it) {
        i=0;
        for (;i<n;++i) {
            if (is_undef(vars[i]))
                continue;
            if (*it==(deps[i]=vars[i]) ||
                    *it==(deps[i]=exp(vars[i],contextptr)) ||
                    is_zero(_simplify(*it-(deps[i]=tan(vars[i]/gen(2),contextptr)),contextptr))) {
                vars[i]=undef;
                depvars[i]=make_idnt("depvar",i);
                break;
            }
        }
        if (i==n)
            break;
    }
    if (it!=lv.end() || find(depvars.begin(),depvars.end(),gen(0))!=depvars.end())
        return *_solve(makesequence(e_orig,vars_orig),contextptr)._VECTptr;
    vecteur e_subs(subst(e,deps,depvars,false,contextptr));
    vecteur sol(*_solve(makesequence(e_subs,depvars),contextptr)._VECTptr);
    vecteur ret;
    for (const_iterateur it=sol.begin();it!=sol.end();++it) {
        vecteur r(n);
        i=0;
        for (;i<n;++i) {
            gen c(it->_VECTptr->at(i));
            if (deps[i].type==_IDNT)
                r[i]=c;
            else if (deps[i].is_symb_of_sommet(at_exp) && is_strictly_positive(c,contextptr))
                r[i]=_ratnormal(ln(c,contextptr),contextptr);
            else if (deps[i].is_symb_of_sommet(at_tan))
                r[i]=_ratnormal(2*atan(c,contextptr),contextptr);
            else
                break;
        }
        if (i==n)
            ret.push_back(r);
    }
    return ret;
}

/*
 * Traverse the tree of symbolic expression 'e' and collect all points of
 * transition in piecewise subexpressions, no matter of the inequality sign.
 * Nested piecewise expressions are not supported.
 */
void find_spikes(const gen &e,vecteur &cv,GIAC_CONTEXT) {
    if (e.type!=_SYMB)
        return;
    gen &f=e._SYMBptr->feuille;
    if (f.type==_VECT) {
        for (const_iterateur it=f._VECTptr->begin();it!=f._VECTptr->end();++it) {
            if (e.is_symb_of_sommet(at_piecewise) || e.is_symb_of_sommet(at_when)) {
                if (it->is_symb_of_sommet(at_equal) ||
                        it->is_symb_of_sommet(at_different) ||
                        it->is_symb_of_sommet(at_inferieur_egal) ||
                        it->is_symb_of_sommet(at_superieur_egal) ||
                        it->is_symb_of_sommet(at_inferieur_strict) ||
                        it->is_symb_of_sommet(at_superieur_strict)) {
                    vecteur &w=*it->_SYMBptr->feuille._VECTptr;
                    cv.push_back(w[0].type==_IDNT?w[1]:w[0]);
                }
            }
            else find_spikes(*it,cv,contextptr);
        }
    }
    else find_spikes(f,cv,contextptr);
}

bool next_binary_perm(vector<bool> &perm,int to_end=0) {
    if (to_end==int(perm.size()))
        return false;
    int end=int(perm.size())-1-to_end;
    perm[end]=!perm[end];
    return perm[end]?true:next_binary_perm(perm,to_end+1);
}

vecteur make_temp_vars(const vecteur &vars,const vecteur &ineq,GIAC_CONTEXT) {
    gen t,xmin,xmax;
    vecteur tmpvars;
    int index=0;
    for (const_iterateur it=vars.begin();it!=vars.end();++it) {
        xmin=undef;
        xmax=undef;
        for (const_iterateur jt=ineq.begin();jt!=ineq.end();++jt) {
            if (jt->is_symb_of_sommet(at_superieur_egal) &&
                    jt->_SYMBptr->feuille._VECTptr->front()==*it &&
                    (t=jt->_SYMBptr->feuille._VECTptr->back()).evalf(1,contextptr).type==_DOUBLE_)
                xmin=t;
            if (jt->is_symb_of_sommet(at_inferieur_egal) &&
                    jt->_SYMBptr->feuille._VECTptr->front()==*it &&
                    (t=jt->_SYMBptr->feuille._VECTptr->back()).evalf(1,contextptr).type==_DOUBLE_)
                xmax=t;
        }
        gen v=make_idnt("var",index++);
        if (!is_undef(xmax) && !is_undef(xmin))
            assume_t_in_ab(v,xmin,xmax,false,false,contextptr);
        else if (!is_undef(xmin))
            giac_assume(symb_superieur_egal(v,xmin),contextptr);
        else if (!is_undef(xmax))
            giac_assume(symb_inferieur_egal(v,xmax),contextptr);
        tmpvars.push_back(v);
    }
    return tmpvars;
}

/*
 * Determine critical points of function f under constraints g<=0 and h=0 using
 * Karush-Kuhn-Tucker conditions.
 */
vecteur solve_kkt(gen &f,vecteur &g,vecteur &h,vecteur &vars_orig,GIAC_CONTEXT) {
    int n=vars_orig.size(),m=g.size(),l=h.size();
    vecteur vars(vars_orig),gr_f(*_grad(makesequence(f,vars_orig),contextptr)._VECTptr),mug;
    matrice gr_g,gr_h;
    vars.resize(n+m+l);
    for (int i=0;i<m;++i) {
        vars[n+i]=make_idnt("mu",n+i);
        giac_assume(symb_superieur_strict(vars[n+i],gen(0)),contextptr);
        gr_g.push_back(*_grad(makesequence(g[i],vars_orig),contextptr)._VECTptr);
    }
    for (int i=0;i<l;++i) {
        vars[n+m+i]=make_idnt("lambda",n+m+i);
        gr_h.push_back(*_grad(makesequence(h[i],vars_orig),contextptr)._VECTptr);
    }
    vecteur eqv;
    for (int i=0;i<n;++i) {
        gen eq(gr_f[i]);
        for (int j=0;j<m;++j) {
            eq+=vars[n+j]*gr_g[j][i];
        }
        for (int j=0;j<l;++j) {
            eq+=vars[n+m+j]*gr_h[j][i];
        }
        eqv.push_back(eq);
    }
    eqv=mergevecteur(eqv,h);
    vector<bool> is_mu_zero(m,false);
    matrice cv;
    do {
        vecteur e(eqv);
        vecteur v(vars);
        for (int i=m-1;i>=0;--i) {
            if (is_mu_zero[i]) {
                e=subst(e,v[n+i],gen(0),false,contextptr);
                v.erase(v.begin()+n+i);
            }
            else
                e.push_back(g[i]);
        }
        cv=mergevecteur(cv,solve2(e,v,contextptr));
    } while(next_binary_perm(is_mu_zero));
    vars.resize(n);
    for (int i=cv.size()-1;i>=0;--i) {
        cv[i]._VECTptr->resize(n);
        for (int j=0;j<m;++j) {
            if (is_strictly_positive(subst(g[j],vars,cv[i],false,contextptr),contextptr)) {
                cv.erase(cv.begin()+i);
                break;
            }
        }
    }
    return cv;
}

/*
 * Determine critical points of an univariate function f(x). Points where it is
 * not differentiable are considered critical as well as zeros of the first
 * derivative. Also, bounds of the range of x are critical points.
 */
matrice critical_univariate(const gen &f,const gen &x,GIAC_CONTEXT) {
    gen df(_derive(makesequence(f,x),contextptr));
    matrice cv(*_zeros(makesequence(df,x),contextptr)._VECTptr);
    gen den(_denom(df,contextptr));
    if (!is_constant_wrt(den,x,contextptr)) {
        cv=mergevecteur(cv,*_zeros(makesequence(den,x),contextptr)._VECTptr);
    }
    find_spikes(f,cv,contextptr);  // assuming that f is not differentiable on transitions
    for (int i=cv.size()-1;i>=0;--i) {
        if (cv[i].is_symb_of_sommet(at_and))
            cv.erase(cv.begin()+i);
        else
            cv[i]=vecteur(1,cv[i]);
    }
    return cv;
}

/*
 * Compute global minimum mn and global maximum mx of function f(vars) under
 * conditions g<=0 and h=0. The list of points where global minimum is achieved
 * is returned.
 */
vecteur global_extrema(gen &f,vecteur &g,vecteur &h,vecteur &vars,gen &mn,gen &mx,GIAC_CONTEXT) {
    int n=vars.size();
    matrice cv;
    vecteur tmpvars=make_temp_vars(vars,g,contextptr);
    gen ff=subst(f,vars,tmpvars,false,contextptr);
    if (n==1) {
        cv=critical_univariate(ff,tmpvars[0],contextptr);
        for (const_iterateur it=g.begin();it!=g.end();++it) {
            cv.push_back(makevecteur(it->_SYMBptr->feuille._VECTptr->back()));
        }
    } else {
        vecteur gg=subst(g,vars,tmpvars,false,contextptr);
        vecteur hh=subst(h,vars,tmpvars,false,contextptr);
        cv=solve_kkt(ff,gg,hh,tmpvars,contextptr);
    }
    if (cv.empty())
        return vecteur(0);
    bool min_set=false,max_set=false;
    matrice min_locations;
    for (const_iterateur it=cv.begin();it!=cv.end();++it) {
        gen val=_eval(subst(f,vars,*it,false,contextptr),contextptr);
        if (min_set && is_exactly_zero(_ratnormal(val-mn,contextptr))) {
            if (find(min_locations.begin(),min_locations.end(),*it)==min_locations.end())
                min_locations.push_back(*it);
        }
        else if (!min_set || is_strictly_greater(mn,val,contextptr)) {
            mn=val;
            min_set=true;
            min_locations=vecteur(1,*it);
        }
        if (!max_set || is_strictly_greater(val,mx,contextptr)) {
            mx=val;
            max_set=true;
        }
    }
    if (n==1) {
        for (int i=0;i<int(min_locations.size());++i) {
            min_locations[i]=min_locations[i][0];
        }
    }
    return min_locations;
}

int parse_varlist(const gen &g,vecteur &vars,vecteur &ineq,vecteur &initial,GIAC_CONTEXT) {
    vecteur varlist(g.type==_VECT ? *g._VECTptr : vecteur(1,g));
    int n=0;
    for (const_iterateur it=varlist.begin();it!=varlist.end();++it) {
        if (it->is_symb_of_sommet(at_equal)) {
            vecteur &ops=*it->_SYMBptr->feuille._VECTptr;
            gen &v=ops.front(), &rh=ops.back();
            if (v.type!=_IDNT)
                return 0;
            vars.push_back(v);
            if (rh.is_symb_of_sommet(at_interval)) {
                vecteur &range=*rh._SYMBptr->feuille._VECTptr;
                if (!is_inf(range.front()))
                    ineq.push_back(symbolic(at_superieur_egal,makevecteur(v,range.front())));
                if (!is_inf(range.back()))
                    ineq.push_back(symbolic(at_inferieur_egal,makevecteur(v,range.back())));
            }
            else
                initial.push_back(rh);
        }
        else if (it->type!=_IDNT)
            return 0;
        else
            vars.push_back(*it);
        ++n;
    }
    return n;
}

/*
 * Function 'minimize' minimizes a multivariate continuous function on a
 * closed and bounded region using the method of Lagrange multipliers. The
 * feasible region is specified by bounding variables or by adding one or
 * more (in)equality constraints.
 *
 * Usage
 * ^^^^^
 *     minimize(obj,[constr],vars,[opt])
 *
 * Parameters
 * ^^^^^^^^^^
 *   - obj                 : objective function to minimize
 *   - constr (optional)   : single equality or inequality constraint or
 *                           a list of constraints, if constraint is given as
 *                           expression it is assumed that it is equal to zero
 *   - vars                : single variable or a list of problem variables, where
 *                           optional bounds of a variable may be set by appending '=a..b'
 *   - location (optional) : the option keyword 'locus' or 'coordinates' or 'point'
 *
 * Objective function must be continuous in all points of the feasible region,
 * which is assumed to be closed and bounded. If one of these condinitions is
 * not met, the final result may be incorrect.
 *
 * When the fourth argument is specified, point(s) at which the objective
 * function attains its minimum value are also returned as a list of vector(s).
 * The keywords 'locus', 'coordinates' and 'point' all have the same effect.
 * For univariate problems, a vector of numbers (x values) is returned, while
 * for multivariate problems it is a vector of vectors, i.e. a matrix.
 *
 * The function attempts to obtain the critical points in exact form, if the
 * parameters of the problem are all exact. It works best for problems in which
 * the lagrangian function gradient consists of rational expressions. Points at
 * which the function is not differentiable are also considered critical. This
 * function also handles univariate piecewise functions.
 *
 * If no critical points were obtained, the return value is undefined.
 *
 * Examples
 * ^^^^^^^^
 * minimize(sin(x),[x=0..4])
 *    >> sin(4)
 * minimize(asin(x),x=-1..1)
 *    >> -pi/2
 * minimize(x^2+cos(x),x=0..3)
 *    >> 1
 * minimize(x^4-x^2,x=-3..3,locus)
 *    >> -1/4,[-sqrt(2)/2]
 * minimize(abs(x),x=-1..1)
 *    >> 0
 * minimize(x-abs(x),x=-1..1)
 *    >> -2
 * minimize(abs(exp(-x^2)-1/2),x=-4..4)
 *    >> 0
 * minimize(piecewise(x<=-2,x+6,x<=1,x^2,3/2-x/2),x=-3..2)
 *    >> 0
 * minimize(x^2-3x+y^2+3y+3,[x=2..4,y=-4..-2],point)
 *    >> -1,[[2,-2]]
 * minimize(2x^2+y^2,x+y=1,[x,y])
 *    >> 2/3
 * minimize(2x^2-y^2+6y,x^2+y^2<=16,[x,y])
 *    >> -40
 * minimize(x*y+9-x^2-y^2,x^2+y^2<=9,[x,y])
 *    >> -9/2
 * minimize(sqrt(x^2+y^2)-z,[x^2+y^2<=16,x+y+z=10],[x,y,z])
 *    >> -4*sqrt(2)-6
 * minimize(x*y*z,x^2+y^2+z^2=1,[x,y,z])
 *    >> -sqrt(3)/9
 * minimize(sin(x)+cos(x),x=0..20,coordinates)
 *    >> -sqrt(2),[5*pi/4,13*pi/4,21*pi/4]
 * minimize((1+x^2+3y+5x-4*x*y)/(1+x^2+y^2),x^2/4+y^2/3=9,[x,y])
 *    >> -2.44662490691
 * minimize(x^2-2x+y^2+1,[x+y<=0,x^2<=4],[x,y],locus)
 *    >> 1/2,[[1/2,-1/2]]
 * minimize(x^2*(y+1)-2y,[y<=2,sqrt(1+x^2)<=y],[x,y])
 *    >> -4
 * minimize(4x^2+y^2-2x-4y+1,4x^2+y^2=1,[x,y])
 *    >> -sqrt(17)+2
 * minimize(cos(x)^2+cos(y)^2,x+y=pi/4,[x,y],locus)
 *    >> (-sqrt(2)+2)/2,[[5*pi/8,-3*pi/8]]
 * minimize(x^2+y^2,x^4+y^4=2,[x,y])
 *    >> 1.41421356237
 * minimize(z*x*exp(y),z^2+x^2+exp(2y)=1,[x,y,z])
 *    >> -sqrt(3)/9
 */
gen _minimize(const gen &args,GIAC_CONTEXT) {
    if (args.type==_STRNG && args.subtype==-1) return args;
    if (args.type!=_VECT || args.subtype!=_SEQ__VECT || args._VECTptr->size()>4)
        return gentypeerr(contextptr);
    vecteur &argv=*args._VECTptr,g,h;
    bool location=false;
    int nargs=argv.size();
    if (argv.back()==at_coordonnees || argv.back()==at_lieu || argv.back()==at_point) {
        location=true;
        --nargs;
    }
    if (nargs==3) {
        vecteur constr(argv[1].type==_VECT ? *argv[1]._VECTptr : vecteur(1,argv[1]));
        for (const_iterateur it=constr.begin();it!=constr.end();++it) {
            if ((*it).is_symb_of_sommet(at_equal))
                h.push_back(equal2diff(*it));
            else if ((*it).is_symb_of_sommet(at_superieur_egal) ||
                     (*it).is_symb_of_sommet(at_inferieur_egal))
                g.push_back(*it);
            else
                h.push_back(*it);
        }
    }
    vecteur vars,initial;
    int n;  // number of variables
    if ((n=parse_varlist(argv[nargs-1],vars,g,initial,contextptr))==0 || !initial.empty())
        return gensizeerr(contextptr);
    if (n>1) {
        for (int i=0;i<int(g.size());++i) {
            gen &gi=g[i];
            vecteur &s=*gi._SYMBptr->feuille._VECTptr;
            g[i]=gi.is_symb_of_sommet(at_inferieur_egal)?s[0]-s[1]:s[1]-s[0];
        }
    }
    gen &f=argv[0];
    gen mn,mx;
    vecteur loc(global_extrema(f,g,h,vars,mn,mx,contextptr));
    if (loc.empty())
        return undef;
    if (location)
        return makesequence(_simplify(mn,contextptr),_simplify(loc,contextptr));
    return _simplify(mn,contextptr);
}
static const char _minimize_s []="minimize";
static define_unary_function_eval (__minimize,&_minimize,_minimize_s);
define_unary_function_ptr5(at_minimize,alias_at_minimize,&__minimize,0,true)

/*
 * 'maximize' takes the same arguments as the function 'minimize', but
 * maximizes the objective function. See 'minimize' for details.
 *
 * Examples
 * ^^^^^^^^
 * maximize(cos(x),x=1..3)
 *    >> cos(1)
 * maximize(piecewise(x<=-2,x+6,x<=1,x^2,3/2-x/2),x=-3..2)
 *    >> 4
 * minimize(x-abs(x),x=-1..1)
 *    >> 0
 * maximize(x^2-3x+y^2+3y+3,[x=2..4,y=-4..-2])
 *    >> 11
 * maximize(x*y*z,x^2+2*y^2+3*z^2<=1,[x,y,z],point)
 *    >> sqrt(2)/18,[[-sqrt(3)/3,sqrt(6)/6,-1/3],[sqrt(3)/3,-sqrt(6)/6,-1/3],
 *                   [-sqrt(3)/3,-sqrt(6)/6,1/3],[sqrt(3)/3,sqrt(6)/6,1/3]]
 * maximize(x^2-x*y+2*y^2,[x=-1..0,y=-1/2..1/2],coordinates)
 *    >> 2,[[-1,1/2]]
 * maximize(x*y,[x+y^2<=2,x>=0,y>=0],[x,y],locus)
 *    >> 4*sqrt(6)/9,[[4/3,sqrt(6)/3]]
 * maximize(y^2-x^2*y,y<=x,[x=0..2,y=0..2])
 *    >> 4/27
 * maximize(2x+y,4x^2+y^2=8,[x,y])
 *    >> 4
 * maximize(x^2*(y+1)-2y,[y<=2,sqrt(1+x^2)<=y],[x,y])
 *    >> 5
 * maximize(4x^2+y^2-2x-4y+1,4x^2+y^2=1,[x,y])
 *    >> sqrt(17)+2
 * maximize(3x+2y,2x^2+3y^2<=3,[x,y])
 *    >> sqrt(70)/2
 * maximize(x*y,[2x+3y<=10,x>=0,y>=0],[x,y])
 *    >> 25/6
 * maximize(x^2+y^2+z^2,[x^2/16+y^2+z^2=1,x+y+z=0],[x,y,z])
 *    >> 8/3
 * assume(a>0);maximize(x^2*y^2*z^2,x^2+y^2+z^2=a^2,[x,y,z])
 *    >> a^6/27
 */
gen _maximize(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT || g._VECTptr->size()<2)
        return gentypeerr(contextptr);
    vecteur gv(*g._VECTptr);
    gv[0]=-gv[0];
    gen res=_minimize(_feuille(gv,contextptr),contextptr);
    if (res.type==_VECT && res._VECTptr->size()>0) {
        res._VECTptr->front()=-res._VECTptr->front();
    }
    else if (res.type!=_VECT)
        res=-res;
    return res;
}
static const char _maximize_s []="maximize";
static define_unary_function_eval (__maximize,&_maximize,_maximize_s);
define_unary_function_ptr5(at_maximize,alias_at_maximize,&__maximize,0,true)

int ipdiff::sum_ivector(const ivector &v,bool drop_last) {
    int res=0;
    for (ivector_iter it=v.begin();it!=v.end()-drop_last?1:0;++it) {
        res+=*it;
    }
    return res;
}

/*
 * IPDIFF CLASS IMPLEMENTATION
 */

ipdiff::ipdiff(const gen &f_orig,const vecteur &g_orig,const vecteur &vars_orig,GIAC_CONTEXT) {
    ctx=contextptr;
    f=f_orig;
    g=g_orig;
    vars=vars_orig;
    ord=0;
    nconstr=g.size();
    nvars=vars.size()-nconstr;
    assert(nvars>0);
    pdv[ivector(nvars,0)]=f; // make the zeroth order derivative initially available
}

void ipdiff::ipartition(int m,int n,ivectors &c,const ivector &p) {
    for (int i=0;i<n;++i) {
        if (!p.empty() && p[i]!=0)
            continue;
        ivector r;
        if (p.empty())
            r.resize(n,0);
        else r=p;
        for (int j=0;j<m;++j) {
            ++r[i];
            int s=sum_ivector(r);
            if (s==m && find(c.begin(),c.end(),r)==c.end())
                c.push_back(r);
            else if (s<m)
                ipartition(m,n,c,r);
            else break;
        }
    }
}

ipdiff::diffterms ipdiff::derive_diffterms(const diffterms &terms,ivector &sig) {
    while (!sig.empty() && sig.back()==0) {
        sig.pop_back();
    }
    if (sig.empty())
        return terms;
    int k=sig.size()-1,p;
    diffterms tv;
    ivector u(nvars+1,0);
    for (diffterms::const_iterator it=terms.begin();it!=terms.end();++it) {
        int c=it->second;
        diffterm t(it->first);
        const ivector_map &h_orig=it->first.second;
        ++t.first.at(k);
        tv[t]+=c;
        --t.first.at(k);
        ivector_map h(h_orig);
        for (ivector_map::const_iterator jt=h_orig.begin();jt!=h_orig.end();++jt) {
            ivector v=jt->first;
            if ((p=jt->second)==0)
                continue;
            if (p==1)
                h.erase(h.find(v));
            else
                --h[v];
            ++v[k];
            ++h[v];
            t.second=h;
            tv[t]+=c*p;
            --h[v];
            --v[k];
            ++h[v];
        }
        t.second=h_orig;
        for (int i=0;i<nconstr;++i) {
            ++t.first.at(nvars+i);
            u[k]=1;
            u.back()=i;
            ++t.second[u];
            tv[t]+=c;
            --t.first.at(nvars+i);
            --t.second[u];
            u[k]=0;
        }
    }
    --sig.back();
    return derive_diffterms(tv,sig);
}

const gen &ipdiff::get_pd(const pd_map &pds,const ivector &sig) const {
    try {
        return pds.at(sig);
    }
    catch (out_of_range &e) {
        return undef;
    }
}

const gen &ipdiff::differentiate(const gen &e,pd_map &pds,const ivector &sig) {
    const gen &pd=get_pd(pds,sig);
    if (!is_undef(pd))
        return pd;
    vecteur v(1,e);
    bool do_derive=false;
    assert(vars.size()<=sig.size());
    for (int i=0;i<int(vars.size());++i) {
        if (sig[i]>0) {
            v=mergevecteur(v,vecteur(sig[i],vars[i]));
            do_derive=true;
        }
    }
    if (do_derive)
        return pds[sig]=_derive(_feuille(v,ctx),ctx);
    return e;
}

void ipdiff::compute_h(const vector<diffterms> &grv,int order) {
    if (g.empty())
        return;
    ivectors hsigv;
    matrice A;
    vecteur b(g.size()*grv.size(),gen(0));
    gen t;
    int grv_sz=grv.size();
    for (int i=0;i<nconstr;++i) {
        for (int j=0;j<grv_sz;++j) {
            vecteur eq(g.size()*grv_sz,gen(0));
            const diffterms &grvj=grv[j];
            for (diffterms::const_iterator it=grvj.begin();it!=grvj.end();++it) {
                ivector sig(it->first.first),hsig;
                sig.push_back(i);
                t=gen(it->second)*differentiate(g[i],pdg,sig);
                for (ivector_map::const_iterator ht=it->first.second.begin();ht!=it->first.second.end();++ht) {
                    if (ht->second==0)
                        continue;
                    const ivector &sigh=ht->first;
                    if (sum_ivector(sigh,true)<order) {
                        gen h(get_pd(pdh,sigh));
                        assert(!is_undef(h));
                        t=t*pow(h,ht->second);
                    }
                    else {
                        assert(ht->second==1);
                        hsig=sigh;
                    }
                }
                if (hsig.empty())
                    b[grv_sz*i+j]-=t;
                else {
                    int k=0,hsigv_sz=hsigv.size();
                    for (;k<hsigv_sz;++k) {
                        if (hsigv[k]==hsig)
                            break;
                    }
                    eq[k]+=t;
                    if (k==hsigv_sz)
                        hsigv.push_back(hsig);
                }
            }
            A.push_back(*_ratnormal(eq,ctx)._VECTptr);
        }
    }
    matrice B;
    B.push_back(*_ratnormal(b,ctx)._VECTptr);
    matrice invA=*_inv(A,ctx)._VECTptr;
    vecteur sol(*mtran(mmult(invA,mtran(B))).front()._VECTptr);
    for (int i=0;i<int(sol.size());++i) {
        pdh[hsigv[i]]=_ratnormal(sol[i],ctx);
    }
}

void ipdiff::find_nearest_terms(const ivector &sig,diffterms &match,ivector &excess) {
    excess=sig;
    int i;
    for (map<ivector,diffterms>::const_iterator it=cterms.begin();it!=cterms.end();++it) {
        ivector ex(nvars,0);
        for (i=0;i<nvars;++i) {
            if ((ex[i]=sig[i]-it->first.at(i))<0)
                break;
        }
        if (i<nvars)
            continue;
        if (sum_ivector(ex)<sum_ivector(excess)) {
            excess=ex;
            match=it->second;
        }
    }
}

void ipdiff::raise_order(int order) {
    if (g.empty())
        return;
    ivectors c;
    ivector excess,init_f(nvars+nconstr,0);
    diffterm init_term;
    init_term.first=init_f;
    diffterms init_terms;
    init_terms[init_term]=1;
    vector<diffterms> grv;
    for (int k=ord+1;k<=order;++k) {
        grv.clear();
        c.clear();
        ipartition(k,nvars,c);
        for (ivectors::const_iterator it=c.begin();it!=c.end();++it) {
            diffterms terms=init_terms;
            find_nearest_terms(*it,terms,excess);
            if (sum_ivector(excess)>0) {
                terms=derive_diffterms(terms,excess);
                cterms[*it]=terms;
            }
            grv.push_back(terms);
        }
        compute_h(grv,k);
    }
    ord=order;
}

void ipdiff::compute_pd(int order,const ivector &sig) {
    gen pd;
    ivectors c;
    ipartition(order,nvars,c);
    for (ivectors::const_iterator ct=c.begin();ct!=c.end();++ct) {
        if (!sig.empty() && sig!=*ct)
            continue;
        if (g.empty()) {
            differentiate(f,pdv,sig);
            continue;
        }
        diffterms &terms=cterms[*ct];
        pd=gen(0);
        for (diffterms::const_iterator it=terms.begin();it!=terms.end();++it) {
            ivector sig(it->first.first);
            gen t(gen(it->second)*differentiate(f,pdf,sig));
            if (!is_zero(t)) {
                for (ivector_map::const_iterator jt=it->first.second.begin();jt!=it->first.second.end();++jt) {
                    if (jt->second==0)
                        continue;
                    gen h(get_pd(pdh,jt->first));
                    assert(!is_undef(h));
                    t=t*pow(h,jt->second);
                }
                pd+=t;
            }
        }
        pdv[*ct]=_ratnormal(pd,ctx);
    }
}

void ipdiff::gradient(vecteur &res) {
    if (nconstr==0)
        res=*_grad(makesequence(f,vars),ctx)._VECTptr;
    else {
        res.resize(nvars);
        ivector sig(nvars,0);
        if (ord<1) {
            raise_order(1);
            compute_pd(1);
        }
        for (int i=0;i<nvars;++i) {
            sig[i]=1;
            res[i]=derivative(sig);
            sig[i]=0;
        }
    }
}

void ipdiff::hessian(matrice &res) {
    if (nconstr==0)
        res=*_hessian(makesequence(f,vars),ctx)._VECTptr;
    else {
        res.clear();
        ivector sig(nvars,0);
        if (ord<2) {
            raise_order(2);
            compute_pd(2);
        }
        for (int i=0;i<nvars;++i) {
            vecteur r(nvars);
            ++sig[i];
            for (int j=0;j<nvars;++j) {
                ++sig[j];
                r[j]=derivative(sig);
                --sig[j];
            }
            res.push_back(r);
            --sig[i];
        }
    }
}

const gen &ipdiff::derivative(const ivector &sig) {
    if (nconstr==0)
        return differentiate(f,pdf,sig);
    int k=sum_ivector(sig); // the order of the derivative
    if (k>ord) {
        raise_order(k);
        compute_pd(k,sig);
    }
    return get_pd(pdv,sig);
}

const gen &ipdiff::derivative(const vecteur &dvars) {
    ivector sig(nvars,0);
    const_iterateur jt;
    for (const_iterateur it=dvars.begin();it!=dvars.end();++it) {
        if ((jt=find(vars.begin(),vars.end(),*it))==vars.end())
            return undef;
        ++sig[jt-vars.begin()];
    }
    return derivative(sig);
}

void ipdiff::partial_derivatives(int order,pd_map &pdmap) {
    if (nconstr>0 && ord<order) {
        raise_order(order);
        compute_pd(order);
    }
    ivectors c;
    ipartition(order,nvars,c);
    for (ivectors::const_iterator it=c.begin();it!=c.end();++it) {
        pdmap[*it]=derivative(*it);
    }
}

gen ipdiff::taylor_term(const vecteur &a,int k) {
    assert(k>=0);
    if (k==0)
        return subst(f,vars,a,false,ctx);
    ivectors sigv;
    ipartition(k,nvars,sigv);
    gen term(0);
    if (nconstr>0) while (k>ord) {
        raise_order(ord+1);
        compute_pd(ord);
    }
    for (ivectors::const_iterator it=sigv.begin();it!=sigv.end();++it) {
        gen pd;
        if (g.empty()) {
            vecteur args(1,f);
            for (int i=0;i<nvars;++i) {
                for (int j=0;j<it->at(i);++j) {
                    args.push_back(vars[i]);
                }
            }
            pd=_derive(_feuille(args,ctx),ctx);
        }
        else
            pd=derivative(*it);
        pd=subst(pd,vars,a,false,ctx);
        for (int i=0;i<nvars;++i) {
            int ki=it->at(i);
            if (ki==0)
                continue;
            pd=pd*pow(vars[i]-a[i],ki)/factorial(ki);
        }
        term+=pd;
    }
    return term;
}

gen ipdiff::taylor(const vecteur &a,int order) {
    assert(order>=0);
    gen T(0);
    for (int k=0;k<=order;++k) {
        T+=taylor_term(a,k);
    }
    return T;
}

/*
 * END OF IPDIFF CLASS
 */

void vars_arrangements(matrice J,ipdiff::ivectors &arrs,GIAC_CONTEXT) {
    int m=J.size(),n=J.front()._VECTptr->size();
    assert(n<=32 && m<n);
    matrice tJ(mtran(J));
    ulong N=std::pow(2,n);
    vector<ulong> sets(comb(n,m).val);
    int i=0;
    for (ulong k=1;k<N;++k) {
        bitset<32> b(k);
        if (b.count()==(size_t)m)
            sets[i++]=k;
    }
    matrice S;
    ipdiff::ivector arr(n);
    for (vector<ulong>::const_iterator it=sets.begin();it!=sets.end();++it) {
        for (i=0;i<n;++i) arr[i]=i;
        N=std::pow(2,n);
        for (i=n;i-->0;) {
            N/=2;
            if ((*it & N)!=0) {
                arr.erase(arr.begin()+i);
                arr.push_back(i);
            }
        }
        S.clear();
        for (ipdiff::ivector::const_iterator it=arr.end()-m;it!=arr.end();++it) {
            S.push_back(tJ[*it]);
        }
        if (!is_zero(_det(S,contextptr)))
            arrs.push_back(arr);
    }
}

matrice jacobian(vecteur &g,vecteur &vars,GIAC_CONTEXT) {
    matrice J;
    for (int i=0;i<int(g.size());++i) {
        J.push_back(*_grad(makesequence(g[i],vars),contextptr)._VECTptr);
    }
    return J;
}

bool ck_jacobian(vecteur &g,vecteur &vars,GIAC_CONTEXT) {
    matrice J(jacobian(g,vars,contextptr));
    int m=g.size();
    int n=vars.size()-m;
    if (_rank(J,contextptr).val<m)
        return false;
    J=mtran(J);
    J.erase(J.begin(),J.begin()+n);
    return !is_zero(_det(J,contextptr));
}

/*
 * 'implicitdiff' differentiates function(s) defined by equation(s) or a
 * function f(x1,x2,...,xn,y1,y2,...,ym) where y1,...,ym are functions of
 * x1,x2,...,xn defined by m equality constraints.
 *
 * Usage
 * ^^^^^
 *      implicitdiff(f,constr,depvars,diffvars)
 *      implicitdiff(f,constr,vars,order_size=<posint>,[P])
 *      implicitdiff(constr,[depvars],y,diffvars)
 *
 * Parameters
 * ^^^^^^^^^^
 *      - f         : expression
 *      - constr    : (list of) equation(s)
 *      - depvars   : (list of) dependent variable(s), each of them given
 *                    either as a symbol, e.g. y, or a function, e.g. y(x,z)
 *      - diffvars  : sequence of variables w.r.t. which the differentiation
 *                    will be carried out
 *      - vars      : list all variables on which f depends such that
 *                  : dependent variables come after independent ones
 *      - P         : (list of) coordinate(s) to compute derivatives at
 *      - y         : (list of) dependent variable(s) to differentiate w.r.t.
 *                    diffvars, each of them given as a symbol
 *
 * The return value is partial derivative specified by diffvars. If
 * 'order_size=m' is given as the fourth argument, all partial derivatives of
 * order m will be computed and returned as vector for m=1, matrix for m=2 or
 * table for m>2. The first two cases produce gradient and hessian of f,
 * respectively. For m>2, the partial derivative
 * pd=d^m(f)/(d^k1(x1)*d^k2(x2)*...*d^kn(xn)) is saved under key [k1,k2,...kn].
 * If P is specified, pd(P) is saved.
 *
 * Examples
 * ^^^^^^^^
 * implicitdiff(x^2*y+y^2=1,y,x)
 *      >> -2*x*y/(x^2+2*y)
 * implicitdiff(R=P*V/T,P,T)
 *      >> P/T
 * implicitdiff([x^2+y=z,x+y*z=1],[y(x),z(x)],y,x)
 *      >> (-2*x*y-1)/(y+z)
 * implicitdiff([x^2+y=z,x+y*z=1],[y(x),z(x)],[y,z],x)
 *      >> [(-2*x*y-1)/(y+z),(2*x*z-1)/(y+z)]
 * implicitdiff(y=x^2/z,y,x)
 *      >> 2x/z
 * implicitdiff(y=x^2/z,y,z)
 *      >> -x^2/z^2
 * implicitdiff(y^3+x^2=1,y,x)
 *      >> -2*x/(3*y^2)
 * implicitdiff(y^3+x^2=1,y,x,x)
 *      >> (-8*x^2-6*y^3)/(9*y^5)x+3y-z,2x^2+y^2=z,[x,y,z]
 * implicitdiff(a*x^3*y-2y/z=z^2,y(x,z),x)
 *      >> -3*a*x^2*y*z/(a*x^3*z-2)
 * implicitdiff(a*x^3*y-2y/z=z^2,y(x,z),x,z)
 *      >> (12*a*x^2*y-6*a*x^2*z^3)/(a^2*x^6*z^2-4*a*x^3*z+4)
 * implicitdiff([-2x*z+y^2=1,x^2-exp(x*z)=y],[y(x),z(x)],y,x)
 *      >> 2*x/(y*exp(x*z)+1)
 * implicitdiff([-2x*z+y^2=1,x^2-exp(x*z)=y],[y(x),z(x)],[y,z],x)
 *      >> [2*x/(y*exp(x*z)+1),(2*x*y-y*z*exp(x*z)-z)/(x*y*exp(x*z)+x)]
 * implicitdiff([a*sin(u*v)+b*cos(w*x)=c,u+v+w+x=z,u*v+w*x=z],[u(x,z),v(x,z),w(x,z)],u,z)
 *      >> (a*u*x*cos(u*v)-a*u*cos(u*v)+b*u*x*sin(w*x)-b*x*sin(w*x))/
 *         (a*u*x*cos(u*v)-a*v*x*cos(u*v)+b*u*x*sin(w*x)-b*v*x*sin(w*x))
 * implicitdiff(x*y,-2x^3+15x^2*y+11y^3-24y=0,y(x),x$2)
 *      >> (162*x^5*y+1320*x^4*y^2-320*x^4-3300*x^3*y^3+800*x^3*y+968*x^2*y^4-1408*x^2*y^2+
 *          512*x^2-3630*x*y^5+5280*x*y^3-1920*x*y)/(125*x^6+825*x^4*y^2-600*x^4+
 *          1815*x^2*y^4-2640*x^2*y^2+960*x^2+1331*y^6-2904*y^4+2112*y^2-512)
 * implicitdiff((x-u)^2+(y-v)^2,[x^2/4+y^2/9=1,(u-3)^2+(v+5)^2=1],[v(u,x),y(u,x)],u,x)
 *      >> (-9*u*x-4*v*y+27*x-20*y)/(2*v*y+10*y)
 * implicitdiff(x*y*z,-2x^3+15x^2*y+11y^3-24y=0,[x,z,y],order_size=1)
 *      >> [(2*x^3*z-5*x^2*y*z+11*y^3*z-8*y*z)/(5*x^2+11*y^2-8),x*y]
 * implicitdiff(x*y*z,-2x^3+15x^2*y+11y^3-24y=0,[x,z,y],order_size=2,[1,-1,0])
 *      >> [[64/9,-2/3],[-2/3,0]]
 * pd:=implicitdiff(x*y*z,-2x^3+15x^2*y+11y^3-24y=0,[x,z,y],order_size=4,[0,z,0]);pd[4,0,0]
 *      >> -2*z
 */
gen _implicitdiff(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT || g._VECTptr->size()<2)
        return gentypeerr(contextptr);
    vecteur &gv=*g._VECTptr;
    gen &f=gv[0];
    if (int(gv.size())<3)
        return gensizeerr(contextptr);
    int ci=gv[0].type!=_VECT && !gv[0].is_symb_of_sommet(at_equal)?1:0;
    vecteur freevars,depvars,diffdepvars;
    gen_map diffvars;
    // get the constraints as a list of vanishing expressions
    vecteur constr(gv[ci].type==_VECT?*gv[ci]._VECTptr:vecteur(1,gv[ci]));
    for (int i=0;i<int(constr.size());++i) {
        if (constr[i].is_symb_of_sommet(at_equal))
            constr[i]=equal2diff(constr[i]);
    }
    int m=constr.size();
    int dvi=3;
    if (ci==0) {
        if (gv[ci+1].type==_VECT)
            diffdepvars=gv[ci+2].type==_VECT?*gv[ci+2]._VECTptr:vecteur(1,gv[ci+2]);
        else
            dvi=2;
    }
    bool compute_all=false;
    int order=0;
    if (ci==1 && gv[dvi].is_symb_of_sommet(at_equal)) {
        vecteur &v=*gv[dvi]._SYMBptr->feuille._VECTptr;
        if (v.front()!=at_order_size || !v.back().is_integer())
            return gentypeerr(contextptr);
        order=v.back().val;
        if (order<=0)
            return gendimerr(contextptr);
        compute_all=true;
    }
    // get dependency specification
    vecteur deplist(gv[ci+1].type==_VECT?*gv[ci+1]._VECTptr:vecteur(1,gv[ci+1]));
    if (compute_all) {
        // vars must be specified as x1,x2,...,xn,y1,y2,...,ym
        int nd=deplist.size();
        if (nd<=m)
            return gensizeerr(contextptr);
        for (int i=0;i<nd;++i) {
            if (i<nd-m)
                freevars.push_back(deplist[i]);
            else
                depvars.push_back(deplist[i]);
        }
    }
    else {
        // get (in)dependent variables
        for (const_iterateur it=deplist.begin();it!=deplist.end();++it) {
            if (it->type==_IDNT)
                depvars.push_back(*it);
            else if (it->is_symb_of_sommet(at_of)) {
                vecteur fe(*it->_SYMBptr->feuille._VECTptr);
                depvars.push_back(fe.front());
                if (fe.back().type==_VECT) {
                    for (int i=0;i<int(fe.back()._VECTptr->size());++i) {
                        gen &x=fe.back()._VECTptr->at(i);
                        if (find(freevars.begin(),freevars.end(),x)==freevars.end())
                            freevars.push_back(x);
                    }
                }
                else
                    freevars.push_back(fe.back());
            }
            else
                return gentypeerr(contextptr);
        }
        // get diffvars
        for (const_iterateur it=gv.begin()+dvi;it!=gv.end();++it) {
            gen v(eval(*it,contextptr));
            gen x;
            if (v.type==_IDNT)
                diffvars[(x=v)]+=1;
            else if (v.type==_VECT && v.subtype==_SEQ__VECT)
                diffvars[(x=v._VECTptr->front())]+=v._VECTptr->size();
            else
                return gentypeerr(contextptr);
            if (find(freevars.begin(),freevars.end(),x)==freevars.end())
                freevars.push_back(x);
        }
    }
    int n=freevars.size();  // number of independent variables
    if (m!=int(depvars.size()))
        return gensizeerr(contextptr);
    vecteur vars(mergevecteur(freevars,depvars));  // list of all variables
    // check whether the conditions of implicit function theorem hold
    if (!ck_jacobian(constr,vars,contextptr))
        return gendimerr(contextptr);
    // build partial derivative specification 'sig'
    ipdiff::ivector sig(n,0); // sig[i]=k means: derive k times with respect to ith independent variable
    ipdiff ipd(f,constr,vars,contextptr);
    if (compute_all) {
        vecteur pt(0);
        if (int(gv.size())>4) {
            pt=gv[4].type==_VECT?*gv[4]._VECTptr:vecteur(1,gv[4]);
            if (int(pt.size())!=n+m)
                return gensizeerr(contextptr);
        }
        ipdiff::pd_map pdv;
        ipd.partial_derivatives(order,pdv);
        if (order==1) {
            vecteur gr;
            ipd.gradient(gr);
            return pt.empty()?gr:_ratnormal(subst(gr,vars,pt,false,contextptr),contextptr);
        }
        else if (order==2) {
            matrice hess;
            ipd.hessian(hess);
            return pt.empty()?hess:_ratnormal(subst(hess,vars,pt,false,contextptr),contextptr);
        }
        else {
            ipdiff::ivectors c;
            ipdiff::ipartition(order,n,c);
            gen_map ret_pdv;
            for (ipdiff::ivectors::const_iterator it=c.begin();it!=c.end();++it) {
                vecteur v;
                for (int i=0;i<n;++i) {
                    v.push_back(gen(it->at(i)));
                }
                ret_pdv[v]=pt.empty()?pdv[*it]:_ratnormal(subst(pdv[*it],vars,pt,false,contextptr),contextptr);
            }
            return ret_pdv;
        }
    }
    for (gen_map::const_iterator it=diffvars.begin();it!=diffvars.end();++it) {
        int i=0;
        for (;i<n;++i) {
            if (it->first==freevars[i]) {
                sig[i]=it->second.val;
                break;
            }
        }
        assert(i<n);
    }
    // compute the partial derivative specified by 'sig'
    order=ipdiff::sum_ivector(sig);
    if (ci==1)
        return _ratnormal(ipd.derivative(sig),contextptr);
    vecteur ret;
    if (diffdepvars.empty()) {
        assert(m==1);
        diffdepvars=vecteur(1,depvars.front());
    }
    for (const_iterateur it=diffdepvars.begin();it!=diffdepvars.end();++it) {
        if (find(depvars.begin(),depvars.end(),*it)==depvars.end()) {
            // variable *it is not in depvars, so it's treated as a constant
            ret.push_back(gen(0));
            continue;
        }
        ipdiff tmp(*it,constr,vars,contextptr);
        ret.push_back(_ratnormal(tmp.derivative(sig),contextptr));
    }
    return ret.size()==1?ret.front():ret;
}
static const char _implicitdiff_s []="implicitdiff";
static define_unary_function_eval (__implicitdiff,&_implicitdiff,_implicitdiff_s);
define_unary_function_ptr5(at_implicitdiff,alias_at_implicitdiff,&__implicitdiff,0,true)

void find_local_extrema(gen_map &cpts,const gen &f,const vecteur &g,const vecteur &vars,const ipdiff::ivector &arr,
                        const vecteur &ineq,const vecteur &initial,int order_size,GIAC_CONTEXT) {
    assert(order_size>=0);
    int nv=vars.size(),m=g.size(),n=nv-m,cls;
    vecteur tmpvars=make_temp_vars(vars,ineq,contextptr);
    if (order_size==0 && m>0) { // apply the method of Lagrange
        gen L(f);
        vecteur multipliers(m),allinitial;
        if (!initial.empty())
            allinitial=mergevecteur(vecteur(m,0),initial);
        for (int i=m;i-->0;) {
            L+=-(multipliers[i]=make_idnt("lambda",i))*g[i];
        }
        L=subst(L,vars,tmpvars,false,contextptr);
        vecteur allvars=mergevecteur(multipliers,tmpvars),
                gr=*_grad(makesequence(L,allvars),contextptr)._VECTptr,
                eqv=mergevecteur(gr,subst(g,vars,tmpvars,false,contextptr));
        matrice cv,bhess;
        if (allinitial.empty())
            cv=solve2(eqv,allvars,contextptr);
        else {
            vecteur fsol(*_fsolve(makesequence(eqv,allvars,allinitial),contextptr)._VECTptr);
            if (!fsol.empty())
                cv.push_back(fsol);
        }
        if (!cv.empty())
            bhess=*_hessian(makesequence(L,allvars),contextptr)._VECTptr; // bordered Hessian
        gen s,cpt;
        for (const_iterateur it=cv.begin();it!=cv.end();++it) {
            matrice H=subst(bhess,allvars,*it,false,contextptr);
            cls=_CPCLASS_UNDECIDED;
            for (int k=1;k<=n;++k) {
                matrice M;
                for (int i=0;i<2*m+k;++i) {
                    const vecteur &row=*H[i]._VECTptr;
                    M.push_back(vecteur(row.begin(),row.begin()+2*m+k));
                }
                s=_sign(_det(M,contextptr),contextptr);
                if (is_zero(s)) {
                    cls=_CPCLASS_UNDECIDED;
                    break;
                }
                if (cls==_CPCLASS_SADDLE) continue;
                if (cls!=_CPCLASS_MAX && is_strictly_positive(s*pow(gen(-1),m),contextptr)) cls=_CPCLASS_MIN;
                else if (cls!=_CPCLASS_MIN && is_strictly_positive(s*pow(gen(-1),m+k),contextptr)) cls=_CPCLASS_MAX;
                else cls=_CPCLASS_SADDLE;
            }
            cpt=subst(*_simplify(vecteur(it->_VECTptr->begin()+m,it->_VECTptr->end()),contextptr)._VECTptr,
                      tmpvars,vars,false,contextptr);
            cpts[cpt]=gen(cls);
        }
    } else if (order_size>0) { // use implicit differentiation instead of Lagrange multipliers
        vecteur gr,taylor_terms,a(nv),cpt_arr(nv);
        ipdiff ipd(f,g,vars,contextptr);
        ipd.gradient(gr);
        matrice cv,hess;
        vecteur eqv=subst(mergevecteur(gr,g),vars,tmpvars,false,contextptr);
        if (initial.empty())
            cv=solve2(eqv,tmpvars,contextptr);
        else {
            vecteur fsol(*_fsolve(makesequence(eqv,tmpvars,initial),contextptr)._VECTptr);
            if (!fsol.empty())
                cv.push_back(fsol);
        }
        if (cv.empty())
            return;
        if (nv==1) {
            gen d,x=vars.front();
            for (const_iterateur it=cv.begin();it!=cv.end();++it) {
                gen &x0=it->_VECTptr->front();
                cls=_CPCLASS_UNDECIDED;
                for (int k=2;k<=order_size;++k) {
                    d=_simplify(subst(_derive(makesequence(f,x,k),contextptr),x,x0,false,contextptr),contextptr);
                    if (is_zero(d))
                        continue;
                    if ((k%2)!=0)
                        cls=_CPCLASS_SADDLE;
                    else cls=is_strictly_positive(d,contextptr)?_CPCLASS_MIN:_CPCLASS_MAX;
                    break;
                }
                cpts[x0]=gen(cls);
            }
        } else {
            vecteur fvars(vars);
            fvars.resize(n);
            ipd.hessian(hess);
            for (int i=0;i<nv;++i) {
                a[i]=make_idnt("a",i);
            }
            for (const_iterateur it=cv.begin();it!=cv.end();++it) {
                for (int j=0;j<nv;++j) {
                    cpt_arr[arr[j]]=it->_VECTptr->at(j);
                }
                cpt_arr=subst(*_simplify(cpt_arr,contextptr)._VECTptr,tmpvars,vars,false,contextptr);
                gen &cpt_class=cpts[cpt_arr];
                if (order_size==1 || !is_zero(cpt_class))
                    continue;
                cls=_CPCLASS_UNDECIDED;
                // apply the second partial derivative test
                matrice H=*_evalf(subst(hess,vars,*it,false,contextptr),contextptr)._VECTptr;
                vecteur eigvals(*_eigenvals(H,contextptr)._VECTptr);
                gen e(0);
                for (const_iterateur et=eigvals.begin();et!=eigvals.end();++et) {
                    if (is_zero(*et)) {
                        cls=_CPCLASS_UNDECIDED;
                        break;
                    } else if (is_zero(e)) {
                        e=*et;
                        cls=is_positive(e,contextptr)?_CPCLASS_MIN:_CPCLASS_MAX;
                    } else if (is_strictly_positive(-e*(*et),contextptr))
                        cls=_CPCLASS_SADDLE;
                }
                // if that's not enough, use the higher derivatives
                if (cls==_CPCLASS_UNDECIDED && order_size>=2) {
                    for (int k=2;k<=order_size;++k) {
                        if (int(taylor_terms.size())<k-1)
                            taylor_terms.push_back(ipd.taylor_term(a,k));
                        if (is_zero(taylor_terms.back()))
                            break;
                        gen p=expand(subst(taylor_terms[k-2],a,*it,false,contextptr),contextptr);  // homogeneous poly
                        if (is_zero(p))
                            continue;
                        gen pmin,pmax;
                        gen sphere(-1);
                        for (int j=0;j<n;++j) {
                            sphere+=pow(vars[j]-it->_VECTptr->at(j),2);
                        }
                        vecteur gp,hp(1,sphere);
                        if (global_extrema(p,gp,hp,fvars,pmin,pmax,contextptr).empty())
                            break;
                        if (is_zero(pmin) && is_zero(pmax)) // p is nullpoly
                            continue;
                        if ((k%2)!=0 || (is_strictly_positive(-pmin,contextptr) && is_strictly_positive(pmax,contextptr)))
                            cls=_CPCLASS_SADDLE;
                        else if (is_strictly_positive(pmin,contextptr))
                            cls=_CPCLASS_MIN;
                        else if (is_strictly_positive(-pmax,contextptr))
                            cls=_CPCLASS_MAX;
                        else if (is_zero(pmin))
                            cls=_CPCLASS_POSSIBLE_MIN;
                        else if (is_zero(pmax))
                            cls=_CPCLASS_POSSIBLE_MAX;
                        break;
                    }
                }
                cpt_class=gen(cls);
            }
        }
    }
}

/*
 * 'extrema' attempts to find all points of strict local minima/maxima of a
 * smooth (uni/multi)variate function subject to one or more equality
 * constraints. The implemented method uses Lagrange multipliers.
 *
 * Usage
 * ^^^^^
 *     extrema(expr,[constr],vars,[order_size])
 *
 * Parameters
 * ^^^^^^^^^^
 *   - expr                  : differentiable expression
 *   - constr (optional)     : (list of) equality constraint(s)
 *   - vars                  : (list of) problem variable(s)
 *   - order_size (optional) : specify 'order_size=<nonnegative integer>' to
 *                             bound the order of the derivatives being
 *                             inspected when classifying the critical points
 *
 * The number of constraints must be less than the number of variables. When
 * there are more than one constraint/variable, they must be specified in
 * form of list.
 *
 * Variables may be specified with symbol, e.g. 'var', or by using syntax
 * 'var=a..b', which restricts the variable 'var' to the open interval (a,b),
 * where a and b are real numbers or +/-infinity. If variable list includes a
 * specification of initial point, such as, for example, [x=1,y=0,z=2], then
 * numeric solver is activated to find critical point in the vicinity of the
 * given point. In this case, the single critical point, if found, is examined.
 *
 * The function attempts to find the critical points in exact form, if the
 * parameters of the problem are all exact. It works best for problems in which
 * the gradient of lagrangian function consists of rational expressions. The
 * result may be inexact, however, if exact solutions could not be obtained.
 *
 * For classifying critical points, the bordered hessian method is used first.
 * It is only a second order test, so it may be inconclusive in some cases. In
 * these cases function looks at higher-order derivatives, up to order
 * specified by 'order_size' option (the extremum test). Set 'order_size' to 1
 * to use only the bordered hessian test or 0 to output critical points without
 * attempting to classify them. Setting 'order_size' to 2 or higher will
 * activate checking for saddle points and inspecting higher derivatives (up to
 * 'order_size') to determine the nature of some or all unclassified critical
 * points. By default 'order_size' equals to 5.
 *
 * The return value is a sequence with two elements: list of strict local
 * minima and list of strict local maxima. If only critical points are
 * requested (by setting 'order_size' to 0), the output consists of a single
 * list, as no classification was attempted. For univariate problems the points
 * are real numbers, while for multivariate problems they are specified as
 * lists of coordinates, so "lists of points" are in fact matrices with rows
 * corresponding to points in multivariate cases, i.e. vectors in univariate
 * cases.
 *
 * The function prints out information about saddle/inflection points and also
 * about critical points for which no decision could be made, so that the user
 * can inspect candidates for local extrema by plotting the graph, for example.
 *
 * Examples
 * ^^^^^^^^
 * extrema(-2*cos(x)-cos(x)^2,x)
 *    >> [0],[pi]
 * extrema((x^3-1)^4/(2x^3+1)^4,x=0..inf)
 *    >> [1],[]
 * extrema(x/2-2*sin(x/2),x=-12..12)
 *    >> [2*pi/3,-10*pi/3],[10*pi/3,-2*pi/3]
 * extrema(x-ln(abs(x)),x)
 *    >> [1],[]
 * assume(a>=0);extrema(x^2+a*x,x)
 *    >> [-a/2],[]
 * extrema(x^7+3x^6+3x^5+x^4+2x^2-x,x)
 *    >> [0.225847362349],[-1.53862319761]
 * extrema((x^2+x+1)/(x^4+1),x)
 *    >> [],[0.697247087784]
 * extrema(x^2+exp(-x),x)
 *    >> [0.351733711249],[]
 * extrema(exp(-x)*ln(x),x)
 *    >> [],[1.76322283435]
 * extrema(tan(x)*(x^3-5x^2+1),x=-0.5)
 *    >> [-0.253519032024],[]
 * extrema(tan(x)*(x^3-5x^2+1),x=0.5)
 *    >> [],[0.272551772027]
 * extrema(exp(x^2-2x)*ln(x)*ln(1-x),x=0.5)
 *    >> [],[0.277769149124]
 * extrema(ln(2+x-sin(x)^2),x=0..2*pi)
 *    >> [],[] (needed to compute third derivative to drop critical points pi/4 and 5pi/4)
 * extrema(x^3-2x*y+3y^4,[x,y])
 *    >> [[12^(1/5)/3,(12^(1/5))^2/6]],[]
 * extrema((2x^2-y)*(y-x^2),[x,y])  //Peano surface
 *    >> [],[] (saddle point at origin)
 * extrema(5x^2+3y^2+x*z^2-z*y^2,[x,y,z])
 *    >> [],[] (possible local minimum at origin, in fact saddle)
 * extrema(3*atan(x)-2*ln(x^2+y^2+1),[x,y])
 *    >> [],[[3/4,0]]
 * extrema(x*y,x+y=1,[x,y])
 *    >> [],[[1/2,1/2]]
 * extrema(sqrt(x*y),x+y=2,[x,y])
 *    >> [],[[1,1]]
 * extrema(x*y,x^3+y^3=16,[x,y])
 *    >> [],[[2,2]]
 * extrema(x^2+y^2,x*y=1,[x=0..inf,y=0..inf])
 *    >> [[1,1]],[]
 * extrema(ln(x*y^2),2x^2+3y^2=8,[x,y])
 *    >> [],[[2*sqrt(3)/3,-4/3],[-2*sqrt(3)/3,-4/3],[2*sqrt(3)/3,4/3],[-2*sqrt(3)/3,4/3]]
 * extrema(y^2+4y+2x-x^2,x+2y=2,[x,y])
 *    >> [],[[-2/3,4/3]]
 * assume(a>0);extrema(x/a^2+a*y^2,x+y=a,[x,y])
 *    >> [[(2*a^4-1)/(2*a^3),1/(2*a^3)]],[]
 * extrema(6x+3y+2z,4x^2+2y^2+z^2=70,[x,y,z])
 *    >> [[-3,-3,-4]],[[3,3,4]]
 * extrema(x*y*z,x+y+z=1,[x,y,z])
 *    >> [],[[1/3,1/3,1/3]]
 * extrema(x*y^2*z^2,x+y+z=5,[x,y,z])
 *    >> [],[[1,2,2]]
 * extrema(4y-2z,[2x-y-z=2,x^2+y^2=1],[x,y,z])
 *    >> [[2*sqrt(13)/13,-3*sqrt(13)/13,(7*sqrt(13)-26)/13]],
 *       [[-2*sqrt(13)/13,3*sqrt(13)/13,(-7*sqrt(13)-26)/13]]
 * extrema((x-3)^2+(y-1)^2+(z-1)^2,x^2+y^2+z^2=4,[x,y,z])
 *    >> [[6*sqrt(11)/11,2*sqrt(11)/11,2*sqrt(11)/11]],
 *       [[-6*sqrt(11)/11,-2*sqrt(11)/11,-2*sqrt(11)/11]]
 * extrema(x+3y-z,2x^2+y^2=z,[x,y,z])
 *    >> [],[[1/4,3/2,19/8]]
 * extrema(2x*y+2y*z+x*z,x*y*z=4,[x,y,z])
 *    >> [[2,1,2]],[]
 * extrema(x+y+z,[x^2+y^2=1,2x+z=1],[x,y,z])
 *    >> [[sqrt(2)/2,-sqrt(2)/2,-sqrt(2)+1]],[[-sqrt(2)/2,sqrt(2)/2,sqrt(2)+1]]
 * assume(a>0);extrema(x+y+z,[y^2-x^2=a,x+2z=1],[x,y,z])
 *    >> [[-sqrt(3)*sqrt(a)/3,2*sqrt(3)*sqrt(a)/3,(sqrt(3)*sqrt(a)+3)/6]],
 *       [[sqrt(3)*sqrt(a)/3,-2*sqrt(3)*sqrt(a)/3,(-sqrt(3)*sqrt(a)+3)/6]]
 * extrema((x-u)^2+(y-v)^2,[x^2/4+y^2/9=1,(u-3)^2+(v+5)^2=1],[u,v,x,y])
 *    >> [[2.35433932354,-4.23637555425,0.982084902545,-2.61340692712]],
 *       [[3.41406613851,-5.91024679428,-0.580422508346,2.87088778158]]
 * extrema(x2^6+x1^3+4x1+4x2,x1^5+x2^4+x1+x2=0,[x1,x2])
 *    >> [[-0.787457596325,0.758772338924],[-0.784754836317,-1.23363062357]],
 *       [[0.154340184382,-0.155005038065]]
 * extrema(x*y,-2x^3+15x^2*y+11y^3-24y=0,[x,y])
 *    >> [[sqrt(17)*sqrt(3*sqrt(33)+29)/17,sqrt(-15*sqrt(33)+127)*sqrt(187)/187],
 *        [-sqrt(17)*sqrt(3*sqrt(33)+29)/17,-sqrt(-15*sqrt(33)+127)*sqrt(187)/187],
 *        [sqrt(-3*sqrt(33)+29)*sqrt(17)/17,-sqrt(15*sqrt(33)+127)*sqrt(187)/187],
 *        [-sqrt(-3*sqrt(33)+29)*sqrt(17)/17,sqrt(15*sqrt(33)+127)*sqrt(187)/187]],
 *       [[1,1],[-1,-1],[0,0]]
 * extrema(x2^4-x1^4-x2^8+x1^10,[x1,x2],order_size=1)
 *    >> [[0,0],[0,-(1/2)^(1/4)],[0,(1/2)^(1/4)],[-(2/5)^(1/6),0],[-(2/5)^(1/6),-(1/2)^(1/4)],
 *        [-(2/5)^(1/6),(1/2)^(1/4)],[(2/5)^(1/6),0],[(2/5)^(1/6),-(1/2)^(1/4)],[(2/5)^(1/6),(1/2)^(1/4)]]
 * extrema(x2^4-x1^4-x2^8+x1^10,[x1,x2])
 *    >> [[(2/5)^(1/6),0],[-(2/5)^(1/6),0]],[[0,(1/2)^(1/4)],[0,-(1/2)^(1/4)]]
 * extrema(x2^6+x1^3+4x1+4x2,x1^5+x2^4+x1+x2=0,[x1,x2])
 *    >> [[-0.787457596325,0.758772338924],[-0.784754836317,-1.23363062357]],
 *       [[0.154340184382,-0.155005038065]]
 * extrema(x2^6+x1^3+2x1^2-x2^2+4x1+4x2,x1^5+x2^4+x1+x2=0,[x1,x2])
 *    >> [[-0.662879934158,-1.18571027742],[0,0]],[[0.301887394815,-0.314132376868]]
 * extrema(3x^2-2x*y+y^2-8y,[x,y])
 *    >> [[2,6]],[]
 * extrema(x^3+3x*y^2-15x-12y,[x,y])
 *    >> [[2,1]],[[-2,-1]]
 * extrema(4x*y-x^4-y^4,[x,y])
 *    >> [],[[1,1],[-1,-1]]
 * extrema(x*sin(y),[x,y])
 *    >> [],[]
 * extrema(x^4+y^4,[x,y])
 *    >> [[0,0]],[]
 * extrema(x^3*y-x*y^3,[x,y])  //dog saddle at origin
 *    >> [],[]
 * extrema(x^2+y^2+z^2,x^4+y^4+z^4=1,[x,y,z])
 *    >> [[0,0,1],[0,0,-1]],[]
 * extrema(3x+3y+8z,[x^2+z^2=1,y^2+z^2=1],[x,y,z])
 *    >> [[-3/5,-3/5,-4/5]],[[3/5,3/5,4/5]]
 * extrema(2x^2+y^2,x^4-x^2+y^2=5,[x,y])
 *    >> [[0,-sqrt(5)],[0,sqrt(5)]],
 *       [[-1/2*sqrt(6),-1/2*sqrt(17)],[-1/2*sqrt(6),1/2*sqrt(17)],
 *        [1/2*sqrt(6),-1/2*sqrt(17)],[1/2*sqrt(6),1/2*sqrt(17)]]
 * extrema((3x^4-4x^3-12x^2+18)/(12*(1+4y^2)),[x,y])
 *    >> [[2,0]],[[0,0]]
 * extrema(x-y+z,[x^2+y^2+z^2=1,x+y+2z=1],[x,y,z])
 *    >> [[(-2*sqrt(70)+7)/42,(4*sqrt(70)+7)/42,(-sqrt(70)+14)/42]],
 *       [[(2*sqrt(70)+7)/42,(-4*sqrt(70)+7)/42,(sqrt(70)+14)/42]]
 * extrema(ln(x)+2*ln(y)+3*ln(z)+4*ln(u)+5*ln(v),x+y+z+u+v=1,[x,y,z,u,v])
 *    >> [],[[1/15,2/15,1/5,4/15,1/3]]
 * extrema(x*y*z,-2x^3+15x^2*y+11y^3-24y=0,[x,y,z])
 *    >> [],[]
 * extrema(x+y-exp(x)-exp(y)-exp(x+y),[x,y])
 *    >> [],[[ln(-1/2*(-sqrt(5)+1)),ln(-1/2*(-sqrt(5)+1))]]
 * extrema(x^2*sin(y)-4*x,[x,y])    // has two saddle points
 *    >> [],[]
 * extrema((1+y*sinh(x))/(1+y^2+tanh(x)^2),[x,y])
 *    >> [],[[0,0]]
 * extrema((1+y*sinh(x))/(1+y^2+tanh(x)^2),y=x^2,[x,y])
 *    >> [[1.42217627369,2.02258535346]],[[8.69443642205e-16,7.55932246971e-31]]
 * extrema(x^2*y^2,[x^3*y-2*x^2+3x-2y^2=0],[x=0..inf,y])
 *    >> [[3/2,0]],[[0.893768095046,-0.5789326693514]]
 */
gen _extrema(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT)
        return gentypeerr(contextptr);
    vecteur &gv=*g._VECTptr,constr;
    int order_size=5; // will not compute the derivatives of order higher than 'order_size'
    int ngv=gv.size();
    if (gv.back()==at_lagrange) {
        order_size=0; // use Lagrange method
        --ngv;
    } else if (gv.back().is_symb_of_sommet(at_equal)) {
        vecteur &v=*gv.back()._SYMBptr->feuille._VECTptr;
        if (v[0]==at_order_size && is_integer(v[1])) {
            if ((order_size=v[1].val)<1)
                return gensizeerr("Expected a positive integer,");
            --ngv;
        }
    }
    if (ngv<2 || ngv>3)
        return gensizeerr("Wrong number of input arguments,");
    // get the variables
    int nv;
    vecteur vars,ineq,initial;
    // parse variables and their ranges, if given
    if ((nv=parse_varlist(gv[ngv-1],vars,ineq,initial,contextptr))==0)
        return gentypeerr("Failed to parse variables,");
    if (!initial.empty() && int(initial.size())<nv)
        return gendimerr(contextptr);
    if (ngv==3) {
        // get the constraints
        if (gv[1].type==_VECT)
            constr=*gv[1]._VECTptr;
        else
            constr=vecteur(1,gv[1]);
    }
    if (order_size==0 && constr.empty())
        return gensizeerr("At least one constraint is required for Lagrange method,");
    for (iterateur it=constr.begin();it!=constr.end();++it) {
        if (it->is_symb_of_sommet(at_equal))
            *it=equal2diff(*it);
    }
    ipdiff::ivectors arrs;
    if (order_size>0 && !constr.empty()) {
        matrice J(jacobian(constr,vars,contextptr));
        if (constr.size()>=vars.size() || _rank(J,contextptr).val<int(constr.size()))
            return gendimerr("Too many constraints,");
        vars_arrangements(J,arrs,contextptr);
    } else {
        ipdiff::ivector arr(nv);
        for (int i=0;i<nv;++i) {
            arr[i]=i;
        }
        arrs.push_back(arr);
    }
    gen_map cpts;
    vecteur tmp_vars(vars.size());
    /* iterate through all possible variable arrangements */
    for (ipdiff::ivectors::const_iterator ait=arrs.begin();ait!=arrs.end();++ait) {
        const ipdiff::ivector &arr=*ait;
        for (ipdiff::ivector::const_iterator it=arr.begin();it!=arr.end();++it) {
            tmp_vars[it-arr.begin()]=vars[*it];
        }
        find_local_extrema(cpts,gv[0],constr,tmp_vars,arr,ineq,initial,order_size,contextptr);
    }
    if (order_size==1) { // return the list of critical points
        vecteur cv;
        for (gen_map::const_iterator it=cpts.begin();it!=cpts.end();++it) {
            cv.push_back(it->first);
        }
        return cv;
    }
    // return sequence of minima and maxima in separate lists and report non- or possible extrema
    vecteur minv,maxv;
    for (gen_map::const_iterator it=cpts.begin();it!=cpts.end();++it) {
        gen dispt(nv==1?symb_equal(vars[0],it->first):_zip(makesequence(at_equal,vars,it->first),contextptr));
        switch(it->second.val) {
        case _CPCLASS_MIN:
            minv.push_back(it->first);
            break;
        case _CPCLASS_MAX:
            maxv.push_back(it->first);
            break;
        case _CPCLASS_SADDLE:
            *logptr(contextptr) << dispt << (nv==1?": inflection point":": saddle point") << endl;
            break;
        case _CPCLASS_POSSIBLE_MIN:
            *logptr(contextptr) << dispt << ": possible local minimum" << endl;
            break;
        case _CPCLASS_POSSIBLE_MAX:
            *logptr(contextptr) << dispt << ": possible local maximum" << endl;
            break;
        case _CPCLASS_UNDECIDED:
            *logptr(contextptr) << dispt << ": unclassified critical point" << endl;
            break;
        }
    }
    return makesequence(minv,maxv);
}
static const char _extrema_s []="extrema";
static define_unary_function_eval (__extrema,&_extrema,_extrema_s);
define_unary_function_ptr5(at_extrema,alias_at_extrema,&__extrema,0,true)

/*
 * Compute the value of expression f(var) (or |f(var)| if 'absolute' is true)
 * for var=a.
 */
gen compf(const gen &f,identificateur &x,gen &a,bool absolute,GIAC_CONTEXT) {
    gen val(subst(f,x,a,false,contextptr));
    return _evalf(absolute?_abs(val,contextptr):val,contextptr);
}

/*
 * find zero of expression f(x) for x in [a,b] using Brent solver
 */
gen find_zero(const gen &f,identificateur &x,gen &a,gen &b,GIAC_CONTEXT) {
    gen I(symb_interval(a,b));
    gen var(symb_equal(x,I));
    vecteur sol(*_fsolve(makesequence(f,var,_BRENT_SOLVER),contextptr)._VECTptr);
    return sol.empty()?(a+b)/2:sol[0];
}

/*
 * Find point of maximum/minimum of unimodal expression f(x) in [a,b] using the
 * golden-section search.
 */
gen find_peak(const gen &f,identificateur &x,gen &a_orig,gen &b_orig,GIAC_CONTEXT) {
    gen a(a_orig),b(b_orig);
    gen c(b-(b-a)/GOLDEN_RATIO),d(a+(b-a)/GOLDEN_RATIO);
    while (is_strictly_greater(_abs(c-d,contextptr),epsilon(contextptr),contextptr)) {
        gen fc(compf(f,x,c,true,contextptr)),fd(compf(f,x,d,true,contextptr));
        if (is_strictly_greater(fc,fd,contextptr))
            b=d;
        else
            a=c;
        c=b-(b-a)/GOLDEN_RATIO;
        d=a+(b-a)/GOLDEN_RATIO;
    }
    return (a+b)/2;
}

/*
 * Compute n Chebyshev nodes in [a,b].
 */
vecteur chebyshev_nodes(gen &a,gen &b,int n,GIAC_CONTEXT) {
    vecteur nodes(1,a);
    for (int i=1;i<=n;++i) {
        nodes.push_back(_evalf((a+b)/2+(b-a)*symbolic(at_cos,((2*i-1)*cst_pi/(2*n)))/2,contextptr));
    }
    nodes.push_back(b);
    return *_sort(nodes,contextptr)._VECTptr;
}

/*
 * Implementation of Remez method for minimax polynomial approximation of a
 * continuous bounded function, which is not necessary differentiable in all
 * points of (a,b).
 *
 * Source: http://homepages.rpi.edu/~tasisa/remez.pdf
 *
 * Usage
 * ^^^^^
 *      minimax(expr,var=a..b,n,[opts])
 *
 * Parameters
 * ^^^^^^^^^^
 *      - expr            : expression to be approximated on [a,b]
 *      - var             : variable
 *      - a,b             : bounds of the function domain
 *      - n               : degree of the minimax approximation polynomial
 *      - opts (optional) : sequence of options
 *
 * This function uses 'compf', 'find_zero' and 'find_peak'. It does not use
 * derivatives to determine points of local extrema of error function, but
 * instead implements the golden search algorithm to find these points in the
 * exchange phase of Remez method.
 *
 * The returned polynomial may have degree lower than n, because the latter is
 * decremented during execution of the algorithm if there is no unique solution
 * for coefficients of a nth degree polynomial. After each decrement, the
 * algorithm is restarted. If the degree of resulting polynomial is m<n, it
 * means that polynomial of degree between m and n cannot be obtained by using
 * this implementation.
 *
 * In 'opts' one may specify 'limit=<posint>' which limits the number of
 * iterations. By default, it is unlimited.
 *
 * Be aware that, in some cases, the result with high n may be unsatisfying,
 * producing larger error than the polynomials for smaller n. This happens
 * because of the rounding errors. Nevertheless, a good approximation of an
 * "almost" smooth function can usually be obtained with n less than 30. Highly
 * oscillating functions containing sharp cusps and spikes will probably be
 * approximated poorly.
 *
 * Examples
 * ^^^^^^^^
 * minimax(x*exp(-x),x=0..10,24)
 * minimax(x*sin(x),x=0..10,25)
 * minimax(ln(2+x-sin(x)^2),x=0..2*pi,20)
 * minimax(cos(x^2-x+1),x=-2..2,40)
 * minimax(atan(x),x=-5..5,25)
 * minimax(tanh(sin(9x)),x=-1/2..1/2,40)
 * minimax(abs(x),x=-1..1,20)
 * minimax(abs(x)*sqrt(abs(x)),x=-2..2,15)
 * minimax(min(1/cosh(3*sin(10x)),sin(9x)),x=-0.3..0.4,25)
 * minimax(when(x==0,0,exp(-1/x^2)),x=-1..1,30)
 */
gen _minimax(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT)
        return gentypeerr(contextptr);
    vecteur &gv=*g._VECTptr;
    if (gv.size()<3)
        return gensizeerr(contextptr);
    if (!gv[1].is_symb_of_sommet(at_equal) || !is_integer(gv[2]))
        return gentypeerr(contextptr);
    // detect parameters
    vecteur s(*gv[1]._SYMBptr->feuille._VECTptr);
    if (s[0].type!=_IDNT || !s[1].is_symb_of_sommet(at_interval))
        return gentypeerr((contextptr));
    identificateur x(*s[0]._IDNTptr);
    s=*s[1]._SYMBptr->feuille._VECTptr;
    gen a(_evalf(s[0],contextptr)),b(_evalf(s[1],contextptr));
    if (!is_strictly_greater(b,a,contextptr))
        return gentypeerr(contextptr);
    gen &f=gv[0];
    int n=gv[2].val;
    gen threshold(1.02);  // threshold for stopping criterion
    // detect options
    int limit=0;
    //bool poly=true;
    for (const_iterateur it=gv.begin()+3;it!=gv.end();++it) {
        if (it->is_symb_of_sommet(at_equal)) {
            vecteur &p=*it->_SYMBptr->feuille._VECTptr;
            if (p[0]==at_limit) {
                if (!is_integer(p[1]) || !is_strictly_positive(p[1],contextptr))
                    return gentypeerr(contextptr);
                limit=p[1].val;
            }
        }
        else if (is_integer(*it)) {
            switch (it->val) {
//          case _FRAC:
//              poly=false;
//              break;
            }
        }
    }
    // create Chebyshev nodes to start with
    vecteur nodes(chebyshev_nodes(a,b,n,contextptr));
    gen p,best_p,best_emax,emax,emin;
    int iteration_count=0;
    while (true) { // iterate the algorithm
        iteration_count++;
        if (n<1 || (limit>0 && iteration_count>limit))
            break;
        // compute polynomial p
        matrice m;
        vecteur fv;
        for (int i=0;i<n+2;++i) {
            fv.push_back(_evalf(subst(f,x,nodes[i],false,contextptr),contextptr));
            vecteur r;
            for (int j=0;j<n+1;++j) {
                r.push_back(j==0?gen(1):pow(nodes[i],j));
            }
            r.push_back(pow(gen(-1),i));
            m.push_back(r);
        }
        vecteur sol(*_linsolve(makesequence(m,fv),contextptr)._VECTptr);
        if (!_lname(sol,contextptr)._VECTptr->empty()) {
            // Solution is not unique, it contains a symbol.
            // Decrease n and start over.
            nodes=chebyshev_nodes(a,b,--n,contextptr);
            continue;
        }
        p=gen(0);
        for (int i=0;i<n+1;++i) {
            p+=sol[i]*pow(x,i);
        }
        // compute the error function and its zeros
        gen e(f-p);
        vecteur zv(1,a);
        for (int i=0;i<n+1;++i) {
            zv.push_back(find_zero(e,x,nodes[i],nodes[i+1],contextptr));
        }
        zv.push_back(b);
        // remez exchange:
        // determine points of local extrema of error function e
        vecteur ev(n+2,0);
        for (int i=0;i<n+2;++i) {
            if (i>0 && i<n+1) {
                nodes[i]=find_peak(e,x,zv[i],zv[i+1],contextptr);
                ev[i]=compf(e,x,nodes[i],true,contextptr);
                continue;
            }
            gen e1(compf(e,x,zv[i],true,contextptr)),e2(compf(e,x,zv[i+1],true,contextptr));
            if (is_greater(e1,e2,contextptr)) {
                nodes[i]=zv[i];
                ev[i]=e1;
            }
            else {
                nodes[i]=zv[i+1];
                ev[i]=e2;
            }
        }
        // compute minimal and maximal absolute error
        emin=_min(ev,contextptr);
        emax=_max(ev,contextptr);
        if (is_exactly_zero(best_emax) || is_strictly_greater(best_emax,emax,contextptr)) {
            best_p=p;
            best_emax=emax;
        }
        // emin >= E is required to continue, also check
        // if the threshold is reached, i.e. the difference
        // between emax and emin is at least fifty times
        // smaller than emax
        if (is_strictly_greater(sol.back(),emin,contextptr) ||
                is_greater(threshold*emin,emax,contextptr)) {
            break;
        }
    }
    *logptr(contextptr) << "max. absolute error: " << best_emax << endl;
    return best_p;
}
static const char _minimax_s []="minimax";
static define_unary_function_eval (__minimax,&_minimax,_minimax_s);
define_unary_function_ptr5(at_minimax,alias_at_minimax,&__minimax,0,true)

/*
 * TPROB CLASS IMPLEMENTATION
 */

tprob::tprob(const vecteur &s,const vecteur &d,const gen &m,GIAC_CONTEXT) {
    eps=exact(epsilon(contextptr)/2,contextptr);
    ctx=contextptr;
    supply=s;
    demand=d;
    M=m;
}

/*
 * North-West-Corner method giving the initial feasible solution to the
 * transportatiom problem with given supply and demand vectors. It handles
 * degeneracy cases (assignment problems, for example, always have degenerate
 * solutions).
 */
void tprob::north_west_corner(matrice &feas) {
    feas.clear();
    int m=supply.size(),n=demand.size();
    for (int k=0;k<m;++k) {
        feas.push_back(vecteur(n,0));
    }
    int i=0,j=0;
    while (i<m && j<n) {
        const gen &s=supply[i],&d=demand[j];
        gen u,v;
        for (int k=0;k<i;++k) {
            v+=_epsilon2zero(feas[k][j],ctx);
        }
        for (int k=0;k<j;++k) {
            u+=_epsilon2zero(feas[i][k],ctx);
        }
        gen a=min(s-u,d-v,ctx);
        feas[i]._VECTptr->at(j)=a;
        int k=i+j;
        if (u+a==s)
            ++i;
        if (v+a==d)
            ++j;
        if (i<m && j<n && i+j-k==2) // avoid degeneracy
            feas[i-1]._VECTptr->at(j)=eps;
    }
}

/*
 * Stepping stone path method for determining a closed path "jumping" from one
 * positive element of X to another in the same row or column.
 */
tprob::ipairs tprob::stepping_stone_path(ipairs &path_orig,const matrice &X) {
    ipairs path(path_orig);
    int I=path.back().first,J=path.back().second;
    int m=X.size(),n=X.front()._VECTptr->size();
    if (path.size()>1 && path.front().second==J)
        return path;
    bool hrz=path.size()%2==1;
    for (int i=0;i<(hrz?n:m);++i) {
        int cnt=0;
        for (ipairs::const_iterator it=path.begin();it!=path.end();++it) {
            if ((hrz && it->second==i) || (!hrz && it->first==i))
                ++cnt;
        }
        if (cnt<2 && !is_exactly_zero(X[hrz?I:i][hrz?i:J])) {
            path.push_back(make_pair(hrz?I:i,hrz?i:J));
            ipairs fullpath(stepping_stone_path(path,X));
            if (!fullpath.empty())
                return fullpath;
            path.pop_back();
        }
    }
    return ipairs(0);
}

/*
 * Implementation of MODI (modified ditribution) method. It handles degenerate
 * solutions if they appear during the process.
 */
void tprob::modi(const matrice &P_orig,matrice &X) {
    matrice P(P_orig);
    int m=X.size(),n=X.front()._VECTptr->size();
    vecteur u(m),v(n);
    if (M.type==_IDNT) {
        gen largest(0);
        for (int i=0;i<m;++i) {
            for (int j=0;j<n;++j) {
                if (is_greater(X[i][j],largest,ctx))
                    largest=X[i][j];
            }
        }
        P=subst(P,M,100*largest,false,ctx);
    }
    for (int i=0;i<m;++i) {
        u[i]=i==0?gen(0):make_idnt("u",i);
    }
    for (int j=0;j<n;++j) {
        v[j]=make_idnt("v",j);
    }
    vecteur vars(mergevecteur(vecteur(u.begin()+1,u.end()),v));
    while (true) {
        vecteur eqv;
        for (int i=0;i<m;++i) {
            for (int j=0;j<n;++j) {
                if (!is_exactly_zero(X[i][j]))
                    eqv.push_back(u[i]+v[j]-P[i][j]);
            }
        }
        vecteur sol(*_linsolve(makesequence(eqv,vars),ctx)._VECTptr);
        vecteur U(1,0),V(sol.begin()+m-1,sol.end());
        U=mergevecteur(U,vecteur(sol.begin(),sol.begin()+m-1));
        gen cmin(0);
        bool optimal=true;
        int I,J;
        for (int i=0;i<m;++i) {
            for (int j=0;j<n;++j) {
                if (is_exactly_zero(X[i][j])) {
                    gen c(P[i][j]-U[i]-V[j]);
                    if (is_strictly_greater(cmin,c,ctx)) {
                        cmin=c;
                        optimal=false;
                        I=i;
                        J=j;
                    }
                }
            }
        }
        if (optimal)
            break;
        ipairs path;
        path.push_back(make_pair(I,J));
        path=stepping_stone_path(path,X);
        gen d(X[path.at(1).first][path.at(1).second]);
        for (ipairs::const_iterator it=path.begin()+3;it<path.end();it+=2) {
            d=min(d,X[it->first][it->second],ctx);
        }
        for (int i=0;i<int(path.size());++i) {
            gen &Xij=X[path.at(i).first]._VECTptr->at(path.at(i).second);
            gen x(Xij+(i%2?-d:d));
            bool has_zero=false;
            for (ipairs::const_iterator it=path.begin();it!=path.end();++it) {
                if (is_exactly_zero(X[it->first][it->second])) {
                    has_zero=true;
                    break;
                }
            }
            if ((!is_exactly_zero(x) && is_strictly_greater(gen(1)/gen(2),x,ctx)) ||
                    (is_exactly_zero(x) && has_zero))
                x=eps;
            Xij=x;
        }
    }
    X=*exact(_epsilon2zero(_evalf(X,ctx),ctx),ctx)._VECTptr;
}

void tprob::solve(const matrice &cost_matrix,matrice &sol) {
    north_west_corner(sol);
    modi(cost_matrix,sol);
}

/*
 * END OF TPROB CLASS
 */

/*
 * Function 'tpsolve' solves a transportation problem using MODI method.
 *
 * Usage
 * ^^^^^
 *      tpsolve(supply,demand,cost_matrix)
 *
 * Parameters
 * ^^^^^^^^^^
 *      - supply      : source capacity (vector of m positive integers)
 *      - demand      : destination demand (vector of n positive integers)
 *      - cost_matrix : real matrix C=[c_ij] of type mXn where c_ij is cost of
 *                      transporting an unit from ith source to jth destination
 *                      (a nonnegative number)
 *
 * Supply and demand vectors should contain only positive integers. Cost matrix
 * must be consisted of nonnegative real numbers, which do not have to be
 * integers. There is a possibility of adding a certain symbol to cost matrix,
 * usually M, to indicate the "infinite cost", effectively forbidding the
 * transportation on a certain route. The notation of the symbol may be chosen
 * arbitrarily, but must be used consistently within a single problem.
 *
 * The return value is a sequence of total (minimal) cost and matrix X=[x_ij]
 * of type mXn where x_ij is equal to number of units which have to be shipped
 * from ith source to jth destination, for all i=1,2,..,m and j=1,2,..,n.
 *
 * This function uses 'north_west_corner' to determine initial feasible
 * solution and then applies MODI method to optimize it (function 'modi', which
 * uses 'stepping_stone_path'). Also, it is capable of handling degeneracy of
 * the initial solution and during iterations of MODI method.
 *
 * If the given problem is not balanced, i.e. if supply exceeds demand or vice
 * versa, dummy supply/demand points will be automatically added to the
 * problem, augmenting the cost matrix with zeros. Resulting matrix will not
 * contain dummy point.
 *
 * Examples
 * ^^^^^^^^
 * Balanced transportation problem:
 *  tpsolve([12,17,11],[10,10,10,10],[[500,750,300,450],[650,800,400,600],[400,700,500,550]])
 *      >> 2020,[[0,0,2,10],[0,9,8,0],[10,1,0,0]]
 * Non-balanced transportation problem:
 *  tpsolve([7,10,8,8,9,6],[9,6,12,8,10],[[36,40,32,43,29],[28,27,29,40,38],[34,35,41,29,31],[41,42,35,27,36],[25,28,40,34,38],[31,30,43,38,40]])
 *      >> [[0,0,2,0,5],[0,0,10,0,0],[0,0,0,0,5],[0,0,0,8,0],[9,0,0,0,0],[0,6,0,0,0]]
 * Transportation problem with forbidden routes:
 *  tpsolve([95,70,165,165],[195,150,30,45,75],[[15,M,45,M,0],[12,40,M,M,0],[0,15,25,25,0],[M,0,M,12,0]])
 *      >> [[20,0,0,0,75],[70,0,0,0,0],[105,0,30,30,0],[0,150,0,15,0]]
 * Assignment problem:
 *  tpsolve([1,1,1,1],[1,1,1,1],[[10,12,9,11],[5,10,7,8],[12,14,13,11],[8,15,11,9]])
 *      >> [[0,0,1,0],[1,0,0,0],[0,1,0,0],[0,0,0,1]]
 */
gen _tpsolve(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT)
        return gentypeerr(contextptr);
    vecteur &gv=*g._VECTptr;
    if (gv.size()<3)
        return gensizeerr(contextptr);
    if (gv[0].type!=_VECT || gv[1].type!=_VECT ||
            gv[2].type!=_VECT || !ckmatrix(*gv[2]._VECTptr))
        return gentypeerr(contextptr);
    vecteur supply(*gv[0]._VECTptr),demand(*gv[1]._VECTptr);
    matrice P(*gv[2]._VECTptr);
    vecteur sy(*_lname(P,contextptr)._VECTptr);
    int m=supply.size(),n=demand.size();
    if (sy.size()>1 || m!=int(P.size()) || n!=int(P.front()._VECTptr->size()))
        return gensizeerr(contextptr);
    gen M(sy.size()==1 && sy[0].type==_IDNT?sy[0]:0);
    gen ts(_sum(supply,contextptr)),td(_sum(demand,contextptr));
    if (ts!=td) {
        *logptr(contextptr) << "Warning: transportation problem is not balanced" << endl;
        if (is_greater(ts,td,contextptr)) {
            demand.push_back(ts-td);
            P=mtran(P);
            P.push_back(vecteur(m,0));
            P=mtran(P);
        }
        else {
            supply.push_back(td-ts);
            P.push_back(vecteur(n,0));
        }
    }
    matrice X;
    tprob tp(supply,demand,M,contextptr);
    tp.solve(P,X);
    if (is_strictly_greater(ts,td,contextptr)) {
        X=mtran(X);
        X.pop_back();
        X=mtran(X);
    }
    else if (is_strictly_greater(td,ts,contextptr))
        X.pop_back();
    gen cost(0);
    for (int i=0;i<m;++i) {
        for (int j=0;j<n;++j) {
            cost+=P[i][j]*X[i][j];
        }
    }
    return makesequence(cost,X);
}
static const char _tpsolve_s []="tpsolve";
static define_unary_function_eval (__tpsolve,&_tpsolve,_tpsolve_s);
define_unary_function_ptr5(at_tpsolve,alias_at_tpsolve,&__tpsolve,0,true)

gen compute_invdiff(int n,int k,vecteur &xv,vecteur &yv,map<tprob::ipair,gen> &invdiff,GIAC_CONTEXT) {
    tprob::ipair I=make_pair(n,k);
    assert(n<=k);
    gen res(invdiff[I]);
    if (!is_zero(res))
        return res;
    if (n==0)
        return invdiff[I]=yv[k];
    if (n==1)
        return invdiff[I]=(xv[k]-xv[0])/(yv[k]-yv[0]);
    gen d1(compute_invdiff(n-1,n-1,xv,yv,invdiff,contextptr));
    gen d2(compute_invdiff(n-1,k,xv,yv,invdiff,contextptr));
    return invdiff[I]=(xv[k]-xv[n-1])/(d2-d1);
}

gen thiele(int k,vecteur &xv,vecteur &yv,identificateur &var,map<tprob::ipair,gen> &invdiff,GIAC_CONTEXT) {
    if (k==int(xv.size()))
        return gen(0);
    gen phi(compute_invdiff(k,k,xv,yv,invdiff,contextptr));
    return (var-xv[k-1])/(phi+thiele(k+1,xv,yv,var,invdiff,contextptr));
}

/*
 * 'thiele' computes rational interpolation for the given list of points using
 * Thiele's method with continued fractions.
 *
 * Source: http://www.astro.ufl.edu/~kallrath/files/pade.pdf (see page 19)
 *
 * Usage
 * ^^^^^
 *      thiele(data,v)
 * or   thiele(data_x,data_y,v)
 *
 * Parameters
 * ^^^^^^^^^^
 *      - data      : list of points [[x1,y1],[x2,y2],...,[xn,yn]]
 *      - v         : identifier (may be any symbolic expression)
 *      - data_x    : list of x coordinates [x1,x2,...,xn]
 *      - data_y    : list of y coordinates [y1,y2,...,yn]
 *
 * The return value is an expression R(v), where R is rational interpolant of
 * the given set of points.
 *
 * Note that the interpolant may have singularities in
 * [min(data_x),max(data_x)].
 *
 * Example
 * ^^^^^^^
 * Function f(x)=(1-x^4)*exp(1-x^3) is sampled on interval [-1,2] in 13
 * equidistant points:
 *
 * data_x:=[-1,-0.75,-0.5,-0.25,0,0.25,0.5,0.75,1,1.25,1.5,1.75,2],
 * data_y:=[0.0,2.83341735599,2.88770329586,2.75030303645,2.71828182846,
 *          2.66568510781,2.24894558809,1.21863761951,0.0,-0.555711613283,
 *         -0.377871362418,-0.107135851128,-0.0136782294833]
 *
 * To obtain rational function passing through these points, input:
 *      thiele(data_x,data_y,x)
 * Output:
 *      (-1.55286115659*x^6+5.87298387514*x^5-5.4439152812*x^4+1.68655817708*x^3
 *       -2.40784868317*x^2-7.55954205222*x+9.40462512097)/(x^6-1.24295718965*x^5
 *       -1.33526268624*x^4+4.03629272425*x^3-0.885419321*x^2-2.77913222418*x+3.45976823393)
 */
gen _thiele(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT)
        return gentypeerr(contextptr);
    vecteur &gv=*g._VECTptr;
    if (gv.size()<2)
        return gensizeerr(contextptr);
    vecteur xv,yv;
    gen x;
    if (gv[0].type!=_VECT)
        return gentypeerr(contextptr);
    if (ckmatrix(gv[0])) {
        matrice m(mtran(*gv[0]._VECTptr));
        if (m.size()!=2)
            return gensizeerr(contextptr);
        xv=*m[0]._VECTptr;
        yv=*m[1]._VECTptr;
        x=gv[1];
    }
    else {
        if (gv[1].type!=_VECT)
            return gentypeerr(contextptr);
        if (gv[0]._VECTptr->size()!=gv[1]._VECTptr->size())
            return gensizeerr(contextptr);
        xv=*gv[0]._VECTptr;
        yv=*gv[1]._VECTptr;
        x=gv[2];
    }
    gen var(x.type==_IDNT?x:identificateur(" x"));
    map<tprob::ipair,gen> invdiff;
    gen rat(yv[0]+thiele(1,xv,yv,*var._IDNTptr,invdiff,contextptr));
    if (x.type==_IDNT) {
        // detect singularities
        gen den(_denom(rat,contextptr));
        matrice sing;
        if (*_lname(den,contextptr)._VECTptr==vecteur(1,x)) {
            for (int i=0;i<int(xv.size())-1;++i) {
                gen y1(_evalf(subst(den,x,xv[i],false,contextptr),contextptr));
                gen y2(_evalf(subst(den,x,xv[i+1],false,contextptr),contextptr));
                if (is_positive(-y1*y2,contextptr))
                    sing.push_back(makevecteur(xv[i],xv[i+1]));
            }
        }
        if (!sing.empty()) {
            *logptr(contextptr) << "Warning, the interpolant has singularities in ";
            for (int i=0;i<int(sing.size());++i) {
                *logptr(contextptr) << "(" << sing[i][0] << "," << sing[i][1] << ")";
                if (i<int(sing.size())-1)
                    *logptr(contextptr) << (i<int(sing.size())-2?", ":" and ");
            }
            *logptr(contextptr) << endl;
        }
    }
    else
        rat=_simplify(subst(rat,var,x,false,contextptr),contextptr);
    return ratnormal(rat,contextptr);
}
static const char _thiele_s []="thiele";
static define_unary_function_eval (__thiele,&_thiele,_thiele_s);
define_unary_function_ptr5(at_thiele,alias_at_thiele,&__thiele,0,true)

void add_identifiers(const gen &source,vecteur &dest,GIAC_CONTEXT) {
    vecteur v(*_lname(source,contextptr)._VECTptr);
    for (const_iterateur it=v.begin();it!=v.end();++it) {
        if (!contains(dest,*it))
            dest.push_back(*it);
    }
    dest=*_sort(dest,contextptr)._VECTptr;
}

int indexof(const gen &g,const vecteur &v) {
    int n=v.size();
    for (int i=0;i<n;++i) {
        if (v.at(i)==g)
            return i;
    }
    return -1;
}

/*
 * 'nlpsolve' computes an optimum of a nonlinear objective function, subject to
 * nonlinear equality and inequality constraints, using the COBYLA algorithm.
 *
 * Syntax
 * ^^^^^^
 *      nlpsolve(objective, [constr], [bd], [opts])
 *
 * - objective: objective function
 * - constr: list of constraints
 * - bd: sequence of arguments of type x=a..b, where x is a problem variable
 *       and a,b are reals
 * - opts: one of:
 *       assume=nlp_nonnegative
 *       maximize[=true]
 *       nlp_initialpoint=[x1=a,x2=b,...]
 *       nlp_precision=real
 *       nlp_iterationlimit=intg
 *
 * If initial point is not given, it will be automatically generated. The given
 * point does not need to be feasible. Note that choosing a good initial point
 * is needed for obtaining a correct solution in some cases.
 *
 * Examples
 * ^^^^^^^^
 * (problems taken from:
 *      www.ai7.uni-bayreuth.de/test_problem_coll.pdf
 *      https://www.maplesoft.com/support/help/maple/view.aspx?path=Optimization%2FNLPSolve)
 *
 * nlpsolve(
 *  (x1-10)^3+(x2-20)^3,
 *  [(x1-5)^2+(x2-5)^2>=100,(x2-5)^2+(x1-6)^2<=82.81],
 *  nlp_initialpoint=[x1=20.1,x2=5.84]) // problem 19, using initial point
 * nlpsolve(sin(x1+x2)+(x1-x2)^2-1.5x1+2.5x2+1,x1=-1.5..4,x2=-3..3) // problem 5
 * nlpsolve(ln(1+x1^2)-x2,[(1+x1^2)^2+x2^2=4]) // problem 7
 * nlpsolve(
 *  x1,[x2>=exp(x1),x3>=exp(x2)],maximize=true,
 *  x1=0..100,x2=0..100,x3=0..10,nlp_initialpoint=[x1=1,x2=1.05,x3=2.9]) // problem 34 (modified)
 * nlpsolve(-x1*x2*x3,[72-x1-2x2-2x3>=0],x1=0..20,x2=0..11,x3=0..42) // problem 36
 * nlpsolve(2-1/120*x1*x2*x3*x4*x5,[x1<=1,x2<=2,x3<=3,x4<=4,x5<=5],assume=nlp_nonnegative) // problem 45
 * nlpsolve(sin(x)/x,x=1..30) // Maple computes wrong result for this example, at least on their web page
 * nlpsolve(x^3+2x*y-2y^2,x=-10..10,y=-10..10,nlp_initialpoint=[x=3,y=4],maximize) // Maple example
 * nlpsolve(w^3*(v-w)^2+(w-x-1)^2+(x-y-2)^2+(y-z-3)^2,[w+x+y+z<=5,3z+2v=3],assume=nlp_nonnegative) // Maple example
 * nlpsolve(sin(x)*Psi(x),x=1..20,nlp_initialpoint=[x=16]) // Maple example, needs an initial point
 */
gen _nlpsolve(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT || g._VECTptr->size() < 2)
        return gentypeerr(contextptr);
    vecteur &gv=*g._VECTptr;
    vecteur constr,vars,initp;
    gen &obj=gv.front();
    add_identifiers(obj,vars,contextptr);
    const_iterateur it=gv.begin();
    bool maximize=false;
    int maxiter=RAND_MAX;
    double eps=epsilon(contextptr);
    if (gv.at(1).type==_VECT) {
        constr=*gv.at(1)._VECTptr;
        add_identifiers(constr,vars,contextptr);
        ++it;
    }
    initp=vecteur(vars.size(),gen(1));
    while (++it!=gv.end()) {
        if (*it==at_maximize || (it->is_integer() && it->val==_NLP_MAXIMIZE))
            maximize=true;
        else if (it->is_symb_of_sommet(at_equal)) {
            gen &lh=it->_SYMBptr->feuille._VECTptr->front();
            gen &rh=it->_SYMBptr->feuille._VECTptr->back();
            if (lh==at_assume && rh.is_integer() && rh.val==_NLP_NONNEGATIVE) {
                for (const_iterateur jt=vars.begin();jt!=vars.end();++jt) {
                    constr.push_back(symbolic(at_inferieur_egal,makevecteur(gen(0),*jt)));
                }
            } else if (lh==at_maximize && rh.is_integer())
                maximize=(bool)rh.val;
            else if (lh.is_integer() && lh.val==_NLP_INITIALPOINT && rh.type==_VECT) {
                vecteur &pnt=*rh._VECTptr;
                for (const_iterateur jt=pnt.begin();jt!=pnt.end();++jt) {
                    gen var;
                    if (jt->is_symb_of_sommet(at_equal) &&
                            contains(vars,var=jt->_SYMBptr->feuille._VECTptr->front()))
                        initp.at(indexof(var,vars))=jt->_SYMBptr->feuille._VECTptr->back();
                }
            } else if (lh.is_integer() && lh.val==_NLP_ITERATIONLIMIT && rh.is_integer())
                maxiter=rh.val;
            else if (lh.is_integer() && lh.val==_NLP_MAXIMIZE && rh.is_integer())
                maximize=(bool)rh.val;
            else if(lh.is_integer() && lh.val==_NLP_PRECISION && rh.type==_DOUBLE_)
                eps=rh.DOUBLE_val();
            else if (contains(vars,lh) && rh.is_symb_of_sommet(at_interval)) {
                gen &lb=rh._SYMBptr->feuille._VECTptr->front();
                gen &ub=rh._SYMBptr->feuille._VECTptr->back();
                if (!is_inf(lh))
                    constr.push_back(symbolic(at_superieur_egal,makevecteur(lh,lb)));
                if (!is_inf(rh))
                    constr.push_back(symbolic(at_inferieur_egal,makevecteur(lh,ub)));
            }
        }
    }
    if (constr.empty()) {
        *logptr(contextptr) << "Error: no contraints detected" << endl;
        return gensizeerr(contextptr);
    }
    bool feasible=true;
    for (it=constr.begin();it!=constr.end();++it) {
        if (it->is_symb_of_sommet(at_equal)) {
            gen expr=_equal2diff(*it,contextptr);
            if (!is_zero(_subs(makesequence(expr,vars,initp),contextptr))) {
                feasible=false;
                break;
            }
        } else if (it->is_symb_of_sommet(at_inferieur_egal) || it->is_symb_of_sommet(at_superieur_egal)) {
            if (_evalb(_subs(makesequence(*it,vars,initp),contextptr),contextptr).val==0) {
                feasible=false;
                break;
            }
        } else {
            *logptr(contextptr) << "Error: unrecognized constraint " << *it << endl;
            return gentypeerr(contextptr);
        }
    }
    gen sol,optval;
    try {
        if (!feasible) {
            initp=*_fMin(makesequence(gen(0),constr,vars,initp),contextptr)._VECTptr;
            if (is_undef(initp) || initp.empty()) {
                *logptr(contextptr) << "Error: unable to generate a feasible initial point" << endl;
                return undef;
            }
            *logptr(contextptr) << "Using a generated feasible initial point " << initp << endl;
        }
        gen args=makesequence(obj,constr,vars,initp,gen(eps),gen(maxiter));
        if (maximize)
            sol=_fMax(args,contextptr);
        else
            sol=_fMin(args,contextptr);
    } catch (std::runtime_error &err) {
        *logptr(contextptr) << "Error: " << err.what() << endl;
        return undef;
    }
    if (is_undef(sol))
        return undef;
    optval=_subs(makesequence(obj,vars,sol),contextptr);
    return gen(makevecteur(optval,_zip(makesequence(at_equal,vars,sol),contextptr)),_LIST__VECT);
}
static const char _nlpsolve_s []="nlpsolve";
static define_unary_function_eval (__nlpsolve,&_nlpsolve,_nlpsolve_s);
define_unary_function_ptr5(at_nlpsolve,alias_at_nlpsolve,&__nlpsolve,0,true)

/*
 * Returns the trigonometric polynomial in variable x passing through points
 * with ordinate componets in 'data' and the abscissa components equally spaced between
 * a and b (the first being equal a and the last being equal to b).
 */
gen triginterp(const vecteur &data,const gen &a,const gen &b,const identificateur &x,GIAC_CONTEXT) {
    int n=data.size();
    if (n<2)
        return gensizeerr(contextptr);
    int N=(n%2)==0?n/2:(n-1)/2;
    gen T=(b-a)*fraction(n,n-1),twopi=2*_IDNT_pi(),X;
    matrice cos_coeff=*_matrix(makesequence(N,n,0),contextptr)._VECTptr;
    matrice sin_coeff=*_matrix(makesequence(N,n,0),contextptr)._VECTptr;
    for (int k=0;k<n;++k) {
        X=twopi*(a/T+fraction(k,n));
        for (int j=1;j<=N;++j) {
            cos_coeff[j-1]._VECTptr->at(k)=cos(j*X,contextptr);
            sin_coeff[j-1]._VECTptr->at(k)=sin(j*X,contextptr);
        }
    }
    gen tp=_mean(data,contextptr);
    for (int j=0;j<N;++j) {
        gen c=fraction(((n%2)==0 && j==N-1)?1:2,n);
        gen ak=_evalc(trig2exp(scalarproduct(data,*cos_coeff[j]._VECTptr,contextptr),contextptr),contextptr);
        gen bk=_evalc(trig2exp(scalarproduct(data,*sin_coeff[j]._VECTptr,contextptr),contextptr),contextptr);
        tp+=_simplify(c*ak,contextptr)*cos(_ratnormal((j+1)*twopi/T,contextptr)*x,contextptr);
        tp+=_simplify(c*bk,contextptr)*sin(_ratnormal((j+1)*twopi/T,contextptr)*x,contextptr);
    }
    return tp;
}

gen _triginterp(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT)
        return gentypeerr(contextptr);
    vecteur &args=*g._VECTptr;
    if (args.size()<2)
        return gensizeerr(contextptr);
    if (args.front().type!=_VECT)
        return gentypeerr(contextptr);
    vecteur &data=*args.front()._VECTptr;
    gen x,ab,a,b,&vararg=args.at(1);
    if (vararg.is_symb_of_sommet(at_equal) &&
               (x=_lhs(vararg,contextptr)).type==_IDNT &&
               (ab=_rhs(vararg,contextptr)).is_symb_of_sommet(at_interval)) {
        a=_lhs(ab,contextptr);
        b=_rhs(ab,contextptr);
    } else if (args.size()==4 && (x=args.back()).type==_IDNT) {
        a=args.at(1);
        b=args.at(2);
    } else return gensizeerr(contextptr);
    gen tp=triginterp(data,a,b,*x._IDNTptr,contextptr);
    if (is_approx(data) || is_approx(a) || is_approx(b))
        tp=_evalf(tp,contextptr);
    return tp;
}
static const char _triginterp_s []="triginterp";
static define_unary_function_eval (__triginterp,&_triginterp,_triginterp_s);
define_unary_function_ptr5(at_triginterp,alias_at_triginterp,&__triginterp,0,true)

/* select a good bandwidth for kernel density estimation using a direct plug-in method (DPI),
 * Gaussian kernel is assumed */
double select_bandwidth_dpi(const vector<double> &data,double sd) {
    int n=data.size();
    double g6=1.23044723*sd,s=0,t,t2;
    for (vector<double>::const_iterator it=data.begin();it!=data.end();++it) {
        for (vector<double>::const_iterator jt=it+1;jt!=data.end();++jt) {
            t=(*it-*jt)/g6;
            t2=t*t;
            s+=(2*t2*(t2*(t2-15)+45)-30)*std::exp(-t2/2);
        }
    }
    s-=15*n;
    double g4=g6*std::pow(-(6.0*n)/s,1/7.0);
    s=0;
    for (vector<double>::const_iterator it=data.begin();it!=data.end();++it) {
        for (vector<double>::const_iterator jt=it+1;jt!=data.end();++jt) {
            t=(*it-*jt)/g4;
            t2=t*t;
            s+=(2*t2*(t2-6)+6)*std::exp(-t2/2);
        }
    }
    s+=3*n;
    return std::pow(double(n)/(M_SQRT2*s),0.2)*g4;
}

gen fft_sum(const vecteur &c,const vecteur &k,int M,GIAC_CONTEXT) {
    return _scalar_product(makesequence(c,_mid(makesequence(_convolution(makesequence(c,k),contextptr),M,M),contextptr)),contextptr);
}

/* faster bandwidth DPI selector using binned data and FFT */
double select_bandwidth_dpi_bins(int n,const vecteur &c,double d,double sd,GIAC_CONTEXT) {
    int M=c.size();
    vecteur k(2*M+1);
    double g6=1.23044723*sd,s=0,t,t2;
    for (int i=0;i<=2*M;++i) {
        t=d*double(i-M)/g6;
        t2=t*t;
        k[i]=gen((2*t2*(t2*(t2-15)+45)-30)*std::exp(-t2/2));
    }
    s=_evalf(fft_sum(c,k,M,contextptr),contextptr).DOUBLE_val();
    double g4=g6*std::pow(-(6.0*n)/s,1/7.0);
    for (int i=0;i<=2*M;++i) {
        t=d*double(i-M)/g4;
        t2=t*t;
        k[i]=gen((2*t2*(t2-6)+6)*std::exp(-t2/2));
    }
    s=_evalf(fft_sum(c,k,M,contextptr),contextptr).DOUBLE_val();
    return std::pow(double(n)/(M_SQRT2*s),0.2)*g4;
}

/* kernel density estimation with Gaussian kernel */
gen kernel_density(const vector<double> &data,double bw,double sd,int bins,double a,double b,int interp,const gen &x,GIAC_CONTEXT) {
    int n=data.size();
    double SQRT_2PI=std::sqrt(2.0*M_PI);
    if (bins<=0) { // return density as a sum of exponential functions, usable for up to few hundred samples
        if (bw<=0)
            bw=select_bandwidth_dpi(data,sd);
        double fac=bw*n*SQRT_2PI;
        gen res(0),h(2.0*bw*bw);
        for (vector<double>::const_iterator it=data.begin();it!=data.end();++it) {
            res+=exp(-pow(x-gen(*it),2)/h,contextptr);
        }
        return res/gen(fac);
    }
    /* FFT method, constructs an approximation on [a,b] with the specified number of bins.
     * If interp>0, interpolation of order interp is performed and the density is returned piecewise. */
    assert(b>a && bins>0);
    double d=(b-a)/(bins-1);
    vecteur c(bins,0);
    int index;
    for (vector<double>::const_iterator it=data.begin();it!=data.end();++it) {
        index=(int)((*it-a)/d+0.5);
        if (index>=0 && index<bins) c[index]+=1;
    }
    if (bw<=0) { // select bandwidth
        if (n<=1000)
            bw=select_bandwidth_dpi(data,sd);
        else bw=select_bandwidth_dpi_bins(n,c,d,sd,contextptr);
        *logptr(contextptr) << "selected bandwidth: " << bw << endl;
    }
    int L=std::min(bins-1,(int)std::floor(1+4*bw/d));
    vecteur k(2*L+1);
    for (int i=0;i<=2*L;++i) {
        k[i]=gen(1.0/(n*bw*SQRT_2PI)*std::exp(-std::pow(d*double(i-L)/bw,2)/2.0));
    }
    gen res=_mid(makesequence(_convolution(makesequence(c,k),contextptr),L,bins),contextptr);
    if (interp>0) { // interpolate the obtained points
        int pos0=0;
        if (x.type!=_IDNT) {
            double xd=_evalf(x,contextptr).DOUBLE_val();
            if (xd<a || xd>=b || (pos0=std::floor((xd-a)/d))>bins-2)
                return 0;
            if (interp==1) {
                gen &y1=res._VECTptr->at(pos0),&y2=res._VECTptr->at(pos0+1),x1=a+pos0*d;
                return y1+(x-x1)*(y2-y1)/gen(d);
            }
        }
        vecteur pos(bins);
        for (int i=0;i<bins;++i) pos[i]=a+d*i;
        identificateur X=x.type==_IDNT?*x._IDNTptr:identificateur(" X");
        vecteur p=*_spline(makesequence(pos,res,X,interp),contextptr)._VECTptr;
        vecteur args(0);
        if (x.type==_IDNT)
            args.reserve(2*bins+1);
        for (int i=0;i<bins;++i) {
            if (x.type==_IDNT) {
                args.push_back(i+1<bins?symb_inferieur_strict(X,pos[i]):symb_inferieur_egal(X,pos[i]));
                args.push_back(i==0?gen(0):p[i-1]);
            } else if (i==pos0) res=_ratnormal(_subst(makesequence(p[i],X,x),contextptr),contextptr);
            if (i+1<bins && !_solve(makesequence(p[i],symb_equal(X,symb_interval(pos[i],pos[i+1]))),contextptr)._VECTptr->empty())
                *logptr(contextptr) << "Warning: interpolated density has negative values in ["
                                    << pos[i] << "," << pos[i+1] << "]" << endl;
        }
        if (x.type!=_IDNT) return res;
        args.push_back(0);
        res=symbolic(at_piecewise,change_subtype(args,_SEQ__VECT));
        return res;
    }
    return res;
}

bool parse_interval(const gen &feu,double &a,double &b,GIAC_CONTEXT) {
    vecteur &v=*feu._VECTptr;
    gen l=v.front(),r=v.back();
    if ((l=_evalf(l,contextptr)).type!=_DOUBLE_ || (r=_evalf(r,contextptr)).type!=_DOUBLE_ ||
            !is_strictly_greater(r,l,contextptr))
        return false;
    a=l.DOUBLE_val(); b=r.DOUBLE_val();
    return true;
}

gen _kernel_density(const gen &g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT)
        return gentypeerr(contextptr);
    gen x=identificateur("x");
    double a=0,b=0,bw=0,sd,d,sx=0,sxsq=0;
    int bins=100,interp=1,method=_KDE_METHOD_LIST,bw_method=_KDE_BW_METHOD_DPI;
    if (g.subtype==_SEQ__VECT) {
        // parse options
        for (const_iterateur it=g._VECTptr->begin()+1;it!=g._VECTptr->end();++it) {
            if (it->is_symb_of_sommet(at_equal)) {
                gen &opt=it->_SYMBptr->feuille._VECTptr->front();
                gen &v=it->_SYMBptr->feuille._VECTptr->back();
                if (opt==_KDE_BANDWIDTH) {
                    if (v==at_select)
                        bw_method=_KDE_BW_METHOD_DPI;
                    else if (v==at_gauss || v==at_normal || v==at_normald)
                        bw_method=_KDE_BW_METHOD_ROT;
                    else {
                        gen ev=_evalf(v,contextptr);
                        if (ev.type!=_DOUBLE_ || !is_strictly_positive(ev,contextptr))
                            return gensizeerr(contextptr);
                        bw=ev.DOUBLE_val();
                    }
                } else if (opt==_KDE_BINS) {
                    if (!v.is_integer() || !is_strictly_positive(v,contextptr))
                        return gensizeerr(contextptr);
                    bins=v.val;
                } else if (opt==at_range) {
                    if (v.type==_VECT) {
                        if (v._VECTptr->size()!=2 || !parse_interval(v,a,b,contextptr))
                            return gensizeerr(contextptr);
                    } else if (!v.is_symb_of_sommet(at_interval) ||
                               !parse_interval(v._SYMBptr->feuille,a,b,contextptr))
                        return gensizeerr(contextptr);
                } else if (opt==at_output || opt==at_Output) {
                    if (v==at_exact)
                        method=_KDE_METHOD_EXACT;
                    else if (v==at_piecewise)
                        method=_KDE_METHOD_PIECEWISE;
                    else if (v==_MAPLE_LIST)
                        method=_KDE_METHOD_LIST;
                    else return gensizeerr(contextptr);
                } else if (opt==at_interp) {
                    if (!v.is_integer() || (interp=v.val)<1)
                        return gensizeerr(contextptr);
                } else if (opt==at_spline) {
                    if (!v.is_integer() || (interp=v.val)<1)
                        return gensizeerr(contextptr);
                    method=_KDE_METHOD_PIECEWISE;
                } else if (opt.type==_IDNT) {
                    x=opt;
                    if (!v.is_symb_of_sommet(at_interval) || !parse_interval(v._SYMBptr->feuille,a,b,contextptr))
                        return gensizeerr(contextptr);
                } else if (opt==at_eval) x=v;
                else return gensizeerr(contextptr);
            } else if (it->type==_IDNT) x=*it;
            else if (it->is_symb_of_sommet(at_interval)) {
                if (!parse_interval(it->_SYMBptr->feuille,a,b,contextptr))
                    return gensizeerr(contextptr);
            } else if (*it==at_exact)
                method=_KDE_METHOD_EXACT;
            else if (*it==at_piecewise)
                method=_KDE_METHOD_PIECEWISE;
            else return gensizeerr(contextptr);
        }
    }
    if (x.type!=_IDNT && (_evalf(x,contextptr).type!=_DOUBLE_ || method==_KDE_METHOD_LIST))
        return gensizeerr(contextptr);
    vecteur &data=g.subtype==_SEQ__VECT?*g._VECTptr->front()._VECTptr:*g._VECTptr;
    int n=data.size();
    if (n<2)
        return gensizeerr(contextptr);
    vector<double> ddata(n);
    gen e;
    for (const_iterateur it=data.begin();it!=data.end();++it) {
        if ((e=_evalf(*it,contextptr)).type!=_DOUBLE_)
            return gensizeerr(contextptr);
        d=ddata[it-data.begin()]=e.DOUBLE_val();
        sx+=d;
        sxsq+=d*d;
    }
    sd=std::sqrt(1/double(n-1)*(sxsq-1/double(n)*sx*sx));
    if (bw_method==_KDE_BW_METHOD_ROT) { // Silverman's rule of thumb
        double iqr=_evalf(_quartile3(data,contextptr)-_quartile1(data,contextptr),contextptr).DOUBLE_val();
        bw=1.06*std::min(sd,iqr/1.34)*std::pow(double(data.size()),-0.2);
        *logptr(contextptr) << "selected bandwidth: " << bw << endl;
    }
    if (bins>0 && a==0 && b==0) {
        a=_evalf(_min(data,contextptr),contextptr).DOUBLE_val()-3*bw;
        b=_evalf(_max(data,contextptr),contextptr).DOUBLE_val()+3*bw;
    }
    if (method==_KDE_METHOD_EXACT)
        bins=0;
    else if (method==_KDE_METHOD_LIST) {
        if (bins<1)
            return gensizeerr(contextptr);
        interp=0;
    } else if (method==_KDE_METHOD_PIECEWISE) {
        if (bins<1 || interp<1)
            return gensizeerr(contextptr);
    }
    return kernel_density(ddata,bw,sd,bins,a,b,interp,x,contextptr);
}
static const char _kernel_density_s []="kernel_density";
static define_unary_function_eval (__kernel_density,&_kernel_density,_kernel_density_s);
define_unary_function_ptr5(at_kernel_density,alias_at_kernel_density,&__kernel_density,0,true)

static const char _kde_s []="kde";
static define_unary_function_eval (__kde,&_kernel_density,_kde_s);
define_unary_function_ptr5(at_kde,alias_at_kde,&__kde,0,true)

#ifndef NO_NAMESPACE_GIAC
}
#endif // ndef NO_NAMESPACE_GIAC
