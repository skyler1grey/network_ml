/*
 * test_fangcha.cpp
 *
 *  Created on: Jul 14, 2018
 *      Author: root
 */
#include <iostream>
#include <vector>
#include <algorithm>
#include "decode.hpp"

using namespace std;

int mainfda()
{
	vector<int> temp;
	temp.push_back(1);
	temp.push_back(2);
	temp.push_back(3);
	cout<<fangcha(temp)<<endl;
}
