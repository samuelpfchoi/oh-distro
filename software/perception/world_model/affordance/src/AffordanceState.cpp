/*
 * AffordanceState.cpp
 *
 *  Created on: Jan 13, 2013
 *      Author: mfleder
 */

#include "AffordanceState.h"
#include "boost/assign.hpp"
#include <iostream>

using namespace affordance;
using namespace Eigen;
using namespace boost;
using namespace std;


//--------set common name fields for drc::affordance_t

string AffordanceState::X_NAME 		= "x";
string AffordanceState::Y_NAME 		= "y";
string AffordanceState::Z_NAME 		= "z";
string AffordanceState::ROLL_NAME 	= "roll";
string AffordanceState::PITCH_NAME 	= "pitch";
string AffordanceState::YAW_NAME 	= "yaw";
string AffordanceState::RADIUS_NAME     = "radius";
string AffordanceState::LENGTH_NAME     = "length";
string AffordanceState::WIDTH_NAME      = "width";
string AffordanceState::HEIGHT_NAME     = "height";
string AffordanceState::R_COLOR_NAME  	= "r_color";
string AffordanceState::G_COLOR_NAME  	= "g_color";
string AffordanceState::B_COLOR_NAME  	= "b_color";


/**initialize the idEnumMap*/
unordered_map<int16_t, AffordanceState::OTDF_TYPE> *AffordanceState::initIdEnumMap()
{

  unordered_map<int16_t, OTDF_TYPE> *m = new unordered_map<int16_t, OTDF_TYPE>;
  int16_t c 	   = drc::affordance_t::CYLINDER;
  int16_t lev 	   = drc::affordance_t ::LEVER;
  int16_t box	   = drc::affordance_t::BOX;
  int16_t sphere   = drc::affordance_t::SPHERE;
  (*m)[c]  	   = AffordanceState::CYLINDER;
  (*m)[lev]           = AffordanceState::LEVER;
  (*m)[box]	   = AffordanceState::BOX;
  (*m)[sphere]        = AffordanceState::SPHERE;
  
  int unknown = box + 1;
  if (m->find(unknown) != m->end())
    throw ArgumentException("initToIdEnumMap : investigate");
  (*m)[unknown] = AffordanceState::UNKNOWN;

  return m;
}

const unordered_map<int16_t, AffordanceState::OTDF_TYPE> *AffordanceState::idToEnum = initIdEnumMap();

/**Constructs an AffordanceState from an lcm message.*/
AffordanceState::AffordanceState(const drc::affordance_t *msg) 
{
	initHelper(msg);
}

/**copy constructor by using toMsg and then the constructor*/
AffordanceState::AffordanceState(const AffordanceState &other)
{
	drc::affordance_t msg;
	other.toMsg(&msg);
	initHelper(&msg);
}

/**Constructs an affordance and sets the name, objId,mapid, frame, and color as specified
@param name affordance name
@param unique object id.  must be unique for the map
@param mapId
@param frame transformation in the map
@param rgb color values from [0,1]
*/
AffordanceState::AffordanceState(const string &name,
				 const int &objId, const int &mapId,
				 const KDL::Frame &frame,
				 const Eigen::Vector3f &color)
  : _name(name), _object_id(objId), _map_id(mapId), _otdf_id(UNKNOWN)
{
	//---set xyz roll pitch yaw from the frame
	_params[X_NAME] = frame.p[0];
	_params[Y_NAME] = frame.p[1];
	_params[Z_NAME] = frame.p[2];

	double roll,pitch,yaw;
	frame.M.GetRPY(roll,pitch,yaw);
	_params[ROLL_NAME] 	= roll;
	_params[PITCH_NAME] = pitch;
	_params[YAW_NAME] 	= yaw;

	//------set the color
	_params[R_COLOR_NAME] = color[0];
	_params[G_COLOR_NAME] = color[1];
	_params[B_COLOR_NAME] = color[2];
}

AffordanceState& AffordanceState::operator=( const AffordanceState& rhs )
{
	if (this == &rhs) // protect against invalid self-assignment
		return *this;

	//clear an object state
	_states.clear();
	_params.clear();
	_ptinds.clear();

	//convert to message and then run init
	drc::affordance_t msg;
	rhs.toMsg(&msg);
	initHelper(&msg);

	return *this;
}


/**used by constructor and copy constructor*/
void AffordanceState::initHelper(const drc::affordance_t *msg)
{
	if (_states.size() != 0 || _params.size() != 0 || _ptinds.size() != 0)
		throw ArgumentException("shouldn't call init if these fields aren't empty");

	_map_utime 	= msg->map_utime;
	_map_id 	= msg->map_id;
	_object_id 	= msg->object_id;
	_name 		= msg->name;
	_ptinds 	= msg->ptinds;


	//argument check
	if (idToEnum->find(msg->otdf_id) == idToEnum->end())
	  {
	    printIdToEnumMap();
	    throw InvalidOtdfID(string("not recognized: ") + toStr<short>(msg->otdf_id) 
				+ string("  : name =  ") + msg->name );
	    
	  }

	_otdf_id = idToEnum->find(msg->otdf_id)->second;

	for(uint i = 0; i < msg->nstates; i++)
		_states[msg->state_names[i]] = msg->states[i];

	for (uint i = 0; i < msg->nparams; i++)
		_params[msg->param_names[i]] = msg->params[i];
}

AffordanceState::~AffordanceState()
{
	//
}

void AffordanceState::printIdToEnumMap()
{
  cout << "\n map size = " << idToEnum->size() << endl;
  for(unordered_map<int16_t, OTDF_TYPE>::const_iterator i = idToEnum->begin();
      i != idToEnum->end(); ++i)
    cout << "\n next mapping = ( " << i->first << ", " << i->second << ")" << endl;
}

