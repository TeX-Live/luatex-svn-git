
prologues:=3;

path p;
p = (0,0){right}..(20,100)..(50,60)--(75,50)...(25,25)..cycle;

beginfig(1);
fill p withcolor (1,0,0);
pickup pencircle scaled 2;
draw p dashed (evenly scaled 4);
endfig;


beginfig(2);
label ("whatever",(0,0)) scaled 10 rotated 24 ;
endfig;


beginfig(3);
fill p withcolor (1,0,0);
setbounds currentpicture to unitsquare scaled 20;
endfig;

end.
