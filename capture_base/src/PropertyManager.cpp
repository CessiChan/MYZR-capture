/*
 * PropertyManager.cpp
 *
 *  Created on: 2016-4-1
 *      Author: WJ
 */

#include "PropertyManager.h"
#include <fstream>
#include "global.h"

namespace wj {

PropertyManager::PropertyManager() {
	// TODO Auto-generated constructor stub

}

PropertyManager::~PropertyManager() {
	// TODO Auto-generated destructor stub
}


void PropertyManager::updateLastCaptureLocation(string path){
//	printLog("path = %s \r\n", path.c_str());
	ofstream fout( PROPERTY_FILE );
	fout << path << endl; // fout�÷���coutһ��, ������д���ļ�����ȥ

}

string PropertyManager::getLastCaputreLocation(){

	ifstream ifs( PROPERTY_FILE );
	string str;
	int i = 0;
	while(getline(ifs, str))
	{
		return str;
	}

	return "";
}

} /* namespace wj */
