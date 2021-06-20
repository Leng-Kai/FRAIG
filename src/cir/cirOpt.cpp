/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
    CirGate::_globalRef++;
//    _dfsList.clear();
//    for(size_t i = 0; i < _poList.size(); ++i)
//        _poList[i]->DFS(_gateList, _dfsList);
    for(size_t i = 0; i < _dfsList.size(); ++i)
        _dfsList[i]->_ref = CirGate::_globalRef;
    
    unsigned cnt = 0;
    CirGate* gate;
    for(unsigned i = I + 1; i <= M; ++i)
    {
        gate = getGate(i);
        if(!gate) continue;
        if(gate->_ref != CirGate::_globalRef)
        {
            cout << "Sweeping: " << gate->getTypeStr() << "(" << i << ") removed...\n";
            removeGate(i);
            cnt++;
        }
        // A
        // gateList
        // aigList
    }
    A -= cnt;
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
    bool removed = true;
    CirGate* gate;
    size_t index = 0;
    while(removed)
    {
        removed = false;
        if(const0->_fanout.size())
        {
            sort(const0->_fanout.begin(), const0->_fanout.end());
            while(const0->_fanout.size())
            {
                if(index >= const0->_fanout.size()) break;
                if(getGate(const0->_fanout[index])->getTypeStr() == "PO")
                {
                    index++;
                    continue;
                }
                gate = getGate(const0->_fanout[index]);
                if(gate->_fanin0 == 0 & gate->_fanin1 != 0)
                {
                    if(gate->_invPhase0)
                    {
                        replaceByFanin(const0->_fanout[index], gate->_fanin1);
                    }
                    else
                        replaceByFanin(const0->_fanout[index], 0);
                }
                else if(gate->_fanin1 == 0 & gate->_fanin0 != 0)
                {
                    if(gate->_invPhase1)
                        replaceByFanin(const0->_fanout[index], gate->_fanin0);
                    else
                        replaceByFanin(const0->_fanout[index], 0);
                }
                else if(gate->_fanin0 == 0 & gate->_fanin1 == 0)
                {
                    replaceByFanin(const0->_fanout[index], 0);
                }
            }
            removed = true;
            if(index >= const0->_fanout.size()) removed = false;
        }
        
        for(size_t i = 0; i < _dfsList.size(); ++i)
        {
            if(!_dfsList[i]) continue;
            else if(_dfsList[i]->getTypeStr() == "AIG")
            {
                gate = _dfsList[i];
                if(!(gate->_fanin0 == gate->_fanin1)) continue;
                else
                {
                    if(gate->_invPhase0 == gate->_invPhase1)
                        replaceByFanin(gate->_gateID, gate->_fanin0);
                    else
                        replaceByFanin(gate->_gateID, 0);
                    removed = true;
                }
            }
        }
    }
    
    // dfs list update
    
    genDFSList();
    sort(_floting.begin(), _floting.end());
    sort(_unused.begin(), _unused.end());
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/