//------methods-------
/**convert this to a drc_affordacne_t lcm message*/
void AffordanceState::toMsg(drc::affordance_t *msg) const
{
	msg->map_utime 	= _map_utime;
	msg->map_id		= _map_id;
	msg->object_id 	= _object_id;
	msg->otdf_id	= _otdf_id;
	msg->name		= _name;


	unordered_map<string,double>::const_iterator iter;

	//params
	msg->nparams = _params.size();
	for(iter = _params.begin(); iter != _params.end(); ++iter)
	{
		msg->param_names.push_back(iter->first);
		msg->params.push_back(iter->second);
	}

	//states
	msg->nstates = _states.size();
	for(iter = _states.begin(); iter != _states.end(); ++iter)
	{
		msg->state_names.push_back(iter->first);
		msg->states.push_back(iter->second);
	}

	//ptinds
	msg->nptinds = _ptinds.size();
	for(int i = 0; i < _ptinds.size(); i++)
		msg->ptinds.push_back(_ptinds[i]);
}


/**@return x,y,z or throws an exception if any of those are not present*/
Vector3f AffordanceState::getXYZ() const
{
	//defensive checks
	assertContainsKey(_params, X_NAME);
	assertContainsKey(_params, Y_NAME);
	assertContainsKey(_params, Z_NAME);

	//using find method b/c operator[] isn't a const method
	return Vector3f(_params.find(X_NAME)->second,
					_params.find(Y_NAME)->second,
					_params.find(Z_NAME)->second);
}


/**@return true if we have roll/pitch/yaw parameters.  false otherwise*/
bool AffordanceState::hasRPY() const
{
	return (_params.find(ROLL_NAME) != _params.end() &&
			_params.find(PITCH_NAME) != _params.end() &&
			_params.find(YAW_NAME) != _params.end());
}

/**@return roll,pitch,yaw or 0,0,0 none of those are not present*/
Vector3f AffordanceState::getRPY() const
{
  //using find method b/c operator[] isn't a const method
  return hasRPY()
    ? Vector3f(_params.find(ROLL_NAME)->second,
	       _params.find(PITCH_NAME)->second,
	       _params.find(YAW_NAME)->second)
    : Vector3f(0,0,0);
}


/**@return radius or throws exception if not present*/
double AffordanceState::radius() const
{
	assertContainsKey(_params, RADIUS_NAME);
	return _params.find(RADIUS_NAME)->second;
}

/**@return length or throws exception if not present*/
double AffordanceState::length() const
{
	assertContainsKey(_params, LENGTH_NAME);
	return _params.find(LENGTH_NAME)->second;
}

/**@return width or throws exception if not present*/
double AffordanceState::width() const
{
	assertContainsKey(_params, WIDTH_NAME);
	return _params.find(WIDTH_NAME)->second;
}


/**@return height or throws exception if not present*/
double AffordanceState::height() const
{
	assertContainsKey(_params, HEIGHT_NAME);
	return _params.find(HEIGHT_NAME)->second;
}


void AffordanceState::assertContainsKey(const unordered_map<string, double> &map,
					   	   	   	   	   	const string &key)
{
	if (map.find(key) == map.end())
		throw KeyNotFoundException("Key = " + key + " not found");
}

template <class T> string AffordanceState::toStr(T t)
{
  stringstream s;
  s << t;
  return s.str();
}

string AffordanceState::toStrFromMap(unordered_map<string,double> m)
{
	stringstream s;
	for(unordered_map<string,double>::const_iterator it = m.begin();
	    it != m.end(); ++it)
	{
		s << "\t(" << it->first << ", "
 		  << it->second << ")\n";
	}
	return s.str();
}


namespace affordance
{
	/**operator << */
	ostream& operator<<(ostream& out, const AffordanceState& other )
	{
		out << "=====Affordance " << other._name << "========" << endl;
		out << "(mapId, objectId, otdfId) = (" << other._map_id << ", "
			  << other._object_id << ", " << other._otdf_id << ")\n";
		out << "------params: \n" << AffordanceState::toStrFromMap(other._params) << endl;;
		out << "------states: \n" << AffordanceState::toStrFromMap(other._states) << endl;
		return out;
	}



//=============================================
//=============================================
//=============================================
//=============================================
//================Model State API==============
//=============================================
//=============================================
//=============================================
GlobalUID AffordanceState::getGlobalUniqueId() const
{
  return GlobalUID(_map_id, _object_id);
}

string AffordanceState::getGUIDAsString()  const 
{
    return toStr(getGlobalUniqueId().first) + "," +  toStr(getGlobalUniqueId().second);
}

string AffordanceState:: getName() const
{
  return _name;
}

Vector3f AffordanceState::getColor() const
{
  return Vector3f(_params.find(R_COLOR_NAME)->second,
		  _params.find(G_COLOR_NAME)->second,
		  _params.find(B_COLOR_NAME)->second);
}
  

bool AffordanceState::isAffordance() const 
{  return true; }

bool AffordanceState::isManipulator() const 
{  return false; }

bool AffordanceState::hasChildren() const 
{
  throw NotImplementedException("aff state");
  return true; 
}

bool AffordanceState::hasParent() const 
{
  throw NotImplementedException("aff state");
  return true; 
}

void AffordanceState::getChildren(vector<AffConstPtr> &children) const 
{
  throw NotImplementedException("aff state");
}

void AffordanceState::getParents(vector<AffConstPtr> &parents) const 
{
  throw NotImplementedException("aff state");
}

void AffordanceState::getCopy(AffordanceState &copy) const 
{
  throw NotImplementedException("aff state");
}


} //namespace affordance

