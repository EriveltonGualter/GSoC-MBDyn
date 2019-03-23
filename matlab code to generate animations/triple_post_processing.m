
clear all
close all

   
mov = load(['triple_pendulum.mov']);

label = mov(:,1);
nodes = unique(label);

idx_node1 = 1:size(nodes):length(label);
idx_node2 = 2:size(nodes):length(label);
idx_node3 = 3:size(nodes):length(label);

mov1 = mov(idx_node1, :);
mov2 = mov(idx_node2, :);
mov3 = mov(idx_node3, :);
t = (0:length(mov1)-1)*1.e-3;

node(1).pos = mov1(:,2:4);
node(1).euler = mov1(:,5:7);
node(1).lin_vel = mov1(:,8:10);
node(1).ang_vel = mov1(:,11:13);

node(2).pos = mov2(:,2:4);
node(2).euler = mov2(:,5:7);
node(2).lin_vel = mov2(:,8:10);
node(2).ang_vel = mov2(:,11:13);

node(3).pos = mov3(:,2:4);
node(3).euler = mov3(:,5:7);
node(3).lin_vel = mov3(:,8:10);
node(3).ang_vel = mov3(:,11:13);

figure; 
subplot(311); hold on; box on;
plot(t, node(1).pos(:,1), 'LineWidth', 2)
plot(t, node(1).pos(:,3), 'LineWidth', 2)
xlim([0 t(end)]);
title('Position along the time');
ylabel('Link 1 Position, m'); xlabel('Time, s');

subplot(312); hold on; box on;
plot(t, node(2).pos(:,1), 'LineWidth', 2)
plot(t, node(2).pos(:,3), 'LineWidth', 2)
xlim([0 t(end)]);
ylabel('Link 2 Position, m'); xlabel('Time, s');

subplot(313); hold on; box on;
plot(t, node(3).pos(:,1), 'LineWidth', 2)
plot(t, node(3).pos(:,3), 'LineWidth', 2)
xlim([0 t(end)]);
ylabel('Link 3 Position, m'); xlabel('Time, s');

print('postime','-djpeg','-noui')

figure; hold on; box on;
title('Phase Plane');
plot(node(1).pos(:,1), node(1).pos(:,3), 'LineWidth', 2);
plot(node(2).pos(:,1), node(2).pos(:,3), 'LineWidth', 2);
plot(node(3).pos(:,1), node(3).pos(:,3), 'LineWidth', 2);

print('phaseplane','-djpeg','-noui')

x1 = node(1).pos(:,1);
y1 = node(1).pos(:,3);
x2 = node(2).pos(:,1);
y2 = node(2).pos(:,3);
x3 = node(3).pos(:,1);
y3 = node(3).pos(:,3);

fsim = figure('Name','Simulation', 'NumberTitle','off'); hold on; box on;
first_frame = true;
time = 0;
tic;
while time < t(end)
    set(0, 'currentfigure', fsim);
    cla
    % Compute the position of the system at the current real world time
    x1Draw = interp1(t',x1',time')';
    y1Draw = interp1(t',y1',time')';
    x2Draw = interp1(t',x2',time')';
    y2Draw = interp1(t',y2',time')';
    x3Draw = interp1(t',x3',time')';
    y3Draw = interp1(t',y3',time')';
    
    % Update current time
    time = toc;

    Draw([x1Draw y1Draw], [x2Draw y2Draw], [x3Draw y3Draw]);
    axis(3*[-1 1 -1 1]);
    
    str = horzcat('Time = ',num2str(time,3),' s ...');
    title(str,'interpreter','latex','fontsize', 14);

    drawnow
    
    % gif utilities
    set(gcf,'color','w');   % set figure background to white
    frame = getframe(fsim);    % get desired frame
    im = frame2im(frame);   % Transform frame to image
    [imind,cm] = rgb2ind(im,256);  % Convert RGB image to indexed image
    outfile = 'doublependulum_sim.gif';    % GIF is the BEST. However, you can modify the extensions.

    % On the first loop, create the file. In subsequent loops, append.
    if first_frame
        imwrite(imind,cm,outfile,'gif','DelayTime',0,'loopcount',inf); 
        first_frame = false;
    else
        imwrite(imind,cm,outfile,'gif','DelayTime',0,'writemode','append');
    end
end 

function Draw(pos1, pos2, pos3)

    X(1) = 0;
    Y(1) = 0;
    X(2) = 2*pos1(1);
    Y(2) = 2*pos1(2);
    
    th = atan2(pos2(2)-Y(2),pos2(1)-X(2));
    L = sqrt(X(2)*X(2) + Y(2)*Y(2));
    X(3) = pos2(1) + 0.5*L*cos(th);
    Y(3) = pos2(2) + 0.5*L*sin(th);

    th = atan2(pos3(2)-Y(3),pos3(1)-X(3));
    X(4) = pos3(1) + 0.5*L*cos(th);
    Y(4) = pos3(2) + 0.5*L*sin(th);

    plot(X, Y, 'LineWidth', 3);
    axis equal
end


