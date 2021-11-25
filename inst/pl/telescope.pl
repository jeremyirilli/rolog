% Simple grammar with sentences, noun/verb/participle phrases
s(s(NP,VP)) --> np(NP, Num), vp(VP, Num).

np(NP, Num) --> pn(NP, Num).
np(np(Det, N), Num) --> det(Det, Num), n(N, Num).
np(np(Det, N, PP), Num) --> det(Det, Num), n(N, Num), pp(PP).

vp(vp(V, NP), Num) --> v(V, Num), np(NP, _).
vp(vp(V, NP, PP), Num) --> v(V, Num), np(NP, _), pp(PP).

pp(pp(P, NP)) --> p(P), np(NP, _).

% Determiners, personal nouns, nouns, verbs and participles
det(det(a), sg) --> ["a"].
det(det(the), _) --> ["the"].

pn(pn(john), sg) --> ["john"].

n(n(man), sg) --> ["man"].
n(n(men), pl) --> ["men"].
n(n(telescope), sg) --> ["telescope"].

v(v(sees), sg) --> ["sees"].
v(v(see), pl) --> ["see"].
v(v(saw), _) --> ["saw"].

p(p(with)) --> ["with"].
