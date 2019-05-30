/***************************************************************************************[Solver.cc]
abcdSAT i17 -- Copyright (c) 2017, Jingchao Chen, Donghua University   

abcdSAT sources are obtained by modifying Glucose (see below Glucose copyrights). Permissions and copyrights of
abcdSAT are exactly the same as Glucose.

----------------------------------------------------------
Glucose -- Copyright (c) 2009, Gilles Audemard, Laurent Simon
				CRIL - Univ. Artois, France
				LRI  - Univ. Paris Sud, France
 
Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose are exactly the same as Minisat on which it is based on. (see below).

---------------

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>
#include "abcdsat/mtl/Sort.h"
#include "abcdsat/core/Solver.h"
#include "abcdsat/core/Constants.h"
#include "abcdsat/utils/System.h"

int mod_k=4;
unsigned int all_conflicts=0;
unsigned int unsatCnt=0;
unsigned int unsat9Cnt=0;
int outvars=0;
  
int cancel_subsolve=0;
int LBDmode=1;
extern int size30;

extern int **Bclauses;
extern int nBlocked2;
extern int BCDfalses;
extern int nClausesB;
int  *varRange=0;
float *size_diff=0;
int learnCallbackBuffer[10000];
 
void XOR_Preprocess(PPS *pps, int way);
void release_pps (PPS * pps);

using namespace abcdSAT;

vec<Lit> priorityLit;

inline Lit makeLit(int lit) { return (lit > 0) ? mkLit(lit-1) : ~mkLit(-lit-1);}

Solver* mainSolver=0;
int    treeDepth=0;
int    * ite2ext=0;

inline int GetextLit (int ilit) 
{
  if(ilit < 0) return -ite2ext[-ilit];
  return ite2ext[ilit];
}

//=================================================================================================
// Options:

static const char* _cat = "CORE";
static const char* _cr = "CORE -- RESTART";
static const char* _cred = "CORE -- REDUCE";
static const char* _cm = "CORE -- MINIMIZE";

static DoubleOption opt_K                 (_cr, "K",           "The constant used to force restart",            0.8,     DoubleRange(0, false, 1, false));           
static DoubleOption opt_R                 (_cr, "R",           "The constant used to block restart",            1.4,     DoubleRange(1, false, 5, false));           
static IntOption     opt_size_lbd_queue     (_cr, "szLBDQueue",      "The size of moving average for LBD (restarts)", 50, IntRange(10, INT32_MAX));
static IntOption     opt_size_trail_queue     (_cr, "szTrailQueue",      "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT32_MAX));

static IntOption     opt_first_reduce_db     (_cred, "firstReduceDB",      "The number of conflicts before the first reduce DB", 2000, IntRange(0, INT32_MAX));
static IntOption     opt_inc_reduce_db     (_cred, "incReduceDB",      "Increment for reduce DB", 300, IntRange(0, INT32_MAX));
static IntOption     opt_spec_inc_reduce_db     (_cred, "specialIncReduceDB",      "Special increment for reduce DB", 1000, IntRange(0, INT32_MAX));
static IntOption    opt_lb_lbd_frozen_clause      (_cred, "minLBDFrozenClause",        "Protect clauses if their LBD decrease and is lower than (for one turn)", 30, IntRange(0, INT32_MAX));

static IntOption     opt_lb_size_minimzing_clause     (_cm, "minSizeMinimizingClause",      "The min size required to minimize clause", 30, IntRange(3, INT32_MAX));
static IntOption     opt_lb_lbd_minimzing_clause     (_cm, "minLBDMinimizingClause",      "The min LBD required to minimize clause", 6, IntRange(3, INT32_MAX));


static DoubleOption  opt_var_decay         (_cat, "var-decay",   "The variable activity decay factor",            0.8,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay      (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);

static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.20, DoubleRange(0, false, HUGE_VAL, false));

//=================================================================================================
// Constructor/Destructor:

Solver::Solver() :

    // Parameters (user settable):
    //
    verbosity        (0)
    , K              (opt_K)
    , R              (opt_R)
    , sizeLBDQueue   (opt_size_lbd_queue)
    , sizeTrailQueue   (opt_size_trail_queue)
    , firstReduceDB  (opt_first_reduce_db)
    , incReduceDB    (opt_inc_reduce_db)
    , specialIncReduceDB    (opt_spec_inc_reduce_db)
    , lbLBDFrozenClause (opt_lb_lbd_frozen_clause)
    , lbSizeMinimizingClause (opt_lb_size_minimzing_clause)
    , lbLBDMinimizingClause (opt_lb_lbd_minimzing_clause)
  , var_decay        (opt_var_decay)
  , clause_decay     (opt_clause_decay)
  , random_var_freq  (opt_random_var_freq)
  , random_seed      (opt_random_seed)
  , ccmin_mode       (opt_ccmin_mode)
  , phase_saving     (opt_phase_saving)
  , rnd_pol          (false)
  , rnd_init_act     (opt_rnd_init_act)
  , garbage_frac     (opt_garbage_frac)
 
    // Statistics: (formerly in 'SolverStats')
    //
  ,  nbRemovedClauses(0),nbReducedClauses(0), nbDL2(0),nbBin(0),nbUn(0) , nbReduceDB(0)
    , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0),nbstopsrestarts(0)
  , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)
    , curRestart(1)

  , ok                 (true)
  , cla_inc            (1)
  , var_inc            (1)
  , watches            (WatcherDeleted(ca))
  , watchesBin            (WatcherDeleted(ca))
  , qhead              (0)
  , simpDB_assigns     (-1)
  , simpDB_props       (0)
  , order_heap         (VarOrderLt(activity))
  , sched_heap         (VarSchedLt(vscore))
  , remove_satisfied   (true)

    // Resource constraints:
    //
  , conflict_budget    (-1)
  , propagation_budget (-1)
  , asynch_interrupt   (false)

{   MYFLAG=0;
    g_pps=(struct PPS *) calloc (1, sizeof (PPS));
    recursiveDepth=0;
    conflict_limit=-1;
    completesolve=0;
    originVars=0;
    equhead=equlink=0;
    equMemSize=0;
    dummyVar=0;
    hbrsum=equsum=unitsum=probeSum=0;
    varRange=0;
    longcls=0;
    probe_unsat=0;
    BCD_conf=500000;
    maxDepth=0;
    subformuleClause=0;
    needExtVar=0;
    touchMark=0;
    solvedXOR=0;
    lbdQueue.initSize(50);
    incremental=0;
    preprocessflag=0;
    termCallback=0;
    learnCallback=0;
    learnCallbackLimit=0;
}

Solver::~Solver()
{
   ClearClause(clauses, 0);
   ClearClause(learnts, 0);

   if(probe_unsat==0) garbageCollect();
   if(g_pps->unit) free(g_pps->unit);
  
   if(equhead) free(equhead);
   if(equlink) free(equlink);
   equhead = equlink=0;
   if(dummyVar) free(dummyVar);
   dummyVar=0;
   delete g_pps;
}

CRef Solver::unitPropagation(Lit p)
{
     uncheckedEnqueue(p);
     CRef confl = propagate();
     return confl;
}

bool Solver :: is_conflict(vec<Lit> & lits)
{  
   cancelUntil(0);
   int i;
   for (i=0; i<lits.size(); i++){
        Lit p = ~lits[i];
        if ( value(p) == l_True) continue;
        if ( value(p) == l_False) break;
        newDecisionLevel();
        uncheckedEnqueue(p);
        CRef confl = propagate();
        if (confl != CRef_Undef) break;
   }
   cancelUntil(0);
   if(i<lits.size()) return true;
   return false;
}

bool Solver :: is_conflict(Lit p, Lit q)
{
      vec<Lit> ps;
      ps.push(p);
      ps.push(q);
      return is_conflict(ps);
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches  .init(mkLit(v, false));
    watches  .init(mkLit(v, true ));
    watchesBin  .init(mkLit(v, false));
    watchesBin  .init(mkLit(v, true ));
    assigns  .push(l_Undef);
    vardata  .push(mkVarData(CRef_Undef, 0));
    activity .push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen     .push(0);
    permDiff  .push(0);
    polarity .push(sign);
    decision .push();
    trail    .capacity(v+1);
    setDecisionVar(v, dvar);
    return v;
}

bool Solver::addClause_(vec<Lit>& ps)
{
    if (!ok) return false;

    sort(ps);
    Lit p; int i, j;

    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
            if (value(ps[i]) == l_True || ps[i] == ~p) return true;
            else if (value(ps[i]) != l_False && ps[i] != p)
                 ps[j++] = p = ps[i];
           ps.shrink(i - j);
   
    if (ps.size() == 0) return ok = false;
    else if (ps.size() == 1){
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    }else{
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);
    }
    return true;
}

void Solver::attachClause(CRef cr) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    if(c.size()==2) {
      watchesBin[~c[0]].push(Watcher(cr, c[1]));
      watchesBin[~c[1]].push(Watcher(cr, c[0]));
    } else {
      watches[~c[0]].push(Watcher(cr, c[1]));
      watches[~c[1]].push(Watcher(cr, c[0]));
    }
    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size(); 
}

void Solver::detachClause(CRef cr, bool strict) {
    const Clause& c = ca[cr];
    
    assert(c.size() > 1);
    if(c.size()==2) {
      if (strict){
        remove(watchesBin[~c[0]], Watcher(cr, c[1]));
        remove(watchesBin[~c[1]], Watcher(cr, c[0]));
      }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watchesBin.smudge(~c[0]);
        watchesBin.smudge(~c[1]);
      }
    } else {
      if (strict){
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
      }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
       watches.smudge(~c[0]);
       watches.smudge(~c[1]);
      }
    }
    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size(); }


void Solver::removeClause(CRef cr) {
  
  Clause& c = ca[cr];
  detachClause(cr);
  // Don't leave pointers to free'd memory!
  if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
  c.mark(1); 
  ca.free(cr);
}

bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++){
        if (value(c[i]) == l_True) return true;
    }
  
    return false; 
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
           if(maxDepth < trail.size()-trail_lim[0]) maxDepth=trail.size()-trail_lim[0];
           for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var      x  = var(trail[c]);
            assigns [x] = l_Undef;
            if (phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
                polarity[x] = sign(trail[c]);
            insertVarOrder(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    } }


//=================================================================================================
// Major methods:

extern int *extVarRank,numVarRk;

Lit Solver::pickBranchLit()
{
    Var next = var_Undef;
    int clevel=decisionLevel();
    if(clevel < 6 && numVarRk>0){
          double MaxAct=0;
          for(int i=0; i<numVarRk && i<100; i++){
             int v=extVarRank[i]-1;
             if (decision[v] && value(v) == l_Undef){
                   if(activity[v]>MaxAct){
                          MaxAct=activity[v];
                          next=v;
                   }
             }
          }
          if(next != var_Undef) goto found;
    }
   if (next == var_Undef){
        if (order_heap.empty()) return lit_Undef;
        next = order_heap.removeMin();
    }
   
   // Activity based decision:
    while (value(next) != l_Undef || !decision[next])
        if (order_heap.empty()) return lit_Undef;
        else next = order_heap.removeMin();

found:
    if(bitVecNo>=0){
           if(clevel<12){
                   polarity[next]=(bitVecNo>>(clevel% mod_k)) & 1;
           }
     }
     return mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|  
|  Description:
|    Analyze conflict and produce a reason clause.
|  
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|  
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the 
|        rest of literals. There may be others from the same level though.
|  
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel,unsigned int &lbd)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;

    do{
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause& c = ca[confl];

	// Special case for binary clauses
	// The first one has to be SAT
	if( p != lit_Undef && c.size()==2 && value(c[0])==l_False) {
	  
	  assert(value(c[1])==l_True);
	  Lit tmp = c[0];
	  c[0] =  c[1], c[1] = tmp;
	}
	
       if (c.learnt())  claBumpActivity(c);
       if(c.learnt()  && c.lbd()>2) {
         unsigned int nblevels = compute0LBD(c);
	 if(nblevels+1<c.lbd() ) { // improve the LBD
	   if(c.lbd()<=lbLBDFrozenClause) c.setCanBeDel(false); 
	   c.setLBD(nblevels);
	 }
       }

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];

            if (!seen[var(q)] && level(var(q)) > 0){
                varBumpActivity(var(q),(1e4-j)*var_inc/1e4);
                seen[var(q)] = 1;
                if (level(var(q)) >= decisionLevel()) {
                    pathC++;
		    if((reason(var(q))!= CRef_Undef)  && ca[reason(var(q))].learnt()) 
		      lastDecisionLevel.push(q);
		} else {
                    out_learnt.push(q);
		}
	    }
        }
        
        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2){
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
        
    }else if (ccmin_mode == 1){
        for (i = j = 1; i < out_learnt.size(); i++){
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else{
                Clause& c = ca[reason(var(out_learnt[i]))];
                for (int k = ((c.size()==2) ? 0:1); k < c.size(); k++)
               // for (int k = 1; k < c.size(); k++) bug
                    if (!seen[var(c[k])] && level(var(c[k])) > 0){
                        out_learnt[j++] = out_learnt[i];
                        break; }
            }
        }
    }else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();


    /* ***************************************
      Minimisation with binary clauses of the asserting clause
      First of all : we look for small clauses
      Then, we reduce clauses with small LBD.
      Otherwise, this can be useless
     */
    if(out_learnt.size()<=lbSizeMinimizingClause) {
      // Find the LBD
       lbd = compute0LBD(out_learnt);
       if(lbd<=lbLBDMinimizingClause) binResMinimize(out_learnt);
    }
    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)    out_btlevel = 0;
    else{
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level(var(p));
    }

  // Find the LBD measure 
   lbd = compute0LBD(out_learnt);
 
  if(lastDecisionLevel.size()>0) {
    for(int i = 0;i<lastDecisionLevel.size();i++) {
      if(ca[reason(var(lastDecisionLevel[i]))].lbd()<lbd)
	varBumpActivity2(var(lastDecisionLevel[i]));
    }
    lastDecisionLevel.clear();
  } 

    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear(); analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();
	if(c.size()==2 && value(c[0])==l_False) {
	  assert(value(c[1])==l_True);
	  Lit tmp = c[0];
	  c[0] =  c[1], c[1] = tmp;
	}

        for (int i = 1; i < c.size(); i++){
            Lit p  = c[i];
            if (!seen[var(p)] && level(var(p)) > 0){
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|  
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if (seen[x]){
            if (reason(x) == CRef_Undef){
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = ca[reason(x)];
		// for (int j = 1; j < c.size(); j++) Minisat (glucose 2.0) loop 
		// Many thanks to Sam Bayless (sbayless@cs.ubc.ca) for discover this bug.
		for (int j = ((c.size()==2) ? 0:1); j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }  

            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}


void Solver::uncheckedEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|  
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|  
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate()
{
    CRef    confl     = CRef_Undef;
    int     num_props = 0;
    
    watches.cleanAll();
    watchesBin.cleanAll();
 
    while (qhead < trail.size()){
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watcher>&  ws  = watches[p];
        Watcher        *i, *j, *end;
        num_props++;
	
	    // First, Propagate binary clauses 
	vec<Watcher>&  wbin  = watchesBin[p];
	
	for(int k = 0;k<wbin.size();k++) {
	  
	  Lit imp = wbin[k].blocker;
	  
	  if(value(imp) == l_False) {
	    return wbin[k].cref;
	  }
	  
	  if(value(imp) == l_Undef) {
	    uncheckedEnqueue(imp,wbin[k].cref);
	  }
	}

        for (i = j = (Watcher*)ws, end = i + ws.size();  i != end;){
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True){
                *j++ = *i++; continue; }

            // Make sure the false literal is data[1]:
            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];
            Lit      false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit     first = c[0];
            Watcher w     = Watcher(cr, first);
            if (first != blocker && value(first) == l_True){
	      
	      *j++ = w; continue; }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
                if (value(c[k]) != l_False){
                    c[1] = c[k]; c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause; }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False){
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            }else {
                uncheckedEnqueue(first, cr);
	    }
        NextClause:;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;
    
    return confl;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|  
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt_lbd{ 
    ClauseAllocator& ca;
    reduceDB_lt_lbd(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) { 
 
    // Main criteria... Like in MiniSat we keep all binary clauses
    if(ca[x].size()> 2 && ca[y].size()==2) return 1; //old 
    if(ca[y].size()>2 && ca[x].size()==2) return 0;  //old
    
     if(ca[x].size()==2 && ca[y].size()==2) return 0;
    
    // Second one  based on literal block distance
    if(ca[x].lbd()> ca[y].lbd()) return 1;
    if(ca[x].lbd()< ca[y].lbd()) return 0;    
    
    
    // Finally we can use old activity or size, we choose the last one
        return ca[x].activity() < ca[y].activity();
    }    
};

struct reduceDB_lt_sz { 
    ClauseAllocator& ca;
    reduceDB_lt_sz(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) { 
 
      if(ca[x].size()> ca[y].size()) return 1;//new
      if(ca[x].size()< ca[y].size()) return 0;//new
   
       if(ca[x].size()==2)  return 0;
    
    // Second one  based on literal block distance
       if(ca[x].lbd()> ca[y].lbd()) return 1;
       if(ca[x].lbd()< ca[y].lbd()) return 0;    
       return ca[x].activity() < ca[y].activity();
    }    
};

void Solver::reduceDB()
{
  int     i, j;
  if(learnts.size() < 30) return;
  nbReduceDB++;
  int limit = learnts.size() / 2;

  if(LBDmode) sort(learnts, reduceDB_lt_lbd(ca));
  else  sort(learnts, reduceDB_lt_sz(ca));

  // We have a lot of "good" clauses, it is difficult to compare them. Keep more !
  if(ca[learnts[learnts.size() / RATIOREMOVECLAUSES]].lbd()<=3) nbclausesbeforereduce +=specialIncReduceDB; 
  // Useless :-)
  if(ca[learnts.last()].lbd()<=5)  nbclausesbeforereduce +=specialIncReduceDB; 
  
  // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
  // Keep clauses which seem to be usefull (their lbd was reduce during this sequence)
  for (i = j = 0; i < learnts.size(); i++){
    Clause& c = ca[learnts[i]];
    if (c.lbd()>2 && c.size() > 2 && c.canBeDel() &&  !locked(c) && (i < limit)) {
         removeClause(learnts[i]);
         nbRemovedClauses++;
    }
    else {
      if(!c.canBeDel()) limit++; //we keep c, so we can delete an other clause
      c.setCanBeDel(true);       // At the next step, c can be delete
      learnts[j++] = learnts[i];
    }
  }
  learnts.shrink(i - j);
  checkGarbage();
}

void Solver::removeSatisfied(vec<CRef>& cs,int delFlag)
{  int i, j;
   (void) delFlag;
   for (i = j = 0; i < cs.size(); i++){
        Clause& c = ca[cs[i]];
        if (c.size()>=2 && satisfied(c)){  // A bug if we remove size ==2, We need to correct it, but later.
                 removeClause(cs[i]);
        }
        else    cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

void Solver::rebuildOrderHeap()
{   
    if(equhead) equlinkmem();
    if(conflicts>500 && equhead && nFreeVars()>1200 && (recursiveDepth || nFreeVars()>20000)){
      for (int i =0; i < nVars(); i++) {
         int exv=i+1;
         if(equhead[exv]==0 || equhead[exv]==exv) continue;
         activity[i]=0;
      }            
    }
    vec<Var> vs;
    bool eq_condi = equhead && recursiveDepth && nFreeVars()>1200;
    bool condi2   = (conflicts>50000 || recursiveDepth);
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef){
             if(eq_condi){
                int exv=v+1;
                if(equhead[exv] && equhead[exv]!=exv) continue;
             }
             vs.push(v);
             if(condi2){
               Lit p = mkLit(v);//new idea
               int pos=watchesBin[p].size();
               int neg=watchesBin[~p].size();
               activity[v] += (pos+1)*(neg+1)/100.0;
            }
        }
    order_heap.build(vs);
}
/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|  
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
   if (!ok || propagate() != CRef_Undef) return ok = false;

   if (nAssigns() == simpDB_assigns || (simpDB_props > 0)) return true;
   
 // Remove satisfied clauses:
   removeSatisfied(learnts,1); 
   
   if (remove_satisfied){        // Can be turned off.
          static int pre_conf=0;
          if( (nVars() < 500000 || longcls<300000) && 
              (conflicts>200000 || recursiveDepth>3) && 
              (nClauses()<300000 || ((int)conflicts-pre_conf)>120000)){
                  pre_conf=(int)conflicts;
                  lbool ret=probe();
                  if(ret == l_False){
                       probe_unsat=1;
                       return ok = false;
                  }
          }
          else {
             removeSatisfied(clauses, 0);
          }
    }
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|  
|  Description:
|    Search for a model the specified number of conflicts. 
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|  
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{
    assert(ok);
    vec<Lit>    best_learnt;
    uint64_t old_conf=conflicts;
    bool restart=false;
    starts++;
    for (;;){
        CRef confl = propagate();
nexta:
        if (confl != CRef_Undef){
 	     conflicts++; 
         
	  if(conflicts%5000==0 && var_decay<0.95) var_decay += 0.01;
 	  if (verbosity > 0 && conflicts%verbEveryConflicts==0 && (recursiveDepth==0 || conflicts%40000==0)){ 
            printf("c | %g ", cpuTime());//dec_vars
	    if(conflicts) printf(" LBD=%d ", int(sumLBD / conflicts));

            printf("%8d  %7d  %5d  %7d | %7d %8d %8d | %5d %8d   %6d %8d | %6.3f %% |\n", 
		   (int)starts,(int)nbstopsrestarts, (int)(conflicts/starts), (int)conflicts, 
		   (int)nVars() - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals, 
		   (int)nbReduceDB, nLearnts(), (int)nbDL2,(int)nbRemovedClauses, progressEstimate()*100);
	  }
	  if (decisionLevel() == 0) return l_False;
	  
          trailQueue.push(trail.size());
          if(conflicts-old_conf <2000){
             if( conflicts>LOWER_BOUND_FOR_BLOCKING_RESTART && lbdQueue.isvalid()  && trail.size()>R*trailQueue.getavg()) {
	         lbdQueue.fastclear();
	         nbstopsrestarts++;
	     }
           }
           int min_back;
           unsigned int min_nblevels;
           best_learnt.clear();
           analyze(confl, best_learnt, min_back, min_nblevels);
           cancelUntil(min_back);
//ipasir 
           if (recursiveDepth==0 && learnCallback != 0 && best_learnt.size() <= learnCallbackLimit) {
                  for (int i = 0; i < best_learnt.size(); i++) {
                      Lit lit = best_learnt[i];
                      learnCallbackBuffer[i] = sign(lit) ? -(var(lit)+1) : (var(lit)+1);
                      if(ite2ext) learnCallbackBuffer[i] = GetextLit (learnCallbackBuffer[i]);  
                   }
                   learnCallbackBuffer[best_learnt.size()] = 0;
                   learnCallback(learnCallbackState, learnCallbackBuffer);
           }
//
           if (best_learnt.size() == 1){
                    confl = unitPropagation(best_learnt[0]);
                    lbdQueue.push(min_nblevels);
	            sumLBD += min_nblevels;
                    nbUn++;
                    goto nexta;
           }
           else{
                    CRef cr = ca.alloc(best_learnt, true);
        	    
                    if(incremental) ca[cr].setLBD(min_nblevels); 
                    else ca[cr].setLBD(0x7fffffff);

                    if(min_nblevels<=2) nbDL2++; 
        	    if(ca[cr].size()==2) {nbBin++;} 
              
                    learnts.push(cr);
                    attachClause(cr);
                    claBumpActivity(ca[cr]);
                    uncheckedEnqueue(best_learnt[0], cr);
            }
            varDecayActivity();
            claDecayActivity();
           
            lbdQueue.push(min_nblevels);
	    sumLBD += min_nblevels;
            restart = (nof_conflicts > 0) && (conflicts > (unsigned)nof_conflicts);
            restart = restart || (conflicts - old_conf > 20000);
        }else{
	  // Our dynamic restart, see the SAT09 competition compagnion paper 
          if(restart) goto backlev;  
          if ( lbdQueue.isvalid() && ((lbdQueue.getavg()*K) > (sumLBD / conflicts) )) {
	        lbdQueue.fastclear();
backlev:        ;
                int lev = 0;
	        if(incremental) { // do not back to 0
	             lev = (decisionLevel()<assumptions.size()) ? decisionLevel() : assumptions.size();
	        }
                cancelUntil(lev);
	        return l_Undef; 
          }
          // Simplify the set of problem clauses:
	  if (decisionLevel() == 0 && !simplify()) {
   	    return l_False;
	  }

          // Perform clause database reduction !
	    if((unsigned)conflicts>= curRestart* (unsigned)nbclausesbeforereduce) 
	      {
          	assert(learnts.size()>0);
		curRestart = (conflicts/ nbclausesbeforereduce)+1;
		reduceDB();
		nbclausesbeforereduce += incReduceDB;
	      }
//
            Lit next = lit_Undef;
            if(incremental) { 
	         while (decisionLevel() < assumptions.size()){
                    // Perform user provided assumption:
                    Lit p = assumptions[decisionLevel()];
                    if (value(p) == l_True){
                       // Dummy decision level:
                       newDecisionLevel();
                    }else if (value(p) == l_False){
                          analyzeFinal(~p, conflict);
                          return l_False;
                    }else{
                          next = p;
                          break;
                     }
                 }
            }

            if (next == lit_Undef){
                decisions++;
               // New variable decision:
                next = pickBranchLit();
                if (next == lit_Undef){
	          // Model found:
		   return l_True;
		}
            }
          // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}


double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

void SameReplaceSolve(int *Fsolution, PPS *pps);
void gaussTriSolve(int *Fsolution, PPS *pps);

lbool Solver::addassume()
{
      if(assumptions.size() == 0) return l_False;
      vec<Lit> lits;
      lits.clear();
      for(int i=0; i < assumptions.size(); i++){
             Lit p = assumptions[i];
             lits.push(~p);
      }
      bool ret=addClause(lits);
      if(ret==false) return l_False;
      return l_Undef;
}

void Solver:: buildEquAssume()
{   vec<Lit> lits;
    if(equhead==0) return;
    equlinkmem();
    for (int i=1; i <= nVars(); i++){
         if(equhead[i]==0 || equhead[i]==i) continue;
         Lit p=mkLit(i-1);
         int lit=equhead[i];
         Lit q = (lit>0) ? mkLit(lit-1) : ~mkLit(-lit-1);
         for(int j=0; j< assumptions.size(); j++){
                 Lit r = assumptions[j];
                 if(r==p || r==~p || r==q || r==~q){
                     if(!hasBin(p, ~q)){
                         lits.clear();
                         lits.push(p);
                         lits.push(~q);
                         simpAddClause(lits);
                     }
                     if(!hasBin(~p, q)){
                         lits.clear();
                         lits.push(~p);
                         lits.push(q);
                         simpAddClause(lits);
                    }
                    goto nextbin;
                }
         }
nextbin: ;
    }
}

lbool Solver::solve_()
{
    incremental=1;
    cancelUntil(0);
    int ct;
    bool Thisolver;
    lbool  status = l_Undef;
    int lim_conf=20000;
    int unFix=nUnfixedVars();

    outvars=originVars=nVars();
   // auxVar=nVars();

    if(preprocessflag==0) longcls=longClauses();
    oldVars=nVars();
   
    rebuildOrderHeap();
    model.clear();
    conflict.clear();
    if (!ok) return l_False;
  
    lbdQueue.initSize(sizeLBDQueue);
    trailQueue.initSize(sizeTrailQueue);
    sumLBD = 0;
    solves++;
    nbclausesbeforereduce = firstReduceDB;
    verbEveryConflicts=10000;
    ct=originVars>210000? 120000 :10000;
    Thisolver= (nClauses() > 1500000) || (originVars>100000 && longcls<10000) || 
          (originVars>50000 && originVars<250000 && nClauses()*2>=3*originVars && longcls>50000 &&
          nClauses()<2*originVars+ct && nClauses()>originVars) ;
   bitVecNo=-1;

   BCD_delay=0;
   if(preprocessflag==0 && nClauses() < 5000000 && originVars < 600000) status = s_probe ();
   preprocessflag=1;
   int cur_conf=0;
   int climit=-1;
   old_assigns.clear();
refind:
   buildEquAssume();
   while (status == l_Undef){
         bitVecNo = conflicts < 1e5? bitVecNo+1:-1;
         int prev_c=conflicts;
         status = search(climit);
         cur_conf += conflicts-prev_c;
         if (!withinBudget()) break;
         int cur_unFix=nUnfixedVars();
          if(status == l_Undef && lim_conf<cur_conf && cur_unFix != unFix) {
                 status = inprocess();
                 buildEquAssume();
                 unFix=cur_unFix;
                 lim_conf = cur_conf+3000;
         }
         if(cur_conf < 300000 || Thisolver) continue;
         if(originVars > 550000) continue;
//midsol: 
         if(status == l_Undef){
             addlearnt();
             if(verbosity > 0) printf("c mid_simplify cur_conflicts=%d \n",cur_conf);
             buildEquAssume();
             cancelUntil(0);
             old_assigns.growTo(nVars());
             for (int i = 0; i < nVars(); i++) old_assigns[i]=assigns[i];
             int rc=simp_solve();
             if(rc==UNSAT){
                   if(assumptions.size() == 0) status = l_False;
                   else status=addassume();
                   if(status == l_False) ok = false;
                   return l_False;
             }
             if(rc==SAT) status=l_True;
             break;
         }
   }
   if (status == l_True){
        if(old_assigns.size()<nVars()){
             old_assigns.growTo(nVars());
             for (int i = 0; i < nVars(); i++) old_assigns[i]=assigns[i];
        }
        s_extend ();     // Extend & copy model:
        if(verbosity > 0) printf("c solution found by first solver \n");
       
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
        verify();
        if(equhead) equlinkmem();
        solveEqu(equhead, nVars(), model);
       
        for (int i = 0; i < nVars(); i++) assigns[i]=model[i];
        bool redo=false;
        for (int i=0; i < assumptions.size(); i++){
             Lit p = assumptions[i];
             if (value(p) == l_True) continue;
             redo=true; break;
        }
        for (int i = 0; i < nVars(); i++) assigns[i]=old_assigns[i];
        cancelUntil(0);
        if(redo) goto refind;
       // printf("c done by abcdsat_i17 \n");
        return status;
    }
    if (status == l_False && assumptions.size() == 0) ok = false;
    cancelUntil(0);
    return status;
}

lbool Solver::solve2_()
{
    if(recursiveDepth<3) printf("c Depth=%d \n",recursiveDepth);
    if(recursiveDepth==0) all_conflicts=unsatCnt=unsat9Cnt=0;

    originVars=nVars();
    size30=0;
    if(ite2ext == 0) needExtVar=0;
 
    lbool   status        = l_Undef;
    BCDfalses=1000;
    BCD_conf=0;
    int mp= (originVars<10000)? 30: 21; 
    if(recursiveDepth==0 && numVarRk<30 && nClauses()< mp*originVars){
         if(originVars > 1600 && originVars < 15000)  BCD_conf=6000001;
         else                                         BCD_conf=500001;
    }

    BCD_delay=0;
    if(recursiveDepth==0){
        //  status = s_probe ();
          status = s_lift ();
          if(status != l_Undef) status = unhide ();
          if(status == l_Undef) status = distill(); 
          if(status == l_Undef) status = tarjan_SCC();
          if(status == l_Undef) status = transitive_reduction();
          if(status == l_Undef) status = s_eliminate ();
    }      
    model.clear();
    conflict.clear();
    if (!ok) return l_False;
 
    lbdQueue.initSize(sizeLBDQueue);
    trailQueue.initSize(sizeTrailQueue);
    sumLBD = 0;
    solves++;
    nbclausesbeforereduce = firstReduceDB;
    verbEveryConflicts=10000;
    bitVecNo=-1;
    if(verbosity > 0) printf("c D=%d hard#=%d unsat#=%d unsat9#=%d \n",recursiveDepth,all_conflicts,unsatCnt, unsat9Cnt);
 //   fflush(stdout);

    int force=0;
    if(2*unsatCnt>5*unsat9Cnt && recursiveDepth>=9 || recursiveDepth >=11){
          completesolve=1;
          force=1;
    }
   
    int sLBD=0;
    bitVecNo=-1;
    int unFix=nUnfixedVars();
    uint64_t lim_conf=5000;
    while (status == l_Undef){
         int climit=conflict_limit;
         if(recursiveDepth==0){
             bitVecNo = conflicts > 1e5 && conflicts < 2e5? bitVecNo+1:-1;
         }
         else{
             if(recursiveDepth<=9 && conflicts < 30000) bitVecNo++;
             else bitVecNo=-1;

             if(completesolve==1){
                   if(recursiveDepth < 11){
                        if(nVars() < 1400 && progressEstimate()>0.081 && (force || recursiveDepth !=9)) climit=-1;
                        else climit=force ? 600000:50000;
                   }
                   else  climit=-1;
             }
             if(completesolve==0 || cancel_subsolve){
                  if(recursiveDepth < 11) climit=500;
                  else climit=10000;
             }
             if(recursiveDepth > 100) climit=-1;
         }
         if(climit != -1) {
                 if(conflicts >= (unsigned) climit) goto done; 
         }
         status = search(climit);
         if (!withinBudget()) break;
       
         if(recursiveDepth==0){
             int cur_unFix=nUnfixedVars();
             if(status == l_Undef && lim_conf<conflicts && cur_unFix != unFix) {
                 status = inprocess();
                 unFix=cur_unFix;
                 lim_conf = conflicts+3000;
             }
         }

         if(solvedXOR && conflicts>30000000) break;

         if(BCD_conf>500001){
            if(conflicts>5000 && conflicts<300000 & sumLBD < 6*conflicts && sLBD==0) sLBD=int(sumLBD/conflicts);
            if(conflicts>300000 && sumLBD < 8*conflicts && (sLBD==0 || int(sumLBD/conflicts) <= sLBD)){
                BCD_conf=500001;
            }
         }
         if(BCD_conf){
              if(nClauses()>20*originVars && sumLBD < 10*conflicts && conflicts<1500000) continue;
         }   
//       
         if(recursiveDepth > 1900){
               if(conflicts > 20000){
                   if(status == l_Undef) return l_Undef;
               } 
               continue;
         }   
         if(recursiveDepth > 100){
               if(conflicts > 1500000){
                   if(status == l_Undef) return l_Undef;
               } 
               continue;
         }   
         if(recursiveDepth == 0){
              if(nVars() < 1400 && ((nVars()>=nUnfixedVars()+2 && conflicts<1500000) ||conflicts<500000)) continue;
              if(nVars() > 2500 && conflicts<6000000) continue;
              int aLBD=sumLBD/conflicts;
              if(aLBD > 16 || nVars() > 10000 || outvars<500) continue;
              if(nVars() > 1500){
                  if(sumLBD /13 < conflicts){
                         if(conflicts<500000) continue;
                         if(outvars>2000 && conflicts<3000000) continue;
                   }
                   if(conflicts>7000000 || conflicts>3000000 && (aLBD<=10 || nVars() > 3500 && aLBD<=11)) {
                      cancel_subsolve=1; continue;
                  }
               }
         }
         if(status==l_False){
               if(recursiveDepth <= 9) unsat9Cnt++;
               break;
         }
         if(recursiveDepth >= 11 && conflicts > 10000){
               if(all_conflicts> unsatCnt+1200) cancel_subsolve=1;
               if(conflicts > 2500000) cancel_subsolve=1;
        }

        if(recursiveDepth == 0 && cancel_subsolve) continue;
       
        if(completesolve==0){
             if(recursiveDepth==0 && conflicts >= 100000 || recursiveDepth>0 && conflicts >= (unsigned)climit){//9
                if(status == l_Undef) {
                       status=subsolve();
                       if(recursiveDepth==0 && status == l_Undef){
                             completesolve=1;
                             if(!cancel_subsolve) status=subsolve();
                       }
                 }
                 if(recursiveDepth) break;
                 cancel_subsolve=1;
                 continue;
             }
             if(conflicts >= (unsigned)climit) goto done;
             continue;
        }
        if(recursiveDepth >=11 || climit==-1){//13
              if(conflicts < 2500000) continue;
              cancel_subsolve=1;
        }
        if(cancel_subsolve) goto done;
        if(conflicts >= (unsigned)climit){
               if(status == l_Undef)  status=subsolve();
        }
     }

    if (verbosity > 0 && recursiveDepth==0)
      printf("c =========================================================================================================\n");
      if(conflicts > 4500000){
            if(recursiveDepth > 9) cancel_subsolve=1;
      }
      else{
             if(conflicts > 1650000) all_conflicts += 60;
             else if(conflicts > 230000){
                       int m=conflicts/200000; 
                       all_conflicts += (m*m);
                   }
                   else all_conflicts += ((int)conflicts/80000);
      }
done:
    if (status == l_Undef){
          cancel_subsolve = 1;
          cancelUntil(0);
          return l_Undef;
    }
    if (status == l_True){
        s_extend ();     // Extend & copy model:
     
        if(verbosity > 0) printf("c solution found depth=%d \n",recursiveDepth);

        if(equhead) equlinkmem();
        solveEqu(equhead, nVars(), assigns);
        if(recursiveDepth) return status;
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
        return status;
    }

    if(conflict.size() == 0) ok = false;
 
    unsatCnt++;
    return l_False;
}
  
//=================================================================================================
// Writing CNF to DIMACS:
// 
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
    if (map.size() <= x || map[x] == -1){
        map.growTo(x+1, -1);
        map[x] = max++;
    }
    return map[x];
}


void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
    if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
    fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit>& assumps)
{
    FILE* f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}


void Solver::toDimacs(FILE* f, const vec<Lit>& assumps)
{   (void)assumps;
    // Handle case when solver is in contradictory state:
    if (!ok){
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return; }
    
    vec<Var> map; Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
            cnt++;
        
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]])){
            Clause& c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++){
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max)+1);
    }

    for (int i = 0; i < clauses.size(); i++)
        toDimacs(f, ca[clauses[i]], map, max);
}

