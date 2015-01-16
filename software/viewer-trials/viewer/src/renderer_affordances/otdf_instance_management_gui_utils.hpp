#ifndef OTDF_INSTANCE_MANAGEMENT_GUI_UTILS_HPP
#define OTDF_INSTANCE_MANAGEMENT_GUI_UTILS_HPP
#include "renderer_affordances.hpp"
#include "lcm_utils.hpp"

using namespace std;
using namespace boost;
using namespace visualization_utils;
using namespace collision;
using namespace renderer_affordances;
using namespace renderer_affordances_lcm_utils;

namespace renderer_affordances_gui_utils
{


    static gboolean on_popup_close (GtkButton* button, GtkWidget* pWindow)
    {
        gtk_widget_destroy (pWindow);
        return TRUE;
    }
  
    // ================================================================================
    // Second Stage Popup of OTDF instance management for Adjust Params and Dofs using Spinboxes and Sliders
    //------------------------------------------------------------------
    // PARAM adjust popup management


    //------------------
    static void on_adjust_params_popup_close (BotGtkParamWidget *pw, void *user)
    {

        RendererAffordances *self = (RendererAffordances*) user;
        //    string instance_name=  (self->instance_selection_ptr);
        //    typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        //    object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);
    
        typedef map<string, double > params_mapType;
        //for( params_mapType::const_iterator it2 = it->second._otdf_instance->params_map_.begin(); it2!=it->second._otdf_instance->params_map_.end(); it2++)
        for( params_mapType::const_iterator it2 = self->otdf_instance_hold._otdf_instance->params_map_.begin(); it2!=self->otdf_instance_hold._otdf_instance->params_map_.end(); it2++) 
        { 
            double t = bot_gtk_param_widget_get_double (pw,it2->first.c_str());
            //cout << it->first << ": " << t << endl;
            //it->second._otdf_instance->setParam(it2->first, t);
            self->otdf_instance_hold._otdf_instance->setParam(it2->first, t);
        }
        // regen kdl::tree and reset fksolver
        // regen link tfs and shapes for display
        //update_OtdfInstanceStruc(it->second);
        self->otdf_instance_hold._otdf_instance->update();
        self->otdf_instance_hold._gl_object->set_state(self->otdf_instance_hold._otdf_instance);
        self->selection_hold_on=false;
   
        //was AFFORDANCE_FIT
        //string otdf_type = string(self->otdf_instance_hold.otdf_type);
        self->affCollection->publish_otdf_instance_to_affstore("AFFORDANCE_TRACK",(self->otdf_instance_hold.otdf_type),self->otdf_instance_hold.uid,self->otdf_instance_hold._otdf_instance); 
        bot_viewer_request_redraw(self->viewer);
    }  
    //------------------
    static void on_otdf_adjust_param_widget_changed(BotGtkParamWidget *pw, const char *name,void *user)
    {

        RendererAffordances *self = (RendererAffordances*) user;
        //    string instance_name=  (self->instance_selection_ptr);
        //    typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        //    object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);
    
        typedef map<string, double > params_mapType;
        //for( params_mapType::const_iterator it2 = it->second._otdf_instance->params_map_.begin(); it2!=it->second._otdf_instance->params_map_.end(); it2++)
        for( params_mapType::const_iterator it2 = self->otdf_instance_hold._otdf_instance->params_map_.begin(); it2!=self->otdf_instance_hold._otdf_instance->params_map_.end(); it2++)
        { 
            if(!strcmp(name, it2->first.c_str())) {
                double t = bot_gtk_param_widget_get_double (pw,it2->first.c_str());
                //cout << it->first << ": " << t << endl;
                //it->second._otdf_instance->setParam(it2->first, t);
                self->otdf_instance_hold._otdf_instance->setParam(it2->first, t);
            }
        }
    
        self->otdf_instance_hold._otdf_instance->update(); //update_OtdfInstanceStruc(it->second);
        self->otdf_instance_hold._gl_object->set_state(self->otdf_instance_hold._otdf_instance);
        bot_viewer_request_redraw(self->viewer);// gives realtime feedback of the geometry changing.
    }
  
  
    //------------------------------------------------------------------
    // DOF adjust popup management
    static void on_adjust_dofs_popup_close (BotGtkParamWidget *pw, void *user)
    {
        RendererAffordances *self = (RendererAffordances*) user;
        string instance_name=  self->instance_selection;
        self->affCollection->publish_otdf_instance_to_affstore("AFFORDANCE_TRACK",(self->otdf_instance_hold.otdf_type),self->otdf_instance_hold.uid,self->otdf_instance_hold._otdf_instance); 
        self->selection_hold_on=false;
        bot_viewer_request_redraw(self->viewer);
  
    }
    //------------------------------------------------------------------ 
    static void on_otdf_adjust_dofs_widget_changed(BotGtkParamWidget *pw, const char *name,void *user)
    {
        RendererAffordances *self = (RendererAffordances*) user;

        // get desired state from popup sliders
        KDL::Frame T_world_object = self->otdf_instance_hold._gl_object->_T_world_body;
        map<string, double> jointpos_in;
      
        typedef map<string,boost::shared_ptr<otdf::Joint> > joints_mapType;
        for (joints_mapType::iterator joint = self->otdf_instance_hold._otdf_instance->joints_.begin();joint != self->otdf_instance_hold._otdf_instance->joints_.end(); joint++) {     
            double dof_pos = 0;
            if(joint->second->type!=(int) otdf::Joint::FIXED) {
                dof_pos =  bot_gtk_param_widget_get_double (pw, joint->first.c_str())*(M_PI/180);
                jointpos_in.insert(make_pair(joint->first, dof_pos)); 
                self->otdf_instance_hold._otdf_instance->setJointState(joint->first, dof_pos,0);
                cout <<  joint->first << " DOF changed to " << dof_pos*(180/M_PI) << endl;
            }
        }

        self->otdf_instance_hold._gl_object->set_state(T_world_object,jointpos_in); 
        bot_viewer_request_redraw(self->viewer);
    }
  
  
    //------------------------------------------------------------------  
    static void on_otdf_manip_map_dof_range_widget_popup_close (BotGtkParamWidget *pw, void *user)
    {
        RendererAffordances *self = (RendererAffordances*) user;
        string instance_name=  self->instance_selection;
        typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);
    
