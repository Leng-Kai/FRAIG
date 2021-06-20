/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <unordered_map>
#include <queue>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

#define bigP 27644437

using namespace std;

static unsigned r;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

string
toBinary(size_t num, unsigned I)
{
    string pattern = "";         // const 0
    for(size_t i = 0; i < I; ++i)
        pattern += "0";
    int n = I - 1, m;
    while(n >= 0)
    {
        m = pow(2, n);
        if(num >= m)
        {
            num -= m;
            pattern[I - n - 1] = '1';
        }
        n--;
    }
    return pattern;
}

size_t toDecimal(string& pattern)
{
    size_t num = 0;
    for(size_t i = 0; i < pattern.size(); ++i)
        if(pattern[i] == '1')
            num += pow(2, pattern.size() - 1 - i);
    return num;
}

string
CirMgr::randomPattern(unsigned I)
{
    string pattern = "";
    if(SATpatterns.size())
    {
        pattern = SATpatterns[0];
        SATpatterns.erase(SATpatterns.begin());
        return pattern;
    }
    size_t t = pow(2, 16);
    while(I > 16)
    {
        pattern += toBinary(r % t, 16);
        r += bigP;
        I -= 16;
    }
    pattern += toBinary(r % (long)pow(2, I), I);
    r += bigP;
    return pattern;
}

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
    r = rand();
    if(fecPairs.empty())
    {
        vector<unsigned>* v = new vector<unsigned>;
        v->push_back(0);
        v->push_back(1);
        for(size_t i = 0; i < _dfsList.size(); ++i)
        {
            if(_dfsList[i]->getTypeStr() != "AIG") continue;
            v->push_back(_dfsList[i]->_gateID * 2);
            v->push_back(_dfsList[i]->_gateID * 2 + 1);
        }
        fecPairs.push(v);
    }
    
//    size_t maxUnsigned = pow(2, I);
    string pattern;
//    size_t results;
    size_t oldPairs = 0;
    size_t noNewPairGen = 0;
    size_t cnt = 0;
    string v1, v2;
    while(noNewPairGen < 30 & cnt < 4 * M)
    {
//        for(size_t i = 0; i < A; ++i)
//            _aigList[i]->_simResults = 0;

        for(size_t th = 0; th < 4; ++th)
        {
            for(size_t i = 0; i < _aigList.size(); ++i)
                _aigList[i]->_simResults = 0;
            
            for(size_t i = 0; i < 16; ++i)
            {
                pattern = randomPattern(I);
                simulate(pattern);
                for(size_t j = 0; j < _aigList.size(); ++j)
                    if(_aigList[j]->_sim)
                        _aigList[j]->_simResults += (1 << i);
                for(size_t j = 0; j < _piList.size(); ++j)
                    if(_piList[j]->_sim)
                        _piList[j]->_simResults += (1 << i);
                for(size_t j = 0; j < _poList.size(); ++j)
                    if(_poList[j]->_sim)
                        _poList[j]->_simResults += (1 << i);
                cnt++;
            }
            updateValue();
            oldPairs = fecPairs.size();
            genFECPairs();
            if(oldPairs == fecPairs.size()) noNewPairGen++;
        }
//        cerr << noNewPairGen << endl;
//        cerr << cnt << endl;
    }
    cout << cnt << " patterns simulated.\n";
}

