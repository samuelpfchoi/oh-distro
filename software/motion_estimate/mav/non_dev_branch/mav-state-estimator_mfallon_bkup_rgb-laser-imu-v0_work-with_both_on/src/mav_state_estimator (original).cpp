#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

#include <lcm/lcm.h>
#include <bot_core/bot_core.h>
#include <bot_frames/bot_frames.h>
#include <bot_param/param_client.h>
#include <bot_lcmgl_client/lcmgl.h>

#include <lcmtypes/mav_pose_t.h>
#include <lcmtypes/mav_ins_t.h>
#include <lcmtypes/mav_gps_data_t.h>
#include <lcmtypes/mav_filter_state_t.h>
#include <lcmtypes/bot_core_rigid_transform_t.h>
#include <lcmtypes/mav_optical_flow_t.h>

#include <Eigen/Dense>

#include <mav_state_est/rbis.hpp>
#include <mav_state_est/mav_state_est.hpp>
#include <mav_state_est/sensor_handlers.hpp>
#include <eigen_utils/eigen_utils.hpp>

#include <mav_state_est/gpf/rbis_gpf_update.hpp>
#include <mav_state_est/Initializer.hpp>

#include <getopt.h>
#include <ConciseArgs>

using namespace std;
using namespace Eigen;
using namespace eigen_utils;

class App {
public:
  //--------------------LCM stuff-----------------------
  lcm_t * lcm_pub;
  lcm_t * lcm_recv;
  BotParam * param;
  BotFrames * frames;

  string pose_channel;
  string filter_state_channel;

  bool publish_filter_state;

  string laser_channel;

  ViconHandler * vicon_handler;
  InsHandler * ins_handler;
  GpsHandler * gps_handler;
  LaserGPFHandler * laser_gpf_handler;
  IndexedMeasurementHandler * laser_gpf_separate_process_handler;
  ScanMatcherHandler * scan_matcher_handler;
  OpticalFlowHandler * optical_flow_handler;

  bool filter_initialized;
  MavStateEstimator * state_estimator;
  int64_t history_span;

  bool no_republish;
  bool smooth_at_end;

  char init_mode;
  Initializer * initializer;

};

static void vicon_message_handler(const lcm_recv_buf_t *rbuf, const char * channel,
    const bot_core_rigid_transform_t * msg, void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->vicon_handler == NULL) {
    return;
  }

  RBISUpdateInterface * vicon_update = self->vicon_handler->processMessage(msg);

  if (vicon_update != NULL) {
    self->state_estimator->addUpdate(vicon_update);
  }

  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    //republish the vicon for the log
    bot_core_rigid_transform_t_publish(self->lcm_pub, channel, msg);
  }
}

static void ins_message_handler(const lcm_recv_buf_t *rbuf, const char * channel, const mav_ins_t * msg, void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->ins_handler == NULL) {
    return;
  }

  bot_tictoc("rec_to_send");
  RBISUpdateInterface * update = self->ins_handler->processMessage(msg);
  if (update != NULL) {
    self->state_estimator->addUpdate(update);
  }

  RBIS state;
  RBIM cov;
  self->state_estimator->getHeadState(state, cov);

  if (!self->smooth_at_end) {
    //publish the pose msg
    rigid_body_pose_t pose_msg;
    state.getPose(&pose_msg);
    pose_msg.utime = msg->utime;
    rigid_body_pose_t_publish(self->lcm_pub, self->pose_channel.c_str(), &pose_msg);
    bot_tictoc("rec_to_send");

    //publish filter state
    if (self->publish_filter_state) {
      mav_filter_state_t * fs_msg = rbisCreateFilterStateMessage(state, cov);
      fs_msg->utime = msg->utime;
      mav_filter_state_t_publish(self->lcm_pub, self->filter_state_channel.c_str(), fs_msg);
      mav_filter_state_t_destroy(fs_msg);
    }
  }

  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    mav_ins_t_publish(self->lcm_pub, channel, msg);
  }

}

