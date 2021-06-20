/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/

CirGate* CirMgr::const0 = new CirPiGate(0, 0);

bool
CirMgr::readCircuit(const string& fileName)
{
    reset();
    CirGate::_globalRef = 0;
    ifstream inFile(fileName);

    if(!inFile)
    {
        cerr << "Cannot open design \"" << fileName << "\"!!\n";
        return false;
    }

    if(!readHeader(inFile)) return false;
    if(!readPI(inFile)) return false;
    if(!readPO(inFile)) return false;
    if(!readAig(inFile)) return false;
    if(!readSymbol(inFile)) return false;

    genDFSList();

    for(size_t i = 0; i < _aigList.size(); ++i)
    {
        bool withFloatingFanin = false;
        if(_gateList[_aigList[i]->_fanin0])
            _gateList[_aigList[i]->_fanin0]->addFanout(_aigList[i]->_gateID);
        else
        {
            withFloatingFanin = true;
            _floGateList[_aigList[i]->_fanin0] = new CirUndefGate(_aigList[i]->_fanin0, M + O + 1);
            _floGateList[_aigList[i]->_fanin0]->addFanout(_aigList[i]->_gateID);
        }
        if(_gateList[_aigList[i]->_fanin1])
            _gateList[_aigList[i]->_fanin1]->addFanout(_aigList[i]->_gateID);
        else
        {
            withFloatingFanin = true;
            _floGateList[_aigList[i]->_fanin1] = new CirUndefGate(_aigList[i]->_fanin1, M + O + 1);
            _floGateList[_aigList[i]->_fanin1]->addFanout(_aigList[i]->_gateID);
        }
        if(withFloatingFanin) _floting.push_back(_aigList[i]->_gateID);
    }
    
    for(size_t i = 0; i < _poList.size(); ++i)
    {
        if(_gateList[_poList[i]->_fanin0])
            _gateList[_poList[i]->_fanin0]->addFanout(_poList[i]->_gateID);
        else
        {
            _floting.push_back(_poList[i]->_gateID);
            _floGateList[_poList[i]->_fanin0] = new CirUndefGate(_poList[i]->_fanin0, M + O + 1);
            _floGateList[_poList[i]->_fanin0]->addFanout(_poList[i]->_gateID);
        }
    }

    for(size_t i = 0; i < _piList.size(); ++i)
        if(!_piList[i]->_fanout.size()) _unused.push_back(_piList[i]->_gateID);
    for(size_t i = 0; i < _aigList.size(); ++i)
        if(!_aigList[i]->_fanout.size()) _unused.push_back(_aigList[i]->_gateID);

    sort(_floting.begin(), _floting.end());
    sort(_unused.begin(), _unused.end());
    
   return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
    cout << "\nCircuit Statistics\n"
    << "==================\n"
    << "  PI" << right << setw(12) << _piList.size() << endl
    << "  PO" << right << setw(12) << _poList.size() << endl
    << "  AIG" << right << setw(11) << _aigList.size() << endl
    << "------------------\n"
    << "  Total" << setw(9) << _piList.size() + _poList.size() + _aigList.size() << endl;
}

void
CirMgr::printNetlist() const
{
   cout << endl;
   for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
      cout << "[" << i << "] ";
      _dfsList[i]->printGate();
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
    for(size_t i = 0; i < _piList.size(); ++i)
        cout << " " << _piList[i]->_gateID;
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
    for(size_t i = 0; i < _poList.size(); ++i)
        cout << " " << _poList[i]->_gateID;
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
    if(_floting.size())
    {
        cout << "Gates with floating fanin(s):";
        for(size_t i = 0; i < _floting.size(); ++i)
            cout << " " << _floting[i];
        cout << endl;
    }
    if(_unused.size())
    {
        cout << "Gates defined but not used  :";
        for(size_t i = 0; i < _unused.size(); ++i)
            cout << " " << _unused[i];
        cout << endl;
    }
}

void
CirMgr::printFECPairs() const
{
    size_t n = fecPairs.size();
    vector<unsigned>* v;
    for(size_t i = 0; i < n; ++i)
    {
        v = fecPairs.front();
        fecPairs.pop();
        if(v->size() > 1)
            fecPairs.push(v);
    }
//    cout << "size = " << n << endl;
    n = fecPairs.size();
    for(size_t i = 0; i < n; ++i)
    {
        v = fecPairs.front();
        fecPairs.pop();
        sort(v->begin(), v->end());
        cout << "[" << i << "] ";
        for(size_t j = 0; j < v->size() - 1; ++j)
        {
            if((*v)[j] % 2 == 1) cout << "!";
            cout << (*v)[j] / 2 << " ";
        }
        if((*v)[v->size() - 1] % 2 == 1) cout << "!";
        cout << (*v)[v->size() - 1] / 2;
        cout << endl;
        fecPairs.push(v);
    }
}

