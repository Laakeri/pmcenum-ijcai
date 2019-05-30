% Generate
vertex(X) :- X = 1..N, vertices(N).
hyper(X) :- X = 1..N, hyperedges(N).
{ selected(X) : hyper(X) } = K :- size(K).
{ pmc(X) : vertex(X) }.
% Define
covered(X) :- inedge(X, Y), selected(Y).
edge(X,Y) :- edge(Y,X).
c(X,X) :- vertex(X).
c(X,Y) :- edge(X,Y).
c(X,Y) :- c(X,Z), not pmc(Z), edge(Z,Y).
d(X,Y) :- vertex(X), not c(X,Y), pmc(Y).
% Test
:- not covered(X), pmc(X).
:- pmc(X), pmc(Y), not c(X,Y).
:- not pmc(X), 0 = #count { vertex(Y) : d(X,Y) }, X = 1..N, vertices(N).
% Display
#show pmc/1.