static void gps_message_handler(const lcm_recv_buf_t *rbuf, const char * channel, const mav_gps_data_t * msg,
    void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->gps_handler == NULL) {
    return;
  }
  //require 3d lock to do anything with gps
  if (msg->gps_lock != 3) {
    fprintf(stderr, "gps lock is %d, discarding\n", msg->gps_lock); //todo: don't spew?
    return;
  }
  RBISUpdateInterface * update = self->gps_handler->processMessage(msg);
  if (update != NULL) {
    self->state_estimator->addUpdate(update);
  }

  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    mav_gps_data_t_publish(self->lcm_pub, channel, msg);
  }

}

static void laser_message_handler(const lcm_recv_buf_t *rbuf, const char * channel, const bot_core_planar_lidar_t * msg,
    void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->laser_gpf_handler == NULL) {
    return;
  }
  RBISUpdateInterface * update = self->laser_gpf_handler->processMessage(msg);
  if (update != NULL) {
    self->state_estimator->addUpdate(update);
  }

  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    bot_core_planar_lidar_t_publish(self->lcm_pub, channel, msg);
  }
}

static void laser_gpf_message_handler(const lcm_recv_buf_t *rbuf, const char * channel,
    const mav_indexed_measurement_t * msg, void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->laser_gpf_separate_process_handler == NULL) {
    return;
  }

  RBISUpdateInterface * update = self->laser_gpf_separate_process_handler->processMessage(msg);

  if (update != NULL) {
    self->state_estimator->addUpdate(update);
  }
  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    mav_indexed_measurement_t_publish(self->lcm_pub, channel, msg);
  }
}

static void scan_matcher_handler(const lcm_recv_buf_t *rbuf, const char * channel, const bot_core_pose_t * msg,
    void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->scan_matcher_handler == NULL) {
    return;
  }

  RBISUpdateInterface * update = self->scan_matcher_handler->processMessage(msg);

  if (update != NULL) {
    self->state_estimator->addUpdate(update);
  }

  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    bot_core_pose_t_publish(self->lcm_pub, channel, msg);
  }
}

static void optical_flow_handler(const lcm_recv_buf_t *rbuf, const char * channel, const mav_optical_flow_t * msg,
    void * user)
{
  App * self = (App *) user;
  if (self->state_estimator == NULL || self->optical_flow_handler == NULL) {
    return;
  }

  RBISUpdateInterface * update = self->optical_flow_handler->processMessage(msg);

  if (update != NULL) {
    self->state_estimator->addUpdate(update);
  }

  if (self->lcm_pub != self->lcm_recv && !self->no_republish) {
    mav_optical_flow_t_publish(self->lcm_pub, channel, msg);
  }
}

static void create_filter(App * self, const RBIS & init_state, const RBIM &init_cov, int64_t utime)
{
  RBISResetUpdate * init_filter_state = new RBISResetUpdate(init_state, init_cov, RBISResetUpdate::reset, utime);
  self->state_estimator = new MavStateEstimator(init_filter_state, self->history_span);
  self->filter_initialized = true;
}

static void filter_init_handler(const lcm_recv_buf_t *rbuf, const char * channel, const mav_filter_state_t * msg,
    void * user)
{
  App * self = (App *) user;
  if (self->filter_initialized) {
    fprintf(stderr, "WARNING: received initialization message when filter was already initialized!!\n");
    return; //TODO: add option allowing reinitialization?
  }

  Map<const RBIM> init_cov(msg->cov);
  Map<const RBIS::VectorNd> state_vec_map(msg->state);
  Quaterniond quat;
  botDoubleToQuaternion(quat, msg->quat);
  RBIS init_state(state_vec_map, quat);

  create_filter(self, init_state, init_cov, msg->utime);
}

static void shutdown_module(int unused __attribute__((unused)))
{
  fprintf(stderr, "shutting down!\n");
  bot_tictoc_print_stats(BOT_TICTOC_AVG);
  exit(1);
}