void
CirMgr::printFanins(unsigned gateID, int curLevel, int level, bool inv)
{
    if(curLevel < 0) return;
    
    cout << setw(2 * (level - curLevel)) << "";
    if(inv) cout << "!";
    
    CirGate* gate = _gateList[gateID];
    if(!gate) gate = _floGateList[gateID];
    
    string type = gate->getTypeStr();
    cout << type << " " << gateID;
    
    if(type == "PI" | type == "CONST")
    {
        cout << endl;
        return;
    }
    
    if(gate->_ref == CirGate::_globalRef)
    {
        cout << " (*)\n";
        return;
    }
    
    cout << endl;
    
    if(type == "PO")
    {
        printFanins(gate->_fanin0, curLevel - 1, level, gate->_invPhase0);
        if((_gateList[gate->_fanin0] != nullptr | _floGateList[gate->_fanin0] != nullptr) & (curLevel != 0)) gate->_ref = CirGate::_globalRef;
    }
    
    if(type == "AIG")
    {
        printFanins(gate->_fanin0, curLevel - 1, level, gate->_invPhase0);
        printFanins(gate->_fanin1, curLevel - 1, level, gate->_invPhase1);
        if(((_gateList[gate->_fanin0] != nullptr) | (_floGateList[gate->_fanin0] != nullptr) | (_gateList[gate->_fanin1] != nullptr) | (_floGateList[gate->_fanin1] != nullptr)) & (curLevel != 0)) gate->_ref = CirGate::_globalRef;
    }
}

void
CirMgr::printFanouts(unsigned gateID, int curLevel, int level, unsigned from)
{
    if(curLevel < 0) return;
    
    CirGate* gate = _gateList[gateID];
    if(!gate) gate = _floGateList[gateID];
    
    cout << setw(2 * (level - curLevel)) << "";
    
    if(curLevel != level)
    {
        bool inv = false;
        if((gate->_fanin0 == from) & (gate->_invPhase0)) inv = true;
        if((gate->_fanin1 == from) & (gate->_invPhase1)) inv = true;
        if(inv) cout << "!";
    }
    
    string type = gate->getTypeStr();
    cout << type << " " << gateID;
    
    if(gate->_ref == CirGate::_globalRef)
    {
        cout << " (*)\n";
        return;
    }
    
    cout << endl;
    
    for(size_t i = 0; i < gate->_fanout.size(); ++i)
        printFanouts(gate->_fanout[i], curLevel - 1, level, gateID);
    
    if(gate->_fanout.size() != 0 & curLevel != 0) gate->_ref = CirGate::_globalRef;
}

void
CirMgr::writeAag(ostream& outfile) const
{
    unsigned adjA = 0;
    for(size_t i = 0; i < _aigList.size(); ++i)
        if(_aigList[i]->_canBeReached)
            adjA++;
    outfile << "aag " << M << " " << I << " " << L << " " << O << " " << adjA << endl;
    
    for(size_t i = 0; i < _piList.size(); ++i)
        outfile << _piList[i]->_gateID * 2 << endl;
    
    for(size_t i = 0; i < _poList.size(); ++i)
    {
        unsigned fanin = _poList[i]->_fanin0 * 2;
        if(_poList[i]->_invPhase0) fanin++;
        outfile << fanin << endl;
    }
    
    for(size_t i = 0; i < _aigList.size(); ++i)
    {
        if(!_aigList[i]->_canBeReached) continue;
        unsigned fanin0 = _aigList[i]->_fanin0 * 2;
        unsigned fanin1 = _aigList[i]->_fanin1 * 2;
        if(_aigList[i]->_invPhase0) fanin0++;
        if(_aigList[i]->_invPhase1) fanin1++;
        outfile << _aigList[i]->_gateID * 2 << " " << fanin0 << " " << fanin1 << endl;
    }
    
    bool stop = false;
    
    for(size_t i = 0; i < _piList.size(); ++i)
    {
        if(_piList[i]->_symbol.size())
            outfile << "i" << i << " " << _piList[i]->_symbol << endl;
        else
        {
            stop = true;
            break;
        }
    }
    
    if(!stop)
        for(size_t i = 0; i < _poList.size(); ++i)
        {
            if(_poList[i]->_symbol.size())
                outfile << "o" << i << " " << _poList[i]->_symbol << endl;
            else
            {
                stop = true;
                break;
            }
        }
    
    outfile << "c\n";
    outfile << "    (\\ (\\         期末好運兔兔\n";
    outfile << "c(￣(_ˊ• x •`)_—☆\n";
    outfile << "-----------------\n";
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
    if(g->getTypeStr() != "AIG")
        cout << "Error: Gate(" << g->_gateID << ") is NOT an AIG!!\n";
    vector<unsigned> _pi;
//    vector<CirGate*> _po;
    vector<CirGate*> _aig;
    vector<CirGate*> _dfs;
    CirGate::_globalRef++;
    g->DFS(_gateList, _dfs);
    for(size_t i = 0; i < _dfs.size(); ++i)
    {
        if(_dfs[i]->getTypeStr() == "PI")
            _pi.push_back(_dfs[i]->_gateID);
//        else if(_dfs[i]->getTypeStr() == "PO")
//            _po.push_back(_dfs[i]);
        else if(_dfs[i]->getTypeStr() == "AIG")
            _aig.push_back(_dfs[i]);
    }
    
    unsigned i = _pi.size();
//    unsigned o = _po.size();
    unsigned a = _aig.size();
    unsigned m = a + i;
    if(m < g->_gateID) m = g->_gateID;
    outfile << "aag" << " " << m << " " << i << " 0 1 " << a << endl;
    sort(_pi.begin(), _pi.end());
    for(size_t k = 0; k < _pi.size(); ++k)
        outfile << _pi[k]* 2 << endl;
    outfile << g->_gateID * 2 << endl;
    for(size_t k = 0; k < _aig.size(); ++k)
    {
        unsigned f1 = _aig[k]->_f0ptr->_gateID * 2;
        unsigned f2 = _aig[k]->_f1ptr->_gateID * 2;
        if(_aig[k]->_invPhase0) f1++;
        if(_aig[k]->_invPhase1) f2++;
        outfile << _aig[k]->_gateID * 2 << " " << f1 << " " << f2 << endl;
    }
    for(size_t k = 0; k < _pi.size(); ++k)
    {
        if(getGate(_pi[k])->_symbol.size())
            cout << "i" << k << " " << getGate(_pi[k])->_symbol << endl;
    }
    outfile << "o0 " << g->_gateID << endl;
    
    outfile << "c\n";
    outfile << "    (\\ (\\         期末好運兔兔\n";
    outfile << "c(￣(_ˊ• x •`)_—☆\n";
    outfile << "-----------------\n";
}