void Solver::cleanAllclause()
{
    watches.cleanAll();
    watchesBin.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            watches[p].clear();
            watchesBin[p].clear();
        }
    clauses.clear();
    learnts.clear();
}

//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
    // All watchers:
    //
    watches.cleanAll();
    watchesBin.cleanAll();
    
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            vec<Watcher>& ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
            vec<Watcher>& ws2 = watchesBin[p];
            for (int j = 0; j < ws2.size(); j++)
                ca.reloc(ws2[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++){
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++)
        ca.reloc(learnts[i], to);

    // All original:
    //
int i, j;
    for (i = j = 0; i < clauses.size(); i++)
        if (ca[clauses[i]].mark() != 1){
            ca.reloc(clauses[i], to);
            clauses[j++] = clauses[i]; }
    clauses.shrink(i - j);

//    for (int i = 0; i < clauses.size(); i++)
//        ca.reloc(clauses[i], to);
}


void Solver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted()); 

    relocAll(to);
    if (verbosity >= 2)
        printf("c |  Garbage collection:   %12d bytes => %12d bytes             |\n", 
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

//------------------------------------------------------------------------------

lbool Solver::dumpClause(vec<CRef>& cs,Solver* fromSolver,Solver* toSolver,int sizelimit) 
{    int i,j;
     vec<Lit> lits;
     for (i =0; i < cs.size(); i++){
                Clause & c = fromSolver->ca[cs[i]];
 		int sz=c.size();
           	if(sz > 6 && sizelimit==2) continue;
		lits.clear();
		for (j = 0; j < sz; j++) {
                        Lit lit=c[j];
			if (toSolver->value(lit) == l_True) break;
   			if (toSolver->value(lit) == l_False) continue;
                        lits.push(lit);	
		}
                if(j<sz) continue;
            	if(sizelimit==2 && lits.size() > 2) continue;
 //add clause   
                if(lits.size()==0) return l_False;
                if(lits.size() == 1){
                       toSolver->uncheckedEnqueue(lits[0]);
                       if(toSolver->propagate() != CRef_Undef ) return l_False;
                }
                else{
                    CRef cr = toSolver->ca.alloc(lits, false);
                    toSolver->clauses.push(cr);
                    toSolver->attachClause(cr);
                }
      }
      return l_Undef;
}

CRef Solver::trypropagate(Lit p)//new idea
{
    newDecisionLevel();
    uncheckedEnqueue(p);
    return propagate();
}    

float *fweight;

inline float Solver::dynamicWeight() //new idea 
{   float w=0.01;
    for (int c = trail.size()-1; c >= trail_lim[0]; c--) w+=fweight[trail[c].x];
    cancelUntil(0);
    return w;
}

void Solver::LitWeight(vec<CRef>& cs) //new idea
{
   int size=2*nVars()+2;
   fweight=(float *) malloc(sizeof(float) *(size));
   int i, j;
   for( i = 0; i <size; i++ ) fweight[i]=0;
   for (i = 0; i < cs.size(); i++){
            Clause& c = ca[cs[i]];
 	    int sz=c.size()-1;
   	    if(sz>80) continue; 
            for (j = 0; j <= sz; j++) fweight[c[j].x]+=size_diff[sz];
    }
}

lbool Solver :: run_subsolver(Solver* & newsolver, Lit plit, int conf_limit,int fullsolve)
{
        int nV=nVars();
        newsolver=new Solver();

        newsolver->subformuleClause=1;
        newsolver->needExtVar = needExtVar;

        while (nV > newsolver->nVars()) newsolver->newVar();
        newsolver->recursiveDepth=recursiveDepth+1;
       
        for(int i=0; i<nV; i++){
	      if(assigns[i] != l_Undef){
		       Lit lt = (assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                       newsolver->uncheckedEnqueue(lt);
               }
        }
            
        if(value(var(plit)) == l_Undef){
               newsolver->uncheckedEnqueue(plit);
        }
        lbool ret=dumpClause(clauses, this, newsolver, 10000000);
        if(ret == l_False) goto run_end;
        ret=dumpClause(learnts, this, newsolver, 2);
        if(ret == l_False) goto run_end;
        
        if(newsolver->propagate() != CRef_Undef ){
              ret=l_False;
run_end:
              goto run_end2;
        }     
        newsolver->conflict_limit=conf_limit;
        newsolver->completesolve=fullsolve;

        newsolver->setTermCallback (termCallbackState, termCallback);
        newsolver->setLearnCallback(learnCallbackState,learnCallbackLimit,learnCallback);
        ret=newsolver->solveNoAssump();
run_end2:
        return ret;
}      

void Solver :: ClearClause(vec<CRef>& cs,int deleteflag)
{   (void)deleteflag;
    for (int i = 0; i < cs.size(); i++) removeClause(cs[i]);
    cs.clear();
}

int Solver::Out_oneClause(Solver* solver,vec<CRef>& cs,Stack<int> * outClause, int lenLimit,int clause_type) 
{       int i,j,num=0;
	for (i =0; i < cs.size(); i++){
                Clause & c = solver->ca[cs[i]];
 		int sz=c.size();
	   	if(lenLimit==2 && sz>6) continue;
		int k=0;
		for (j = 0; j < sz; j++) {
			if (solver->value(c[j]) == l_True) break;
			if (solver->value(c[j]) == l_False) continue;
			k++;
		}
		if(j<sz) continue;
         	if(lenLimit==2 && k>2) continue;
		outClause->push(((k+1)<<FLAGBIT) | clause_type);
          	for (j = 0; j < sz; j++) {
			Lit p=c[j];
                        if (solver->value(p) == l_False) continue;
	                outClause->push(toIntLit(p));
         	}
                num++;
        }
        return num;
}

int Solver :: Output_All_Clause(Solver* solver)
{   int clause_type;
    if(g_pps->clause==0) {
            clause_type=CNF_CLS; 
            g_pps->clause=new Stack<int>;
    }
    else clause_type=CNF_C_B; 

    int Numclause = Out_oneClause(solver,solver->clauses, g_pps->clause, 1000000, clause_type);
    Numclause += Out_oneClause(solver,solver->learnts, g_pps->clause, 2,       clause_type);
    return Numclause;
}

lbool Solver::dumpClause(Solver* fromSolver, Solver* toSolver)
{
     for(int i=0; i<nVars(); i++){
	    if(toSolver->assigns[i] != l_Undef) continue;
	    if(fromSolver->assigns[i] != l_Undef){
		       Lit lit = (fromSolver->assigns[i] == l_True) ? mkLit(i) : ~mkLit(i);
                       toSolver->uncheckedEnqueue(lit);
            }
      }
  
     lbool ret=dumpClause(fromSolver->clauses, fromSolver, toSolver, 10000000);

     if(ret == l_False) return l_False; 
     return dumpClause(fromSolver->learnts, fromSolver, toSolver, 2);
}      

void Solver::copyAssign(Solver* fromSolver)
{
    for(int i=0; i<nVars(); i++){
	    if(assigns[i] != l_Undef) continue;
	    assigns[i]=fromSolver->assigns[i];
    }    
}

inline void Solver :: simpAddClause(const vec<Lit>& lits)
{
     CRef cr = ca.alloc(lits, false);
     clauses.push(cr);
     attachClause(cr);
}

void init_weight()
{
	if(size_diff) return;
	int i, longest_clause = 100; 
        size_diff = (float *) malloc(sizeof(float) * longest_clause );
	size_diff[0]=size_diff[1] = 0.0;
	for( i = 2; i < longest_clause; i++ )  size_diff[ i ] = (float)pow(5.0, 2.0-i);
}

lbool Solver :: subsolve()
{       cancelUntil(0);
        init_weight();
        g_pps->numVar=nVars();
	OutputClause(g_pps->clause, (int *)0, g_pps->numClause,0);

        int way=1;
        XOR_Preprocess(g_pps,way);

        int Mvar0;
        int rc0=findmaxVar(Mvar0);
        release_pps (g_pps);
        if(rc0 == UNSAT) return l_False;
      
        lbool ret;
        int mvar;
        float maxWeight;
        if( Mvar0 >=0 ){
             mvar=Mvar0;
             goto done;
        }
retry:	
        ret=l_Undef;
        mvar=-1;
        maxWeight=-1;
        LitWeight(clauses);
        for(int i=0; i<numVarRk; i++){
	      int var=extVarRank[i]-1;
	      if(assigns[var] != l_Undef) continue;
         
              Lit p0=~mkLit(var);
              Lit p1=mkLit(var);
              CRef confl0=trypropagate(p0);
              float w0=dynamicWeight();
              CRef confl1=trypropagate(p1);
              float w1=dynamicWeight();
              if (confl0 != CRef_Undef){
                    if (confl1 != CRef_Undef) {ret=l_False; break;}
                    if(value(p1) != l_Undef) continue; 
                   // uncheckedEnqueue(p1);
                   // propagate();
                    unitPropagation(p1);
                    continue;
              }
              if (confl1 != CRef_Undef){
                    if(value(p0) != l_Undef) continue; 
             //        uncheckedEnqueue(p0);
              //      propagate();
                    unitPropagation(p0);
                    continue;
              }
              if(maxWeight < w0*w1 && i<15 || mvar==-1){//15
                     maxWeight = w0*w1;
                     mvar=var;
              }
              if(i>30) break;
        }
        free(fweight); 
        if(ret==l_False) return l_False;
        if(mvar==-1) return l_True;
done:
        if(assigns[mvar] != l_Undef) goto retry;
        
        Solver* subsolver2;
        lbool ret2=run_subsolver(subsolver2, mkLit(mvar),-1,completesolve);
        if(ret2==l_True){
               copyAssign(subsolver2);
               delete subsolver2;
               return l_True;
        }
        delete subsolver2;
        if(ret2==l_Undef){
               cancel_subsolve=1;
               return l_Undef;
        }

        Solver* subsolver1;
        lbool ret1=run_subsolver(subsolver1, ~mkLit(mvar),-1,completesolve);
        if(ret1==l_True) {
                copyAssign(subsolver1);
                delete subsolver1;
                return l_True;
        }
        delete subsolver1;
        if(ret1==l_Undef){
               cancel_subsolve=1;
               return l_Undef;
        }
        return l_False;
}

void Solver::Out_removeClause(vec<CRef>& cs,Stack<int> * & outClause, int & numCls,int lenLimit,int Del) //new idea
{       int i,j,m=0;
	for (i =0; i < cs.size(); i++){
                Clause & c = ca[cs[i]];
 		int sz=c.size();
		int k=0;
		for (j = 0; j < sz; j++) {
			if (value(c[j]) == l_True) break;
			if(g_pps->unit){
                             int vv=var(c[j])+1;
		             if((vv+1)==g_pps->unit[vv]) break;
                        }
			if (value(c[j]) == l_False) continue;
			k++;
		}
		if(j<sz){
			 if(Del) removeClause(cs[i]);
			 continue;
		}
         	if(lenLimit==2 && k>2){
                        if(Del) cs[m++] = cs[i];
  			continue;
		}
		outClause->push(((k+1)<<FLAGBIT) | CNF_CLS);
          	for (j = 0; j < sz; j++) {
			Lit p=c[j];
                        if (value(p) == l_False) continue;
	                outClause->push(toIntLit(p));
         	}
	        if(Del) removeClause(cs[i]);
                numCls++;
	}
        if(Del) cs.shrink(i - m);
}

void Solver :: OutputClause(Stack<int> * & clauseCNF, int *solution, int & numCls,int Del)
{
    if(clauseCNF) delete clauseCNF;
    cancelUntil(0);
	
    if(solution){
       	 for (int i = nVars()-1; i >=0 ; i--){
		 if(solution[i+1]) continue;
		 if (assigns[i] != l_Undef){
                        if(assigns[i]==l_True) solution[i+1]=i+1;
			else                   solution[i+1]=-(i+1);
		 }
	 }
    }

    clauseCNF=new Stack<int>; 
    numCls=0;

    Out_removeClause(clauses, clauseCNF, numCls,10000000,Del); //new idea
    Out_removeClause(learnts, clauseCNF, numCls,2,Del);
}

//------------------------------------------------------
int Solver :: simp_solve()
{    int i,j;

     mainSolver=this;
     int *ext2ite=(int *) calloc (nVars()+1, sizeof (int));
     Solver * s_solver=new Solver();
     s_solver->subformuleClause=1;
//assume
     for(i=0; i<assumptions.size(); i++){
          Lit p = assumptions[i];
          if (value(p) == l_True) continue;
          if (value(p) == l_False){
                delete s_solver;
                return UNSAT;
          }
          int v=var(p)+1;
          if(ext2ite[v]==0){
               s_solver->newVar();
               ext2ite[v]=s_solver->nVars();
          }
          int ivar=ext2ite[v]-1;
          Lit ilit= sign(p) ? ~mkLit(ivar)  : mkLit(ivar);
          s_solver->uncheckedEnqueue(ilit);
          if(s_solver->propagate() != CRef_Undef){
                delete s_solver;
                return UNSAT;
          }
      }
//
      vec<Lit> lits;
      for (i =0; i < clauses.size(); i++){
           Clause & c = ca[clauses[i]];
 	   int sz=c.size();
	   lits.clear();
           for (j = 0; j < sz; j++) {
		Lit p=c[j];
          	if (value(p) == l_True) goto nextc;
		if (value(p) == l_False) continue;
	        int v=var(p)+1;
                if(ext2ite[v]==0){
                       s_solver->newVar();
                       ext2ite[v]=s_solver->nVars();
                }
                int ivar=ext2ite[v]-1;
                Lit ilit= sign(p) ? ~mkLit(ivar)  : mkLit(ivar);
                lits.push(ilit);
	   }
           if(!s_solver->addClause_(lits)){
                delete s_solver;
                return UNSAT;
           }
        nextc:  ;
     }

      ite2ext=(int *) malloc(sizeof(int)*(s_solver->nVars()+1));
      for(int i=1; i<=nVars(); i++) ite2ext[ext2ite[i]]=i;
      free(ext2ite);

      vec<Lit> dummy;
      s_solver->needExtVar=1;
      s_solver->solvedXOR=solvedXOR;
      s_solver->setTermCallback(termCallbackState, termCallback);
      s_solver->setLearnCallback(learnCallbackState,learnCallbackLimit,learnCallback);
      lbool rc = s_solver->solveLimited2(dummy);
      if(rc==l_True || rc==l_Undef) {
           for (i=1; i <= s_solver->nVars(); i++){
                  int ev = ite2ext[i];
                  if(assigns[ev-1] != l_Undef) continue;
                  if(rc==l_True) assigns[ev-1]= s_solver->assigns[i-1];
                  else{
                       Lit p = (s_solver->assigns[i-1]==l_True) ? mkLit(ev-1)  : ~mkLit(ev-1);
                       uncheckedEnqueue(p);
                  }
            }
            free(ite2ext);
            ite2ext=0;
            delete s_solver;
            if(rc==l_Undef) return _UNKNOWN;
            return SAT; //10
    }
    free(ite2ext);
    ite2ext=0;
    delete s_solver;
    return UNSAT;//res=20
}

//--------------------------------------------------------
//look both by an iterative uint propagation
int Solver :: IterativeUnitPropagation(int lit0, int * Queue, float *diff,int *unit,int *pre_unit)
{   int i,ForcedLit=0;
    int lt,var,sp,sp1;
    int rc=_UNKNOWN;
    PPS *pps = g_pps;
    Stack<int> * clause=pps->clause;
    Stack<int> * occCls=pps->occCls;
    Queue[0]=unit[ABS(lit0)]=lit0;
    Lit pp0 = mkLit(ABS(lit0)-1);

    sp1=0; sp=1;*diff=1;
    while(sp1<sp){
	    lt=Queue[sp1++];
	    int numocc = occCls[-lt];
	    for (i = 0; i < numocc; i++) {
		 int cpos=occCls[-lt][i];
                 int len=(*clause)[cpos];
		 int markf=len & MARKBIT;
                 if(markf!=CNF_CLS) continue;
	   	 len=len>>FLAGBIT;
 		 int *clsp=&(*clause)[cpos];
		 int *lits=clsp+1;
                 clsp+=len;
		 int m=0;
                 for (; lits<clsp; lits++){
			 var=ABS(*lits); 
			 if(unit[var]==*lits) goto nextC;
                      
                         int iVar=var-1;
                         if(assigns[iVar] != l_Undef){
                                if( (*lits > 0) && (assigns[iVar] == l_True)) goto nextC;  
                                if( (*lits < 0) && (assigns[iVar] == l_False)) goto nextC;  
                                continue;
			 }
                         
                         if(unit[var]==0) {m++; ForcedLit=*lits;}
		  }
		  if(m==0) {rc=UNSAT; goto ret;}; //conflict
		  if(m==1) {
                          int fvar=ABS(ForcedLit);
			  Queue[sp++]=unit[fvar]=ForcedLit;
                         
                           if(pre_unit){
                               if(unit[fvar]==pre_unit[fvar]){
	                          int iVar=fvar-1;
                                  if(assigns[iVar] != l_Undef) continue;
                  	          Lit ulit=(unit[fvar] > 0) ? mkLit(iVar) : ~mkLit(iVar);
                                 // uncheckedEnqueue(ulit);
                                 // CRef confl = propagate();
                                 if(!addbinclause(pp0,  ulit)) continue;
                                 if(!addbinclause(~pp0, ulit)) continue;
                                 CRef confl = unitPropagation(ulit);
                                 if (confl != CRef_Undef) {Queue[sp]=0; return UNSAT;} 
                             }
                          }
                        
		  }
		  else{
			 if(m<100) (*diff)+=size_diff[m];
		  }
nextC:          ;
		}
      }
ret:
//     for(i=0; i<sp; i++) unit[ABS(Queue[i])]=0;
     Queue[sp]=0;
     return rc;
}

void setDoubleDataIndex(PPS *pps, bool add_delcls);
void clear_unit(int *Queue,int *unit)
{    
     for(int i=0; Queue[i]; i++) unit[ABS(Queue[i])]=0;
}

int Solver :: findmaxVar(int & Mvar)
{   	
    setDoubleDataIndex(g_pps,false);
    if(g_pps->unit){
           free(g_pps->unit);
           g_pps->unit=0;
    } 
    int *Queue0=(int *)calloc (4*g_pps->numVar+4, sizeof (int));
    int *Queue1=Queue0 + g_pps->numVar+1;
    int *unit0=Queue1 + g_pps->numVar+1;
    int *unit1=unit0 + g_pps->numVar+1;
    float max=-1,diff1,diff2;
    Mvar=-1;
    for(int i=0; i<numVarRk && i < 100; i++){
 	     int eVar=extVarRank[i];
	     if(assigns[eVar-1] != l_Undef) continue;
             int rc1=IterativeUnitPropagation(eVar,  Queue0, &diff1, unit0, (int *)0);
	     int rc2=IterativeUnitPropagation(-eVar, Queue1, &diff2, unit1, unit0);
             clear_unit(Queue0,unit0);
             clear_unit(Queue1,unit1);
             CRef confl = CRef_Undef;
             if(rc1==UNSAT){
		  Lit p = ~mkLit(eVar-1); // ~eVar
                  //if(value(p) == l_Undef) uncheckedEnqueue(p);
                  confl = unitPropagation(p);
                  if(rc2==UNSAT) {
                        free(Queue0); 
                        return UNSAT;
                  }
             }
	     else{
        	  if(rc2==UNSAT){
                       Lit p = mkLit(eVar-1); // eVar
                       if(value(p) == l_Undef){
                         //   uncheckedEnqueue(p);
                          confl = unitPropagation(p);
                      }
                  }
             }
            // CRef confl = propagate();
             if (confl != CRef_Undef) {
                     free(Queue0); 
                     return UNSAT;
             }
             if(rc1==UNSAT || rc2==UNSAT) continue;
	     float diff=diff1*diff2;
	     if(diff>max){
		       max=diff;
		       Mvar=eVar-1;
	      }
	}
	free(Queue0);
	return _UNKNOWN;
}

void Solver :: verify()
{
    for (int i = 0; i < clauses.size(); i++){
        Clause& c = ca[clauses[i]];
        if (!satisfied(c)){
            printf("c {");
            for (int j = 0; j < c.size(); j++){
               if(sign(c[j])) printf("-");
               printf("%d", var(c[j])+1);
               if(value(c[j]) == l_True) printf(":1 ");
               else printf(":0 ");
            }
            printf(" }\n");
            exit(0);
        }
    }   
    if (verbosity > 0) printf("c verified by abcdSAT_i17 \n");
}
/*
int Solver :: findclause()
{   printf("c findclause \n");
    int ret=0;
    for (int i = 0; i < clauses.size(); i++){
        Clause& c = ca[clauses[i]];
        int cnt=0;
        for (int j = 0; j < c.size(); j++){
             if(var(c[j]) == 3072 ) cnt++;
        }
        if(cnt){
            printClause(clauses[i]);
        }
    }
    return ret;   
}

void Solver :: copyclause()
{ 
   for (int i = 0; i < clauses.size(); i++){
        Clause& c = ca[clauses[i]];
        for (int j = 0; j < c.size(); j++) orgclauses.push(c[j]);
        orgclauses.push(lit_Undef);
   }
}
*/
bool Solver::satisfied(vec <Lit> & c) {
    for (int i = 0; i < c.size(); i++){
        if (value(c[i]) == l_True) return true;
    }
    return false; 
}
/*
void Solver :: verifyOrg()
{
    printf("c verifyOrg \n");
    int j,k=0;
    for (int i = 0; i < orgclauses.size(); ){
         vec <Lit> lits;
         for (j = i; j < orgclauses.size(); j++){
             if(orgclauses[j] == lit_Undef){ i = j+1; break;}
             lits.push(orgclauses[j]);
         }
         k++;
         if (!satisfied(lits)){
            printf("c k=%d sz=%d {",k,lits.size());
            for (int j = 0; j < lits.size(); j++){
               if(sign(lits[j])) printf("-");
               printf("%d", var(lits[j])+1);
               if(value(lits[j]) == l_True) printf(":1 ");
               else if(value(lits[j]) == l_False) printf(":0 ");
                    else printf(":Unknown ");
            }
            printf(" }\n");
            return;
        }
    }   
}
*/
// a bin clasue exists?
bool Solver::hasBin(Lit p, Lit q)
{
    vec<Watcher>&  wbin  = watchesBin[~p];
    for(int k = 0;k<wbin.size();k++) {
	  Lit imp = wbin[k].blocker;
	  if( imp == q) return true;
    }
    return false;
}	  

inline void Solver::setmark(vec<int> & liftlit)
{   liftlit.clear();
    for (int c = trail.size()-1; c >= trail_lim[0]; c--) {
            int lit=toInt(trail[c]);
            mark[lit]=1;
            liftlit.push(lit);
    }
    cancelUntil(0);
}

inline void Solver::clearmark(vec<int> & liftlit)
{
     for (int i =0; i < liftlit.size(); i++) mark[liftlit[i]]=0;
}

lbool Solver:: addequ(Lit p, Lit q)
{ 
    int pul=toInt(p);
    int qul=toInt(q);
 
    if(pul == qul)  return l_Undef;
    if(pul == (qul^1)) return l_False;

    touchVar.push(var(p));
    touchVar.push(var(q));
    return add_equ_link(pul,qul);
}

void Solver:: equlinkmem()
{
    if (equMemSize >= nVars()+1) return;
    int oldsize=equMemSize;
    equMemSize = nVars()+1;
    if(equhead == 0){
          equhead = (int * ) calloc (equMemSize, sizeof(int));
          equlink = (int * ) calloc (equMemSize, sizeof(int));
          return;
    }
    equhead = (int * )realloc ((void *)equhead, sizeof(int)*equMemSize);
    equlink = (int * )realloc ((void *)equlink, sizeof(int)*equMemSize);
    for(int i=oldsize; i<equMemSize; i++) equhead[i]=equlink[i]=0;
} 

lbool Solver:: add_equ_link(int pul, int qul)
{
    equlinkmem();
    if( pul < qul){
       int t=pul;  pul=qul;  qul=t;
    }
    int pl=toIntLit(toLit(pul));
    int ql=toIntLit(toLit(qul));

    if(pl<0){ pl=-pl; ql=-ql; }
    int qv=ABS(ql);
    if(equhead[pl] == equhead[qv]){
        if(equhead[pl] == 0){
           equhead[pl]=ql; equhead[qv]=qv;
           equlink[qv]=pl;
           equsum++;
           return l_Undef;
        }
        if(ql < 0) return l_False;
        return l_Undef;
    }
    if(equhead[pl] == -equhead[qv]){
        if(ql < 0) return l_Undef;
        return l_False;
    }
    equsum++;
    if(equhead[pl] == 0 && equhead[qv]){
        if(ql < 0) equhead[pl]=-equhead[qv];
        else equhead[pl]=equhead[qv];
        int h=ABS(equhead[pl]);
        int t = equlink[h];
        equlink[h]=pl;
        equlink[pl]=t;
        return l_Undef;
    }
    if(equhead[pl] && equhead[qv]==0){
        if(ql < 0) equhead[qv]=-equhead[pl];
        else equhead[qv]=equhead[pl];
        int h=ABS(equhead[qv]);
        int t = equlink[h];
        equlink[h]=qv;
        equlink[qv]=t;
        return l_Undef;
    }
//merge
    int x=equhead[pl];
    int y=equhead[qv];
    if(ql < 0) y=-y;
    int next=ABS(y);
    while(1){
         if(equhead[next]==y) equhead[next]=x;
         else  equhead[next]=-x;
         if(equlink[next]==0) break;
         next=equlink[next];
    }    
    int xh=ABS(x);
    int t=equlink[xh];
    equlink[xh]=ABS(y);
    equlink[next]=t;
    return l_Undef;
}

void Solver:: buildEquClause()
{   vec<Lit> lits;
    if(equhead==0) return;
 
    if(dummyVar == 0)  dummyVar = (char * ) calloc (nVars()+1, sizeof(char));
    equlinkmem();
    for (int i=1; i <= nVars(); i++){
         if(equhead[i]==0 || equhead[i]==i || dummyVar[i]) continue;
         Lit p=mkLit(i-1);
         int lit=equhead[i];
         dummyVar[i]=1;
         Lit q = (lit>0) ? mkLit(lit-1) : ~mkLit(-lit-1);
         if( !hasBin(p, ~q) ) {
                lits.clear();
                lits.push(p);
                lits.push(~q);
                simpAddClause(lits);
         }
         if(!hasBin(~p, q) ) {
                lits.clear();
                lits.push(~p);
                lits.push(q);
                simpAddClause(lits);
         }
    }
}


void Solver:: solveEqu(int *equRepr, int equVars, vec<lbool> & Amodel)
{   
   if(equRepr==0) return;
   if (verbosity > 0) printf("c solve Equ Vn=%d \n",equVars);
   for (int i=1; i <= equVars; i++){
         if(equRepr[i]==0 || equRepr[i]==i) continue;
         int v=equRepr[i];
         v=ABS(v)-1;
         Amodel[i-1] = l_False;
         if (Amodel[v] == l_Undef) Amodel[v] = l_True;
         if (Amodel[v] == l_True){
                   if(equRepr[i] > 0) Amodel[i-1] = l_True;
         }
         else      if(equRepr[i] < 0) Amodel[i-1] = l_True;
    }
}

/*
void Solver:: printEqu(int *equRepr)
{   
   if(equRepr==0) return;
   for (int i=1; i <= nVars(); i++){
         if(equRepr[i]==0 || equRepr[i]==i) continue;
         printf("<%d=%d>", i, equRepr[i]);
    }
}
*/

//hyper bin resol
lbool Solver:: simpleprobehbr (Clause & c)
{   vec<int> ostack;
    vec<Lit> lits;
    ostack.clear();
    CRef confl= CRef_Undef;
    int sum=0,cnt=0;
    int maxcount=0;
    lbool ret=l_Undef;

    for (int i =0; i < 3; i++){
        Lit lit=c[i];
        vec<Watcher>&  bin  = watchesBin[lit];
        if(bin.size()) cnt++;
    }
    if(cnt <= 1) goto done;
    for (int i =0; i < 3; i++){
        Lit lit=c[i];
        int litint=toInt(lit);
        sum += litint;
        vec<Watcher>&  bin  = watchesBin[lit];
        for(int k = 0;k<bin.size();k++) {
	      Lit other = bin[k].blocker;
              int oint = toInt(other);
              if(mark[oint]) continue;
              int nother = oint^1;
              if(mark[nother]) {//unit
                    if(value(lit) != l_Undef) goto done;
                   // uncheckedEnqueue(~lit);
                   // confl=propagate();
                    confl = unitPropagation(~lit);
                    goto done;
              }
              count[oint]++;
              if(maxcount < count[oint]) maxcount = count[oint];
              litsum[oint] += litint;
              mark[oint] =1;
              ostack.push(oint);
       }
       for(int k = 0;k<bin.size();k++) {
 	    Lit other = bin[k].blocker;
            mark[toInt(other)]= 0;
       }
   }
   if(maxcount < 2 ) goto done;
   for (int i =0; i < ostack.size(); i++){
           int oint=ostack[i];
           if(count[oint]<=1) continue;
           Lit other = toLit(oint);
           if(count[oint]==3){//unit
                  if(value(other) != l_Undef) continue; //bug 2016.3.9
                  //uncheckedEnqueue(other);
                 // confl=propagate();
                  confl = unitPropagation(other);
                  if (confl != CRef_Undef) goto done;
                  continue;
           }
           int litint = sum - litsum[oint];
           Lit lit = toLit(litint);
           if(other == lit) {//unit
                  if(value(other) != l_Undef) continue; 
              //    uncheckedEnqueue(other);
               //   confl=propagate();
                  confl = unitPropagation(other);
                  if (confl != CRef_Undef) goto done;
                  continue;
           }
           if(other == (~lit)) continue;
           if(hasBin(other, lit)) continue;

           hbrsum++;
           lits.clear();
           lits.push(other);
           lits.push(lit);

           simpAddClause(lits); //add <other, lit>
         
           if(hasBin(~other, ~lit)){//other=~lit
                   ret=addequ(other, ~lit);
                   if(ret == l_False){
                         goto done;
                   }
           }
           else{
                   touchVar.push(var(other));
                   touchVar.push(var(lit));
           }               
  }              
done:
  for (int i =0; i < ostack.size(); i++){
        int oint=ostack[i];
        count[oint]=litsum[oint]=0;
        mark[oint]=0;
  }     
  if (confl != CRef_Undef) return l_False;
  return ret;
}

lbool Solver::removeEqVar(vec<CRef>& cls, bool learntflag)
{
    vec<Lit> newCls;
    int i, j,k,n;

    lbool ret=l_Undef;
    equlinkmem();
    
    for (i = j = 0; i < cls.size(); i++){
        Clause& c = ca[cls[i]];
        if(c.mark()) continue;
        if(ret!=l_Undef){
            cls[j++] = cls[i];
            continue;
        }
        newCls.clear();
        int T=0;
        int del=0;
        for (k = 0; k < c.size(); k++){
             Lit p=c[k];
             if (value(p) == l_True) {
                    del=T=1; 
                    break;
             }
             if (value(p) == l_False){
                     del=1; 
                     continue;
             }
             int v = var(p)+1;
             Lit q;
             if(equhead[v]==0 || equhead[v]==v) q=p;
             else{ int lit;
                 if(sign(p)) lit = -equhead[v];
                 else        lit = equhead[v];
                 q = (lit>0) ? mkLit(lit-1) : ~mkLit(-lit-1);
                 del=1;
            }
             if(del){
                for(n=newCls.size()-1; n>=0; n--){
                   if( q  == newCls[n]) break;
                   if( ~q == newCls[n]) {T=1; break;}
                }
             }
             else n=-1;
             if(T) break;
             if(n<0) newCls.push(q);
        }
        if(del==0){
            cls[j++] = cls[i];
            continue;
        }
        if(T){
           removeClause(cls[i]);
           continue;
        }
        if(touchMark && newCls.size()<3){
            for (k = 0; k < newCls.size(); k++) touchMark[var(newCls[k])]=1;
        }
        if(newCls.size()==0){
              cls[j++] = cls[i];
              ret=l_False;
              continue;
       }
       if(newCls.size()==1){
              removeClause(cls[i]);
              if( unitPropagation(newCls[0]) != CRef_Undef ) ret=l_False;
              unitsum++;
              continue;
       }
       removeClause(cls[i]);
       CRef cr = ca.alloc(newCls, learntflag);
       attachClause(cr);
       cls[j++] = cr;
    }
    cls.shrink(i - j);
    checkGarbage();
    return ret; 
}

bool Solver:: addbinclause(Lit p, Lit q)
{
      if(hasBin(p,q)) return true;
      vec<Lit> ps;
      ps.clear();
      ps.push(p);
      ps.push(q);
      if(is_conflict(ps)){
           CRef cr = ca.alloc(ps, false);
           clauses.push(cr);
           attachClause(cr);
           return true;
      }
      return false;
}

void Solver:: addbinclause2(Lit p, Lit q, CRef & cr, int learntflag)
{
      vec<Lit> ps;
      ps.push(p);
      ps.push(q);
      cr = ca.alloc(ps, learntflag);
      attachClause(cr);
}

bool Solver:: out_binclause(Lit p, Lit q)
{
      if(hasBin(p,q)) return true;
      vec<Lit> ps;
      ps.clear();
      ps.push(p);
      ps.push(q);
      if(is_conflict(ps)) return true;
      return false;
}

lbool Solver::replaceEqVar()
{  
     watches.cleanAll();
     watchesBin.cleanAll();
     if(equhead==0) return l_Undef;
    
     lbool ret=removeEqVar(clauses, false);
     if(ret == l_False) return l_False;
     ret=removeEqVar(learnts, true);
     watches.cleanAll();
     watchesBin.cleanAll();
     return ret;
}

lbool Solver::probeaux()
{    
     int old_equsum = equsum;
     for (int i = 0; i<clauses.size() && i<400000; i++){
           Clause & c = ca[clauses[i]];
 	   int sz=c.size();
           if(sz!=3) continue;
	   int tcnt=0;
           for (int j = 0; j < sz; j++) {
                tcnt += touchMark[var(c[j])];
                if(value(c[j]) != l_Undef) goto next;
           }
           if(tcnt < 2) continue;
           if(simpleprobehbr (c) == l_False) return l_False;
next:      ;
     }
//lift
    vec<int> liftlit;
    vec<int> unit;
    int liftcnt;
    if(probeSum) liftcnt = nVars()/2;
    else liftcnt=10000;
    equlinkmem();
    for(int vv=0; vv<nVars() && liftcnt > 0; vv++){
	     if(touchMark[vv]==0) continue;
             if(assigns[vv] != l_Undef) continue;
             if(equhead)
                if(equhead[vv+1] && ABS(equhead[vv+1]) <= vv) continue;

            Lit p = mkLit(vv);
            int pos=watchesBin[p].size();
            int neg=watchesBin[~p].size();

            if(pos==0 || neg==0) continue;
            liftcnt--;
            if(pos < neg) p=~p;
            CRef confl=trypropagate(p);
            if (confl != CRef_Undef){
                    cancelUntil(0);
                    if(value(p) != l_Undef) continue; 
                    confl=unitPropagation(~p);
                    if (confl != CRef_Undef) return l_False;
                    unitsum++;
                    continue;
            }
            setmark(liftlit);
            confl=trypropagate(~p);
            if (confl != CRef_Undef){
                    cancelUntil(0);
                    clearmark(liftlit);
                    if(value(p) != l_Undef) continue;
                    unitPropagation(p);
                    unitsum++;
                    continue;
            }
            unit.clear();
            vec<Lit> eqLit;
            eqLit.clear();
            for (int c = trail.size()-1; c > trail_lim[0]; c--) {//not contain p
                 int lit=toInt(trail[c]);
                 if(mark[lit]) unit.push(lit);
                 else{
                      if(mark[lit^1]) {//equ p=~lit
                          Lit q = ~trail[c];//~toLit(lit); 
                          eqLit.push(q);
                          unit.push(lit);
                       }
                 }
             }
            clearmark(liftlit);
            cancelUntil(0);
//p=q
             for (int i =0; i< eqLit.size(); i++) {
                     Lit q=eqLit[i];
                     lbool ret=addequ(p,q);
                     if(ret == l_False) return l_False;
            }
//unit  
            for (int i =0; i < unit.size(); i++){
                  int lit=unit[i];
              	  Lit uLit=toLit(lit);
	          if(value(uLit) != l_Undef) continue;
                  if(!addbinclause(p,  uLit)) continue;
                  if(!addbinclause(~p, uLit)) continue;
                  confl=unitPropagation(uLit);
                  if (confl != CRef_Undef) return l_False;
                  unitsum++;
            }
   }
  
   for (int i =0; i < nVars(); i++) {
         touchMark[i]=0;
         if(equhead){
             int exv=i+1;
             if(equhead[exv]==0 || equhead[exv]==exv) continue;
         }
         decision[i]=1;
   }
  
   for (int i =0; i < touchVar.size(); i++) touchMark[touchVar[i]]=1;
   if(old_equsum != equsum) return replaceEqVar();
   return l_Undef;
}

lbool Solver::probe()
{    if(verbosity>0) printf("c probe \n");
    
     lbool ret=l_Undef;
     count = (int* ) calloc (2*nVars()+1, sizeof(int));
     litsum = (int* ) calloc (2*nVars()+1, sizeof(int));
     mark = (char * ) calloc (2*nVars()+1, sizeof(char));
     touchMark = (char *) malloc(sizeof(char) * (nVars()+1));

     for (int i =0; i < nVars(); i++)  touchMark[i]=1;
     int nC=nClauses();
     nC += (nC/2);
     int m=0;
     if(hbrsum > 1000 && conflicts>5000000 || (recursiveDepth && conflicts>300000)) goto noprobe;
     while(hbrsum<nC){
          m++;
          touchVar.clear();
          int oldhbr=hbrsum;
          int oldequ=equsum;
          int olduni=unitsum;
          ret=probeaux();
          if(ret == l_False) break;
          if(recursiveDepth || probeSum>3 || conflicts>100000 && m>25000) break;        
          if(oldhbr==hbrsum && oldequ==equsum && olduni==unitsum) break;
      }
noprobe:
     if(verbosity>0) printf("c hyper bin resol#=%d equ#=%d uni#=%d \n",  hbrsum,equsum,unitsum);
    
     touchVar.clear();
     free(count);
     free(litsum);
     free(mark);
     free(touchMark);
     touchMark=0;
     probeSum++;
     return ret;
}


extern int nVarsB;
extern int sumLit;
void MixBCD(int pure_mode); 

void Solver :: varOrderbyClause()
{  
   int rng=5;
   for (Var v = 0; v < originVars; v++) varRange[v]=0;    
   for (int i = 0; i < nClausesB-5; i++){
      if(Bclauses[i][2]==0) continue;
      int *plit = Bclauses[i];
      int v=ABS(*plit)-1;
      if(varRange[v]==0){
          varRange[v]=i;
          if(varRange[v]<rng) varRange[v]=rng;
      }
   }
   for (int i = 0; i < nClausesB-5; i++){
      if(Bclauses[i][2]) continue;
      int *plit = Bclauses[i];
      int v=ABS(*plit)-1;
      if(varRange[v]==0){
          varRange[v]=i;
          if(varRange[v]<rng) varRange[v]=rng;
      }
   }
}

int Solver :: longClauses()
{  int cnt=0;
   int minsz=0x7ffffff;
   int maxsz=0;
   for (int i = clauses.size()-1; i >= 0; i--){
      Clause& c = ca[clauses[i]];
      if(c.size()>2) cnt++;
      if(c.size()>maxsz) maxsz=c.size();
      if(c.size()<minsz) minsz=c.size();
   }
   if(maxsz==minsz && maxsz<10 || clauses.size()>3000000) LBDmode=0;
   return cnt;
}
 
void Solver :: fast_bcd()
{  
   nVarsB = nVars();
   if( nVarsB < 60 || varRange) return;
   nClausesB=clauses.size();
   sumLit=0;   
   for (int i = 0; i < nClausesB; i++){
      Clause& c = ca[clauses[i]];
      sumLit += c.size() + 1;
   }
   Bclauses = (int**) malloc(sizeof(int*) * (nClausesB+1));
   Bclauses[0] = (int*) malloc (sizeof(int)*sumLit);
   int j; 
   for (int i = 0; i < nClausesB; i++){
      Clause& c = ca[clauses[i]];
      for (j = 0; j < c.size(); j++){
           Bclauses[i][j] = toIntLit(c[j]);
      }
      Bclauses[i][j]=0;
      Bclauses[i+1] = Bclauses[i] +j+1;
  }
  MixBCD(0);

  varRange = (int* )calloc (nVars(), sizeof(int));
  varOrderbyClause();
}
//----------------------------------------------------------------
// find equivalent literals by 
//Tarjan's strongly connected components algorithm, dfsi: depth first search index
lbool Solver :: tarjan_SCC () 
{
  int * dfsimap, * min_dfsimap, lit;
  int dfsi, mindfsi, repr,olit;
  lbool ret=l_Undef;
  int nvar=nVars();

  dfsimap      = (int* ) calloc (2*nvar+1, sizeof(int));
  min_dfsimap  = (int* ) calloc (2*nvar+1, sizeof(int));
  dfsimap     += nvar;
  min_dfsimap += nvar;
  watchesBin.cleanAll();

  vec<int>  stk,component;
  stk.clear();  component.clear();
  dfsi = 0;
  int eqvNum=0;
  for (int idx = 1; idx <= nvar; idx++) {
     if(assigns[idx-1] != l_Undef) continue;
     for (int sign = -1; sign <= 1; sign += 2) {
        lit = sign * idx;
        if (dfsimap[lit]) continue;
        stk.push(lit);
        while (stk.size()>0) {
	    lit = stk.pop_();
            if (lit) {
	        if (dfsimap[lit]) continue;
	        dfsimap[lit] = min_dfsimap[lit] = ++dfsi;
	        component.push(lit);
	        stk.push(lit);
                stk.push(0);
		Lit Lt = makeLit(lit);
                vec<Watcher>&  bin  = watchesBin[Lt];
                for(int k = 0;k<bin.size();k++) {
	             Lit other = bin[k].blocker;
                     int olit = toIntLit(other);
                     if(dfsimap[olit]) continue;
	             stk.push(olit);
	        }  
            } 
            else {
	        lit = stk.pop_();
                mindfsi = dfsimap[lit];
		Lit Lt = makeLit(lit);
                vec<Watcher>&  bin  = watchesBin[Lt];
                for(int k = 0;k<bin.size();k++) {
	             Lit other = bin[k].blocker;
                     int olit = toIntLit(other);
                     if (min_dfsimap[olit] < mindfsi) mindfsi = min_dfsimap[olit];
	        }   
                if (mindfsi == dfsimap[lit]) {
                      repr = lit;
                      for (int k = component.size()-1; (olit =component[k]) != lit; k--) {
	                  if (ABS(olit) < ABS(repr)) repr = olit;
	              }
	              Lit p=makeLit(repr);
                      while ((olit = component.pop_()) != lit) {
	                   min_dfsimap[olit] = INT_MAX;
	                   if (olit == repr) continue;
	                   if (olit == -repr) {//empty clause 
                                 ret =l_False; 
	                          goto DONE;
                            }
                            Lit q=makeLit(olit);
                            ret = check_add_equ (p, q);
                            if(ret == l_False){
                                  goto DONE;
                            }
                            eqvNum++;
                      }
                      min_dfsimap[lit] = INT_MAX;
     	       } 
               else min_dfsimap[lit] = mindfsi;
	  }
        }
      }
  }
DONE:
  dfsimap     -= nvar;
  min_dfsimap -= nvar;
  free(dfsimap);
  free(min_dfsimap);
  if(ret == l_False) return ret;
  printf("c tarjan SCC eqvNum=%d equsum=%d \n",eqvNum,equsum);
  if(eqvNum>0){
      touchMark=0;   
      return replaceEqVar();
  }
  return l_Undef;
}

typedef struct RNG { unsigned z, w; } RNG;
RNG rng;
unsigned s_rand () 
{
  unsigned res;
  rng.z = 36969 * (rng.z & 65535) + (rng.z >> 16);
  rng.w = 18000 * (rng.w & 65535) + (rng.w >> 16);
  res = (rng.z << 16) + rng.w;
  return res;
}

void init_rand_seed () 
{
  rng.w = 0;
  rng.z = ~rng.w;
  rng.w <<= 1;
  rng.z <<= 1;
  rng.w += 1;
  rng.z += 1;
  rng.w *= 2019164533u, rng.z *= 1000632769u;
}

unsigned s_gcd (unsigned a, unsigned b) {
  unsigned tmp;
//  if (a < b) SWAP2 (a, b);
  if (a < b) Swap (a, b);
  while (b) tmp = b, b = a % b, a = tmp;
  return a;
}

lbool Solver :: distill() 
{
  unsigned int pos, i, bin, delta, ncls;
  ncls = clauses.size(); 
  vec<CRef>& cs = clauses;
  for (i = bin=0; i < ncls; i++){
        Clause& c = ca[cs[i]];
        if(c.size()>2) continue;
        if(bin != i ){
             CRef tmp = cs[bin];
             cs[bin]  = cs[i];
             cs[i]    = tmp;
        }
        bin++;
  }
  
  unsigned int mod=ncls-bin;
  if (!mod)  return l_Undef;
  pos   = s_rand () % mod;
  delta = s_rand () % mod;
  if (!delta) delta++;
  while (s_gcd (delta, mod) > 1)   if (++delta == mod) delta = 1;
 
  for (i = bin; i < ncls; i++){
        unsigned int k=bin+pos;
        CRef tmp= cs[i];
        cs[i]   = cs[k];
        cs[k]   = tmp;
        pos = (pos+delta) % mod;
  }
  lbool ret=distillaux(clauses, 0);
  return ret;
}

void Solver::distill_analyze(vec<Lit> & out_clause, Lit p)
{
    vec<Var> scan,visit;
    out_clause.push(p); 
    scan.push(var(p)); 
    seen[var(p)]=1;
    visit.push(var(p));
    while(scan.size()>0){
  	Var qv=scan.pop_();
        CRef  confl = reason(qv);
        if( confl != CRef_Undef ){
               Clause & c = ca[confl];
               for (int i =0; i < c.size(); i++){
                    p = c[i];
                    Var pv= var(p);    
                    if (seen[pv]) continue;
                    seen[pv]=1;
                    visit.push(pv);
                    if( reason(pv) != CRef_Undef) scan.push(pv);
                    else  out_clause.push(p);
                } 
         }
    }
   // for (int c = trail_lim[0]; c<trail.size(); c++)  seen[var(trail[c])] = 0;
    for (int v = 0; v<visit.size(); v++)  seen[visit[v]] = 0;
}

lbool Solver::distillaux(vec<CRef>& cls,int start)
{
    vec<Lit> newCls;
    int i,j,k,m=0;
    lbool ret=l_Undef;
    cancelUntil(0);
    if(start+30000 < cls.size()) start=cls.size()-30000;
    for (i = j = start; i < cls.size(); i++){
        if(ret!=l_Undef){
            cls[j++] = cls[i];
            continue;
        }
        Clause& c = ca[cls[i]];
        newCls.clear();
        int T=0;
        int csize=c.size();
        for (k = 0; k < csize; k++){
             Lit p=c[k];
             if (value(p) == l_True) { T=1; break;}
             if (value(p) == l_False)  continue;
             newCls.push(p);
        }
        if(T){
           removeClause(cls[i]);
           continue;
        }

        if(newCls.size()==csize && csize>2){

             for (int k=0; k<csize; k++){
                  Lit p = newCls[k];
                  if ( value(p) == l_False ) {
                         newCls[k]=newCls[csize-1];
                         newCls.shrink_(1);
                         break;
                  }
                  if (value(p) == l_True) {
                        if(k>=csize-1){
                           if(m<20000){
                              m++;
                              newCls.clear();
                              distill_analyze(newCls, p);
                          }
                        }
                        else  newCls.shrink_(csize-k-1);
                        break;
                  } 
                  newDecisionLevel();
                  uncheckedEnqueue(~p);
                  CRef confl = propagate();
                  if (confl != CRef_Undef) {
                      newCls.shrink_(csize-k-1);
                      break;
                 }
            }
            cancelUntil(0);
        }

       if(newCls.size()>=csize){
               cls[j++] = cls[i];
               continue;
       }
       if(newCls.size()==0){
              cls[j++] = cls[i];
              ret=l_False;
              continue;
       }
       if(newCls.size()==1){
              cls[j++] = cls[i];
              if( unitPropagation(newCls[0]) != CRef_Undef ) ret=l_False;
              unitsum++;
              continue;
       }
       removeClause(cls[i]);
       CRef cr = ca.alloc(newCls, false);
       attachClause(cr);
       cls[j++] = cr;
    }

    cls.shrink(i - j);
    checkGarbage();
    return ret; 
}

int  Solver :: trdbin (Lit start, Lit target, int learntflag) 
{
  int res=0;  
  int ign=1;
  vec<Lit> scan;
  scan.push(start); 
  seen[var(start)]=2+sign(start);
  for(int i=0; i<scan.size(); i++){
       Lit lit = scan[i];
       if(assigns[var(lit)] != l_Undef) continue;
       vec<Watcher>&  bin  = watchesBin[lit];
       for(int k = 0;k<bin.size();k++) {
            Lit other = bin[k].blocker;
            CRef cr=bin[k].cref;
            Clause& c = ca[cr];
            if(c.mark()) continue;
            if(c.learnt() && learntflag==0) continue;
            if (other == start) continue;
            if (other == target) {
                 if (lit == start && ign) { ign = 0; continue; }
                 res = 1;
	         goto DONE;
            }
            int mark=2+sign(other);
            Var ov=var(other);
            if(seen[ov]==0){
                    seen[ov] = mark;
                    scan.push(other);
                    continue;
            } 
            if(seen[ov] != mark){
               if (value(start) != l_Undef) continue;
               newDecisionLevel();
               uncheckedEnqueue(start);
               CRef confl = propagate();
               cancelUntil(0);
        
               confl = unitPropagation(~start);
               if (confl != CRef_Undef) { res = -1; goto DONE;}
            } 
       }
  }        
DONE:
  for(int i=0; i<scan.size(); i++)  seen[var(scan[i])]=0;
  return res;
}

void Solver ::cleanClauses(vec<CRef>& cls)
{
    int i,j;
    for (i = j = 0; i < cls.size(); i++)
        if (ca[cls[i]].mark() == 0) cls[j++] = cls[i];
    cls.shrink(i - j);
}

void Solver :: cleanNonDecision()
{   int i,j;
    for (i = j = 0; i < learnts.size(); i++){
        CRef cr=learnts[i];
        Clause & c = ca[cr];
        if (c.mark()) continue;
        for (int k = 0; k < c.size(); k++){
             if (value(c[k]) == l_True || !decision[var(c[k])]) {
                    removeClause(cr);
                    goto NEXT;
             }
        }
        learnts[j++] = cr;
    NEXT: ;
    }   
    learnts.shrink(i -j);
}

lbool Solver :: trdlit (Lit start, int & remflag) 
{ 
  vec<Watcher>&  bin  = watchesBin[start];
  for(int k = 0;k<bin.size(); k++) {
         Lit target = bin[k].blocker;
         if (var (start) > var (target)) continue;
         CRef cr=bin[k].cref;
         Clause & c = ca[cr];
         if(c.mark()) continue;
         int ret = trdbin (start, target, c.learnt());
         if(ret < 0 ) return l_False;
         if(ret > 0 ) {
              removeClause(cr);
              remflag=1;
              break;
         }
  }
  return l_Undef;
}

//delete taut binary clause
lbool Solver :: transitive_reduction () 
{ int i;
  printf("c transitive_reduction cls#=%d learnt=%d %d \n",clauses.size(),learnts.size(),clauses.size()+learnts.size());
  unsigned int pos,delta,v,mod=nVars();
  if (mod<20)  return l_Undef;
  pos     = s_rand () % mod;
  delta = s_rand () % mod;
  if (!delta) delta++;
  while (s_gcd (delta, mod) > 1)   if (++delta == mod) delta = 1;
  int remflag=0;
  lbool ret=l_Undef;
  cancelUntil(0);
  i=nVars();
  if(i>5000) i=5000;
  for(; i>0; i--) {
        v= pos/2;
        if(assigns[v] == l_Undef){
             Lit lit = (pos & 1) ? mkLit(v) : ~mkLit(v);
             ret=trdlit (lit, remflag);
             if(ret != l_Undef) break;
        }
        pos = (pos+delta) % mod;
  }
  if(remflag) {
          cleanClauses(clauses);
          cleanClauses(learnts);
  }
  printf("c transitive_reduction_end cls#=%d \n",clauses.size());
  return ret;
}

typedef struct DFPR  { int discovered, finished, parent, root;} DFPR;
typedef struct DFOPF { int observed, pushed, flag;} DFOPF;
DFOPF * dfopf;
DFPR * dfpr=0;

typedef struct DFL {
  int discovered, finished;
  int lit, sign;
} DFL;

typedef enum Wrag {
  PREFIX = 0,
  BEFORE = 1,
  AFTER = 2,
  POSTFIX = 3,
} Wrag;

typedef struct Work {
  Wrag wrag : 2;
  int lit : 30;
  int other : 30;
  unsigned red : 1;
  unsigned removed : 1;
  CRef cr;
} Work;

int Solver :: unhdhasbins (Lit lit, int irronly)
{
   vec<Watcher>&  bin  = watchesBin[~lit];
   for(int k = 0;k<bin.size();k++) {
         Lit other = bin[k].blocker;
         CRef cr=bin[k].cref;
         Clause& c = ca[cr];
         if(c.learnt() && irronly) continue;
         if( value(other) != l_Undef) continue;
         if (!dfpr[toInt(other)].discovered) return 1;
   }
   return 0;
}

inline void s_pushwork (vec<Work> & work, Wrag wrag, int lit, int other, int red,CRef cr=CRef_Undef) 
{
  Work w;
  w.wrag = wrag;
  w.other = other;
  w.red = red ? 1 : 0;
  w.removed = 0;
  w.lit = lit;
  w.cr  = cr;
  work.push(w);
}

int Solver :: s_stamp (Lit root, vec <int> &  units, int stamp, int irronly) 
{
  vec<Work> work;
  int observed, discovered, pos;
  unsigned start, end, mod, i, j;
  int startstamp=0;
  int uroot=toInt(root);
  if (dfpr[uroot].discovered) return stamp;
  s_pushwork (work, PREFIX, uroot, uroot, 0);

  vec <int> sccs;
  while (work.size()) {
      Work w = work.pop_();
      if (w.removed) continue;
      if (w.wrag == PREFIX) {
           if (dfpr[w.lit].discovered) {
	        dfopf[w.lit].observed = stamp;
                continue;
           }
           dfpr[w.lit].discovered = ++stamp;
           dfopf[w.lit].observed = stamp;
           if (!startstamp) {
        	startstamp = stamp;
         	dfpr[w.lit].root = w.lit;
           }
           s_pushwork (work, POSTFIX, w.lit, w.lit, 0);
           dfopf[w.lit].pushed = start = work.size();
           dfopf[w.lit].flag = 1;
           sccs.push(w.lit);
           Lit p=toLit(w.lit);
           vec<Watcher>&  bin  = watchesBin[p];
           for(int k = 0;k<bin.size();k++) {
                  Lit other = bin[k].blocker;
                  CRef cr=bin[k].cref;
                  Clause& c = ca[cr];
                  if(c.mark()) continue;
                  if(c.learnt() && irronly) continue;
                  if( value(other) != l_Undef) continue;
                  int uo=toInt(other);
                  if(mark[uo]) continue;
                  mark[uo]=1;
         	  s_pushwork (work, BEFORE, w.lit, uo, c.learnt(),cr);
           }
                
	   for(int k = start;k<work.size();k++) mark[work[k].other]=0;
           end = work.size();
	   mod = end - start;
           for (i = start; i < end - 1;  i++) {
	       j = s_rand () % mod--;
	       if (!j) continue;
	       j = i + j;
               Work tmp=work[i]; work[i] = work[j]; work[j] = tmp;
           }
     } else if (w.wrag == BEFORE) {// before recursive call,stamping edge %d %d before recursion", lit, other);
          s_pushwork (work, AFTER, w.lit, w.other, w.red);
          int Nother = w.other ^ 1; 
          if ( (irronly || w.red) && dfopf[w.other].observed > dfpr[w.lit].discovered) {
               //transitive edge %d %d during stamping", lit, other
               Clause & c = ca[w.cr];
               if(c.mark()==0) removeClause(w.cr);
               if ((pos = dfopf[Nother].pushed) >= 0) {           
                         int Nlit=w.lit^1;
                         while (pos  < work.size()) {
	                     if (work[pos].lit != Nother) break;
	                     if (work[pos].other == Nlit) {//removing edge %d %d from DFS stack", -other, -lit);
	                            work[pos].removed = 1;
	                      }
	                      pos++;
	                  }
	        }
                work.pop();
               	continue;
	   }
           observed = dfopf[Nother].observed;
           if ( startstamp <= observed) {//stamping failing edge %d %d, lit, other
	        int failed;
                for (failed = w.lit;  dfpr[failed].discovered > observed; failed = dfpr[failed].parent);
	        units.push(failed^1);
                if (dfpr[Nother].discovered && !dfpr[Nother].finished) {
                     work.pop();
                     continue;
	        }
           }
           if (!dfpr[w.other].discovered) {
	       dfpr[w.other].parent = w.lit;//stamping %d parent %d", other, lit
               dfpr[w.other].root   = uroot;//stamping %d root %d", other, root
	       s_pushwork (work, PREFIX, w.other, w.other, 0);
           }
    } else if (w.wrag == AFTER) {	// after recursive call. stamping edge %d %d after recursion, lit, other
         if (!dfpr[w.other].finished  && dfpr[w.other].discovered < dfpr[w.lit].discovered) {
                  //stamping back edge %d %d", lit, other
	         dfpr[w.lit].discovered = dfpr[w.other].discovered;
                      //stamping %d reduced discovered to %d",lit, dfpr[ulit].discovered);
	         dfopf[w.lit].flag = 0; // stamping %d as being part of a non-trivial SCC", lit
         }
         dfopf[w.other].observed = stamp;//stamping %d observed %d", other, stamp
    } else {//stamping postfix %d", lit
          if (dfopf[w.lit].flag) {
        	stamp++;
                discovered = dfpr[w.lit].discovered;
                int other;
	        do {
	             other = sccs.pop_();
	             dfopf[other].pushed = -1;
	             dfopf[other].flag = 0;
	             dfpr[other].discovered = discovered;
	             dfpr[other].finished = stamp;
                } while (other != w.lit);
         }
      }
  }
  return stamp;
}
//remove duplicated binary clause 
void Solver :: rmBinDup () 
{
  watchesBin.cleanAll();
  int redrem, irrem;
  redrem = irrem = 0;
  vec<int> scan;
  vec<CRef> delbin;
  for (int idx = 0; idx < nVars(); idx++) {
     for (int sign = -1; sign <= 1; sign += 2) {
         Lit lit = (sign > 0) ? mkLit(idx) : ~mkLit(idx);
         scan.clear();
         for (int round = 0; round < 2; round++) {
	     vec<Watcher>&  bin  = watchesBin[lit];
             delbin.clear();
             for(int k = 0;k<bin.size();k++) {
                  Lit other = bin[k].blocker;
                  CRef cr=bin[k].cref;
                  Clause & c = ca[cr];
                  if(c.mark()) continue;
                  if(c.learnt()    && round == 0) continue;
                  if(c.learnt()==0 && round )     continue;
                  if( value(other) != l_Undef) continue;
                  int uo=toInt(other);
                  if(mark[uo]) delbin.push(cr);
                  else { mark[uo]=1;
                         scan.push(uo);
         	  }
            }
            for(int k = 0;k<delbin.size();k++) removeClause(delbin[k]);
            if(round) redrem += delbin.size();
            else      irrem  += delbin.size();
         }
	 for(int k = 0;k<scan.size();k++) mark[scan[k]]=0;
      }
   }
   if(redrem) cleanClauses(learnts);
   if(irrem)  cleanClauses(clauses);
   watchesBin.cleanAll();
}

inline void find_relative_prime(unsigned & pos, unsigned & delta, unsigned mod)
{
      pos   = s_rand () % mod;
      delta = s_rand () % mod;
      if (!delta) delta++;
      while (s_gcd (delta, mod) > 1)   if (++delta == mod) delta = 1;
}

lbool Solver :: s_stampall (int irronly) 
{
  unsigned pos, delta, mod;
  int stamp, rootsonly;
  lbool ret = l_Undef;

  vec<int>  units;
 
  rmBinDup ();
  int nvar = nVars();
  dfpr  = (DFPR * ) calloc (2*nvar+1, sizeof(DFPR));
  dfopf = (DFOPF *) calloc (2*nvar+1, sizeof(DFOPF));

  for (DFOPF * q = dfopf; q < dfopf + 2*nvar+1; q++) q->pushed = -1;
  stamp = 0;
  for (rootsonly = 1; rootsonly >= 0; rootsonly--) {
      mod = 2*nvar;

     find_relative_prime(pos, delta, mod);
      //unhiding %s round start %u delta %u mod %u,(rootsonly ? "roots-only": "non-root"), pos, delta, mod);
      for (unsigned int i=0; i<mod; i++, pos = (pos+delta) % mod ) {
           Lit root = toLit(pos);
           if( value(root) != l_Undef) continue;
           if (dfpr[pos].discovered) continue;
           if (rootsonly && unhdhasbins (root, irronly)) continue;
           if (!unhdhasbins (~root, irronly)) continue;
           stamp = s_stamp (root, units, stamp, irronly);
           while (units.size()) {
	         int uo = units.pop_();
                 Lit lit = toLit (uo);
                 if( value(lit) != l_Undef) continue;
                 if(!checkUnit (uo, 'S')) continue;
                 CRef confl = unitPropagation(lit);
                 if (confl != CRef_Undef) { ret = l_False; goto DONE;}
 	   }
       }
  }
DONE:
  if (ret != l_Undef) { free(dfpr); dfpr = 0;}
  free(dfopf);
  return ret;
}

lbool Solver :: unhide () 
{ int irr_only=0;
  lbool ret = l_Undef;
  printf("c unhide \n");
  dfpr = 0;
  if (nVars() <20) return l_Undef;
  cancelUntil(0);
  mark = (char * ) calloc (2*nVars()+1, sizeof(char));
  for(int round=0; round<2; round++) 
  {  
      ret = s_stampall (irr_only);
      if (ret != l_Undef) break;
      if (!dfpr) break;
      watchesBin.cleanAll();

      check_all_impl();

      ret=s_unhidefailed ();
      if (ret != l_Undef) break;

      ret=s_unhidebintrn (clauses, irr_only, 0);
      if (ret != l_Undef) break;
      ret=s_unhidebintrn (learnts, irr_only, 1);
      if (ret != l_Undef) break;

      ret= s_unhidelrg (clauses, irr_only, 0);
      if (ret != l_Undef) break;
      cleanClauses(clauses);

      ret= s_unhidelrg (learnts, irr_only, 1);
      cleanClauses(learnts);
      if (ret != l_Undef) break;
     
      irr_only = !irr_only;
      free (dfpr);
      dfpr=0;
  }
  rmBinDup ();
  free(mark);
  if (dfpr) free (dfpr);
  return ret;
}

static int s_unhimpl (int u, int v) {
//  int u = s_ulit (a), v = s_ulit (b),
  int c, d, f, g;
  c = dfpr[u].discovered; if (!c) return 0;
  d = dfpr[v].discovered; if (!d) return 0;
  f = dfpr[u].finished, g = dfpr[v].finished;
  return c < d && g < f;
}

int s_unhimplies2 (int a, int b) {
  return s_unhimpl (a, b) || s_unhimpl (b^1, a^1);
}

int s_unhimplincl (int u, int v) 
{
  int c, d, f, g;
  c = dfpr[u].discovered; if (!c) return 0;
  d = dfpr[v].discovered; if (!d) return 0;
  f = dfpr[u].finished, 
  g = dfpr[v].finished;
  return c <= d && g <= f;
}

int s_unhimplies2incl (int a, int b) {
  return s_unhimplincl (a, b) || s_unhimplincl (b^1, a^1);
}

lbool Solver :: s_unhidefailed () 
{
  int unit;
  for (int idx = 0; idx < nVars(); idx++) {
     for (int sign = -1; sign <= 1; sign += 2) {
         Lit lit = (sign > 0) ? mkLit(idx) : ~mkLit(idx);
         if( value(lit) != l_Undef) continue;
         unit=toInt(lit);
         if(!dfpr[unit^1].discovered) continue;
         if (s_unhimplincl (unit^1, unit)==0) {// unhiding %d implies %d", -lit, lit
                if (s_unhimplincl (unit, unit^1)==0) continue;//unhiding %d implies %d", lit, -lit
                lit = ~lit;
         }
         if(checkUnit (toInt(lit), 'F')){
               CRef confl = unitPropagation(lit);
               if (confl != CRef_Undef) return l_False;
        }
    }
  } 
  return l_Undef;
}

//unhiding least common ancestor of %d and %d is %d", a, b, p
int Solver :: s_unhlca (int a, int b) 
{
  int p;
  if (a == b) return a;
  if (dfpr[a].discovered <= dfpr[b].discovered) p = a;
  else { p = b; b=a; a=p; }
  for (;;) {
    if (dfpr[b].finished <= dfpr[p].finished) break;
    p = dfpr[p].parent;
    if (!p) break;
  }
  return p;
}

int s_unhroot (int ulit) { return dfpr[ulit].root;}

int Solver :: checkUnit (int uLit, char printflag)
{    (void)printflag;
     Lit unit=toLit(uLit^1);
     if(value(unit) == l_False) return 1;
     if(value(unit) == l_Undef) {
            newDecisionLevel();
            uncheckedEnqueue(unit);
            CRef confl = propagate();
            cancelUntil(0);
            if (confl != CRef_Undef) return 1;
     }
     //printf("c checkUnit %c  fail lit=%d \n",printflag, toIntLit(~unit));
     return 0;
}

void Solver :: checkBin (int uL1, int uL2,char printflag)
{    (void)printflag;
     Lit lit;
     for(int i=0; i<2; i++){
              if(i) lit=toLit(uL2^1);
              else lit=toLit(uL1^1);
              if(value(lit) == l_False) goto done;
              if(value(lit) == l_Undef) {
                   newDecisionLevel();
                   uncheckedEnqueue(lit);
                   CRef confl = propagate();
                   if (confl != CRef_Undef) goto done;
              }
     }
       //  printf("c <Bin_%c <%d %d> fail \n",printflag,uL1,uL2);
done:
     cancelUntil(0);
}

void Solver :: checklarg(vec <Lit> & cs, char type)
{     (void)type;
      int i;
      for(i=0; i<cs.size(); i++){
              if(value(cs[i]) == l_True)  break;
              if(value(cs[i]) == l_False) continue;
              newDecisionLevel();
              uncheckedEnqueue(~cs[i]);
              CRef confl = propagate();
              if (confl != CRef_Undef) break;
      }
      //if(i>=cs.size()) printf("c large %c fail \n",type);
      cancelUntil(0);
}

void Solver :: check_all_impl()
{ 
   int m=0;
   for (int i = 0; i < 2*nVars() && i<10000; i++) {
       for (int j = 0; j < 2*nVars() && i<10000; j++) {
             if(i==j) continue;
             if (s_unhimplincl (i, j)) {// i implies j
                   checkBin (i^1, j, 'L');
                   m++;
             }
        }
   }
   printf("c check all %d impl \n",m);
}           
 
lbool Solver :: s_unhidebintrn (vec<CRef>& cls,int irronly,int learntflag)
{
  lbool ret=l_Undef;
  int i,j;
  vec<Lit> newCls,NewBin;
  int root,lca,ul,unit,other2;
	
  for (i = j = 0; i < cls.size(); i++){
        Clause & c = ca[cls[i]];
        if (c.mark()) continue;
        if(c.size()>3 || ret==l_False) goto next;
        ul=toInt(c[0]);
        unit=toInt(c[1]);
        if(c.size() == 2){
             if( value(c[0]) != l_Undef || value(c[1]) != l_Undef) goto next;
             if(!dfpr[ul].discovered) goto next;
             if (s_unhimplies2 (ul, unit)) {
                      if(!checkUnit (unit, 'A')) goto next;
RUNIT:               
                      removeClause(cls[i]);
UNIT:
                      CRef confl = unitPropagation(toLit(unit));
                      if (confl != CRef_Undef) ret=l_False;
                      continue;
	     }
             if (s_unhimplies2 (unit,ul)) {
                      unit=ul;
                      if(!checkUnit (unit, 'B')) goto next;
                      goto RUNIT;
             }
             if ((root = s_unhroot (ul^1)) && value(toLit(root)) == l_Undef && root == s_unhroot (unit^1)) { 
                      //negated literals in binary clause %d %d implied by %d",lit, other, root
                      int lca = s_unhlca (ul^1, unit^1);
                      if(lca==0) goto next;
	              unit = lca^1;
	              if(!checkUnit (unit, 'C')) goto next;
                      cls[j++] = cls[i];
                      goto UNIT;
             }
	     if (irronly==0 && learntflag==0)   goto next;
      	     if (dfpr[unit].parent == (ul^1))   goto next;
             if (dfpr[ul  ].parent == (unit^1)) goto next;
	     if (!s_unhimplies2 (ul^1, unit))   goto next;
             removeClause(cls[i]);
             continue;
        }
        if( value(c[2]) != l_Undef) goto next;
        other2=toInt(c[2]);

        if (s_unhimplies2incl (unit, ul) && s_unhimplies2incl (other2, ul)) {
	    //unit => ul  other2 => ul
            unit = ul;
            if(!checkUnit (unit, 'D')) goto next;
 	    goto RUNIT;
	}

        if ((root = s_unhroot (ul^1)) && value(toLit(root)) == l_Undef && root == s_unhroot (unit^1) && 
           root == s_unhroot (other2^1)) { 
        // root => ul^1 & unit^1  & other2^1;
	    lca = s_unhlca (ul^1, unit^1);
	    lca = s_unhlca (lca, other2^1);
	    if(lca==0) goto next;
            unit = lca^1;
            if(!checkUnit (unit, 'E')) goto next;
	    cls[j++] = cls[i];
            goto UNIT;
	 }
	 if ((learntflag || irronly) &&  (s_unhimplies2incl (ul^1, unit) || s_unhimplies2incl (ul^1, other2))) {
	   // (ul V unit) or (ul V others) => tautological
	    removeClause(cls[i]);
            continue;
         } 
	 if (s_unhimplies2incl (other2, ul)) {
             checkBin (ul, unit,'A');
TRNSTR://unhiding removal of literal %d with implication %d %d from ternary clause %d %d %d,
             removeClause(cls[i]);
             addbinclause2(toLit(ul), toLit(unit), cls[j], learntflag);
             j++;
            continue;
         }
         if (s_unhimplies2incl (unit, ul)) {
 	    unit=other2;
            checkBin (ul, unit,'B');
	    goto TRNSTR;
	 }
         if ((root = s_unhroot (ul^1)) &&  value(toLit(root)) == l_Undef) {
	      if (root == s_unhroot (other2^1)) {
	          lca = s_unhlca (ul^1, other2^1);
	      } else if (root == s_unhroot (unit^1)) {
	          lca = s_unhlca (ul^1, unit^1);
	          Swap(unit,other2);
	      } else if (s_unhimplies2incl (root, other2^1)) lca = root;
	        else if (s_unhimplies2incl (root, unit^1)) {
	                lca = root;
	                Swap(unit, other2);
	              } else goto next;
	    if (lca==0) goto next;
	    if ((lca/2) == (ul/2))     goto next;
	    if ((lca/2) == (unit/2))   goto next;
	    if ((lca/2) == (other2/2)) goto next;
	    if (s_unhimplies2incl (lca, unit)) goto next;
	   // (-lca V -ul) and (-lca V -other) => -lca V unit
              ul=lca^1;
              checkBin (ul, unit,'C');//bug
              if(NewBin.size()<1000){
                   Lit p=toLit(ul);
                   Lit q=toLit(unit);
                   if(!hasBin(p, q)){
                          NewBin.push(p);
                          NewBin.push(q);
                  }
              }
        }
next:
         cls[j++] = cls[i];
    }
    cls.shrink(i - j);
    while(NewBin.size()){
            newCls.clear();
            newCls.push(NewBin.pop_());
            newCls.push(NewBin.pop_());
            CRef cr = ca.alloc(newCls, learntflag);
            attachClause(cr);
            cls.push(cr);
    }
    return ret;
}

void * s_realloc (void * ptr, size_t old, size_t newsz) 
{
  void * res;
  if (ptr==0) res= malloc (newsz);
  else {
       if (!newsz) { free (ptr); return 0; }
       res = realloc (ptr, newsz);
  }
  if (newsz > old) memset ((char *)res + old, 0, newsz - old);
  return res;
}

void Adp_SymPSort(void *a, int n, int es, int (*cmp)(const void *,const void *));
int cmpdfl (const void * a, const void * b)
{  DFL * c =(DFL *)a;
    DFL * d =(DFL *)b;
    return c->discovered - d->discovered;
}

void sort(DFL * dfl,int n)
{
   Adp_SymPSort((void *)dfl, n, sizeof(DFL), cmpdfl);
}

lbool Solver :: s_unhidelrg (vec<CRef>& cls,int irronly,int learntflag)
{  int posdfl, negdfl, ndfl, szdfl = 0;
   int lca,nsize;
   int i,j,k,q,unit,satisfied,tautological,root,root1,root2,lca1,lca2;
   vec<Lit> newCls,bin;
   lbool ret=l_Undef;
   CRef confl,cr;
   DFL *eodfl,*d,*e, * dfl = 0;
   for (i = j = 0; i < cls.size(); i++){
        Clause & c = ca[cls[i]];
        if (c.mark()) continue;
        if(c.size() <=3 || ret==l_False) goto nextcls;
        posdfl = negdfl = satisfied = tautological = ndfl = 0;
        newCls.clear();
        for(k=0; k<c.size(); k++){
             Lit p=c[k];
             if (value(p) == l_True) {
                    satisfied = 1;
                    break;
             }
             if (value(p) == l_False) continue;
             newCls.push(p);
             if (dfpr[toInt(p)].discovered) posdfl++;	// count pos in BIG
             if (dfpr[toInt(p^1)].discovered) negdfl++;	// count neg in BIG
        }
        if (satisfied){
remSAT:
             removeClause(cls[i]);
             continue;
        }
        ndfl = posdfl + negdfl;	// number of literals in BIG
        nsize=newCls.size();
//FAILED: find root implying all negated literals
        if (nsize != negdfl) goto HTE;	// not enough complement lits in BIG
        lca = toInt(newCls[0])^1;
        root = s_unhroot (lca);
        if (!root) goto HTE; //????
        if (value(toLit(root)) != l_Undef) goto HTE;
        for(k=1; k<nsize && s_unhroot (toInt(newCls[k])^1) == root; k++);
        if (k < nsize) goto HTE; // at least two roots
        // lca => -x1, lca => -x2,,, lca => -xn  ==> -lca 
        for(k=1; k<nsize; k++) lca = s_unhlca (toInt(newCls[k])^1, lca);
        unit = lca^1;
        confl = unitPropagation(toLit(unit));
        if (confl != CRef_Undef) ret=l_False;
        goto nextcls;
HTE:    //hide tautology elim
        if (learntflag == 0 && !irronly) goto STRNEG; // skip tautology checking if redundant clauses used and
					   //  irredundant clauses
        if (posdfl < 2 || negdfl < 2) goto STRNEG;
        if (ndfl > szdfl){
             dfl = (DFL *)s_realloc (dfl, szdfl*sizeof(DFL), ndfl*sizeof(DFL));
             szdfl = ndfl;
        }
        ndfl = 0;

    // copy all literals and their complements to 'dfl'
        for(k=0; k<nsize; k++){
            for (int sign = 1; sign >=0; sign--) {
	      int ulit = toInt(newCls[k])^sign;
              if (!dfpr[ulit].discovered) continue;	// not in BIG so skip
	      dfl[ndfl].discovered = dfpr[ulit].discovered;
	      dfl[ndfl].finished = dfpr[ulit].finished;
	      dfl[ndfl].sign = sign;
	      dfl[ndfl].lit  = ulit;
	      ndfl++;
           }
        }
        sort(dfl, ndfl);
        eodfl = dfl + ndfl;
        for (d = dfl; d < eodfl - 1; d++)
             if (d->sign > 0) break;			// get first negative pos
        while (d < eodfl - 1) {
          for (e = d + 1; e < eodfl && e->finished < d->finished; e++) {
	          if (e->sign > 0) continue;		// next positive pos
	              //  d.lit => elit   ==>  tautological;
                 checkBin (d->lit^1, e->lit, 't');
                 goto remSAT;
               /*
                 removeClause(cls[i]);
                 bin.clear();
                 bin.push(toLit(d->lit^1));
                 bin.push(toLit(e->lit));
                 cr = ca.alloc(bin, learntflag);
                 attachClause(cr);
                 cls[i]=cr;
               //  cls[j++]=cr;
                 goto nextcls;
               */
          }
          for (d = e; d < eodfl && d->sign == 0; d++);
       }
STRNEG: //negative
      //????   if (!s_str (spf)) goto HBR;
       if (negdfl < 2) goto STRPOS;
       if (negdfl > szdfl) { 
           dfl = (DFL*)s_realloc (dfl, szdfl*sizeof(DFL), negdfl*sizeof(DFL));
           szdfl = negdfl; 
      }

      ndfl = 0;
    // copy complement literals to 'dfl'
      for(k=0; k<nsize; k++){
            int ulit = toInt(newCls[k])^1;
            if (!dfpr[ulit].discovered) continue;
	    dfl[ndfl].discovered = dfpr[ulit].discovered;
	    dfl[ndfl].finished = dfpr[ulit].finished;
	    dfl[ndfl].lit = ulit^1;
	    ndfl++;
      }
      if (ndfl < 2) goto STRPOS;
      sort(dfl, ndfl);
      eodfl = dfl + ndfl;
      for (d = dfl; d < eodfl - 1; d = e) {
           for (e = d + 1; e < eodfl && d->finished >= e->finished; e++) {
	//  -d->lit => -e->lit;
	      e->lit = -1;
           }
    }
    q = 0;
    for (int p = q; p < nsize; p++) {
      int ulit = toInt(newCls[p])^1;
      if (dfpr[ulit].discovered) continue;
      newCls[q++]=newCls[p];
    }
    // copy from 'dfl' unremoved BIG literals back to clause
    for (d = dfl; d < eodfl; d++) {
      int lit = d->lit;
      if (lit<0) continue;
      newCls[q++]=toLit(lit);
    }
    newCls.shrink(nsize-q);
    nsize=q;

STRPOS:
    if (posdfl < 2) goto HBR;
    if (posdfl > szdfl) { 
          dfl = (DFL*)s_realloc (dfl, szdfl*sizeof(DFL), posdfl*sizeof(DFL));
          szdfl = posdfl; 
    }
    ndfl = 0;
    // copy original literals to 'dfl' but sort reverse wrt 'discovered'
    for(k=0; k<nsize; k++){
           int ulit = toInt(newCls[k]);
           if (!dfpr[ulit].discovered) continue;
	   dfl[ndfl].discovered = -dfpr[ulit].discovered;
	   dfl[ndfl].finished = -dfpr[ulit].finished;
	   dfl[ndfl].lit = ulit;
	   ndfl++;
    }
    if (ndfl < 2) goto NEXT;
    sort(dfl, ndfl);
    eodfl = dfl + ndfl;
    for (d = dfl; d < eodfl - 1; d = e) {
      for (e = d + 1; e < eodfl && d->finished >= e->finished; e++) {
	// e->lit => d->lit;
	e->lit = -1;
      }
    }
    q = 0;
    for (int p = q; p < nsize; p++) {
      int ulit = toInt(newCls[p]);
      if (dfpr[ulit].discovered) continue;
      newCls[q++]=newCls[p];
    }
    for (d = dfl; d < eodfl; d++) {
      int lit = d->lit;
      if (lit<0) continue;
      newCls[q++]=toLit(lit);;
    }
    newCls.shrink(nsize-q);
    nsize=q;
HBR:
   // hyper binary resolution x => -x1, x => -x2,...,x => -xn and y => -y1, y => -y2,...,y => -ym
   // -x1 V -x2 V...V -xn V y1 V y2 V...V ym => -x V -y 
    //if (!spf->opts.unhdhbr.val) goto NEXT;
    if (nsize < 3) goto NEXT;
    root1 = root2 = lca1 = lca2 = 0;
    for (int p = 0; p < nsize; p++) {
         int lit=toInt(newCls[p]);
         root = s_unhroot (lit^1);
         if (!root){
                root = lit^1;//var=1
                if (!root) goto NEXT;
         }
         if (!root1) { root1 = root; continue; }
         if (root1 == root) continue;
         if (!root2) { root2 = root; continue; }
         if (root2 == root) continue;
         if (s_unhimplies2incl (root1, lit^1)) { lca1 = root1; continue; }
         if (s_unhimplies2incl (root2, lit^1)) { lca2 = root2; continue; }
         goto NEXT;			// else more than two roots ...
    }
    if (!root2) goto NEXT;
    if (root1 == (root2^1)) goto NEXT;
    if (s_unhimplies2incl (root1, root2^1)) goto NEXT;
    //root hyper binary resolution %d %d of %s clause,-root1, -root2, type);
    if (!lca1 && !lca2) {
       for (int p = 0; p < nsize; p++) {
          int lit=toInt(newCls[p]);
          root = s_unhroot (lit^1);
	  if (root) {
	      if (root == root1) lca1 = lca1 ? s_unhlca (lca1, lit^1) : (lit^1);
	      if (root == root2) lca2 = lca2 ? s_unhlca (lca2, lit^1) : (lit^1);
	  } else {
	      if (lca1) lca2 = lit^1; 
              else lca1 = lit^1;
	  }
       }
    } 
    else {
      if (!lca1) lca1 = root1;
      if (!lca2) lca2 = root2;
    }
    if (lca1 == lca2^1 || lca1==0 || lca2==0) goto NEXT;
    if (s_unhimplies2incl (lca1, lca2^1)) goto NEXT;
    //lca hyper binary resolution lca1 => -x1, lca1 => -x2,...,lca1 => -xn and lca2 => -y1, lca2 => -y2,...,lca2 => -ym
    checkBin (lca1^1, lca2^1, 'a');
    addbinclause2(toLit(lca1^1), toLit(lca2^1), cr, learntflag);
    cls.push(cr);
    goto nextcls;
NEXT:
    if (nsize<c.size() && nsize>0) {
         // checklarg(newCls,'C');//bug
          removeClause(cls[i]);
          if(nsize>1){
              CRef cr = ca.alloc(newCls, learntflag);
              attachClause(cr);
              cls[j++] = cr;
              continue;
          }
          if(nsize==1){
               CRef confl = unitPropagation(newCls[0]);
               if (confl != CRef_Undef) ret=l_False;
          }
          continue;
      }
nextcls:
      cls[j++] = cls[i];
  }
  cls.shrink(i - j);
  if (dfl) free(dfl);
  return ret;
}
//---------------------------------------
// probing analysis returns 1st UIP
Lit Solver::s_prbana(CRef confl, Lit res)
{
    vec<Var> scan;
    int tpos=trail.size();
    int open=0;
    while(tpos>0){
          Clause & c = ca[confl];
          for (int i =0; i < c.size(); i++){
                 Var pv= var(c[i]);    
                 if (seen[pv]) continue;
                 if(level(pv) > 0 ) {
                        seen[pv]=1;
                        scan.push(pv);
                        open++;
                 }
          }
          while(tpos>0){
                tpos--;
                if(seen[var(trail[tpos])]) break;
          }
          open--;
          if(open==0) {           
                res = trail[tpos];
                break;
          }     
          confl = reason(var(trail[tpos]));
    }
    for (int i = 0; i<scan.size(); i++)  seen[scan[i]] = 0;
    return res;
}

int Solver :: s_randomprobe (vec<int> & outer) 
{
  unsigned mod = outer.size();
  if (mod==0) return -1;
  unsigned pos = s_rand () % mod;
  int res = outer[pos];
  if (assigns[res] != l_Undef) return -1;
  return res;
}

int Solver :: s_innerprobe (int start, vec<int> & probes) 
{
  vec<int> tmp;
  for (int i = start; i < trail.size(); i++) {
         Lit lit = trail[i];
         vec<Watcher>&  trn  = watches[lit];
         for(int j = 0; j<trn.size(); j++) {
                 CRef cr=trn[j].cref;
                 Clause & c = ca[cr];
                 if(c.size() != 3) continue;
                 int m=0;
                 for(int k=0; k<3; k++)
                       if(value(c[k]) != l_Undef) m++;
                 if(m>1) continue;
                 for(int k=0; k<3; k++){
                       if(value(c[k]) == l_Undef){
                             int v = var(c[k]);
                             if(seen[v]) continue;
                             tmp.push(v);
                             seen[v]=1;
                      }
                 }
         } 
  }//found %d inner probes in ternary clauses", s_cntstk (tmp));
  int res = s_randomprobe (tmp);
  for (int i = 0; i < tmp.size(); i++) seen[tmp[i]]=0;
  if (res<0) res = s_randomprobe (probes);
  return res;
}

void Solver :: s_addliftbincls (Lit p, Lit q)
{ 
    if(hasBin(p,q)) return;
    vec<Lit> ps;
    ps.push(p);
    ps.push(q);
    if(!is_conflict(ps)) return;
    CRef cr = ca.alloc(ps, false);
    clauses.push(cr);
    attachClause(cr);
}

lbool Solver :: check_add_equ (Lit p, Lit q)
{
    if( p == q) return l_Undef;
    if( p == ~q ) return l_False;
    if(!out_binclause(p,  ~q)) return l_Undef;
    if(!out_binclause(~p, q)) return l_Undef;

    return add_equ_link(toInt(p), toInt(q));
}

lbool Solver :: addeqvlist (vec <int> & equal)
{
     Lit p=toLit(equal[0]);
     for (int i = 1; i < equal.size(); i++) {
             Lit q=toLit(equal[i]);
             lbool ret = check_add_equ (p, q);
             if(ret == l_False){
                    return l_False;
             }
     }
     return l_Undef;
}       
//double depth unit probe
lbool Solver :: s_liftaux () 
{
  int i,outer,inner;
  unsigned pos, delta, mod;
  vec<int> probes, represented[2], equal[2];
  vec<Lit> saved; 
 
  for (int idx = 0; idx < nVars(); idx++) {
      if (assigns [idx] != l_Undef) continue;
      probes.push(idx);
  }
  mod = probes.size();
  if (mod<20) return l_Undef;
  find_relative_prime(pos, delta, mod);

  lbool ret=l_Undef;
  CRef confl;
  Lit inlit;
  int loop=mod-10;
  if(loop>10000) loop=10000;
  
  while ( --loop > 0 ) {
       outer = probes[pos];
       pos = (pos + delta) % mod;
       if (assigns[outer] != l_Undef ) continue;
        //1st outer branch %d during lifting, outer
       represented[0].clear();
       represented[1].clear();
       equal[0].clear();
       equal[1].clear();
       int oldtrailsize = trail.size();
       Lit outlit=mkLit(outer);
       newDecisionLevel();
       uncheckedEnqueue(outlit);
       confl = propagate();
       if (confl != CRef_Undef) {
FIRST_OUTER_BRANCH_FAILED:
           Lit dom = s_prbana(confl, outlit);
            //1st outer branch failed literal %d during lifting, outer
            cancelUntil(0);
            confl = unitPropagation(~dom);
            if (confl == CRef_Undef) continue;
            ret=l_False;
            break;
       }
       inner = s_innerprobe (oldtrailsize, probes);
       if (inner<0) {
FIRST_OUTER_BRANCH_WIHOUT_INNER_PROBE:
          //no inner probe for 1st outer probe %d", outer
           for (i = oldtrailsize; i < trail.size(); i++) represented[0].push(toInt(trail[i]));
           goto END_OF_FIRST_OUTER_BRANCH;
       }   //1st inner branch %d in outer 1st branch %d", inner, outer
       inlit =mkLit(inner);
       newDecisionLevel();
       uncheckedEnqueue(inlit);
       confl = propagate();
       if (confl != CRef_Undef) {//1st inner branch failed literal %d on 1st outer branch %d,inner, outer
               cancelUntil(0);
               s_addliftbincls (~outlit, ~inlit);
               newDecisionLevel();
               uncheckedEnqueue(outlit);
               confl = propagate();
               if (confl == CRef_Undef ) goto FIRST_OUTER_BRANCH_WIHOUT_INNER_PROBE;
               goto FIRST_OUTER_BRANCH_FAILED; //conflict after propagating negation of 1st inner branch
        }
        saved.clear();
        for (i = oldtrailsize; i < trail.size(); i++) saved.push(trail[i]);
        cancelUntil(1);
        newDecisionLevel();
        uncheckedEnqueue(~inlit);
        confl = propagate();  //2nd inner branch %d in 1st outer branch %d", -inner, outer
        if (confl != CRef_Undef) {// 2nd inner branch failed literal %d on 1st outer branch %d,-inner, outer
               cancelUntil(0);
               s_addliftbincls (~outlit, inlit);
               newDecisionLevel();
               uncheckedEnqueue(outlit);
               confl = propagate();
               if (confl == CRef_Undef) goto FIRST_OUTER_BRANCH_WIHOUT_INNER_PROBE;
               goto FIRST_OUTER_BRANCH_FAILED;  // conflict after propagating negation of 2nd inner branch
        } 
        equal[0].push(toInt(inlit));
        while (saved.size()) {
               Lit lit = saved.pop_();
               if(value(lit) == l_True) represented[0].push(toInt(lit));
               else if(value(lit) == l_False && lit != inlit) equal[0].push(toInt(lit));
       }
END_OF_FIRST_OUTER_BRANCH:
       cancelUntil(0); // 2nd outer branch %d during lifting, -outer
       newDecisionLevel();
       uncheckedEnqueue(~outlit);
       CRef confl = propagate();
       if (confl != CRef_Undef) {
SECOND_OUTER_BRANCH_FAILED:
             Lit dom = s_prbana(confl, ~outlit);   //2nd branch outer failed literal %d during lifting, -outer
             cancelUntil(0);
             confl = unitPropagation(~dom);
             if (confl == CRef_Undef) continue;
             ret=l_False;
             break;
      }

      if (inner < 0 || value(inlit) != l_Undef ) 
              inner = s_innerprobe (oldtrailsize, probes);
      if (inner < 0) {
SECOND_OUTER_BRANCH_WIHOUT_INNER_PROBE:
              //no inner probe for 2nd outer branch %d", -outer
             for (i = oldtrailsize; i < trail.size(); i++) represented[1].push(toInt(trail[i]));
             goto END_OF_SECOND_BRANCH;
      }
  //1st inner branch %d in outer 2nd branch %d", inner, -outer
      inlit =mkLit(inner);
      newDecisionLevel();
      uncheckedEnqueue(inlit);
      confl = propagate();
      if (confl != CRef_Undef) {//1st inner branch failed literal %d on 2nd outer branch %d", inner, -outer
             cancelUntil(0);
             s_addliftbincls (outlit, ~inlit);
             newDecisionLevel();
             uncheckedEnqueue(~outlit);
             confl = propagate();
             if (confl == CRef_Undef) goto SECOND_OUTER_BRANCH_WIHOUT_INNER_PROBE;
                     // conflict after propagating negation of 1st inner branch
             goto SECOND_OUTER_BRANCH_FAILED;
       }
       saved.clear();
       for (i = oldtrailsize; i < trail.size(); i++) saved.push(trail[i]);
       cancelUntil(1);
              // 2nd inner branch %d in 2nd outer branch %d", -inner, -outer
       newDecisionLevel();
       uncheckedEnqueue(~inlit);
       confl = propagate(); 
       if (confl != CRef_Undef) {// 2nd inner branch failed literal %d on 2nd outer branch %d", -inner, -outer
              cancelUntil(0);
              s_addliftbincls (outlit, inlit);
              newDecisionLevel();
              uncheckedEnqueue(~outlit);
              confl = propagate();
              if (confl == CRef_Undef ) goto SECOND_OUTER_BRANCH_WIHOUT_INNER_PROBE;
                  //conflict after propagating negation of 2nd inner branch
              goto SECOND_OUTER_BRANCH_FAILED;
       }
       equal[1].push(toInt(inlit));
       while (saved.size()) {
               Lit lit = saved.pop_();
               if(value(lit) == l_True) represented[1].push(toInt(lit));
               else if(value(lit) == l_False && lit != inlit) equal[1].push(toInt(lit));
       }
END_OF_SECOND_BRANCH:
       cancelUntil(0);
       for (i = 0; i < represented[0].size(); i++) mark[represented[0][i]]=1;
       mark[2*outer]=mark[2*outer+1]=0;
       for (i = 0; i < represented[1].size(); i++) {
               int ulit=represented[1][i]; 
               if(mark[ulit]){//??? bug
                    if(assigns[ulit/2] != l_Undef) continue;

                    Lit q=toLit(ulit);
                    if(checkUnit (ulit, 'p')){//2017.1.17 bug?
                  unitq:
                         confl = unitPropagation(q);
                         if (confl == CRef_Undef) continue;
                         ret=l_False;
                         break;
                    }
                    else {
                        if(!out_binclause(outlit, q)) continue;
                        if(!out_binclause(~outlit, q)) continue;
                        goto unitq;
                    }
               }
               if(mark[ulit^1]){
                      Lit q=toLit(ulit^1);
                      ret = check_add_equ (outlit,q);// outlit => q
                      if(ret == l_False){
                            break;
                      }
               }
      }
      for (i = 0; i < represented[0].size(); i++) mark[represented[0][i]]=0;
      if(ret == l_False) break;
// p => A=B, ~p => A=B   ==>  A=B       
      for (i = 0; i < equal[0].size(); i++) mark[equal[0][i]]=1;
      int eqn=0;
      for (i = 0; i < equal[1].size(); i++) if(mark[equal[1][i]]) eqn++;
      if(eqn < 2){
         eqn=0;
         for (i = 0; i < equal[1].size(); i++){
                  int notl=equal[1][i]^1;
                  equal[1][i]=notl;
                  if(mark[notl]) eqn++;
         }
      }
      for (i = 0; i < equal[0].size(); i++) mark[equal[0][i]]=0;
      if(eqn >= 2){
            ret=addeqvlist(equal[0]);
            if(ret == l_False) break;
            ret=addeqvlist(equal[1]);
            if(ret == l_False) break;
      }
  }
  return ret;
}

lbool Solver :: s_lift ()
{ 
  if (nVars()>500000) return l_Undef;
  cancelUntil(0);
  mark = (char * ) calloc (2*nVars()+1, sizeof(char));
  int oldeqn = equsum;
  lbool ret= s_liftaux ();
  free(mark);

  if(verbosity > 0) printf("c lift old eqn=%d cur eqn=%d \n",  oldeqn,equsum);
     
  if(oldeqn != equsum && ret ==l_Undef) return replaceEqVar();
  return ret;
}

// simple lift or probe unit
lbool Solver :: s_probe ()
{ 
   Lit dom, rootlit;
   unsigned pos, delta;
   vec <int> probes;
   vec <Lit> lift, saved;
   int nprobes,oldtrailsize;

   for (int idx = 0; idx < nVars(); idx++) {
      if (assigns [idx] != l_Undef) continue;
      probes.push(idx);
   }
   nprobes = probes.size();
   if (nprobes < 20) return l_Undef;
   find_relative_prime(pos, delta, nprobes);
 
   int units=0;
   cancelUntil(0);
   lbool ret=l_Undef;
   int loop=nprobes-10;
   if(loop>10000) loop=10000;

   while ( --loop > 0 ) {
        int root = probes[pos];
        pos = (pos+delta) % nprobes;
        if (assigns [root] != l_Undef) continue;
        lift.clear();
        saved.clear();
    //-------------------------level==0
       oldtrailsize = trail.size();
       rootlit=mkLit(root);
       newDecisionLevel();
       uncheckedEnqueue(rootlit);
       CRef confl = propagate();
       if (confl == CRef_Undef) {
          for (int i = oldtrailsize; i < trail.size(); i++) saved.push(trail[i]);
       } 
       else {
             dom = s_prbana(confl, rootlit);
       }
       cancelUntil(0);
       if (confl != CRef_Undef) {// failed literal %d on probing, dom, root
               lift.push(~dom);
               goto LUNIT;
       }// next probe %d negative phase, -root
    //---------------------------------------
       newDecisionLevel();
       uncheckedEnqueue(~rootlit);
       confl = propagate();
       if (confl == CRef_Undef) {
          for (int i = 0; i <saved.size(); i++) 
                if(value(saved[i]) == l_True ) lift.push(saved[i]);
       } 
       else  dom = s_prbana(confl, ~rootlit);
       cancelUntil(0);
   //------------------------------------------------
      if (confl != CRef_Undef) {// failed literal %d on probing %d, dom, -root
             lift.push(~dom);
      }
LUNIT:
      for (int i = 0; i < lift.size(); i++){
             if(value(lift[i]) != l_Undef) continue;
             if(checkUnit (toInt(lift[i]), 's')){
                   units++;
                   confl = unitPropagation(lift[i]);
                   if (confl == CRef_Undef) continue;
                   ret=l_False;
                   goto DONE;
             }
      }
  }
DONE:
  return ret;
}

inline void Solver :: update_score(int ulit)
{
      int v=ulit/2;
      int pos=fullwatch[ulit].size();
      int neg=fullwatch[ulit^1].size();
      if(sched_heap.inHeap(v)){
            int old=vscore[v];
            if( !pos  || !neg ) vscore[v] = 0;
            else if(pos < 10000 && neg < 10000) vscore[v] = pos*neg;
                 else vscore[v]=10000000;
            if(old > vscore[v]) sched_heap.decrease(v);
            if(old < vscore[v]) sched_heap.increase(v);
      }
      else{
        if(decision[v] && (pos+neg) < 16){
             if(vscore[v] > pos*neg && pos+neg>6){
                   vscore[v] = pos*neg;
                   sched_heap.insert(v);
             }
        } 
     }
}        

void Solver::removefullclause(CRef cr) 
{
  Clause& c = ca[cr];
  for(int i=0; i<c.size(); i++){
        int ulit=toInt(c[i]);
        remove(fullwatch[ulit], cr);
        if(vscore.size()>=nVars()) update_score(ulit);
  }
  removeClause(cr);
}

void Solver :: s_addfullcls (vec<Lit> & ps, int learntflag)
{ 
    CRef cr = ca.alloc(ps, learntflag);
    if(!learntflag) clauses.push(cr);
    else            learnts.push(cr);
    for(int i=0; i<ps.size(); i++) {
             int ulit=toInt(ps[i]);
             fullwatch[ulit].push(cr);
             if(vscore.size()>=nVars()) update_score(ulit);
    }
    attachClause(cr);
}

void Solver :: buildfullwatch ()
{
    for (int i =0; i < 2*nVars(); i++) fullwatch[i].clear();
    for (int i =0; i < clauses.size(); i++){
        CRef cr =clauses[i];
        Clause& c = ca[cr];
        if (c.mark()==1) continue;
        for(int j=0; j<c.size(); j++) fullwatch[toInt(c[j])].push(cr);
    }
}       

int Solver :: s_chkoccs4elmlit (int ulit) 
{
   vec <CRef> &  cs  = fullwatch[ulit];
   int sz=cs.size();
   for (int i = 0; i<sz; i++) {
         CRef cr=cs[i];
         Clause& c = ca[cr];
         int size = 0;
         for(int j=c.size()-1; j>=0; j--){
             if(fullwatch[toInt(c[j])].size() > 800) return 0;
             if (++size > 600) return 0;
         }
   }
   return 1;
}

int Solver :: s_chkoccs4elm (int v) 
{
   if(fullwatch[2*v].size()   > 800) return 0;
   if(fullwatch[2*v+1].size() > 800) return 0;
   if (!s_chkoccs4elmlit (2*v)) return 0;
   return s_chkoccs4elmlit (2*v+1);
}

vec <int> elm_m2i, clv;
 
static const uint64_t s_basevar2funtab[6] = {
  0xaaaaaaaaaaaaaaaaull, 0xccccccccccccccccull, 0xf0f0f0f0f0f0f0f0ull,
  0xff00ff00ff00ff00ull, 0xffff0000ffff0000ull, 0xffffffff00000000ull,
};

// mapped external variable to marked variable
int Solver :: s_s2m (int ulit) 
{  int v=ulit/2;
   int res = markNo[v];
   if (!res) {
       elm_m2i.push(v); 
       res = elm_m2i.size();
       if (res > 11) return 0;
       markNo[v] = res;
  }
  return 2*res + (ulit%2);
}

// mapped external variable to marked variable
int Solver :: s_i2m (int ulit) 
{  int v=ulit/2;
   int res = markNo[v];
   if (!res) {
        elm_m2i.push(v);
        res = elm_m2i.size();
        markNo[v] = res;
  }
  return 2*res - 2 + (ulit%2);
}

// map marked variable to external variable
int Solver :: s_m2i (int mlit) {
  int res, midx = mlit/2;
  res = elm_m2i[midx-1];
  return 2*res + (mlit%2);
}

void s_var2funaux (int v, Fun res, int negate) {
  uint64_t tmp;
  int i, j, p;
  if (v < 6) {
    tmp = s_basevar2funtab[v];
    if (negate) tmp = ~tmp;
    for (i = 0; i < FUNQUADS; i++) res[i] = tmp;
  } else {
    tmp = negate ? ~0ull : 0ull;
    p = 1 << (v - 6);
    j = 0;
    for (i = 0; i < FUNQUADS; i++) {
      res[i] = tmp;
      if (++j < p) continue;
      tmp = ~tmp;
      j = 0;
    }
  }
}

static Cnf s_pos2cnf (int pos)  { return pos; }
static Cnf s_size2cnf (int s)   { return ((Cnf)s) << 32; }
static int s_cnf2pos (Cnf cnf)  { return cnf & 0xfffffll; }
static int s_cnf2size (Cnf cnf) { return cnf >> 32; }
static Cnf s_cnf (int pos, int size) {
  return s_pos2cnf (pos) | s_size2cnf (size);
}

void s_negvar2fun (int v, Fun res) {  s_var2funaux (v, res, 1);}
void s_var2fun (int v, Fun res)    {  s_var2funaux (v, res, 0);}

void s_s2fun (int marklit, Fun res) 
{
  int sidx = marklit/2 - 2;
  if ( marklit & 1 ){
        s_negvar2fun (sidx, res);
  }
  else {
       s_var2fun (sidx, res);
  }
}

static void s_orfun (Fun a, const Fun b) {
  for (int i = 0; i < FUNQUADS; i++) a[i] |= b[i];
}

static void s_andfun (Fun a, const Fun b) {
  for (int i = 0; i < FUNQUADS; i++)    a[i] &= b[i];
}

static void s_or3fun (Fun a, const Fun b, const Fun c) {
  for (int i = 0; i < FUNQUADS; i++)  a[i] = b[i] | c[i];
}

static void s_and3negfun (Fun a, const Fun b, const Fun c) {
  for (int i = 0; i < FUNQUADS; i++)    a[i] = b[i] & ~c[i];
}

static void s_and3fun (Fun a, const Fun b, const Fun c) {
  for (int i = 0; i < FUNQUADS; i++)    a[i] = b[i] & c[i];
}

static void s_andornegfun (Fun a, const Fun b, const Fun c) {
  for (int i = 0; i < FUNQUADS; i++)    a[i] &= b[i] | ~c[i];
}

static void s_funcpy (Fun dst, const Fun src) {
  for (int i = 0; i < FUNQUADS; i++) dst[i] = src[i];
}

static void s_ornegfun (Fun a, const Fun b) {
  for (int i = 0; i < FUNQUADS; i++) a[i] |= ~b[i];
}

static void s_slfun (Fun a, int shift) {
  uint64_t rest, tmp;
  int i, j, q, b, l;
  b = shift & 63;
  q = shift >> 6;
  j = FUNQUADS - 1;
  i = j - q;
  l = 64 - b;
  while (j >= 0) {
    if (i >= 0) {
      tmp = a[i] << b;
      rest = (b && i > 0) ? (a[i-1] >> l) : 0ll;
      a[j] = rest | tmp;
    } else a[j] = 0ll;
    i--, j--;
  }
}

static void s_srfun (Fun a, int shift) {
  uint64_t rest, tmp;
  int i, j, q, b, l;
  b = shift & 63;
  q = shift >> 6;
  j = 0;
  i = j + q;
  l = 64 - b;
  while (j < FUNQUADS) {
    if (i < FUNQUADS) {
      tmp = a[i] >> b;
      rest = (b && i+1 < FUNQUADS) ? (a[i+1] << l) : 0ull;
      a[j] = rest | tmp;
    } else a[j] = 0ull;
    i++, j++;
  }
}

static void s_falsefun (Fun res) {
  for (int i = 0; i < FUNQUADS; i++)   res[i] = 0ll;
}

static void s_truefun (Fun res) {
  for (int i = 0; i < FUNQUADS; i++)
    res[i] = (unsigned long long)(~0ll);
}

static int s_isfalsefun (const Fun f) {
  for (int i = 0; i < FUNQUADS; i++)    if (f[i] != 0ll) return 0;
  return 1;
}

static int s_istruefun (const Fun f) {
  for (int i = 0; i < FUNQUADS; i++) if (f[i] != (unsigned long long)(~0ll)) return 0;
  return 1;
}

static void s_negcofactorfun (const Fun f, int v, Fun res) {
  Fun mask, masked;
  s_var2fun (v, mask);            //mask <- v
  s_and3negfun (masked, f, mask); //masked = f & ~mask;
  s_funcpy (res, masked);         //res <-masked
  s_slfun (masked, (1 << v));     // masked << v
  s_orfun (res, masked);          // res = res | masked
}

static void s_poscofactorfun (const Fun f, int v, Fun res) {
  Fun mask, masked;
  s_var2fun (v, mask);     //mask <- v
  s_and3fun (masked, f, mask); // //masked = a[i] = f & v
  s_funcpy (res, masked);      
  s_srfun (masked, (1 << v)); //// masked >> v
  s_orfun (res, masked);   // res = res | masked
}

static int s_eqfun (const Fun a, const Fun b) {
  for (int i = 0; i < FUNQUADS; i++)    if (a[i] != b[i]) return 0;
  return 1;
}

static int s_smalltopvar (const Fun f, int min) {
  Fun p, n;
  int v;
  for (v = min; v < FUNVAR; v++) {
    s_poscofactorfun (f, v, p);
    s_negcofactorfun (f, v, n);
    if (!s_eqfun (p, n)) return v;
  }
  return v;
}

static void s_or3negfun (Fun a, const Fun b, const Fun c) {
  for (int i = 0; i < FUNQUADS; i++)  a[i] = b[i] | ~c[i];
}

static void s_smallevalcls (unsigned cls, Fun res) {
  Fun tmp;
  int v;
  s_falsefun (res);
  for (v = 0; v < FUNVAR; v++) {
    if (cls & (1 << (2*v + 1))) {
      s_var2fun (v, tmp);
      s_ornegfun (res, tmp);
    } else if (cls & (1 << (2*v))) {
      s_var2fun (v, tmp);
      s_orfun (res, tmp);
    }
  }
}

static void s_smallevalcnf (Cnf cnf, Fun res) {
  Fun tmp;
  int i, n, p, cls;
  p = s_cnf2pos (cnf);
  n = s_cnf2size (cnf);
  s_truefun (res);
  for (i = 0; i < n; i++) {
    cls = clv[p + i];
    s_smallevalcls (cls, tmp);
    s_andfun (res, tmp);//res = res & cls
  }
}

int Solver :: s_initsmallve (int ulit, Fun res) 
{
  Fun cls, tmp;
  //initializing small variable eliminiation for %d", lit);
  s_s2m (ulit);
  s_truefun (res);
  Lit pivot=toLit(ulit);
  int count = fullwatch[ulit].size();

  vec<CRef>&  crs  = fullwatch[ulit];
  int clsCnt=0;
  for(int i = 0; i < count; i++){
        CRef cr=crs[i];
        Clause& c = ca[cr];
        s_falsefun (cls);// cls <- 000000
        for(int j = 0; j < c.size(); j++){
              if( value(c[j]) != l_Undef ){
                    if( value(c[j]) == l_True ) goto NEXT;
              }
              if( c[j] == pivot ) continue;
              int mlit = s_s2m (toInt(c[j]));
              if (!mlit) {
                     return 0;
	      }
              s_s2fun (mlit, tmp); //convert to fun 
	      s_orfun (cls, tmp);  // cls = cls | tmp 
        }
        clsCnt=1;
        s_andfun (res, cls); //res = res & cls
  NEXT:  ;
  }
  return clsCnt;
}

static Cnf s_smalladdlit2cnf (Cnf cnf, int lit) {
  int p, m, q, n, i, cls;
  Cnf res;
  p = s_cnf2pos (cnf);
  m = s_cnf2size (cnf);
  q = clv.size();
  for (i = 0; i < m; i++) {
    cls = clv[p + i];
    cls |= lit;
    clv.push(cls);
  }
  n = clv.size() - q;
  res = s_cnf (q, n);
  return res;
}

#define FALSECNF	(1ll<<32)
#define TRUECNF		0ll

static Cnf s_smallipos (const Fun U, const Fun L, int min) {
  Fun U0, U1, L0, L1, Unew, ftmp;
  Cnf c0, c1, cstar, ctmp, res;
  int x, y, z;
  if (s_istruefun (U)) return TRUECNF;  //U=11111
  if (s_isfalsefun (L)) return FALSECNF;//U=00000
  y = s_smalltopvar (U, min);
  z = s_smalltopvar (L, min);
  x = (y < z) ? y : z;

  s_negcofactorfun (U, x, U0); // U0 = U & ~x 
  s_poscofactorfun (U, x, U1); // U1 = U & x
  
  s_negcofactorfun (L, x, L0); // L0 = L & ~x
  s_poscofactorfun (L, x, L1); // L1 = L & x

  s_or3negfun (ftmp, U0, L1); // ftmp = U0 | ~L1 => ftmp = (U & ~x) | ~(L & x)  
  c0 = s_smallipos (ftmp, L0, min+1);    // L0= L & ~x

  s_or3negfun (ftmp, U1, L0); // ftmp = U1 | ~L0 => ftmp = (U & x) | ~(L & ~x)
  c1 = s_smallipos (ftmp, L1, min+1);

  s_smallevalcnf (c0, ftmp);     // ftmp <- c0
  s_or3negfun (Unew, U0, ftmp); // Unew = U0 | ~c0;
 
  s_smallevalcnf (c1, ftmp);
  s_andornegfun (Unew, U1, ftmp); // Unew = Unew & (U1 | ~c1);

  s_or3fun (ftmp, L0, L1);//ftmp = L0 | L1
  cstar = s_smallipos (Unew, ftmp, min+1);

  ctmp = s_smalladdlit2cnf (c1, (1 << (2*x + 1)));
  res = s_cnf2pos (ctmp);

  ctmp = s_smalladdlit2cnf (c0, (1 << (2*x)));
  if (res == TRUECNF) res = s_cnf2pos (ctmp);

  ctmp = s_smalladdlit2cnf (cstar, 0);
  if (res == TRUECNF) res = s_cnf2pos (ctmp);

  res |= s_size2cnf (clv.size() - res);
  return res;
}

int Solver :: s_smallisunitcls (int cls) {
  int fidx, fsign, flit, ulit;
  ulit = -1;
  for (fidx = 0; fidx < FUNVAR; fidx++)
    for (fsign = 0; fsign <= 1; fsign++) {
      flit = 1<<(2*fidx + fsign);
      if (!(cls & flit)) continue;
      if (ulit>=0) return -1;
      int mlit = (fidx + 2) * 2 + fsign;
      ulit = s_m2i (mlit);
  }
  return ulit;
}

int Solver :: s_smallcnfunits (Cnf cnf) 
{
  int p, m, i, cls, ulit;
  p = s_cnf2pos (cnf);
  m = s_cnf2size (cnf);
  int units = 0;
  for (i = 0; i < m; i++) {
      cls = clv[p + i];
      ulit = s_smallisunitcls (cls);
      if (ulit < 0) continue;
      units++;
  }
  return units;
}

lbool Solver :: s_smallve (Cnf cnf, int pivotv, bool newaddflag) 
{
  int cls, v, trivial;
  vec <Lit> newCls;
  int end=s_cnf2pos (cnf)+s_cnf2size (cnf);
  int elimFlag=1;
  int count=0;
  for (int i = end-1; i >= s_cnf2pos (cnf); i--) {
     cls = clv[i];
     trivial = 0;
     newCls.clear();
     for (v = 0; v < FUNVAR; v++) {
         int ulit;
         if (cls & (1 << (2*v + 1)))  ulit = s_m2i (2*v+5);
         else if (cls & (1 << (2*v))) ulit = s_m2i (2*v+4);
              else continue;
         Lit lit=toLit(ulit);
         if (value (lit) == l_False) continue;
         if (value (lit) == l_True) trivial = 1;
         newCls.push(lit);
     }
     if (!trivial) {//small elimination resolvent
          count++;
          if(newCls.size()>1) {
              if(newaddflag) s_addfullcls (newCls, 0);
          }
          else {
             if(newCls.size()==1){
                  if(checkUnit (toInt(newCls[0]), 's')){
                      CRef confl = unitPropagation(newCls[0]);
                      if (confl != CRef_Undef) return l_False;
                  }
                  else elimFlag=0;
             }
             else elimFlag=0;
          } 
     }
  }
  if(elimFlag && count && newaddflag){
      s_epusheliminated (pivotv);
  }
  return l_Undef;
}

void Solver :: s_epusheliminated (int v) 
{ 
   setDecisionVar(v, false);
   int pos=fullwatch[2*v].size();
   int neg=fullwatch[2*v+1].size();
   deletedCls.clear();
   if(pos==0 && neg==0) return;
   int ulit= (pos <= neg) ? (2*v) : (2*v+1);
   vec<CRef> &  pcs  = fullwatch[ulit];
   Lit pivot=toLit(ulit);
   for(int i = 0; i < pcs.size(); i++){
        CRef pcr=pcs[i];
        deletedCls.push(pcr);
        Clause& c = ca[pcr];
        extendRemLit.push (lit_Undef);
        extendRemLit.push(pivot);
        for(int k = 0; k < c.size(); k++){
               if (c[k] == pivot) continue;
               extendRemLit.push(c[k]);
        }
  }
  extendRemLit.push (lit_Undef);
  extendRemLit.push(~pivot);
  vec<CRef> &  ncs  = fullwatch[ulit^1];
  for(int i = 0; i < ncs.size(); i++) deletedCls.push(ncs[i]);
}

lbool Solver :: s_trysmallve (int v, int & res) 
{
  int newsz, old, units;
  Fun pos, neg, fun;
  Cnf cnf;

  elm_m2i.clear();
  clv.clear();
  clv.push(0);
  res = 0;
//too many variables for small elimination
  if (s_initsmallve (2*v, pos) && s_initsmallve (2*v+1, neg)) {
   //     printPivotClause (2*v);
        s_or3fun (fun, pos, neg); //fun = pos | neg
        cnf = s_smallipos (fun, fun, 0);
        newsz = s_cnf2size (cnf);
        units = s_smallcnfunits (cnf);
        old = fullwatch[2*v].size() + fullwatch[2*v+1].size();
        //small elimination of %d replaces "%d old with %d new clauses and %d units",idx, old, newsz, units);
        if ((newsz-units) < old || units > 0) {
                int cnt=s_smallvccheck (v);
                if(cnt==newsz){
                    res = 1;
                    return s_smallve (cnf, v, newsz <= old);
                }
                if(cnt<old) {
                     res = 1;
                     return s_forcedve (v);
                }
                if(units > 0){
                    res = 1;
                    return s_smallve (cnf, v, 0);
                } 
        }
  } 
  return l_Undef;
}

lbool Solver ::s_forcedve (int v) 
{
   int pos=fullwatch[2*v].size();
   int neg=fullwatch[2*v+1].size();
   if(pos==0 && neg==0) return l_Undef;
 
   int pivot = (pos <= neg) ? (2*v) : (2*v+1);
   int npivot = pivot^1;
   int elimflag=1;
   int prevsize;
   lbool ret = l_Undef;
   vec<Lit> newcls;
   vec<CRef> &  pcs  = fullwatch[pivot];
   vec<CRef> &  ncs  = fullwatch[npivot];
   Lit plit=toLit(pivot);
   Lit nlit=~plit;
   for(int i = 0; i < pcs.size(); i++){
        CRef pcr=pcs[i];
        Clause & a = ca[pcr];
        newcls.clear();
        for(int k = 0; k < a.size(); k++){
               if (a[k] == plit) continue;
               if (value(a[k]) == l_False) continue;
               if (value(a[k]) == l_True) goto DONE;
               mark[toInt(a[k])]=1;
               newcls.push(a[k]);
        }
        if(newcls.size()==0) return l_Undef;
        prevsize=newcls.size();
        for(int j = 0; j < ncs.size(); j++){
               CRef ncr=ncs[j];
               Clause& b = ca[ncr];
               for(int k = 0; k < b.size(); k++){
                     if (b[k] == nlit) continue;
                     if (value(b[k]) == l_False) continue;
                     if (value(b[k]) == l_True) goto NEXT;
                     int ul=toInt(b[k]);
                     if (mark[ul]) continue;
                     if (mark[ul^1]) goto NEXT;
                     newcls.push(b[k]);
               }
               if(newcls.size()==1){
                    if( value(newcls[0]) != l_Undef ){
                               elimflag=0;
                               goto DONE;
                     }
                     if(checkUnit (toInt(newcls[0]), 'e')){
                            CRef confl = unitPropagation(newcls[0]);
                            if (confl != CRef_Undef) {ret=l_False; goto DONE;}
                       }
                       else elimflag=0; 
              }
              else{
                  s_addfullcls (newcls, 0);
              }
        NEXT:  
              newcls.shrink(newcls.size()-prevsize);
        } 
  DONE:
        for(int k = 0; k < a.size(); k++) mark[toInt(a[k])]=0;
        if(ret==l_False || elimflag==0) return ret;
   }
   if(elimflag)  s_epusheliminated (v);
   return ret;
}

int Solver ::s_smallvccheck (int v) 
{
   int pos=fullwatch[2*v].size();
   int neg=fullwatch[2*v+1].size();
   int pivot = (pos <= neg) ? (2*v) : (2*v+1);
   int npivot = pivot^1;
   vec<CRef> &  pcs  = fullwatch[pivot];
   vec<CRef> &  ncs  = fullwatch[npivot];
   Lit plit=toLit(pivot);
   Lit nlit=~plit;
   int oldsz=pcs.size()+ncs.size();
   int newsz=0;
   for(int i = 0; i < pcs.size(); i++){
        CRef pcr=pcs[i];
        Clause& a = ca[pcr];
        for(int k = 0; k < a.size(); k++){
               if (a[k] == plit) continue;
               mark[toInt(a[k])]=1;
        }
        for(int j = 0; j < ncs.size(); j++){
               CRef ncr=ncs[j];
               Clause & b = ca[ncr];
               for(int k = 0; k < b.size(); k++){
                     if (b[k] == nlit) continue;
                     int ul=toInt(b[k]);
                     if (mark[ul]) continue;
                     if (mark[ul^1]) goto NEXT;
               }
               newsz++;
               if(newsz>=oldsz) goto DONE;
        NEXT: ;
        } 
  DONE:
        for(int k = 0; k < a.size(); k++) mark[toInt(a[k])]=0;
        if(newsz>=oldsz) return newsz;
   }
   return newsz;
}

int Solver :: s_forcedvechk (int v) 
{
  int pos=fullwatch[2*v].size();
  int neg=fullwatch[2*v+1].size();
  if (pos >= (1<<15)) return 0;
  if (neg >= (1<<15)) return 0;
  int old = pos + neg;
  int newpn = pos * neg;
  return newpn <= old;
}

typedef struct ELM_CLAUSE { CRef cr; int size; unsigned csig;} ELM_CLAUSE;
vec <ELM_CLAUSE> eclsInfo; //elim clause info

// backward subsume and strengthened 
int Solver :: s_backsub (int cIdx, int startidx, int lastIdx, Lit pivot, int streFlag)
{
   int size = eclsInfo[cIdx].size;
   unsigned csig = eclsInfo[cIdx].csig;
   int res = 0;
   int marked=0;

   for (int i = startidx; i<lastIdx; i++){
        if(i == cIdx) continue;
        int osize = eclsInfo[i].size;
        if (osize > size || osize==0) continue;//size=0 => deleted
        unsigned ocsig = eclsInfo[i].csig;
        if ((ocsig & ~csig)) continue;
        if (!marked) {
               CRef cr=eclsInfo[cIdx].cr;
               Clause& c = ca[cr];
               for(int j = 0; j < c.size(); j++){
                    int ul=toInt(c[j]);
                    if(streFlag && pivot==c[j])  ul=ul^1;
                    mark[ul]=1;
               }
               marked = 1;
        }
        CRef cr=eclsInfo[i].cr;
        Clause & c = ca[cr];
        res=1;
        for(int k = 0; k < c.size(); k++){
              res=mark[toInt(c[k])];
              if(res==0) break;
        }
        if ( !res || !streFlag || osize < size) {
               if(res) break;
               continue;
        }
       //strengthening by double self-subsuming resolution, 
      // strengthened and subsumed original irredundant clause
      
        deletedCls.push(cr);
        eclsInfo[i].size=0;
        break;
  }
  if (marked) {
         CRef cr=eclsInfo[cIdx].cr;
         Clause & c = ca[cr];
         for(int j = 0; j < c.size(); j++){
                int ul=toInt(c[j]);
                if(streFlag && pivot==c[j])  ul=ul^1;
                mark[ul]=0;
         }
   }
   return res;
}
// elimiate subsume
void  Solver :: s_elmsub (int v) 
{
  Lit pivot = toLit(2*v);
  int lastIdx=eclsInfo.size();
  int ret;
  for (int cIdx =0;  cIdx < lastIdx; cIdx++){
       if(cIdx < neglidx){
            if(neglidx<2) return;
            ret = s_backsub (cIdx, 0,       neglidx, pivot, 0);
       }
       else {
            if(lastIdx-neglidx<2) return;
            ret = s_backsub (cIdx, neglidx, lastIdx, ~pivot,0);
       }
       if (ret) {   //subsumed original irredundant clause, subsumed mapped irredundant clause
            deletedCls.push(eclsInfo[cIdx].cr);
            eclsInfo[cIdx].size=0;
       } 
  }
  //subsumed %d clauses containing %d or %d,subsumed, spf->elm.pivot, -spf->elm.pivot);
}

  // strengthening with pivot 
lbool Solver :: s_elmstr (int v, int & res) 
{ int ret;
  vec <Lit> newCls; 
  Lit lit = toLit(2*v);
  int lastIdx=eclsInfo.size();
  if(neglidx==0 || neglidx >= lastIdx) return l_Undef;
  Lit pivot;
  for (int cIdx =0;  cIdx < lastIdx; cIdx++){
//strengthening with pivot
       for(int i=0; i < deletedCls.size();i++) if(eclsInfo[cIdx].cr==deletedCls[i]) goto NEXT;
       if(cIdx < neglidx){
              pivot= lit;
              ret =s_backsub (cIdx, neglidx, lastIdx, pivot, 1);
       }else  {
              pivot=~lit; 
              ret =s_backsub (cIdx, 0,       neglidx, pivot, 1);
       }
       if (ret) {   //subsumed original irredundant clause, subsumed mapped irredundant clause
              CRef cr=eclsInfo[cIdx].cr;
              deletedCls.push(cr);
              eclsInfo[cIdx].size=0;
              newCls.clear();
              Clause& c = ca[cr];

              for(int j = 0; j < c.size(); j++){
                   if(pivot==c[j]) continue;
                   newCls.push(c[j]);
              }
              if(newCls.size()==1){
                       res = 1;
                       if(value(newCls[0]) != l_Undef){
                             return l_Undef;
                       }
                       if(checkUnit (toInt(newCls[0]), 'D')){
                              CRef confl = unitPropagation(newCls[0]);
                              if (confl != CRef_Undef) return l_False;
                       }
                       return l_Undef;
             }
             if(newCls.size() >1) s_addfullcls (newCls, 0);
       }
NEXT:  ; 
  }
  res=0;
  return l_Undef;
}

//blocked clause elimination and forced resolution of clauses
void Solver :: s_elmfrelit (Lit pivot, int startp, int endp, int startn, int endn) 
{ int k;
  int cover = fullwatch[toInt(~pivot)].size();
  for (int i= startp;  i < endp; i++) {
        CRef cr=eclsInfo[i].cr;
        Clause & a = ca[cr];
        int maxcover = 0;
        for(k = 0; k < a.size(); k++){
              int ul=toInt(a[k]);
              maxcover += fullwatch[ul^1].size();
        }
        if (maxcover < 2*cover - 1) continue;
        for(k = 0; k < a.size(); k++) mark[toInt(a[k])]=1;
        int nontrivial = 0;
        for (int j= startn;  j < endn; j++) {
                CRef bcr=eclsInfo[j].cr;
                Clause & b = ca[bcr];
                int m=0;
                for(k = 0; k < b.size(); k++){
                       m = m+ mark[toInt(b[k])^1];
                       if(m >= 2) break;
                }
                if(m < 2) {	//non trivial resolvent in blocked clause elimination
                    nontrivial = 1;
                    break;
                }
        }
        for(k = 0; k < a.size(); k++) mark[toInt(a[k])]=0;
        if(nontrivial == 0){
              extendRemLit.push (lit_Undef);
              extendRemLit.push(pivot);
              for(k = 0; k < a.size(); k++) {
                     if(pivot == a[k]) continue;
                     extendRemLit.push(a[k]);
             }
             deletedCls.push(cr);
       }
   }
}

void Solver :: s_elmfre (Lit pivot) 
{
  s_elmfrelit (pivot, 0, neglidx, neglidx,eclsInfo.size());
  s_elmfrelit (~pivot, neglidx, eclsInfo.size(),0,neglidx);
}

#define PUSHDF(POSNEG, n, LIT) \
do { \
  if(!dfpr[LIT].discovered) break; \
  POSNEG[n].ulit = LIT; \
  POSNEG[n].discovered = dfpr[LIT].discovered; \
  POSNEG[n++].finished = dfpr[LIT].finished; \
} while (0)

typedef struct DF { int discovered, finished; int ulit;} DF;
DF *pos_df,*neg_df;
// DF *pos_df  = (DF *)malloc (size*sizeof(DF));
 // DF *neg_df = (DF *)malloc (size*sizeof(DF));
 
int cmpdf (const void * a, const void * b)
{  DF * c =(DF *)a;
   DF * d =(DF *)b;
   return c->discovered - d->discovered;
}

int Solver :: s_uhte (vec <Lit> & resolv) 
{
  int size = resolv.size();
  int i, p, n;
  if (size <= 2 || size > 1000) return 0;

  int posN=0;
  int negN=0;
  for (i = 0; i < size; i++) {
      int ulit = toInt(resolv[i]);
      PUSHDF (pos_df, posN,ulit);
      int nlit = ulit^1;
      PUSHDF (neg_df, negN,nlit);
  }
  if (posN==0 || negN==0) return 0;
//  SORT3 (spf->df.pos.dfs, spf->df.pos.ndfs, s_cmpdf);
   Adp_SymPSort((void *)pos_df, posN, sizeof(DF), cmpdf);
   Adp_SymPSort((void *)neg_df, negN, sizeof(DF), cmpdf);
  p = n = 0;
  for (;;) {
    if (neg_df[n].discovered > pos_df[p].discovered) {
      if (++p == posN) return 0;
    } 
    else if (neg_df[n].finished < pos_df[p].finished) {
      if (++n == negN) return 0;
    } 
    else {
        vec<Lit> ps;
        ps.push(~toLit(neg_df[n].ulit));
        ps.push(toLit(pos_df[p].ulit));
        if(is_conflict(ps)) return 1;
        return 0;
    }
  }
}

lbool Solver :: s_trylargeve (int v, int & res) 
{
  int ulit = 2*v;
  res=0;
  for (int i = 0; i <= 1; i++) {
      int clit = ulit^i;
      int n = fullwatch[clit].size();
      vec<CRef> &  crs  = fullwatch[clit];
      for (int j =0;  j < n; j++) {
           CRef cr=crs[j];
           Clause& c = ca[cr];
           for(int k = 0; k < c.size(); k++){
               if(fullwatch[toInt(c[k])].size() >1000) return l_Undef;//number of occurrences of %d larger than limit, ilit
           }
      }
  }
  int limit = fullwatch[ulit].size()+fullwatch[ulit^1].size();;
  // try clause distribution for %d with limit %d ip, limit
  int nlit=ulit^1;
  int n = fullwatch[ulit].size();
  int m = fullwatch[nlit].size();
  Lit pivot = toLit(ulit);
  vec<CRef> &  acrs  = fullwatch[ulit];
  vec<CRef> &  bcrs  = fullwatch[nlit];
  vec<CRef> PairCref;
  vec <Lit> resolvent;
  int j, leftsize;
  for (int i =0;  i < n && limit >= 0; i++) {
         CRef acr=acrs[i];
         Clause& a = ca[acr];
         int asize=a.size();
         resolvent.clear();
         for(int k = 0; k < asize; k++){
                if(a[k] == pivot) continue;
                if (value(a[k]) == l_False) continue;
                if (value(a[k]) == l_True) goto DONE1;
                mark[toInt(a[k])]=1;
                resolvent.push(a[k]);
         }
         leftsize = resolvent.size();
         if(leftsize==0){
                limit = -2; res=1;
                goto DONE1;
         }
         for (int j=0;  j < m && limit >= 0; j++) {
                 CRef bcr=bcrs[j];
                 Clause & b = ca[bcr];
                 int bsize=b.size();
                 int taut=0;     
                 for(int k = 0; k < bsize; k++){
                       if(b[k] == ~pivot) continue;
                       if (value(b[k]) == l_False) continue;
                       if (value(b[k]) == l_True) { taut=1; break;}
                       int ul=toInt(b[k]);
                       taut = mark[ul^1];
                       if(taut) break;
                       if(mark[ul]==0) resolvent.push(b[k]);
                 }       
                 int rsize=resolvent.size();
                 if (taut == 0){
                        if(  rsize == 1) {//trying resolution ends with unit clause");
                             limit += fullwatch[toInt(resolvent[0])].size();
                             goto addpair;
                        }
                         //trying resolution ends with hidden tautology");
                       if(dfpr && (rsize > 2 || asize > 1 || bsize > 1) && s_uhte (resolvent)) goto NEXT;
                       limit--;
      addpair:
                       PairCref.push(acr);
                       PairCref.push(bcr);
                }
         NEXT:  resolvent.shrink(rsize-leftsize);
         }
     DONE1:                
         for(int k = 0; k < asize; k++) mark[toInt(a[k])]=0;
   }
   if (limit < 0) return l_Undef; // resolving away %d would increase number of clauses;
   int elimflag=1; 
   lbool ret=l_Undef;
   for(int i=0; i<PairCref.size(); ){
         CRef acr=PairCref[i];
         Clause& a = ca[acr];
         resolvent.clear();
         for(int k = 0; k < a.size(); k++) {
               if(a[k] == pivot) continue;
               if (value(a[k]) == l_False) continue;
               mark[toInt(a[k])]=1;
               resolvent.push(a[k]);
         }
         int leftsize=resolvent.size();
         for(j=i+1;  j<PairCref.size() && acr==PairCref[j-1]; j+=2){
               CRef bcr=PairCref[j];
               Clause& b = ca[bcr];
               for(int k = 0; k < b.size(); k++) {
                   if(b[k] == ~pivot) continue;
                   if (value(b[k]) == l_False) continue;
                   if(mark[toInt(b[k])]==0) resolvent.push(b[k]);
               }
                if(resolvent.size()==1){
                       if(value(resolvent[0]) != l_Undef){
                               elimflag=0;
                               goto DONE2;
                       }
                       if(checkUnit (toInt(resolvent[0]), 'l')){
                            CRef confl = unitPropagation(resolvent[0]);
                            if (confl != CRef_Undef) {ret=l_False; goto DONE2;}
                       }
                       else elimflag=0; 
              }
              else {
                  s_addfullcls (resolvent, 0);
              }
              resolvent.shrink(resolvent.size()-leftsize);
         }
         i=j-1;
DONE2:
         for(int k = 0; k < a.size(); k++) mark[toInt(a[k])]=0;
         if (ret==l_False || elimflag==0) return ret;
   }
   if(elimflag) s_epusheliminated (v);
   res=1;
   return l_Undef;
}

lbool Solver :: s_tryforcedve (int v, int & success) 
{
    if (!s_forcedvechk (v)){
        success=0;
        return l_Undef;
    }
   // printPivotClause (2*v);
   // printPivotClause (2*v+1);
    success=1;       
    return s_forcedve (v);
}

lbool Solver :: s_elimlitaux (int v) 
{
   s_elmsub (v);
 //  if(deletedCls.size()) return  l_Undef;
   return  l_Undef;
}

// init eliminate clause
int Solver :: s_initecls (int v) 
{
   eclsInfo.clear();
   elm_m2i.clear();
   neglidx = fullwatch[2*v].size();
  
  if(!s_ecls (2*v)) return 0;
  if(!s_ecls (2*v+1)) return 0;
  return 1;
}

unsigned s_sig (int ulit) 
{
  unsigned res = (1u << (ulit & 31));
  return res;
}

int Solver :: s_addecl (CRef cr, Lit pivot)
{
    unsigned csig = 0;
    Clause& c = ca[cr];
    int idx=eclsInfo.size();
    eclsInfo.push();
    eclsInfo[idx].cr=cr;
    eclsInfo[idx].size=c.size();
    for(int i = 0; i < c.size(); i++){
          if( pivot == c[i] ) continue;
          int ul=toInt(c[i]);
          int markedlit = s_i2m (ul);
          if(markedlit>1000) return 0;
          csig |= s_sig (markedlit);
    }
    eclsInfo[idx].csig = csig;
   // for(int i = 0; i < c.size(); i++){
     //     int ul=toInt(c[i]);
    //      int umlit = s_i2m (ul);
         // if(umlit <= elm_lsigs.size()) {
         //     elm_lsigs.growTo(umlit+1);
         //     elm_lsigs[umlit] = csig;
         // }
         // else elm_lsigs[umlit] |= csig;
       //   if(umlit>1998) return 0;
        //  cls_idx[umlit].push(idx);
   // }
    return 1;
}

int Solver :: s_ecls (int ulit) 
{
   int occNum = fullwatch[ulit].size();
   vec<CRef> &  crs  = fullwatch[ulit];
   Lit lit=toLit(ulit);
   for(int i = 0; i < occNum; i++){
         if(!s_addecl (crs[i], lit)) return 0;
   }
   return 1;
}

inline void Solver :: clearMarkNO()
{ 
   for(int i=0; i<elm_m2i.size(); i++) markNo[elm_m2i[i]]=0;
   elm_m2i.clear();
}   
// trying to eliminate v
lbool Solver :: s_elimlit (int v) 
{
     if (!s_chkoccs4elm (v)) return l_Undef;
     int success=0;
     lbool ret=l_Undef;
     int pos=fullwatch[2*v].size();
     int neg=fullwatch[2*v+1].size();
     if(pos==0 && neg==0) return l_Undef;
     deletedCls.clear();
     if(pos==0 || neg==0){
         s_epusheliminated (v);
         goto DONE;
     }
     ret = s_trysmallve (v, success);
     clearMarkNO();
     if(success || ret == l_False) goto DONE;

   //  ret = s_tryforcedve (v, res);//inefficient
   //   clearMarkNO(); 
   //  if(res || ret == l_False) goto DONE;

   //  replaceClause (2*v);
   //  replaceClause (2*v+1);
   //  printPivotClause (2*v);
   //  printPivotClause (2*v+1);
   
    if(s_initecls (v)){
          ret=s_elimlitaux (v);
   }

DONE: 
     clearMarkNO();
     for(int i=0; i<deletedCls.size(); i++) {
          CRef cr=deletedCls[i];
          if(ca[cr].mark()==0) removefullclause(cr);
     }
   
     deletedCls.clear();
     eclsInfo.clear();
     return ret;
}

void Solver ::  s_eschedall() 
{   int nV=nVars();
    vscore.growTo(nV);
    sched_heap.clear();
    for(int v=0; v<nV; v++){
          if (assigns[v] == l_Undef && decision[v]){
              int pos=fullwatch[2*v].size();
              int neg=fullwatch[2*v+1].size();
              if( !pos  || !neg ) vscore[v] = 0;
              else  if(pos < 10000 && neg < 10000) vscore[v] = pos*neg;
                    else continue;
             sched_heap.insert(v);
         }
    }
}

void Solver :: addlearnt()
{
   for (int i =0; i < learnts.size(); i++){
           CRef  cr=learnts[i];
           Clause & c = ca[cr];
	   int sz=0;
           for (int j = 0; j < c.size(); j++) {
		Lit p=c[j];
          	if (value(p) == l_True) goto nextlc;
		if (value(p) == l_False) continue;
                sz++;
          }
          if(sz==2) {
                 c.clearlearnt();
                 clauses.push(cr);
                 continue;
          }
        nextlc:  
           removeClause(cr);
   }
   learnts.clear(true);
   checkGarbage();
}  

lbool Solver :: inprocess()
{
    lbool status = s_lift ();
    if(status != l_Undef) return status;
    
    status = s_probe ();
    return status;
}
  
lbool Solver :: s_eliminate ()
{   int loop;
    if( nVars() < 20 || nVars() > 500000) return l_Undef;
    mark = (char * ) calloc (2*nVars()+1, sizeof(char));
 
    cancelUntil(0);
    markNo=0;
    pos_df=neg_df=0;
    vscore.clear(true);

    lbool ret = l_Undef;
    
    fullwatch = new vec<CRef>[2*nVars()];
    buildfullwatch ();
              
    ret = s_hte ();
    if( ret != l_Undef )  goto DONE;
 
    s_eschedall();
    s_block ();
//  
    s_eschedall ();
    loop = sched_heap.size();
    if( loop > 20000) loop =20000;
    markNo = (int * ) calloc (nVars()+1, sizeof(int));
    pos_df = (DF *)malloc (1000*sizeof(DF));
    neg_df = (DF *)malloc (1000*sizeof(DF));
   
    while ( --loop > 0){
        if (sched_heap.empty()) goto DONE;
        int v = sched_heap.removeMin();
        if(vscore[v]>160) continue;
        if (assigns[v] == l_Undef && decision[v]){
             ret = s_elimlit (v);
             if( ret != l_Undef ) goto DONE;
        }
   }

DONE:
  if (dfpr) free(dfpr);
  if(markNo) free(markNo);
  if(pos_df){
       free(pos_df);
       free(neg_df);
  }
  sched_heap.clear(true);
  vscore.clear(true);
  delete [] fullwatch;
  free(mark);
  cleanClauses(clauses);
  cleanNonDecision();
  return ret;
}

int Solver :: s_blockcls (int ulit) 
{
   vec<CRef>&  crs  = fullwatch[ulit];
   int occNum = fullwatch[ulit].size();
   for(int i = 0; i < occNum; i++){
        CRef cr=crs[i];
        Clause& c = ca[cr];
        int j, size =0;
        for(j = 0; j < c.size(); j++){
               if(mark[toInt(c[j])^1]) break;
               if (++size > 500) return 0;
        }
        if(j >= c.size()) return 0;
   }
   return 1;
}  

int Solver :: s_blocklit (int pivot) 
{
   int occNum = fullwatch[pivot].size();
   if(occNum == 0 || occNum >1500) return 0;

   vec<int> scan;
   vec<CRef> blockedcls;
   vec<CRef> &  crs  = fullwatch[pivot];
   Lit lit=toLit(pivot);
   for(int i = 0; i < occNum; i++){
        CRef cr=crs[i];
        Clause& c = ca[cr];
        int blocked = 0;
        scan.clear();
        int size =0;
        for(int j = 0; j < c.size(); j++){
               if (c[j] == lit) continue;
               if(fullwatch[toInt(c[j])].size() >1000) goto CONTINUE;
               if (++size > 1500) goto CONTINUE;
               int ul=toInt(c[j]);
                scan.push(ul);
	        mark[ul]=1;
        }
        blocked = s_blockcls (pivot^1);
CONTINUE:
        for(int k = 0; k < scan.size(); k++) mark[scan[k]]=0;
        if (blocked==0) continue;
       blockedcls.push(cr);
  }   
  int count = blockedcls.size();
  for(int i=0; i < count; i++){
        CRef cr=blockedcls[i];
        removefullclause(cr);
        Clause & c = ca[cr];
        extendRemLit.push (lit_Undef);
        extendRemLit.push (lit);
        for(int j = 0; j < c.size(); j++){
               if (c[j] == lit) continue;
               extendRemLit.push(c[j]);
        }
     }
  return count;
}

void Solver :: s_block () 
{
  int count = 0;
  int loop = sched_heap.size();
  if( loop > 20000) loop =20000;
  while ( --loop > 0 && count <10000){
        if (sched_heap.empty()) return;
        int v = sched_heap.removeMin();
        if ( assigns [v] != l_Undef ) continue;

      //  printPivotClause (2*v);
      //  printPivotClause (2*v+1);
  
        count += s_blocklit (2*v);
        count += s_blocklit (2*v+1);
  }
}

// hidden literal addition 
lbool Solver :: s_hla (Lit start,  vec <Lit> & scan) 
{
  scan.clear();
  scan.push(start);
  mark[toInt(start)]=1;
  //starting hidden literal addition start
  for(int next=0; next < scan.size() && next<1000; next++) {
          Lit lit = scan[next];
	  vec<Watcher>&  bin  = watchesBin[lit];
          for(int k = 0;k<bin.size();k++) {
                 Lit other = bin[k].blocker;
                 if( value(other) != l_Undef) continue;
                 int uo=toInt(other);
                 if ( mark[uo] ) continue;
                 if ( mark[uo^1]) {
                  	// failed literal %d in hidden tautology elimination, start
                      if(checkUnit (toInt(start)^1, 'h')){
                          CRef confl = unitPropagation(~start);
                          if (confl != CRef_Undef) return l_False;
                          return l_True;
                       }
                 }
                 mark[uo]=1;
                 scan.push(other);
          }
  }
  return l_Undef;
}

//Hidden Tautology Elimination
//hidden literal addition
lbool Solver :: s_hte () //need remove learnt clauses
{ 
  unsigned int nlits = 2*nVars();
  if (nlits <= 40) return l_Undef;
 
  unsigned pos, delta; 
  find_relative_prime(pos, delta, nlits);
  lbool ret;
  vec <Lit> scan;
  vec <Lit> newCs;
  int limit= nlits/2;
  if(limit>10000) limit=10000; 
  vec <CRef> bigCr;
  int lsize;
  for(; limit>0; limit--) {
       Lit root = toLit(pos);
       if ( value(root) != l_Undef) goto CONTINUE;
       ret=s_hla(~root,scan); // root -> a^b^c.....
       if(ret != l_Undef) goto CONTINUE;
       lsize  = fullwatch[pos].size();
       bigCr.clear();
       for(int j = 0; j<lsize; j++) bigCr.push(fullwatch[pos][j]);
       for(int j = 0; j<bigCr.size(); j++){
              CRef cr=bigCr[j];
              Clause & c = ca[cr];
              if(c.mark()==1 || c.size()<3) continue;
              int taut=0;
              newCs.clear();
              newCs.push(root);
              for(int k=0; k<c.size(); k++){
                    if( c[k] == root) continue;
                    int ulit=toInt(c[k]);
                    if(mark[ulit]){
                         taut=1;
                         break;
                    }
                    if(mark[ulit^1]==0) newCs.push(c[k]);
              }
              if(taut==0){
                   if(newCs.size()==1){
                        if(checkUnit (toInt(newCs[0]), 'u')){
                            CRef confl = unitPropagation(newCs[0]);
                            if (confl != CRef_Undef) ret=l_False;
                         }
                         goto CONTINUE;
                   }
                   if(newCs.size() < c.size()) {
                        s_addfullcls (newCs, c.learnt());
                    }
              }
              if( newCs.size() < c.size()) {
                   removefullclause(cr);
              }
      }
CONTINUE:
       for(int j = 0; j<scan.size(); j++) mark[toInt(scan[j])]=0;
       if(ret == l_False ) return l_False;
       pos = (pos+delta) % nlits;
  }
  return l_Undef;
}

void  Solver :: s_extend ()
{
  printf("c elimination extend.sz=%d \n",extendRemLit.size());
  Lit lit;
  if (equhead){
       equlinkmem();
       for(int i=0; i<extendRemLit.size(); i++){
          lit = extendRemLit[i];
          if (lit==lit_Undef) continue;
          int cv=var(lit)+1;
          if(equhead[cv] && equhead[cv]!=cv){
                int elit =  equhead[cv];
                Lit eqLit = makeLit(elit);
                if(sign(lit)) extendRemLit[i]= ~eqLit;
                else  extendRemLit[i]= eqLit;
                if (value (eqLit) == l_Undef ) assigns[var(eqLit)] = lbool(!sign(eqLit));
          }
      }
  }

  Lit next = lit_Undef;
  while (extendRemLit.size()) {
     int satisfied = 0;
     do {
          lit = next;
          next = extendRemLit.pop_();
          if (lit==lit_Undef || satisfied) continue;
          if ( value (lit) == l_True ) satisfied = 1;
     } while (next != lit_Undef);
     if (satisfied) continue;
     int cv=var(lit);
     assigns[cv] = lbool(!sign(lit));
  }
}

//-------------------------------------------------------------------------
/*
// XOR   gausss solve
#define  RMSHFT         4
#define NOTALIT		((INT_MAX >> RMSHFT))

void get_XOR_equivalent(PPS *pps);

int Solver ::gaussSolve()
{
	int nV=nVars();
        if(g_pps->unit==0) g_pps->unit=(int *) calloc (nV+1, sizeof (int));
        g_pps->numVar=nV;
        OutputClause(g_pps->clause, g_pps->unit, g_pps->numClause,0);

        get_XOR_equivalent(g_pps);
        bool rc=gaussFirst();
        if(rc) return _UNKNOWN;
 	return UNSAT;
}
bool parity (unsigned x);
bool Solver :: addxorclause (int *xorlit, int xorlen, int & exVar)
{
 	int xbuf[10];
	int size;
        vec<Lit> lits;
      
        if(xorlen > 4){
            int n=xorlen/2-1;
            while(n) {n--; newVar();}
        }    
	
        for(int pos=0; pos<xorlen; pos+=size){
	     if(xorlen<=4) size=xorlen;
	     else{
		     if(pos+3>=xorlen) size=xorlen-pos;
		     else size=2;
	      }
	      int xs;
	      for(xs=0;xs<size; xs++) xbuf[xs]=xorlit[pos+xs];
	      if(pos) xbuf[xs++]=exVar++;
              if(size!=xorlen-pos) xbuf[xs++]=-exVar;
	      unsigned pow2=1<<xs;
      	      unsigned int j;
	      for(j=0; j<pow2; j++){ // XOR -> CNF
		    if(parity(j)) continue;
           	    unsigned bits=j;
                    lits.clear();
       		    for(int k=0; k<xs; k++) {
			     int lit=xbuf[k];
			     if(bits & 1) lit=-lit;
			     bits=bits>>1;
                             lits.push( (lit > 0) ? mkLit(lit-1) : ~mkLit(-lit-1));
	  	   }
        	   bool ret=addClause(lits); //close a clause
	           if(ret==false) return ret;
	      }
	}
	return true;
}

int Solver :: reload_clause(Stack<int> * & xclause)
{ 
    cleanAllclause();
    bool ret;
    vec<Lit> lits;
    int *pcls=xclause->begin();
    int *pend=xclause->end();
    inVars=nVars();
    int exVar=inVars+1;
    int CNFsum=0;
    int XOR3=0;
    int XOR4=0;
    int XOR5=0;
    while(pcls < pend){
          int len=*pcls;
	  int cmark=len & MARKBIT;
	  len=len>>FLAGBIT;
	  int *litp=pcls+1;
          pcls+=len;
	  if(cmark==DELETED) continue;
	  if(cmark==CNF_CLS){
		   lits.clear();
		   for (; litp<pcls; litp++){
	                int lit=*litp;
                        lits.push( (lit > 0) ? mkLit(lit-1) : ~mkLit(-lit-1));
		   }
      		   ret=addClause(lits); //close a clause
                   CNFsum++;
	           if(ret==false) return UNSAT;
           }
	   if(cmark==XOR_CLS){
                   if(len==4) XOR3++;
                   if(len==5) XOR4++;
                   if(len > 5) XOR5++;
                   ret=addxorclause (litp, len-1, exVar);
	           if(ret==false) return UNSAT;
           }
    }
    delete xclause;
    xclause=0;
    return _UNKNOWN;
}

void Solver :: gaussconnect () 
{
    int i,var, vars=0,connected = 0;
    int xsz=gauss_xors.size();
    for (int c =0; c < xsz; c=i+1){
        for (i = c; (var = gauss_xors[i]) > 1; i++) {
            if(gauss_occs[var].size()==0) vars++;
            gauss_occs[var].push(c);
        }
        connected++;
    }
}

inline void Solver :: gaussdisconnect () 
{
   for (int i =0; i <= xorAuxVar; i++) gauss_occs[i].clear();
 
}

//garbage collect
void  Solver :: gaussgc (int useIndex) //useIndex=0 
{
    int count = gauss_xors.size();
    if (gauss_garbage < count/2+1000) return;
    int p,q=0;
    vec <int> newszStk;
    if(useIndex==0){
        gaussdisconnect ();
        for (p=0; p < count; p++)
            if (gauss_xors[p] != NOTALIT) gauss_xors[q++] = gauss_xors[p];
    }
    else{
        int pos=0,size=0;
        vec <int> & XORidx = gauss_occs[1];
        for (int px=0; px < XORidx.size(); px++) 
        {    pos += size;
             if(pos>=gauss_xors.size()) break;
             size= ABS(XORidx[px]);
             if( gauss_xors[pos] >= NOTALIT) continue;
             int newsize=1; 
             for (p = pos; gauss_xors[p] > 1; p++) {gauss_xors[q++] = gauss_xors[p]; newsize++;}
             gauss_xors[q++] = gauss_xors[p];
             if(XORidx[px] < 0) newsize = -newsize;
             newszStk.push(newsize);
         }
         gaussdisconnect ();
   }
   gauss_xors.shrink(count - q);
   gaussconnect ();
   if(useIndex){//copy size
        gauss_occs[1].clear();
        for (int psz=0; psz < newszStk.size(); psz++) gauss_occs[1].push(newszStk[psz]);
   }
   gauss_garbage = 0;
}

void Solver :: gaussdiseqn (int eqn) {
  int i,var;
  for (i = eqn; (var = gauss_xors[i]) > 1; i++) {
    gauss_xors[i] = NOTALIT;
    gauss_garbage++;
    remove(gauss_occs[var], eqn);
  }
  gauss_xors[i] = NOTALIT;
  gauss_garbage++;
}

int Solver :: gausspickeqn (int pivot) 
{
  int tmp, other, e, q;
  int res = -1;
  int weight = INT_MAX; 
  int size = INT_MAX;
  vec <int> & occs = gauss_occs[pivot];

  for (int p = 0; p < occs.size(); p++) {
      e = occs[p];
      tmp = 0;
      for (q = e; (other = gauss_xors[q]) > 1; q++) {
           if (gauss_eliminated[other]) break;
           if (other == pivot) continue;
           tmp += gauss_occs[other].size() - 1;
      }
      if (other > 1) continue;
      if (res >= 0){
            if(q - e > size) continue;
            if(q - e == size && tmp >= weight) continue;
      }
      weight = tmp;
      size = q - e;
      res = e;
  }
  return res;
}

int Solver :: gaussaddeqn (int eqn) 
{
  int var;
  for (int p = eqn; (var = gauss_xors[p]) > 1; p++){
      if (!xmark[var]) xclause.push(var);
      xmark[var]= !xmark[var];
  }
  return var;
}

void Solver :: gaussconeqn (int eqn) 
{
  int i, var;
  for (i = eqn; (var = gauss_xors[i]) > 1; i++) gauss_occs[var].push(eqn);
}

void Solver :: popnunmarkstk () 
{
    for(int i=xclause.size()-1; i>=0; i--)  xmark[xclause[i]]=0;
    xclause.clear();
}

inline int getCNF_occs(PPS *pps, int var)
{
   int eidx=var-1;
   return pps->occCls[eidx].nowpos()+pps->occCls[-eidx].nowpos();
}

void Solver :: gaussNewClause (int storeSize, int newsz, int rhs) 
{  vec <int> xorc[2];
   for(int i=0; i<newsz; i++){
         int var=xclause[i];
         if(newsz>4 && xorc[1].size()<3){
               if(getCNF_occs(g_pps,var)) {
                      xorc[1].push(var);
                      continue;
               } 
         }
         xorc[0].push(var);
  }
  if(newsz>4){
       while(xorc[1].size()<2){
              xorc[1].push(xorc[0].pop_());
       }
       xorAuxVar++;
       xorc[1].push(xorAuxVar);
       xorc[0].push(xorAuxVar);
       newVar();
       xmark.push(0);
       gauss_eliminated.push(0);
       gauss_occs.growTo(xorAuxVar+1);
  }
  for(int k=0; k<2; k++){
       if(xorc[k].size()==0) break;
       int xend = gauss_xors.size();
       for(int i=0; i<xorc[k].size(); i++) gauss_xors.push(xorc[k][i]);
       gauss_xors.push(rhs);
       rhs=0;
       gaussconeqn (xend);//add index
       if(storeSize){
             int curend=gauss_xors.size();
             gauss_occs[1].push(curend-xend);
       }
  }
}

bool Solver :: gaussubst (int pivot, int subst, int storeSize) 
{
  int eqn, rhs,p,q;
  bool ret=true;
  while (gauss_occs[pivot].size() > 1 && ret) {
       eqn = gauss_occs[pivot][0];
       if (eqn == subst) eqn = gauss_occs[pivot][1];
       rhs = gaussaddeqn (eqn);
       if (gaussaddeqn (subst)) rhs = !rhs;
       for (p = q=0; p < xclause.size(); p++)
           if (xmark[xclause[p]]) xclause[q++]=xclause[p];
       gaussdiseqn (eqn);//remove equ
       if (q) {

            int res = gauss_xors.size();
            for(int i=0; i<q; i++) gauss_xors.push(xclause[i]);
            gauss_xors.push(rhs);
            gaussconeqn (res);//add index
            if(storeSize){//new idea, see lglgaussXORsize
                 int end=gauss_xors.size();
                 gauss_occs[1].push(end-res);//size 
            }
    } else if (rhs == 0) {
      printf("c trivial result row 0 = 0\n");
    } else {
      printf("c inconsistent result row 0 = 1 from gaussian elimination \n");
      ret= false;//UNSAT
    }
    popnunmarkstk();//clear xmark
  }
  return ret;
}

void set_CNF_occ(PPS *pps);

bool Solver :: gaussInactive()
{
  set_CNF_occ(g_pps);
  int occNo;
  int inactives=0;
  for(int pivot=2; pivot <= originVars; pivot++){
         if(getCNF_occs(g_pps,pivot)>0) continue;
         occNo = gauss_occs[pivot].size();
         if (occNo >= 2 ) inactives++;
  }
  if(inactives < 10) return true;
  for(int pivot=2; pivot <= originVars; pivot++){
         if(getCNF_occs(g_pps,pivot)>0) continue;
         occNo = gauss_occs[pivot].size();
         if (occNo < 2 ) continue;
         gaussgc ();
         int subst = gausspickeqn (pivot);
         if(subst < 0) continue;
         bool ret=gaussubst (pivot, subst);
         gauss_eliminated[pivot] = 1;
         if(!ret) return false;//UNSAT
   }
  
   g_pps->trigonXORequ = new Stack<int>;
   for(int pivot=originVars; pivot > 1; pivot--){
         if(getCNF_occs(g_pps,pivot)>0) continue;
         occNo = gauss_occs[pivot].size();
         if(occNo==0) continue;
         if (occNo > 1 ){
              continue;
         }
         vec <int> & occs = gauss_occs[pivot];
         int var,eqn = occs[0];
         for (int p = eqn; (var = gauss_xors[p]) > 1; p++) 
                 if(pivot!=var) g_pps->trigonXORequ->push(var);              
         g_pps->trigonXORequ->push(pivot);
         g_pps->trigonXORequ->push(var);
         gaussdiseqn (eqn);//remove equ
         solvedXOR++;
   }
   return true;
}

bool Solver :: gauss_shorten(int pivot, int subst) 
{
  vec <int> shareLit;
  int eqn, rhs, res=-1;
  int p,q;
  for (int px = 0; px < gauss_occs[pivot].size(); px++){
      eqn = gauss_occs[pivot][px];
      if (eqn == subst) continue;
      rhs = gaussaddeqn (eqn);
      int oldsize= xclause.size();
      if (gaussaddeqn (subst)) rhs = !rhs;
      shareLit.clear();
      for (p = q =0; p < xclause.size(); p++)
          if (xmark[xclause[p]]) xclause[q++] = xclause[p];
          else shareLit.push(xclause[p]);

      if( q < oldsize){//==
          gaussdiseqn (eqn);//remove equ
          if (q) {
            res = gauss_xors.size();
            for(int i=0; i<q; i++) gauss_xors.push(xclause[i]);
            gauss_xors.push(rhs);
            gaussconeqn (res);//add index
            int end=gauss_xors.size();
            gauss_occs[1].push(end-res);//size 
          } else if (rhs == 0) {
                   printf("c trivial result row 0 = 0\n");
          } else {//rhs == 1;
                printf("c inconsistent result row 0 = 1 from gaussian elimination\n");
               return false;//UNSAT
          }
      }
      else px++;
      popnunmarkstk ();
   }
   return true;
}

bool Solver :: gaussMinimize ()
{
  gaussgc ();
  gaussXORsize();
  vec <int> & XORsize= gauss_occs[1];
  int equ=0;
  int size=0;
  int cnt=XORsize.size();
  for (int i=0 ; i < cnt; i++) 
  {  
     equ += ABS(size); 
     size=XORsize[i];
     if( gauss_xors[equ] >= NOTALIT || size < 0) continue;
     int k=equ;
     while(k<equ+1000){
          int var = gauss_xors[k];
          if(var < 2) break;
          k++;
          if(!gauss_shorten(var, equ)) return false;
     }
  }
  return true;
}

bool Solver :: gaussFirst () 
{
  g_pps->garbage=0;
  gauss_garbage=0;
  xorAuxVar=nVars()+1;
  gauss_occs.growTo(xorAuxVar+1);
  xmark.growTo(xorAuxVar+1, 0);

  gaussloadxor();

  if (gauss_xors.size()) {
       gaussconnect ();
       gauss_eliminated.growTo(nVars()+3, 0);
       if(!gaussInactive()) return false; //UNSAT;
       if(solvedXOR){
           if(!gaussMinimize()) return false;
       }
       gaussdisconnect ();
       exportXOR();
  }
  xmark.clear(true);
  gauss_eliminated.clear(true);
  gauss_xors.clear(true);
  gauss_occs.clear(true);
  return true;
}

void Solver :: exportXOR() 
{
  int p;
  Stack<int> *clsSAT = g_pps->clause;
  for(p = 0; p < gauss_xors.size(); p++) {
       if (gauss_xors[p] >= NOTALIT) continue;
       int pos=clsSAT->nowpos();
       clsSAT->push(1);
       int size=1; 
       while (gauss_xors[p] > 1) {
            int var=gauss_xors[p++];
            clsSAT->push(var-1);
            size++;
       }
       if(size < 2) {
          clsSAT->pop();
          continue;
       } 
       (*clsSAT)[pos]=(size<<FLAGBIT) | XOR_CLS;
       if(gauss_xors[p]==0){
           int var=clsSAT->pop();
           clsSAT->push(-var); 
        }
  }
}

void Solver :: gaussloadxor()
{
    int *pcls=g_pps->clause->begin();
    int *pend=g_pps->clause->end();
    pcls=g_pps->clause->begin();
    while(pcls < pend){
         int len=*pcls;
	 int cMark=len & MARKBIT;
	 int *lits=pcls;
	 len=len>>FLAGBIT;
         pcls+=len;
         if(cMark==DELETED) continue;
         if(cMark==XOR_CLS){
               int rhs=1;//right hand side
               for(int k=1; k<len; k++){
                     int lit=lits[k];
                     if(lit<0){
                          lit=-lit;
                          rhs=!rhs;
                     }     
                     gauss_xors.push(lit+1);
               }
               gauss_xors.push(rhs);
               lits[0]=(len<<FLAGBIT) | DELETED;
         }
     }
}

void Solver :: gaussXORsize () 
{
  int c, i, eox = gauss_xors.size();
  gauss_occs[1].clear();
  for (c = 0; c < eox; c = i) {
    i = c;
    if(gauss_xors[c] >= NOTALIT){
         while(1){
              i++;
              if(i >= eox) break;
              if(gauss_xors[i] < NOTALIT) break;
         }
    }
    else{
         for (; gauss_xors[i]> 1; i++);
         i++;
    }
    gauss_occs[1].push(i-c);//size
  }
}

int Solver :: gaussBestEquNo () 
{
  int res, weight, minsize, tmp;
  int p,q,indexp=0;
  
  vec <int> & XORsize= gauss_occs[1];
  res = -1;
  weight = minsize= INT_MAX;
  int var, pos=0, size=0;
  for (p = 0; p < XORsize.size(); p++) 
  {
     pos += size; 
     size= ABS(XORsize[p]);
     if( gauss_xors[pos] >= NOTALIT || XORsize[p] < 0) continue;
     tmp = 0;
     for (q = pos; (var=gauss_xors[q]) > 1; q++) tmp += gauss_occs[var].size();
     if (res >= 0){
           if(size > minsize) continue;
           if ( size == minsize && tmp >= weight) continue;
    }
    weight = tmp;
    minsize = size;
    res = pos;
    indexp=p;
  }
  if(res>=0) XORsize[indexp] = - XORsize[indexp];//already processed
  return res;
}

int Solver :: gaussMaxSubst (int pivot, int subst) 
{
  vec <int> & occs = gauss_occs[pivot];
  int size=0, eqn;
  for (int px = 0; px < occs.size(); px++){
      eqn = occs[px];
      if (eqn == subst) continue;
      gaussaddeqn (eqn);
      gaussaddeqn (subst);
      size=0;
      for (int p = 0; p < xclause.size(); p++) if (xmark[xclause[p]]) size++;
      popnunmarkstk ();
      if(size > 6) return size;
  }
  return size;
}

void gaussSameReplace(PPS *pps,int minVar, int maxVar);
void gaussEqvReplace(PPS *pps, int minLit, int maxLit);
void CNFgarbageCollection(PPS *pps);

bool Solver :: gaussSimplify ()
{
  gaussXORsize();
  int p, q,pivot,occs,var,min,max;
  int ret=true;
  while (ret) {
      CNFgarbageCollection(g_pps);
      gaussgc (1);//useIndex=1
      int equ=gaussBestEquNo ();
      if(equ < 0) break;
nextVar:  
      pivot  = gauss_xors[equ];
      min = getCNF_occs(g_pps,pivot);
      int szpow2 =1;
      for (p =equ+1; (var=gauss_xors[p]) > 1; p++){
          if(szpow2 <= 4096) szpow2 *= 2;
          occs=getCNF_occs(g_pps,var);
          if(min > occs) {
                 min = occs;
                 pivot = var;
          }
      }
      int size = gaussMaxSubst(pivot, equ); 
      if(size > 6) continue;
      ret=gaussubst (pivot, equ,1);//storeSize=0
      if(szpow2 == 2){ //a(+)b=1 or 0
           int maxLit;
           if(pivot == gauss_xors[equ]) maxLit = gauss_xors[equ+1];
           else  maxLit = gauss_xors[equ];
           if(gauss_xors[equ+2]==1) maxLit = -maxLit;
           gaussEqvReplace(g_pps, pivot, maxLit);//pivot=maxLit
           gaussdiseqn (equ);//remove equ
           continue;
      }
      if(szpow2 > 4){
           if(szpow2 < min || min > 8) continue;
      }
      else if(min > 8 || szpow2 <=2) continue;
  
      int mVar = -1;
      max=-1;
      for (p = equ; (var=gauss_xors[p]) > 1; p++){
              if(var == pivot) continue;
              occs=getCNF_occs(g_pps, var);
              if(max < occs) { max = occs; mVar = var;}
      }
      if (mVar < 1) continue;
      gaussSameReplace(g_pps,pivot, mVar); // x <=> pivot (+) mVar  and let x=pivot;
      
      for (q=p=equ; (var=gauss_xors[p]) > 1; p++) if(var != mVar){ gauss_xors[q++]=var;}
      gauss_xors[q]=var;
      gauss_xors[q+1]=NOTALIT;
 
      remove(gauss_occs[mVar], equ);
      goto  nextVar;
  }
  printf("c gaussSimplify done \n");
  return ret;
}
*/
//--------------------------------------
// Try further learnt clause minimization by means of binary clause resolution.
bool Solver::binResMinimize(vec<Lit>& out_learnt)
{
    // Preparation: remember which false variables we have in 'out_learnt'.
    MYFLAG++;
    for (int i = 1; i < out_learnt.size(); i++)
        permDiff[var(out_learnt[i])] = MYFLAG;

    // Get the list of binary clauses containing 'out_learnt[0]'.
    const vec<Watcher>& ws = watchesBin[~out_learnt[0]];

    int to_remove = 0;
    for (int i = 0; i < ws.size(); i++){
        Lit the_other = ws[i].blocker;
        // Does 'the_other' appear negatively in 'out_learnt'?
        if (permDiff[var(the_other)] == MYFLAG && value(the_other) == l_True){
            to_remove++;
            permDiff[var(the_other)] = MYFLAG - 1; // Remember to remove this variable.
        }
    }

    // Shrink.
    if (to_remove > 0){
        int last = out_learnt.size() - 1;
        for (int i = 1; i < out_learnt.size() - to_remove; i++)
            if (permDiff[var(out_learnt[i])] != MYFLAG)
                out_learnt[i--] = out_learnt[last--];
        out_learnt.shrink(to_remove);
    }
    return to_remove != 0;
}

