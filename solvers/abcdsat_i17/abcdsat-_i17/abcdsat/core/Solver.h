/***************************************************************************************[Solver.h]
abcdSAT RAT -- Copyright (c) 2017, Jingchao Chen, Donghua University   

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

#ifndef abcdSAT_Solver_h
#define abcdSAT_Solver_h

#include "abcdsat/mtl/Vec.h"
#include "abcdsat/mtl/Heap.h"
#include "abcdsat/mtl/Alg.h"
#include "abcdsat/utils/Options.h"
#include "abcdsat/core/SolverTypes.h"
#include "abcdsat/core/BoundedQueue.h"
#include "abcdsat/core/Constants.h"
#include "abcdsat/core/Stack_PPS.h"

#define FUNVAR	     11
#define FUNQUADS     32

typedef int64_t Cnf;
typedef uint64_t Fun[FUNQUADS];

namespace abcdSAT {

//=================================================================================================
// Solver -- the main class:

class Solver {
public:

    void* termCallbackState;
    int (*termCallback)(void* state);
    void setTermCallback(void* state, int (*termCallback)(void*)) {
	this->termCallbackState = state;
	this->termCallback = termCallback;
    }

    void* learnCallbackState;
    int learnCallbackLimit;
    void (*learnCallback)(void * state, int * clause);
  //  int learnCallbackBuffer[10000];
    void setLearnCallback(void * state, int maxLength, void (*learn)(void * state, int * clause)) {
        this->learnCallbackState = state;
        this->learnCallbackLimit = maxLength;
        this->learnCallback = learn;
    }
    int preprocessflag;
    lbool addassume();
    int equMemSize;
    void equlinkmem();
    int incremental;
    void buildEquAssume();
   
    // Constructor/Destructor:
    //
    Solver();
    virtual ~Solver();

    // Problem specification:
    //
    Var     newVar    (bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.

    bool    addClause (const vec<Lit>& ps);                     // Add a clause to the solver. 
    bool    addEmptyClause();                                   // Add the empty clause, making the solver contradictory.
    bool    addClause (Lit p);                                  // Add a unit clause to the solver. 
    bool    addClause (Lit p, Lit q);                           // Add a binary clause to the solver. 
    bool    addClause (Lit p, Lit q, Lit r);                    // Add a ternary clause to the solver. 
    bool    addClause_(vec<Lit>& ps);                           // Add a clause to the solver without making superflous internal copy. Will
                                                                // change the passed vector 'ps'.

    // Solving:
    //
    bool    simplify     ();                        // Removes already satisfied clauses.
    bool    solve        (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    lbool   solveLimited  (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    lbool   solveLimited2 (const vec<Lit>& assumps); 
    bool    solve        ();                        // Search without assumptions.
    bool    solve        (Lit p);                   // Search for a model that respects a single assumption.
    bool    solve        (Lit p, Lit q);            // Search for a model that respects two assumptions.
    bool    solve        (Lit p, Lit q, Lit r);     // Search for a model that respects three assumptions.
    bool    okay         () const;                  // FALSE means solver is in a conflicting state
    lbool   solveNoAssump();

    void    toDimacs     (FILE* f, const vec<Lit>& assumps);            // Write CNF to file in DIMACS-format.
    void    toDimacs     (const char *file, const vec<Lit>& assumps);
    void    toDimacs     (FILE* f, Clause& c, vec<Var>& map, Var& max);
    void    printLit(Lit l);
    void    printClause(CRef c);
    // Convenience versions of 'toDimacs()':
    void    toDimacs     (const char* file);
    void    toDimacs     (const char* file, Lit p);
    void    toDimacs     (const char* file, Lit p, Lit q);
    void    toDimacs     (const char* file, Lit p, Lit q, Lit r);
    
    // Variable mode:
    // 
    void    setPolarity    (Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void    setDecisionVar (Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    lbool   value      (Var x) const;       // The current value of a variable.
    lbool   value      (Lit p) const;       // The current value of a literal.
    lbool   modelValue (Var x) const;       // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool   modelValue (Lit p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
    int     nAssigns   ()      const;       // The current number of assigned literals.
    int     nClauses   ()      const;       // The current number of original clauses.
    int     nLearnts   ()      const;       // The current number of learnt clauses.
    int     nVars      ()      const;       // The current number of variables.
    int     nFreeVars  ()      const;
    int     nUnfixedVars()     const;
    // Resource contraints:
    //
    void    setConfBudget(int64_t x);
    void    setPropBudget(int64_t x);
    void    budgetOff();
    void    interrupt();          // Trigger a (potentially asynchronous) interruption of the solver.
    void    clearInterrupt();     // Clear interrupt indicator flag.

    // Memory managment:
    //
    virtual void garbageCollect();
    void    checkGarbage(double gf);
    void    checkGarbage();

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          // If problem is unsatisfiable (possibly under assumptions),
                                  // this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    //
    int       verbosity;
    int       verbEveryConflicts;
    // Constants For restarts
    double    K;
    double    R;
    double    sizeLBDQueue;
    double    sizeTrailQueue;

    // Constants for reduce DB
    int firstReduceDB;
    int incReduceDB;
    int specialIncReduceDB;
    unsigned int lbLBDFrozenClause;

    // Constant for reducing clause
    int lbSizeMinimizingClause;
    unsigned int lbLBDMinimizingClause;

    double    var_decay;
    double    clause_decay;
    double    random_var_freq;
    double    random_seed;
    int       ccmin_mode;         // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
    int       phase_saving;       // Controls the level of phase saving (0=none, 1=limited, 2=full).
    bool      rnd_pol;            // Use random polarities for branching heuristics.
    bool      rnd_init_act;       // Initialize variable activities with a small random value.
    double    garbage_frac;       // The fraction of wasted memory allowed before a garbage collection is triggered.

    // Statistics: (read-only member variable)
    //
    uint64_t nbRemovedClauses,nbReducedClauses,nbDL2,nbBin,nbUn,nbReduceDB,solves, starts, decisions, rnd_decisions, propagations, conflicts,nbstopsrestarts;
    uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;

  int bitVecNo,bitconLimit; //new idea
  int simp_solve();
  int findmaxVar(int & Mvar);
  int IterativeUnitPropagation(int lit0, int * Queue, float *diff,int *unit,int *pre_unit);
  int nFreeV;
  int maxDepth;
  void find_k_largest(vec<Var> & vn, int s, int e, int k);

  int inVars;
  void fast_bcd();
  void  varOrderbyClause();
  int probe_unsat;

  bool hasBin(Lit p, Lit q);
  void setmark(vec<int> & seenlit);
  void clearmark(vec<int> & seenlit);
  void addbinCls(vec<int> & ilit, Lit p);

  lbool probe();
  lbool probeaux();
  bool  addbinclause(Lit p, Lit q);
  void  addbinclause2(Lit p, Lit q, CRef & cr, int learntflag);
  bool out_binclause(Lit p, Lit q);

  lbool addequ(Lit p, Lit q);
  lbool add_equ_link(int plit, int qlit);
  lbool simpleprobehbr (Clause & c);
  int  *count;
  int  *litsum;
  int  *equhead;
  int  *equlink;
  char  *dummyVar;
  char  *mark;
  char  *touchMark;
  vec<int>  touchVar ;
  lbool removeEqVar(vec<CRef>& cls, bool learntflag);
  void  solveEqu(int *equRepr, int equVars, vec<lbool> & solution);
  void  buildEquClause();

  bool is_conflict(vec<Lit> & lits);
 
  int hbrsum, equsum,unitsum,probeSum;
//--------------------------------
//debug
  int findclause();
  vec <Lit > orgclauses;
  void copyclause();
  void verifyOrg();
  bool satisfied(vec <Lit> & c);
  void printextendRemLit();
  void printEqu(int *equRepr);
  void printPivotClause (int ulit);
  void replaceClause (int ulit);
  void printXOR(int eqn);
  void printXclause();

  void addlearnt();

  lbool replaceEqVar();
  lbool tarjan_SCC();
  
  lbool distill();
  lbool distillaux(vec<CRef>& cls,int start);
  void distill_analyze(vec<Lit> & out_clause, Lit p);

  lbool transitive_reduction ();
  lbool trdlit (Lit start, int & remflag);
  int   trdbin (Lit start, Lit target, int learntflag);
  void  cleanClauses(vec<CRef>& cls);

  int unhdhasbins (Lit lit, int irronly);
  int s_stamp (Lit root, vec <int> & units, int stamp, int irronly);
  void rmBinDup ();
  lbool s_stampall (int irr_only); 
  lbool unhide ();
  lbool s_unhidefailed (); 
  lbool s_unhidebintrn (vec<CRef>& cls,int irronly,int learntflag);
  lbool s_unhidelrg (vec<CRef>& cls,int irronly,int learntflag);
  int s_unhlca (int a, int b);
  int  checkUnit (int uLit, char printflag);
  void checkBin (int uL1, int uL2,char printflag);
  void checklarg(vec <Lit> & cs, char type);
  void check_all_impl();
  
  int *markNo;
  int neglidx;
  vec <CRef> deletedCls;
  vec <Lit> extendRemLit;
  Lit s_prbana(CRef confl, Lit res);
  int s_randomprobe (vec<int> & outer); 
  int s_innerprobe (int start, vec<int> & probes); 
  void  s_addliftbincls (Lit p, Lit q);
  lbool addeqvlist (vec <int> & equal);
  lbool s_liftaux ();
  lbool s_lift ();
  lbool s_probe ();
  void update_score(int ulit);
  void removefullclause(CRef cr);
  void s_addfullcls (vec<Lit> & ps, int learntflag);
  void buildfullwatch ();
  int s_chkoccs4elmlit (int ulit);
  int s_chkoccs4elm (int v); 
  int s_s2m (int ulit);
  int s_i2m (int ulit);
  int s_m2i (int mlit);
  int s_initsmallve (int ulit, Fun res); 
  int s_smallisunitcls (int cls);
  int s_smallcnfunits (Cnf cnf);
  void s_smallve (Cnf cnf); 
  lbool s_smallve (Cnf cnf, int pivotv, bool newaddflag);
  int s_uhte (vec <Lit> & resolv); 

  lbool s_trysmallve (int v, int & res);
  void s_epusheliminated (int v);
  lbool s_forcedve (int v);
  int s_forcedvechk (int v);
  int s_smallvccheck (int v);

  int s_backsub (int cIdx, int startidx, int lastIdx, Lit pivot, int streFlag);
  void  s_elmsub (int v);
  lbool s_elmstr (int v, int & res);
  void s_elmfrelit (Lit pivot, int startp, int endp, int startn, int endn); 
  void s_elmfre (Lit pivot);
  lbool s_elimlitaux (int v);
  lbool s_tryforcedve (int v, int & res);
  lbool s_trylargeve (int v, int & res);
  int  s_initecls (int v);
  int  s_ecls (int ulit);
  int  s_addecl (CRef cr, Lit pivot);
  lbool s_elimlit (int v);
  void s_eschedall(); 
  lbool s_eliminate ();
  lbool s_hte ();
  void  s_block (); 
  int s_blockcls (int ulit);
  int  s_blocklit (int ulit);
  lbool s_hla (Lit start,  vec <Lit> & scan);
  lbool check_add_equ (Lit p, Lit q);
  void  s_extend ();
  void  cleanNonDecision();
  void  clearMarkNO();
  bool  is_conflict(Lit p, Lit q);
  lbool inprocess();
//-------------------------------------------------------------
// XOR
  void cleanAllclause();
  int  gaussSolve();
  bool gaussFirst();
  vec <int>  gauss_xors;
  vec <int>  xclause;
  vec < vec <int> >  gauss_occs;
  vec <char>  gauss_eliminated;
  vec <char>  xmark;
  int  xorAuxVar,solvedXOR;
  int  gauss_garbage;

  void gaussloadxor();
  int reload_clause(Stack<int> * & xclause);
  bool addxorclause (int *xorlit, int xorlen, int & exVar);
  void gaussconnect ();
  void gaussdisconnect ();
  void gaussgc (int useIndex=0);
  bool gaussInactive();
  void gaussdiseqn (int eqn);
  int  gausspickeqn (int pivot);
  bool gaussubst (int pivot, int subst, int storeSize=0);
  int  gaussaddeqn (int eqn);
  void gaussconeqn (int eqn);
  void popnunmarkstk ();
  bool gaussSimplify ();
  void gaussXORsize ();
  int  gaussBestEquNo ();
  int  gaussMaxSubst (int pivot, int subst);
  bool gauss_shorten(int pivot, int subst);
  bool gaussMinimize ();
  void exportXOR();
  void gaussNewClause (int storeSize, int newsz, int rhs);
//
   bool binResMinimize(vec<Lit>& out_learnt);
   template<class V> int computeLBD(const V& c) {
        int lbd = 0;
        MYFLAG++;
        for (int i = 0; i < c.size(); i++){
            int l = level(var(c[i]));
            if (l != 0 && permDiff[l] != MYFLAG){
                permDiff[l] = MYFLAG;
                lbd++; } }
        return lbd;
    }
    template<class V> int compute0LBD(const V& c) {
        int lbd = 0;
        MYFLAG++;
        for (int i = 0; i < c.size(); i++){
            int l = level(var(c[i]));
            if (permDiff[l] != MYFLAG) {
	        permDiff[l] = MYFLAG;
                lbd++; } }
        return lbd;
    }

//
//--------------
  lbool run_subsolver(Solver* & newsolver, Lit plit, int conf_limit,int fullsolve);
  lbool subsolve();
  void  ClearClause(vec<CRef>& cs, int deleteflag);
  void  ClearUnit();
  CRef  unitPropagation(Lit p);

  int   Out_oneClause(Solver* solver,vec<CRef>& cs,Stack<int> * outClause, int lenLimit,int clause_type); 
  int   Output_All_Clause(Solver* solver);
  lbool dumpClause(vec<CRef>& cs,Solver* fromSolver,Solver* ToSolver,int sizelimit);
  lbool dumpClause(Solver* fromSolver, Solver* toSolver);
  void copyAssign(Solver* fromSolver);
  void simpAddClause(const vec<Lit>& lits);
  void reconstructClause(int var, Solver * & solver1, Solver * & solver2,int NumCls,int **clsPtr);
  CRef copyTwoAssign(int var, Solver * & solver1, Solver * & solver2);
  int conflict_limit;
  int completesolve;
  int recursiveDepth;
  void verify();
  int originVars;
  int subformuleClause;
  int needExtVar;

protected:
    long curRestart;
    // Helper structures:
    //
    struct VarData { CRef reason; int level; };
    static inline VarData mkVarData(CRef cr, int l){ VarData d = {cr, l}; return d; }

    struct Watcher {
        CRef cref;
        Lit  blocker;
        Watcher(CRef cr, Lit p) : cref(cr), blocker(p) {}
        bool operator==(const Watcher& w) const { return cref == w.cref; }
        bool operator!=(const Watcher& w) const { return cref != w.cref; }
    };

    struct WatcherDeleted
    {
        const ClauseAllocator& ca;
        WatcherDeleted(const ClauseAllocator& _ca) : ca(_ca) {}
        bool operator()(const Watcher& w) const { return ca[w.cref].mark() == 1; }
    };

    struct VarOrderLt {
        const vec<double>&  activity;
        bool operator () (Var x, Var y) const { return activity[x] > activity[y]; }
        VarOrderLt(const vec<double>&  act) : activity(act) { }
    };

    struct VarSchedLt {
        const vec<int>&  vscore;
        bool operator () (Var x, Var y) const { return vscore[x] < vscore[y];}
        VarSchedLt(const vec<int>&  act) : vscore(act) { }
    };


    // Solver state:
    //
    int lastIndexRed;
    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    double              cla_inc;          // Amount to bump next clause with.
    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    vec<int>            vscore;            // A heuristic measurement of the occ number of a variable.
    double              var_inc;          // Amount to bump next variable with.
    OccLists<Lit, vec<Watcher>, WatcherDeleted>
                        watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<CRef>           *fullwatch;
    OccLists<Lit, vec<Watcher>, WatcherDeleted>
                        watchesBin;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    public:
    vec<CRef>           clauses;          // List of problem clauses.
    vec<CRef>           learnts;          // List of learnt clauses.

    vec<lbool>          assigns;          // The current assignments.
    vec<lbool>          old_assigns;
   
    vec<char>           polarity;         // The preferred polarity of each variable.
    vec<char>           decision;         // Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
 
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<VarData>        vardata;          // Stores reason and level for each variable.
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
    int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.
    Heap<VarOrderLt>    order_heap;       // A priority queue of variables ordered with respect to the variable activity.
    Heap<VarSchedLt>    sched_heap;       // A priority schedule queue of variables with respect to score.
  
    bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
    vec<unsigned int> permDiff;      // permDiff[var] contains the current conflict number... Used to count the number of  LBD
    
#ifdef UPDATEVARACTIVITY
    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
    vec<Lit> lastDecisionLevel; 
#endif

    ClauseAllocator     ca;

    int nbclausesbeforereduce;            // To know when it is time to reduce clause database
    
    bqueue<unsigned int> trailQueue,lbdQueue; // Bounded queues for restarts.
    float sumLBD; // used to compute the global average of LBD. Restarts...


    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vec<char>           seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit>            add_tmp;
    unsigned int  MYFLAG;


    double              max_learnts;
    double              learntsize_adjust_confl;
    int                 learntsize_adjust_cnt;

    // Resource contraints:
    //
    int64_t             conflict_budget;    // -1 means no budget.
    int64_t             propagation_budget; // -1 means no budget.
    bool                asynch_interrupt;

//new idea
    int  oldVars;

    // Main internal methods:
    //
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    Lit      pickBranchLit    ();                                                      // Return the next decision variable.
    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     uncheckedEnqueue (Lit p, CRef from = CRef_Undef);                         // Enqueue a literal. Assumes value of literal is undefined.
    bool     enqueue          (Lit p, CRef from = CRef_Undef);                         // Test if fact 'p' contradicts current state, enqueue otherwise.
    CRef     propagate        ();                                                      // Perform unit propagation. Returns possibly conflicting clause.
    int BCD_conf;
    int BCD_delay;
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.
    void     analyze          (CRef confl, vec<Lit>& out_learnt, int& out_btlevel,unsigned int &nblevels);    // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool     litRedundant     (Lit p, uint32_t abstract_levels);                       // (helper method for 'analyze()')
    lbool    search           (int nof_conflicts);                                     // Search for a given number of conflicts.
    lbool    solve_           ();                                                      // Main solve method (assumptions given in 'assumptions').
   
    lbool   solve2_          (); //new idea 
   
    void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
    void     removeSatisfied  (vec<CRef>& cs, int delflag);                                         // Shrink 'cs' to contain only non-satisfied clauses.
    void     rebuildOrderHeap ();

//new idea
    PPS      *g_pps;
    void     extend_subsolution( Solver * sub_solver);
    void     Out_removeClause (vec<CRef>& cs,Stack<int> * & outClause, int & numCls,int lenLimit,int remove);
    void     OutputClause(Stack<int> * & clauseCNF, int *solution, int & numCls,int remove);
 
    CRef     trypropagate(Lit p);
    float    dynamicWeight();
    void     LitWeight(vec<CRef>& cs);
    void     XorMark();

    int     longClauses();
    int     longcls;

    // Maintaining Variable/Clause activity:
    //
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v, double inc);     // Increase a variable with the current 'bump' value.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     varBumpActivity2 (Var v){
                varBumpActivity(v, 0.5*var_inc);
             }
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
    void     claBumpActivity  (Clause& c);             // Increase a clause with the current 'bump' value.

    // Operations on clauses:
    //
    void     attachClause     (CRef cr);               // Attach a clause to watcher lists.
    void     detachClause     (CRef cr, bool strict = false); // Detach a clause to watcher lists.
    void     removeClause     (CRef cr);               // Detach and free a clause.
    bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    bool     satisfied        (const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

    void     relocAll         (ClauseAllocator& to);

    // Misc:
    //
    int      decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (Var x) const; // Used to represent an abstraction of sets of decision levels.
    CRef     reason           (Var x) const;
    int      level            (Var x) const;
    double   progressEstimate ()      const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
    bool     withinBudget     ()      const;

    // Static helpers:
    //

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double& seed) {
        seed *= 1389796;
        int q = (int)(seed / 2147483647);
        seed -= (double)q * 2147483647;
        return seed / 2147483647; }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double& seed, int size) {
        return (int)(drand(seed) * size); }
};


//=================================================================================================
// Implementation of inline methods:

inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
inline int  Solver::level (Var x) const { return vardata[x].level; }

inline void Solver::insertVarOrder(Var x) {
    if (!order_heap.inHeap(x) && decision[x]) order_heap.insert(x); }

inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }
inline void Solver::varBumpActivity(Var v) {
         varBumpActivity(v, var_inc);
}
inline void Solver::varBumpActivity(Var v, double inc) {
    if ( (activity[v] += inc) > 1e100 ) {
        // Rescale:
        for (int i = 0; i < nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100; }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v); 
}

inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
inline void Solver::claBumpActivity (Clause& c) {
        if ( (c.activity() += cla_inc) > 1e20 ) {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                ca[learnts[i]].activity() *= 1e-20;
            cla_inc *= 1e-20; } }

inline void Solver::checkGarbage(void){ return checkGarbage(garbage_frac); }
inline void Solver::checkGarbage(double gf){
    if (ca.wasted() > ca.size() * gf)  garbageCollect();
}

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool     Solver::enqueue         (Lit p, CRef from)      { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
inline bool     Solver::addClause       (const vec<Lit>& ps)    { ps.copyTo(add_tmp); return addClause_(add_tmp); }
inline bool     Solver::addEmptyClause  ()                      { add_tmp.clear(); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p)                 { add_tmp.clear(); add_tmp.push(p); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q)          { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q, Lit r)   { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); add_tmp.push(r); return addClause_(add_tmp); }
 inline bool     Solver::locked          (const Clause& c) const { 
   if(c.size()>2) 
     return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c; 
   return 
     (value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c)
     || 
     (value(c[1]) == l_True && reason(var(c[1])) != CRef_Undef && ca.lea(reason(var(c[1]))) == &c);
 }
inline void     Solver::newDecisionLevel()                      { trail_lim.push(trail.size()); }

inline int      Solver::decisionLevel ()      const   { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel (Var x) const   { return 1 << (level(x) & 31); }
inline lbool    Solver::value         (Var x) const   { return assigns[x]; }
inline lbool    Solver::value         (Lit p) const   { return assigns[var(p)] ^ sign(p); }
inline lbool    Solver::modelValue    (Var x) const   { return model[x]; }
inline lbool    Solver::modelValue    (Lit p) const   { return model[var(p)] ^ sign(p); }
inline int      Solver::nAssigns      ()      const   { return trail.size(); }
inline int      Solver::nClauses      ()      const   { return clauses.size(); }
inline int      Solver::nLearnts      ()      const   { return learnts.size(); }
inline int      Solver::nVars         ()      const   { return vardata.size(); }
inline int      Solver::nFreeVars     ()      const   { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline int      Solver::nUnfixedVars  ()      const   { return (int)nVars()  - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline void     Solver::setPolarity   (Var v, bool b) { polarity[v] = b; }
inline void     Solver::setDecisionVar(Var v, bool b) 
{ 
    if      ( b && !decision[v]) dec_vars++;
    else if (!b &&  decision[v]) dec_vars--;

    decision[v] = b;
    insertVarOrder(v);
}
inline void     Solver::setConfBudget(int64_t x){ conflict_budget    = conflicts    + x; }
inline void     Solver::setPropBudget(int64_t x){ propagation_budget = propagations + x; }
inline void     Solver::interrupt(){ asynch_interrupt = true; }
inline void     Solver::clearInterrupt(){ asynch_interrupt = false; }
inline void     Solver::budgetOff(){ conflict_budget = propagation_budget = -1; }

inline bool     Solver::withinBudget() const {
    return !asynch_interrupt && (termCallback == NULL || 0 == termCallback(termCallbackState)) &&
           (conflict_budget    < 0 || conflicts < (uint64_t)conflict_budget) &&
           (propagation_budget < 0 || propagations < (uint64_t)propagation_budget); }

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool     Solver::solve         ()                    { budgetOff(); assumptions.clear(); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p)               { budgetOff(); assumptions.clear(); assumptions.push(p); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q)        { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q, Lit r) { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); assumptions.push(r); return solve_() == l_True; }
inline bool     Solver::solve         (const vec<Lit>& assumps){ budgetOff(); assumps.copyTo(assumptions); return solve_() == l_True; }
inline lbool    Solver::solveLimited  (const vec<Lit>& assumps){ assumps.copyTo(assumptions); return solve_(); }
inline lbool    Solver::solveLimited2 (const vec<Lit>& assumps){ assumps.copyTo(assumptions); return solve2_(); }
inline bool     Solver::okay          ()      const   { return ok; }
inline lbool    Solver::solveNoAssump()  { budgetOff(); assumptions.clear(); return solve2_(); }

inline void     Solver::toDimacs     (const char* file){ vec<Lit> as; toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p){ vec<Lit> as; as.push(p); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q){ vec<Lit> as; as.push(p); as.push(q); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q, Lit r){ vec<Lit> as; as.push(p); as.push(q); as.push(r); toDimacs(file, as); }


//=================================================================================================
// Debug etc:

inline void Solver::printLit(Lit l)
{
    printf("%s%d ", sign(l) ? "-" : "", var(l)+1);
}

inline void Solver::printClause(CRef cr)
{
  Clause &c = ca[cr];
    for (int i = 0; i < c.size() && i<100; i++){
        printLit(c[i]);
        printf(" ");
    }
    printf("sz=%d \n",c.size());
}

//=================================================================================================
}

#endif
