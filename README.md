#DistroSP

Team Name: Sky Summit

Team Members:

Essam Ahmed Maher AlNaggar - elnagar.essam@gmail.com - Alexandria University - Computer and System Engieering Department - Undergrad.

Mohamed Ahmed Aboelhassan - muhammad.aboelhassan@gmail.com - Alexandria University - Computer and System Engieering Department - Undergrad.

Ayman Khaled Geneidy - ayman.geneidy@gmail.com - Alexandria University - Computer and System Engieering Department - Undergrad.

Description:

The system solves the problem of shortest path on a dynamic graph, in which the use interacts with the system to either order a modification on the graph or query the existing graph for a certain path.

When the system first runs, it expects to be given the description of the graph as a list of edges.

The three main capabilties after then are:

Add a new edge: the system then loops over all pair of nodes (in parallel) an checks whether this new edge would affect the shortest path between them or not, and update the shortest path acoordingly.

Delete an Existing edge: the system checks for each node whether it has affected any of its paths, and thus decides if a node requires to be recomputed completeley or not.

Make a shortest path query: the system responds with the shortest path between the pair of nodes given, if none found, returns -1.