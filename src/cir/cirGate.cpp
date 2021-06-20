/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <queue>
#include <algorithm>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
    unsigned line = _lineNo + 1;
    if(_gateID == 0) line--;
    string str = getTypeStr() + "(" + to_string(_gateID) + ")";
    if(_symbol.size()) str += ("\"" + _symbol + "\"");
    str += ", line " + to_string(line);
    cout << "================================================================================"; // 80
    cout << "= " << left << setw(78) << str << endl;
    cout << "= FECs:";
    int b = (_invFec ? 0 : 1);
    int c;
    if(_fecGroup)
    {
        sort(_fecGroup->begin(), _fecGroup->end());
        for(size_t i = 0; i < _fecGroup->size(); ++i)
        {
            c = (*_fecGroup)[i] / 2;
            if(c == _gateID) continue;
            cout << " " << ((*_fecGroup)[i] % 2 == b ? "!" : "") << c;
        }
    }
    cout << endl;
    cout << "= Value: ";
    string v;
    for(size_t i = 0; i < 7; ++i)
    {
        v = _value.front();
        _value.pop();
        cout << v << "_";
        _value.push(v);
    }
    v = _value.front();
    _value.pop();
    cout << v;
    _value.push(v);
    cout << endl;
    cout << "================================================================================\n";
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
    _globalRef++;
    cirMgr->printFanins(_gateID, level, level, false);
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
    _globalRef++;
    cirMgr->printFanouts(_gateID, level, level, 0);
}

unsigned CirGate::_globalRef = 0;