int main(int argc, char **argv)
{

  signal(SIGINT, shutdown_module);
//  Need this environment variable: export BOT_TICTOC = 1

  App * self = new App();

  string in_log_fname;
  string out_log_fname;
  string param_file;
  string override_str;
  bool verbose = false;
  string output_likelihood_filename;
  self->no_republish = false;
  self->init_mode = 'm';
  self->smooth_at_end = false;

  ConciseArgs opt(argc, argv);
  opt.add(in_log_fname, "L", "in_log_name", "Run state estimation directly on this log");
  opt.add(out_log_fname, "l", "out_log_name", "publish into this log");
  opt.add(param_file, "P", "param_file", "Pull params from this file instead of LCM");
  opt.add(override_str, "O", "override_params",
      "Override the parameters in BotParam with a '|' seperated list of key=value pairs");
  opt.add(param_file, "v", "verbose");
  opt.add(self->no_republish, "R", "no_republish", "Don't republish messagse for visualization");
  opt.add(self->init_mode, "i", "init", "Init mode: 'm'-message, 'o'-origin, 'g'-gps, 'v'-vicon ");
  opt.add(self->smooth_at_end, "S", "smooth",
      "Run Kalman smoothing and publish poses at end - only works when running from log");
  opt.add(output_likelihood_filename, "M", "meas_like", "save the measurement likelihood to this file");
  opt.parse();

  bool running_from_log = !in_log_fname.empty();
  bool running_to_log = !out_log_fname.empty();
  if (self->smooth_at_end && !running_from_log) {
    fprintf(stderr, "must specify in log if in smoothing mode\n");
    opt.usage(true);
  }

  if (opt.wasParsed("L") && in_log_fname == out_log_fname) {
    fprintf(stderr, "must specify different logname for output %s!\n", out_log_fname.c_str());
    exit(1);
  }

  if (self->smooth_at_end && !self->no_republish) {
    fprintf(stderr, "warning: republishing will put messages out of order when smoothing\n");
  }

//---------------------------INITIALIZE APP--------------------------------------------
  bot_gauss_rand_init(time(NULL)); //randomize for particles

  if (running_from_log) {
    printf("running from log file: %s\n", in_log_fname.c_str());
    string lcmurl = "file://" + in_log_fname + "?speed=0";
    self->lcm_recv = lcm_create(lcmurl.c_str());
    if (self->lcm_recv == NULL) {
      fprintf(stderr, "Error couldn't load log file %s\n", lcmurl.c_str());
      exit(1);
    }
  }
  else {
    self->lcm_recv = bot_lcm_get_global(NULL);
  }

  if (running_to_log) {
    printf("publishing into log file: %s\n", out_log_fname.c_str());
    string lcmurl = "file://" + out_log_fname + "?mode=w";
    self->lcm_pub = lcm_create(lcmurl.c_str());
    if (self->lcm_pub == NULL) {
      fprintf(stderr, "Error couldn't open log file %s\n", lcmurl.c_str());
      exit(1);
    }
  }
  else {
    self->lcm_pub = bot_lcm_get_global(NULL);
  }

  if (param_file.empty()) {
    self->param = bot_param_get_global(self->lcm_pub, 0);
  }
  else {
    self->param = bot_param_new_from_file(param_file.c_str());
  }

  if (self->param == NULL) {
    exit(1);
  }
  else if (!override_str.empty()) {
    int ret = bot_param_override_local_params(self->param, override_str.c_str());
    if (ret <= 0) {
      fprintf(stderr, "Error overriding params with %s\n", override_str.c_str());
      exit(1);
    }
  }

  self->frames = bot_frames_get_global(self->lcm_recv, self->param);

  self->filter_state_channel = bot_param_get_str_or_fail(self->param, "state_estimator.filter_state_channel");
  self->pose_channel = bot_param_get_str_or_fail(self->param, "state_estimator.pose_channel");

  self->publish_filter_state = bot_param_get_boolean_or_fail(self->param, "state_estimator.publish_filter_state");

  self->state_estimator = NULL;
  self->history_span = bot_param_get_double_or_fail(self->param, "state_estimator.history_span") * 1e6;
  if (self->smooth_at_end) {
    //set utime span to 24 hours
    self->history_span = 1e6 * 60 * 60 * 24;
    fprintf(stderr, "overiding history span to %jd (24 hours) so everything is kept for smoothing\n",
        self->history_span);
  }

//setup the message handlers
  self->vicon_handler = NULL;
  self->gps_handler = NULL;
  self->scan_matcher_handler = NULL;
  self->laser_gpf_handler = NULL;
  self->laser_gpf_separate_process_handler = NULL;
  self->optical_flow_handler = NULL;

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_ins")) {
    self->ins_handler = new InsHandler(self->param, self->frames);
    cout << "Expecting ins messages on channel " << self->ins_handler->channel << endl;
  }

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_vicon")) {
    self->vicon_handler = new ViconHandler(self->param);
    cout << "Expecting Vicon messages on channel " << self->vicon_handler->channel << endl;
  }

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_gps")) {
    self->gps_handler = new GpsHandler(self->param);
    cout << "Expecting gps messages on channel " << self->gps_handler->channel << endl;
  }

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_scan_matcher")) {
    self->scan_matcher_handler = new ScanMatcherHandler(self->param);
    cout << "Expecting scan_matcher pose messages on channel " << self->scan_matcher_handler->channel << endl;
  }

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_laser_gpf")) {
    self->laser_gpf_handler = new LaserGPFHandler(self->lcm_pub, self->param, self->frames);
    cout << "Expecting laser messages on channel " << self->laser_gpf_handler->laser_channel << endl;
  }

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_laser_gpf_separate_process")) {
    self->laser_gpf_separate_process_handler = new IndexedMeasurementHandler(self->param, "laser_gpf");
    self->laser_gpf_separate_process_handler->downsample_factor = 1; //we need to do this laser_gpf does the downsampling out of process
    cout << "Expecting laser_gpf updates on channel " << self->laser_gpf_separate_process_handler->channel << endl;
  }

  if (bot_param_get_boolean_or_fail(self->param, "state_estimator.use_optical_flow")) {
    self->optical_flow_handler = new OpticalFlowHandler(self->param, self->frames);
    cout << "Expecting optical flow messages on channel " << self->optical_flow_handler->channel << endl;
  }