        self->popup_widget_name_list.clear();
        it->second._gl_object->set_future_state_changing(false);
        it->second._gl_object->disable_future_display();
        // reset object to what it was at the time of popup
   
        it->second.otdf_instance_viz_object_sync = true;
        it->second._gl_object->set_state(it->second._otdf_instance);
        //it->second._gl_object->set_state(self->otdf_T_world_body_hold,self->otdf_current_jointpos_hold);
        it->second._gl_object->set_future_state(self->otdf_T_world_body_hold,self->otdf_current_jointpos_hold);
        cout << "dof_range_widget popup close\n";

        //map<string, vector<KDL::Frame> > ee_frames_map;
        //map<string, vector<drc::affordance_index_t> > ee_frame_affindices_map;
        if(self->doBatchFK) {
            //doBatchFK given DOF Desired Ranges
            int num_of_increments = 30; // determines no of intermediate holds between dof_min and dof_max;
            self->dofRangeFkQueryHandler->doBatchFK(self->dof_names, self->dof_min, self->dof_max, num_of_increments);
        } 
        
        self->ee_frames_map.clear();
        self->ee_frame_affindices_map.clear();
        // get ee dof range constraints for associated sticky hands and feet
        string aff_name=it->first;
        self->stickyHandCollection->get_aff_indexed_ee_constraints(aff_name,it->second,self->dofRangeFkQueryHandler,self->ee_frames_map,self->ee_frame_affindices_map);
        self->stickyFootCollection->get_aff_indexed_ee_constraints(aff_name,it->second,self->dofRangeFkQueryHandler,self->ee_frames_map,self->ee_frame_affindices_map);

