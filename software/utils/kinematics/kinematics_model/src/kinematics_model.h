#ifndef KINEMATICS_MODEL_KINEMATICS_MODEL_H
#define KINEMATICS_MODEL_KINEMATICS_MODEL_H

#include <iostream>
#include <string>

#include <kdl/tree.hpp>
#include "lcmtypes/drc/position_3d_t.hpp"
#include "lcmtypes/drc/transform_t.hpp"

namespace kinematics {
  class Kinematics_Model {
  public:
    Kinematics_Model();
    Kinematics_Model( const Kinematics_Model& other );
    ~Kinematics_Model();

    static void drc_position_3d_t_to_kdl_frame( const drc::position_3d_t& position3d, KDL::Frame& frame );
    static void drc_transform_t_to_kdl_frame( const drc::transform_t& transform, KDL::Frame& frame );
    static void kdl_frame_to_drc_transform_t( const KDL::Frame& frame, drc::transform_t& transform );
    static void kdl_frame_to_drc_position_3d_t( const KDL::Frame& frame, drc::position_3d_t& position3d );    

  protected:
   
  private:
  
  };
  std::ostream& operator<<( std::ostream& out, const Kinematics_Model& other );
}

#endif /* KINEMATICS_MODEL_KINEMATICS_MODEL */
