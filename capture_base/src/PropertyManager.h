/*
 * PropertyManager.h
 *
 *  Created on: 2016-4-1
 *      Author: WJ
 */

#ifndef PROPERTYMANAGER_H_
#define PROPERTYMANAGER_H_

#include <iostream>
#include <string>

using namespace std;

namespace wj {

class PropertyManager {
public:
	PropertyManager();
	virtual ~PropertyManager();

	static void updateLastCaptureLocation(string path);
	static string getLastCaputreLocation();


};

} /* namespace wj */
#endif /* PROPERTYMANAGER_H_ */