        string channel  = "DESIRED_MANIP_MAP_EE_LOCI"; 
        publish_aff_indexed_traj_opt_constraint(channel, self->ee_frames_map, self->ee_frame_affindices_map, self);
    }
    
    //------------------------------------------------------------------    
    static void on_otdf_manip_map_dof_range_widget_changed(BotGtkParamWidget *pw, const char *name,void *user)
    {
        RendererAffordances *self = (RendererAffordances*) user;
        string instance_name=  self->instance_selection;
        typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);
    
        // get desired dof ranges from popup sliders

        vector<string> dof_names;
        vector<double> dof_min,dof_max;
        map<string, double> current_jointpos_in;
        map<string, double> future_jointpos_in; 
        
       
    
        bool doFK= false;
        self->doBatchFK = false;
        // Build up a map of the desired joint position ranges
        typedef map<string,boost::shared_ptr<otdf::Joint> > joints_mapType;
        for (joints_mapType::iterator joint = it->second._otdf_instance->joints_.begin();joint != it->second._otdf_instance->joints_.end(); joint++) {     
            if(joint->second->type!=(int) otdf::Joint::FIXED) {
                double current_dof_vel = 0;
                double current_dof_pos = 0;
                it->second._otdf_instance->getJointState (joint->first, current_dof_pos, current_dof_vel);
                string temp1 = joint->first + "_MIN"; 
                string temp2 = joint->first +"_MAX";   
                vector<string>::const_iterator found1,found2;
                found1 = find (self->popup_widget_name_list.begin(), self->popup_widget_name_list.end(), temp1);
                found2 = find (self->popup_widget_name_list.begin(), self->popup_widget_name_list.end(), temp2);
                if ((found1 != self->popup_widget_name_list.end()) && (found2 != self->popup_widget_name_list.end())) {
                    unsigned int index1 = found1 - self->popup_widget_name_list.begin();      
                    unsigned int index2 = found2 - self->popup_widget_name_list.begin(); 
                    double desired_dof_pos_min = 0;
                    double desired_dof_pos_max = 0;
                    if (joint->second->type == otdf::Joint::REVOLUTE) {
                        desired_dof_pos_min =  bot_gtk_param_widget_get_double (pw, self->popup_widget_name_list[index1].c_str())*(M_PI/180);
                        desired_dof_pos_max =  bot_gtk_param_widget_get_double (pw, self->popup_widget_name_list[index2].c_str())*(M_PI/180);
                    }
                    else {
                        desired_dof_pos_min =  bot_gtk_param_widget_get_double (pw, self->popup_widget_name_list[index1].c_str());
                        desired_dof_pos_max =  bot_gtk_param_widget_get_double (pw, self->popup_widget_name_list[index2].c_str());
                    }

                    current_jointpos_in.insert(make_pair(joint->first, desired_dof_pos_min)); 
                    future_jointpos_in.insert(make_pair(joint->first, desired_dof_pos_max)); 
                    //if((current_dof_pos>(desired_dof_pos_min+1e-2)) && (current_dof_pos<(desired_dof_pos_max-1e-2))) {
                    if((current_dof_pos>(desired_dof_pos_min-1e-4)) && (current_dof_pos<(desired_dof_pos_max+1e-4))) {
                        if (joint->second->type == otdf::Joint::REVOLUTE)
                            cout <<  joint->first << ":: desired dof range set to " << desired_dof_pos_min*(180/M_PI) << " : "<< desired_dof_pos_max*(180/M_PI)<< "(rad)" << endl;
                        else
                            cout <<  joint->first << ":: desired dof range set to " << desired_dof_pos_min << " : "<< desired_dof_pos_max<< endl;

                        dof_names.push_back(joint->first);
                        dof_min.push_back(desired_dof_pos_min);
                        dof_max.push_back(desired_dof_pos_max);
                        doFK = true;
                    }
                }  
            }// end if
        }// end for
        self->dof_names.clear();self->dof_min.clear();self->dof_max.clear();
        self->dof_names=dof_names;
        self->dof_min=dof_min;
        self->dof_max=dof_max;
        /*if(doFK) {
            //doBatchFK given DOF Desired Ranges
            //int num_of_increments = 10; // determines no of intermediate holds between dof_min and dof_max;
            int num_of_increments = 5; // determines no of intermediate holds between dof_min and dof_max;
            self->dofRangeFkQueryHandler->doBatchFK(dof_names, dof_min, dof_max, num_of_increments);
        } */

        // Visualizing how seeds change with dof range changes
        //--------------------------------------
        if(!it->second._gl_object->is_future_state_changing()) {
            it->second._gl_object->set_future_state_changing(true);
        }      
        fprintf (stdout, "Visualizing range for manip map\n");
        KDL::Frame T_world_object = it->second._gl_object->_T_world_body;
        it->second.otdf_instance_viz_object_sync = false;
        it->second._gl_object->set_state(T_world_object,current_jointpos_in);  
        it->second._gl_object->set_future_state(T_world_object,future_jointpos_in);
        bot_viewer_request_redraw(self->viewer);
    }  

  
    //------------------------------------------------------------------
    static void spawn_adjust_params_popup (RendererAffordances *self)
    {

        GtkWidget *window, *close_button, *vbox;
        BotGtkParamWidget *pw;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(self->viewer->window));
        gtk_window_set_modal(GTK_WINDOW(window), FALSE);
        gtk_window_set_decorated  (GTK_WINDOW(window),FALSE);
        gtk_window_stick(GTK_WINDOW(window));
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
        gtk_window_set_default_size(GTK_WINDOW(window), 300, 250);
        gint pos_x, pos_y;
        gtk_window_get_position(GTK_WINDOW(window),&pos_x,&pos_y);
        pos_x+=125;    pos_y-=75;
        gtk_window_move(GTK_WINDOW(window),pos_x,pos_y);
        //gtk_widget_set_size_request (window, 300, 250);
        //gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_title(GTK_WINDOW(window), "Adjust Params");
        gtk_container_set_border_width(GTK_CONTAINER(window), 5);
        pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

        string instance_name=  self->instance_selection;
        typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);

        for(vector<string>::const_iterator it2 = it->second._otdf_instance->params_order_.begin(); it2 != it->second._otdf_instance->params_order_.end(); ++it2) 
            {
                double inc = it->second._otdf_instance->param_properties_map_[*it2].inc;
                double min = it->second._otdf_instance->param_properties_map_[*it2].min_value;
                double max = it->second._otdf_instance->param_properties_map_[*it2].max_value;
                double value = it->second._otdf_instance->params_map_[*it2];
                bot_gtk_param_widget_add_double(pw, (*it2).c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX,
                                                min, max, inc, value); 
            }

        // create a temp copy of the selected otdf instance to make modifications to.    
        if(!self->selection_hold_on) { // Assuming only one object instance is changed at any given time
            self->otdf_instance_hold.uid=it->second.uid;
            self->otdf_instance_hold.otdf_type = it->second.otdf_type;
            self->otdf_instance_hold._otdf_instance = otdf::duplicateOTDFInstance(it->second._otdf_instance);
            self->otdf_instance_hold._gl_object.reset();
            self->otdf_instance_hold._collision_detector.reset();
            self->otdf_instance_hold._collision_detector = boost::shared_ptr<Collision_Detector>(new Collision_Detector());     
            self->otdf_instance_hold._gl_object = boost::shared_ptr<InteractableGlKinematicBody>(new InteractableGlKinematicBody(self->otdf_instance_hold._otdf_instance,self->otdf_instance_hold._collision_detector,true,"otdf_instance_hold"));
            self->otdf_instance_hold._otdf_instance->update();
            self->otdf_instance_hold._gl_object->set_state(self->otdf_instance_hold._otdf_instance);
            self->otdf_instance_hold._gl_object->triangles = it->second._gl_object->triangles;
            self->otdf_instance_hold._gl_object->points = it->second._gl_object->points;
            self->otdf_instance_hold._gl_object->isShowMeshSelected = it->second._gl_object->isShowMeshSelected;
            self->selection_hold_on=true;
        }
        

        g_signal_connect(G_OBJECT(pw), "changed", G_CALLBACK(on_otdf_adjust_param_widget_changed), self);


        close_button = gtk_button_new_with_label ("Close");
        g_signal_connect (G_OBJECT (close_button),
                          "clicked",
                          G_CALLBACK (on_popup_close),
                          (gpointer) window);
        g_signal_connect(G_OBJECT(pw), "destroy",
                         G_CALLBACK(on_adjust_params_popup_close), self); 


        vbox = gtk_vbox_new (FALSE, 3);
        gtk_box_pack_end (GTK_BOX (vbox), close_button, FALSE, FALSE, 5);
        gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(pw), FALSE, FALSE, 5);
        gtk_container_add (GTK_CONTAINER (window), vbox);
        gtk_widget_show_all(window); 
    }
    //------------------------------------------------------------------

    static void spawn_adjust_dofs_popup (RendererAffordances *self)
    {

        GtkWidget *window, *close_button, *vbox,*hbox;
        BotGtkParamWidget *pw_L, *pw_R;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(self->viewer->window));
        gtk_window_set_modal(GTK_WINDOW(window), FALSE);
        gtk_window_set_decorated  (GTK_WINDOW(window),FALSE);
        gtk_window_stick(GTK_WINDOW(window));
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
        gtk_window_set_default_size(GTK_WINDOW(window), 500, 250);
        gint pos_x, pos_y;
        gtk_window_get_position(GTK_WINDOW(window),&pos_x,&pos_y);
        pos_x+=125;    pos_y-=75;
        gtk_window_move(GTK_WINDOW(window),pos_x,pos_y);
        //gtk_widget_set_size_request (window, 300, 250);
        //gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_title(GTK_WINDOW(window), "Adjust Current DOFs");
        gtk_container_set_border_width(GTK_CONTAINER(window), 5);
        pw_L = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());
        pw_R = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

        string instance_name=  self->instance_selection;

        typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);

        // Need tracked joint positions of all objects.
        typedef map<string,boost::shared_ptr<otdf::Joint> > joints_mapType;
        for (joints_mapType::iterator joint = it->second._otdf_instance->joints_.begin();joint != it->second._otdf_instance->joints_.end(); joint++) {     
            // Skip certain joints
            if (!joint->first.compare("front_left_steering_joint") || !joint->first.compare("front_left_wheel_joint") ||
                !joint->first.compare("front_right_steering_joint") || !joint->first.compare("front_right_wheel_joint") ||
                !joint->first.compare("rear_left_wheel_joint") || !joint->first.compare("rear_right_wheel_joint"))
                continue;
            string blank = " ";
            double current_dof_velocity = 0;
            double current_dof_position = 0;
            it->second._otdf_instance->getJointState(joint->first,current_dof_position,current_dof_velocity);
            if(joint->second->type!=(int) otdf::Joint::FIXED) { // All joints that not of the type FIXED.
                if(joint->second->type==(int) otdf::Joint::CONTINUOUS) {//BOT_GTK_PARAM_WIDGET_SPINBOX,BOT_GTK_PARAM_WIDGET_SLIDER
                    bot_gtk_param_widget_add_double(pw_L, joint->first.c_str(), BOT_GTK_PARAM_WIDGET_SLIDER, 
                                                    -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .1, current_dof_position*(180/M_PI)); 
                    bot_gtk_param_widget_add_double(pw_R, joint->first.c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX, 
                                                    -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .1, current_dof_position*(180/M_PI)); 
                }
                else if (joint->second->type == (int) otdf::Joint::REVOLUTE)  {
                    bot_gtk_param_widget_add_double(pw_L, joint->first.c_str(), BOT_GTK_PARAM_WIDGET_SLIDER,
                                    joint->second->limits->lower*(180/M_PI), joint->second->limits->upper*(180/M_PI), 
                                                    .1, current_dof_position*(180/M_PI));
                    bot_gtk_param_widget_add_double(pw_R, joint->first.c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX,
                                                    joint->second->limits->lower*(180/M_PI), joint->second->limits->upper*(180/M_PI), 
                                                    .1, current_dof_position*(180/M_PI));
                }
                else {
                    bot_gtk_param_widget_add_double(pw_L, joint->first.c_str(), BOT_GTK_PARAM_WIDGET_SLIDER,
                                    joint->second->limits->lower, joint->second->limits->upper, .1, current_dof_position); 
                    bot_gtk_param_widget_add_double(pw_R, joint->first.c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX,
                                                    joint->second->limits->lower, joint->second->limits->upper, .1, current_dof_position);  
               }     
            }
        }
        //Have to handle joint_patterns separately   
        // DoF of all joints in joint patterns.
        typedef map<string,boost::shared_ptr<otdf::Joint_pattern> > jp_mapType;
        for (jp_mapType::iterator jp_it = it->second._otdf_instance->joint_patterns_.begin();jp_it != it->second._otdf_instance->joint_patterns_.end(); jp_it++) {
            // for all joints in joint pattern.
            for (unsigned int i=0; i < jp_it->second->joint_set.size(); i++) {
                double current_dof_velocity = 0;
                double current_dof_position = 0;
                it->second._otdf_instance->getJointState(jp_it->second->joint_set[i]->name,current_dof_position,current_dof_velocity);
                if(jp_it->second->joint_set[i]->type!=(int) otdf::Joint::FIXED) { // All joints that not of the type FIXED.
                    if(jp_it->second->joint_set[i]->type==(int) otdf::Joint::CONTINUOUS) {
                        bot_gtk_param_widget_add_double(pw_L, jp_it->second->joint_set[i]->name.c_str(), BOT_GTK_PARAM_WIDGET_SLIDER,
                                                        -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .1, current_dof_position*(180/M_PI)); 
                        bot_gtk_param_widget_add_double(pw_R, jp_it->second->joint_set[i]->name.c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX,
                                                        -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .1, current_dof_position*(180/M_PI)); 
                    }
                    
                    else if (jp_it->second->joint_set[i]->type == (int) otdf::Joint::REVOLUTE) {
                        bot_gtk_param_widget_add_double(pw_L, jp_it->second->joint_set[i]->name.c_str(), BOT_GTK_PARAM_WIDGET_SLIDER,
                                                        jp_it->second->joint_set[i]->limits->lower*(180/M_PI), 
                                                        jp_it->second->joint_set[i]->limits->upper*(180/M_PI), .1, current_dof_position*(180/M_PI));
                        bot_gtk_param_widget_add_double(pw_R, jp_it->second->joint_set[i]->name.c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX,
                                                        jp_it->second->joint_set[i]->limits->lower*(180/M_PI), 
                                                        jp_it->second->joint_set[i]->limits->upper*(180/M_PI), .1, current_dof_position*(180/M_PI));
                    }   
                    else {
                        bot_gtk_param_widget_add_double(pw_L, jp_it->second->joint_set[i]->name.c_str(), BOT_GTK_PARAM_WIDGET_SLIDER,
                                                        jp_it->second->joint_set[i]->limits->lower, 
                                                        jp_it->second->joint_set[i]->limits->upper, .1, current_dof_position);
                        bot_gtk_param_widget_add_double(pw_R, jp_it->second->joint_set[i]->name.c_str(), BOT_GTK_PARAM_WIDGET_SPINBOX,
                                                        jp_it->second->joint_set[i]->limits->lower, 
                                                        jp_it->second->joint_set[i]->limits->upper, .1, current_dof_position);
                    } 
                } // end if         
            } // end for all joints in jp
        }// for all joint patterns

                
        // create a temp copy of the selected otdf instance to make modifications to.    
        if(!self->selection_hold_on) { // Assuming only one object instance is changed at any given time
            self->otdf_instance_hold.uid=it->second.uid;
     
            self->otdf_instance_hold.otdf_type = it->second.otdf_type;
            self->otdf_instance_hold._otdf_instance = otdf::duplicateOTDFInstance(it->second._otdf_instance);
            self->otdf_instance_hold._gl_object.reset();
            self->otdf_instance_hold._collision_detector.reset();
            self->otdf_instance_hold._collision_detector = boost::shared_ptr<Collision_Detector>(new Collision_Detector());     
            self->otdf_instance_hold._gl_object = boost::shared_ptr<InteractableGlKinematicBody>(new InteractableGlKinematicBody(self->otdf_instance_hold._otdf_instance,self->otdf_instance_hold._collision_detector,true,"otdf_instance_hold"));
            self->otdf_instance_hold._otdf_instance->update();
            self->otdf_instance_hold._gl_object->set_state(self->otdf_instance_hold._otdf_instance);
            self->otdf_instance_hold._gl_object->triangles = it->second._gl_object->triangles;
            self->otdf_instance_hold._gl_object->points = it->second._gl_object->points;
            self->otdf_instance_hold._gl_object->isShowMeshSelected = it->second._gl_object->isShowMeshSelected;
            self->selection_hold_on=true;
        }
        

        g_signal_connect(G_OBJECT(pw_L), "changed", G_CALLBACK(on_otdf_adjust_dofs_widget_changed), self);
        g_signal_connect(G_OBJECT(pw_R), "changed", G_CALLBACK(on_otdf_adjust_dofs_widget_changed), self);
        
        close_button = gtk_button_new_with_label ("Close");
        g_signal_connect (G_OBJECT (close_button),
                          "clicked",
                          G_CALLBACK (on_popup_close),
                          (gpointer) window);
        g_signal_connect(G_OBJECT(pw_L), "destroy",
                         G_CALLBACK(on_adjust_dofs_popup_close), self); 
        g_signal_connect(G_OBJECT(pw_R), "destroy",
                         G_CALLBACK(on_adjust_dofs_popup_close), self);   
                         
        hbox= gtk_hbox_new (FALSE, 3);
        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET(pw_L), TRUE, TRUE, 5);
        gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET(pw_R), FALSE, FALSE, 5);
        
        vbox = gtk_vbox_new (FALSE, 3);
        gtk_box_pack_end (GTK_BOX (vbox), close_button, FALSE, FALSE, 5);
        gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(hbox), FALSE, FALSE, 5);
        gtk_container_add (GTK_CONTAINER (window), vbox);
        gtk_widget_show_all(window); 
    }
    //------------------------------------------------------------------

    static void spawn_set_manip_map_dof_range_popup (RendererAffordances *self)
    {

        GtkWidget *window, *close_button, *vbox;
        BotGtkParamWidget *pw;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(self->viewer->window));
        gtk_window_set_modal(GTK_WINDOW(window), FALSE);
        gtk_window_set_decorated  (GTK_WINDOW(window),FALSE);
        gtk_window_stick(GTK_WINDOW(window));
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
        gtk_window_set_default_size(GTK_WINDOW(window), 300, 250);
        gint pos_x, pos_y;
        gtk_window_get_position(GTK_WINDOW(window),&pos_x,&pos_y);
        pos_x+=125;    pos_y-=75;
        gtk_window_move(GTK_WINDOW(window),pos_x,pos_y);
        //gtk_widget_set_size_request (window, 300, 250);
        //gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_title(GTK_WINDOW(window), "Adjust DOFs");
        gtk_container_set_border_width(GTK_CONTAINER(window), 5);
        pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

        string instance_name=  self->instance_selection;
        //self->popup_widget_name_list.clear(); 

        typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
        object_instance_map_type_::iterator it = self->affCollection->_objects.find(instance_name);

        // Need tracked joint positions of all objects.
        typedef map<string,boost::shared_ptr<otdf::Joint> > joints_mapType;
        for (joints_mapType::iterator joint = it->second._otdf_instance->joints_.begin();joint != it->second._otdf_instance->joints_.end(); joint++) {     
            double current_dof_velocity = 0;
            double current_dof_position = 0;
            it->second._otdf_instance->getJointState(joint->first,current_dof_position,current_dof_velocity);
            if(joint->second->type!=(int) otdf::Joint::FIXED) { // All joints that not of the type FIXED.

                // Skip certain joints
                if (!joint->first.compare("front_left_steering_joint") || !joint->first.compare("front_left_wheel_joint") ||
                    !joint->first.compare("front_right_steering_joint") || !joint->first.compare("front_right_wheel_joint") ||
                    !joint->first.compare("rear_left_wheel_joint") || !joint->first.compare("rear_right_wheel_joint"))
                    continue;

                if(joint->second->type==(int) otdf::Joint::CONTINUOUS) {
                    self->popup_widget_name_list.push_back(joint->first+"_MIN"); 
                    bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                    BOT_GTK_PARAM_WIDGET_SLIDER, -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .01, 
                                                    current_dof_position*(180/M_PI)); 
                    self->popup_widget_name_list.push_back(joint->first+"_MAX");   
                    bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                    BOT_GTK_PARAM_WIDGET_SLIDER, -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .01, 
                                                    current_dof_position*(180/M_PI)); 
                    bot_gtk_param_widget_add_separator (pw," ");
                }
                else if (joint->second->type == (int) otdf::Joint::REVOLUTE) {
                    self->popup_widget_name_list.push_back(joint->first+"_MIN"); 
                    bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                    BOT_GTK_PARAM_WIDGET_SLIDER,joint->second->limits->lower*(180/M_PI), 
                                                    joint->second->limits->upper*(180/M_PI), .01, current_dof_position*(180/M_PI));
                    self->popup_widget_name_list.push_back(joint->first+"_MAX"); 
                    bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                    BOT_GTK_PARAM_WIDGET_SLIDER,joint->second->limits->lower*(180/M_PI), 
                                                    joint->second->limits->upper*(180/M_PI), .01, current_dof_position*(180/M_PI));
                    bot_gtk_param_widget_add_separator (pw," ");
                }   
                else {
                    self->popup_widget_name_list.push_back(joint->first+"_MIN"); 
                    bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                    BOT_GTK_PARAM_WIDGET_SLIDER,joint->second->limits->lower, 
                                                    joint->second->limits->upper, .005, current_dof_position);
                    self->popup_widget_name_list.push_back(joint->first+"_MAX"); 
                    bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                    BOT_GTK_PARAM_WIDGET_SLIDER,joint->second->limits->lower, 
                                                    joint->second->limits->upper, .005, current_dof_position);
                    bot_gtk_param_widget_add_separator (pw," ");
                }
            }
        }
        //Have to handle joint_patterns separately   
        // DoF of all joints in joint patterns.
        typedef map<string,boost::shared_ptr<otdf::Joint_pattern> > jp_mapType;
        for (jp_mapType::iterator jp_it = it->second._otdf_instance->joint_patterns_.begin();jp_it != it->second._otdf_instance->joint_patterns_.end(); jp_it++) {
            // for all joints in joint pattern.
            for (unsigned int i=0; i < jp_it->second->joint_set.size(); i++) {
                double current_dof_velocity = 0;
                double current_dof_position = 0;
                it->second._otdf_instance->getJointState(jp_it->second->joint_set[i]->name,current_dof_position,current_dof_velocity);
                if(jp_it->second->joint_set[i]->type!=(int) otdf::Joint::FIXED) { // All joints that not of the type FIXED.
                    if(jp_it->second->joint_set[i]->type==(int) otdf::Joint::CONTINUOUS) {          
                        self->popup_widget_name_list.push_back(jp_it->second->joint_set[i]->name+"_MIN");
                        bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                        BOT_GTK_PARAM_WIDGET_SLIDER, -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .01, 
                                                        current_dof_position*(180/M_PI)); 
                        self->popup_widget_name_list.push_back(jp_it->second->joint_set[i]->name+"_MAX");  
                        bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                        BOT_GTK_PARAM_WIDGET_SLIDER, -2*M_PI*(180/M_PI), 2*M_PI*(180/M_PI), .01, 
                                                        current_dof_position*(180/M_PI)); 
                        //bot_gtk_param_widget_add_separator (pw," ");
                    }
                    else if (jp_it->second->joint_set[i]->type==(int) otdf::Joint::REVOLUTE) {
                        self->popup_widget_name_list.push_back(jp_it->second->joint_set[i]->name+"_MIN");  
                        bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                        BOT_GTK_PARAM_WIDGET_SLIDER, jp_it->second->joint_set[i]->limits->lower*(180/M_PI), 
                                                        jp_it->second->joint_set[i]->limits->upper*(180/M_PI), .01, current_dof_position*(180/M_PI));
                        self->popup_widget_name_list.push_back(jp_it->second->joint_set[i]->name+"_MAX"); 
                        bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                        BOT_GTK_PARAM_WIDGET_SLIDER, jp_it->second->joint_set[i]->limits->lower*(180/M_PI), 
                                                        jp_it->second->joint_set[i]->limits->upper*(180/M_PI), .01, current_dof_position*(180/M_PI));
                        //bot_gtk_param_widget_add_separator (pw," ");
                    }   
                    else {
                        self->popup_widget_name_list.push_back(jp_it->second->joint_set[i]->name+"_MIN");  
                        bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                        BOT_GTK_PARAM_WIDGET_SLIDER, jp_it->second->joint_set[i]->limits->lower, 
                                                        jp_it->second->joint_set[i]->limits->upper, .01, current_dof_position);
                        self->popup_widget_name_list.push_back(jp_it->second->joint_set[i]->name+"_MAX"); 
                        bot_gtk_param_widget_add_double(pw, self->popup_widget_name_list[self->popup_widget_name_list.size()-1].c_str(), 
                                                        BOT_GTK_PARAM_WIDGET_SLIDER, jp_it->second->joint_set[i]->limits->lower, 
                                                        jp_it->second->joint_set[i]->limits->upper, .01, current_dof_position);
                        //bot_gtk_param_widget_add_separator (pw," ");
                    }
                } // end if         
            } // end for all joints in jp
        }// for all joint patterns
        
    
        // store current object state (will be restored on popup close).
        self->otdf_T_world_body_hold=it->second._gl_object->_T_world_body;
        self->otdf_current_jointpos_hold=it->second._gl_object->_current_jointpos;    
    
        self->dofRangeFkQueryHandler.reset();
        int num_of_increments = 30; // preallocates spaces for speed
        self->dofRangeFkQueryHandler=boost::shared_ptr<BatchFKQueryHandler>(new BatchFKQueryHandler(it->second._otdf_instance,num_of_increments));

        g_signal_connect(G_OBJECT(pw), "changed", G_CALLBACK(on_otdf_manip_map_dof_range_widget_changed), self);


        close_button = gtk_button_new_with_label ("Close");
        g_signal_connect (G_OBJECT (close_button),
                          "clicked",
                          G_CALLBACK (on_popup_close),
                          (gpointer) window);
        g_signal_connect(G_OBJECT(pw), "destroy",
                         G_CALLBACK(on_otdf_manip_map_dof_range_widget_popup_close), self); 


        vbox = gtk_vbox_new (FALSE, 3);
        gtk_box_pack_end (GTK_BOX (vbox), close_button, FALSE, FALSE, 5);
        gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(pw), FALSE, FALSE, 5);
        gtk_container_add (GTK_CONTAINER (window), vbox);
        gtk_widget_show_all(window); 
    }




    // ================================================================================
    // First Stage Popup of OTDF instance management

    static void on_otdf_instance_management_widget_changed(BotGtkParamWidget *pw, const char *name,void *user)
    {
        RendererAffordances *self = (RendererAffordances*) user;

        // int selection = bot_gtk_param_widget_get_enum (pw, PARAM_OTDF_INSTANCE_SELECT);
        const char *instance_name;
        instance_name = bot_gtk_param_widget_get_enum_str( pw, PARAM_OTDF_INSTANCE_SELECT );
        self->instance_selection  = string(instance_name);

        if(!strcmp(name,PARAM_OTDF_ADJUST_PARAM)) {
            spawn_adjust_params_popup(self);
        }
        else if(!strcmp(name,PARAM_OTDF_ADJUST_DOF)) {
            spawn_adjust_dofs_popup(self);
        }
        else if(!strcmp(name,PARAM_OTDF_FLIP_PITCH)) {
          typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
          object_instance_map_type_::iterator it = self->affCollection->_objects.find(string(instance_name));
          if(it!=self->affCollection->_objects.end()){
            double p = it->second._otdf_instance->getParam("pitch");
            p = (p>0)?(p-M_PI):(p+M_PI); // flip 180
            it->second._otdf_instance->setParam("pitch", p);
            it->second._otdf_instance->update(); //update_OtdfInstanceStruc(it->second);
            it->second._gl_object->set_state(it->second._otdf_instance);
            self->affCollection->publish_otdf_instance_to_affstore("AFFORDANCE_TRACK",(it->second.otdf_type),it->second.uid,it->second._otdf_instance); 
            bot_viewer_request_redraw(self->viewer);// gives realtime feedback of the geometry changing.
          }else cout << "Can't find: " << name << endl;

        }
        else if(!strcmp(name,PARAM_OTDF_INSTANCE_CLEAR)) {
            fprintf(stderr,"\nClearing Selected Instance\n");
        
            typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
            object_instance_map_type_::iterator it = self->affCollection->_objects.find(string(instance_name));
      
            if(it!=self->affCollection->_objects.end())
            {
                self->affCollection->delete_otdf_from_affstore("AFFORDANCE_FIT", it->second.otdf_type, it->second.uid);

                self->affCollection->_objects.erase(it);
                if(self->object_selection==string(instance_name))
                {
                    self->link_selection = " ";
                    self->object_selection = " ";
                }  

                typedef map<string, StickyHandStruc > sticky_hands_map_type_;
   
                sticky_hands_map_type_::iterator hand_it = self->stickyHandCollection->_hands.begin();
                while (hand_it!=self->stickyHandCollection->_hands.end()) {
                    string hand_name = string(hand_it->second.object_name);
                    if (hand_name == string(instance_name))
                    {
                        if(self->stickyhand_selection==hand_it->first){
							self->seedSelectionManager->remove(self->stickyhand_selection);
                            self->stickyhand_selection = " ";
                            
                        }
                        self->stickyHandCollection->_hands.erase(hand_it++);
                    }
                    else
                        hand_it++;
                } 

                typedef map<string, StickyFootStruc > sticky_feet_map_type_;
   
                sticky_feet_map_type_::iterator foot_it = self->stickyFootCollection->_feet.begin();
                while (foot_it!=self->stickyFootCollection->_feet.end()) {
                    string foot_name = string(foot_it->second.object_name);
                    if (foot_name == string(instance_name))
                    {
                        if(self->stickyfoot_selection==foot_it->first)
                        {
						 self->seedSelectionManager->remove(self->stickyfoot_selection);                            
						 self->stickyfoot_selection = " ";
                           
                        }
                        self->stickyFootCollection->_feet.erase(foot_it++);
                    }
                    else
                        foot_it++;
                } 

                bot_viewer_request_redraw(self->viewer);
            }
        }
        else if(!strcmp(name,PARAM_OTDF_INSTANCE_CLEAR_ALL)) {
            fprintf(stderr,"\nClearing Instantiated Objects\n");
            typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
            self->affCollection->_objects.clear();
            self->stickyHandCollection->_hands.clear();
            for( map<string,int >::iterator it = self->instance_cnt.begin(); it!=self->instance_cnt.end(); it++) { 
                it->second = 0;
            }
            self->link_selection = " ";
            self->object_selection = " ";
            self->stickyhand_selection = " ";
            self->stickyfoot_selection = " ";
            self->seedSelectionManager->clear();
            
            bot_viewer_request_redraw(self->viewer);
        }
    }

    static void spawn_instance_management_popup (RendererAffordances *self)
    {
        GtkWidget *window, *close_button, *vbox;
        BotGtkParamWidget *pw; 

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(self->viewer->window));
        gtk_window_set_modal(GTK_WINDOW(window), FALSE);
        gtk_window_set_decorated  (GTK_WINDOW(window),FALSE);
        gtk_window_stick(GTK_WINDOW(window));
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
        gtk_window_set_default_size(GTK_WINDOW(window), 300, 250);
        gint pos_x, pos_y;
        gtk_window_get_position(GTK_WINDOW(window),&pos_x,&pos_y);
        pos_x+=125;    pos_y-=75;
        gtk_window_move(GTK_WINDOW(window),pos_x,pos_y);
        gtk_window_set_title(GTK_WINDOW(window), "Instance Management");
        gtk_container_set_border_width(GTK_CONTAINER(window), 5);
        pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

        int num_otdf_instances;
        char ** otdf_instance_names;
        int * otdf_instance_nums;

        if( self->affCollection->_objects.size() > 0) {
            num_otdf_instances = self->affCollection->_objects.size();
            otdf_instance_names =(char **) calloc(num_otdf_instances, sizeof(char *));
            otdf_instance_nums = (int *)calloc(num_otdf_instances, sizeof(int));
            
            unsigned int i = 0;  
            typedef map<string, OtdfInstanceStruc > object_instance_map_type_;
            for( object_instance_map_type_::const_iterator it = self->affCollection->_objects.begin(); it!=self->affCollection->_objects.end(); it++) { 
                string instance_name = it->first;
                otdf_instance_names[i] = (char *) instance_name.c_str();
                otdf_instance_nums[i] =i;
                // cout << "Instance:  " << instance_name  << " i: "<< i << endl;
                // self->instantiated_objects_id.insert(make_pair(i,instance_name));
                ++i;
            }
        }
        else {
            num_otdf_instances = 1;
            otdf_instance_names =(char **) calloc(num_otdf_instances, sizeof(char *));
            otdf_instance_nums = (int *)calloc(num_otdf_instances, sizeof(int));
            
            string instance_name = "No objects Instantiated";
            otdf_instance_names[0]= (char *) instance_name.c_str();
            otdf_instance_nums[0] =0; 
        }

        bot_gtk_param_widget_add_enumv (pw, PARAM_OTDF_INSTANCE_SELECT, BOT_GTK_PARAM_WIDGET_MENU, 
                                        0,
                                        num_otdf_instances,
                                        (const char **)  otdf_instance_names,
                                        otdf_instance_nums);

        if( self->affCollection->_objects.size() > 0) {
                bot_gtk_param_widget_add_buttons(pw,PARAM_OTDF_ADJUST_PARAM, NULL);
                bot_gtk_param_widget_add_buttons(pw,PARAM_OTDF_ADJUST_DOF, NULL);
                bot_gtk_param_widget_add_buttons(pw,PARAM_OTDF_FLIP_PITCH, NULL);
                bot_gtk_param_widget_add_buttons(pw,PARAM_OTDF_INSTANCE_CLEAR, NULL);
                bot_gtk_param_widget_add_buttons(pw,PARAM_OTDF_INSTANCE_CLEAR_ALL, NULL);
        }

        g_signal_connect(G_OBJECT(pw), "changed", G_CALLBACK(on_otdf_instance_management_widget_changed), self);

        close_button = gtk_button_new_with_label ("Close");
        g_signal_connect (G_OBJECT (close_button),
                          "clicked",
                          G_CALLBACK (on_popup_close),
                          (gpointer) window);

        vbox = gtk_vbox_new (FALSE, 3);
        gtk_box_pack_end (GTK_BOX (vbox), close_button, FALSE, FALSE, 5);

        gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(pw), FALSE, FALSE, 5);
        gtk_container_add (GTK_CONTAINER (window), vbox);
        gtk_widget_show_all(window);
    }

}
#endif