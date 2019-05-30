% Generate
vertex(X) :- X = 1..N, vertices(N).
{ pmc(X) : vertex(X) } = K :- size(K).
% Define
edge(X,Y) :- edge(Y,X).
c(X,X) :- vertex(X).
c(X,Y) :- edge(X,Y).
c(X,Y) :- c(X,Z), not pmc(Z), edge(Z,Y).
d(X,Y) :- vertex(X), not c(X,Y), pmc(Y).
% Test
:- pmc(X), pmc(Y), not c(X,Y).
:- not pmc(X), 0 = #count { vertex(Y) : d(X,Y) }, X = 1..N, vertices(N).
% Display
#show pmc/1.