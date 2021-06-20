/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <queue>
#include <map>
#include <algorithm>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
    friend class CirGate;
    friend class CirPiGate;
    friend class CirPoGate;
    friend class CirAigGate;
public:
   CirMgr() {}
   ~CirMgr() {} 

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const
    {
        if(_gateList[gid]) return _gateList[gid];
        else               return _floGateList[gid];
    }

   // Member functions about circuit construction
   bool readCircuit(const string&);
    void genDFSList();

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
    void printFanins(unsigned gateID, int curLevel, int level, bool inv);
    void printFanouts(unsigned gateID, int curLevel, int level, unsigned from);
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   ofstream           *_simLog;
    static CirGate* const0;
    mutable map<unsigned, CirGate*> _gateList;
    mutable map<unsigned, CirGate*> _floGateList; // undef
    size_t M, I, L, O, A;
    vector<CirGate*> _piList;
    vector<CirGate*> _poList;
    vector<CirGate*> _aigList;
    vector<CirGate*> _dfsList;
    vector<unsigned> _floting;      // with floting fanins
    vector<unsigned> _unused;
    
    void reset();
    bool readHeader(ifstream& inFile);
    bool readPI(ifstream& inFile);
    bool readPO(ifstream& inFile);
    bool readAig(ifstream& inFile);
    bool readSymbol(ifstream& inFile);
    
    void removeGate(unsigned gid);
    void replaceByFanin(unsigned gid, unsigned fanin);
//    void replaceByConst(unsigned gid);
    void merge(CirGate* mgate, CirGate* gate);
    
    void simulate(string& pattern);
    mutable queue<vector<unsigned>*> fecPairs;
    string randomPattern(unsigned I);
    void genFECPairs();
    size_t getResults(unsigned n);
    void deleteDupVec();
    void deleteSingle();
    void updateValue();
    
    void genProofModel(SatSolver& s);
    vector<string> SATpatterns;
};

#endif // CIR_MGR_H
