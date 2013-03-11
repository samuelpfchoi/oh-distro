function [Xright, Xleft] = optimizeFreeFootsteps(biped, poses, interactive)

X = interp1([1:length(poses(1,:))]', poses', [1:0.5:length(poses(1,:))]')';
total_steps = length(X(1,:));

fixed_steps = repmat({[]}, total_steps, 2);

ndx_r = int32([1, 2, 4:2:(total_steps-1), total_steps]);
ndx_l = int32([1:2:(total_steps-1), total_steps]);
for p = poses
  [~,j] = min(sum((X - repmat(p, 1, length(X(1,:)))).^2));
  if find(ndx_r == j)
    [fixed_steps{j,1},~] = biped.stepLocations(X(:,j));
  end
  if find(ndx_l == j)
    [~, fixed_steps{j,2}] = biped.stepLocations(X(:,j));
  end
end
[X, outputflag] = updateFastFootsteps(biped, X, fixed_steps, ndx_r, ndx_l, @heightfun);


done = false;
function set_done()
  done = true;
end

h = figure(22);
set(h, 'WindowButtonDownFcn', @(s, e) mouse_down_handler(h));
set(h, 'WindowButtonUpFcn', @(s, e) mouse_up_handler(h));
uicontrol('style', 'pushbutton', 'String', 'Done', 'Callback', @(s, e) set_done());

drag_ndx = 1;


while 1
  modified = 0;
  [Xright, Xleft] = biped.stampedStepLocations(X);
  ndx_fixed = find(any(cellfun(@(x) ~isempty(x),fixed_steps),2));
  [d_r, r_r] = biped.stepDistance(Xright(1:6,1:(end-1)), Xright(1:6,2:end), 0);
  [d_l, r_l] = biped.stepDistance(Xleft(1:6,1:(end-1)), Xleft(1:6,2:end), 0);
  for n = 1:(length(ndx_fixed)-1)
    num_steps = ndx_fixed(n+1) - ndx_fixed(n);
    dist = max(sum(d_r(ndx_fixed(n):(ndx_fixed(n+1)-1))),...
               sum(d_l(ndx_fixed(n):(ndx_fixed(n+1)-1))));
    rot = max(sum(r_r(ndx_fixed(n):(ndx_fixed(n+1)-1))),...
              sum(r_l(ndx_fixed(n):(ndx_fixed(n+1)-1))));
    if  ((dist > num_steps * biped.max_step_length * .4 ...
          || rot > num_steps * biped.max_step_rot * .4) ...
         && num_steps > 1)
      j = ndx_fixed(n);
      fixed_steps(j+3:end+2,:) = fixed_steps(j+1:end,:);
      fixed_steps([j+1,j+2],:) = repmat({[]}, 2, 2);
      X(:,j+3:end+2) = X(:,j+1:end);
      X(:,[j+1,j+2]) = interp1([0,1], X(:,[j,j+1])', [1/3, 2/3])';
      modified = 1;
      if drag_ndx > j
        drag_ndx = drag_ndx + 2;
      end
      break
    elseif (dist < num_steps * biped.max_step_length / 4 ...
            && rot < num_steps * biped.max_step_rot / 4) ...
        && (num_steps > 2)
      j = ndx_fixed(n);
      fixed_steps(j+1:end-2,:) = fixed_steps(j+3:end,:);
      fixed_steps(end-1:end,:) = [];
      X(:,j+1:end-2) = X(:,j+3:end);
      X(:,end-1:end) = [];
      modified = 1;
      if drag_ndx > j+1
        drag_ndx = drag_ndx - 2;
      end
      break
    end
  end
  total_steps = length(X(1,:));
  ndx_r = int32([1, 2, 4:2:(total_steps-1), total_steps]);
  ndx_l = int32([1:2:(total_steps-1), total_steps]);
  
  [X, outputflag] = updateFastFootsteps(biped, X, fixed_steps, ndx_r, ndx_l, @heightfun);
    
  if outputflag ~= 1 && outputflag ~= 2
    modified = 1;
  end
  
  [Xright, Xleft] = biped.stepLocations(X, ndx_r, ndx_l);
  figure(22)
  cla
  hold on
  plotFootstepPlan(X, Xright, Xleft);
  if exist('vs.obj_collection_t')
    plot_lcm_poses(Xright(1:3,:)', Xright(6:-1:4,:)', 1, 'Foot Steps (right)', 4, 1, 0, -1);
    plot_lcm_poses(Xleft(1:3,:)', Xleft(6:-1:4,:)', 2, 'Foot Steps (left)', 4, 1, 0, -1);
  end
  hold on
  for j = 1:length(ndx_r)
    if ~isempty(fixed_steps{ndx_r(j), 1})
      plot(Xright(1,j), Xright(2,j), 'go', 'MarkerSize', 8, 'MarkerFaceColor', 'g')
    end
  end
  for j = 1:length(ndx_l)
    if~isempty(fixed_steps{ndx_l(j), 2})
      plot(Xleft(1,j), Xleft(2,j), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r')
    end
  end
  drawnow
  hold off
  if (~interactive && ~modified) || (done)
    break
  end
end


function mouse_down_handler(hFig)
  ax = gca;
  mouse_pt = get(ax, 'CurrentPoint');
  mouse_x = mouse_pt(1,1);
  mouse_y = mouse_pt(1,2);
  dist_r = sum((Xright(1:2,:) - repmat([mouse_x; mouse_y], 1, length(Xright(1,:)))).^2, 1);
  dist_l = sum((Xleft(1:2,:) - repmat([mouse_x; mouse_y], 1, length(Xleft(1,:)))).^2, 1);
  [min_r, step_r] = min(dist_r);
  [min_l, step_l] = min(dist_l);
  if min_r < min_l
    current_foot = 1;
    drag_ndx = ndx_r(step_r);
    closest_point = Xright(:,step_r);
  else
    current_foot = 2;
    drag_ndx = ndx_l(step_l);
    closest_point = Xleft(:,step_l);
  end
  
  set(hFig, 'WindowButtonMotionFcn', @(s, e) mouse_drag_handler(hFig, current_foot, closest_point));
end

function mouse_drag_handler(hFig, current_foot, closest_point)
  ax = gca;
  mouse_pt = get(ax, 'CurrentPoint');
  mouse_x = mouse_pt(1,1);
  mouse_y = mouse_pt(1,2);
  fixed_steps{drag_ndx, current_foot} = [[mouse_x; mouse_y]; closest_point(3:end)];
end

function mouse_up_handler(hFig)
  set(hFig, 'WindowButtonMotionFcn', '');
end

  function h = heightfun(xy)
    h = zeros(1, length(xy(1,:)));
%     h(xy(1,:) > 0.5 & xy(1,:) < 1 & xy(2,:) > -0.25 & xy(2,:) < 0.25) = -0.6;
%     h(xy(1,:) > 0.7 & xy(1,:) < .8 & xy(2,:) > -0.05 & xy(2,:) < 0.05) = 0;
  end

end
