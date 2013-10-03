function atlasStandingLegTuning
%NOTEST

% simple function for tuning position and torque control gains
% joint-by-joint while standing using position control

% gain spec: 
% q, qd, f are sensed position, velocity, torque, from AtlasJointState
%
% q_d, qd_d, f_d are desired position, velocity, torque, from
% AtlasJointDesired
%
% The final joint command will be:
%
%  k_q_p   * ( q_d - q ) +
%  k_q_i   * 1/s * ( q_d - q ) +
%  k_qd_p  * ( qd_d - qd ) +
%  k_f_p   * ( f_d - f ) +
%  ff_qd   * qd +
%  ff_qd_d * qd_d +
%  ff_f_d  * f_d +
%  ff_const


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% SET JOINT/MOVEMENT PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
joint = 'l_leg_hpz';% <---- 
control_mode = 'force';% <----  force, position
signal = 'chirp';% <----  zoh, foh, chirp

% SIGNAL PARAMS %%%%%%%%%%%%%
dim = 3; % what spatial dimension to move COM: x/y/z (1/2/3)
if strcmp( signal, 'chirp' )
  zero_crossing = false;
  ts = linspace(0,40,500);% <----
  amp = -0.085;% <---- meters, COM DELTA
  freq = linspace(0.025,0.1,500);% <----  cycles per second
else
  vals = 0.02*[0 0 1 0 0];% <---- meters, COM DELTA
  ts = linspace(0,30,length(vals));% <----
end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
T=ts(end);


% load robot model
r = Atlas();
load(strcat(getenv('DRC_PATH'),'/control/matlab/data/atlas_fp.mat'));
r = removeCollisionGroupsExcept(r,{'toe','heel'});
r = compile(r);
r = r.setInitialState(xstar);

% setup frames
state_frame = getStateFrame(r);
state_frame.subscribe('EST_ROBOT_STATE');
input_frame = getInputFrame(r);
ref_frame = AtlasPosTorqueRef(r);

nu = getNumInputs(r);
nq = getNumDOF(r);

joint_index_map = struct(); % maps joint names to indices
for i=1:nq
  joint_index_map.(state_frame.coordinates{i}) = i;
end

act_idx_map = getActuatedJoints(r);
gains = getAtlasGains(input_frame); % change gains in this file

joint_index = joint_index_map.(joint);
joint_input_index = find(act_idx_map==joint_index_map.(joint));

% zero out force gains to start --- move to nominal joint position
gains.k_f_p = zeros(nu,1);
gains.ff_f_d = zeros(nu,1);
gains.ff_qd = zeros(nu,1);
ref_frame.updateGains(gains);

% move to fixed point configuration 
qdes = xstar(1:nq);
atlasLinearMoveToPos(qdes,state_frame,ref_frame,act_idx_map,3);

gains2 = getAtlasGains(input_frame); 
% reset force gains for joint being tuned
gains.k_f_p(joint_input_index) = gains2.k_f_p(joint_input_index); 
gains.ff_f_d(joint_input_index) = gains2.ff_f_d(joint_input_index);
gains.ff_qd(joint_input_index) = gains2.ff_qd(joint_input_index);
% set joint position gains to 0 for joint being tuned
gains.k_q_p(joint_input_index) = 0;
gains.k_q_i(joint_input_index) = 0;
gains.k_qd_p(joint_input_index) = 0;

ref_frame.updateGains(gains);

% compute desired COM trajectory
x0 = r.getInitialState(); 
q0 = x0(1:nq);
com0 = getCOM(r,q0);
comtraj = ConstantTrajectory(com0);

if strcmp(signal,'zoh')
  input_traj = PPTrajectory(zoh(ts,vals));
elseif strcmp(signal,'foh')
  input_traj = PPTrajectory(foh(ts,vals));
elseif strcmp(signal,'chirp')
  offset = 0;
  if strcmp(control_mode,'position')
    offset=joint_offset_map.(joint);
  end
  if zero_crossing
  	input_traj = PPTrajectory(foh(ts, offset + amp*sin(ts.*freq*2*pi)));
  else
    input_traj = PPTrajectory(foh(ts, offset + 0.5*amp - 0.5*amp*cos(ts.*freq*2*pi)));
  end
