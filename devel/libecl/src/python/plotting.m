close all
clear
path(path, '/d/proj/bg/ior_fsenter2/grane/ressim/hstruct/2008a/e100/EnKF/sf02rg01/Seismic/matlab_AI')


load VP_BASE.dat
load VS_BASE.dat
load VP_MON.dat
load VS_MON.dat
load VP_DIFF.dat
load D_SGAS.dat


subplot(3,2,1)
plot(D_SGAS, VP_DIFF, '.k'), xlabel('SGAS'), ylabel('VP'), title('Monitor - Base')

x = 0:100:4400;

subplot(3,2,3)
[rows, cols, vals] = find(VP_BASE);
rhist(vals, x), xlabel('VP'), ylabel('Frequency'), title('Base')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','r','EdgeColor','w')
subplot(3,2,4)
[rows, cols, vals] = find(VS_BASE);
rhist(vals, x), xlabel('VS'), ylabel('Frequency'), title('Base')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','r','EdgeColor','w')

subplot(3,2,5)
[rows, cols, vals] = find(VP_MON);
rhist(vals, x), xlabel('VP'), ylabel('Frequency'), title('Monitor')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','g','EdgeColor','w')
subplot(3,2,6)
[rows, cols, vals] = find(VS_MON);
rhist(vals, x), xlabel('VS'), ylabel('Frequency'), title('Monitor')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','g','EdgeColor','w')

load B_SGAS.dat
load B_SWAT.dat
load B_PORO.dat
load B_PRESS.dat
load M_SGAS.dat
load M_SWAT.dat
load M_PORO.dat
load M_PRESS.dat

[Vp_b, Vs_b, Rh_b] = PEM_main(B_PRESS, B_PORO, B_SWAT, B_SGAS);
[Vp_m, Vs_m, Rh_m] = PEM_main(M_PRESS, M_PORO, M_SWAT, M_SGAS);

V_DIFF = Vp_m - Vp_b;

figure
subplot(3,2,1)
plot(M_SGAS - B_SGAS, V_DIFF, '.k'), xlabel('SGAS'), ylabel('VP'), title('Monitor - Base')
subplot(3,2,2)
plot(M_SWAT - B_SWAT, V_DIFF, '.k'), xlabel('SWAT'), ylabel('VP'), title('Monitor - Base')

subplot(3,2,3)
[rows, cols, vals] = find(Vp_b);
rhist(vals, x), xlabel('VP'), ylabel('Frequency'), title('Base')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','r','EdgeColor','w')

subplot(3,2,4)
[rows, cols, vals] = find(Vs_b);
rhist(vals, x), xlabel('VS'), ylabel('Frequency'), title('Base')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','r','EdgeColor','w')

subplot(3,2,5)
[rows, cols, vals] = find(Vp_m);
rhist(vals, x), xlabel('VP'), ylabel('Frequency'), title('Monitor')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','g','EdgeColor','w')

subplot(3,2,6)
[rows, cols, vals] = find(Vs_m);
rhist(vals, x), xlabel('VS'), ylabel('Frequency'), title('Monitor')
h = findobj(gca,'Type','patch');
set(h,'FaceColor','g','EdgeColor','w')

figure

p = [M_SGAS - B_SGAS, M_SWAT - B_SWAT, Vp_m - Vp_b];

plot3k({p(:,1) p(:,2) p(:,3)}, gradient(p(:,3)), [-0.5 0.5], {'o',2}, 11,{'Gassmann SGAS SWAT VP'}, 'FontName','Arial','FontSize',18,'FontWeight','Bold'), 

xlabel('SGAS'), ylabel('SWAT'), zlabel('VP'), axis square


save matlab_output/V_DIFF.dat V_DIFF -ASCII -DOUBLE
save matlab_output/VP_BASE.dat Vp_b -ASCII -DOUBLE
save matlab_output/VP_MON.dat Vp_m -ASCII -DOUBLE