void
CirMgr::fileSim(ifstream& patternFile)
{
    if(fecPairs.empty())
    {
        vector<unsigned>* v = new vector<unsigned>;
        v->push_back(0);
        v->push_back(1);
        for(size_t i = 0; i < _dfsList.size(); ++i)
        {
            if(_dfsList[i]->getTypeStr() != "AIG") continue;
            v->push_back(_dfsList[i]->_gateID * 2);
            v->push_back(_dfsList[i]->_gateID * 2 + 1);
        }
        fecPairs.push(v);
    }
    
    string pattern;
    size_t cnt = 0;
    size_t digit = 0;
    for(size_t i = 0; i < _aigList.size(); ++i)
        _aigList[i]->_simResults = 0;
    string resStr = "";
    for(size_t i = 0; i < I; ++i)
        resStr += "0";
    while(true)
    {
        if(!patternFile.eof()) patternFile >> pattern;
        if(patternFile.eof() | !pattern.size())
        {
            if(digit == 0)  break;
            else
            {
                pattern = resStr;
                cnt--;
            }
        }
        if(pattern.size() != I)
        {
            cout << "\nError: Pattern(" << pattern << ") length(" << pattern.size() << ") does not match the number of inputs(" << I << ") in a circuit!!\n";
            cnt = 0;
            break;
        }
        if(pattern.find_first_not_of("01") != string::npos)
        {
            cout << "\nError: Pattern(" << pattern << ") contains a non-0/1 character('" << pattern[pattern.find_first_not_of("01")] << "').\n";
            cnt = 0;
            break;
        }
        simulate(pattern);
        for(size_t j = 0; j < A; ++j)
            if(_aigList[j]->_sim)
                _aigList[j]->_simResults += (1 << digit);
        cnt++;
        digit++;
//        cout << pattern << "!" << cnt << "\n";
        if(digit == 16)
        {
            genFECPairs();
            updateValue();
//            for(size_t i = 0; i < A; ++i)
//                cout << toBinary(_aigList[i]->_simResults, 16) << endl;
            for(size_t i = 0; i < A; ++i)
                _aigList[i]->_simResults = 0;
            digit = 0;
        }
        if((digit == 0) & (patternFile.eof())) break;
    }
//    for(size_t i = 0; i < A; ++i)
//        cout << toBinary(_aigList[i]->_simResults, 16) << endl;
//    genFECPairs();
    //        cerr << noNewPairGen << endl;
    if(fecPairs.front()->size() == (_aigList.size() + 1) * 2) fecPairs.pop();
    cout << cnt << " patterns simulated.\n";
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

void
CirMgr::simulate(string& pattern)
{
    const0->_sim = false;
    for(size_t i = 0; i < I; ++i)
        _piList[i]->_sim = (pattern[i] == '1');
    
//    string results = "";
//    for(size_t i = 0; i < O; ++i)
//        results += "0";
    
    CirGate::_globalRef++;
    for(size_t i = 0; i < O; ++i)
        _poList[i]->simulateByDFS();
    
//    return toDecimal(results);
}

void
CirMgr::genFECPairs()
{
    unordered_map<size_t, vector<unsigned>*> m1;
//    unordered_map<unsigned, size_t> m2;
    size_t n = fecPairs.size();
    
    vector<unsigned>* v;
    for(size_t i = 0; i < n; ++i)
    {
        v = fecPairs.front();
        fecPairs.pop();
//        if((*v)[0] / 2 == (*fecPairs.front())[0] / 2) fecPairs.pop();
        m1.clear();
        for(size_t j = 0; j < v->size(); ++j)
        {
//            cout << (*v)[j] << " " << getResults((*v)[j]) << endl;
            if(!m1[getResults((*v)[j])])
            {
                if(m1[(1 << 16) - 1 - getResults((*v)[j])]) continue;
                m1[getResults((*v)[j])] = new vector<unsigned>;
                m1[getResults((*v)[j])]->push_back((*v)[j]);
                fecPairs.push(m1[getResults((*v)[j])]);
            }
            else
            {
                m1[getResults((*v)[j])]->push_back((*v)[j]);
//                for(size_t k = 0; k < m1[getResults((*v)[j])]->size(); ++k)
//                    cout << (*m1[getResults((*v)[j])])[k] << " ";
//                cout << endl;
            }
            getGate((*v)[j] / 2)->_fecGroup = m1[getResults((*v)[j])];
            getGate((*v)[j] / 2)->_invFec = ((*v)[j] % 2 == 1);
        }
        deleteDupVec();
        deleteSingle();
    }
}

size_t
CirMgr::getResults(unsigned n)
{
    bool inverse = false;
    if(n % 2 == 1) inverse = true;
    n = n / 2;
    size_t t = getGate(n)->_simResults;
    if(inverse) t = (1 << 16) - 1 - t;
    return t;
}

void
CirMgr::deleteDupVec()
{
    size_t n = fecPairs.size();
    unordered_map<size_t, int*> m;
    vector<unsigned>* v;
    for(size_t i = 0; i < n; ++i)
    {
        v = fecPairs.front();
        fecPairs.pop();
        if(!m[(*v)[0] / 2])
        {
            m[(*v)[0] / 2] = new int;
            fecPairs.push(v);
        }
    }
}

void
CirMgr::deleteSingle()
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
}

void
CirMgr::updateValue()
{
    string v1, v2;
    for(size_t i = 0; i < _aigList.size(); ++i)
    {
        v1 = toBinary(_aigList[i]->_simResults, 16);
        v2 = v1.substr(8);
        v1 = v1.substr(0, 8);
//        cout << v1 << " " << v2 << endl;
        _aigList[i]->_value.push(v1);
        _aigList[i]->_value.push(v2);
        _aigList[i]->_value.pop();
        _aigList[i]->_value.pop();
    }
    for(size_t i = 0; i < _piList.size(); ++i)
    {
        v1 = toBinary(_piList[i]->_simResults, 16);
        v2 = v1.substr(8);
        v1 = v1.substr(0, 8);
//        cout << v1 << " " << v2 << endl;
        _piList[i]->_value.push(v1);
        _piList[i]->_value.push(v2);
        _piList[i]->_value.pop();
        _piList[i]->_value.pop();
    }
    for(size_t i = 0; i < _poList.size(); ++i)
    {
        v1 = toBinary(_poList[i]->_simResults, 16);
        v2 = v1.substr(8);
        v1 = v1.substr(0, 8);
//        cout << v1 << " " << v2 << endl;
        _poList[i]->_value.push(v1);
        _poList[i]->_value.push(v2);
        _poList[i]->_value.pop();
        _poList[i]->_value.pop();
    }
}
