# OntExt
Implementation of the OntExt algorithm.
The OntExt algorithm is a way to find new knowledge in existing ontological knowledge bases.
Inputs for the algorithm are: a SVO file (a tab-separated RDF of Subject/Verbal Phrase/Object/count),
a list of categories to be searched,
and a list of instances for each category.

The algorithm will then find co-occurring contexts and promote them to new relations
in the knowledge base, along with seed instances for those relations.

OntExt was initially develop for the NELL system of the [Read the Web project](http://rtw.ml.cmu.edu/rtw/).
This implementation is an ongoing improvement over the original algorithm.

## PrOntExt
The current implementation of OntExt seeks to integrate with another NELL component, the Prophet.
The union of these two components is called the PrOntExt.
This implementation is built upon this integration.
More information can be found in the references.

### References
OntExt was initially proposed by T. Mohamed, E. R. Hruscha Jr and T. Mitchell in [Discovering Relations between Nouns and Categories](http://aclanthology.info/papers/discovering-relations-between-noun-categories).

A new version was shown by P. H. Barchi and E. R. Hruschka Jr in Barchi's master thesis,
"Expansão de Ontologia através de leitura de máquina contínua", 2014, Universidade Federal de São Carlos (UFSCar), in Portuguese.

This specific implementation was built by D. Brognara and E. R. Hruschka Jr in Brognara's Bachelor Final Project,
"Aprimoramento na geração de matrizes de co-ocorrência", 2016, Universidade Federal de São Carlos (UFSCar), in Portuguese.