void
CirMgr::genDFSList()
{
    CirGate::_globalRef++;
    _dfsList.clear();
    for(size_t i = 0; i < _poList.size(); ++i)
        _poList[i]->DFS(_gateList, _dfsList);
}

void
CirMgr::reset()
{
    lineNo = 0;
    colNo = 0;
    _gateList.clear();
    _floGateList.clear();   //
    _piList.clear();
    _poList.clear();
    _aigList.clear();
    _dfsList.clear();
    _floting.clear();
    _unused.clear();
    const0->_ref = 0;
    const0->_fanout.clear();
    CirGate::_globalRef = 0;
    
    while(fecPairs.size())
        fecPairs.pop();
    SATpatterns.clear();
}

bool
CirMgr::readHeader(ifstream& inFile)
{
    string command;
    inFile >> command;
    inFile >> command;
    M = stoi(command);
    inFile >> command;
    I = stoi(command);
    inFile >> command;
    L = stoi(command);
    inFile >> command;
    O = stoi(command);
    inFile >> command;
    A = stoi(command);

    return true;
}

bool
CirMgr::readPI(ifstream& inFile)
{
    _gateList[0] = const0;
    string command;

    for(size_t i = 0; i < I; ++i)
    {
        lineNo++;
        inFile >> command;
        unsigned lit = stoi(command);
        unsigned gateID = lit / 2;
        CirGate* pi = new CirPiGate(gateID, lineNo);
        _gateList[gateID] = pi;
        _piList.push_back(pi);
    }

    return true;
}

bool
CirMgr::readPO(ifstream& inFile)
{
    string command;
    for(size_t i = 0; i < O; ++i)
    {
        lineNo++;
        inFile >> command;
        unsigned lit = stoi(command);
//        unsigned fanin = lit / 2;
        CirGate* po = new CirPoGate(M + 1 + i, lineNo, lit);
        _gateList[M + 1 + i] = po;
        _poList.push_back(po);
    }

    return true;
}

bool
CirMgr::readAig(ifstream& inFile)
{
    string command;
    for(size_t i = 0; i < A; ++i)
    {
        lineNo++;
        inFile >> command;
        unsigned lit = stoi(command);
        unsigned gateID = lit / 2;
        unsigned fanin0, fanin1;
        inFile >> command;
        fanin0 = stoi(command);
        inFile >> command;
        fanin1 = stoi(command);
        CirGate* aig = new CirAigGate(gateID, lineNo, fanin0, fanin1);
        _gateList[gateID] = aig;
        _aigList.push_back(aig);
    }

    return true;
}

bool
CirMgr::readSymbol(ifstream& inFile)
{
    string command;
    bool done = false;
    while(true)
    {
        lineNo++;
        inFile >> command;

        if(inFile.eof()) break;
        if(command[0] == 'c')
        {
            done = true;
            break;
        }

        string type = command.substr(0, 1);
        command = command.substr(1);
        unsigned index = stoi(command);

        inFile >> command;

        if(type == "i")
            _piList[index]->setSymbol(command);
        if(type == "o")
            _poList[index]->setSymbol(command);
    }

    return true;
}





























