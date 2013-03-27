function [X] = optimizeRLFootstepPlan(biped, x0, navgoal, outputfun, updatefun, data)
  
  interactive = true
  poses = navgoal(1:6);
  time_limit = navgoal(7);
  % if nargin < 5
  %   interactive = false;
  % else
  %   interactive = true;
  % end
  % if nargin < 3
  %   outputfun = @(X, hfun) X;
  % end

  [X, foot_goals] = biped.createInitialSteps(x0, poses);

  done = false;

  while 1
    X_old = X;
    while 1
      if interactive
        [data, changed, changelist] = updatefun(data);
        if changelist.plan_commit || changelist.plan_reject
          done = true;
          break
        end

        if changelist.plan_con
          new_X = FootstepPlanListener.decodeFootstepPlan(data.plan_con);
          new_X = new_X(1);
          new_X.pos = biped.footOrig2Contact(new_X.pos, 'center', new_X.is_right_foot);
          X([X.id] == new_X.id) = new_X;
          t = num2cell(biped.getStepTimes([X.pos]));
          [X.time] = t{:};
        else
          break
        end
      end
    end
    if done
      break
    end

    [X, outputflag] = updateRLFootstepPlan(biped, X, foot_goals, time_limit, @heightfun);
    
    if isequal(size(X_old), size(X)) && all(all(abs([X_old.pos] - [X.pos]) < 0.01))
      modified = false;
    else
      modified = true;
    end
    % if modifed
      % biped.publish_footstep_plan(X);
      outputfun(X, @heightfun);
    % end
    if (~interactive && ~modified) || (done)
      break
    end
  end

  % Convert from foot center to foot origin
  for j = 1:length(X)
    X(j).pos = biped.footContact2Orig(X(j).pos, 'center', X(j).is_right_foot);
  end
  
end

function h = heightfun(xy)
  h = zeros(1, length(xy(1,:)));
end