else
  error('unknown signal');
end

if dim==1
  comtraj = comtraj + [input_traj;0;0];
elseif dim==2
  comtraj = comtraj + [0;input_traj;0];
else
  comtraj = comtraj + [0;0;input_traj];
end

% get foot positions
kinsol = doKinematics(r,q0);
rfoot_body = r.findLinkInd('r_foot');
lfoot_body = r.findLinkInd('l_foot');

rfoot0 = forwardKin(r,kinsol,rfoot_body,[0;0;0]);
lfoot0 = forwardKin(r,kinsol,lfoot_body,[0;0;0]);

cost = Point(r.getStateFrame,1);
cost.base_x = 0;
cost.base_y = 0;
cost.base_z = 0;
cost.base_roll = 1000;
cost.base_pitch = 1000;
cost.base_yaw = 1000;
cost.back_bkz = 10;
cost.back_bky = 100;
cost.back_bkx = 100;
cost.r_arm_usy = 10;
cost.r_arm_shx = 10;
cost.r_arm_ely = 10;
cost.r_arm_elx = 10;
cost.r_arm_uwy = 10;
cost.r_arm_mwx = 10;
cost.l_arm_usy = 10;
cost.l_arm_shx = 10;
cost.l_arm_ely = 10;
cost.l_arm_elx = 10;
cost.l_arm_uwy = 10;
cost.l_arm_mwx = 10;

cost = double(cost);
ikoptions = IKoptions(r);
ikoptions = ikoptions.setQ(diag(cost(1:nq)));

v = r.constructVisualizer;
for i=1:length(ts)
  t = ts(i);
  if (i>1)
    kc_com = constructPtrWorldCoMConstraintmex(r.getMexModelPtr,comtraj.eval(t),comtraj.eval(t));
    rfarg = {constructPtrWorldPositionConstraintmex(r.getMexModelPtr,rfoot_body,[0;0;0],rfoot0,rfoot0),...
      constructPtrWorldEulerConstraintmex(r.getMexModelPtr,rfoot_body,[0;0;0],[0;0;0])};
    lfarg = {constructPtrWorldPositionConstraintmex(r.getMexModelPtr,lfoot_body,[0;0;0],lfoot0,lfoot0),...
      constructPtrWorldEulerConstraintmex(r.getMexModelPtr,lfoot_body,[0;0;0],[0;0;0])};
    q(:,i) = inverseKin(r,q(:,i-1),q0,kc_com,rfarg{:},lfarg{:},ikoptions);
  else
    q = q0;
  end
  v.draw(t,q(:,i));
end
qtraj = PPTrajectory(spline(ts,q));
qddtraj = fnder(qtraj,2);
keyboard;

% kalman filter params
Hk = [eye(nq) zeros(nq)];
R = 5e-4*eye(nq);
P = eye(2*nq);
x_est = zeros(2*nq,1);

solver_options.outputflag = 0; % not verbose
solver_options.method = 2; % -1=automatic, 0=primal simplex, 1=dual simplex, 2=barrier
solver_options.presolve = 0;
solver_options.bariterlimit = 20; % iteration limit
solver_options.barhomogeneous = 0; % 0 off, 1 on
solver_options.barconvtol = 5e-4;

qp_active_set = [];

float_idx = 1:6;
act_idx = 7:nq;

nc=8; nd=4;
nf = nc*nd; % number of contact force variables
nparams = nq+nf;
Iqdd = zeros(nq,nparams); Iqdd(:,1:nq) = eye(nq);
Ibeta = zeros(nf,nparams); Ibeta(:,nq+(1:nf)) = eye(nf);

lb = [-1e3*ones(nq,1); zeros(nf,1)]; % qddot/contact forces
ub = [ 1e3*ones(nq,1); 500*ones(nf,1)];