//----------------------------------------------------------------------------

  bool internal_initialization = running_from_log && self->init_mode != 'm';

  if (internal_initialization) {
    fprintf(stderr, "running from log, with non-message initialization, will initialize internally\n");
  }
  switch (self->init_mode) {
  case 'o':
    self->initializer = new Initializer(self->lcm_recv, self->param, self->frames, Initializer::origin_init,
        internal_initialization);
    break;
  case 'g':
    self->initializer = new Initializer(self->lcm_recv, self->param, self->frames, Initializer::gps_init,
        internal_initialization);
    break;
  case 'v':
    self->initializer = new Initializer(self->lcm_recv, self->param, self->frames, Initializer::vicon_init,
        internal_initialization);
    break;
  case 'm':
    break;
  default:
    fprintf(stderr, "Invalid initialization mode!\n");
    opt.usage(true);
  }

//subscribe to the initialization message, and wait for it
  mav_filter_state_t_subscription_t * init_sub = NULL;
  if (!internal_initialization) {
    init_sub = mav_filter_state_t_subscribe(self->lcm_recv, "MAV_STATE_EST_INITIALIZER", filter_init_handler,
        (void *) self);
  }
  self->filter_initialized = false;
  fprintf(stderr, "waiting for initialization message\n");
  while (!self->filter_initialized) {
    lcm_handle(self->lcm_recv);

    if (internal_initialization) {
      if (self->initializer->done) {
        filter_init_handler(NULL, "INTERNAL_INITIALIZATION", self->initializer->init_msg, (void *) self);
      }
    }
  }
  fprintf(stderr, "initialization done!\n");
