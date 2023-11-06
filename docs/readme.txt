Thicket is a file tree merging utility that allows to build a single big directory as a result of merging several other directories. Thicket was conceived as a tool for assembling a single C/C++ source tree from multiple software componets, but it may be used for other programming languagies or even for any purpose unrelated to programming.

The main idea of Thicket is quite similar to «union» or «overlay» filesystems, but Thicket is more abstract and more strict. In case of union fs a regular file in foreground fs just hides the same regular file in background fs. In contrast with union fs, Thicket prohibits any discrepancy: all regular file «versions» shall point to the same «final source» file. 

Thicket also separates the process of buiding an abstract result tree from «materialization» technique. Indeed, the result tree may be built in various ways. 

The first way is just copying the files from source subtrees. This technique, however, has an obvious drawback: any modification of files in the «big» result tree is not propagated to the initial source files.

The second way (currently implemented in Thicket) is to symlink target files or directories wherever possible. If a directory is a result of merging several «final» source directories, then such a directory is «materialized» as a «real» filesystem dirctory. However, if a directory has just a single «final source», a symlink to the source directory is created. Regular files are always symlinked.

Currently we do not consider similar materialization technique using hard links (instead of synbolic links)  because hard links may dangerously fool source control systems. However, such a technique would be sometimes useful if all Thicket artefacts were consistently ignored by SCM.

Theoretically it is also possible to «materialize» the resulting «big tree» by mounting union (overlay) filesystems (if the operating system supports such fuilesystems, e.g. Linux). Currently we do not see any practical reason to implement union (overlay) materialization, but it is definetely possible. 

While building the result tree, we distinguish between «real» filesystem nodes (we call them «final» nodes) and «virtual» or «reference» nodes (that shall be materialized). Virtual (reference)  node is a «mountpoint» node or any descendant of «mountpoint» node. 

A mountpoint node is specified by a special text file (utf-8 encoded) which has a name like <mountpoint_node_name>.thicket_mount.txt. Every line in mountpoint description file defines one «target» subtree. All target subtrees are intended to be merged into this mountpoint node. Every line (in cescription file) is just a filesystem path (to the target subtree) relative to the parent directory of the description file.

Note, a target subtree may be a final node or may be itself a reference node. The reference chain of indirections, however, shall finaly point to some final nodes. So every refernce node has a union of «final targets» that are obtained by consecutive dereferencing through the reference chain. All «final targets» are final («real filesystem») nodes, i.e. directories or regular files.

The mountpoint description file says that <mountpoint_node_name> virtual node exists here and can be materialized here (i.e. alongside the description file itself).

Note, we use the term «target» assuming a «reference target», or a referent. But at the same time the «target» may be also treated as one of the «sources» used while building our resulting «big» tree. Interchangable use of terms «target» and «source» may be confusing, but this is a nature of things.


