#for i in {1..48}; do echo $i; ./flexfringe --ini ini/css-stream-active-learning.ini --aptafile=data/PAutomaC-competition_sets/${i}.pautomac.train.dat data/PAutomaC-competition_sets/$i.pautomac.train.dat; done
for i in {1..48}; do echo $i; ./flexfringe --ini ini/css-stream.ini data/PAutomaC-competition_sets/$i.pautomac.train.dat; done