//unsubscribe so we don't accidentally receive another one!
  if (!internal_initialization) {
    mav_filter_state_t_unsubscribe(self->lcm_recv, init_sub);
  }
  //subscribe to sensor messages
  if (self->ins_handler != NULL) {
    mav_ins_t_subscribe(self->lcm_recv, self->ins_handler->channel.c_str(), ins_message_handler, (void *) self);
  }

  if (self->vicon_handler != NULL) {
    bot_core_rigid_transform_t_subscribe(self->lcm_recv, self->vicon_handler->channel.c_str(), vicon_message_handler,
        (void *) self);
  }

  if (self->gps_handler != NULL) {
    mav_gps_data_t_subscribe(self->lcm_recv, self->gps_handler->channel.c_str(), gps_message_handler, (void *) self);
  }

  if (self->laser_gpf_handler != NULL) {
    bot_core_planar_lidar_t_subscribe(self->lcm_recv, self->laser_gpf_handler->laser_channel.c_str(),
        laser_message_handler, (void *) self);
  }

  if (self->laser_gpf_separate_process_handler != NULL) {
    mav_indexed_measurement_t_subscribe(self->lcm_recv, self->laser_gpf_separate_process_handler->channel.c_str(),
        laser_gpf_message_handler, (void *) self);
  }

  if (self->scan_matcher_handler != NULL) {
    bot_core_pose_t_subscribe(self->lcm_recv, self->scan_matcher_handler->channel.c_str(), scan_matcher_handler,
        (void *) self);
  }

  if (self->optical_flow_handler != NULL) {
    mav_optical_flow_t_subscribe(self->lcm_recv, self->optical_flow_handler->channel.c_str(), optical_flow_handler,
        (void *) self);
  }

  while (true) {
    int ret = lcm_handle(self->lcm_recv);
    if (ret != 0) {
      printf("log is done\n");
      break;
    }
  }

  if (self->smooth_at_end) {
    fprintf(stderr, "performing smoothing backwards pass\n");
    self->state_estimator->EKFSmoothBackwardsPass(self->ins_handler->dt);

    updateHistory::historyMapIterator current_it;

    fprintf(stderr, "republishing smoothed pose messages\n");
    for (current_it = self->state_estimator->history.updateMap.begin();
        current_it != self->state_estimator->history.updateMap.end(); current_it++) {
      RBISUpdateInterface * current_update = current_it->second;
      if (current_update->sensor_id == RBISUpdateInterface::ins) {
        RBIS state = current_update->posterior_state;
        RBIM cov = current_update->posterior_covariance;

        //publish the pose msg
        rigid_body_pose_t pose_msg;
        state.getPose(&pose_msg);
        pose_msg.utime = current_it->first;
        rigid_body_pose_t_publish(self->lcm_pub, self->pose_channel.c_str(), &pose_msg);

        //publish filter state
        if (self->publish_filter_state) {
          mav_filter_state_t * fs_msg = rbisCreateFilterStateMessage(state, cov);
          fs_msg->utime = current_it->first;
          mav_filter_state_t_publish(self->lcm_pub, self->filter_state_channel.c_str(), fs_msg);
          mav_filter_state_t_destroy(fs_msg);
        }
      }
    }
  }

  double log_likelihood = self->state_estimator->getMeasurementsLogLikelihood();
  fprintf(stderr, "total measurement log likelihood: %f\n", log_likelihood);
  if (!output_likelihood_filename.empty()) {
    Eigen::Matrix<double, 1, 1> log_likelihood_mat; //FIXME set to 2x2 so the write/read in eigen_utils works
    log_likelihood_mat.setConstant(log_likelihood);
    eigen_utils::writeToFile(output_likelihood_filename, log_likelihood_mat);
  }

// TODO: cleanup
  printf("all_done!\n");
//_nv012tls() bug related to nvidia drivers, see http://www.nvnews.net/vbulletin/showthread.php?t=164619
  return 0;
}