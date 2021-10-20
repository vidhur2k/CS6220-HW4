# How this repo is structured

The repo contains the following files/directories:
1. `fimi01`: This refers to the FIMI03 implementation of the APRIORI algorithm by Ferenc Bodon.
2. `fimi11`: This refers to the FIMI03 implementation of the APRIORI algorithm by Christian Borgelt.
3. `data`: This directory contains the T10I4D100K and Kosarak (Hungarian News Portal Clickstream Data) datasets in a `.dat` file format.
4. `outputs`: This directory contains the outputs to all experiments run at various minimum support thresholds.

# Running Experiments
Our experiments consist of running each of the two FIM implementations on the two benchmark datasets.

In order to run *timed* experiments with the T10I4D100K dataset, run the following commands:

`time fimi01/apriori data/T10I4D100K.dat <MIN_SUPPORT_THRESHOLD> outputs/<OUTPUT_FILE_NAME>`
`time fimi11/apriori/fim_all data/T10I4D100K.dat <MIN_SUPPORT_THRESHOLD * 100> outputs/<OUTPUT_FILE_NAME>`

In order to run *timed* experiments with the Kosarak dataset, run the following commands:

`time fimi01/apriori data/kosarak.dat <MIN_SUPPORT_THRESHOLD> outputs/<OUTPUT_FILE_NAME>`
`time fimi11/apriori/fim_all data/kosarak.dat <MIN_SUPPORT_THRESHOLD * 100> outputs/<OUTPUT_FILE_NAME>`

(**NOTE:** For the Borgelt implementation, the min_support_threshold is multiplied by 100 due to the way the algorithm is scaled.)
# References
Ferenc Bodon. A fast APRIORI implementation. Informatics Laboratory, Computer and Automation Research Institute, Hungarian Academy of Sciences.

Christian Borgelt, Efficient Implementations of Apriori and Eclat.

Frequent Itemset Mining Datasets Repository. http://fimi.uantwerpen.be/data/