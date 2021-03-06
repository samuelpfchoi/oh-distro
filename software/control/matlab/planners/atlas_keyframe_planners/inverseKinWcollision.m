function [q,info,infeasible_constraint] = inverseKinWcollision(obj,collision_status,q_seed,q_nom,varargin)
% The interface is the same as inverseKin
% @param collision_status    - 0, no validation, no optimization with collision
%                            - 1, validation only, no optimization
%                            - 2, optimization with collision constraint
if(isa(varargin{end},'IKoptions'))
  ikoptions = varargin{end};
  varargin = varargin(1:end-1);
else
  ikoptions = IKoptions(obj);
end
collision_constraint_cell = {};
other_constraint_cell = {};
for i = 1:length(varargin)
  if(isa(varargin{i},'AllBodiesClosestDistanceConstraint'))
    collision_constraint_cell = [collision_constraint_cell varargin(i)];
  elseif(isa(varargin{i},'DrakeMexPointer'))
    if strcmp(varargin{i}.name,'AllBodiesClosestDistanceConstraint')
      error('Please construct MATLAB AllBodiesClosestDistanceConstraint object');
    end
  else
    other_constraint_cell = [other_constraint_cell,varargin(i)];
  end
end
if(collision_status == 0 || collision_status == 1)
  [q,info,infeasible_constraint] = inverseKin(obj,q_seed,q_nom,other_constraint_cell{:},ikoptions);
end
if(collision_status == 1)
  for i = 1:length(collision_constraint_cell)
    [collisionFlag, dist,ptsA,ptsB,idxA,idxB] = collision_constraint_cell{i}.checkConstraint(q);
    if(~collisionFlag)
      for j = 1:length(dist)
        send_status(4,0,0,sprintf('Dist from %s to %s is %f\n',...
          sendNameString(collision_constraint_cell{i},idxA),...
          sendNameString(collision_constraint_cell{i},idxB),...
          dist(j)));
      end
    end
  end
elseif(collision_status == 2)
  [q,info,infeasible_constraint] = inverseKin(obj,q_seed,q_nom,collision_constraint_cell{:},other_constraint_cell{:},ikoptions);
end
end

function name_str = sendNameString(collision_constraint,body_ind)
robotnum = collision_constraint.robot.getBody(body_ind).robotnum;
if(robotnum == 1) % atlas
  name_str = collision_constraint.robot.getBody(body_ind).linkname;
else % affordance
  name_str = collision_constraint.robot.name{robotnum};
end
end