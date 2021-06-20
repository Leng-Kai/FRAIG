/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

static string nonV = "00000000";

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
    friend class CirMgr;
    friend unsigned hashKey(CirGate* gate, unsigned k);
public:
    CirGate(unsigned gateID, unsigned lineNo): _gateID(gateID), _lineNo(lineNo), _symbol(""), _ref(0), _canBeReached(false), _var(-1)
    {
        for(size_t i = 0; i < 8; ++i)
            _value.push(nonV);
    }
    virtual ~CirGate() {}

   // Basic access methods
    virtual string getTypeStr() const = 0;
    unsigned getLineNo() const { return _lineNo + 1; }
    virtual bool isAig() const = 0;         // ?

   // Printing functions
    virtual void printGate() const = 0;
    void reportGate() const;
    void reportFanin(int level) const;
    void reportFanout(int level) const;
    
    void setSymbol(string& str) { _symbol = str; }
    virtual void DFS(map<unsigned, CirGate*>& _gateList, vector<CirGate*>& _dfsList) = 0;
    void addFanout(unsigned n) { _fanout.push_back(n); }
    
    virtual bool simulateByDFS() = 0;
    bool                _sim;
    unsigned            _ref;
    vector<unsigned>*   _fecGroup;
    bool                _invFec;
    
    void setVar(const Var& v) { _var = v; }
    Var  getVar() const { return _var; }
    
    mutable queue<string>   _value;
   
private:

protected:
    unsigned    _gateID;
    unsigned    _lineNo;
    unsigned    _fanin0;
    unsigned    _fanin1;
    CirGate*    _f0ptr;
    CirGate*    _f1ptr;
    bool        _invPhase0;
    bool        _invPhase1;
    string      _symbol;
    
    vector<unsigned>    _fanout;
    
    static unsigned     _globalRef;
    bool                _canBeReached;
    
    size_t      _simResults;
    
    Var         _var;
};

class CirPiGate: public CirGate
{
public:
    CirPiGate(unsigned gateID, unsigned lineNo): CirGate(gateID, lineNo) {}
    ~CirPiGate() {}
    bool isAig() const { return false; }
    string getTypeStr() const { if(_gateID == 0) return "CONST"; return "PI"; }
    void printGate() const
    {
        if(_gateID == 0) cout << "CONST0\n";
        else
        {
            cout << "PI  " << _gateID;
            if(_symbol.size()) cout << " (" << _symbol << ")";
            cout << endl;
        }
    }
    void DFS(map<unsigned, CirGate*>& _gateList, vector<CirGate*>& _dfsList)
    {
        if(_ref == _globalRef) return;
        _dfsList.push_back(this);
        _ref = _globalRef;
        _canBeReached = true;
    }
    bool simulateByDFS()
    {
//        cout << _gateID << endl;
//        cout << "PI" << _gateID << ":" << _sim << endl;
        _ref = _globalRef;
        return _sim;
    }
};

class CirPoGate: public CirGate
{
public:
    CirPoGate(unsigned gateID, unsigned lineNo, unsigned fanin): CirGate(gateID, lineNo)
    {
        _fanin0 = fanin / 2;
        _invPhase0 = (fanin % 2 == 1);
    }
    ~CirPoGate() {}
    bool isAig() const { return false; }
    string getTypeStr() const { return "PO"; }
    void printGate() const
    {
        cout << "PO  " << _gateID << " ";
        if(!_f0ptr) cout << "*";
        if(_invPhase0) cout << "!";
        cout << _fanin0;
        if(_symbol.size()) cout << " (" << _symbol << ")";
        cout << endl;
    }
    void DFS(map<unsigned, CirGate*>& _gateList, vector<CirGate*>& _dfsList)
    {
        if(_ref == _globalRef) return;
        bool withFloatingFanin = false;
        if(_gateList[_fanin0])
        {
            _gateList[_fanin0]->DFS(_gateList, _dfsList);
            _f0ptr = _gateList[_fanin0];
//            cout << _gateID << " " << _f0ptr << endl;
        }
        else withFloatingFanin = true;
        _dfsList.push_back(this);
        _ref = _globalRef;
        _canBeReached = true;
    }
    bool simulateByDFS()
    {
        bool _f0;
        if(_f0ptr->_ref == _globalRef) _f0 = _f0ptr->_sim;
        else _f0 = _f0ptr->simulateByDFS();
        if(_invPhase0) _f0 = !_f0;
//        cout << "PO" << _gateID << ":" << _f0 << endl;
        _sim = _f0;
        _ref = _globalRef;
        return _f0;
    }
};

class CirAigGate: public CirGate
{
public:
    CirAigGate(unsigned gateID, unsigned lineNo, unsigned fanin0, unsigned fanin1): CirGate(gateID, lineNo)
    {
        _fanin0 = fanin0 / 2;
        _fanin1 = fanin1 / 2;
        _invPhase0 = (fanin0 % 2 == 1);
        _invPhase1 = (fanin1 % 2 == 1);
    }
    ~CirAigGate() {}
    bool isAig() const { return true; }
    string getTypeStr() const { return "AIG"; }
    void printGate() const
    {
        cout << "AIG " << _gateID << " ";
        if(!_f0ptr) cout << "*";
        if(_invPhase0) cout << "!";
        cout << _fanin0 << " ";
        if(!_f1ptr) cout << "*";
        if(_invPhase1) cout << "!";
        cout << _fanin1 << " ";
        cout << endl;
    }
    void DFS(map<unsigned, CirGate*>& _gateList, vector<CirGate*>& _dfsList)
    {
        if(_ref == _globalRef) return;
        bool withFloatingFanin = false;
        if(_gateList[_fanin0])
        {
            _gateList[_fanin0]->DFS(_gateList, _dfsList);
            _f0ptr = _gateList[_fanin0];
//            cout << _gateID << " " << _f0ptr << endl;
        }
        else withFloatingFanin = true;
        if(_gateList[_fanin1])
        {
            _gateList[_fanin1]->DFS(_gateList, _dfsList);
            _f1ptr = _gateList[_fanin1];
//            cout << _gateID << " " << _f1ptr << endl;
        }
        else withFloatingFanin = true;
        _dfsList.push_back(this);
        _ref = _globalRef;
        _canBeReached = true;
    }
    bool simulateByDFS()
    {
        bool _f0;
        if(_f0ptr->_ref == _globalRef) _f0 = _f0ptr->_sim;
        else _f0 = _f0ptr->simulateByDFS();
        if(_invPhase0) _f0 = !_f0;
        bool _f1;
        if(_f1ptr->_ref == _globalRef) _f1 = _f1ptr->_sim;
        else _f1 = _f1ptr->simulateByDFS();
        if(_invPhase1) _f1 = !_f1;
//        cout << _gateID << endl;
//        cout << "AIG" << _gateID << ":" << _f0 << " " << _f1 << endl;
        _sim = _f0 & _f1;
        _ref = _globalRef;
        return _f0 & _f1;
    }
};

class CirUndefGate: public CirGate
{
public:
    CirUndefGate(unsigned gateID, unsigned nofanin): CirGate(gateID, 0)
    {
        _fanin0 = _fanin1 = nofanin;
    }
    ~CirUndefGate() {}
    bool isAig() const { return false; }
    string getTypeStr() const { return "UNDEF"; }
    void printGate() const {}
    void DFS(map<unsigned, CirGate*>& _gateList, vector<CirGate*>& _dfsList) { /* _canBeReached = true; */ }
    bool simulateByDFS()
    {
        _sim = false;
        _ref = _globalRef;
        return false;
    }
};

#endif // CIR_GATE_H
