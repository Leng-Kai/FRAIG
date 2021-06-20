/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

unsigned
hashKey(CirGate* gate, unsigned k)
{
    unsigned fanin0 = gate->_fanin0 * 2;
    if(gate->_invPhase0) fanin0++;
    unsigned fanin1 = gate->_fanin1 * 2;
    if(gate->_invPhase1) fanin1++;
    unsigned a = (fanin0 < fanin1 ? fanin0 : fanin1);
    unsigned b = (fanin0 < fanin1 ? fanin1 : fanin0);
    return (k * (a - 1) - (a * (a - 1)) / 2 + (b - a));
}

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
    unsigned k = (M + O + 3) * 2;
    unordered_map<unsigned, CirGate*> m;
    for(size_t i = 0; i < _dfsList.size(); ++i)
    {
        if(_dfsList[i]->getTypeStr() != "AIG") continue;
        if(m[hashKey(_dfsList[i], k)])
        {
            merge(m[hashKey(_dfsList[i], k)], _dfsList[i]);
            cout << "Strashing: " << m[hashKey(_dfsList[i], k)]->_gateID
            << " merging " << _dfsList[i]->_gateID << "...\n";
        }
        else
            m[hashKey(_dfsList[i], k)] = _dfsList[i];
    }
    
    genDFSList();
    sort(_floting.begin(), _floting.end());
    sort(_unused.begin(), _unused.end());
}

void
CirMgr::fraig()
{
    SatSolver solver;
    solver.initialize();
    
//    initCircuit();
    genProofModel(solver);
   
    unordered_map<vector<unsigned>*, CirGate*> leadingGate;
    size_t n = fecPairs.size();
    CirGate* gate;
    vector<unsigned>* v;
    string SATpattern;
    size_t gateMerged = 0;
    for(size_t i = 0; i < _dfsList.size(); ++i)
    {
        gate = _dfsList[i];
        if(gate->getTypeStr() == "PO") continue;
        if(!gate->_fecGroup) continue;
        if(!leadingGate[gate->_fecGroup]) leadingGate[gate->_fecGroup] = gate;
        else
        {
            Var newV = solver.newVar();
            int a, b;
            for(size_t j = 0; j < gate->_fecGroup->size(); ++j)
            {
                if((*gate->_fecGroup)[j] / 2 == leadingGate[gate->_fecGroup]->_gateID)
                {
                    a = (*gate->_fecGroup)[j];
                }
                if((*gate->_fecGroup)[j] / 2 == gate->_gateID)
                {
                    b = (*gate->_fecGroup)[j];
                    (*gate->_fecGroup)[j] = (*gate->_fecGroup)[0];
                    gate->_fecGroup->erase(gate->_fecGroup->begin());
                    j--;
//                    cout << "ID:" << gate->_gateID << endl;
//                    for(size_t k = 0; k < gate->_fecGroup->size(); ++k)
//                        cout << (*gate->_fecGroup)[k] << " ";
//                    cout << endl << endl;
                }
            }
            bool inv = (abs(b - a) % 2 == 1);
            solver.addXorCNF(newV, leadingGate[gate->_fecGroup]->getVar(), false, gate->getVar(), inv);
            solver.assumeRelease();
            solver.assumeProperty(newV, true);
            bool result = solver.assumpSolve();
//            cerr << leadingGate[gate->_fecGroup]->_gateID << " " << gate->_gateID << " " << inv << " " << result << endl;
            if(!result) // UNSAT
            {gateMerged++;
                merge(leadingGate[gate->_fecGroup], gate);
                cout << "Fraig: " << leadingGate[gate->_fecGroup]->_gateID << " merging " << (inv? "!" : "") << gate->_gateID << "...\n";
            }
            else
            {
                // ?
                SATpattern = "";
                for(size_t i = 0; i < I; ++i)
                {
                    if(solver.getValue(_piList[i]->getVar()))
                        SATpattern += "1";
                    else SATpattern += "0";
//                    cout << solver.getValue(_piList[i]->getVar()) << " ";
                }
//                cout << solver.getValue(const0->getVar());
//                cout << endl;
                SATpatterns.push_back(SATpattern);
            }
        }
    }
    
    genDFSList();
    sort(_floting.begin(), _floting.end());
    sort(_unused.begin(), _unused.end());
//    cout << "gate merged: " << gateMerged << endl;
    while(fecPairs.size())
        fecPairs.pop();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/

void
CirMgr::merge(CirGate* mgate, CirGate* gate)
{
    unsigned mgid = mgate->_gateID;
    unsigned gid  = gate->_gateID;
    
    CirGate* fanin = getGate(gate->_fanin0);
    for(vector<unsigned>::iterator it = fanin->_fanout.begin();
        it != fanin->_fanout.end(); ++it)
    {
        if(*it == gid)
        {
            fanin->_fanout.erase(it);
            break;
        }
    }
    fanin = getGate(gate->_fanin1);
    for(vector<unsigned>::iterator it = fanin->_fanout.begin();
        it != fanin->_fanout.end(); ++it)
    {
        if(*it == gid)
        {
            fanin->_fanout.erase(it);
            break;
        }
    }
    
    CirGate* fanouts;
    for(size_t i = 0; i < gate->_fanout.size(); ++i)
    {
        fanouts = getGate(gate->_fanout[i]);
        if(fanouts->_fanin0 == gid)
            fanouts->_fanin0 = mgid;
        else
            fanouts->_fanin1 = mgid;
        mgate->_fanout.push_back(gate->_fanout[i]);
    }
    
    for(vector<CirGate*>::iterator it = _aigList.begin();
        it != _aigList.end(); ++it)
    {
        if(*it == gate)
        {
            _aigList.erase(it);
            break;
        }
    }
    
    for(vector<unsigned>::iterator it = _floting.begin();
        it != _floting.end(); ++it)
    {
        if(*it == gid)
        {
            _floting.erase(it);
            break;
        }
    }
}

//void
//CirMgr::initCircuit()
//{
//
//}

void
CirMgr::genProofModel(SatSolver& s)
{
    for(size_t i = 0; i < _dfsList.size(); ++i)
    {
        Var v = s.newVar();
        _dfsList[i]->setVar(v);
    }
    
    for(size_t i = 0; i < _dfsList.size(); ++i)
    {
        if(_dfsList[i]->getTypeStr() != "AIG") continue;
        
        s.addAigCNF(_dfsList[i]->getVar(), _dfsList[i]->_f0ptr->getVar(), _dfsList[i]->_invPhase0, _dfsList[i]->_f1ptr->getVar(), _dfsList[i]->_invPhase1);
    }
    
    if(const0->getVar() == -1)
    {
        Var v = s.newVar();
        const0->setVar(v);
    }
    
    s.addAigCNF(const0->getVar(), const0->getVar(), true, const0->getVar(), false);
}