void
CirMgr::removeGate(unsigned gid)
{
    CirGate* gate = _gateList[gid];
    if(gate) _gateList.erase(gid);
    else
    {
        gate = _floGateList[gid];
        _floGateList.erase(gid);
    }
    CirGate* fanin0 = _gateList[gate->_fanin0];
    CirGate* fanin1 = _gateList[gate->_fanin1];
//    cout << fanin0->_gateID << " " << fanin1->_gateID;
    if(fanin0)
    {
        for(vector<unsigned>::iterator it = fanin0->_fanout.begin();
            it != fanin0->_fanout.end(); ++it)
        {
            if(*it == gid)
            {
                fanin0->_fanout.erase(it);
                break;
            }
        }
//        cout << fanin0->_fanout.size() << endl;
        if(!fanin0->_fanout.size())
        {
//            cout << gate->_gateID << " " << fanin0->_gateID << endl;
            _unused.push_back(fanin0->_gateID);
        }
    }
    if(fanin1)
    {
        for(vector<unsigned>::iterator it = fanin1->_fanout.begin();
            it != fanin1->_fanout.end(); ++it)
        {
            if(*it == gid)
            {
                fanin1->_fanout.erase(it);
                break;
            }
        }
//        cout << fanin1->_fanout.size() << endl;
        if(!fanin1->_fanout.size())
        {
//            cout << gate->_gateID << " " << fanin1->_gateID << endl;
            _unused.push_back(fanin1->_gateID);
        }
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
    
    for(vector<unsigned>::iterator it = _unused.begin();
        it != _unused.end(); ++it)
    {
        if(*it == gid)
        {
            _unused.erase(it);
            break;
        }
    }
    
    delete gate;
}

void
CirMgr::replaceByFanin(unsigned gid, unsigned fanin)
{
    CirGate* gate = _gateList[gid];
    if(gate) _gateList.erase(gid);
    else
    {
        gate = _floGateList[gid];
        _floGateList.erase(gid);
    }
    CirGate* fanin0 = _gateList[gate->_fanin0];
    CirGate* fanin1 = _gateList[gate->_fanin1];
//    cout << gate->_fanin0 << " " << gate->_fanin1 << endl;
    if(fanin0)
    {
        for(vector<unsigned>::iterator it = fanin0->_fanout.begin();
            it != fanin0->_fanout.end(); ++it)
        {
            if(*it == gid)
            {
                fanin0->_fanout.erase(it);
                break;
            }
        }
        if(!fanin0->_fanout.size())
        {
            _unused.push_back(fanin0->_gateID);
        }
    }
    if(fanin1)
    {
        for(vector<unsigned>::iterator it = fanin1->_fanout.begin();
            it != fanin1->_fanout.end(); ++it)
        {
            if(*it == gid)
            {
                fanin1->_fanout.erase(it);
                break;
            }
        }
        if(!fanin1->_fanout.size())
        {
            _unused.push_back(fanin1->_gateID);
        }
    }
    
    bool inverse;
    if(gate->_fanin0 == fanin)
        inverse = gate->_invPhase0;
    else
        inverse = gate->_invPhase1;
    if(gate->_fanin0 == gate->_fanin1)
    {
        if(gate->_invPhase0 == gate->_invPhase1)
            inverse = gate->_invPhase0;
        else
            inverse = false;
    }
    
    CirGate* merge = getGate(fanin);
    CirGate* fanouts;
    for(size_t i = 0; i < gate->_fanout.size(); ++i)
    {
        fanouts = getGate(gate->_fanout[i]);
//        cout << gate->_fanout[i];
        if(fanouts->_fanin0 == gid)
        {
            fanouts->_fanin0 = fanin;
            if(inverse)
                fanouts->_invPhase0 = !fanouts->_invPhase0;
            if(fanouts->_fanin1 != gid)
                merge->_fanout.push_back(gate->_fanout[i]);
            else
                if(inverse) fanouts->_fanin1 = !fanouts->_fanin1;
        }
        else
        {
            fanouts->_fanin1 = fanin;
            if(inverse)
                fanouts->_invPhase1 = !fanouts->_invPhase1;
            if(fanouts->_fanin0 != gid)
                merge->_fanout.push_back(gate->_fanout[i]);
            else
                if(inverse) fanouts->_fanin0 = !fanouts->_fanin0;
        }
    }
    
//    cout << getGate(13)->_fanin0 << " " << getGate(13)->_fanin1 << endl;
//    cout << getGate(13)->_invPhase0 << " " << getGate(13)->_invPhase1 << endl;
    cout << "Simplifying: " << fanin << " merging ";
    if(inverse) cerr << "!";
    cout << gid << "...\n";
    
//    cout << getGate(11)->_fanin0 << " " << getGate(11)->_fanin1 << endl;
//    cout << gate->_fanin0 << " " << gate->_fanin1 << endl;
//    cout << gate->_invPhase0 << " " << gate->_invPhase1 << endl;
    
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
    
    for(vector<unsigned>::iterator it = _unused.begin();
        it != _unused.end(); ++it)
    {
        if(*it == gid)
        {
            _unused.erase(it);
            break;
        }
    }
    
    for(vector<CirGate*>::iterator it = _dfsList.begin();
        it != _dfsList.end(); ++it)
    {
        if(*it == gate)
        {
            *it = nullptr;
            break;
        }
    }
    
    delete gate;
}

//void
//CirMgr::replaceByConst0(unsigned gid)
//{
//    CirGate* gate = _gateList[gid];
//    if(gate) _gateList.erase(gid);
//    else
//    {
//        gate = _floGateList[gid];
//        _floGateList.erase(gid);
//    }
//    CirGate* fanin0 = _gateList[gate->_fanin0];
//    CirGate* fanin1 = _gateList[gate->_fanin1];
//}
