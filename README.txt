ANAPTYKSH LOGISMIKOY GIA PLHROFORIAKA SYSTHMATA, ASKHSH 1
OMADA:	KALANTZIS GRIGORIOS	1115201400055
	KONOMIS ARIS 		1115201400073


H ergasia grafthke se C kai ulopoihthike se perivallon Windows 10, enw elegxthike h leitourgikothta ths sta mhxanhmata ths sxolhs mesw putty.
Meta th metaglwttish (make) to programma ekteleitai me thn entolh ./project .

Se prwth fash to programma den exei orismata kai oi eisodoi tou (2 pinakes, arithmos grammwn kai sthlwn) dinontai authaireta sth main().
Oi duo pinakes apothikeuontai sth mnhmh kata sthles, dhladh ws duo monodiastatoi pinakes pou periexoun mia mia tis sthles tou kathe pinaka.
Gia logous eukolias thewroume pws oi pinakes exoun mia sthlh mono wste na exoume arketa megales sthles gia th join.
Oi pinakes einai arketa megalou megethous(taksews misou ekatommuriou) kai gemizontai me th vohtheia ths rand().
Kataskeuazontai epishs 2 pinakes me ta row-ids twn pinakwn, dhladh arxika exoume pinakes opou P[i]=i+1.

Oson afora th kuria leitourgia tou programmatos, dhladh thn ulopoihsh ths Radix Hash Join, exoume akolouthisei ta vhmata ths ekfwnhshs.
Thewroume oti to join twn 2 parapanw pinakwn ginetai ws pros th prwth sthlh tous.

Etsi, ftiaxnoume 2 struct relation, ena gia kathe pinaka opou o pinakas apo tuples pou periexontai sto struct tha exei megethos oso o arithmos 
twn grammwn tous. Sth prwth sthlh twn tuples tou kathe relation apothikeuetai h timh h1 (h timh pou epistrefei h sunarthsh katakermatismou 1) 
gia th sugkekrimenh timh, enw sth deuterh sthlh apothikeuetai apla h idia h timh.
Kataskeuazontai me th vohtheia twn parapanw pinakwn ta istogrammata kai ta athroistika istogrammata tou kathenos.
Epeita, ta 2 struct relation pou exoume dhmiourghsei taksinomountai mesw ths quicksort dhmiourgwntas buckets gia th sunarthsh h1. Tautoxrona
metavallontai kai oi times stous pinakes row-ids pou exoume antistoixismenous.

Epeita, xtizoume to eurethrio sto mikrotero apo tous 2 pinakes ws ekshs: allazoun oi times ths prwths sthlhs tou mikroterou pinaka kai antikathistantai
apo tis times pou prokuptoun efarmozontas thn h2 (opou h2 = (x*2654435761) % 101) panw sthn timh ths deuterhs sthlhs. Etsi, fortwnetai eks oloklhrou
olos o pinakas sth mnhmh (R sto sxhma ths ekfwnhshs) kai oxi ena ena ta buckets tou. H prospelash tou pinaka, dhladh o diaxwrismos twn buckets tou,
ginetai me th vohtheia twn istogrammatwn pou exoun upologistei. Gia ton pinaka Chain exei uiotheththei h logikh ths ekfwnhshs opou oi deiktes deixnoun
pros ta panw kai kathe akolouthia teleiwnei deixnontas se auto (gia praktikous logous to 1o keli exei timh -1). O pinakas Bucket exei megethos oso to 
plithos twn buckets ths h1 epi to plithos twn buckets ths h2, dhladh sthn periptwsh mas (2^n)*101 .

Telos, gia na paroume ta apotelesmata sarwnoume ena ena ta buckets tou megalou pinaka (autou xwris eurethrio) kai gia kathe stoixeio tou diasxizetai h
domh tou eurethriou (tou mikrou pinaka) mesa sta oria tou bucket autou. Etsi, elegxoume gia poia stoixeia twn arxikwn pinakwn prokuptoun idies times an 
efarmosoume thn h1 alla k idia gia thn h2. An isxuei auto, afou elegxthei pws einai isa kai ta stoixeia, ginetai eisagwgh sth lista apotelesmatwn ths 
duadas twn grammwn opou entopisthke isothta. Auto ginetai me th vohtheia twn antistoixismenwn se autous pinakes pou kratane ta row-ids. H domh ths listas
opou kataxwrountai oles oi zeukseis apoteleitai apo lista me structs (struct result) opou periexetai enas buffer apo tupples megethous 1MB kai molis
gemizei to trexon dhmiourgeitai epomeno struct.
Ta apotelesmata tha ektupwnontai sto arxeio eksodou "output.txt" (zeugh row-ids). Ektupwnontai akoma xronoi ekteleshs kai to plithos twn joins pou eginan.    