xy_offset = [0;0];
udes = zeros(nu,1);
toffset = -1;
tt=-1;
while tt<T
  [x,t] = getNextMessage(state_frame,1);
  if ~isempty(x)
    if toffset==-1
      toffset=t;
      tlast=0;
      xy_offset = x(1:2); % because state estimate will not be 0,0 to start
    end
    tt=t-toffset;
    dt = tt-tlast;
    tlast =tt;

    F = [eye(nq) dt*eye(nq); zeros(nq) eye(nq)];
    Q = 0.3*[dt*eye(nq) zeros(nq); zeros(nq) eye(nq)];
    
    % estimate state
    jprior = F*x_est;
    Pprior = F*P*F' + Q;
    meas_resid = x(1:nq) - Hk*jprior;
    S = Hk*Pprior*Hk' + R;
    K = (P*Hk')/S;
    x_est = jprior + K*meas_resid;
    P = (eye(2*nq) - K*Hk)*Pprior;
    
    q = x_est(1:nq);
    q(1:2) = q(1:2)-xy_offset;
    qd = x_est(nq+(1:nq));
    
    % get desired configuration
    qt = qtraj.eval(tt);
    qdes = qt(act_idx_map);

    qt(6) = q(6); % ignore yaw
    
    % get desired acceleration
    qdddes = qddtraj.eval(tt) + 17.0*(qt-q) - 0.55*qd;
    
    % solve QP to compute inputs 
    kinsol = doKinematics(r,q);
    [H,C,B] = manipulatorDynamics(r,q,qd);

    H_float = H(float_idx,:);
    C_float = C(float_idx);

    H_act = H(act_idx,:);
    C_act = C(act_idx);
    B_act = B(act_idx,:);
    
    [~,~,JB] = contactConstraintsBV(r,kinsol);
    Dbar = [JB{:}];
    Dbar_float = Dbar(float_idx,:);
    Dbar_act = Dbar(act_idx,:);
     
    % constrained dynamics
    Aeq = H_float*Iqdd - Dbar_float*Ibeta;
    beq = -C_float;

%    Ain_{1} = B_act'*(H_act*Iqdd1e-8 - Dbar_act*Ibeta);
%    bin_{1} = -B_act'*C_act + r.umax;
%    Ain_{2} = -Ain_{1};
%    bin_{2} = B_act'*C_act - r.umin;
% 
%    Ain = sparse(vertcat(Ain_{:}));
%    bin = vertcat(bin_{:});
    
    Hqp = 1e-6*eye(nparams);
    Hqp(1:nq,1:nq) = Hqp(1:nq,1:nq)+ eye(nq);
    fqp = -qdddes'*Iqdd;
    
    IR = eye(nparams);  
    lbind = lb>-999;  ubind = ub<999;  % 1e3 was used like inf above... right?
    Ain_fqp = full([-IR(lbind,:); IR(ubind,:)]);
    bin_fqp = [-lb(lbind); ub(ubind)];
    QblkDiag = Hqp;
    Aeq_fqp = full(Aeq);
    % NOTE: model.obj is 2* f for fastQP!!!
    [alpha,info_fqp] = fastQPmex(QblkDiag,fqp,Aeq_fqp,beq,Ain_fqp,bin_fqp,qp_active_set);

    if info_fqp < 0
      model.Q = sparse(Hqp);
      model.obj = 2*fqp;
      model.A = sparse(Aeq);
      model.rhs = beq;
      model.sense = repmat('=',length(beq),1);
      model.lb = lb;
      model.ub = ub;
      result = gurobi(model,solver_options);
      alpha = result.x;
    end
    
    qp_active_set = find(abs(Ain_fqp*alpha - bin_fqp)<1e-6);
    qdd = alpha(1:nq);
    beta = alpha(nq+(1:nf));
    u = B_act'*(H_act*qdd + C_act - Dbar_act*beta);
    u(joint_input_index)
    udes(joint_input_index) = u(joint_input_index);
    
    ref_frame.publish(t,[qdes;udes],'ATLAS_COMMAND');
  end
end

end