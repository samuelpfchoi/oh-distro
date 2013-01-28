/*
 * ManipulatorState.h
 *
 *  Created on: Jan 23, 2013
 *      Author: mfleder
 */

#ifndef MANIPULATOR_STATE_H
#define MANIPULATOR_STATE_H

#include "affordance/ModelState.h"
#include "urdf/model.h"

namespace affordance
{

  /**Represents that state of a manipulator*/
  class ManipulatorState : public ModelState<ManipulatorState>
  {
    //-------------fields----
  private: 
    const std::string _name;
    const GlobalUID _guid;
    const boost::shared_ptr<const urdf::Link> _link;

    //-----------constructor/destructor
  public:
    ManipulatorState(boost::shared_ptr<const urdf::Link> link, 
		     const GlobalUID &uid);
    ManipulatorState(const std::string &s, const GlobalUID &uid);

    //ManipulatorState(const ManipulatorState &other); //todo
    //ManipulatorState& operator=( const ManipulatorState& rhs ); //todo
    virtual ~ManipulatorState();
    
    //-------------------observers
  public:
    //ModelState interface
    virtual GlobalUID getGlobalUniqueId() const;
    virtual std::string getName() const;

    virtual Eigen::Vector3f getColor() const;

    virtual Eigen::Vector3f getXYZ() const;
    virtual Eigen::Vector3f getRPY() const; 

    virtual bool isAffordance() const ;
    virtual bool isManipulator() const;
    virtual bool hasChildren() const; //any
    virtual bool hasParent() const; //1 or more
    virtual void getChildren(std::vector<boost::shared_ptr<const ManipulatorState> > &children) const;
    virtual void getParents(std::vector<boost::shared_ptr<const ManipulatorState> > &children) const;
    virtual void getCopy(ManipulatorState &copy) const;
    
    //specific to this class
  public:
    boost::shared_ptr<const urdf::Link> getLink() const;
    std::string getGUIDAsString()  const;

  };
  
  std::ostream& operator<<( std::ostream& out, const ManipulatorState& other );
  
  typedef boost::shared_ptr<ManipulatorState> ManipulatorStatePtr;
  typedef boost::shared_ptr<const ManipulatorState> ManipulatorStateConstPtr;
  
} //namespace affordance

#endif /* MANIPULATOR_STATE_H */
